// The MIT License (MIT)
//
// Copyright (c) 2015 Johan Johansson
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// DXGI_capture_lib.cpp : The entry point for the application.
//
// NOTES:
// The first time that `AcquireNextFrame` is called, the resulting frame is very likely to be all
// black, with no error. Following frames are not though. Why? I don't know. However, since this lib
// will not just take a single screenshot, but rather stream the screen, this doesn't really matter.

#include "DXGIManager.hpp"
#include <functional>
#include <chrono>
#include <thread>

// TODO: add tests!!

// TODO: Add member vars for frame buf and buf size, to avoid malloc for every frame

// The default format of B8G8R8A8 gives a pixel-size of 4 bytes
#define PIXEL_SIZE 4
#define PIXEL uint32_t
#define REFRESH_MAX_TRIES 4


vector<CComPtr<IDXGIOutput>> get_adapter_outputs(IDXGIAdapter1* adapter) {
	vector<CComPtr<IDXGIOutput>> out_vec;
	for (UINT16 i = 0; ; i++) {
		CComPtr<IDXGIOutput> output;
		HRESULT hr = adapter->EnumOutputs(i, &output);
		if (FAILED(hr)) {
			break;
		}

		DXGI_OUTPUT_DESC out_desc;
		output->GetDesc(&out_desc);
		if (out_desc.AttachedToDesktop) {
			out_vec.push_back(output);
		}
	}

	return out_vec;
}

DuplicatedOutput::DuplicatedOutput(ID3D11Device* device,
	ID3D11DeviceContext* context,
	IDXGIOutput1* output,
	IDXGIOutputDuplication* output_dup):
		m_device(device),
		m_device_context(context),
		m_output(output),
		m_dxgi_output_dup(output_dup) { }
DuplicatedOutput::~DuplicatedOutput() {
	m_device.Release();
	m_device_context.Release();
	m_output.Release();
	m_dxgi_output_dup.Release();
}

DXGI_OUTPUT_DESC DuplicatedOutput::get_desc() {
	DXGI_OUTPUT_DESC desc;
	m_output->GetDesc(&desc);
	return desc;
}

// Returns status of AcquireNextFrame. May well be DXGI_ERROR_ACCESS_LOST due to mode change.
HRESULT DuplicatedOutput::get_frame(IDXGISurface1** out_surface, uint32_t timeout) {
	IDXGIResource* frame_resource;
	DXGI_OUTDUPL_FRAME_INFO frame_info;
	TRY_RETURN(m_dxgi_output_dup->AcquireNextFrame(timeout, &frame_info, &frame_resource));

	ID3D11Texture2D* frame_texture;
	frame_resource->QueryInterface(__uuidof(ID3D11Texture2D),
		reinterpret_cast<void **>(&frame_texture));
	frame_resource->Release();

	D3D11_TEXTURE2D_DESC texture_desc;
	frame_texture->GetDesc(&texture_desc);

	// Configure the description to make the texture readable
	texture_desc.Usage = D3D11_USAGE_STAGING;
	texture_desc.BindFlags = 0;
	texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texture_desc.MiscFlags = 0;

	ID3D11Texture2D* readable_texture;
	TRY_RETURN(m_device->CreateTexture2D(&texture_desc, NULL, &readable_texture));

	// Lower priorities causes stuff to be needlessly copied from gpu to ram, causing huge
	// fluxuations on some systems.
	readable_texture->SetEvictionPriority(DXGI_RESOURCE_PRIORITY_MAXIMUM);

	m_device_context->CopyResource(readable_texture, frame_texture);
	frame_texture->Release();

	IDXGISurface1* texture_surface;
	readable_texture->QueryInterface(__uuidof(IDXGISurface1),
		reinterpret_cast<void **>(&texture_surface));
	readable_texture->Release();

	m_dxgi_output_dup->ReleaseFrame();

	*out_surface = texture_surface;
	return S_OK;
}

void DuplicatedOutput::release_frame() {
	m_dxgi_output_dup->ReleaseFrame();
}

bool DuplicatedOutput::is_primary() {
	DXGI_OUTPUT_DESC output_desc;
	m_output->GetDesc(&output_desc);
	MONITORINFO monitor_info;
	monitor_info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(output_desc.Monitor, &monitor_info);

	return monitor_info.dwFlags & MONITORINFOF_PRIMARY;
}

