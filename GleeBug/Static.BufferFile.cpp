#include "Static.BufferFile.h"

namespace GleeBug
{
    BufferFile::BufferFile(void* data, uint32 size)
        : File(nullptr),
        _data(data),
        _size(size)
    {
    }

    bool BufferFile::Open()
    {
        return true;
    }

    bool BufferFile::Create(bool)
    {
        return true;
    }

    bool BufferFile::IsOpen() const
    {
        return !!_data;
    }

    void BufferFile::Close()
    {
    }

    uint32 BufferFile::GetSize() const
    {
        return _size;
    }

    bool BufferFile::Read(uint32 offset, void* data, uint32 size, uint32* bytesRead) const
    {
        if (offset >= _size)
            return false;
        auto readSize = size;
        auto result = true;
        if (offset + size > _size)
        {
            readSize = _size - offset;
            result = false;
        }
        memcpy(data, (uint8*)_data + offset, readSize);
        if (bytesRead)
            *bytesRead = readSize;
        return result;
    }

    bool BufferFile::Write(uint32 offset, const void* data, uint32 size, uint32* bytesWritten)
    {
        if (offset >= _size)
            return false;
        auto writeSize = size;
        auto result = true;
        if (offset + size > _size)
        {
            writeSize = _size - offset;
            result = false;
        }
        memcpy((uint8*)_data + offset, data, writeSize);
        if (bytesWritten)
            *bytesWritten = writeSize;
        return result;
    }
};