//
// Written by Igor Tolmachev, IT Samples
//        mailto:support@itsamples.com
//
// Copyright (c) 2009.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability for any damage/loss of business that
// this product may cause.
//
// Expect bugs!
// 
// Please use and enjoy, and let me know of any bugs/mods/improvements 
// that you have found/implemented and I will fix/incorporate them into 
// this file new version. 

// NetworkIndicator.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "NetworkIndicator.h"
#include "commctrl.h"
#include <stdio.h>

#define MAX_LOADSTRING 100
#define WM_NOTIFYTRAY (WM_USER + 1)

#pragma warning(disable: 4996) // _CRT_SECURE_NO_WARNINGS: swprintf

// Global Variables:
HWND   m_hWnd = NULL;
HICON  m_hActiveIcon = NULL;
HICON  m_hInactiveIcon = NULL;
HICON  m_hSendIcon = NULL;
HICON  m_hReceiveIcon = NULL;

HANDLE m_hTcpThread = NULL;
BOOL   m_bSetIconContinue = TRUE;
BOOL   m_bWorkContinue = TRUE;
BOOL   m_bSettingsOpened = FALSE;
BOOL   m_bStatisticsOpened = FALSE;
BOOL   m_bTrafficOpened = FALSE;
BOOL   m_bIsUnderXP = FALSE;

BOOL   m_bDisplayTCP = TRUE;
BOOL   m_bDisplayUDP = FALSE;
BOOL   m_bDisplayICMP = FALSE;

int    m_nDuration = 100;
UINT   m_nStartUp = BST_UNCHECKED;
UINT   m_nVistaStyle = BST_UNCHECKED;

DWORD  m_dwPacketsReceived = 0;
DWORD  m_dwPacketsSent = 0;

