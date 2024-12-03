#include "framework.h"
#include "resource.h"
#include "guitar.h"
#include <Xinput.h>
#include <mmsystem.h>

HINSTANCE g_hInst;

// progress bars
HWND hWhammy, hTilt, hSlider;

// text controls
HWND hDisconnected, hType, hConnection;
HWND hTimerRes, hGraphTick;

#define C_GGreen	(COLORREF)(0x00CC00)
#define C_GRed		(COLORREF)(0x0000FF)
#define C_GYellow	(COLORREF)(0x00CCCC)
#define C_GBlue		(COLORREF)(0xFF0000)
#define C_GOrange	(COLORREF)(0x0080FF)
#define C_GPurple	(COLORREF)(0xFF00FF)
#define C_GDPurple	(COLORREF)(0x800080)

#define C_GX 16
#define M_GraphX(x)	(int)((x) * C_GX)
#define M_SetBrushPen(hdc, color) (SetDCBrushColor((hdc), (color)), SetDCPenColor((hdc), (color)))

#define C_NumHistoric 512
unsigned int g_ulPxPerHistoric = 1;

#define M_GetHistoric(x, b) (unsigned char)(((x) >> (b)) & 1)
#define M_SetHistoric(x, b) ((x) |= (1 << (b)))

HWND hGraph;
HWND hFreeze;

char const *szGraphLabels = "GRYBOUDLRSP";
COLORREF a_xGraphColors[GRAPH_LAST+1] = { C_GGreen, C_GRed, C_GYellow, C_GBlue, C_GOrange, C_GPurple, C_GDPurple };
unsigned int a_ulHistoricStates[C_NumHistoric] = { 0 };
unsigned int a_ulHistoricTimers[BUTTON_COUNT] = { 0 };
unsigned int a_ulHistoricCounters[BUTTON_COUNT] = { 0 };

unsigned int g_ulTimerId = 0;
unsigned int g_ulTimerPeriod = 0;

BOOL g_bGraphFreeze = TRUE;

XINPUT_STATE state = { 0 };
int playerId = 0;
DWORD dwPrevResult;

unsigned int g_ulTmpHistoric = 0;
WORD g_uwTmpWhammy = 0;
WORD g_uwTmpTilt = 0;
WORD g_uwTmpSlider = 0;


void UpdateHistoric( unsigned int ulNewHistoric ) 
{
	static unsigned int s_HistoricCounter = 0;

	if ( ulNewHistoric )
		s_HistoricCounter = 0;
	else
		++s_HistoricCounter;

	if ( g_bGraphFreeze && s_HistoricCounter > 32 )
		return;

	memmove(a_ulHistoricStates+1, a_ulHistoricStates, sizeof(a_ulHistoricStates[0]) * (C_NumHistoric - 1));
	a_ulHistoricStates[0] = ulNewHistoric;
}

HDC hBackBuf = NULL;
HBITMAP hBackBmp = NULL;

