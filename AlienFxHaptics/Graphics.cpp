#define _WIN32_WINNT 0x500
#include "Graphics.h"
#include <windows.h>
#include <Commctrl.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "resource.h"
#include "resource_config.h"
//#include "AudioIn.h"
#include <string>

#pragma comment(lib, "winmm.lib")


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int bars;
int* freq;
int nCmdShow;
double y_scale;
HFONT g_hfFont;
char*** rms;
int rmsI;
double power;
double short_term_power;
double long_term_power;
int avg_freq;
int short_term_avg_freq;
int long_term_avg_freq;
bool axis_draw = true;

//LFXUtil::LFXUtilC* lfxUtil = NULL;
ConfigHandler* config = NULL;
WSAudioIn* audio = NULL;

HINSTANCE ghInstance;

NOTIFYICONDATA niData;

// default constructor
Graphics::Graphics(HINSTANCE hInstance, int mainCmdShow, int* freqp, ConfigHandler *conf)
{

	nCmdShow=mainCmdShow;
	bars = conf->numbars;
	y_scale = conf->res;
	rmsI=1;
	g_hfFont = NULL;
	g_bOpaque = TRUE;
	g_rgbText = RGB(255, 255, 255);
	g_rgbBackground = RGB(0, 0, 0);
	freq=freqp;
	//lfxUtil = lfxutil;
	config = conf;

	strcpy_s(g_szClassName,14,"myWindowClass");
	for (int i=0; i<16; i++)
		 g_rgbCustom[i]=0;

	wc.cbSize		 = sizeof(WNDCLASSEX);
	wc.style		 = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc	 = WndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hInstance;
	ghInstance = hInstance;
	//wc.hIcon		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	//wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH) ;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MYMENU);
	wc.lpszClassName = g_szClassName;
	//wc.hIconSm		 = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ALIEN));
	wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ALIEN), IMAGE_ICON, 16, 16, 0);
	

	if(!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
	}

	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"Universal AlienFX haptics",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 700,
		NULL, NULL, hInstance, NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
	}

	if (config->inpType)
		CheckMenuItem(GetMenu(hwnd), ID_INPUT_DEFAULTINPUTDEVICE, MF_CHECKED);
	else
		CheckMenuItem(GetMenu(hwnd), ID_INPUT_DEFAULTOUTPUTDEVICE, MF_CHECKED);

	//configHwnd = CreateDialog(GetModuleHandle(NULL),         /// instance handle
	//	MAKEINTRESOURCE(IDD_DIALOG_CONFIG),    /// dialog box template
	//	hwnd,                    /// handle to parent
	//	(DLGPROC)DialogConfigStatic);

}


void Graphics::start(){
	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
}

int Graphics:: getBarsNum(){
	return bars;
}

double Graphics:: getYScale(){
	return y_scale;
}

void Graphics::setYScale(double newS) {
	y_scale = newS;
}
void Graphics::refresh(){
	RedrawWindow(hwnd, 0, 0, RDW_INTERNALPAINT );
}
void Graphics::setCurrentPower(double po){
	power=po;
}
void Graphics::setShortPower(double po){
	short_term_power=po;
}
void Graphics::setLongPower(double po){
	long_term_power=po;
}

void Graphics::setCurrentAvgFreq(int af){
	avg_freq=af;
}
void Graphics::setShortAvgFreq(int af){
	short_term_avg_freq=af;
}
void Graphics::setLongAvgFreq(int af){
	long_term_avg_freq=af;
}

void Graphics::ShowError(char* T)
{
	MessageBox(hwnd, T, "Error!", MB_OK);
}

void Graphics::SetAudioObject(WSAudioIn* wsa)
{
	audio = wsa;
}

