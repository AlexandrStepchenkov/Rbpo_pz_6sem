#include <windows.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <rpc.h>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <array>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <winhttp.h>
#include <iphlpapi.h>
#include <wincrypt.h>
#include <Aclapi.h>
#include <Accctrl.h>

#pragma warning(disable : 4710 4711 4820 5039 5045)

#include "../AvEngine.h"
#include "../common/AppConfig.h"
#include "../common/AvDatabaseManager.h"
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

struct AuthState
{
    bool isAuthenticated = false;
    std::wstring username;
    std::wstring accessToken;
    std::wstring refreshToken;
    ULONGLONG accessExpires = 0;
    ULONGLONG refreshExpires = 0;
};

struct LicenseState
{
    bool hasTicket = false;
    bool blocked = false;
    std::wstring expirationDate;
    ULONGLONG nextRefresh = 0;
    std::wstring activationKey;
};

AuthState g_authState;
LicenseState g_licenseState;
std::wstring g_deviceMac;
std::wstring g_deviceName;
std::wstring g_productId;
std::wstring g_deviceMacBase;
HANDLE g_workerStopEvent = nullptr;
HANDLE g_workerWakeEvent = nullptr;
HANDLE g_workerThread = nullptr;

void LogMessage(const std::wstring& message);

avstore::AvDatabaseManager g_avManager([](const std::wstring& message)
{
    LogMessage(message);
});
ULONGLONG g_nextAvUpdate = 0;

std::wstring GetSelfDirectory();

bool HasActiveLicenseUnlocked()
{
    return g_licenseState.hasTicket && !g_licenseState.blocked;
}

std::wstring BuildScanSummary(const std::vector<av::ScanFinding>& hits, const std::wstring& footer)
{
    std::wstring summary = footer;
    if (!hits.empty())
    {
        summary += L"\r\n\r\n";
        for (const auto& finding : hits)
        {
            summary += L"Файл: " + finding.matchedPath + L"\r\n";
            summary += L"Тип: " + av::ObjectTypeName(finding.objectType) + L"\r\n";
            summary += L"Смещение: " + std::to_wstring(finding.offset) + L"\r\n";
            summary += L"Сигнатура: " + finding.matchedSignature + L"\r\n\r\n";
        }
    }
    return summary;
}

std::wstring GetApiHost()
{
    wchar_t buf[256]{};
    DWORD len = GetEnvironmentVariableW(L"RBPO_API_HOST", buf, ARRAYSIZE(buf));
    if (len > 0 && len < ARRAYSIZE(buf))
    {
        return std::wstring(buf, len);
    }
    return kDefaultApiHost;
}

unsigned short GetApiPort()
{
    wchar_t buf[32]{};
    DWORD len = GetEnvironmentVariableW(L"RBPO_API_PORT", buf, ARRAYSIZE(buf));
    if (len > 0 && len < ARRAYSIZE(buf))
    {
        return static_cast<unsigned short>(_wtoi(buf));
    }
    return kDefaultApiPort;
}

std::wstring GetLogDirectory()
{
    wchar_t programData[MAX_PATH]{};
    DWORD len = GetEnvironmentVariableW(L"ProgramData", programData, MAX_PATH);
    std::wstring base = len > 0 ? std::wstring(programData, len) : GetSelfDirectory();
    std::wstring dir = base + L"\\RbpoPz";
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

std::string WideToUtf8(const std::wstring& value)
{
    if (value.empty())
    {
        return {};
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
    {
        return {};
    }

    std::string result(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWide(const std::string& value)
{
    if (value.empty())
    {
        return {};
    }

    int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 0)
    {
        return {};
    }

    std::wstring result(static_cast<size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    return result;
}

std::string JsonEscape(const std::string& value)
{
    std::string out;
    out.reserve(value.size());
    for (char c : value)
    {
        switch (c)
        {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

bool ExtractJsonString(const std::string& json, const std::string& key, std::string& value)
{
    const std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos)
    {
        return false;
    }

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos)
    {
        return false;
    }

    pos = json.find('"', pos);
    if (pos == std::string::npos)
    {
        return false;
    }

    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos)
    {
        return false;
    }

    value = json.substr(pos + 1, end - pos - 1);
    return true;
}

bool ExtractJsonNumber(const std::string& json, const std::string& key, long long& value)
{
    const std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos)
    {
        return false;
    }

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos)
    {
        return false;
    }

    size_t start = json.find_first_of("-0123456789", pos + 1);
    if (start == std::string::npos)
    {
        return false;
    }

    size_t end = json.find_first_not_of("0123456789", start);
    value = std::stoll(json.substr(start, end - start));
    return true;
}

bool ExtractJsonObject(const std::string& json, const std::string& key, std::string& object)
{
    const std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos)
    {
        return false;
    }

    pos = json.find('{', pos);
    if (pos == std::string::npos)
    {
        return false;
    }

    size_t depth = 0;
    const size_t start = pos;
    for (size_t i = pos; i < json.size(); ++i)
    {
        if (json[i] == '{')
        {
            ++depth;
        }
        else if (json[i] == '}')
        {
            --depth;
            if (depth == 0)
            {
                object = json.substr(start, i - start + 1);
                return true;
            }
        }
    }

    return false;
}

bool ExtractJsonBoolean(const std::string& json, const std::string& key, bool& value)
{
    const std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos)
    {
        return false;
    }

    pos = json.find(':', pos + pattern.size());
    if (pos == std::string::npos)
    {
        return false;
    }

    size_t start = json.find_first_not_of(" \t\r\n", pos + 1);
    if (start == std::string::npos)
    {
        return false;
    }

    if (json.compare(start, 4, "true") == 0)
    {
        value = true;
        return true;
    }

    if (json.compare(start, 5, "false") == 0)
    {
        value = false;
        return true;
    }

    return false;
}

