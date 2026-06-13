// Simple append-only debug logger (ASCII) for NativeTrainer
#pragma once
#include <windows.h>
#include <string>
#include <cstdarg>

namespace DebugLog {
    inline bool& enabled() { static bool on = true; return on; }
    inline void setEnabled(bool on) { enabled() = on; }

    inline std::wstring logPath() {
        wchar_t tmp[MAX_PATH]{};
        DWORD n = GetTempPathW(MAX_PATH, tmp);
        std::wstring p = (n > 0) ? std::wstring(tmp) : std::wstring(L"C:\\Windows\\Temp\\");
        if (!p.empty() && p.back() != L'\\' && p.back() != L'/') p += L"\\";
        p += L"NativeTrainer.log";
        return p;
    }

    inline std::wstring altLogPath() {
        wchar_t buf[MAX_PATH]{};
        DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
        std::wstring base = (n && n < MAX_PATH) ? std::wstring(buf) : std::wstring();
        if (base.empty()) return L"";
        if (!base.empty() && base.back() != L'\\') base += L"\\";
        base += L"NativeTrainer";
        // Ensure folder exists
        CreateDirectoryW(base.c_str(), nullptr);
        if (!base.empty() && base.back() != L'\\') base += L"\\";
        else base += L"\\"; // ensure trailing backslash
        base += L"NativeTrainer.log";
        return base;
    }

    // User-requested log path: Documents\nativetraner.log
    inline std::wstring docsLogPath() {
        wchar_t up[MAX_PATH]{};
        DWORD n = GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH);
        if (!n || n >= MAX_PATH) return L"";
        std::wstring p(up);
        if (!p.empty() && p.back() != L'\\') p += L"\\";
        p += L"Documents";
        if (!p.empty() && p.back() != L'\\') p += L"\\";
        else p += L"\\";
        p += L"nativetraner.log"; // spelling per user request
        return p;
    }

    inline void writeOne(const std::wstring& path, const char* line, DWORD len) {
        if (path.empty()) return;
        HANDLE h = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return;
        DWORD written = 0;
        WriteFile(h, line, len, &written, nullptr);
        WriteFile(h, "\r\n", 2, &written, nullptr);
        CloseHandle(h);
    }

    inline void log(const char* fmt, ...) {
        if (!enabled()) return;
        char buf[1024];
        va_list args; va_start(args, fmt);
        _vsnprintf_s(buf, _TRUNCATE, fmt, args);
        va_end(args);
        // Prefix with tick timestamp
        char line[1152];
        DWORD ms = GetTickCount();
        _snprintf_s(line, _TRUNCATE, "[%08lu] %s", (unsigned long)ms, buf);
        DWORD len = (DWORD)strlen(line);
    writeOne(logPath(), line, len);
    writeOne(altLogPath(), line, len);
    writeOne(docsLogPath(), line, len);
    }
}
