#include "framework.h"
#include "resource.h"
#include "guitar.h"
#include <Xinput.h>

HINSTANCE g_hInst;

// progress bars
HWND hWhammy, hTilt, hSlider;

// text controls
HWND hDisconnected, hType, hConnection;

XINPUT_STATE state = { 0 };
int playerId = 0;
DWORD dwPrevResult;


void SetPlayer( int newPlayerId )
{
	XINPUT_CAPABILITIES caps = { 0 };
	char *szType;
	char *szConnection;

	playerId = newPlayerId;

	if ( XInputGetCapabilities(playerId, 0, &caps) == ERROR_SUCCESS )
	{
		switch ( caps.SubType )
		{
		case XINPUT_DEVSUBTYPE_GUITAR:
			szType = "Guitar";
			break;

		case XINPUT_DEVSUBTYPE_GUITAR_ALTERNATE:
			szType = "Guitar (Alternate)";
			break;

		case XINPUT_DEVSUBTYPE_GUITAR_BASS:
			szType = "Bass Guitar";
			break;

		case XINPUT_DEVSUBTYPE_DRUM_KIT:
			szType = "Drum Kit";
			break;

		case XINPUT_DEVSUBTYPE_GAMEPAD:
			szType = "Gamepad";
			break;

		default:
			szType = "Unknown/Other";
			break;
		}

		if ( caps.Flags & XINPUT_CAPS_WIRELESS )
		{
			szConnection = "Wireless";
		}
		else
		{
			szConnection = "Wired";
		}

		ShowWindow(hDisconnected, SW_HIDE);
	}
	else
	{
		szType = "None";
		szConnection = "Disconnected";

		// reset all values
		SendMessage(hWhammy, PBM_SETPOS, 0, 0);
		SendMessage(hTilt, PBM_SETPOS, 0, 0);
		SendMessage(hSlider, PBM_SETPOS, 0, 0);
		for ( int i = 0; i < BUTTON_COUNT; i++ )
		{
			SendMessage(buttons[i].hControl, BM_SETCHECK, BST_UNCHECKED, 0);
		}

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
		for ( int i = 0; i < BUTTON_COUNT; i++ )
		{
			BUTTON_INFO *button = &buttons[i];
			DWORD checkValue = (state.Gamepad.wButtons & button->wXButton) ? BST_CHECKED : BST_UNCHECKED;
			SendMessage(button->hControl, BM_SETCHECK, checkValue, 0);
		}

		WORD whammyValue = state.Gamepad.sThumbRX + 0x8000;
		SendMessage(hWhammy, PBM_SETPOS, whammyValue, 0);

		WORD tiltValue = state.Gamepad.sThumbRY + 0x8000;
		SendMessage(hTilt, PBM_SETPOS, tiltValue, 0);

		WORD sliderValue = state.Gamepad.sThumbLX + 0x8000;
		SendMessage(hSlider, PBM_SETPOS, sliderValue, 0);
	}

	if ( dwResult != dwPrevResult )
	{
		// Device changed, refresh xinput caps
		SetPlayer(playerId);
		
		dwPrevResult = dwResult;
	}
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
		}

		hWhammy = GetDlgItem(hWnd, IDC_WHAMMY);
		hTilt = GetDlgItem(hWnd, IDC_TILT);
		hSlider = GetDlgItem(hWnd, IDC_SLIDER);

		hDisconnected = GetDlgItem(hWnd, IDC_DISCONNECTED);
		hType = GetDlgItem(hWnd, IDC_TYPE);
		hConnection = GetDlgItem(hWnd, IDC_CONNECTION);

		SendMessage(hWhammy, PBM_SETRANGE, 0, MAKELPARAM(0, 0xFFFF));
		SendMessage(hTilt, PBM_SETRANGE, 0, MAKELPARAM(0, 0xFFFF));
		SendMessage(hSlider, PBM_SETRANGE, 0, MAKELPARAM(0, 0xFFFF));

		// Set player 1 as default
		SendMessage(GetDlgItem(hWnd, IDC_PLAYER1), BM_SETCHECK, BST_CHECKED, 0);
		SetPlayer(0);

		SetTimer(hWnd, IDT_XINPUT, 8, NULL);
		break;

	case WM_TIMER:
		switch ( wParam )
		{
		case IDT_XINPUT:
			ReadXInput();
			break;

		default: return FALSE;
		}
		break;

	case WM_COMMAND:
		switch ( LOWORD(wParam) )
		{
		case IDC_PLAYER1:
			SetPlayer(0);
			break;

		case IDC_PLAYER2:
			SetPlayer(1);
			break;

		case IDC_PLAYER3:
			SetPlayer(2);
			break;

		case IDC_PLAYER4:
			SetPlayer(3);
			break;

		case IDOK:
		case IDCANCEL:
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		default: return FALSE;
		}
		break;

	case WM_CLOSE:
		KillTimer(hWnd, IDT_XINPUT);
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default: return FALSE;
	}

	return TRUE;
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
