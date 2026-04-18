#pragma once

#include "resource.h"
#include <windows.h>

constexpr UINT WMAPP_TRAYICON = WM_APP + 1;
constexpr UINT TRAY_ICON_ID = 1;

extern HINSTANCE g_hInst;
extern WCHAR g_szTitle[100];
extern WCHAR g_szWindowClass[100];
extern HANDLE g_singleInstanceMutex;
extern bool g_isExiting;
extern UINT g_taskbarCreatedMessage;

ATOM RegisterMainWindowClass(HINSTANCE hInstance);
BOOL InitMainWindow(HINSTANCE hInstance, int nCmdShow, bool startHidden);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

bool IsStartHidden(LPWSTR cmdLine);
bool CreateSingleInstanceMutex();
void ReleaseSingleInstanceMutex();

bool AddTrayIcon(HWND hWnd);
void RemoveTrayIcon(HWND hWnd);
void ShowTrayMenu(HWND hWnd);
void ShowMainWindow(HWND hWnd);