void DrawFreq(HDC hdc, LPRECT rcClientP)
{
	int i, rectop;
	char szSize[100]; //freq axis

	HWND hwnd = WindowFromDC(hdc);
	if (!IsIconic(hwnd)) {

		//setting collors:
		SetDCBrushColor(hdc, RGB(255, 255, 255));
		SetDCPenColor(hdc, RGB(255, 255, 39));
		SetTextColor(hdc, RGB(255, 255, 255));
		SelectObject(hdc, GetStockObject(DC_BRUSH));
		SelectObject(hdc, GetStockObject(DC_PEN));
		SetBkMode(hdc, TRANSPARENT);

		if (axis_draw) {
			//draw x axis:
			MoveToEx(hdc, 40, rcClientP->bottom - 21, (LPPOINT)NULL);
			LineTo(hdc, rcClientP->right - 50, rcClientP->bottom - 21);
			LineTo(hdc, rcClientP->right - 55, rcClientP->bottom - 26);
			MoveToEx(hdc, rcClientP->right - 50, rcClientP->bottom - 21, (LPPOINT)NULL);
			LineTo(hdc, rcClientP->right - 55, rcClientP->bottom - 16);
			TextOut(hdc, rcClientP->right - 45, rcClientP->bottom - 27, "f(kHz)", 6);

			//draw y axis:
			MoveToEx(hdc, 40, rcClientP->bottom - 21, (LPPOINT)NULL);
			LineTo(hdc, 40, 30);
			LineTo(hdc, 45, 35);
			MoveToEx(hdc, 40, 30, (LPPOINT)NULL);
			LineTo(hdc, 35, 35);
			TextOut(hdc, 15, 10, "[Power]", 7);
			//wsprintf(szSize, "%6d", (int)y_scale);
			//TextOut(hdc, 150, 10, szSize, 6);
			TextOut(hdc, 10, 40, "255", 3);
			TextOut(hdc, 10, (rcClientP->bottom) / 2, "128", 3);
			TextOut(hdc, 10, rcClientP->bottom - 35, "  0", 3);
			//axis_draw = false;
			int oldvalue = (-1);
			double coeff = 22 / (log(22.0));
			for (i = 0; i <= 22; i++) {
				int frq = int(22 - round((log(22.0 - i) * coeff)));
				if (frq > oldvalue) {
					wsprintf(szSize, "%2d", frq);
					TextOut(hdc, ((rcClientP->right - 100) * i) / 23 + 50, rcClientP->bottom - 20, szSize, 2);
					oldvalue = frq;
				}
			}
		}

		for (i = 0; i < bars; i++) {
			rectop = ((255 - freq[i]) * (rcClientP->bottom - 30)) / 255;
			if (rectop < 50) rectop = 50;
			Rectangle(hdc, ((rcClientP->right - 120) * i) / bars + 50, rectop, ((rcClientP->right - 120) * (i + 1)) / bars - 2 + 50, rcClientP->bottom - 30);
			//wsprintf(szSize, "%3d", freq[i]);
			//TextOut(hdc, ((rcClientP->right - 120) * i) / bars + 50, rectop - 15, szSize, 3);
		}
	}
} 



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//char szSize[500];
	INT_PTR dlg;

	switch(msg)
	{
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
				case ID_INPUT_DEFAULTOUTPUTDEVICE:
					config->inpType = 0;
					CheckMenuItem(GetMenu(hwnd), ID_INPUT_DEFAULTINPUTDEVICE, MF_UNCHECKED);
					CheckMenuItem(GetMenu(hwnd), ID_INPUT_DEFAULTOUTPUTDEVICE, MF_CHECKED);
					audio->RestartDevice(0);
					config->Save();
					break;
				case ID_INPUT_DEFAULTINPUTDEVICE:
					config->inpType = 1;
					CheckMenuItem(GetMenu(hwnd), ID_INPUT_DEFAULTINPUTDEVICE, MF_CHECKED);
					CheckMenuItem(GetMenu(hwnd), ID_INPUT_DEFAULTOUTPUTDEVICE, MF_UNCHECKED);
					audio->RestartDevice(1);
					config->Save();
					break;
				/*case ID_PARAMS_1:
					wsprintf(szSize, "          \n          Instantaneous Power: %d µW          \n\n          Weighted Average Power: %d µW          \n\n          Average Power: %d µW          \n\n          ", ((int)(power*1000000)), ((int)(short_term_power*1000000)), ((int)(long_term_power*1000000)) );
					MessageBox(hwnd, szSize, "Parameters", MB_OK);
				break;
				case ID_PARAMS_2:
					wsprintf(szSize, "          \n\n          Instantaneous center frequency: %d Hz          \n\n          Weighted Average of the center frequency: %d Hz          \n\n          Average center frequency: %d Hz          \n\n          ", avg_freq, short_term_avg_freq, long_term_avg_freq );
					MessageBox(hwnd, szSize, "Parameters", MB_OK);
				break;*/
				case ID_PARAMETERS_SETTINGS:
					dlg=DialogBox/*CreateDialog*/(GetModuleHandle(NULL),         /// instance handle
						MAKEINTRESOURCE(IDD_DIALOG_CONFIG),    /// dialog box template
						hwnd,                    /// handle to parent
						(DLGPROC)DialogConfigStatic);
					//ShowWindow(dlg, SW_SHOW);
				break;
			}
		break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
		break;
		case WM_PAINT:
		{
			RECT rcClient;

			HDC hdc = GetDC(hwnd);
			GetClientRect(hwnd, &rcClient);

			HBRUSH hb = CreateSolidBrush(RGB(0,0,0)); 
			//HBRUSH hb=CreatePatternBrush((HBITMAP) LoadImage(0,("background.bmp"),IMAGE_BITMAP,rcClient.right,rcClient.bottom,LR_CREATEDIBSECTION|LR_LOADFROMFILE));
			FillRect(hdc, &rcClient, hb);
			DeleteObject(hb);

			DrawFreq(hdc, &rcClient);

			//EndPaint(hwnd, &ps);
			ReleaseDC(hwnd, hdc);

			RedrawWindow(hwnd, 0, 0, RDW_VALIDATE );
		}
		break;
		case WM_DESTROY:
			DeleteObject(g_hfFont);

			PostQuitMessage(0);
		break;
		case WM_SIZE:
			if (wParam == SIZE_MINIMIZED) {
				// go to tray...

				ZeroMemory(&niData, sizeof(NOTIFYICONDATA));
				niData.cbSize = sizeof(NOTIFYICONDATA);
				niData.uID = IDI_ALIEN;
				niData.uFlags = NIF_ICON | NIF_MESSAGE;
				niData.hIcon =
					(HICON)LoadImage(GetModuleHandle(NULL),
						MAKEINTRESOURCE(IDI_ALIEN),
						IMAGE_ICON,
						GetSystemMetrics(SM_CXSMICON),
						GetSystemMetrics(SM_CYSMICON),
						LR_DEFAULTCOLOR);
				niData.hWnd = hwnd;
				niData.uCallbackMessage = WM_APP + 1;
				Shell_NotifyIcon(NIM_ADD, &niData);
				ShowWindow(hwnd, SW_HIDE);
			} break;
		case WM_APP + 1: {
			switch (lParam)
			{
			case WM_LBUTTONDBLCLK:
			case WM_LBUTTONUP:
				ShowWindow(hwnd, SW_RESTORE);
				SetWindowPos(hwnd,       // handle to window
					HWND_TOPMOST,  // placement-order handle
					0,     // horizontal position
					0,      // vertical position
					0,  // width
					0, // height
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE// window-positioning options
				);
				Shell_NotifyIcon(NIM_DELETE, &niData);
			break;
			//case WM_RBUTTONDOWN:
			//case WM_CONTEXTMENU:
			//	ShowContextMenu(hWnd);
			}
			break;
		} break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}



