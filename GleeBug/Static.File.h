#ifndef _STATIC_FILE_H
#define _STATIC_FILE_H

#include "Static.Global.h"

namespace GleeBug
{
    class File
    {
    public:
        enum Mode
        {
            ReadOnly,
            ReadWrite
        };

        explicit File(const wchar_t* szFileName, Mode mode = ReadOnly);
        ~File();

        bool Open();
        bool Create(bool overwrite = true);
        bool IsOpen() const;
        void Close();

        uint32 GetSize();
        bool Read(uint32 offset, uint8* data, uint32 size, uint32* bytesRead = nullptr);
        bool Write(uint32 offset, const uint8* data, uint32 size, uint32* bytesWritten = nullptr);

    private:
        bool internalOpen(DWORD creation);

        std::wstring _fileName;
        Mode _mode;
        HANDLE _hFile;
    };
};

#endif //_STATIC_FILE_H
