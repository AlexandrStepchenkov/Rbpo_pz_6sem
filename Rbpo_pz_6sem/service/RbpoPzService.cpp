#include <windows.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <rpc.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <array>
#include <Aclapi.h>
#include <Accctrl.h>

#include "../common/AppConfig.h"
#include "../rpc/ServiceControl.h"

#if defined(_M_ARM64) || defined(_M_ARM64EC)
extern "C" {
#include "../rpc/ServiceControl_s_arm64.c"
}
#elif defined(_M_IX86)
extern "C" {
#include "../rpc/ServiceControl_s_win32.c"
}
#else
extern "C" {
#include "../rpc/ServiceControl_s_x64.c"
}
#endif

#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "Userenv.lib")
#pragma comment(lib, "Advapi32.lib")

SERVICE_STATUS_HANDLE g_serviceHandle = nullptr;
SERVICE_STATUS g_serviceStatus{};
CRITICAL_SECTION g_lock;
std::vector<PROCESS_INFORMATION> g_apps;

std::wstring GetSelfDirectory();

std::wstring GetLogDirectory()
{
    wchar_t programData[MAX_PATH]{};
    DWORD len = GetEnvironmentVariableW(L"ProgramData", programData, MAX_PATH);
    std::wstring base = len > 0 ? std::wstring(programData, len) : GetSelfDirectory();
    std::wstring dir = base + L"\\RbpoPz6sem";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

std::wstring GetLogPath()
{
    return GetLogDirectory() + L"\\service.log";
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

void LogWin32Error(const std::wstring& context, DWORD error = GetLastError())
{
    wchar_t errorText[32]{};
    swprintf_s(errorText, L"%lu", error);
    LogMessage(context + L" (error=" + errorText + L")");
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

std::wstring GetSelfDirectory()
{
    wchar_t path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring s = path;
    size_t p = s.find_last_of(L"\\/");
    if (p == std::wstring::npos)
    {
        return L".";
    }
    return s.substr(0, p);
}

std::wstring GetTrayAppPath()
{
    return GetSelfDirectory() + L"\\" + kTrayAppExeName;
}

bool IsRunning(HANDLE process)
{
    if (!process)
    {
        return false;
    }

    DWORD code = 0;
    if (!GetExitCodeProcess(process, &code))
    {
        return false;
    }

    return code == STILL_ACTIVE;
}

bool HasAppForSession(DWORD sessionId)
{
    EnterCriticalSection(&g_lock);

    bool found = false;
    for (const auto& pi : g_apps)
    {
        if (!pi.hProcess)
        {
            continue;
        }

        DWORD pidSession = 0;
        if (ProcessIdToSessionId(pi.dwProcessId, &pidSession) && pidSession == sessionId && IsRunning(pi.hProcess))
        {
            found = true;
            break;
        }
    }

    LeaveCriticalSection(&g_lock);
    return found;
}

bool StartAppInSession(DWORD sessionId)
{
    if (sessionId == 0)
    {
        LogMessage(L"StartAppInSession: sessionId=0, skip");
        return false;
    }

    if (HasAppForSession(sessionId))
    {
        LogMessage(L"StartAppInSession: already running for session " + std::to_wstring(sessionId));
        return true;
    }

    HANDLE userToken = nullptr;
    if (!WTSQueryUserToken(sessionId, &userToken))
    {
        LogWin32Error(L"WTSQueryUserToken failed for session " + std::to_wstring(sessionId));
        return false;
    }

    HANDLE primaryToken = nullptr;
    BOOL tokenOk = DuplicateTokenEx(
        userToken,
        TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_SESSIONID,
        nullptr,
        SecurityImpersonation,
        TokenPrimary,
        &primaryToken);
    CloseHandle(userToken);

    if (!tokenOk)
    {
        LogWin32Error(L"DuplicateTokenEx failed for session " + std::to_wstring(sessionId));
        return false;
    }

    LPVOID env = nullptr;
    if (!CreateEnvironmentBlock(&env, primaryToken, FALSE))
    {
        LogWin32Error(L"CreateEnvironmentBlock failed for session " + std::to_wstring(sessionId));
    }

    std::wstring appPath = GetTrayAppPath();
    std::wstring command = L"\"" + appPath + L"\" --hidden";

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.lpDesktop = const_cast<LPWSTR>(L"winsta0\\default");

    PROCESS_INFORMATION pi{};
    BOOL created = CreateProcessAsUserW(
        primaryToken,
        appPath.c_str(),
        command.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        env,
        nullptr,
        &si,
        &pi);

    if (env)
    {
        DestroyEnvironmentBlock(env);
    }

    CloseHandle(primaryToken);

    if (!created)
    {
        LogWin32Error(L"CreateProcessAsUserW failed for session " + std::to_wstring(sessionId));
        return false;
    }

    LogMessage(L"Started tray app for session " + std::to_wstring(sessionId) + L" pid=" + std::to_wstring(pi.dwProcessId));

    CloseHandle(pi.hThread);

    EnterCriticalSection(&g_lock);
    g_apps.push_back(pi);
    LeaveCriticalSection(&g_lock);

    return true;
}

void StartAppsInCurrentSessions()
{
    PWTS_SESSION_INFO sessions = nullptr;
    DWORD count = 0;

    if (!WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessions, &count))
    {
        LogWin32Error(L"WTSEnumerateSessionsW failed");
        return;
    }

    LogMessage(L"Enumerated sessions: " + std::to_wstring(count));

    for (DWORD i = 0; i < count; ++i)
    {
        DWORD id = sessions[i].SessionId;
        if (id == 0)
        {
            continue;
        }

        StartAppInSession(id);
    }

    WTSFreeMemory(sessions);
}

void StopAllApps()
{
    EnterCriticalSection(&g_lock);

    for (auto& pi : g_apps)
    {
        if (pi.hProcess && IsRunning(pi.hProcess))
        {
            TerminateProcess(pi.hProcess, 0);
        }

        if (pi.hProcess)
        {
            CloseHandle(pi.hProcess);
            pi.hProcess = nullptr;
        }
    }

    g_apps.clear();

    LeaveCriticalSection(&g_lock);
}

void SetState(DWORD state, DWORD acceptedControls)
{
    g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_serviceStatus.dwCurrentState = state;
    g_serviceStatus.dwControlsAccepted = acceptedControls;
    g_serviceStatus.dwWin32ExitCode = NO_ERROR;
    g_serviceStatus.dwServiceSpecificExitCode = 0;
    g_serviceStatus.dwCheckPoint = 0;
    g_serviceStatus.dwWaitHint = 0;

    if (g_serviceHandle)
    {
        SetServiceStatus(g_serviceHandle, &g_serviceStatus);
    }
}

bool StartRpc()
{
    RPC_STATUS s = RpcServerUseProtseqEpW(
        (RPC_WSTR)L"ncalrpc",
        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
        (RPC_WSTR)kRpcEndpoint,
        nullptr);

    if (s != RPC_S_OK)
    {
        return false;
    }

    s = RpcServerRegisterIf2(
        ServiceControl_v1_0_s_ifspec,
        nullptr,
        nullptr,
        RPC_IF_ALLOW_LOCAL_ONLY,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
        static_cast<unsigned>(-1),
        nullptr);

    return s == RPC_S_OK;
}

bool ConfirmStopFromActiveSession()
{
    DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId == 0xFFFFFFFF)
    {
        return true;
    }

    std::wstring title = kServiceDisplayName;
    std::wstring message = L"Остановить службу?";
    DWORD response = 0;

    BOOL sent = WTSSendMessageW(
        WTS_CURRENT_SERVER_HANDLE,
        sessionId,
        const_cast<LPWSTR>(title.c_str()),
        static_cast<DWORD>((title.size() + 1) * sizeof(wchar_t)),
        const_cast<LPWSTR>(message.c_str()),
        static_cast<DWORD>((message.size() + 1) * sizeof(wchar_t)),
        MB_YESNO | MB_ICONWARNING | MB_SYSTEMMODAL,
        30,
        &response,
        TRUE);

    if (!sent)
    {
        return true;
    }

    return response == IDYES;
}

void RpcRequestStop(handle_t)
{
    if (!ConfirmStopFromActiveSession())
    {
        LogMessage(L"RpcRequestStop: stop rejected by user");
        return;
    }

    LogMessage(L"RpcRequestStop: stopping service");
    SetState(SERVICE_STOP_PENDING, SERVICE_ACCEPT_SESSIONCHANGE);
    RpcMgmtStopServerListening(nullptr);
}

DWORD WINAPI ServiceHandler(DWORD control, DWORD eventType, LPVOID eventData, LPVOID)
{
    if (control == SERVICE_CONTROL_STOP || control == SERVICE_CONTROL_SHUTDOWN)
    {
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    if (control == SERVICE_CONTROL_SESSIONCHANGE && eventType == WTS_SESSION_LOGON)
    {
        auto* data = reinterpret_cast<WTSSESSION_NOTIFICATION*>(eventData);
        if (data)
        {
            LogMessage(L"Session logon detected: " + std::to_wstring(data->dwSessionId));
            StartAppInSession(data->dwSessionId);
        }
    }

    return NO_ERROR;
}

void WINAPI ServiceMain(DWORD, LPWSTR*)
{
    InitializeCriticalSection(&g_lock);

    LogMessage(L"ServiceMain: starting");

    g_serviceHandle = RegisterServiceCtrlHandlerExW(kServiceName, ServiceHandler, nullptr);
    if (!g_serviceHandle)
    {
        LogWin32Error(L"RegisterServiceCtrlHandlerExW failed");
        DeleteCriticalSection(&g_lock);
        return;
    }

    ApplyProcessDacl();

    SetState(SERVICE_START_PENDING, 0);

    if (!StartRpc())
    {
        LogWin32Error(L"StartRpc failed");
        SetState(SERVICE_STOPPED, 0);
        DeleteCriticalSection(&g_lock);
        return;
    }

    StartAppsInCurrentSessions();
    SetState(SERVICE_RUNNING, SERVICE_ACCEPT_SESSIONCHANGE);

    RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);

    LogMessage(L"ServiceMain: stopping");

    RpcServerUnregisterIf(ServiceControl_v1_0_s_ifspec, nullptr, FALSE);
    StopAllApps();
    SetState(SERVICE_STOPPED, 0);

    DeleteCriticalSection(&g_lock);
}

int wmain()
{
    SERVICE_TABLE_ENTRYW table[] =
    {
        { const_cast<LPWSTR>(kServiceName), ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(table))
    {
        LogWin32Error(L"StartServiceCtrlDispatcherW failed");
        return 1;
    }

    return 0;
}
