#include "Static.BufferFile.h"

namespace GleeBug
{
    BufferFile::BufferFile(void* data, uint32 size)
        : File(nullptr),
        mData(data),
        mSize(size)
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
        return !!mData;
    }

    void BufferFile::Close()
    {
    }

    uint32 BufferFile::GetSize() const
    {
        return mSize;
    }

    bool BufferFile::Read(uint32 offset, void* data, uint32 size, uint32* bytesRead) const
    {
        if (offset >= mSize)
            return false;
        auto readSize = size;
        auto result = true;
        if (offset + size > mSize)
        {
            readSize = mSize - offset;
            result = false;
        }
        memcpy(data, (uint8*)mData + offset, readSize);
        if (bytesRead)
            *bytesRead = readSize;
        return result;
    }

    bool BufferFile::Write(uint32 offset, const void* data, uint32 size, uint32* bytesWritten)
    {
        if (offset >= mSize)
            return false;
        auto writeSize = size;
        auto result = true;
        if (offset + size > mSize)
        {
            writeSize = mSize - offset;
            result = false;
        }
        memcpy((uint8*)mData + offset, data, writeSize);
        if (bytesWritten)
            *bytesWritten = writeSize;
        return result;
    }
};