std::string Base64UrlDecode(const std::string& input)
{
    std::string base64 = input;
    std::replace(base64.begin(), base64.end(), '-', '+');
    std::replace(base64.begin(), base64.end(), '_', '/');
    while (base64.size() % 4 != 0)
    {
        base64.push_back('=');
    }

    DWORD required = 0;
    if (!CryptStringToBinaryA(base64.c_str(), static_cast<DWORD>(base64.size()), CRYPT_STRING_BASE64, nullptr, &required, nullptr, nullptr))
    {
        return {};
    }

    std::string output(required, '\0');
    if (!CryptStringToBinaryA(base64.c_str(), static_cast<DWORD>(base64.size()), CRYPT_STRING_BASE64, reinterpret_cast<BYTE*>(output.data()), &required, nullptr, nullptr))
    {
        return {};
    }

    output.resize(required);
    return output;
}

ULONGLONG GetNowFileTime()
{
    FILETIME ft{};
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER uli{};
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    return uli.QuadPart;
}

ULONGLONG UnixSecondsToFileTime(unsigned long long seconds)
{
    const unsigned long long epochDiff = 11644473600ULL;
    unsigned long long ft = (seconds + epochDiff) * 10000000ULL;
    return ft;
}

bool ParseJwtExp(const std::string& token, ULONGLONG& expiresAt, std::string& subject)
{
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot == std::string::npos ? firstDot : firstDot + 1);
    if (firstDot == std::string::npos || secondDot == std::string::npos)
    {
        return false;
    }

    std::string payload = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string decoded = Base64UrlDecode(payload);
    if (decoded.empty())
    {
        return false;
    }

    long long exp = 0;
    if (!ExtractJsonNumber(decoded, "exp", exp))
    {
        return false;
    }

    std::string sub;
    ExtractJsonString(decoded, "sub", sub);
    subject = sub;
    expiresAt = UnixSecondsToFileTime(static_cast<unsigned long long>(exp));
    return true;
}

struct HttpResponse
{
    DWORD status = 0;
    std::string body;
};

std::wstring QuoteForCmd(const std::wstring& value)
{
    std::wstring escaped = L"\"";
    for (wchar_t c : value)
    {
        if (c == L'\"')
        {
            escaped += L"\\\"";
        }
        else
        {
            escaped.push_back(c);
        }
    }
    escaped += L"\"";
    return escaped;
}

bool RunProcessCaptureOutput(const std::wstring& command, std::string& output)
{
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
    {
        return false;
    }

    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = L"cmd.exe /C " + command;
    BOOL created = CreateProcessW(
        nullptr,
        cmdLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi);

    CloseHandle(writePipe);

    if (!created)
    {
        CloseHandle(readPipe);
        return false;
    }

    std::string buffer;
    char chunk[4096]{};
    DWORD read = 0;
    while (ReadFile(readPipe, chunk, sizeof(chunk), &read, nullptr) && read > 0)
    {
        buffer.append(chunk, chunk + read);
    }

    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(readPipe);

    output = buffer;
    return true;
}

bool SendHttpsRequestCurlFallback(const std::wstring& method, const std::wstring& path, const std::string& body, const std::wstring& accessToken, HttpResponse& response)
{
    wchar_t tempDir[MAX_PATH]{};
    DWORD len = GetTempPathW(MAX_PATH, tempDir);
    if (len == 0 || len >= MAX_PATH)
    {
        return false;
    }

    wchar_t tempFile[MAX_PATH]{};
    if (GetTempFileNameW(tempDir, L"zpv", 0, tempFile) == 0)
    {
        return false;
    }

    HANDLE file = CreateFileW(tempFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        DeleteFileW(tempFile);
        return false;
    }

    DWORD written = 0;
    WriteFile(file, body.data(), static_cast<DWORD>(body.size()), &written, nullptr);
    CloseHandle(file);

    const std::wstring host = GetApiHost();
    const unsigned short port = GetApiPort();
    std::wstring url = L"https://" + host + L":" + std::to_wstring(port) + path;
    std::wstring command = L"curl.exe -k -s -X " + method +
        L" " + QuoteForCmd(url) +
        L" -H \"Content-Type: application/json\"";

    if (!accessToken.empty())
    {
        command += L" -H " + QuoteForCmd(L"Authorization: Bearer " + accessToken);
    }

    command += L" --data-binary @" + QuoteForCmd(tempFile) + L" -w \"\\n%{http_code}\"";

    std::string output;
    bool ok = RunProcessCaptureOutput(command, output);
    DeleteFileW(tempFile);
    if (!ok || output.empty())
    {
        return false;
    }

    size_t pos = output.find_last_of('\n');
    if (pos == std::string::npos)
    {
        return false;
    }

    std::string statusText = output.substr(pos + 1);
    while (!statusText.empty() && (statusText.back() == '\r' || statusText.back() == '\n' || statusText.back() == ' ' || statusText.back() == '\t'))
    {
        statusText.pop_back();
    }

    response.body = output.substr(0, pos);
    response.status = static_cast<DWORD>(atoi(statusText.c_str()));
    LogMessage(L"curl fallback " + method + L" " + path + L" -> status=" + std::to_wstring(response.status));
    return response.status != 0;
}