HINSTANCE hInst = NULL;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	Settings(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	Statistics(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	Traffic(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	BOOL bAlreadyRunning = FALSE;

	HANDLE hMutexOneInstance = ::CreateMutex( NULL, FALSE, _T("NetworkIndicator"));
	bAlreadyRunning = ( ::GetLastError() == ERROR_ALREADY_EXISTS || ::GetLastError() == ERROR_ACCESS_DENIED);

	if(bAlreadyRunning)
		return FALSE;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_NETWORKINDICATOR, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NETWORKINDICATOR));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

BOOL SetTrayIcon(DWORD dwMessage, HICON hIcon) 
{ 
	BOOL bResult = FALSE; 

	NOTIFYICONDATA tnd; 

	tnd.cbSize = sizeof(NOTIFYICONDATA); 
	tnd.hWnd = m_hWnd; 
	tnd.uID = NULL; 
	tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP; 
	tnd.uCallbackMessage = WM_NOTIFYTRAY;
	tnd.hIcon = hIcon;

	TCHAR szMessage[64];
	memset(szMessage, 0, 64*sizeof(TCHAR));
	_stprintf(szMessage, _T("Network Indicator\nSent: %u packets\nRcvd: %u packets"), m_dwPacketsSent, m_dwPacketsReceived);
	szMessage[63] = '\0';
	lstrcpyn(tnd.szTip, szMessage, 64);

	bResult = Shell_NotifyIcon(dwMessage, &tnd); 

	return bResult; 
}

void TCPThread(LPVOID pInfo)
{
	MIB_TCPSTATS  mibTcpStats;
	MIB_UDPSTATS  mibUdpStats;
	MIB_ICMP      mibIcmpStats;

	UINT nCounter = 0;
	DWORD dwSegTcpRcvd = 0;
	DWORD dwSegTcpSent = 0;
	DWORD dwSegUdpRcvd = 0;
	DWORD dwSegUdpSent = 0;
	DWORD dwSegIcmpRcvd = 0;
	DWORD dwSegIcmpSent = 0;
	DWORD dwLocalOutSegs = 0;
	DWORD dwLocalInSegs = 0;

	while(m_bWorkContinue)
	{
		if(m_bSetIconContinue)
		{
			m_dwPacketsSent = 0;
			m_dwPacketsReceived = 0;

			m_bSetIconContinue = FALSE;

			if(m_bDisplayTCP)
			{
				if(GetTcpStatistics(&mibTcpStats) == NO_ERROR)
				{
					dwLocalInSegs = mibTcpStats.dwInSegs;
					dwLocalOutSegs = mibTcpStats.dwOutSegs;

					m_dwPacketsSent += dwLocalOutSegs;
					m_dwPacketsReceived += dwLocalInSegs;

					if(dwLocalOutSegs > dwSegTcpSent && dwLocalInSegs > dwSegTcpRcvd)
					{
						dwSegTcpSent = dwLocalOutSegs;
						dwSegTcpRcvd = dwLocalInSegs;
						SetTrayIcon(NIM_MODIFY, m_hActiveIcon);
						goto done;
					}
					else if(dwLocalOutSegs > dwSegTcpSent && dwLocalInSegs <= dwSegTcpRcvd)
					{
						dwSegTcpSent = dwLocalOutSegs;
						SetTrayIcon(NIM_MODIFY, m_hSendIcon);
						goto done;
					}
					else if(dwLocalInSegs > dwSegTcpRcvd && dwLocalOutSegs <= dwSegTcpSent)
					{
						dwSegTcpRcvd = dwLocalInSegs;
						SetTrayIcon(NIM_MODIFY, m_hReceiveIcon);
						goto done;
					}
					else 
						nCounter++;

					if(nCounter == 10)
					{
						nCounter = 0;
						SetTrayIcon(NIM_MODIFY, m_hInactiveIcon);
						goto done;
					}
				}
			}

			if(m_bDisplayUDP)
			{
				if(GetUdpStatistics(&mibUdpStats) == NO_ERROR)
				{
					dwLocalInSegs = mibUdpStats.dwInDatagrams;
					dwLocalOutSegs = mibUdpStats.dwOutDatagrams;

					m_dwPacketsSent += dwLocalOutSegs;
					m_dwPacketsReceived += dwLocalInSegs;

					if(dwLocalOutSegs > dwSegUdpSent && dwLocalInSegs > dwSegUdpRcvd)
					{
						dwSegUdpSent = dwLocalOutSegs;
						dwSegUdpRcvd = dwLocalInSegs;
						SetTrayIcon(NIM_MODIFY, m_hActiveIcon);
						goto done;
					}
					else if(dwLocalOutSegs > dwSegUdpSent && dwLocalInSegs <= dwSegUdpRcvd)
					{
						dwSegUdpSent = dwLocalOutSegs;
						SetTrayIcon(NIM_MODIFY, m_hSendIcon);
						goto done;
					}
					else if(dwLocalInSegs > dwSegUdpRcvd && dwLocalOutSegs <= dwSegUdpSent)
					{
						dwSegUdpRcvd = dwLocalInSegs;
						SetTrayIcon(NIM_MODIFY, m_hReceiveIcon);
						goto done;
					}
					else 
						nCounter++;

					if(nCounter == 10)
					{
						nCounter = 0;
						SetTrayIcon(NIM_MODIFY, m_hInactiveIcon);
						goto done;
					}
				}
			}

			if(m_bDisplayICMP)
			{
				if(GetIcmpStatistics(&mibIcmpStats) == NO_ERROR)
				{
					dwLocalInSegs = mibIcmpStats.stats.icmpInStats.dwMsgs;
					dwLocalOutSegs = mibIcmpStats.stats.icmpOutStats.dwMsgs;

					m_dwPacketsSent += dwLocalOutSegs;
					m_dwPacketsReceived += dwLocalInSegs;

					if(dwLocalOutSegs > dwSegIcmpSent && dwLocalInSegs > dwSegIcmpRcvd)
					{
						dwSegIcmpSent = dwLocalOutSegs;
						dwSegIcmpRcvd = dwLocalInSegs;
						SetTrayIcon(NIM_MODIFY, m_hActiveIcon);
						goto done;
					}
					else if(dwLocalOutSegs > dwSegIcmpSent && dwLocalInSegs <= dwSegIcmpRcvd)
					{
						dwSegIcmpSent = dwLocalOutSegs;
						SetTrayIcon(NIM_MODIFY, m_hSendIcon);
						goto done;
					}
					else if(dwLocalInSegs > dwSegIcmpRcvd && dwLocalOutSegs <= dwSegIcmpSent)
					{
						dwSegIcmpRcvd = dwLocalInSegs;
						SetTrayIcon(NIM_MODIFY, m_hReceiveIcon);
						goto done;
					}
					else 
						nCounter++;

					if(nCounter == 10)
					{
						nCounter = 0;
						SetTrayIcon(NIM_MODIFY, m_hInactiveIcon);
						goto done;
					}
				}
			}
done:
			m_bSetIconContinue = TRUE;
		}

		Sleep(m_nDuration);
	}

	m_hTcpThread = NULL;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NETWORKINDICATOR));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	m_hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!m_hWnd)
	{
		return FALSE;
	}

	m_hActiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_ACTIVE));
	m_hInactiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_INACTIVE));
	m_hSendIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_SEND));
	m_hReceiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_RECEIVE));

	HKEY hKey = NULL;
	DWORD dwType = NULL;
	DWORD dwRes = NULL;
	DWORD dwSize = sizeof(DWORD);
	LONG nQuery = RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\ITSamples\\NetworkIndicator"),0,KEY_ALL_ACCESS,&hKey);
	if (nQuery == ERROR_SUCCESS)
	{
		LONG nQue = RegQueryValueEx(hKey, _T("UseVistaStyle"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
		if(nQue == ERROR_SUCCESS && dwRes != 0) 
		{
			m_hActiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_ACTIVEVISTA));
			m_hInactiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_INACTIVEVISTA));
			m_hSendIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_SENDVISTA));
			m_hReceiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_RECEIVEVISTA));
		}

		dwRes = NULL;
		dwSize = sizeof(DWORD);
		nQue = RegQueryValueEx(hKey, _T("Duration"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
		if(nQue == ERROR_SUCCESS)
		{
			if(dwRes < 50) 
				dwRes = 50;

			if(dwRes > 5000)
				dwRes = 5000;

			m_nDuration = dwRes;
		}

		dwRes = NULL;
		dwSize = sizeof(DWORD);
		nQue = RegQueryValueEx(hKey, _T("DisplayTCP"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
		if(nQue == ERROR_SUCCESS)
			m_bDisplayTCP = (BOOL)dwRes;

		dwRes = NULL;
		dwSize = sizeof(DWORD);
		nQue = RegQueryValueEx(hKey, _T("DisplayUDP"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
		if(nQue == ERROR_SUCCESS)
			m_bDisplayUDP = (BOOL)dwRes;

		dwRes = NULL;
		dwSize = sizeof(DWORD);
		nQue = RegQueryValueEx(hKey, _T("DisplayICMP"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
		if(nQue == ERROR_SUCCESS)
			m_bDisplayICMP = (BOOL)dwRes;
	}
	if(hKey) 
		RegCloseKey(hKey);

	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	
	if(osvi.dwMajorVersion < 6)
		m_bIsUnderXP = TRUE;

	SetTrayIcon(NIM_ADD, m_hInactiveIcon);

	ShowWindow(m_hWnd, SW_HIDE);
	UpdateWindow(m_hWnd);

	m_hTcpThread = (HANDLE)_beginthread(TCPThread, 0, m_hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) 
	{
		case WM_COMMAND:
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
					DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
					break;
				case IDM_SETTINGS:
					if(!m_bSettingsOpened)
						DialogBox(hInst, (LPCTSTR)IDD_DIALOG_SETTINGS, hWnd, (DLGPROC)Settings);
					break;
				case IDM_TRAFFIC:
					if(!m_bTrafficOpened)
						DialogBox(hInst, (LPCTSTR)IDD_DIALOG_TRAFFIC, hWnd, (DLGPROC)Traffic);
					break;
				case IDM_STATISTICS:
					if(!m_bStatisticsOpened)
						DialogBox(hInst, (LPCTSTR)IDD_DIALOG_STATISTICS, hWnd, (DLGPROC)Statistics);
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;
				case IDM_NETWORK:
					::ShellExecute(hWnd, _T("open"), _T("rundll32.exe"), _T("shell32.dll,Control_RunDLL ncpa.cpl"), NULL, SW_SHOWNORMAL);
					break;
				case IDM_FIREWALL:
					::ShellExecute(hWnd, _T("open"), _T("rundll32.exe"), _T("shell32.dll,Control_RunDLL firewall.cpl"), NULL, SW_SHOWNORMAL);
					break;
				case ID_POPUP_NETWORKCENTER:
					::ShellExecute(hWnd, _T("open"), _T("control.exe"), _T(" /name Microsoft.NetworkAndSharingCenter"), NULL, SW_SHOWNORMAL);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...
			EndPaint(hWnd, &ps);
			break;
		case WM_DESTROY:
			SetTrayIcon(NIM_DELETE, NULL);
			
			m_bWorkContinue = FALSE;
			while(m_hTcpThread) Sleep(m_nDuration);

			PostQuitMessage(0);
			break;
		case WM_NOTIFYTRAY:
			if(/*lParam == WM_LBUTTONDOWN || */lParam == WM_RBUTTONDOWN)
			{
				HMENU hMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDM_TRAY));
				HMENU hSubMenu = GetSubMenu(hMenu,0);
				if (!hMenu) break;
				if (!hSubMenu) break;

				if(m_bIsUnderXP)
					DeleteMenu(hSubMenu, ID_POPUP_NETWORKCENTER, MF_BYCOMMAND);

				// Display and track the popup menu
				POINT pos;
				GetCursorPos(&pos);

				SetForegroundWindow(hWnd);

				TrackPopupMenu(hSubMenu, 0, pos.x, pos.y, 0, hWnd, NULL);

				PostMessage(hWnd, WM_NULL, 0, 0);
				DestroyMenu(hMenu);
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

LRESULT CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			m_bSettingsOpened = TRUE;

			HWND hDuration = GetDlgItem(hDlg, IDC_EDIT_DURATION);

			CheckDlgButton(hDlg, IDC_CHECK_DISPLAYTCP, BST_CHECKED);
			CheckDlgButton(hDlg,IDC_CHECK_STARTUP, BST_UNCHECKED);
			SetWindowText(hDuration, _T("100"));

			HKEY hKey = NULL;
			DWORD dwType = NULL;
			DWORD dwRes = NULL;
			DWORD dwSize = sizeof(DWORD);
			LONG nQuery = RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\ITSamples\\NetworkIndicator"), 0, KEY_ALL_ACCESS, &hKey);
			if (nQuery == ERROR_SUCCESS)
			{
				LONG nQue = RegQueryValueEx(hKey, _T("StartUp"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
				if(nQue == ERROR_SUCCESS  && dwRes !=0)
					CheckDlgButton(hDlg,IDC_CHECK_STARTUP, BST_CHECKED);

				dwRes = NULL;
				nQue = RegQueryValueEx(hKey, _T("UseVistaStyle"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
				if(nQue == ERROR_SUCCESS  && dwRes !=0) 
					CheckDlgButton(hDlg, IDC_CHECK_VISTASTYLE, BST_CHECKED);

				dwRes = NULL;
				nQue = RegQueryValueEx(hKey, _T("DisplayTCP"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
				if(nQue == ERROR_SUCCESS)
				{
					if(dwRes !=0) 
						CheckDlgButton(hDlg, IDC_CHECK_DISPLAYTCP, BST_CHECKED);
					else
						CheckDlgButton(hDlg, IDC_CHECK_DISPLAYTCP, BST_UNCHECKED);
				}

				dwRes = NULL;
				nQue = RegQueryValueEx(hKey, _T("DisplayUDP"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
				if(nQue == ERROR_SUCCESS) 
				{
					if(dwRes !=0)
						CheckDlgButton(hDlg, IDC_CHECK_DISPLAYUDP, BST_CHECKED);
					else
						CheckDlgButton(hDlg, IDC_CHECK_DISPLAYUDP, BST_UNCHECKED);
				}

				dwRes = NULL;
				nQue = RegQueryValueEx(hKey, _T("DisplayICMP"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
				if(nQue == ERROR_SUCCESS) 
				{
					if(dwRes !=0)
						CheckDlgButton(hDlg, IDC_CHECK_DISPLAYICMP, BST_CHECKED);
					else
						CheckDlgButton(hDlg, IDC_CHECK_DISPLAYICMP, BST_UNCHECKED);
				}

				dwRes = NULL;
				dwSize = sizeof(DWORD);
				nQue = RegQueryValueEx(hKey, _T("Duration"), NULL, &dwType, (LPBYTE)&dwRes, &dwSize);
				if(nQue == ERROR_SUCCESS)
				{
					if(dwRes < 50) dwRes = 50;

					TCHAR szDuration[16];
					memset(szDuration, 0, 16*sizeof(TCHAR));
					_stprintf(szDuration, _T("%i"), dwRes);
					SetWindowText(hDuration, szDuration);
				}
			}
			if(hKey) 
				RegCloseKey(hKey);

			m_nStartUp = IsDlgButtonChecked(hDlg, IDC_CHECK_STARTUP);
			m_nVistaStyle = IsDlgButtonChecked(hDlg, IDC_CHECK_VISTASTYLE);

			return TRUE;
		}
	case WM_COMMAND:
			if(LOWORD(wParam) == IDOK)
			{
				HWND hDuration = GetDlgItem(hDlg, IDC_EDIT_DURATION);
				HKEY hKey = NULL;
				DWORD dwType = NULL;

				if(!IsDlgButtonChecked(hDlg, IDC_CHECK_DISPLAYTCP) && !IsDlgButtonChecked(hDlg, IDC_CHECK_DISPLAYUDP) && !IsDlgButtonChecked(hDlg, IDC_CHECK_DISPLAYICMP))
					CheckDlgButton(hDlg, IDC_CHECK_DISPLAYTCP, BST_CHECKED);

				LONG nQuery = RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\ITSamples\\NetworkIndicator"), 0, KEY_ALL_ACCESS, &hKey);
				if(nQuery != ERROR_SUCCESS) nQuery = RegCreateKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\ITSamples\\NetworkIndicator"), NULL, REG_NONE, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwType);
				if(nQuery == ERROR_SUCCESS)
				{
					DWORD dwValue = IsDlgButtonChecked(hDlg, IDC_CHECK_STARTUP);
					RegSetValueEx(hKey, _T("StartUp"), NULL, REG_DWORD, (BYTE* const)&dwValue,sizeof(DWORD));
					
					dwValue = IsDlgButtonChecked(hDlg, IDC_CHECK_VISTASTYLE);
					RegSetValueEx(hKey, _T("UseVistaStyle"), NULL, REG_DWORD, (BYTE* const)&dwValue, sizeof(DWORD));

					dwValue = IsDlgButtonChecked(hDlg, IDC_CHECK_DISPLAYTCP);
					RegSetValueEx(hKey, _T("DisplayTCP"), NULL, REG_DWORD, (BYTE* const)&dwValue, sizeof(DWORD));
					m_bDisplayTCP = (BOOL)dwValue;

					dwValue = IsDlgButtonChecked(hDlg, IDC_CHECK_DISPLAYUDP);
					RegSetValueEx(hKey, _T("DisplayUDP"), NULL,REG_DWORD, (BYTE* const)&dwValue, sizeof(DWORD));
					m_bDisplayUDP = (BOOL)dwValue;

					dwValue = IsDlgButtonChecked(hDlg, IDC_CHECK_DISPLAYICMP);
					RegSetValueEx(hKey, _T("DisplayICMP"), NULL, REG_DWORD, (BYTE* const)&dwValue, sizeof(DWORD));
					m_bDisplayICMP = (BOOL)dwValue;

					TCHAR szDuration[16];
					memset(szDuration, 0, 16*sizeof(TCHAR));
					GetWindowText(hDuration, szDuration, 15);

					dwValue = _tstol(szDuration);
					
					if(dwValue < 50)
						dwValue = 50;

					m_nDuration = dwValue;

					RegSetValueEx(hKey, _T("Duration"), NULL, REG_DWORD, (BYTE* const)&dwValue, sizeof(DWORD));
				}
				if(hKey)
					RegCloseKey(hKey);

				if(m_nVistaStyle != IsDlgButtonChecked(hDlg, IDC_CHECK_VISTASTYLE)) // icon style changed
				{
					BOOL bVistaStyle = IsDlgButtonChecked(hDlg, IDC_CHECK_VISTASTYLE);
					if(bVistaStyle)
					{
						m_hActiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_ACTIVEVISTA));
						m_hInactiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_INACTIVEVISTA));
						m_hSendIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_SENDVISTA));
						m_hReceiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_RECEIVEVISTA));
					}
					else
					{
						m_hActiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_ACTIVE));
						m_hInactiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_INACTIVE));
						m_hSendIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_SEND));
						m_hReceiveIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_RECEIVE));
					}
				}

				if(m_nStartUp != IsDlgButtonChecked(hDlg,IDC_CHECK_STARTUP)) // modified
				{
					nQuery = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hKey);
					if(nQuery == ERROR_SUCCESS)
					{
						if(IsDlgButtonChecked(hDlg,IDC_CHECK_STARTUP) == BST_CHECKED)
						{
							TCHAR szPath[MAX_PATH]; 
							GetModuleFileName(NULL, szPath, sizeof(szPath));
							RegSetValueEx(hKey, _T("NetworkIndicator"), NULL, REG_SZ, (LPBYTE)szPath, (DWORD)lstrlen(szPath)); 
						}
						else 
							RegDeleteValue(hKey, _T("NetworkIndicator"));
					}
					if(hKey) 
						RegCloseKey(hKey);
				}
								
				EndDialog(hDlg, LOWORD(wParam));

				m_bSettingsOpened = FALSE;

				return TRUE;
			}
			else if(LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				m_bSettingsOpened = FALSE;
				return TRUE;
			}
			break;
	}
    return FALSE;
}
void GetStatistics(HWND hWndList)
{
	MIB_TCPSTATS  mibTcpStats;

	if(GetTcpStatistics(&mibTcpStats) == NO_ERROR)
	{
		TCHAR szText[255];
		memset(szText, 0, 255*sizeof(TCHAR));
		switch(mibTcpStats.dwRtoAlgorithm)
		{
		case MIB_TCP_RTO_CONSTANT:
			_tcscpy(szText, _T("Constant Time-Out"));
			break;
		case MIB_TCP_RTO_RSRE:
			_tcscpy(szText, _T("MIL-STD-1778"));
			break;
		case MIB_TCP_RTO_VANJ:
			_tcscpy(szText, _T("Van Jacobson's"));
			break;
		default:
		case MIB_TCP_RTO_OTHER:
			_tcscpy(szText, _T("Unknown"));
			break;
		}

		ListView_SetItemText(hWndList, 0, 0, _T("Time Out Algorithm:"));
		ListView_SetItemText(hWndList, 0, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwRtoMin);
		ListView_SetItemText(hWndList, 1, 0, _T("Minimum Timeout (ms):"));
		ListView_SetItemText(hWndList, 1, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwRtoMax);
		ListView_SetItemText(hWndList, 2, 0, _T("Maximum Timeout (ms):"));
		ListView_SetItemText(hWndList, 2, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwMaxConn);
		ListView_SetItemText(hWndList, 3, 0, _T("Maximum Connections:"));
		ListView_SetItemText(hWndList, 3, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwActiveOpens);
		ListView_SetItemText(hWndList, 4, 0, _T("Active Open Connections:"));
		ListView_SetItemText(hWndList, 4, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwPassiveOpens);
		ListView_SetItemText(hWndList, 5, 0, _T("Passive Open Connections:"));
		ListView_SetItemText(hWndList, 5, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwAttemptFails);
		ListView_SetItemText(hWndList, 6, 0, _T("Failed Connection Attempts:"));
		ListView_SetItemText(hWndList, 6, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwEstabResets);
		ListView_SetItemText(hWndList, 7, 0, _T("Total Connections Reset:"));
		ListView_SetItemText(hWndList, 7, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwCurrEstab);
		ListView_SetItemText(hWndList, 8, 0, _T("Current Established Connections:"));
		ListView_SetItemText(hWndList, 8, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwInSegs);
		ListView_SetItemText(hWndList, 9, 0, _T("Segments Received:"));
		ListView_SetItemText(hWndList, 9, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwOutSegs);
		ListView_SetItemText(hWndList, 10, 0, _T("Segments Sent:"));
		ListView_SetItemText(hWndList, 10, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwRetransSegs);
		ListView_SetItemText(hWndList, 11, 0, _T("Segments Retransmitted:"));
		ListView_SetItemText(hWndList, 11, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwInErrs);
		ListView_SetItemText(hWndList, 12, 0, _T("Errors Received:"));
		ListView_SetItemText(hWndList, 12, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwOutRsts);
		ListView_SetItemText(hWndList, 13, 0, _T("Segments sent with reset flag:"));
		ListView_SetItemText(hWndList, 13, 1, szText);

		_stprintf(szText, _T("%u"), mibTcpStats.dwNumConns);
		ListView_SetItemText(hWndList, 14, 0, _T("Cumulative Number of Connections:"));
		ListView_SetItemText(hWndList, 14, 1, szText);

		MIB_UDPSTATS  mibUdpStats;
		MIB_ICMP      mibIcmpStats;

		int nIndex = 15;
		if(GetUdpStatistics(&mibUdpStats) == NO_ERROR)
		{
			_stprintf(szText, _T("%u"), mibUdpStats.dwOutDatagrams);
			ListView_SetItemText(hWndList, nIndex, 0, _T("UDP Datagrams Sent:"));
			ListView_SetItemText(hWndList, nIndex, 1, szText);
			nIndex++;

			_stprintf(szText, _T("%u"), mibUdpStats.dwInDatagrams);
			ListView_SetItemText(hWndList, nIndex, 0, _T("UDP Datagrams Received:"));
			ListView_SetItemText(hWndList, nIndex, 1, szText);
			nIndex++;
		}
		if(GetIcmpStatistics(&mibIcmpStats) == NO_ERROR)
		{
			_stprintf(szText, _T("%u"), mibIcmpStats.stats.icmpOutStats.dwMsgs);
			ListView_SetItemText(hWndList, nIndex, 0, _T("ICMP Packets Sent:"));
			ListView_SetItemText(hWndList, nIndex, 1, szText);
			nIndex++;

			_stprintf(szText, _T("%u"), mibIcmpStats.stats.icmpInStats.dwMsgs);
			ListView_SetItemText(hWndList, nIndex, 0, _T("ICMP Packets Received:"));
			ListView_SetItemText(hWndList, nIndex, 1, szText);
		}
	}	
	else
	{
		for(int i=0; i<19; i++)
			ListView_SetItemText(hWndList, i, 1, _T("..."));
	}
}

