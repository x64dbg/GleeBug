#include "GleeBug.h"

namespace GleeBug
{
    //Conversion functions taken from: http://www.nubaria.com/en/blog/?p=289
    std::string Utf16ToUtf8(const std::wstring & wstr)
    {
        std::string convertedString;
        auto requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if(requiredSize > 0)
        {
            std::vector<char> buffer(requiredSize);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, nullptr, nullptr);
            convertedString.assign(buffer.begin(), buffer.end() - 1);
        }
        return convertedString;
    }

    std::string Utf16ToUtf8(const wchar_t* wstr)
    {
        return Utf16ToUtf8(wstr ? std::wstring(wstr) : std::wstring());
    }

    std::wstring Utf8ToUtf16(const std::string & str)
    {
        std::wstring convertedString;
        int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if(requiredSize > 0)
        {
            std::vector<wchar_t> buffer(requiredSize);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], requiredSize);
            convertedString.assign(buffer.begin(), buffer.end() - 1);
        }
        return convertedString;
    }

    std::wstring Utf8ToUtf16(const char* str)
    {
        return Utf8ToUtf16(str ? std::string(str) : std::string());
    }
};