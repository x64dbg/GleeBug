#include "Static.Pe.h"

namespace GleeBug
{
    Pe::Pe(File & file)
        : _file(file)
    {
        Clear();
    }

    void Pe::Clear()
    {
        _fileSize = 0;
        _data.clear();
        _offset = 0;
        _dosHeader = Region<IMAGE_DOS_HEADER>();
        _afterDosData = Region<uint8>();
        _ntHeaders32 = Region<IMAGE_NT_HEADERS32>();
        _ntHeaders64 = Region<IMAGE_NT_HEADERS64>();
        _sectionHeaders = Region<IMAGE_SECTION_HEADER>();
    }

    Pe::Error Pe::ParseHeaders()
    {
        //clear all current data
        Clear();

        //read the DOS header
        _dosHeader = readRegion<IMAGE_DOS_HEADER>();
        if (!_dosHeader)
            return ErrorDosHeaderRead;

        //verify the DOS header
        if (_dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return ErrorDosHeaderMagic;

        //get the NT headers offset
        auto newOffset = uint32(_dosHeader->e_lfanew);

        //verify the new offset
        if (newOffset < 0 || newOffset < _offset || newOffset >= _file.GetSize())
            return ErrorDosHeaderNtHeaderOffset;

        //read & verify the data between the DOS header and the NT headers
        auto afterDosCount = newOffset - _offset;
        _afterDosData = readRegion<uint8>(afterDosCount);
        if (!_afterDosData)
            return ErrorAfterDosHeaderData;

        //read & verify the signature
        auto signature = readRegion<DWORD>();
        if (!signature)
            return ErrorNtSignatureRead;
        if (*(signature()) != IMAGE_NT_SIGNATURE)
            return ErrorNtSignatureMagic;

        //read the file header
        auto ifh = readRegion<IMAGE_FILE_HEADER>();
        if (!ifh)
            return ErrorNtFileHeaderRead;

        //read the optional header
        uint32 realSizeOfIoh;
        switch (ifh->Machine)
        {
        case IMAGE_FILE_MACHINE_I386:
        {
            //read & verify the optional header
            realSizeOfIoh = uint32(sizeof(IMAGE_OPTIONAL_HEADER32));
            auto ioh = readRegion<IMAGE_OPTIONAL_HEADER32>();
            if (!ioh)
                return ErrorNtOptionalHeaderRead;
            if (ioh->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                return ErrorNtOptionalHeaderMagic;

            //construct & verify the NT headers region
            _ntHeaders32 = Region<IMAGE_NT_HEADERS32>(&_data, signature.Offset());
            if (!_ntHeaders32)
                return ErrorNtHeadersRegionSize;
        }
        break;

        case IMAGE_FILE_MACHINE_AMD64:
        {
            //read & verify the optional header
            realSizeOfIoh = uint32(sizeof(IMAGE_OPTIONAL_HEADER64));
            auto ioh = readRegion<IMAGE_OPTIONAL_HEADER64>();
            if (!ioh)
                return ErrorNtOptionalHeaderRead;
            if (ioh->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                return ErrorNtOptionalHeaderMagic;

            //construct & verify the NT headers region
            _ntHeaders64 = Region<IMAGE_NT_HEADERS64>(&_data, signature.Offset());
            if (!_ntHeaders64)
                return ErrorNtHeadersRegionSize;
        }
        break;

        default: //unsupported machine
            return ErrorNtFileHeaderUnsupportedMachine;
        }

        //read data after the optional header
        auto sizeOfIoh = ifh->SizeOfOptionalHeader; //this field can be zero
        uint32 afterOptionalCount = sizeOfIoh > realSizeOfIoh ? sizeOfIoh - realSizeOfIoh : 0;
        _afterOptionalData = readRegion<uint8>(afterOptionalCount);

        //read the section headers
        auto sectionCount = ifh->NumberOfSections;
        _sectionHeaders = readRegion<IMAGE_SECTION_HEADER>(sectionCount);

        return ErrorOk;
    }

    uint32 Pe::readData(uint32 size)
    {
        std::vector<uint8> temp(size);

        if (!_file.Read(_offset, temp.data(), size))
            return INVALID_VALUE;

        auto result = _offset;
        _offset += size;
        _data.insert(_data.end(), temp.begin(), temp.end());
        return result;
    }

    bool Pe::IsValidPe() const
    {
        return _sectionHeaders.Valid();
    }

    bool Pe::IsPe64() const
    {
        return IsValidPe() ? _ntHeaders64.Valid() : false;
    }
};