LRESULT CALLBACK Statistics(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			m_bStatisticsOpened = TRUE;

			HWND hWndList = GetDlgItem(hDlg, IDC_LIST_INFO);

			LV_COLUMN lvColumn;
			memset(&lvColumn, 0, sizeof(LV_COLUMN));
			lvColumn.fmt = LVCFMT_LEFT;
			lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;

			DWORD dwStyle = GetWindowLong(hWndList, GWL_STYLE);
			SetWindowLong(hWndList, GWL_STYLE, dwStyle|LVS_REPORT);
			ListView_SetExtendedListViewStyle(hWndList, LVS_EX_GRIDLINES);

			lvColumn.cx = 208;
			lvColumn.pszText = _T("   Parameters");
			ListView_InsertColumn(hWndList, 0, &lvColumn);

			lvColumn.cx = 100;
			lvColumn.pszText = _T("   Value");
			ListView_InsertColumn(hWndList, 1, &lvColumn);

			LVITEM lvItem;
			memset(&lvItem, 0, sizeof(LVITEM));

			lvItem.mask = LVIF_TEXT;
			lvItem.iImage = 0;
			lvItem.iSubItem = 0;
			lvItem.pszText = _T("");

			for(int i=0; i<19; i++)
			{
				lvItem.iItem = i;
				ListView_InsertItem(hWndList, &lvItem);
			}

			ListView_SetBkColor(hWndList, RGB(200,230,255));
			ListView_SetTextBkColor(hWndList, RGB(200,230,255));
			ListView_SetTextColor(hWndList, RGB(0,0,50));

			GetStatistics(hWndList);
			
			return TRUE;
		}
	case WM_COMMAND:
			if(LOWORD(wParam) == IDOK)
			{
				EndDialog(hDlg, LOWORD(wParam));
				m_bStatisticsOpened = FALSE;
				return TRUE;
			}
			else if(LOWORD(wParam) == IDC_BUTTON_REFRESH)
			{
				HWND hWndList = GetDlgItem(hDlg, IDC_LIST_INFO);
				GetStatistics(hWndList);

				return TRUE;
			}
			else if(LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				m_bStatisticsOpened = FALSE;
				return TRUE;
			}
			break;
	}

	return FALSE;
}