void DrawGraph( HDC hdc, RECT *rc )
{
	if ( !hBackBuf )
	{
		hBackBuf = CreateCompatibleDC(hdc);
		hBackBmp = CreateCompatibleBitmap(hdc, rc->right, rc->bottom);
	}

	int backBufState = SaveDC(hBackBuf);
	SelectObject(hBackBuf, hBackBmp);

	SetBkColor(hBackBuf, GetSysColor(COLOR_3DFACE));
	SelectObject(hBackBuf, GetStockObject(DEFAULT_GUI_FONT));
	SelectObject(hBackBuf, GetStockObject(DC_BRUSH));
	SelectObject(hBackBuf, GetStockObject(DC_PEN));

	// backbuf background
	HBRUSH hBkBrush = GetSysColorBrush(COLOR_3DFACE);
	FillRect(hBackBuf, rc, hBkBrush);

	int yTotal = C_NumHistoric * g_ulPxPerHistoric;
	int dy = 8;

	RECT rect = { 0, 0, C_GX, C_GX };

	// headers
	for ( int i = 0; i <= GRAPH_LAST; ++i )
	{
		rect.left = M_GraphX(i+1);
		rect.right = M_GraphX(i+2);
		DrawText(hBackBuf, szGraphLabels + i, 1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	}

	rect = (RECT){ C_GX-1, C_GX-1, C_GX+M_GraphX(GRAPH_LAST+1), C_GX+yTotal };

	// borders
	MoveToEx(hBackBuf, rect.left-5, rect.top, NULL);
	LineTo(hBackBuf, rect.right, rect.top);
	MoveToEx(hBackBuf, rect.left, rect.top, NULL);
	LineTo(hBackBuf, rect.left, rect.bottom);
	MoveToEx(hBackBuf, rect.right, rect.top, NULL);
	LineTo(hBackBuf, rect.right, rect.bottom);

	// inputs bg
	M_SetBrushPen(hBackBuf, 0xFFFFFF);
	Rectangle(hBackBuf, rect.left+1, rect.top+1, rect.right, rect.bottom);

	// markers
	for ( int i = dy; i <= yTotal; i += dy )
	{
		M_SetBrushPen(hBackBuf, 0);
		MoveToEx(hBackBuf, rect.left-5, rect.top+i, NULL);
		LineTo(hBackBuf, rect.left+1, rect.top+i);
		M_SetBrushPen(hBackBuf, 0xCCCCCC);
		LineTo(hBackBuf, rect.right, rect.top+i);
	}

	for ( int i = 0; i <= GRAPH_LAST; ++i )
	{
		M_SetBrushPen(hBackBuf, a_xGraphColors[i]);
		int lX = M_GraphX(i+1);

		for ( int y = 0; y < C_NumHistoric; ++y )
		{
			int lY1 = y;
			int lY2 = 0;

			for ( ; y < C_NumHistoric && M_GetHistoric(a_ulHistoricStates[y], i); ++y )
				++lY2;

			if ( lY2 )
				Rectangle(hBackBuf, lX, C_GX + lY1 * g_ulPxPerHistoric, C_GX + lX, C_GX + (lY1 + lY2) * g_ulPxPerHistoric);
		}
	}

	BitBlt(hdc, rc->left, rc->top, rc->right-rc->left, rc->bottom-rc->top, hBackBuf, rc->left, rc->top, SRCCOPY);
	RestoreDC(hBackBuf, backBufState);
}

void UpdateDisplay( void )
{
	for ( int i = 0; i < BUTTON_COUNT; ++i )
	{
		BUTTON_INFO *button = &buttons[i];
		DWORD checkValue = M_GetHistoric(g_ulTmpHistoric, i) ? BST_CHECKED : BST_UNCHECKED;
		SendMessage(button->hControl, BM_SETCHECK, checkValue, 0);

		if ( button->hLabelCounter && button->hLabelTimer )
		{
			char szTmp[32];
			float buttonMs = a_ulHistoricTimers[i] * g_ulTimerPeriod * 0.001f;

			sprintf_s(szTmp, sizeof(szTmp), "%u", a_ulHistoricCounters[i]);
			SetWindowText(button->hLabelCounter, szTmp);
			sprintf_s(szTmp, sizeof(szTmp), "%.3f", buttonMs);
			SetWindowText(button->hLabelTimer, szTmp);
		}
	}

	SendMessage(hWhammy, PBM_SETPOS, g_uwTmpWhammy, 0);
	SendMessage(hTilt, PBM_SETPOS, g_uwTmpTilt, 0);
	SendMessage(hSlider, PBM_SETPOS, g_uwTmpSlider, 0);

	RECT s_rc = { C_GX, C_GX, M_GraphX(GRAPH_LAST+2), C_GX + (C_NumHistoric * g_ulPxPerHistoric) };
	InvalidateRect(hGraph, &s_rc, FALSE);
}

void SetPlayer( int newPlayerId )
{
	static char const *a_szType[] = {
		[XINPUT_DEVSUBTYPE_GUITAR] = "Guitar",
		[XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE] = "Guitar (Alternate)",
		[XINPUT_DEVSUBTYPE_GUITAR_BASS] = "Bass Guitar",
		[XINPUT_DEVSUBTYPE_DRUM_KIT] = "Drum Kit",
		[XINPUT_DEVSUBTYPE_GAMEPAD] = "Gamepad",
	};

	XINPUT_CAPABILITIES caps = { 0 };
	char const *szType;
	char const *szConnection;

	playerId = newPlayerId;

	if ( XInputGetCapabilities(playerId, 0, &caps) == ERROR_SUCCESS )
	{
		szType = (caps.SubType < ARRAYSIZE(a_szType) && a_szType[caps.SubType])
			? a_szType[caps.SubType]
			: "Unknown/Other";

		szConnection = (caps.Flags & XINPUT_CAPS_WIRELESS) ? "Wireless" : "Wired";

		ShowWindow(hDisconnected, SW_HIDE);
	}
	else
	{
		szType = "None";
		szConnection = "Disconnected";

		// reset all values
		g_ulTmpHistoric = 0;
		g_uwTmpWhammy = 0;
		g_uwTmpTilt = 0;
		g_uwTmpSlider = 0;
		UpdateHistoric(0);
		memset(a_ulHistoricCounters, 0, sizeof(a_ulHistoricCounters));
		memset(a_ulHistoricTimers, 0, sizeof(a_ulHistoricTimers));

		SendMessage(hWhammy, PBM_SETPOS, 0, 0);
		SendMessage(hTilt, PBM_SETPOS, 0, 0);
		SendMessage(hSlider, PBM_SETPOS, 0, 0);
		for ( int i = 0; i < BUTTON_COUNT; i++ )
			SendMessage(buttons[i].hControl, BM_SETCHECK, BST_UNCHECKED, 0);

		ShowWindow(hDisconnected, SW_SHOW);
	}

	SetWindowText(hType, szType);
	SetWindowText(hConnection, szConnection);
}

void ReadXInput( void )
{
	DWORD dwResult = XInputGetState(playerId, &state);
	
	if ( dwResult == ERROR_SUCCESS )
	{
		unsigned int ulHistoric = 0;
		for ( int i = 0; i < BUTTON_COUNT; i++ )
		{
			BUTTON_INFO *button = &buttons[i];
			if ( state.Gamepad.wButtons & button->wXButton ) // is pressed
			{
				M_SetHistoric(ulHistoric, i);
				if ( !M_GetHistoric(g_ulTmpHistoric, i) ) // was not pressed before
				{
					a_ulHistoricTimers[i] = 0;
					a_ulHistoricCounters[i]++;
				}
				a_ulHistoricTimers[i]++;
			}
		}

		g_uwTmpWhammy = state.Gamepad.sThumbRX + 0x8000;
		g_uwTmpTilt = state.Gamepad.sThumbRY + 0x8000;
		g_uwTmpSlider = state.Gamepad.sThumbLX + 0x8000;

		g_ulTmpHistoric = ulHistoric;
		UpdateHistoric(ulHistoric);
	}

	if ( dwResult != dwPrevResult )
	{
		// Device changed, refresh xinput caps
		SetPlayer(playerId);
		
		dwPrevResult = dwResult;
	}
}

void CALLBACK TimeProc( UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2 )
{
	ReadXInput();
	//UpdateDisplay();
}

void StartTimer( void )
{
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(TIMECAPS));
	unsigned int ms = tc.wPeriodMin;
	//ms = 2;
	timeBeginPeriod(ms);
	g_ulTimerId = timeSetEvent(ms, ms, TimeProc, 0, TIME_PERIODIC);
	g_ulTimerPeriod = ms;

	if ( g_ulTimerPeriod <= 2 )
		g_ulPxPerHistoric = 1;
	else if ( g_ulTimerPeriod <= 4 )
		g_ulPxPerHistoric = 2;
	else
		g_ulPxPerHistoric = 4;

	char szMs[16];
	sprintf_s(szMs, sizeof(szMs), "%u ms", g_ulTimerPeriod);
	SetWindowText(hTimerRes, szMs);
	sprintf_s(szMs, sizeof(szMs), "%u ms", (unsigned int)(8 * g_ulTimerPeriod / g_ulPxPerHistoric));
	SetWindowText(hGraphTick, szMs);
}