bool SendHttpsRequest(const std::wstring& method, const std::wstring& path, const std::string& body, const std::wstring& accessToken, HttpResponse& response)
{
    HINTERNET hSession = WinHttpOpen(L"RbpoPzService/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
    {
        LogWin32Error(L"WinHttpOpen failed");
        return false;
    }

    DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols, sizeof(secureProtocols));

    const std::wstring apiHost = GetApiHost();
    HINTERNET hConnect = WinHttpConnect(hSession, apiHost.c_str(), GetApiPort(), 0);
    if (!hConnect)
    {
        LogWin32Error(L"WinHttpConnect failed");
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hRequest)
    {
        LogWin32Error(L"WinHttpOpenRequest failed");
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
#ifdef SECURITY_FLAG_IGNORE_WEAK_SIGNATURE
    flags |= SECURITY_FLAG_IGNORE_WEAK_SIGNATURE;
#endif
    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));

    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!accessToken.empty())
    {
        headers += L"Authorization: Bearer " + accessToken + L"\r\n";
    }

    auto sendRequest = [&]() -> BOOL
    {
        return WinHttpSendRequest(
            hRequest,
            headers.c_str(),
            static_cast<DWORD>(headers.size()),
            body.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(body.data()),
            static_cast<DWORD>(body.size()),
            static_cast<DWORD>(body.size()),
            0);
    };

    BOOL sent = sendRequest();
    if (!sent)
    {
        DWORD error = GetLastError();
        if (error == ERROR_WINHTTP_SECURE_FAILURE)
        {
            LogMessage(L"WinHttpSendRequest secure failure, retry with ignored cert checks");
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
            sent = sendRequest();
            if (!sent)
            {
                DWORD retryError = GetLastError();
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);

                if (retryError == ERROR_WINHTTP_SECURE_FAILURE)
                {
                    if (SendHttpsRequestCurlFallback(method, path, body, accessToken, response))
                    {
                        return true;
                    }
                }

                LogWin32Error(L"WinHttpSendRequest failed", retryError);
                return false;
            }
        }
        else
        {
            LogWin32Error(L"WinHttpSendRequest failed", error);
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        LogWin32Error(L"WinHttpReceiveResponse failed");
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX);
    response.status = status;

    std::string responseBody;
    for (;;)
    {
        DWORD size = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &size))
        {
            LogWin32Error(L"WinHttpQueryDataAvailable failed");
            break;
        }

        if (size == 0)
        {
            break;
        }

        std::string buffer(size, '\0');
        DWORD read = 0;
        if (!WinHttpReadData(hRequest, buffer.data(), size, &read))
        {
            LogWin32Error(L"WinHttpReadData failed");
            break;
        }

        buffer.resize(read);
        responseBody += buffer;
    }

    response.body = responseBody;
    LogMessage(L"HTTP " + method + L" " + path + L" -> status=" + std::to_wstring(response.status));
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return true;
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

std::wstring FormatMacAddress(const BYTE* address, ULONG length)
{
    if (!address || length == 0)
    {
        return L"";
    }

    std::wstringstream ss;
    for (ULONG i = 0; i < length; ++i)
    {
        if (i > 0)
        {
            ss << L":";
        }
        ss << std::hex << std::uppercase;
        ss.width(2);
        ss.fill(L'0');
        ss << static_cast<int>(address[i]);
    }
    return ss.str();
}

std::wstring GetPrimaryMacAddress()
{
    ULONG size = 0;
    if (GetAdaptersInfo(nullptr, &size) != ERROR_BUFFER_OVERFLOW || size == 0)
    {
        return L"";
    }

    std::vector<BYTE> buffer(size);
    auto* adapters = reinterpret_cast<IP_ADAPTER_INFO*>(buffer.data());
    if (GetAdaptersInfo(adapters, &size) != ERROR_SUCCESS)
    {
        return L"";
    }

    for (IP_ADAPTER_INFO* adapter = adapters; adapter != nullptr; adapter = adapter->Next)
    {
        if (adapter->Type == MIB_IF_TYPE_LOOPBACK || adapter->AddressLength == 0)
        {
            continue;
        }

        std::wstring mac = FormatMacAddress(adapter->Address, adapter->AddressLength);
        if (!mac.empty())
        {
            return mac;
        }
    }

    return L"";
}

void EnsureDeviceInfo()
{
    if (g_deviceName.empty())
    {
        wchar_t name[MAX_COMPUTERNAME_LENGTH + 1]{};
        DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
        if (GetComputerNameW(name, &size))
        {
            g_deviceName = name;
        }
    }

    if (g_deviceMacBase.empty())
    {
        g_deviceMacBase = GetPrimaryMacAddress();
    }

    if (g_deviceMac.empty())
    {
        g_deviceMac = g_deviceMacBase;
    }
}

void UpdateDeviceIdentityForUser(const std::wstring& username)
{
    EnsureDeviceInfo();
    if (username.empty())
    {
        g_deviceMac = g_deviceMacBase;
        return;
    }

    g_deviceMac = g_deviceMacBase + L"-" + username;
}

void ClearAuthState()
{
    g_authState = AuthState{};
}

void ClearLicenseState()
{
    g_licenseState = LicenseState{};
}

bool UpdateAuthStateFromResponse(const std::string& body, const std::wstring& usernameOverride)
{
    std::string accessToken;
    std::string refreshToken;
    if (!ExtractJsonString(body, "accessToken", accessToken) || !ExtractJsonString(body, "refreshToken", refreshToken))
    {
        return false;
    }

    ULONGLONG accessExp = 0;
    ULONGLONG refreshExp = 0;
    std::string sub;
    if (!ParseJwtExp(accessToken, accessExp, sub))
    {
        accessExp = GetNowFileTime() + 15ULL * 60ULL * 10000000ULL;
    }

    std::string refreshSub;
    if (!ParseJwtExp(refreshToken, refreshExp, refreshSub))
    {
        refreshExp = GetNowFileTime() + 30ULL * 24ULL * 60ULL * 60ULL * 10000000ULL;
    }

    std::wstring username = usernameOverride;
    if (username.empty() && !sub.empty())
    {
        username = Utf8ToWide(sub);
    }

    EnterCriticalSection(&g_lock);
    g_authState.isAuthenticated = true;
    g_authState.username = username;
    g_authState.accessToken = Utf8ToWide(accessToken);
    g_authState.refreshToken = Utf8ToWide(refreshToken);
    g_authState.accessExpires = accessExp;
    g_authState.refreshExpires = refreshExp;
    LeaveCriticalSection(&g_lock);
    return true;
}

