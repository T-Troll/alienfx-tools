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

#include <Windows.h>
#include <atlbase.h>
#include <DXGI1_2.h>
#include <d3d11.h>
#include <Wincodec.h>
#include <vector>
#include <memory>
#include <cstdlib>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

// printf(#expr " failed with error: %x\n", e); use for debugging
// Try performing the expression with return type HRESULT. If the result is a failure, return it.
#define TRY_RETURN(expr) { \
	HRESULT e = expr; \
	if (FAILED(e)) { \
		return e; \
	} \
}
// -''- If the result is a failure, throw it as exception.
#define TRY_EXCEPT(expr) { \
	HRESULT e = expr; \
	if (FAILED(e)) { \
		throw e; \
	} \
}
// -''- If the result is a failure, panic.
#define TRY_PANIC(expr) { \
	HRESULT e = expr; \
	if (FAILED(e)) { \
		printf("`" #expr "` failed with code %xn", e); \
		std::exit(e); \
	} \
}

using std::vector;

enum CaptureResult {
	CR_OK = 0,
	// Could not duplicate output, access denied. Might be in protected fullscreen.
	CR_ACCESS_DENIED,
	// Access to the duplicated output was lost. Likely, mode was changed
	CR_ACCESS_LOST,
	// Error when trying to refresh outputs after failure.
	CR_REFRESH_FAILURE,
	// AcquireNextFrame timed out.
	CR_TIMEOUT,
	// General/Unexpected failure
	CR_FAIL,
};


vector<CComPtr<IDXGIOutput>> get_adapter_outputs(IDXGIAdapter1* adapter);

class DuplicatedOutput {
public:
	DuplicatedOutput(ID3D11Device* device,
		ID3D11DeviceContext* context,
		IDXGIOutput1* output,
		IDXGIOutputDuplication* output_dup);
	~DuplicatedOutput();
	DXGI_OUTPUT_DESC get_desc();
	HRESULT get_frame(IDXGISurface1** out_surface, uint32_t timeout);
	void release_frame();
	bool is_primary();
private:
	CComPtr<ID3D11Device> m_device;
	CComPtr<ID3D11DeviceContext> m_device_context;
	CComPtr<IDXGIOutput1> m_output;
	CComPtr<IDXGIOutputDuplication> m_dxgi_output_dup;
};

class DXGIManager {
public:
	DXGIManager();
	~DXGIManager();
	void setup();
	void set_capture_source(UINT16 cs);
	UINT16 get_capture_source();
	void set_timeout(uint32_t timeout);
	RECT get_output_rect();
	bool refresh_output();
	CaptureResult get_output_data(BYTE** out_buf, size_t* out_buf_size);
private:
	// returns whether allocation was updated
	bool update_buffer_allocation();
	void gather_output_duplications();
	DuplicatedOutput* get_output_duplication();
	void clear_output_duplications();

	DuplicatedOutput* m_output_duplication;
	vector<DuplicatedOutput> m_out_dups;
	UINT16 m_capture_source;
	RECT m_output_rect;
	BYTE* m_frame_buf;
	size_t m_frame_buf_size;
	uint32_t m_timeout;
};