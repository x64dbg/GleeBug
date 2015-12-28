#ifndef _STATIC_PE_H
#define _STATIC_PE_H

#include "Static.File.h"
#include "Static.Region.h"

namespace GleeBug
{
    class Pe
    {
    public:
        explicit Pe(File & file);

        void Clear();
        bool ParseHeaders();
        bool IsValidPe() const;
        bool IsPe64() const;

        const Region<IMAGE_DOS_HEADER> & GetDosHeader() const { return _dosHeader; }
        const Region<uint8> & GetAfterDosData() const { return _afterDosData; }
        const Region<IMAGE_NT_HEADERS32> & GetNtHeaders32() const { return _ntHeaders32; }
        const Region<IMAGE_NT_HEADERS64> & GetNtHeaders64() const { return _ntHeaders64; }
        const Region<IMAGE_SECTION_HEADER> & GetSectionHeaders() const { return _sectionHeaders; }

    private:
        uint32 readData(uint32 size);

        template<typename T>
        inline Region<T> readRegion(uint32 count = 1)
        {
            return Region<T>(&_data, readData(sizeof(T) * count), count);
        }

        File & _file;
        uint32 _fileSize;
        std::vector<uint8> _data;
        uint32 _offset;

        Region<IMAGE_DOS_HEADER> _dosHeader;
        Region<uint8> _afterDosData;
        Region<IMAGE_NT_HEADERS32> _ntHeaders32;
        Region<IMAGE_NT_HEADERS64> _ntHeaders64;
        Region<IMAGE_SECTION_HEADER> _sectionHeaders;
    };
};

#endif //_STATIC_PE_H