#include "framework.h"
#include "Rbpo_pz_6sem.h"
#include <lmcons.h>
#include <tlhelp32.h>
#include <rpc.h>
#include <commdlg.h>
#include <shlobj.h>
#include <cstdlib>
#include <array>
#include <mutex>
#include <vector>
#include <Aclapi.h>
#include <Accctrl.h>

#include "common/AppConfig.h"
#include "rpc/ServiceControl.h"

#if defined(_M_ARM64) || defined(_M_ARM64EC)
extern "C" {
#include "rpc/ServiceControl_c_arm64.c"
}
#elif defined(_M_IX86)
extern "C" {
#include "rpc/ServiceControl_c_win32.c"
}
#else
extern "C" {
#include "rpc/ServiceControl_c_x64.c"
}
#endif

#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Comdlg32.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

UINT g_taskbarCreatedMessage = 0;
NOTIFYICONDATAW g_notifyIconData{};
bool g_trayAdded = false;
bool g_isExiting = false;
HANDLE g_singleInstanceMutex = nullptr;
HWND g_statusText = nullptr;
HWND g_userText = nullptr;
HWND g_licenseText = nullptr;
HWND g_expirationText = nullptr;
HWND g_scanButton = nullptr;
HWND g_avStatusText = nullptr;
bool g_antivirusEnabled = false;
bool g_uiFlowRunning = false;
bool g_scheduleEnabled = false;
unsigned int g_scheduleIntervalMinutes = 60;
std::wstring g_lastScanSummary;
std::vector<std::wstring> g_monitoredDirectories;
std::mutex g_stateMutex;
constexpr UINT_PTR kScheduleTimerId = 2;
constexpr UINT WM_APP_STARTUP = WM_APP + 10;
constexpr UINT WM_APP_REFRESH_LICENSE = WM_APP + 11;

