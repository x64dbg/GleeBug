#ifndef _STATIC_PE_H
#define _STATIC_PE_H

#include "Static.File.h"
#include "Static.Region.h"

namespace GleeBug
{
    class Pe
    {
    public:
        enum Error
        {
            ErrorOk = 0,
            ErrorDosHeaderRead = 1,
            ErrorDosHeaderMagic = 2,
            ErrorDosHeaderNtHeaderOffset = 3,
            ErrorDosHeaderNtHeaderOffsetOverlap = 4,
            ErrorAfterDosHeaderData = 5,
            ErrorNtSignatureRead = 6,
            ErrorNtSignatureMagic = 7,
            ErrorNtFileHeaderRead = 8,
            ErrorNtFileHeaderSizeOfOptionalHeaderOverlap = 9,
            ErrorNtFileHeaderUnsupportedMachine = 10,
            ErrorNtFileHeaderUnsupportedMachineOptionalHeaderRead = 11,
            ErrorNtFileHeaderUnsupportedMachineNtHeadersRegionSize = 12,
            ErrorNtOptionalHeaderRead = 13,
            ErrorNtOptionalHeaderMagic = 14,
            ErrorNtHeadersRegionSize = 15,
        };

        explicit Pe(File & file);

        void Clear();
        Error ParseHeaders();
        bool IsValidPe() const;
        bool IsPe64() const;

        const Region<IMAGE_DOS_HEADER> & GetDosHeader() const { return _dosHeader; }
        const Region<uint8> & GetAfterDosData() const { return _afterDosData; }
        const Region<IMAGE_NT_HEADERS32> & GetNtHeaders32() const { return _ntHeaders32; }
        const Region<IMAGE_NT_HEADERS64> & GetNtHeaders64() const { return _ntHeaders64; }
        const Region<uint8> & GetAfterOptionalData() const { return _afterOptionalData; }
        const Region<IMAGE_SECTION_HEADER> & GetSectionHeaders() const { return _sectionHeaders; }

    private:
        uint32 readData(uint32 size);
        void setupErrorMap();

        template<typename T>
        inline Region<T> readRegion(uint32 count = 1)
        {
            return Region<T>(&_data, readData(sizeof(T) * count), count);
        }

        std::unordered_map<Error, const char*> _errorMap;
        File & _file;
        uint32 _fileSize;
        std::vector<uint8> _data;
        uint32 _offset;

        Region<IMAGE_DOS_HEADER> _dosHeader;
        Region<uint8> _afterDosData;
        Region<IMAGE_NT_HEADERS32> _ntHeaders32;
        Region<IMAGE_NT_HEADERS64> _ntHeaders64;
        Region<uint8> _afterOptionalData;
        Region<IMAGE_SECTION_HEADER> _sectionHeaders;
    };
};

#endif //_STATIC_PE_H