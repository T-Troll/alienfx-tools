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
#include "AudioIn.h"
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

LFXUtil::LFXUtilC* lfxUtil;
ConfigHandler* config;

HINSTANCE ghInstance;

// default constructor
Graphics::Graphics(HINSTANCE hInstance, int mainCmdShow, int* freqp, LFXUtil::LFXUtilC *lfxutil, ConfigHandler *conf)
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
	lfxUtil = lfxutil;
	config = conf;

	strcpy(g_szClassName,"myWindowClass");
	for (int i=0; i<16; i++)
		 g_rgbCustom[i]=0;
	rms = (char***)malloc(7*sizeof(char**));
	for(int i=0; i<7; i++){
		rms[i]=(char**)malloc(2*sizeof(char*));
		rms[i][0]=(char*)malloc(4*sizeof(char*));
		rms[i][1]=(char*)malloc(4*sizeof(char*));
	}
	strcpy(rms[0][0],"500");
	strcpy(rms[0][1],"250");

	strcpy(rms[1][0],"250");
	strcpy(rms[1][1],"125");

	strcpy(rms[2][0],"100");
	strcpy(rms[2][1]," 50");

	strcpy(rms[3][0]," 50");
	strcpy(rms[3][1]," 25");

	strcpy(rms[4][0]," 20");
	strcpy(rms[4][1]," 10");

	strcpy(rms[5][0]," 10");
	strcpy(rms[5][1],"  5");

	strcpy(rms[6][0],"  5");
	strcpy(rms[6][1],"2.5");



	
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
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
	wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
	

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
		CW_USEDEFAULT, CW_USEDEFAULT, 700, 400,
		NULL, NULL, hInstance, NULL);

	if(hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
	}

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