std::wstring GetLogDirectory()
{
    wchar_t programData[MAX_PATH]{};
    DWORD len = GetEnvironmentVariableW(L"ProgramData", programData, MAX_PATH);
    std::wstring base = len > 0 ? std::wstring(programData, len) : L".";
    std::wstring dir = base + L"\\RbpoPz";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

std::wstring GetLogPath()
{
    return GetLogDirectory() + L"\\tray.log";
}

void LogMessage(const std::wstring& message)
{
    const std::wstring path = GetLogPath();
    if (path.empty())
    {
        return;
    }

    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t timestamp[64]{};
    swprintf_s(timestamp, L"%04u-%02u-%02u %02u:%02u:%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    std::wstring line = L"[" + std::wstring(timestamp) + L"] " + message + L"\r\n";
    int size = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
    {
        return;
    }

    std::string bytes(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, line.c_str(), -1, bytes.data(), size, nullptr, nullptr);

    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD written = 0;
    WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    CloseHandle(file);
}

void CenterWindow(HWND hWnd);

bool CreateRpcBinding(handle_t& binding)
{
    RPC_WSTR bindingText = nullptr;
    RPC_STATUS status = RpcStringBindingComposeW(
        nullptr,
        (RPC_WSTR)L"ncalrpc",
        nullptr,
        (RPC_WSTR)kRpcEndpoint,
        nullptr,
        &bindingText);

    if (status != RPC_S_OK)
    {
        return false;
    }

    status = RpcBindingFromStringBindingW(bindingText, &binding);
    RpcStringFreeW(&bindingText);
    return status == RPC_S_OK;
}

void FreeRpcBinding(handle_t binding)
{
    if (binding)
    {
        RpcBindingFree(&binding);
    }
}

DWORD RpcCallGetUserInfo(bool& isAuthenticated, std::wstring& username)
{
    handle_t binding = nullptr;
    if (!CreateRpcBinding(binding))
    {
        return ERROR_GEN_FAILURE;
    }

    long auth = 0;
    wchar_t* name = nullptr;
    long status = ERROR_GEN_FAILURE;

    RpcTryExcept
    {
        status = RpcGetUserInfo(binding, &auth, &name);
    }
    RpcExcept(1)
    {
        status = ERROR_GEN_FAILURE;
    }
    RpcEndExcept;

    FreeRpcBinding(binding);

    if (name)
    {
        username = name;
        MIDL_user_free(name);
    }

    isAuthenticated = auth != 0;
    return status;
}

DWORD RpcCallLogin(const std::wstring& username, const std::wstring& password)
{
    handle_t binding = nullptr;
    if (!CreateRpcBinding(binding))
    {
        return ERROR_GEN_FAILURE;
    }

    long status = ERROR_GEN_FAILURE;
    RpcTryExcept
    {
        status = RpcLogin(binding, username.c_str(), password.c_str());
    }
    RpcExcept(1)
    {
        status = ERROR_GEN_FAILURE;
    }
    RpcEndExcept;

    FreeRpcBinding(binding);
    return status;
}

DWORD RpcCallLogout()
{
    handle_t binding = nullptr;
    if (!CreateRpcBinding(binding))
    {
        return ERROR_GEN_FAILURE;
    }

    long status = ERROR_GEN_FAILURE;
    RpcTryExcept
    {
        status = RpcLogout(binding);
    }
    RpcExcept(1)
    {
        status = ERROR_GEN_FAILURE;
    }
    RpcEndExcept;

    FreeRpcBinding(binding);
    return status;
}

DWORD RpcCallGetLicenseInfo(bool& hasLicense, bool& blocked, std::wstring& expirationDate)
{
    handle_t binding = nullptr;
    if (!CreateRpcBinding(binding))
    {
        return ERROR_GEN_FAILURE;
    }

    long has = 0;
    long isBlocked = 0;
    wchar_t* expiration = nullptr;
    long status = ERROR_GEN_FAILURE;

    RpcTryExcept
    {
        status = RpcGetLicenseInfo(binding, &has, &isBlocked, &expiration);
    }
    RpcExcept(1)
    {
        status = ERROR_GEN_FAILURE;
    }
    RpcEndExcept;

    FreeRpcBinding(binding);

    if (expiration)
    {
        expirationDate = expiration;
        MIDL_user_free(expiration);
    }

    hasLicense = has != 0;
    blocked = isBlocked != 0;
    return status;
}

DWORD RpcCallActivate(const std::wstring& activationKey)
{
    handle_t binding = nullptr;
    if (!CreateRpcBinding(binding))
    {
        return ERROR_GEN_FAILURE;
    }

    long status = ERROR_GEN_FAILURE;
    RpcTryExcept
    {
        status = RpcActivate(binding, activationKey.c_str());
    }
    RpcExcept(1)
    {
        status = ERROR_GEN_FAILURE;
    }
    RpcEndExcept;

    FreeRpcBinding(binding);
    return status;
}

DWORD RpcCallGetAvDatabaseInfo(bool& isLoaded, std::wstring& releaseDate, long& recordCount)
{
    handle_t binding = nullptr;
    if (!CreateRpcBinding(binding))
    {
        return ERROR_GEN_FAILURE;
    }

    long loaded = 0;
    wchar_t* date = nullptr;
    long count = 0;
    long status = ERROR_GEN_FAILURE;

    RpcTryExcept
    {
        status = RpcGetAvDatabaseInfo(binding, &loaded, &date, &count);
    }
    RpcExcept(1)
    {
        status = ERROR_GEN_FAILURE;
    }
    RpcEndExcept;

    FreeRpcBinding(binding);

    if (date)
    {
        releaseDate = date;
        MIDL_user_free(date);
    }

    isLoaded = loaded != 0;
    recordCount = count;
    return status;
}

DWORD RpcCallScanPath(const std::wstring& path, bool isFolder, bool& infected, std::wstring& summary)
{
    handle_t binding = nullptr;
    if (!CreateRpcBinding(binding))
    {
        return ERROR_GEN_FAILURE;
    }

    long isInfected = 0;
    wchar_t* text = nullptr;
    long status = ERROR_GEN_FAILURE;

    RpcTryExcept
    {
        status = RpcScanPath(binding, path.c_str(), isFolder ? 1 : 0, &isInfected, &text);
    }
    RpcExcept(1)
    {
        status = ERROR_GEN_FAILURE;
    }
    RpcEndExcept;

    FreeRpcBinding(binding);

    if (text)
    {
        summary = text;
        MIDL_user_free(text);
    }

    infected = isInfected != 0;
    return status;
}

extern "C" void* __RPC_USER MIDL_user_allocate(size_t size)
{
    return malloc(size);
}

extern "C" void __RPC_USER MIDL_user_free(void* p)
{
    free(p);
}

bool ApplyProcessDacl()
{
    std::array<BYTE, SECURITY_MAX_SID_SIZE> systemSidBuffer{};
    std::array<BYTE, SECURITY_MAX_SID_SIZE> adminSidBuffer{};
    std::array<BYTE, SECURITY_MAX_SID_SIZE> usersSidBuffer{};
    DWORD systemSidSize = static_cast<DWORD>(systemSidBuffer.size());
    DWORD adminSidSize = static_cast<DWORD>(adminSidBuffer.size());
    DWORD usersSidSize = static_cast<DWORD>(usersSidBuffer.size());

    if (!CreateWellKnownSid(WinLocalSystemSid, nullptr, systemSidBuffer.data(), &systemSidSize) ||
        !CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, adminSidBuffer.data(), &adminSidSize) ||
        !CreateWellKnownSid(WinBuiltinUsersSid, nullptr, usersSidBuffer.data(), &usersSidSize))
    {
        return false;
    }

    EXPLICIT_ACCESSW entries[5]{};
    DWORD count = 0;

    entries[count].grfAccessPermissions = PROCESS_TERMINATE;
    entries[count].grfAccessMode = DENY_ACCESS;
    entries[count].grfInheritance = NO_INHERITANCE;
    entries[count].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entries[count].Trustee.ptstrName = reinterpret_cast<LPWSTR>(usersSidBuffer.data());
    ++count;

    entries[count].grfAccessPermissions = PROCESS_TERMINATE;
    entries[count].grfAccessMode = DENY_ACCESS;
    entries[count].grfInheritance = NO_INHERITANCE;
    entries[count].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entries[count].Trustee.ptstrName = reinterpret_cast<LPWSTR>(adminSidBuffer.data());
    ++count;

    entries[count].grfAccessPermissions = PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
    entries[count].grfAccessMode = SET_ACCESS;
    entries[count].grfInheritance = NO_INHERITANCE;
    entries[count].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entries[count].Trustee.ptstrName = reinterpret_cast<LPWSTR>(usersSidBuffer.data());
    ++count;

    entries[count].grfAccessPermissions = PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
    entries[count].grfAccessMode = SET_ACCESS;
    entries[count].grfInheritance = NO_INHERITANCE;
    entries[count].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entries[count].Trustee.ptstrName = reinterpret_cast<LPWSTR>(adminSidBuffer.data());
    ++count;

    entries[count].grfAccessPermissions = PROCESS_ALL_ACCESS;
    entries[count].grfAccessMode = SET_ACCESS;
    entries[count].grfInheritance = NO_INHERITANCE;
    entries[count].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entries[count].Trustee.ptstrName = reinterpret_cast<LPWSTR>(systemSidBuffer.data());
    ++count;

    PACL dacl = nullptr;
    if (SetEntriesInAclW(count, entries, nullptr, &dacl) != ERROR_SUCCESS)
    {
        return false;
    }

    const DWORD result = SetSecurityInfo(GetCurrentProcess(), SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, dacl, nullptr);

    if (dacl)
    {
        LocalFree(dacl);
    }

    return result == ERROR_SUCCESS;
}

std::wstring BuildMutexNamePerUser()
{
    wchar_t userName[UNLEN + 1]{};
    DWORD size = UNLEN + 1;
    if (!GetUserNameW(userName, &size))
    {
        return L"Local\\Rbpo_pz_6sem-singleinstance-default";
    }

    return std::wstring(L"Local\\Rbpo_pz_6sem-singleinstance-") + userName;
}

void ShowMainWindow(HWND hWnd)
{
    CenterWindow(hWnd);
    ShowWindow(hWnd, SW_SHOW);
    ShowWindow(hWnd, SW_RESTORE);
    SetForegroundWindow(hWnd);
}

void AddTrayIcon(HWND hWnd)
{
    ZeroMemory(&g_notifyIconData, sizeof(g_notifyIconData));
    g_notifyIconData.cbSize = sizeof(g_notifyIconData);
    g_notifyIconData.hWnd = hWnd;
    g_notifyIconData.uID = ID_TRAY_ICON;
    g_notifyIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_notifyIconData.uCallbackMessage = WM_TRAYICON;
    g_notifyIconData.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_SMALL));
    lstrcpynW(g_notifyIconData.szTip, L"Rbpo_pz_6sem", ARRAYSIZE(g_notifyIconData.szTip));

    g_trayAdded = Shell_NotifyIconW(NIM_ADD, &g_notifyIconData) == TRUE;
    if (g_trayAdded)
    {
        g_notifyIconData.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &g_notifyIconData);
    }
}

