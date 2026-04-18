// Rbpo_pz_6sem.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Rbpo_pz_6sem.h"
#include <shellapi.h>
#include <string>

#define MAX_LOADSTRING 100

HINSTANCE g_hInst;
WCHAR g_szTitle[MAX_LOADSTRING];
WCHAR g_szWindowClass[MAX_LOADSTRING];
HANDLE g_singleInstanceMutex = nullptr;
bool g_isExiting = false;
UINT g_taskbarCreatedMessage = 0;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    if (!CreateSingleInstanceMutex())
    {
        return FALSE;
    }

    LoadStringW(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RBPOPZ6SEM, g_szWindowClass, MAX_LOADSTRING);

    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    RegisterMainWindowClass(hInstance);

    const bool startHidden = IsStartHidden(lpCmdLine);
    if (!InitMainWindow(hInstance, nCmdShow, startHidden))
    {
        ReleaseSingleInstanceMutex();
        return FALSE;
    }

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ReleaseSingleInstanceMutex();
    return static_cast<int>(msg.wParam);
}

ATOM RegisterMainWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RBPOPZ6SEM));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_RBPOPZ6SEM);
    wcex.lpszClassName = g_szWindowClass;
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitMainWindow(HINSTANCE hInstance, int nCmdShow, bool startHidden)
{
    g_hInst = hInstance;

    HWND hWnd = CreateWindowW(g_szWindowClass, g_szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    if (!AddTrayIcon(hWnd))
    {
        DestroyWindow(hWnd);
        return FALSE;
    }

    if (!startHidden)
    {
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
    }
    else
    {
        ShowWindow(hWnd, SW_HIDE);
    }

    return TRUE;
}

bool IsStartHidden(LPWSTR cmdLine)
{
    if (cmdLine == nullptr)
    {
        return false;
    }

    const std::wstring args = cmdLine;
    return args.find(L"-tray") != std::wstring::npos ||
           args.find(L"/tray") != std::wstring::npos ||
           args.find(L"-hidden") != std::wstring::npos ||
           args.find(L"/hidden") != std::wstring::npos;
}

bool CreateSingleInstanceMutex()
{
    g_singleInstanceMutex = CreateMutexW(nullptr, FALSE, L"Local\\Rbpo_pz_6sem_single_instance");
    if (g_singleInstanceMutex == nullptr)
    {
        return false;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
        return false;
    }

    return true;
}

void ReleaseSingleInstanceMutex()
{
    if (g_singleInstanceMutex != nullptr)
    {
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
    }
}

bool AddTrayIcon(HWND hWnd)
{
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = TRAY_ICON_ID;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WMAPP_TRAYICON;
    nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_RBPOPZ6SEM));
    lstrcpynW(nid.szTip, g_szTitle, ARRAYSIZE(nid.szTip));

    return Shell_NotifyIconW(NIM_ADD, &nid) == TRUE;
}

void RemoveTrayIcon(HWND hWnd)
{
    NOTIFYICONDATAW nid{};
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hWnd;
    nid.uID = TRAY_ICON_ID;

    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void ShowTrayMenu(HWND hWnd)
{
    HMENU hMenu = CreatePopupMenu();
    if (hMenu == nullptr)
    {
        return;
    }

    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OPEN, L"Открыть");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"Выход");

    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}

void ShowMainWindow(HWND hWnd)
{
    ShowWindow(hWnd, SW_SHOW);
    ShowWindow(hWnd, SW_RESTORE);
    SetForegroundWindow(hWnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == g_taskbarCreatedMessage)
    {
        AddTrayIcon(hWnd);
        return 0;
    }

    switch (message)
    {
    case WM_COMMAND:
    {
        const int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_EXIT:
        case IDM_TRAY_EXIT:
            g_isExiting = true;
            DestroyWindow(hWnd);
            return 0;
        case IDM_TRAY_OPEN:
            ShowMainWindow(hWnd);
            return 0;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    case WMAPP_TRAYICON:
        switch (static_cast<UINT>(lParam))
        {
        case WM_LBUTTONUP:
            ShowMainWindow(hWnd);
            return 0;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayMenu(hWnd);
            return 0;
        default:
            break;
        }
        break;
    case WM_CLOSE:
        if (g_isExiting)
        {
            DestroyWindow(hWnd);
        }
        else
        {
            ShowWindow(hWnd, SW_HIDE);
        }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        UNREFERENCED_PARAMETER(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        RemoveTrayIcon(hWnd);
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