bool UpdateLicenseStateFromResponse(const std::string& body, const std::wstring& activationKey)
{
    std::string ticketBody = body;
    std::string nestedTicket;
    if (ExtractJsonObject(body, "ticket", nestedTicket))
    {
        ticketBody = nestedTicket;
    }

    long long lifetimeSeconds = 0;
    std::string expirationDate;
    bool blocked = false;

    if (!ExtractJsonNumber(ticketBody, "ticketLifetimeSeconds", lifetimeSeconds) ||
        !ExtractJsonString(ticketBody, "expirationDate", expirationDate))
    {
        return false;
    }

    ExtractJsonBoolean(ticketBody, "blocked", blocked);
    ULONGLONG now = GetNowFileTime();
    ULONGLONG refreshDelay = static_cast<ULONGLONG>(lifetimeSeconds) * 10000000ULL;
    ULONGLONG nextRefresh = now + (refreshDelay * 8 / 10);

    EnterCriticalSection(&g_lock);
    g_licenseState.hasTicket = true;
    g_licenseState.blocked = blocked;
    g_licenseState.expirationDate = Utf8ToWide(expirationDate);
    g_licenseState.nextRefresh = nextRefresh;
    if (!activationKey.empty())
    {
        g_licenseState.activationKey = activationKey;
    }
    LeaveCriticalSection(&g_lock);
    return true;
}

bool EnsureProductId(const std::wstring& accessToken)
{
    if (!g_productId.empty())
    {
        return true;
    }

    HttpResponse response;
    if (!SendHttpsRequest(L"GET", L"/api/products", "", accessToken, response) || response.status != 200)
    {
        return false;
    }

    std::string id;
    if (ExtractJsonString(response.body, "id", id))
    {
        g_productId = Utf8ToWide(id);
        return true;
    }

    std::string body = "{\"name\":\"Rbpo Antivirus\",\"isBlocked\":false}";
    HttpResponse createResponse;
    if (!SendHttpsRequest(L"POST", L"/api/products", body, accessToken, createResponse) || (createResponse.status != 200 && createResponse.status != 201))
    {
        return false;
    }

    if (ExtractJsonString(createResponse.body, "id", id))
    {
        g_productId = Utf8ToWide(id);
        return true;
    }

    return false;
}

bool CheckLicense(const std::wstring& accessToken)
{
    EnsureDeviceInfo();
    if (!EnsureProductId(accessToken) || g_productId.empty())
    {
        return false;
    }

    std::string body = "{\"deviceMac\":\"" + JsonEscape(WideToUtf8(g_deviceMac)) +
        "\",\"productId\":\"" + JsonEscape(WideToUtf8(g_productId)) + "\"}";

    HttpResponse response;
    if (!SendHttpsRequest(L"POST", L"/api/licenses/check", body, accessToken, response) || response.status != 200)
    {
        return false;
    }

    std::wstring activationKey;
    EnterCriticalSection(&g_lock);
    activationKey = g_licenseState.activationKey;
    LeaveCriticalSection(&g_lock);
    if (!UpdateLicenseStateFromResponse(response.body, activationKey))
    {
        return false;
    }

    return true;
}

bool ActivateLicense(const std::wstring& accessToken, const std::wstring& activationKey)
{
    EnsureDeviceInfo();
    std::string body = "{\"activationKey\":\"" + JsonEscape(WideToUtf8(activationKey)) +
        "\",\"deviceMac\":\"" + JsonEscape(WideToUtf8(g_deviceMac)) +
        "\",\"deviceName\":\"" + JsonEscape(WideToUtf8(g_deviceName)) + "\"}";

    LogMessage(L"ActivateLicense: key=" + activationKey + L" mac=" + g_deviceMac + L" name=" + g_deviceName);
    LogMessage(L"ActivateLicense body: " + Utf8ToWide(body));

    HttpResponse response;
    if (!SendHttpsRequest(L"POST", L"/api/licenses/activate", body, accessToken, response) || response.status != 200)
    {
        LogMessage(L"ActivateLicense failed: status=" + std::to_wstring(response.status));
        if (!response.body.empty())
        {
            LogMessage(L"ActivateLicense response: " + Utf8ToWide(response.body));
        }
        return false;
    }

    if (!UpdateLicenseStateFromResponse(response.body, activationKey))
    {
        LogMessage(L"ActivateLicense: cannot parse response, trying check");
        return CheckLicense(accessToken);
    }

    LogMessage(L"ActivateLicense success");
    return true;
}

bool RenewLicense(const std::wstring& accessToken, const std::wstring& activationKey)
{
    if (activationKey.empty())
    {
        return false;
    }

    std::string body = "{\"activationKey\":\"" + JsonEscape(WideToUtf8(activationKey)) + "\"}";
    HttpResponse response;
    if (!SendHttpsRequest(L"POST", L"/api/licenses/renew", body, accessToken, response) || response.status != 200)
    {
        return false;
    }

    return UpdateLicenseStateFromResponse(response.body, activationKey);
}