void RemoveTrayIcon()
{
    if (g_trayAdded)
    {
        Shell_NotifyIconW(NIM_DELETE, &g_notifyIconData);
        g_trayAdded = false;
    }
}

void ShowTrayContextMenu(HWND hWnd)
{
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
    {
        return;
    }

    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_OPEN, L"Открыть");
    AppendMenuW(hMenu, MF_STRING, IDM_SCAN_FILE, L"Сканировать файл...");
    AppendMenuW(hMenu, MF_STRING, IDM_SCAN_FOLDER, L"Сканировать папку...");
    AppendMenuW(hMenu, MF_STRING, IDM_SCAN_RESULTS, L"Результаты сканирования");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"Выход");

    POINT pt{};
    GetCursorPos(&pt);

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
    PostMessageW(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

bool WaitServiceRunning(SC_HANDLE service)
{
    for (int i = 0; i < 200; ++i)
    {
        SERVICE_STATUS_PROCESS status{};
        DWORD bytesNeeded = 0;
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytesNeeded))
        {
            return false;
        }

        if (status.dwCurrentState == SERVICE_RUNNING)
        {
            return true;
        }

        Sleep(100);
    }

    return false;
}

bool ServiceMustAllowRun()
{
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm)
    {
        return false;
    }

    SC_HANDLE service = OpenServiceW(scm, kServiceName, SERVICE_QUERY_STATUS | SERVICE_START);
    if (!service)
    {
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_STATUS_PROCESS status{};
    DWORD bytesNeeded = 0;
    bool allowRun = false;

    if (QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&status), sizeof(status), &bytesNeeded))
    {
        if (status.dwCurrentState == SERVICE_RUNNING)
        {
            allowRun = true;
        }
        else
        {
            StartServiceW(service, 0, nullptr);
            WaitServiceRunning(service);
            allowRun = false;
        }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return allowRun;
}

