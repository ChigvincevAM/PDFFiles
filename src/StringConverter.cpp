#pragma once

#include <string>
#include <windows.h>
#include <stdexcept>
#include <algorithm>

#include "StringConverter.h"
#include "Logger.h"

std::wstring StringConverter::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) {
        return std::wstring();
    }

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
        static_cast<int>(utf8.size()),
        nullptr, 0);
    if (sizeNeeded <= 0) {
        return std::wstring();
    }

    std::wstring wide(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
        static_cast<int>(utf8.size()),
        &wide[0], sizeNeeded);
    return wide;
}

std::string StringConverter::WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) {
        return std::string();
    }

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
        static_cast<int>(wide.size()),
        nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        return std::string();
    }

    std::string utf8(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
        static_cast<int>(wide.size()),
        &utf8[0], sizeNeeded, nullptr, nullptr);
    return utf8;
}

std::string StringConverter::Trim(const std::string& input) {
    size_t start = 0;
    size_t end = input.size();

    while (start < end && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return input.substr(start, end - start);
}

std::string StringConverter::StripQuotes(const std::string& input) {
    if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

std::string StringConverter::NormalizePathSeparators(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (char c : input) {
        result += (c == '/') ? '\\' : c;
    }
    return result;
}

std::string StringConverter::SanitizePath(const std::string& rawPath) {
    std::string path = Trim(rawPath);
    path = StripQuotes(path);
    path = NormalizePathSeparators(path);
    return path;
}

std::string StringConverter::ToLowercase(const std::string& input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return result;
}
