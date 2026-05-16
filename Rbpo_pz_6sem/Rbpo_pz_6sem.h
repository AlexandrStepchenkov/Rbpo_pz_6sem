#pragma once

#include "Resource.h"
#include <shellapi.h>
#include <string>

#define MAX_LOADSTRING 100

constexpr UINT WM_TRAYICON = WM_APP + 1;
constexpr UINT ID_TRAY_ICON = 1;

extern HINSTANCE hInst;
extern WCHAR szTitle[MAX_LOADSTRING];
extern WCHAR szWindowClass[MAX_LOADSTRING];

extern UINT g_taskbarCreatedMessage;
extern NOTIFYICONDATAW g_notifyIconData;
extern bool g_trayAdded;
extern bool g_isExiting;
extern HANDLE g_singleInstanceMutex;
extern bool g_antivirusEnabled;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, bool startHidden);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

std::wstring BuildMutexNamePerUser();
void ShowMainWindow(HWND hWnd);
void AddTrayIcon(HWND hWnd);
void RemoveTrayIcon();
void ShowTrayContextMenu(HWND hWnd);
void RefreshLicenseStatus(HWND hWnd);
void RefreshAvStatusText();
void ScanSelectedFile(HWND owner);
void ScanSelectedFolder(HWND owner);
void ShowScanResults(HWND owner);
void ScanAllFixedDrives(HWND owner);
void ConfigureScheduledScan(HWND owner);
void ToggleScheduledScan(HWND owner);
void AddMonitoringDirectory(HWND owner);
void ShowMonitoredDirectories(HWND owner);