DWORD GetParentPid(DWORD pid)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    DWORD parent = 0;
    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (pe.th32ProcessID == pid)
            {
                parent = pe.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }

    CloseHandle(snap);
    return parent;
}

std::wstring GetProcessPath(DWORD pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
    {
        return L"";
    }

    wchar_t path[MAX_PATH]{};
    DWORD size = MAX_PATH;
    std::wstring result;

    if (QueryFullProcessImageNameW(process, 0, path, &size))
    {
        result = path;
    }

    CloseHandle(process);
    return result;
}

bool IsParentService()
{
    DWORD parent = GetParentPid(GetCurrentProcessId());
    if (parent == 0)
    {
        return false;
    }

    std::wstring path = GetProcessPath(parent);
    if (path.empty())
    {
        return false;
    }

    size_t p = path.find_last_of(L"\\/");
    std::wstring fileName = (p == std::wstring::npos) ? path : path.substr(p + 1);

    return _wcsicmp(fileName.c_str(), kServiceExeName) == 0;
}

bool StopServiceByRpc()
{
    RPC_WSTR bindingText = nullptr;
    handle_t binding = nullptr;

    RPC_STATUS s = RpcStringBindingComposeW(
        nullptr,
        (RPC_WSTR)L"ncalrpc",
        nullptr,
        (RPC_WSTR)kRpcEndpoint,
        nullptr,
        &bindingText);

    if (s != RPC_S_OK)
    {
        return false;
    }

    s = RpcBindingFromStringBindingW(bindingText, &binding);
    RpcStringFreeW(&bindingText);
    if (s != RPC_S_OK)
    {
        return false;
    }

    bool ok = true;

    RpcTryExcept
    {
        RpcRequestStop(binding);
    }
    RpcExcept(1)
    {
        ok = false;
    }
    RpcEndExcept;

    RpcBindingFree(&binding);

    return ok;
}

struct LoginDialogState
{
    std::wstring username;
    std::wstring password;
    bool accepted = false;
};

INT_PTR CALLBACK LoginDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        auto* state = reinterpret_cast<LoginDialogState*>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
        CenterWindow(hDlg);
        SetDlgItemTextW(hDlg, IDC_LOGIN_USERNAME, L"admin");
        SetDlgItemTextW(hDlg, IDC_LOGIN_PASSWORD, L"admin12345");
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            auto* state = reinterpret_cast<LoginDialogState*>(GetWindowLongPtr(hDlg, DWLP_USER));
            if (!state)
            {
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;
            }

            wchar_t username[128]{};
            wchar_t password[128]{};
            GetDlgItemTextW(hDlg, IDC_LOGIN_USERNAME, username, ARRAYSIZE(username));
            GetDlgItemTextW(hDlg, IDC_LOGIN_PASSWORD, password, ARRAYSIZE(password));
            state->username = username;
            state->password = password;
            state->accepted = true;
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

struct ActivationDialogState
{
    std::wstring activationKey;
    bool accepted = false;
};

INT_PTR CALLBACK ActivationDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        auto* state = reinterpret_cast<ActivationDialogState*>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
        CenterWindow(hDlg);
        if (state)
        {
            SetDlgItemTextW(hDlg, IDC_ACTIVATION_KEY, state->activationKey.c_str());
        }
        return (INT_PTR)TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            auto* state = reinterpret_cast<ActivationDialogState*>(GetWindowLongPtr(hDlg, DWLP_USER));
            if (!state)
            {
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;
            }

            wchar_t key[128]{};
            GetDlgItemTextW(hDlg, IDC_ACTIVATION_KEY, key, ARRAYSIZE(key));
            state->activationKey = key;
            state->accepted = true;
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

bool ShowLoginDialog(HWND owner, std::wstring& username, std::wstring& password)
{
    LoginDialogState state{};
    DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_LOGIN_DIALOG), owner, LoginDialogProc, reinterpret_cast<LPARAM>(&state));
    if (!state.accepted)
    {
        return false;
    }

    username = state.username;
    password = state.password;
    return true;
}

bool ShowActivationDialog(HWND owner, std::wstring& activationKey)
{
    ActivationDialogState state{};
    state.activationKey = activationKey;
    DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_ACTIVATE_DIALOG), owner, ActivationDialogProc, reinterpret_cast<LPARAM>(&state));
    if (!state.accepted)
    {
        return false;
    }

    activationKey = state.activationKey;
    return true;
}

void UpdateStatusText(const std::wstring& status)
{
    if (g_statusText)
    {
        SetWindowTextW(g_statusText, status.c_str());
    }
}

void UpdateUserText(const std::wstring& username)
{
    if (g_userText)
    {
        SetWindowTextW(g_userText, (L"Пользователь: " + username).c_str());
    }
}

