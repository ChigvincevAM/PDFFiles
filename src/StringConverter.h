#ifndef STRINGCONVERTER_H
#define STRINGCONVERTER_H

#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

class StringConverter {
public:

    static std::wstring Utf8ToWide(const std::string& utf8);

    static std::string WideToUtf8(const std::wstring& wide);

    static std::string Trim(const std::string& input);

    static std::string StripQuotes(const std::string& input);

    static std::string NormalizePathSeparators(const std::string& input);

    static std::string SanitizePath(const std::string& rawPath);

    static std::string ToLowercase(const std::string& input);
};

#endif // STRINGCONVERTER_H
