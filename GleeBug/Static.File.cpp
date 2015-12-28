#include "Static.File.h"

namespace GleeBug
{
    File::File(const wchar_t* szFileName, File::Mode mode)
        : _fileName(szFileName ? szFileName : L""),
        _mode(mode),
        _hFile(INVALID_HANDLE_VALUE)
    {
    }

    File::~File()
    {
        Close();
    }

    bool File::Open()
    {
        return internalOpen(OPEN_EXISTING);
    }

    bool File::Create(bool overwrite)
    {
        return internalOpen(overwrite ? CREATE_ALWAYS : CREATE_NEW);
    }

    bool File::IsOpen() const
    {
        return _hFile != INVALID_HANDLE_VALUE;
    }

    void File::Close()
    {
        if (IsOpen())
        {
            CloseHandle(_hFile);
            _hFile = INVALID_HANDLE_VALUE;
        }
    }

    uint32 File::GetSize()
    {
        return IsOpen() ? GetFileSize(_hFile, nullptr) : 0;
    }

    bool File::Read(uint32 offset, uint8* data, uint32 size, uint32* bytesRead)
    {
        if (!IsOpen() || !SetFilePointer(_hFile, offset, nullptr, FILE_BEGIN))
        {
            if (bytesRead)
                *bytesRead = 0;
            return false;
        }
        DWORD NumberOfBytesRead = 0;
        auto result = !!ReadFile(_hFile, data, size, &NumberOfBytesRead, nullptr);
        if (bytesRead)
            *bytesRead = uint32(NumberOfBytesRead);
        return result;
    }

    bool File::Write(uint32 offset, const uint8* data, uint32 size, uint32* bytesWritten)
    {
        if (!IsOpen() || !SetFilePointer(_hFile, offset, nullptr, FILE_BEGIN))
        {
            if (bytesWritten)
                *bytesWritten = 0;
            return false;
        }
        DWORD NumberOfBytesWritten = 0;
        auto result = !!WriteFile(_hFile, data, size, &NumberOfBytesWritten, nullptr);
        if (bytesWritten)
            *bytesWritten = uint32(NumberOfBytesWritten);
        return result;
    }

    bool File::internalOpen(DWORD creation)
    {
        //get the access and sharemode flags
        DWORD access, sharemode;
        switch (_mode)
        {
        case ReadOnly:
            access = GENERIC_READ;
            sharemode = FILE_SHARE_READ;
            break;
        case ReadWrite:
            access = GENERIC_READ | GENERIC_WRITE;
            sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
            break;
        default:
            return false;
        }

        //close the previous file
        Close();

        //use WinAPI to get the file handle
        _hFile = CreateFileW(_fileName.c_str(), access, sharemode, nullptr, creation, 0, nullptr);
        return IsOpen();
    }
};