void UpdateLicenseText(const std::wstring& text)
{
    if (g_licenseText)
    {
        SetWindowTextW(g_licenseText, text.c_str());
    }
}

void UpdateExpirationText(const std::wstring& text)
{
    if (g_expirationText)
    {
        SetWindowTextW(g_expirationText, text.c_str());
    }
}

void SetAntivirusControlsEnabled(bool enabled)
{
    g_antivirusEnabled = enabled;
    if (g_scanButton)
    {
        EnableWindow(g_scanButton, enabled ? TRUE : FALSE);
    }
}

void UpdateAvStatusLine(const std::wstring& text)
{
    if (g_avStatusText)
    {
        SetWindowTextW(g_avStatusText, text.c_str());
    }
}

void RefreshAvStatusText()
{
    if (!g_antivirusEnabled)
    {
        UpdateAvStatusLine(L"Антивирусные базы: недоступны (нет лицензии)");
        return;
    }

    bool loaded = false;
    std::wstring releaseDate;
    long recordCount = 0;
    const DWORD status = RpcCallGetAvDatabaseInfo(loaded, releaseDate, recordCount);
    if (status != ERROR_SUCCESS || !loaded)
    {
        UpdateAvStatusLine(L"Антивирусные базы: не загружены");
        return;
    }

    UpdateAvStatusLine(L"Антивирусные базы: " + releaseDate + L", записей: " + std::to_wstring(recordCount));
}

bool RunRpcScan(HWND owner, const std::wstring& path, bool isFolder)
{
    bool infected = false;
    std::wstring summary;
    const DWORD status = RpcCallScanPath(path, isFolder, infected, summary);
    if (status == ERROR_NOT_FOUND)
    {
        MessageBoxW(owner, L"Сканирование недоступно: нет лицензии или базы.", L"Сканирование", MB_OK | MB_ICONWARNING);
        return false;
    }

    if (status != ERROR_SUCCESS)
    {
        MessageBoxW(owner, L"Ошибка RPC при сканировании.", L"Сканирование", MB_OK | MB_ICONERROR);
        return false;
    }

    g_lastScanSummary = summary;
    MessageBoxW(owner, summary.c_str(), infected ? L"Сканирование: угроза" : L"Сканирование", infected ? MB_OK | MB_ICONWARNING : MB_OK | MB_ICONINFORMATION);
    return true;
}

void ShowScanResults(HWND owner)
{
    if (g_lastScanSummary.empty())
    {
        MessageBoxW(owner, L"Результаты сканирования отсутствуют.", L"Результаты сканирования", MB_OK | MB_ICONINFORMATION);
        return;
    }

    MessageBoxW(owner, g_lastScanSummary.c_str(), L"Результаты сканирования", MB_OK | MB_ICONWARNING);
}

