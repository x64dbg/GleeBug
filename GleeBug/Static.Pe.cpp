#include "Static.Pe.h"

namespace GleeBug
{
    Pe::Pe(File & file)
        : mFile(file)
    {
        Clear();
        setupErrorMap();
    }

    void Pe::Clear()
    {
        mData.clear();
        mOffset = 0;
        mIsPe64 = false;

        mDosHeader.Clear();
        mDosNtOverlap = false;
        mAfterDosData.Clear();
        mNtHeaders32.Clear();
        mNtHeaders64.Clear();
        mAfterOptionalData.Clear();
        mSectionHeaders.Clear();
        mAfterSectionHeadersData.Clear();
        mSections.clear();
    }

    const char* Pe::ErrorText(Error error) const
    {
        auto found = mErrorMap.find(error);
        return found == mErrorMap.end() ? "" : found->second;
    }

    bool Pe::IsValidPe() const
    {
        return IsPe64() ? mNtHeaders64.Valid() : mNtHeaders32.Valid();
    }

    bool Pe::IsPe64() const
    {
        return mIsPe64;
    }

    Pe::Error Pe::Parse(bool allowOverlap)
    {
        //clear all current data
        Clear();

        //read the DOS header
        mDosHeader = readRegion<IMAGE_DOS_HEADER>();
        if(!mDosHeader)
            return ErrorDosHeaderRead;

        //verify the DOS header
        if(mDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return ErrorDosHeaderMagic;

        //get the NT headers offset
        auto newOffset = mDosHeader->e_lfanew;

        //verify the new offset
        if(newOffset < 0 || uint32(newOffset) >= mFile.GetSize())
            return ErrorDosHeaderNtHeaderOffset;

        //special case where DOS and PE header overlap (tinygui.exe)
        if(uint32(newOffset) < mOffset)
        {
            if(!allowOverlap)
                return ErrorDosHeaderNtHeaderOffsetOverlap;

            mDosNtOverlap = true;
            mOffset = newOffset;
            mAfterDosData = readRegion<uint8>(0);
        }
        else
        {
            //read & verify the data between the DOS header and the NT headers
            auto afterDosCount = newOffset - mOffset;
            mAfterDosData = readRegion<uint8>(afterDosCount);
        }

        if(!mAfterDosData)
            return ErrorAfterDosHeaderData;

        //read & verify the signature
        auto signature = readRegion<DWORD>();
        if(!signature)
            return ErrorNtSignatureRead;
        if(*signature() != IMAGE_NT_SIGNATURE)
            return ErrorNtSignatureMagic;

        //read the file header
        auto ifh = readRegion<IMAGE_FILE_HEADER>();
        if(!ifh)
            return ErrorNtFileHeaderRead;

        //read the optional header
        uint32 realSizeOfIoh;
        switch(ifh->Machine)
        {
        case IMAGE_FILE_MACHINE_I386:
        {
            //read & verify the optional header
            realSizeOfIoh = uint32(sizeof(IMAGE_OPTIONAL_HEADER32));
            auto ioh = readRegion<IMAGE_OPTIONAL_HEADER32>();
            if(!ioh)  //TODO: support truncated optional header (tinyXP.exe)
                return ErrorNtOptionalHeaderRead;
            if(ioh->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                return ErrorNtOptionalHeaderMagic;

            //construct & verify the NT headers region
            mNtHeaders32 = Region<IMAGE_NT_HEADERS32>(&mData, signature.Offset());
            if(!mNtHeaders32)
                return ErrorNtHeadersRegionSize;
        }
        break;

        case IMAGE_FILE_MACHINE_AMD64:
        {
            mIsPe64 = true;

            //read & verify the optional header
            realSizeOfIoh = uint32(sizeof(IMAGE_OPTIONAL_HEADER64));
            auto ioh = readRegion<IMAGE_OPTIONAL_HEADER64>();
            if(!ioh)
                return ErrorNtOptionalHeaderRead;
            if(ioh->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                return ErrorNtOptionalHeaderMagic;

            //construct & verify the NT headers region
            mNtHeaders64 = Region<IMAGE_NT_HEADERS64>(&mData, signature.Offset());
            if(!mNtHeaders64)
                return ErrorNtHeadersRegionSize;
        }
        break;

        default: //unsupported machine
        {
            //try the best possible effort (corkami's d_resource.dll)
            auto ioh = readRegion<uint8>(ifh->SizeOfOptionalHeader);
            if(!ioh)
                return ErrorNtFileHeaderUnsupportedMachineOptionalHeaderRead;

            mNtHeaders32 = Region<IMAGE_NT_HEADERS32>(&mData, signature.Offset());
            if(!mNtHeaders32)
                return ErrorNtFileHeaderUnsupportedMachineNtHeadersRegionSize;

            return ErrorNtFileHeaderUnsupportedMachine;
        }
        }

        auto numberOfSections = ifh->NumberOfSections;

        //check the SizeOfOptionalHeader field
        auto sizeOfIoh = ifh->SizeOfOptionalHeader;
        if(numberOfSections && sizeOfIoh < realSizeOfIoh)  //TODO: this can be valid in certain circumstances (nullSOH-XP)
            return ErrorNtFileHeaderSizeOfOptionalHeaderOverlap;

        //read data after the optional header (TODO: check if this is even possible)
        uint32 afterOptionalSize = realSizeOfIoh < sizeOfIoh ? sizeOfIoh - realSizeOfIoh : 0;
        mAfterOptionalData = readRegion<uint8>(afterOptionalSize);
        if(!mAfterOptionalData)
            return ErrorAfterOptionalHeaderDataRead;

        //read the section headers
        mSectionHeaders = readRegion<IMAGE_SECTION_HEADER>(numberOfSections);
        if(!mSectionHeaders)
            return ErrorSectionHeadersRead;

        //parse the sections
        auto sectionsError = parseSections(numberOfSections);
        if(sectionsError != ErrorOk)
            return sectionsError;

        //TODO: parse data directories
        return ErrorOk;
    }

    uint32 Pe::ConvertOffsetToRva(uint32 offset)
    {
        if(!mOffsetSectionMap.size())  //TODO: verify this (no sections means direct mapping)
            return offset;
        const auto found = mOffsetSectionMap.find(Range(offset, offset));
        if(found == mOffsetSectionMap.end())
            return INVALID_VALUE;
        auto index = found->second;
        if(index == HeaderSection)
            return offset;
        const auto & section = mSections[index];
        offset -= uint32(found->first.first); //adjust the offset to be relative to the offset range in the map
        return alignAdjustAddress(section.GetHeader().VirtualAddress, section.GetAlignment()) + offset;
    }

    uint32 Pe::ConvertRvaToOffset(uint32 rva)
    {
        if(!mRvaSectionMap.size())  //TODO: verify this (no sections means direct mapping)
            return rva;
        const auto found = mRvaSectionMap.find(Range(rva, rva));
        if(found == mRvaSectionMap.end())
            return INVALID_VALUE;
        auto index = found->second;
        if(index == HeaderSection)
            return rva;
        const auto & section = mSections[index];
        rva -= uint32(found->first.first); //adjust the rva to be relative to the rva range in the map
        return section.GetHeader().PointerToRawData + rva;
    }

    Pe::Error Pe::parseSections(uint16 count, uint32 alignment)
    {
        if(!count)
            return ErrorOk;

        auto sectionHeaders = GetSectionHeaders();

        struct SectionInfo
        {
            uint16 index;
            IMAGE_SECTION_HEADER header; //by value to prevent pointer invalidation
            Region<uint8> beforeData;
            Region<uint8> data;
        };

        //sort sections on raw address to prevent read errors and have a contiguous buffer
        std::vector<SectionInfo> sortedHeaders;
        sortedHeaders.reserve(count);
        for(uint16 i = 0; i < count; i++)
            sortedHeaders.push_back(SectionInfo{ i, sectionHeaders[i] });

        std::sort(sortedHeaders.begin(), sortedHeaders.end(), [](const SectionInfo & a, const SectionInfo & b)
        {
            if(a.header.PointerToRawData < b.header.PointerToRawData)
                return true;
            if(a.header.PointerToRawData > b.header.PointerToRawData)
                return false;
            //dupsec.exe has 2 identical sections (besides the VirtualAddress)
            return a.header.VirtualAddress < b.header.VirtualAddress;
        });

        /*
        //get after section headers data
        auto firstRawAddress = sortedHeaders[0].header.PointerToRawData;
        auto afterDataSize = mOffset < firstRawAddress ? firstRawAddress - mOffset : 0;
        mAfterSectionHeadersData = readRegion<uint8>(afterDataSize);
        if (!mAfterSectionHeadersData)
            return ErrorAfterSectionHeadersDataRead;

        //read the actual section data.
        for (auto & section : sortedHeaders)
        {
            auto rawAddress = section.header.PointerToRawData;
            auto beforeSize = mOffset < rawAddress ? rawAddress - mOffset : 0;
            section.beforeData = readRegion<uint8>(beforeSize);
            if (!section.beforeData)
                return ErrorBeforeSectionDataRead;
            //bigSoRD.exe: if raw size is bigger than virtual size, then virtual size is taken.
            auto rawSize = min(section.header.SizeOfRawData, section.header.Misc.VirtualSize);
            section.data = readRegion<uint8>(rawSize);
            if (rawSize && !section.data)
                return ErrorSectionDataRead;
        }
        */

        //re-sort the sections by index
        std::sort(sortedHeaders.begin(), sortedHeaders.end(), [](const SectionInfo & a, const SectionInfo & b)
        {
            return a.index < b.index;
        });

        //add the sections to the mSections vector
        mSections.reserve(count);
        for(uint16 i = 0; i < count; i++)
        {
            const auto & section = sortedHeaders[i];
            mSections.push_back(Section(i, alignment, mSectionHeaders, section.beforeData, section.data));
        }

        //create rva/offset -> section maps
        if(count)  //insert pe header offset/rva (file start -> first section is the PE header)
        {
            const auto & section = mSections[0];
            mOffsetSectionMap.insert({ Range(0, section.GetHeader().PointerToRawData - 1), HeaderSection });
            auto rva = alignAdjustSize(section.GetHeader().VirtualAddress, alignment);
            mRvaSectionMap.insert({ Range(0, rva - 1), HeaderSection });
        }
        else //TODO: handle file without sections
        {
        }
        for(const auto & section : mSections)
        {
            //offset -> section index
            auto offset = section.GetHeader().PointerToRawData;
            //bigSoRD.exe: if raw size is bigger than virtual size, then virtual size is taken.
            auto rsize = min(section.GetHeader().SizeOfRawData, section.GetHeader().Misc.VirtualSize);
            if(!rsize) //65535sects.exe
                continue;
            mOffsetSectionMap.insert({ Range(offset, offset + rsize - 1), section.GetIndex() });

            //rva -> section index
            auto rva = alignAdjustAddress(section.GetHeader().VirtualAddress, alignment);
            auto vsize = alignAdjustSize(section.GetHeader().Misc.VirtualSize, alignment);
            mRvaSectionMap.insert({ Range(rva, rva + vsize - 1), section.GetIndex() });
        }

        return ErrorOk;
    }

    uint32 Pe::readData(uint32 size)
    {
        if(!size)
            return mOffset;
        std::vector<uint8> temp(size);

        if(!mFile.Read(mOffset, temp.data(), size))
            return INVALID_VALUE;

        auto result = mOffset;
        mOffset += size;
        mData.insert(mData.end(), temp.begin(), temp.end());
        return result;
    }

    void Pe::setupErrorMap()
    {
        auto add = [this](Error e, const char* s)
        {
            mErrorMap.insert({ e, s });
        };
        add(ErrorOk, "ErrorOk");
        add(ErrorDosHeaderRead, "ErrorDosHeaderRead");
        add(ErrorDosHeaderMagic, "ErrorDosHeaderMagic");
        add(ErrorDosHeaderNtHeaderOffset, "ErrorDosHeaderNtHeaderOffset");
        add(ErrorDosHeaderNtHeaderOffsetOverlap, "ErrorDosHeaderNtHeaderOffsetOverlap");
        add(ErrorAfterDosHeaderData, "ErrorAfterDosHeaderData");
        add(ErrorNtSignatureRead, "ErrorNtSignatureRead");
        add(ErrorNtSignatureMagic, "ErrorNtSignatureMagic");
        add(ErrorNtFileHeaderRead, "ErrorNtFileHeaderRead");
        add(ErrorNtFileHeaderSizeOfOptionalHeaderOverlap, "ErrorNtFileHeaderSizeOfOptionalHeaderOverlap");
        add(ErrorNtFileHeaderUnsupportedMachine, "ErrorNtFileHeaderUnsupportedMachine");
        add(ErrorNtFileHeaderUnsupportedMachineOptionalHeaderRead, "ErrorNtFileHeaderUnsupportedMachineOptionalHeaderRead");
        add(ErrorNtFileHeaderUnsupportedMachineNtHeadersRegionSize, "ErrorNtFileHeaderUnsupportedMachineNtHeadersRegionSize");
        add(ErrorNtOptionalHeaderRead, "ErrorNtOptionalHeaderRead");
        add(ErrorNtOptionalHeaderMagic, "ErrorNtOptionalHeaderMagic");
        add(ErrorAfterOptionalHeaderDataRead, "ErrorAfterOptionalHeaderDataRead");
        add(ErrorNtHeadersRegionSize, "ErrorNtHeadersRegionSize");
        add(ErrorSectionHeadersRead, "ErrorSectionHeadersRead");
        add(ErrorAfterSectionHeadersDataRead, "ErrorAfterSectionHeadersDataRead");
        add(ErrorBeforeSectionDataRead, "ErrorBeforeSectionDataRead");
        add(ErrorSectionDataRead, "ErrorSectionDataRead");
    }
};