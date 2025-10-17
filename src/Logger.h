#pragma once
#include <string>
#include <mutex>
#include <windows.h>

class Logger {
public:
    static void SetEnabled(bool enabled);
    static bool IsEnabled();

    static void Debug(const std::string& messageUtf8);
    static void Error(const std::string& messageUtf8);

    static void SetLogFolder(const std::string& folderPathUtf8);
    static std::wstring GetLogFilePathW();

private:
    static void InitializeIfNeeded();
    static void WriteLineW(const std::wstring& text);

    static std::recursive_mutex mutex_;
    static std::wstring logFilePathW_;
    static bool enabled_;
};
