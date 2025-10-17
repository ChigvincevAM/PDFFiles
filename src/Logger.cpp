#include "Logger.h"
#include "StringConverter.h"
#include <windows.h>
#include <ctime>
#include <vector>

std::recursive_mutex Logger::mutex_;
std::wstring Logger::logFilePathW_;
bool Logger::enabled_ = false;

void Logger::InitializeIfNeeded() {
    if (logFilePathW_.empty()) {
        wchar_t tempPath[MAX_PATH];
        GetTempPathW(MAX_PATH, tempPath);
        logFilePathW_ = std::wstring(tempPath) + L"PdfMerge_Debug.log";

        if (enabled_) {
            Debug("=== PDF Merge Component Debug Log ===");
            SYSTEMTIME st;
            GetLocalTime(&st);
            wchar_t msg[128];
            swprintf_s(msg, L"Started at: %02d-%02d-%04d %02d:%02d:%02d",
                st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
            Debug(StringConverter::WideToUtf8(msg));
        }
    }
}

void Logger::SetEnabled(bool enabled) {
    enabled_ = enabled;
}

bool Logger::IsEnabled() {
    return enabled_;
}

std::wstring Logger::GetLogFilePathW() {
    InitializeIfNeeded();
    return logFilePathW_;
}

void Logger::SetLogFolder(const std::string& folderPathUtf8) {
    if (folderPathUtf8.empty()) return;

    std::wstring folderPathW = StringConverter::Utf8ToWide(folderPathUtf8);
    if (folderPathW.empty()) return;

    CreateDirectoryW(folderPathW.c_str(), nullptr);

    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        logFilePathW_ = folderPathW + L"\\PdfMerge_Debug.log";
    }

    Debug("=== PDF Merge Component Debug Log ===");

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t msg[128];
    swprintf_s(msg, L"Started at: %02d-%02d-%04d %02d:%02d:%02d",
        st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
    Debug(StringConverter::WideToUtf8(msg));

    Debug("Logger redirected to folder: " + folderPathUtf8);
}

void Logger::WriteLineW(const std::wstring& text) {
    if (logFilePathW_.empty()) return;

    HANDLE hFile = CreateFileW(
        logFilePathW_.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE)
        return;

    std::wstring line = text;
    if (line.back() != L'\n')
        line += L"\r\n";

    std::string utf8Line = StringConverter::WideToUtf8(line);
    DWORD bytesWritten = 0;
    WriteFile(hFile, utf8Line.data(), static_cast<DWORD>(utf8Line.size()), &bytesWritten, nullptr);
    CloseHandle(hFile);
}

void Logger::Debug(const std::string& messageUtf8) {
    if (!enabled_) return;
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    InitializeIfNeeded();

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeStr[64];
    swprintf_s(timeStr, L"[%02d:%02d:%02d.%03d] ",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    std::wstring wideTime = timeStr;
    std::wstring wideMsg = StringConverter::Utf8ToWide(messageUtf8);
    WriteLineW(wideTime + wideMsg);
}

void Logger::Error(const std::string& messageUtf8) {
    Debug("ERROR: " + messageUtf8);
}