BOOL CALLBACK DialogConfigStatic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned i;

	HWND freq_list = GetDlgItem(hDlg, IDC_FREQ);
	HWND dev_list = GetDlgItem(hDlg, IDC_DEVICE);
	HWND light_list = GetDlgItem(hDlg, IDC_LIGHTS);
	HWND from_color = GetDlgItem(hDlg, IDC_FROMCOLOR);
	HWND to_color = GetDlgItem(hDlg, IDC_TOCOLOR);
	HWND low_cut = GetDlgItem(hDlg, IDC_EDIT_LOWCUT);
	HWND hi_cut = GetDlgItem(hDlg, IDC_EDIT_HIGHCUT);
	HWND hdecay = GetDlgItem(hDlg, IDC_EDIT_DECAY);
	mapping* map = NULL;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		double coeff = 22030 / log(21);
		char frqname[55]; int prevfreq = 20;
		for (i = 1; i < 21; i++) {
			int frq = 22050 - (int) round((log(21-i) * coeff));
			sprintf_s(frqname, 55, "%d-%dHz", prevfreq, frq);
			prevfreq = frq;
			SendMessage(freq_list, LB_ADDSTRING, 0, (LPARAM)frqname);
		}
		unsigned numdev = config->lfxUtil->GetNumDev();
		for (i = 0; i < numdev; i++) {
			deviceinfo* dev = config->lfxUtil->GetDevInfo(i);
			int pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(TEXT(dev->desc)));
			SendMessage(dev_list, LB_SETITEMDATA, pos, (LPARAM)dev->id);
		}
		SendMessage(dev_list, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
		if (numdev > 0) {
			unsigned lights = config->lfxUtil->GetDevInfo(0)->lights;
			for (i = 0; i < lights; i++) {
				lightinfo *lgh = config->lfxUtil->GetLightInfo(0, i);
				int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(TEXT(lgh->desc)));
				SendMessage(light_list, LB_SETITEMDATA, pos, (LPARAM)lgh->id);
			}
		}
		TCHAR decay[17];
		sprintf_s(decay, 16, "%d", config->res);
		SendMessage(hdecay, WM_SETTEXT, 0, (LPARAM)decay);
	}
	break;

	case WM_COMMAND:
	{
		int lbItem = (int)SendMessage(light_list, LB_GETCURSEL, 0, 0);
		int lid = (int)SendMessage(light_list, LB_GETITEMDATA, lbItem, 0);
		lbItem = (int)SendMessage(dev_list, CB_GETCURSEL, 0, 0);
		int did = (int)SendMessage(dev_list, CB_GETITEMDATA, lbItem, 0);
		int fid = (int)SendMessage(freq_list, LB_GETCURSEL, 0, 0);
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			// save mappings!
			config->Save();
			/// handle OK button click
			EndDialog(hDlg, IDOK);
		}
		break;

		/*case IDCANCEL:
		{
			/// handle Cancel button click
			EndDialog(hDlg, IDCANCEL);
		}*/
		break;
		case IDC_RESET: {
			switch (HIWORD(wParam))
			{
			case BN_CLICKED: {
				for (i = 0; i < config->mappings.size(); i++) {
					config->mappings[i].colorfrom.ci = 0;
					config->mappings[i].colorto.ci = 0;
					config->mappings[i].lowcut = 0;
					config->mappings[i].hicut = 255;
					config->mappings[i].map.clear();
				}
				SendMessage(freq_list, LB_SETSEL, FALSE, -1);
				RedrawWindow(freq_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				SendMessage(from_color, IPM_SETADDRESS, 0, 0);
				RedrawWindow(from_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				SendMessage(to_color, IPM_SETADDRESS, 0, 0);
				RedrawWindow(to_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				TCHAR locut[] = "0", hicut[] = "255";
				SendMessage(low_cut, WM_SETTEXT, 0, (LPARAM)locut);
				RedrawWindow(low_cut, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				SendMessage(hi_cut, WM_SETTEXT, 0, (LPARAM)hicut);
				RedrawWindow(hi_cut, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
			} break;
			}
		} break;
		case IDC_DEVICE: {
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE: {
				unsigned numdev = config->lfxUtil->GetNumDev();
				if (numdev > 0) {
					unsigned lights = config->lfxUtil->GetDevInfo(did)->lights;
					EnableWindow(freq_list, FALSE);
					SendMessage(freq_list, LB_SETSEL, FALSE, -1);
					SendMessage(light_list, LB_RESETCONTENT, 0, 0);
					for (i = 0; i < lights; i++) {
						lightinfo* lgh = config->lfxUtil->GetLightInfo(did, i);
						int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(TEXT(lgh->desc)));
						SendMessage(light_list, LB_SETITEMDATA, pos, (LPARAM)lgh->id);
					}
					RedrawWindow(freq_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
					RedrawWindow(light_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
					// clear colors...
					SendMessage(from_color, IPM_SETADDRESS, 0, 0);
					RedrawWindow(from_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
					SendMessage(to_color, IPM_SETADDRESS, 0, 0);
					RedrawWindow(to_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
					//  clear cuts....
					TCHAR locut[] = "0", hicut[] = "255";
					SendMessage(low_cut, WM_SETTEXT, 0, (LPARAM)locut);
					RedrawWindow(low_cut, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
					SendMessage(hi_cut, WM_SETTEXT, 0, (LPARAM)hicut);
					RedrawWindow(hi_cut, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				}
			} break;
			}
		} break;// should reload dev list
		case IDC_LIGHTS: // should reload mappings
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE: {
				// check in config - do we have mappings?
				for (i = 0; i < config->mappings.size(); i++)
					if (config->mappings[i].devid == did && config->mappings[i].lightid == lid)
						break;
				if (i < config->mappings.size()) {
					map = &config->mappings[i];
				}
				else {
					mapping newmap;
					newmap.devid = did;
					newmap.lightid = lid;
					newmap.colorfrom.ci = 0;
					newmap.colorto.ci = 0;
					newmap.lowcut = 0;
					newmap.hicut = 255;
					config->mappings.push_back(newmap);
					map = &config->mappings[i];
				}
				// load freq....
				EnableWindow(freq_list, TRUE);
				SendMessage(freq_list, LB_SETSEL, FALSE, -1);
				for (int j = 0; j < map->map.size(); j++) {
					SendMessage(freq_list, LB_SETSEL, TRUE, map->map[j]);
				}
				RedrawWindow(freq_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				// load colors...
				unsigned clrmap = MAKEIPADDRESS(map->colorfrom.cs.red, map->colorfrom.cs.green, map->colorfrom.cs.blue, map->colorfrom.cs.brightness);
				SendMessage(from_color, IPM_SETADDRESS, 0, clrmap);
				RedrawWindow(from_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				clrmap = MAKEIPADDRESS(map->colorto.cs.red, map->colorto.cs.green, map->colorto.cs.blue, map->colorto.cs.brightness);
				SendMessage(to_color, IPM_SETADDRESS, 0, clrmap);
				RedrawWindow(to_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				// load cuts...
				TCHAR locut[6], hicut[6];
				sprintf_s(locut, 5, "%d", map->lowcut);
				sprintf_s(hicut, 5, "%d", map->hicut);
				SendMessage(low_cut, WM_SETTEXT, 0, (LPARAM)locut);
				RedrawWindow(low_cut, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				SendMessage(hi_cut, WM_SETTEXT, 0, (LPARAM)hicut);
				RedrawWindow(hi_cut, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
			}
			break;
		}
		break;
		case IDC_FREQ: { // should update mappings list
			switch (HIWORD(wParam))
			{
			case LBN_SELCHANGE: {
				for (i = 0; i < config->mappings.size(); i++)
					if (config->mappings[i].devid == did && config->mappings[i].lightid == lid)
						break;
				if (i < config->mappings.size()) {
					map = &config->mappings[i];
					// add mapping
					std::vector <unsigned char>::iterator Iter = map->map.begin();
					for (i = 0; i < map->map.size(); i++)
						if (map->map[i] == fid)
							break;
						else
							Iter++;
					if (i == map->map.size())
						// new mapping, add and select
						map->map.push_back(fid);
					else
						map->map.erase(Iter);
					// remove selection
				}
				else {
					SendMessage(freq_list, LB_SETSEL, FALSE, -1);
				}
			} break;
			}
		} break;
		case IDC_EDIT_DECAY:
			switch (HIWORD(wParam)) {
			case EN_UPDATE: {
				// update Decay rate
				TCHAR buffer[17]; buffer[0] = 16;
				SendMessage(hdecay, EM_GETLINE, 0, (LPARAM)buffer);
				y_scale = config->res = atoi(buffer);
			} break;
			}
			break;
		case IDC_EDIT_LOWCUT:
			switch (HIWORD(wParam)) {
			case EN_UPDATE: {
				// update lo-cut
				for (i = 0; i < config->mappings.size(); i++)
					if (config->mappings[i].devid == did && config->mappings[i].lightid == lid)
						break;
				if (i < config->mappings.size()) {
					map = &config->mappings[i];
					TCHAR buffer[4]; buffer[0] = 3;
					SendMessage(low_cut, EM_GETLINE, 0, (LPARAM)buffer);
					map->lowcut = atoi(buffer) < 256 ? atoi(buffer) : 255;
				}
			} break;
			} break;
		case IDC_EDIT_HIGHCUT:
			switch (HIWORD(wParam)) {
			case EN_UPDATE: {
				// update lo-cut
				for (i = 0; i < config->mappings.size(); i++)
					if (config->mappings[i].devid == did && config->mappings[i].lightid == lid)
						break;
				if (i < config->mappings.size()) {
					map = &config->mappings[i];
					TCHAR buffer[4]; buffer[0] = 3;
					SendMessage(hi_cut, EM_GETLINE, 0, (LPARAM)buffer);
					map->hicut = atoi(buffer) < 256 ? atoi(buffer) : 255;
				}
			} break;
			} break;
		case IDC_FROMCOLOR: { // should update light data
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:/*EN_CHANGE:*/ {
				for (i = 0; i < config->mappings.size(); i++)
					if (config->mappings[i].devid == did && config->mappings[i].lightid == lid)
						break;
				if (i < config->mappings.size()) {
					map = &config->mappings[i];
					unsigned clrmap = 0;
					SendMessage(from_color, IPM_GETADDRESS, 0, (LPARAM) &clrmap);
					map->colorfrom.cs.red = FIRST_IPADDRESS(clrmap);
					map->colorfrom.cs.green = SECOND_IPADDRESS(clrmap);
					map->colorfrom.cs.blue = THIRD_IPADDRESS(clrmap);
					map->colorfrom.cs.brightness = FOURTH_IPADDRESS(clrmap);
				}
			} break;
			}
		} break;
		case IDC_TOCOLOR: { // the same
			switch (HIWORD(wParam))
			{
			case EN_KILLFOCUS:/*EN_CHANGE:*/ {
				for (i = 0; i < config->mappings.size(); i++)
					if (config->mappings[i].devid == did && config->mappings[i].lightid == lid)
						break;
				if (i < config->mappings.size()) {
					map = &config->mappings[i];
					unsigned clrmap = 0;
					SendMessage(to_color, IPM_GETADDRESS, 0, (LPARAM) &clrmap);
					map->colorto.cs.red = FIRST_IPADDRESS(clrmap);
					map->colorto.cs.green = SECOND_IPADDRESS(clrmap);
					map->colorto.cs.blue = THIRD_IPADDRESS(clrmap);
					map->colorto.cs.brightness = FOURTH_IPADDRESS(clrmap);
				}
			} break;
			}
		} break;
		}
	}
	break;
	default: return false;
	}

	return TRUE;
}