bool LoginUser(const std::wstring& username, const std::wstring& password)
{
    std::string body = "{\"username\":\"" + JsonEscape(WideToUtf8(username)) + "\",\"password\":\"" + JsonEscape(WideToUtf8(password)) + "\"}";
    HttpResponse response;
    if (!SendHttpsRequest(L"POST", L"/api/auth/login", body, L"", response) || response.status != 200)
    {
        LogMessage(L"LoginUser failed for username=" + username + L" status=" + std::to_wstring(response.status));
        if (!response.body.empty())
        {
            LogMessage(L"Login response body: " + Utf8ToWide(response.body));
        }
        return false;
    }

    if (!UpdateAuthStateFromResponse(response.body, username))
    {
        LogMessage(L"LoginUser failed: cannot parse tokens from response");
        if (!response.body.empty())
        {
            LogMessage(L"Login parse body: " + Utf8ToWide(response.body));
        }
        return false;
    }
    UpdateDeviceIdentityForUser(username);
    LogMessage(L"LoginUser success for username=" + username);

    EnterCriticalSection(&g_lock);
    ClearLicenseState();
    LeaveCriticalSection(&g_lock);

    EnterCriticalSection(&g_lock);
    std::wstring accessToken = g_authState.accessToken;
    LeaveCriticalSection(&g_lock);

    if (!accessToken.empty())
    {
        CheckLicense(accessToken);
    }

    return true;
}

bool RefreshTokens()
{
    std::wstring refreshToken;
    EnterCriticalSection(&g_lock);
    refreshToken = g_authState.refreshToken;
    LeaveCriticalSection(&g_lock);

    if (refreshToken.empty())
    {
        return false;
    }

    std::string body = "{\"refreshToken\":\"" + JsonEscape(WideToUtf8(refreshToken)) + "\"}";
    HttpResponse response;
    if (!SendHttpsRequest(L"POST", L"/api/auth/refresh", body, L"", response) || response.status != 200)
    {
        return false;
    }

    EnterCriticalSection(&g_lock);
    std::wstring username = g_authState.username;
    LeaveCriticalSection(&g_lock);

    return UpdateAuthStateFromResponse(response.body, username);
}

void LogoutUser()
{
    EnterCriticalSection(&g_lock);
    ClearAuthState();
    ClearLicenseState();
    LeaveCriticalSection(&g_lock);
    UpdateDeviceIdentityForUser(L"");
}

bool HexToBytes(const std::string& hex, std::vector<BYTE>& bytes)
{
    if (hex.size() % 2 != 0)
    {
        return false;
    }

    auto nibble = [](char c) -> int
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }
        return -1;
    };

    bytes.clear();
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2)
    {
        const int hi = nibble(hex[i]);
        const int lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0)
        {
            return false;
        }
        bytes.push_back(static_cast<BYTE>((hi << 4) | lo));
    }
    return true;
}

bool Base64ToBytes(const std::string& base64, std::vector<BYTE>& bytes)
{
    DWORD required = 0;
    if (!CryptStringToBinaryA(base64.c_str(), static_cast<DWORD>(base64.size()), CRYPT_STRING_BASE64, nullptr, &required, nullptr, nullptr))
    {
        return false;
    }

    bytes.assign(required, 0);
    if (!CryptStringToBinaryA(base64.c_str(), static_cast<DWORD>(base64.size()), CRYPT_STRING_BASE64, bytes.data(), &required, nullptr, nullptr))
    {
        return false;
    }

    bytes.resize(required);
    return true;
}

std::string BytesToHex(const std::vector<BYTE>& bytes)
{
    static const char* hex = "0123456789ABCDEF";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (BYTE b : bytes)
    {
        result.push_back(hex[(b >> 4) & 0xF]);
        result.push_back(hex[b & 0xF]);
    }
    return result;
}

bool ParseAvRecordFromJson(const std::string& json, av::AvRecord& record)
{
    long long prefix = 0;
    long long objectSignatureLength = 0;
    long long offsetBegin = 0;
    long long offsetEnd = 0;
    long long objectType = 0;
    std::string objectSignatureHex;
    std::string recordSignatureBase64;

    if (!ExtractJsonNumber(json, "objectSignaturePrefix", prefix) ||
        !ExtractJsonNumber(json, "objectSignatureLength", objectSignatureLength) ||
        !ExtractJsonString(json, "objectSignature", objectSignatureHex) ||
        !ExtractJsonNumber(json, "offsetBegin", offsetBegin) ||
        !ExtractJsonNumber(json, "offsetEnd", offsetEnd) ||
        !ExtractJsonNumber(json, "objectType", objectType))
    {
        return false;
    }

    if (!ExtractJsonString(json, "avRecordSignature", recordSignatureBase64))
    {
        std::string recordSignatureHex;
        if (!ExtractJsonString(json, "avRecordSignatureHex", recordSignatureHex) || !HexToBytes(recordSignatureHex, record.avRecordSignature))
        {
            return false;
        }
    }
    else if (!Base64ToBytes(recordSignatureBase64, record.avRecordSignature))
    {
        return false;
    }

    if (!HexToBytes(objectSignatureHex, record.objectSignature))
    {
        return false;
    }

    record.objectSignaturePrefix = static_cast<std::uint64_t>(prefix);
    record.objectSignatureLength = static_cast<std::uint32_t>(objectSignatureLength);
    record.offsetBegin = static_cast<std::int64_t>(offsetBegin);
    record.offsetEnd = static_cast<std::int64_t>(offsetEnd);
    record.objectType = static_cast<av::ObjectType>(static_cast<std::uint8_t>(objectType));
    return true;
}