void DrawFreq(HDC hdc, LPRECT rcClientP) 
{ 
	int i,rectop;
	char szSize[100]; //freq axis

	//setting collors:
	SetDCBrushColor(hdc, RGB(255,255,255));
	SetDCPenColor(hdc, RGB(255,255,39));
	SetTextColor(hdc, RGB(255,255,255));
	SelectObject(hdc, GetStockObject(DC_BRUSH));  
	SelectObject(hdc, GetStockObject(DC_PEN));
	SetBkMode(hdc, TRANSPARENT);


	//draw x axis:
	MoveToEx(hdc, 40, rcClientP->bottom - 21, (LPPOINT) NULL); 
	LineTo(hdc, rcClientP->right - 50, rcClientP->bottom - 21);
	LineTo(hdc, rcClientP->right - 55, rcClientP->bottom - 26);
	MoveToEx(hdc, rcClientP->right - 50, rcClientP->bottom - 21, (LPPOINT) NULL); 
	LineTo(hdc, rcClientP->right - 55, rcClientP->bottom - 16);
	TextOut(hdc,rcClientP->right - 45,rcClientP->bottom - 27, "f(kHz)", 6);

	//draw y axis:
	MoveToEx(hdc, 40, rcClientP->bottom - 21, (LPPOINT) NULL);
	LineTo(hdc, 40, 30);
	LineTo(hdc, 45, 35);
	MoveToEx(hdc, 40, 30, (LPPOINT) NULL);
	LineTo(hdc, 35, 35);
	TextOut(hdc,15,10, "[V]", 3);
	//wsprintf(szSize, "%6d", (int)y_scale);
	//TextOut(hdc, 150, 10, szSize, 6);
	TextOut(hdc,10,40, rms[rmsI][0], 3);
	TextOut(hdc,10,(rcClientP->bottom)/2, rms[rmsI][1], 3);
	TextOut(hdc,10,rcClientP->bottom - 35, "  0", 3);


	for (i=0; i<bars; i++){
		rectop = ((255-freq[i])*(rcClientP->bottom-30))/255;
		if (rectop < 50 ) rectop = 50;
		Rectangle(hdc, ((rcClientP->right - 120)*i)/bars + 50, rectop, ((rcClientP->right - 120)*(i+1))/bars-2 + 50, rcClientP->bottom-30);
		//wsprintf(szSize, "%3d", freq[i]);
		//TextOut(hdc, ((rcClientP->right - 120) * i) / bars + 50, rectop - 15, szSize, 3);
	}
	int oldvalue = (-1);
	double coeff = 22 / (log(22.0));
	for (i = 0; i <= 22; i++) {
		int frq = 22 - round((log(22.0 - i) * coeff));
		if (frq > oldvalue) {
			wsprintf(szSize, "%2d", frq);
			TextOut(hdc, ((rcClientP->right - 100) * i) / 23 + 50, rcClientP->bottom - 20, szSize, 2);
			oldvalue = frq;
		}
	}
} 



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char szSize[500];
	INT_PTR dlg;
	switch(msg)
	{
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case ID_FILE_EXIT:
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				break;
				/*case ID_X_41:
					bars = 6;
				break;
				case ID_X_101:
					bars = 11;
				break;
				case ID_X_201:
					bars = 21;
				break;*/

				case ID_Y_1000:
					//y_scale = 1000000000;
					y_scale = 500;
					rmsI=0;
				break;
				case ID_Y_100:
					//y_scale = 100000000;
					y_scale = 250;
					rmsI=1;
				break;
				case ID_Y_10:
					//y_scale = 10000000;
					y_scale = 100;
					rmsI=2;
				break;
				case ID_Y_5:
					//y_scale = 5000000;
					y_scale = 50;
					rmsI=3;
				break;
				case ID_Y_2_5:
					//y_scale = 2500000;
					y_scale = 20;
					rmsI=4;
				break;
				case ID_Y_1:
					//y_scale = 1000000;
					y_scale = 10;
					rmsI=5;
				break;
				case ID_Y_0_5:
					//y_scale = 500000;
					y_scale = 5;
					rmsI=6;
				break;

				case ID_PARAMS_1:
					wsprintf(szSize, "          \n          Instantaneous Power: %d µW          \n\n          Weighted Average Power: %d µW          \n\n          Average Power: %d µW          \n\n          ", ((int)(power*1000000)), ((int)(short_term_power*1000000)), ((int)(long_term_power*1000000)) );
					MessageBox(hwnd, szSize, "Parameters", MB_OK);
				break;
				case ID_PARAMS_2:
					wsprintf(szSize, "          \n\n          Instantaneous center frequency: %d Hz          \n\n          Weighted Average of the center frequency: %d Hz          \n\n          Average center frequency: %d Hz          \n\n          ", avg_freq, short_term_avg_freq, long_term_avg_freq );
					MessageBox(hwnd, szSize, "Parameters", MB_OK);
				break;
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
	mapping* map = NULL;

	switch (message)
	{
	case WM_INITDIALOG:
	{
		double coeff = 22050 / log(20);
		char frqname[55]; int prevfreq = 0;
		for (i = 1; i < 20; i++) {
			int frq = 22050 - round((log(20-i) * coeff));
			sprintf_s(frqname, 55, "%d-%dHz", prevfreq, frq);
			prevfreq = frq;
			SendMessage(freq_list, LB_ADDSTRING, 0, (LPARAM)frqname);
		}
		unsigned numdev = lfxUtil->GetNumDev();
		for (i = 0; i < numdev; i++) {
			deviceinfo* dev = lfxUtil->GetDevInfo(i);
			int pos = (int)SendMessage(dev_list, CB_ADDSTRING, 0, (LPARAM)(TEXT(dev->desc)));
			SendMessage(dev_list, LB_SETITEMDATA, pos, (LPARAM)dev->id);
		}
		SendMessage(dev_list, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
		if (numdev > 0) {
			unsigned lights = lfxUtil->GetDevInfo(0)->lights;
			for (i = 0; i < lights; i++) {
				lightinfo *lgh = lfxUtil->GetLightInfo(0, i);
				int pos = (int)SendMessage(light_list, LB_ADDSTRING, 0, (LPARAM)(TEXT(lgh->desc)));
				SendMessage(light_list, LB_SETITEMDATA, pos, (LPARAM)lgh->id);
			}
		}
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

		case IDCANCEL:
		{
			/// handle Cancel button click
			EndDialog(hDlg, IDCANCEL);
		}
		break;
		case IDC_RESET: {
			switch (HIWORD(wParam))
			{
			case BN_CLICKED: {
				for (i = 0; i < config->mappings.size(); i++) {
					config->mappings[i].colorfrom.ci = 0;
					config->mappings[i].colorto.ci = 0;
					config->mappings[i].map.clear();
				}
				SendMessage(freq_list, LB_SETSEL, FALSE, -1);
				RedrawWindow(freq_list, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				SendMessage(from_color, IPM_SETADDRESS, 0, 0);
				RedrawWindow(from_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
				SendMessage(to_color, IPM_SETADDRESS, 0, 0);
				RedrawWindow(to_color, 0, 0, RDW_INVALIDATE | RDW_UPDATENOW);
			} break;
			}
		} break;
		case IDC_DEVICE: {
			switch (HIWORD(wParam))
			{
			case CBN_SELCHANGE: {
				unsigned numdev = lfxUtil->GetNumDev();
				if (numdev > 0) {
					unsigned lights = lfxUtil->GetDevInfo(did)->lights;
					EnableWindow(freq_list, FALSE);
					SendMessage(freq_list, LB_SETSEL, FALSE, -1);
					SendMessage(light_list, LB_RESETCONTENT, 0, 0);
					for (i = 0; i < lights; i++) {
						lightinfo* lgh = lfxUtil->GetLightInfo(did, i);
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
	}

	return TRUE;
}