void StopTimer( void )
{
	timeKillEvent(g_ulTimerId);
	timeEndPeriod(g_ulTimerPeriod);
}

BOOL CALLBACK MainDialogProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	)
{
	HICON hIcon;

	switch ( uMsg )
	{
	case WM_INITDIALOG:
		// Set app icon
		hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON1));
		SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		// Get control IDs for buttons/analogs
		for ( int i = 0; i < BUTTON_COUNT; i++ )
		{
			BUTTON_INFO *button = &buttons[i];
			button->hControl = GetDlgItem(hWnd, button->wDialogId);
			if ( button->wCtlIdCounter != 0 )
			{
				button->hLabelCounter = GetDlgItem(hWnd, button->wCtlIdCounter);
				button->hLabelTimer = GetDlgItem(hWnd, button->wCtlIdTimer);
			}
		}

		hWhammy = GetDlgItem(hWnd, IDC_WHAMMY);
		hTilt = GetDlgItem(hWnd, IDC_TILT);
		hSlider = GetDlgItem(hWnd, IDC_SLIDER);

		hDisconnected = GetDlgItem(hWnd, IDC_DISCONNECTED);
		hType = GetDlgItem(hWnd, IDC_TYPE);
		hConnection = GetDlgItem(hWnd, IDC_CONNECTION);

		hGraph = GetDlgItem(hWnd, IDC_GRAPH);
		hFreeze = GetDlgItem(hWnd, IDC_FREEZE);
		hTimerRes = GetDlgItem(hWnd, IDC_TIMERRES);
		hGraphTick = GetDlgItem(hWnd, IDC_GRAPHTICK);

		Button_SetCheck(hFreeze, g_bGraphFreeze);

		SendMessage(hWhammy, PBM_SETRANGE, 0, MAKELPARAM(0, 0xFFFF));
		SendMessage(hTilt, PBM_SETRANGE, 0, MAKELPARAM(0, 0xFFFF));
		SendMessage(hSlider, PBM_SETRANGE, 0, MAKELPARAM(0, 0xFFFF));

		// Set player 1 as default
		SendMessage(GetDlgItem(hWnd, IDC_PLAYER1), BM_SETCHECK, BST_CHECKED, 0);
		SetPlayer(0);

		SetTimer(hWnd, IDT_XINPUT, 8, NULL);
		StartTimer();
		return TRUE;

	case WM_TIMER:
		switch ( wParam )
		{
		case IDT_XINPUT:
			UpdateDisplay();
			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch ( LOWORD(wParam) )
		{
		case IDC_PLAYER1:
		case IDC_PLAYER2:
		case IDC_PLAYER3:
		case IDC_PLAYER4:
			SetPlayer(LOWORD(wParam) - IDC_PLAYER1);
			return TRUE;

		case IDC_FREEZE:
			g_bGraphFreeze = (Button_GetCheck(hFreeze) == BST_CHECKED);
			return TRUE;

		case IDOK:
		case IDCANCEL:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			return TRUE;
		}
		break;

	case WM_DRAWITEM:
		if ( wParam == IDC_GRAPH )
		{
			DRAWITEMSTRUCT *pItem = (DRAWITEMSTRUCT *)lParam;
			DrawGraph(pItem->hDC, &pItem->rcItem);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		KillTimer(hWnd, IDT_XINPUT);
		StopTimer();
		DeleteObject(hBackBmp);
		DeleteDC(hBackBuf);
		DestroyWindow(hWnd);
		return TRUE;

	case WM_DESTROY:
		PostQuitMessage(0);
		return TRUE;
	}

	return FALSE;
}

int WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	INT nShowCmd
	)
{
	HWND hDlg;
	MSG msg;
	g_hInst = hInstance;

	hDlg = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDialogProc);

	if ( hDlg == NULL )
		return 1;

	ShowWindow(hDlg, nShowCmd);

	while ( GetMessage(&msg, NULL, 0, 0) > 0 )
	{
		if ( !IsDialogMessage(hDlg, &msg) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}