bool FetchAvRecordFromServer(const av::AvRecord& record, av::AvRecord& repaired)
{
    const std::string body =
        "{\"objectSignaturePrefix\":" + std::to_string(record.objectSignaturePrefix) +
        ",\"objectSignatureLength\":" + std::to_string(record.objectSignatureLength) +
        ",\"objectSignature\":\"" + BytesToHex(record.objectSignature) +
        "\",\"offsetBegin\":" + std::to_string(record.offsetBegin) +
        ",\"offsetEnd\":" + std::to_string(record.offsetEnd) +
        ",\"objectType\":" + std::to_string(static_cast<int>(record.objectType)) + "}";

    HttpResponse response{};
    if (!SendHttpsRequest(L"POST", kAvRecordPath, body, L"", response) || response.status != 200)
    {
        LogMessage(L"AV record fetch failed, status=" + std::to_wstring(response.status));
        return false;
    }

    if (!ParseAvRecordFromJson(response.body, repaired))
    {
        LogMessage(L"AV record fetch: cannot parse response");
        return false;
    }

    return true;
}

bool RepairAvRecord(const av::AvRecord& record, const std::vector<BYTE>&, av::AvRecord& repaired)
{
    return FetchAvRecordFromServer(record, repaired);
}

bool IsNetworkAvailable()
{
    HINTERNET session = WinHttpOpen(L"RbpoPz6SemService/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
    {
        return false;
    }

    const std::wstring host = GetApiHost();
    HINTERNET connect = WinHttpConnect(session, host.c_str(), GetApiPort(), 0);
    const bool available = connect != nullptr;
    if (connect)
    {
        WinHttpCloseHandle(connect);
    }
    WinHttpCloseHandle(session);
    return available;
}

bool DownloadAvDatabase(std::vector<BYTE>& bytes)
{
    HttpResponse response{};
    if (!SendHttpsRequest(L"GET", kAvBasesLatestPath, {}, L"", response) || response.status != 200)
    {
        LogMessage(L"AV update download failed, status=" + std::to_wstring(response.status));
        return false;
    }

    bytes.assign(response.body.begin(), response.body.end());
    return !bytes.empty();
}

bool TryUpdateAvBases(bool forceUpdate)
{
    const ULONGLONG now = GetNowFileTime();
    if (!forceUpdate && now < g_nextAvUpdate)
    {
        return true;
    }

    const bool networkAvailable = IsNetworkAvailable();
    g_avManager.SetNetworkAvailable(networkAvailable);
    if (!networkAvailable)
    {
        LogMessage(L"AV update skipped: network unavailable");
        return false;
    }

    LogMessage(forceUpdate ? L"AV: forced update started" : L"AV: scheduled update started");
    g_avManager.BackupCurrent();

    std::vector<BYTE> downloaded;
    if (!DownloadAvDatabase(downloaded))
    {
        return false;
    }

    if (!g_avManager.SaveDownloadedBytes(downloaded))
    {
        LogMessage(L"AV update failed to save downloaded database");
        g_avManager.RollbackToBackup();
        return false;
    }

    const avstore::LoadReport report = g_avManager.ReloadCurrentFromDisk();
    if (report.status != avstore::LoadStatus::Ok && report.status != avstore::LoadStatus::PartialRecordsLoaded)
    {
        LogMessage(L"AV update reload failed, rolling back");
        g_avManager.RollbackToBackup();
        return false;
    }

    g_nextAvUpdate = now + static_cast<ULONGLONG>(kAvUpdateIntervalHours) * 60ULL * 60ULL * 10000000ULL;
    LogMessage(L"AV update completed, records=" + std::to_wstring(report.recordsLoaded));
    return true;
}

void EnsureBundledAvDataInstalled()
{
    const auto current = avstore::GetCurrentDatabasePath();
    if (std::filesystem::exists(current))
    {
        return;
    }

    if (avstore::InstallBundledDefaultDatabase())
    {
        LogMessage(L"AV: installed bundled default database");
    }
}

void InitializeAvStorage()
{
    g_avManager.SetRecordFetcher(RepairAvRecord);
    g_avManager.SetNetworkAvailable(IsNetworkAvailable());
    EnsureBundledAvDataInstalled();
    const avstore::LoadReport report = g_avManager.LoadStartupDatabase();
    if (report.status == avstore::LoadStatus::ManifestSignatureFailed)
    {
        g_nextAvUpdate = GetNowFileTime();
        LogMessage(L"AV: manifest failure, scheduling forced update");
    }
    else
    {
        g_nextAvUpdate = GetNowFileTime() + static_cast<ULONGLONG>(kAvUpdateIntervalHours) * 60ULL * 60ULL * 10000000ULL;
    }
}

DWORD ComputeNextWakeMs()
{
    ULONGLONG now = GetNowFileTime();
    ULONGLONG next = now + 30ULL * 10000000ULL;
    const ULONGLONG skew = 60ULL * 10000000ULL;

    EnterCriticalSection(&g_lock);
    if (g_authState.isAuthenticated)
    {
        if (g_authState.accessExpires > skew)
        {
            ULONGLONG candidate = g_authState.accessExpires - skew;
            if (candidate < next)
            {
                next = candidate;
            }
        }
        if (g_authState.refreshExpires > skew)
        {
            ULONGLONG candidate = g_authState.refreshExpires - skew;
            if (candidate < next)
            {
                next = candidate;
            }
        }
    }
    if (g_licenseState.hasTicket)
    {
        if (g_licenseState.nextRefresh < next)
        {
            next = g_licenseState.nextRefresh;
        }
    }
    LeaveCriticalSection(&g_lock);

    if (g_nextAvUpdate < next)
    {
        next = g_nextAvUpdate;
    }

    if (next <= now)
    {
        return 0;
    }

    ULONGLONG diff = next - now;
    ULONGLONG ms = diff / 10000ULL;
    if (ms > 300000)
    {
        ms = 300000;
    }
    return static_cast<DWORD>(ms);
}

DWORD WINAPI BackgroundWorkerThread(LPVOID)
{
    HANDLE handles[2] = { g_workerStopEvent, g_workerWakeEvent };

    while (true)
    {
        DWORD timeout = ComputeNextWakeMs();
        DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, timeout);
        if (waitResult == WAIT_OBJECT_0)
        {
            break;
        }

        if (waitResult == WAIT_OBJECT_0 + 1)
        {
            ResetEvent(g_workerWakeEvent);
        }

        std::wstring accessToken;
        std::wstring activationKey;
        ULONGLONG accessExpires = 0;
        ULONGLONG refreshExpires = 0;
        ULONGLONG nextRefresh = 0;
        bool authenticated = false;
        bool hasTicket = false;

        EnterCriticalSection(&g_lock);
        authenticated = g_authState.isAuthenticated;
        accessToken = g_authState.accessToken;
        accessExpires = g_authState.accessExpires;
        refreshExpires = g_authState.refreshExpires;
        activationKey = g_licenseState.activationKey;
        nextRefresh = g_licenseState.nextRefresh;
        hasTicket = g_licenseState.hasTicket;
        LeaveCriticalSection(&g_lock);

        if (!authenticated)
        {
            continue;
        }

        ULONGLONG now = GetNowFileTime();
        const ULONGLONG skew = 60ULL * 10000000ULL;

        if (accessExpires <= now + skew || refreshExpires <= now + skew)
        {
            if (!RefreshTokens())
            {
                LogoutUser();
                continue;
            }
        }

        if (hasTicket && nextRefresh <= now + skew)
        {
            if (!RenewLicense(accessToken, activationKey))
            {
                EnterCriticalSection(&g_lock);
                ClearLicenseState();
                LeaveCriticalSection(&g_lock);
            }
        }

        const bool forceUpdate = g_avManager.NeedsForcedUpdate();
        if (forceUpdate || now >= g_nextAvUpdate)
        {
            g_avManager.SetNetworkAvailable(IsNetworkAvailable());
            if (!TryUpdateAvBases(forceUpdate))
            {
                if (forceUpdate && IsNetworkAvailable())
                {
                    LogMessage(L"AV: forced update failed");
                }
            }
            else if (forceUpdate)
            {
                g_avManager.SetManifestFailurePending(false);
            }
        }
    }

    return 0;
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

    DWORD actualSession = 0;
    if (ProcessIdToSessionId(pi.dwProcessId, &actualSession))
    {
        LogMessage(L"Started TrayApp for session " + std::to_wstring(sessionId) +
            L" pid=" + std::to_wstring(pi.dwProcessId) +
            L" actualSession=" + std::to_wstring(actualSession));
    }
    else
    {
        LogWin32Error(L"ProcessIdToSessionId failed for pid " + std::to_wstring(pi.dwProcessId));
        LogMessage(L"Started TrayApp for session " + std::to_wstring(sessionId) + L" pid=" + std::to_wstring(pi.dwProcessId));
    }

    DWORD waitResult = WaitForSingleObject(pi.hProcess, 2000);
    if (waitResult == WAIT_OBJECT_0)
    {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode))
        {
            LogMessage(L"TrayApp exited quickly. pid=" + std::to_wstring(pi.dwProcessId) + L" exitCode=" + std::to_wstring(exitCode));
        }
        else
        {
            LogWin32Error(L"GetExitCodeProcess failed for pid " + std::to_wstring(pi.dwProcessId));
        }
    }
    else if (waitResult == WAIT_FAILED)
    {
        LogWin32Error(L"WaitForSingleObject failed for pid " + std::to_wstring(pi.dwProcessId));
    }

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

        std::wstring stationName = sessions[i].pWinStationName ? sessions[i].pWinStationName : L"";
        LogMessage(L"Session id=" + std::to_wstring(id) + L" state=" + std::to_wstring(static_cast<int>(sessions[i].State)) + L" station=" + stationName);

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
        (unsigned)-1,
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

    std::wstring title = L"Rbpo PZ Service";
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

