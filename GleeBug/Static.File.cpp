#include "Static.File.h"

namespace GleeBug
{
    File::File(const wchar_t* szFileName, File::Mode mode)
        : mFileName(szFileName ? szFileName : L""),
        mMode(mode),
        mhFile(INVALID_HANDLE_VALUE)
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
        return mhFile != INVALID_HANDLE_VALUE;
    }

    void File::Close()
    {
        if (IsOpen())
        {
            CloseHandle(mhFile);
            mhFile = INVALID_HANDLE_VALUE;
        }
    }

    uint32 File::GetSize() const
    {
        return IsOpen() ? GetFileSize(mhFile, nullptr) : 0;
    }

    bool File::Read(uint32 offset, void* data, uint32 size, uint32* bytesRead) const
    {
        if (!IsOpen() || SetFilePointer(mhFile, offset, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            if (bytesRead)
                *bytesRead = 0;
            return false;
        }
        DWORD NumberOfBytesRead = 0;
        auto result = !!ReadFile(mhFile, data, size, &NumberOfBytesRead, nullptr);
        if (bytesRead)
            *bytesRead = uint32(NumberOfBytesRead);
        return result;
    }

    bool File::Write(uint32 offset, const void* data, uint32 size, uint32* bytesWritten)
    {
        if (!IsOpen() || SetFilePointer(mhFile, offset, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            if (bytesWritten)
                *bytesWritten = 0;
            return false;
        }
        DWORD NumberOfBytesWritten = 0;
        auto result = !!WriteFile(mhFile, data, size, &NumberOfBytesWritten, nullptr);
        if (bytesWritten)
            *bytesWritten = uint32(NumberOfBytesWritten);
        return result;
    }

    bool File::internalOpen(DWORD creation)
    {
        //get the access and sharemode flags
        DWORD access, sharemode;
        switch (mMode)
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
        mhFile = CreateFileW(mFileName.c_str(), access, sharemode, nullptr, creation, 0, nullptr);
        return IsOpen();
    }
};