void ScanSelectedFile(HWND owner)
{
    if (!g_antivirusEnabled)
    {
        MessageBoxW(owner, L"Антивирус заблокирован.", L"Сканирование", MB_OK | MB_ICONWARNING);
        return;
    }

    wchar_t fileName[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    if (!GetOpenFileNameW(&ofn))
    {
        return;
    }

    RunRpcScan(owner, fileName, false);
}

void ScanSelectedFolder(HWND owner)
{
    if (!g_antivirusEnabled)
    {
        MessageBoxW(owner, L"Антивирус заблокирован.", L"Сканирование", MB_OK | MB_ICONWARNING);
        return;
    }

    BROWSEINFOW bi{};
    bi.hwndOwner = owner;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpszTitle = L"Выберите папку для сканирования";
    PIDLIST_ABSOLUTE folder = SHBrowseForFolderW(&bi);
    if (!folder)
    {
        return;
    }

    wchar_t path[MAX_PATH]{};
    if (!SHGetPathFromIDListW(folder, path))
    {
        CoTaskMemFree(folder);
        return;
    }

    CoTaskMemFree(folder);
    RunRpcScan(owner, path, true);
}

void ScanAllFixedDrives(HWND owner)
{
    if (!g_antivirusEnabled)
    {
        MessageBoxW(owner, L"Антивирус заблокирован.", L"Сканирование", MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring report = L"Сканирование несъёмных дисков:\r\n";
    const DWORD drives = GetLogicalDrives();
    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter)
    {
        if ((drives & (1u << (letter - L'A'))) == 0)
        {
            continue;
        }

        wchar_t root[] = { letter, L':', L'\\', L'\0' };
        if (GetDriveTypeW(root) != DRIVE_FIXED)
        {
            continue;
        }

        bool infected = false;
        std::wstring summary;
        if (RpcCallScanPath(root, true, infected, summary) == ERROR_SUCCESS)
        {
            report += root;
            report += L" -> ";
            report += summary;
            report += L"\r\n";
        }
    }

    g_lastScanSummary = report;
    MessageBoxW(owner, report.c_str(), L"Сканирование дисков", MB_OK | MB_ICONINFORMATION);
}

struct ScheduleDialogState
{
    unsigned int minutes = 60;
    bool accepted = false;
};

INT_PTR CALLBACK ScheduleDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        auto* state = reinterpret_cast<ScheduleDialogState*>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(state));
        if (state)
        {
            SetDlgItemInt(hDlg, IDC_SCHEDULE_MINUTES, state->minutes, FALSE);
            SetDlgItemTextW(hDlg, IDC_SCHEDULE_STATUS, g_scheduleEnabled ? L"включено" : L"выключено");
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            auto* state = reinterpret_cast<ScheduleDialogState*>(GetWindowLongPtr(hDlg, DWLP_USER));
            if (state)
            {
                BOOL translated = FALSE;
                const UINT value = GetDlgItemInt(hDlg, IDC_SCHEDULE_MINUTES, &translated, FALSE);
                if (translated && value > 0)
                {
                    state->minutes = value;
                    state->accepted = true;
                }
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ConfigureScheduledScan(HWND owner)
{
    ScheduleDialogState state{};
    state.minutes = g_scheduleIntervalMinutes;
    DialogBoxParamW(hInst, MAKEINTRESOURCEW(IDD_SCHEDULE_DIALOG), owner, ScheduleDialogProc, reinterpret_cast<LPARAM>(&state));
    if (!state.accepted)
    {
        return;
    }

    g_scheduleIntervalMinutes = state.minutes;
    SetTimer(owner, kScheduleTimerId, g_scheduleIntervalMinutes * 60 * 1000, nullptr);
    MessageBoxW(owner, L"Расписание сохранено.", L"Расписание", MB_OK | MB_ICONINFORMATION);
}

void ToggleScheduledScan(HWND owner)
{
    g_scheduleEnabled = !g_scheduleEnabled;
    if (g_scheduleEnabled)
    {
        SetTimer(owner, kScheduleTimerId, g_scheduleIntervalMinutes * 60 * 1000, nullptr);
    }
    else
    {
        KillTimer(owner, kScheduleTimerId);
    }

    UpdateStatusText(g_scheduleEnabled ? L"Статус: расписание включено" : L"Статус: расписание выключено");
}

DWORD WINAPI MonitorThreadProc(LPVOID param)
{
    const std::wstring dir = *reinterpret_cast<std::wstring*>(param);
    delete reinterpret_cast<std::wstring*>(param);

    HANDLE change = FindFirstChangeNotificationW(
        dir.c_str(),
        FALSE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_DIR_NAME);
    if (change == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    while (WaitForSingleObject(change, 5000) == WAIT_OBJECT_0)
    {
        bool infected = false;
        std::wstring summary;
        RpcCallScanPath(dir, true, infected, summary);
        FindNextChangeNotification(change);
    }

    FindCloseChangeNotification(change);
    return 0;
}

void AddMonitoringDirectory(HWND owner)
{
    BROWSEINFOW bi{};
    bi.hwndOwner = owner;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpszTitle = L"Выберите папку для мониторинга";
    PIDLIST_ABSOLUTE folder = SHBrowseForFolderW(&bi);
    if (!folder)
    {
        return;
    }

    wchar_t path[MAX_PATH]{};
    if (SHGetPathFromIDListW(folder, path))
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_monitoredDirectories.emplace_back(path);
        CloseHandle(CreateThread(nullptr, 0, MonitorThreadProc, new std::wstring(path), 0, nullptr));
        MessageBoxW(owner, L"Мониторинг папки запущен.", L"Мониторинг", MB_OK | MB_ICONINFORMATION);
    }

    CoTaskMemFree(folder);
}

void ShowMonitoredDirectories(HWND owner)
{
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (g_monitoredDirectories.empty())
    {
        MessageBoxW(owner, L"Папки мониторинга не настроены.", L"Мониторинг", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::wstring text;
    for (const auto& dir : g_monitoredDirectories)
    {
        text += dir + L"\r\n";
    }
    MessageBoxW(owner, text.c_str(), L"Папки мониторинга", MB_OK | MB_ICONINFORMATION);
}

void CenterWindow(HWND hWnd)
{
    RECT rect{};
    if (!GetWindowRect(hWnd, &rect))
    {
        return;
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;

    SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

bool EnsureAuthenticated(HWND hWnd, std::wstring& username)
{
    bool authenticated = false;
    std::wstring currentUser;
    DWORD status = RpcCallGetUserInfo(authenticated, currentUser);
    if (status == ERROR_SUCCESS && authenticated)
    {
        username = currentUser;
        return true;
    }

    UpdateStatusText(L"Статус: требуется вход");
    UpdateUserText(L"-");
    UpdateLicenseText(L"Лицензия: неизвестно");
    UpdateExpirationText(L"Срок действия: -");
    SetAntivirusControlsEnabled(false);
    RefreshAvStatusText();

    while (true)
    {
        std::wstring login;
        std::wstring password;
        if (!ShowLoginDialog(hWnd, login, password))
        {
            return false;
        }

        DWORD loginStatus = RpcCallLogin(login, password);
        if (loginStatus == ERROR_SUCCESS)
        {
            username = login;
            return true;
        }

        MessageBoxW(hWnd, L"Ошибка входа. Проверьте логин и пароль.", L"Вход", MB_OK | MB_ICONERROR);
    }
}

bool EnsureLicense(HWND hWnd, const std::wstring& username)
{
    UpdateUserText(username);
    UpdateStatusText(L"Статус: проверка лицензии");

    while (true)
    {
        bool hasLicense = false;
        bool blocked = false;
        std::wstring expiration;
        DWORD status = RpcCallGetLicenseInfo(hasLicense, blocked, expiration);
        if (status == ERROR_SUCCESS && hasLicense && !blocked)
        {
            UpdateLicenseText(L"Лицензия: активна");
            UpdateExpirationText(L"Срок действия: " + expiration);
            UpdateStatusText(L"Антивирус активен");
            SetAntivirusControlsEnabled(true);
            RefreshAvStatusText();
            return true;
        }

        if (status == ERROR_SUCCESS && hasLicense && blocked)
        {
            UpdateLicenseText(L"Лицензия: заблокирована");
        }
        else
        {
            UpdateLicenseText(L"Лицензия: отсутствует");
        }

        UpdateExpirationText(L"Срок действия: -");
        UpdateStatusText(L"Антивирус заблокирован");
        SetAntivirusControlsEnabled(false);
        RefreshAvStatusText();

        std::wstring activationKey;
        if (!ShowActivationDialog(hWnd, activationKey))
        {
            RefreshAvStatusText();
            return false;
        }

        DWORD activateStatus = RpcCallActivate(activationKey);
        if (activateStatus == ERROR_SUCCESS)
        {
            continue;
        }

        MessageBoxW(hWnd, L"Ошибка активации. Нужен действительный activationKey из сервера.", L"Активация", MB_OK | MB_ICONERROR);
    }
}

void RefreshLicenseDisplayOnly(HWND hWnd)
{
    bool authenticated = false;
    std::wstring username;
    DWORD userStatus = RpcCallGetUserInfo(authenticated, username);
    if (userStatus != ERROR_SUCCESS || !authenticated)
    {
        SetAntivirusControlsEnabled(false);
        RefreshAvStatusText();
        return;
    }

    UpdateUserText(username);

    bool hasLicense = false;
    bool blocked = false;
    std::wstring expiration;
    DWORD status = RpcCallGetLicenseInfo(hasLicense, blocked, expiration);
    if (status == ERROR_SUCCESS && hasLicense && !blocked)
    {
        UpdateLicenseText(L"Лицензия: активна");
        UpdateExpirationText(L"Срок действия: " + expiration);
        UpdateStatusText(L"Антивирус активен");
        SetAntivirusControlsEnabled(true);
        RefreshAvStatusText();
        return;
    }

    if (status == ERROR_SUCCESS && hasLicense && blocked)
    {
        UpdateLicenseText(L"Лицензия: заблокирована");
    }
    else
    {
        UpdateLicenseText(L"Лицензия: отсутствует");
    }

    UpdateExpirationText(L"Срок действия: -");
    UpdateStatusText(L"Антивирус заблокирован");
    SetAntivirusControlsEnabled(false);
    RefreshAvStatusText();
}

void RefreshLicenseStatus(HWND hWnd)
{
    if (g_uiFlowRunning)
    {
        return;
    }

    g_uiFlowRunning = true;

    std::wstring username;
    if (!EnsureAuthenticated(hWnd, username))
    {
        g_uiFlowRunning = false;
        return;
    }

    EnsureLicense(hWnd, username);
    g_uiFlowRunning = false;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    const bool debugRun = IsDebuggerPresent() != FALSE;

    LogMessage(L"TrayApp starting");
    if (lpCmdLine)
    {
        LogMessage(std::wstring(L"Command line: ") + lpCmdLine);
    }
    LogMessage(std::wstring(L"Debug run: ") + (debugRun ? L"true" : L"false"));

    if (!debugRun)
    {
        if (!IsParentService())
        {
            if (!ServiceMustAllowRun())
            {
                LogMessage(L"Service check failed, exiting");
                return FALSE;
            }

            LogMessage(L"Parent process is not service, exiting");
            return FALSE;
        }
    }

    ApplyProcessDacl();

    const std::wstring mutexName = BuildMutexNamePerUser();
    g_singleInstanceMutex = CreateMutexW(nullptr, FALSE, mutexName.c_str());
    if (!g_singleInstanceMutex || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        LogMessage(L"Single instance check failed, exiting");
        if (g_singleInstanceMutex)
        {
            CloseHandle(g_singleInstanceMutex);
            g_singleInstanceMutex = nullptr;
        }
        return FALSE;
    }

    LogMessage(L"Instance mutex created");

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_RBPOPZ6SEM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    const bool startHidden = (lpCmdLine != nullptr) &&
        (wcsstr(lpCmdLine, L"--hidden") != nullptr || wcsstr(lpCmdLine, L"/hidden") != nullptr);

    if (!InitInstance(hInstance, nCmdShow, startHidden))
    {
        LogMessage(L"InitInstance failed");
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
        return FALSE;
    }

    LogMessage(L"InitInstance ok, entering message loop");

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RBPOPZ6SEM));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (g_singleInstanceMutex)
    {
        CloseHandle(g_singleInstanceMutex);
        g_singleInstanceMutex = nullptr;
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RBPOPZ6SEM));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_RBPOPZ6SEM);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, bool startHidden)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 380, 240, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    if (!startHidden)
    {
        CenterWindow(hWnd);
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);
    }

    return TRUE;
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
    case WM_CREATE:
        AddTrayIcon(hWnd);
        g_statusText = CreateWindowW(L"STATIC", L"Статус: -", WS_CHILD | WS_VISIBLE,
            10, 10, 320, 20, hWnd, reinterpret_cast<HMENU>(IDC_STATUS_TEXT), hInst, nullptr);
        g_userText = CreateWindowW(L"STATIC", L"Пользователь: -", WS_CHILD | WS_VISIBLE,
            10, 35, 320, 20, hWnd, reinterpret_cast<HMENU>(IDC_USER_TEXT), hInst, nullptr);
        g_licenseText = CreateWindowW(L"STATIC", L"Лицензия: -", WS_CHILD | WS_VISIBLE,
            10, 60, 320, 20, hWnd, reinterpret_cast<HMENU>(IDC_LICENSE_TEXT), hInst, nullptr);
        g_expirationText = CreateWindowW(L"STATIC", L"Срок действия: -", WS_CHILD | WS_VISIBLE,
            10, 85, 340, 20, hWnd, reinterpret_cast<HMENU>(IDC_EXPIRATION_TEXT), hInst, nullptr);
        g_avStatusText = CreateWindowW(L"STATIC", L"Антивирусные базы: -", WS_CHILD | WS_VISIBLE,
            10, 110, 340, 20, hWnd, reinterpret_cast<HMENU>(IDC_AV_STATUS_TEXT), hInst, nullptr);
        g_scanButton = CreateWindowW(L"BUTTON", L"Сканировать файл", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 140, 160, 28, hWnd, reinterpret_cast<HMENU>(IDC_SCAN_BUTTON), hInst, nullptr);
        if (HMENU hMenu = LoadMenuW(hInst, MAKEINTRESOURCEW(IDC_RBPOPZ6SEM)))
        {
            SetMenu(hWnd, hMenu);
        }
        SetAntivirusControlsEnabled(false);
        SetTimer(hWnd, 1, 30000, nullptr);
        PostMessageW(hWnd, WM_APP_STARTUP, 0, 0);
        break;
    case WM_CLOSE:
        if (!g_isExiting)
        {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
        case IDM_TRAY_EXIT:
            g_isExiting = true;
            StopServiceByRpc();
            DestroyWindow(hWnd);
            break;
        case IDM_LOGOUT:
            RpcCallLogout();
            PostMessageW(hWnd, WM_APP_STARTUP, 0, 0);
            break;
        case IDM_TRAY_OPEN:
            ShowMainWindow(hWnd);
            break;
        case IDC_SCAN_BUTTON:
            ScanSelectedFile(hWnd);
            break;
        case IDM_SCAN_FILE:
            ScanSelectedFile(hWnd);
            break;
        case IDM_SCAN_FOLDER:
            ScanSelectedFolder(hWnd);
            break;
        case IDM_SCAN_RESULTS:
            ShowScanResults(hWnd);
            break;
        case IDM_SCAN_ALL_DRIVES:
            ScanAllFixedDrives(hWnd);
            break;
        case IDM_SCHEDULE_SCAN:
            ToggleScheduledScan(hWnd);
            break;
        case IDM_SCHEDULE_SETTINGS:
            ConfigureScheduledScan(hWnd);
            break;
        case IDM_MONITOR_ADD:
            AddMonitoringDirectory(hWnd);
            break;
        case IDM_MONITOR_LIST:
            ShowMonitoredDirectories(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_TIMER:
        if (wParam == 1)
        {
            RefreshLicenseDisplayOnly(hWnd);
        }
        else if (wParam == kScheduleTimerId && g_scheduleEnabled && g_antivirusEnabled)
        {
            ScanAllFixedDrives(hWnd);
        }
        break;
    case WM_APP_STARTUP:
        RefreshLicenseStatus(hWnd);
        break;
    case WM_TRAYICON:
        switch (LOWORD(lParam))
        {
        case WM_LBUTTONUP:
            ShowMainWindow(hWnd);
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            ShowTrayContextMenu(hWnd);
            break;
        default:
            break;
        }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        UNREFERENCED_PARAMETER(hdc);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        RemoveTrayIcon();
        KillTimer(hWnd, 1);
        KillTimer(hWnd, kScheduleTimerId);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

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