long RpcGetUserInfo(handle_t, long* isAuthenticated, wchar_t** username)
{
    if (!isAuthenticated || !username)
    {
        return ERROR_INVALID_PARAMETER;
    }

    std::wstring name;
    bool authenticated = false;

    EnterCriticalSection(&g_lock);
    authenticated = g_authState.isAuthenticated;
    name = g_authState.username;
    LeaveCriticalSection(&g_lock);

    *isAuthenticated = authenticated ? 1 : 0;
    size_t len = name.size();
    *username = reinterpret_cast<wchar_t*>(MIDL_user_allocate((len + 1) * sizeof(wchar_t)));
    if (!*username)
    {
        return ERROR_OUTOFMEMORY;
    }

    if (len > 0)
    {
        wcscpy_s(*username, len + 1, name.c_str());
    }
    else
    {
        (*username)[0] = L'\0';
    }

    return ERROR_SUCCESS;
}

long RpcLogin(handle_t, const wchar_t* username, const wchar_t* password)
{
    if (!username || !password || wcslen(username) == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (!LoginUser(username, password))
    {
        return ERROR_ACCESS_DENIED;
    }

    if (g_workerWakeEvent)
    {
        SetEvent(g_workerWakeEvent);
    }

    return ERROR_SUCCESS;
}

long RpcLogout(handle_t)
{
    LogoutUser();
    if (g_workerWakeEvent)
    {
        SetEvent(g_workerWakeEvent);
    }
    return ERROR_SUCCESS;
}

long RpcGetLicenseInfo(handle_t, long* hasLicense, long* blocked, wchar_t** expirationDate)
{
    if (!hasLicense || !blocked || !expirationDate)
    {
        return ERROR_INVALID_PARAMETER;
    }

    bool hasTicket = false;
    bool isBlocked = false;
    std::wstring expiration;

    EnterCriticalSection(&g_lock);
    hasTicket = g_licenseState.hasTicket;
    isBlocked = g_licenseState.blocked;
    expiration = g_licenseState.expirationDate;
    LeaveCriticalSection(&g_lock);

    *hasLicense = hasTicket ? 1 : 0;
    *blocked = isBlocked ? 1 : 0;

    size_t len = expiration.size();
    *expirationDate = reinterpret_cast<wchar_t*>(MIDL_user_allocate((len + 1) * sizeof(wchar_t)));
    if (!*expirationDate)
    {
        return ERROR_OUTOFMEMORY;
    }

    if (len > 0)
    {
        wcscpy_s(*expirationDate, len + 1, expiration.c_str());
    }
    else
    {
        (*expirationDate)[0] = L'\0';
    }

    return hasTicket ? ERROR_SUCCESS : ERROR_NOT_FOUND;
}

long RpcGetAvDatabaseInfo(handle_t, long* isLoaded, wchar_t** releaseDate, long* recordCount)
{
    if (!isLoaded || !releaseDate || !recordCount)
    {
        return ERROR_INVALID_PARAMETER;
    }

    EnterCriticalSection(&g_lock);
    if (!HasActiveLicenseUnlocked())
    {
        LeaveCriticalSection(&g_lock);
        return ERROR_NOT_FOUND;
    }

    const bool loaded = g_avManager.IsLoaded();
    const av::AvDatabase database = g_avManager.Snapshot();
    const std::size_t count = g_avManager.RecordCount();
    LeaveCriticalSection(&g_lock);

    *isLoaded = loaded ? 1 : 0;
    *recordCount = static_cast<long>(loaded ? count : 0);
    const std::wstring text = loaded ? av::FormatReleaseDate(database.releaseDate) : L"-";

    *releaseDate = reinterpret_cast<wchar_t*>(MIDL_user_allocate((text.size() + 1) * sizeof(wchar_t)));
    if (!*releaseDate)
    {
        return ERROR_OUTOFMEMORY;
    }

    wcscpy_s(*releaseDate, text.size() + 1, text.c_str());
    return ERROR_SUCCESS;
}

long RpcScanPath(handle_t, const wchar_t* path, long isFolder, long* infected, wchar_t** summary)
{
    if (!path || !infected || !summary)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *infected = 0;

    av::AvDatabase db{};
    EnterCriticalSection(&g_lock);
    if (!HasActiveLicenseUnlocked() || !g_avManager.IsLoaded())
    {
        LeaveCriticalSection(&g_lock);
        return ERROR_NOT_FOUND;
    }
    db = g_avManager.Snapshot();
    LeaveCriticalSection(&g_lock);

    std::vector<av::ScanFinding> hits;
    std::wstring text;

    if (isFolder != 0)
    {
        text = av::ScanFolder(path, db, hits);
        *infected = hits.empty() ? 0 : 1;
    }
    else
    {
        av::ScanFinding finding{};
        if (av::ScanFile(path, db, finding) && finding.infected)
        {
            hits.push_back(finding);
            *infected = 1;
            text = L"Файл признан вредоносным.";
        }
        else
        {
            text = L"Вредоносные сигнатуры не найдены.";
        }
    }

    text = BuildScanSummary(hits, text);

    *summary = reinterpret_cast<wchar_t*>(MIDL_user_allocate((text.size() + 1) * sizeof(wchar_t)));
    if (!*summary)
    {
        return ERROR_OUTOFMEMORY;
    }

    wcscpy_s(*summary, text.size() + 1, text.c_str());
    return ERROR_SUCCESS;
}

long RpcActivate(handle_t, const wchar_t* activationKey)
{
    if (!activationKey || wcslen(activationKey) == 0)
    {
        return ERROR_INVALID_PARAMETER;
    }

    std::wstring accessToken;
    EnterCriticalSection(&g_lock);
    accessToken = g_authState.accessToken;
    LeaveCriticalSection(&g_lock);

    if (accessToken.empty())
    {
        return ERROR_ACCESS_DENIED;
    }

    if (!ActivateLicense(accessToken, activationKey))
    {
        return ERROR_INVALID_DATA;
    }

    if (g_workerWakeEvent)
    {
        SetEvent(g_workerWakeEvent);
    }

    return ERROR_SUCCESS;
}

DWORD WINAPI ServiceHandler(DWORD control, DWORD eventType, LPVOID eventData, LPVOID)
{
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

    InitializeAvStorage();

    if (!StartRpc())
    {
        LogWin32Error(L"StartRpc failed");
        SetState(SERVICE_STOPPED, 0);
        DeleteCriticalSection(&g_lock);
        return;
    }

    g_workerStopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_workerWakeEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_workerStopEvent || !g_workerWakeEvent)
    {
        LogWin32Error(L"CreateEvent failed");
    }

    g_workerThread = CreateThread(nullptr, 0, BackgroundWorkerThread, nullptr, 0, nullptr);
    if (!g_workerThread)
    {
        LogWin32Error(L"CreateThread failed");
    }

    StartAppsInCurrentSessions();
    SetState(SERVICE_RUNNING, SERVICE_ACCEPT_SESSIONCHANGE);

    RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);

    LogMessage(L"ServiceMain: stopping");

    RpcServerUnregisterIf(ServiceControl_v1_0_s_ifspec, nullptr, FALSE);
    if (g_workerStopEvent)
    {
        SetEvent(g_workerStopEvent);
    }
    if (g_workerThread)
    {
        WaitForSingleObject(g_workerThread, 5000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }
    if (g_workerWakeEvent)
    {
        CloseHandle(g_workerWakeEvent);
        g_workerWakeEvent = nullptr;
    }
    if (g_workerStopEvent)
    {
        CloseHandle(g_workerStopEvent);
        g_workerStopEvent = nullptr;
    }
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