DXGIManager::DXGIManager(): m_capture_source(0),
	m_frame_buf(NULL),
	m_frame_buf_size(0),
	m_timeout(100)
{
	SetRect(&m_output_rect, 0, 0, 0, 0);
}

DXGIManager::~DXGIManager() {
	free(m_frame_buf);
}

void DXGIManager::set_capture_source(uint16_t cs) {
	m_capture_source = cs;
	refresh_output();
}
uint16_t DXGIManager::get_capture_source() {
	return m_capture_source;
}

void DXGIManager::set_timeout(uint32_t timeout) {
	m_timeout = timeout;
}

void DXGIManager::setup() {
	refresh_output();
}

void DXGIManager::gather_output_duplications() {
	clear_output_duplications();

	CComPtr<IDXGIFactory1> factory;
	TRY_PANIC(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&factory)));

	// Getting all adapters
	vector<CComPtr<IDXGIAdapter1>> adapters;
	CComPtr<IDXGIAdapter1> n_adapter;
	for (UINT16 i = 0; factory->EnumAdapters1(i, &n_adapter) != DXGI_ERROR_NOT_FOUND; i++) {
		adapters.push_back(n_adapter);
		n_adapter.Release();
	}
	// Iterating over all adapters to get all outputs
	for (auto& adapter : adapters) {
		vector<CComPtr<IDXGIOutput>> outputs = get_adapter_outputs(adapter);
		if (outputs.size() == 0) {
			continue;
		}

		// Creating device for each adapter that has the output
		CComPtr<ID3D11Device> d3d11_device;
		CComPtr<ID3D11DeviceContext> device_context;
		D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_9_1;
		TRY_PANIC(D3D11CreateDevice(adapter,
			D3D_DRIVER_TYPE_UNKNOWN,
			NULL, 0, NULL, 0,
			D3D11_SDK_VERSION,
			&d3d11_device,
			&feature_level,
			&device_context));

		for (CComQIPtr<IDXGIOutput1> output : outputs) { // CComQIPtr
			CComQIPtr<IDXGIDevice1> dxgi_device = d3d11_device; //CComQIPtr
			CComPtr<IDXGIOutputDuplication> duplicated_output;
			HRESULT hr = output->DuplicateOutput(dxgi_device, &duplicated_output);
			if (FAILED(hr)) {
				continue;
			}

			m_out_dups.push_back(
				DuplicatedOutput(d3d11_device,
					device_context,
					output,
					duplicated_output));
		}
	}
}

// Update all output duplications in manager and refresh the capture source, m_output_duplication.
// Returns whether succeded refreshing.
bool DXGIManager::refresh_output() {
	// Will retry getting specified output. Will fail after too many tries. There is likely
	// a fullscreen app with restricted access giving us E_ACCESSDENIED
	for (uint8_t i = 0; i < REFRESH_MAX_TRIES; i++) {
		gather_output_duplications();
		m_output_duplication = get_output_duplication();
		if (m_output_duplication == NULL) {
			std::this_thread::sleep_for(std::chrono::milliseconds(400));
		} else {
			return true;
		}
	}
	return false;
}

// Returns whether new allocation was made
bool DXGIManager::update_buffer_allocation() {
	RECT output_rect = get_output_rect();
	size_t output_width = output_rect.right - output_rect.left;
	size_t output_height = output_rect.bottom - output_rect.top;
	size_t buf_size = output_width * output_height * PIXEL_SIZE;
	if (m_frame_buf_size != buf_size) {
		m_frame_buf_size = buf_size;
		if (m_frame_buf != NULL) {
			free(m_frame_buf);
		}
		m_frame_buf = (BYTE*)malloc(m_frame_buf_size);
		return true;
	}
	return false;
}

RECT DXGIManager::get_output_rect() {
	DXGI_OUTPUT_DESC output_desc = m_output_duplication->get_desc();
	return output_desc.DesktopCoordinates;
}

