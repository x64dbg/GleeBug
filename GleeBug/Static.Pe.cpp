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
        if (!mDosHeader)
            return ErrorDosHeaderRead;

        //verify the DOS header
        if (mDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
            return ErrorDosHeaderMagic;

        //get the NT headers offset
        auto newOffset = mDosHeader->e_lfanew;

        //verify the new offset
        if (newOffset < 0 || uint32(newOffset) >= mFile.GetSize())
            return ErrorDosHeaderNtHeaderOffset;

        //special case where DOS and PE header overlap (tinygui.exe)
        if (uint32(newOffset) < mOffset)
        {
            if (!allowOverlap)
                return ErrorDosHeaderNtHeaderOffsetOverlap;

            mDosNtOverlap = true;
            mOffset = newOffset;
        }
        else
        {
            //read & verify the data between the DOS header and the NT headers
            auto afterDosCount = newOffset - mOffset;
            mAfterDosData = readRegion<uint8>(afterDosCount);
            if (!mAfterDosData)
                return ErrorAfterDosHeaderData;
        }

        //read & verify the signature
        auto signature = readRegion<DWORD>();
        if (!signature)
            return ErrorNtSignatureRead;
        if (*signature() != IMAGE_NT_SIGNATURE)
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
            if (!ioh) //TODO: support truncated optional header (tinyXP.exe)
                return ErrorNtOptionalHeaderRead;
            if (ioh->Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                return ErrorNtOptionalHeaderMagic;

            //construct & verify the NT headers region
            mNtHeaders32 = Region<IMAGE_NT_HEADERS32>(&mData, signature.Offset());
            if (!mNtHeaders32)
                return ErrorNtHeadersRegionSize;
        }
        break;

        case IMAGE_FILE_MACHINE_AMD64:
        {
            mIsPe64 = true;

            //read & verify the optional header
            realSizeOfIoh = uint32(sizeof(IMAGE_OPTIONAL_HEADER64));
            auto ioh = readRegion<IMAGE_OPTIONAL_HEADER64>();
            if (!ioh)
                return ErrorNtOptionalHeaderRead;
            if (ioh->Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                return ErrorNtOptionalHeaderMagic;

            //construct & verify the NT headers region
            mNtHeaders64 = Region<IMAGE_NT_HEADERS64>(&mData, signature.Offset());
            if (!mNtHeaders64)
                return ErrorNtHeadersRegionSize;
        }
        break;

        default: //unsupported machine
        {
            //try the best possible effort (corkami's d_resource.dll)
            auto ioh = readRegion<uint8>(ifh->SizeOfOptionalHeader);
            if (!ioh)
                return ErrorNtFileHeaderUnsupportedMachineOptionalHeaderRead;

            mNtHeaders32 = Region<IMAGE_NT_HEADERS32>(&mData, signature.Offset());
            if (!mNtHeaders32)
                return ErrorNtFileHeaderUnsupportedMachineNtHeadersRegionSize;

            return ErrorNtFileHeaderUnsupportedMachine;
        }
        }

        auto numberOfSections = ifh->NumberOfSections;

        //check the SizeOfOptionalHeader field
        auto sizeOfIoh = ifh->SizeOfOptionalHeader;
        if (numberOfSections && sizeOfIoh < realSizeOfIoh) //TODO: this can be valid in certain circumstances (nullSOH-XP)
            return ErrorNtFileHeaderSizeOfOptionalHeaderOverlap;

        //read data after the optional header (TODO: check if this is even possible)
        uint32 afterOptionalCount = sizeOfIoh > realSizeOfIoh ? sizeOfIoh - realSizeOfIoh : 0;
        mAfterOptionalData = readRegion<uint8>(afterOptionalCount);

        //read the section headers
        mSectionHeaders = readRegion<IMAGE_SECTION_HEADER>(numberOfSections);
        if (!mSectionHeaders)
            return ErrorSectionHeadersRead;

        //parse the sections
        auto sectionsError = parseSections(numberOfSections);
        if (sectionsError != ErrorOk)
            return sectionsError;

        //TODO: parse data directories
        return ErrorOk;
    }

    Pe::Error Pe::parseSections(uint16 count)
    {
        if (!count)
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
        for (uint16 i = 0; i < count; i++)
            sortedHeaders.push_back(SectionInfo{ i, sectionHeaders[i] });

        std::sort(sortedHeaders.begin(), sortedHeaders.end(), [](const SectionInfo & a, const SectionInfo & b)
        {
            return a.header.PointerToRawData < b.header.PointerToRawData;
        });

        //get after section headers data
        auto firstRawAddress = sortedHeaders[0].header.PointerToRawData;
        if (mOffset < firstRawAddress)
            mAfterSectionHeadersData = readRegion<uint8>(firstRawAddress - mOffset);

        //read the actual section data.
        for (auto & section : sortedHeaders)
        {
            auto rawAddress = section.header.PointerToRawData;
            auto beforeSize = mOffset < rawAddress ? rawAddress - mOffset : 0;
            section.beforeData = readRegion<uint8>(beforeSize);
            if (!section.beforeData)
                return ErrorBeforeSectionDataRead;
            section.data = readRegion<uint8>(section.header.SizeOfRawData);
            if (!section.data)
                return ErrorSectionDataRead;
        }

        //re-sort the sections by index
        std::sort(sortedHeaders.begin(), sortedHeaders.end(), [](const SectionInfo & a, const SectionInfo & b)
        {
            return a.index < b.index;
        });

        //add the sections to the mSections vector
        mSections.reserve(count);
        for (uint16 i = 0; i < count; i++)
        {
            const auto & section = sortedHeaders[i];
            mSections.push_back(Section(i, mSectionHeaders, section.beforeData, section.data));
        }

        return ErrorOk;
    }

    uint32 Pe::readData(uint32 size)
    {
        std::vector<uint8> temp(size);

        if (!mFile.Read(mOffset, temp.data(), size))
            return INVALID_VALUE;

        auto result = mOffset;
        mOffset += size;
        mData.insert(mData.end(), temp.begin(), temp.end());
        return result;
    }

    void Pe::setupErrorMap()
    {
        mErrorMap.insert({ ErrorOk, "ErrorOk" });
        mErrorMap.insert({ ErrorDosHeaderRead, "ErrorDosHeaderRead" });
        mErrorMap.insert({ ErrorDosHeaderMagic, "ErrorDosHeaderMagic" });
        mErrorMap.insert({ ErrorDosHeaderNtHeaderOffset, "ErrorDosHeaderNtHeaderOffset" });
        mErrorMap.insert({ ErrorDosHeaderNtHeaderOffsetOverlap, "ErrorDosHeaderNtHeaderOffsetOverlap" });
        mErrorMap.insert({ ErrorAfterDosHeaderData, "ErrorAfterDosHeaderData" });
        mErrorMap.insert({ ErrorNtSignatureRead, "ErrorNtSignatureRead" });
        mErrorMap.insert({ ErrorNtSignatureMagic, "ErrorNtSignatureMagic" });
        mErrorMap.insert({ ErrorNtFileHeaderRead, "ErrorNtFileHeaderRead" });
        mErrorMap.insert({ ErrorNtFileHeaderSizeOfOptionalHeaderOverlap, "ErrorNtFileHeaderSizeOfOptionalHeaderOverlap" });
        mErrorMap.insert({ ErrorNtFileHeaderUnsupportedMachine, "ErrorNtFileHeaderUnsupportedMachine" });
        mErrorMap.insert({ ErrorNtFileHeaderUnsupportedMachineOptionalHeaderRead, "ErrorNtFileHeaderUnsupportedMachineOptionalHeaderRead" });
        mErrorMap.insert({ ErrorNtFileHeaderUnsupportedMachineNtHeadersRegionSize, "ErrorNtFileHeaderUnsupportedMachineNtHeadersRegionSize" });
        mErrorMap.insert({ ErrorNtOptionalHeaderRead, "ErrorNtOptionalHeaderRead" });
        mErrorMap.insert({ ErrorNtOptionalHeaderMagic, "ErrorNtOptionalHeaderMagic" });
        mErrorMap.insert({ ErrorNtHeadersRegionSize, "ErrorNtHeadersRegionSize" });
        mErrorMap.insert({ ErrorSectionHeadersRead, "ErrorSectionHeadersRead" });
        mErrorMap.insert({ ErrorBeforeSectionDataRead, "ErrorBeforeSectionDataRead" });
        mErrorMap.insert({ ErrorSectionDataRead, "ErrorSectionDataRead" });
    }
};