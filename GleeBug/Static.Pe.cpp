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

    bool Pe::ParseHeaders()
    {
        //clear all current data
        Clear();

        //read the DOS header
        _dosHeader = readRegion<IMAGE_DOS_HEADER>();
        if (!_dosHeader)
            return false;

        //verify the DOS header
        if (_dosHeader()->e_magic != IMAGE_DOS_SIGNATURE)
            return false;

        //get the NT headers offset
        auto newOffset = uint32(_dosHeader()->e_lfanew);

        //verify the new offset
        if (newOffset < 0 || newOffset < _offset || newOffset >= _file.GetSize())
            return false;

        //read the data between the DOS header and the NT headers
        auto offsetDiff = newOffset - _offset;
        _afterDosData = readRegion<uint8>(offsetDiff);

        //read & verify the signature
        auto signature = readRegion<DWORD>();
        if (!signature || *(signature()) != IMAGE_NT_SIGNATURE)
            return false;

        //read the file header
        auto ifh = readRegion<IMAGE_FILE_HEADER>();
        if (!ifh)
            return false;
        switch (ifh()->Machine)
        {
        case IMAGE_FILE_MACHINE_I386:
            _ntHeaders32 = Region<IMAGE_NT_HEADERS32>(&_data, signature.Offset()); //this region is not valid at this point
            if (!readRegion<IMAGE_OPTIONAL_HEADER32>()) //read the optional header data
                return false;
            if (!_ntHeaders32) //validate we read the entire NT headers region
                return false;
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            _ntHeaders64 = Region<IMAGE_NT_HEADERS64>(&_data, signature.Offset()); //this region is not valid at this point
            if (!readRegion<IMAGE_OPTIONAL_HEADER64>()) //read the optional header data
                return false;
            if (!_ntHeaders64) //validate we read the entire NT headers region
                return false;
            break;
        default: //unsupported machine
            return false;
        }

#define IMAGE_FIRST_SECTION123( ntheader ) ((PIMAGE_SECTION_HEADER)        \
    ((ULONG_PTR)(ntheader) +                                            \
     FIELD_OFFSET( IMAGE_NT_HEADERS, OptionalHeader ) +                 \
     ((ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))
        return true;
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
        return false;
    }

    bool Pe::IsPe64() const
    {
        return false;
    }
};