CaptureResult DXGIManager::get_output_data(BYTE** out_buf, size_t* out_buf_size) {
	IDXGISurface1* frame_surface;
	HRESULT hr = m_output_duplication->get_frame(&frame_surface, m_timeout);

	if (hr == DXGI_ERROR_ACCESS_LOST) {
		// Access lost, refresh the output so the next call won't fail
		// TODO: handle refresh failure
		if (!refresh_output()) {
			return CR_REFRESH_FAILURE;
		}
		return CR_ACCESS_LOST;
	} else if (hr == E_ACCESSDENIED) {
		return CR_ACCESS_DENIED;
	} else if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
		return CR_TIMEOUT;
	} else if (FAILED(hr)) {
		// Sometimes when modes are changed or something, AcquireNextFrame complains about
		// DXGI_ERROR_INVALID_CALL, the previous frame was not released. This is not the
		// case though, and to accomodate for the change in modes we refresh outputs.
		refresh_output();
		return CR_FAIL;
	}
	
	DXGI_MAPPED_RECT mapped_surface;
	hr = frame_surface->Map(&mapped_surface, DXGI_MAP_READ);
	if (FAILED(hr)) {
		frame_surface->Release();
		return CR_FAIL;
	}

	DXGI_OUTPUT_DESC output_desc = m_output_duplication->get_desc();
	RECT output_rect = output_desc.DesktopCoordinates;
	size_t output_width = output_rect.right - output_rect.left;
	size_t output_height = output_rect.bottom - output_rect.top;
	// Set origin of `output_rect` to (0, 0)
	OffsetRect(&output_rect, -output_rect.left, -output_rect.top);

	// The Pitch may contain padding, wherefore this is not necessarily pixels per row
	size_t map_pitch_n_pixels = mapped_surface.Pitch / PIXEL_SIZE;

	update_buffer_allocation();

	// Get address offset for source pixel from destination row and column
	// Order: 90deg, 180, 270
	std::function<size_t(size_t, size_t)> ofsetters [] = {
		// [&](size_t row, size_t col) { return row * map_pitch_n_pixels + col; },
		[&](size_t row, size_t col) { return (output_width-1-col) * map_pitch_n_pixels; },
		[&](size_t row, size_t col) {
			return (output_height-1-row) * map_pitch_n_pixels + (output_width-col-1); },
		[&](size_t row, size_t col) {
			return col * map_pitch_n_pixels + (output_height-row-1); }};

	if (output_desc.Rotation == DXGI_MODE_ROTATION_IDENTITY ||
		output_desc.Rotation == DXGI_MODE_ROTATION_UNSPECIFIED) // Assume no rotation
	{
		// Plain copy by byte
		size_t out_row_size = output_width * PIXEL_SIZE;
		for (size_t row_n = 0; row_n < output_height; row_n++) {
			memcpy(m_frame_buf + row_n * out_row_size,
				mapped_surface.pBits + row_n * mapped_surface.Pitch,
				out_row_size);
		}
	} else if (output_desc.Rotation != DXGI_MODE_ROTATION_UNSPECIFIED) {
		auto& ofsetter = ofsetters[output_desc.Rotation - 2]; // 90deg = 2 -> 0
		PIXEL* src_pixels = (PIXEL*)mapped_surface.pBits;
		PIXEL* dest_pixels = (PIXEL*)m_frame_buf;
		for (size_t dst_row = 0; dst_row < output_height; dst_row++) {
			for (size_t dst_col = 0; dst_col < output_width; dst_col ++) {
				*(dest_pixels + dst_row * output_width + dst_col) =
					*(src_pixels + ofsetter(dst_row, dst_col));
			}
		}
	}

	frame_surface->Unmap();
	frame_surface->Release();
	
	*out_buf = m_frame_buf;
	*out_buf_size = m_frame_buf_size;
	return CR_OK;
}

void DXGIManager::clear_output_duplications() {
	m_output_duplication = NULL;
	m_out_dups.clear();
}

// If there are no output duplications, or specified monitor is not found, return NULL
DuplicatedOutput* DXGIManager::get_output_duplication() {
	size_t n_out_dups = m_out_dups.size();
	if (n_out_dups != 0) {
		if (n_out_dups > m_capture_source)
			return &m_out_dups[m_capture_source];
		else
			return &m_out_dups[0];
		/*if (m_capture_source == 0 || n_out_dups < m_capture_source) { // Find the primary output
			for (size_t i = 0; i < n_out_dups; i++) {
				if (m_out_dups[i].is_primary()) {
					return &m_out_dups[i];
				}
			}
		} else {
			return &m_out_dups[m_capture_source - 1];
		}*/
	}
	return NULL;
}