void GetTrafiic(HWND hDlg)
{
	HWND hWndCombo = GetDlgItem(hDlg, IDC_COMBO_ADAPTERS);

	int nIndex = SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);
	if(nIndex != CB_ERR)
	{
		// Declare and initialize variables
		PMIB_IFTABLE ifTable;
		DWORD dwSize = 0;
		DWORD dwRetVal = 0;

		// Allocate memory for our pointers
		ifTable = (MIB_IFTABLE*) malloc(sizeof(MIB_IFTABLE));

		// Make an initial call to GetIfTable to get the
		// necessary size into the dwSize variable
		if (GetIfTable(ifTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			GlobalFree(ifTable);
			ifTable = (MIB_IFTABLE *) malloc(dwSize);
		}

		// Make a second call to GetIfTable to get the 
		// actual data we want
		if ((dwRetVal = GetIfTable(ifTable, &dwSize, TRUE)) == NO_ERROR)
		{
			MIB_IFROW row = ifTable->table[nIndex];

			TCHAR szValue[32];

			memset(szValue, 0, 32*sizeof(TCHAR));
			if(row.dwInOctets > 1048576)
				_stprintf(szValue, _T("%0.3f MB"), (float)row.dwInOctets/(1024.0*1024.0));
			else if(row.dwInOctets > 1024)
				_stprintf(szValue, _T("%0.2f KB"), (float)row.dwInOctets/1024.0);
			else
				_stprintf(szValue, _T("%d B"), row.dwInOctets);
			SetDlgItemText(hDlg, IDC_STATIC_RECEIVED, szValue);

			memset(szValue, 0, 32);

			if(row.dwOutOctets > 1048576)
				_stprintf(szValue, _T("%0.3f MB"), (float)row.dwOutOctets/(1024.0*1024.0));
			else if(row.dwOutOctets > 1024)
				_stprintf(szValue, _T("%0.2f KB"), (float)row.dwOutOctets/1024.0);
			else
				_stprintf(szValue, _T("%d B"), row.dwOutOctets);

			SetDlgItemText(hDlg, IDC_STATIC_SENT, szValue);
		}

		if(ifTable)
			GlobalFree(ifTable);
	}
}

