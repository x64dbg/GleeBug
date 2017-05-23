#pragma once

#include <windows.h>

template<typename T>
struct FileMap
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = nullptr;
    T* data = nullptr;
    unsigned int size = 0;

    FileMap() { }

    ~FileMap()
    {
        Unmap();
    }

    FileMap(const FileMap<T> &) = delete;

    FileMap(FileMap<T> && other)
    {
        other.hFile = hFile;
        other.hMap = hMap;
        other.data = data;
        other.size = size;
    }

    bool Map(const wchar_t* szFileName, bool write = false)
    {
        hFile = CreateFileW(szFileName, GENERIC_READ | (write ? GENERIC_WRITE : 0), FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            size = GetFileSize(hFile, nullptr);
            hMap = CreateFileMappingW(hFile, nullptr, write ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
            if(hMap)
                data = (T*)MapViewOfFile(hMap, write ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);
        }
        return data != nullptr;
    }

    void Unmap()
    {
        if(data)
            UnmapViewOfFile(data);
        if(hMap)
            CloseHandle(hMap);
        if(hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);

        hFile = INVALID_HANDLE_VALUE;
        hMap = nullptr;
        data = nullptr;
        size = 0;
    }
};