LRESULT CALLBACK Traffic(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			m_bTrafficOpened = TRUE;
			
			// Declare and initialize variables
			PMIB_IFTABLE ifTable;
			DWORD dwSize = 0;
			DWORD dwRetVal = 0;

			// Allocate memory for our pointers
			ifTable = (MIB_IFTABLE*) malloc(sizeof(MIB_IFTABLE));

			// Make an initial call to GetIfTable to get the
			// necessary size into the dwSize variable
			if (GetIfTable(ifTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
			{
				GlobalFree(ifTable);
				ifTable = (MIB_IFTABLE *) malloc(dwSize);
			}

			// Make a second call to GetIfTable to get the 
			// actual data we want
			if ((dwRetVal = GetIfTable(ifTable, &dwSize, TRUE)) == NO_ERROR)
			{
				HWND hWndCombo = GetDlgItem(hDlg, IDC_COMBO_ADAPTERS);
				for (UINT i=0; i<ifTable->dwNumEntries; i++)
				{
					MIB_IFROW row = ifTable->table[i];

					CHAR szDescr[MAXLEN_IFDESCR];
					memset(szDescr, 0, MAXLEN_IFDESCR);
					memcpy(szDescr, row.bDescr, row.dwDescrLen);
					szDescr[row.dwDescrLen] = '\0';

					WCHAR wszDescr[MAXLEN_IFDESCR];
					memset(wszDescr, 0, MAXLEN_IFDESCR*sizeof(WCHAR));
					MultiByteToWideChar(CP_ACP, 0, szDescr, -1, wszDescr, MAXLEN_IFDESCR); 

					if(wcslen(wszDescr) > 1)
						SendMessage(hWndCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(wszDescr));
				}

				SendMessage(hWndCombo, CB_SETCURSEL, 0, 0);

				GetTrafiic(hDlg);
			}

			if(ifTable)
				GlobalFree(ifTable);

			return TRUE;
		}
	case WM_COMMAND:
			if(LOWORD(wParam) == IDOK)
			{
				EndDialog(hDlg, LOWORD(wParam));
				m_bTrafficOpened = FALSE;
				return TRUE;
			}
			else if(LOWORD(wParam) == IDC_BUTTON_REFRESH)
			{
				GetTrafiic(hDlg);
				return TRUE;
			}
			else if(LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				m_bTrafficOpened = FALSE;
				return TRUE;
			}
			else if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				GetTrafiic(hDlg);
				return TRUE;
			}

			break;
	}

	return FALSE;
}
