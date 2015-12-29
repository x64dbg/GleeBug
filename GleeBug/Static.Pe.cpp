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
        mFileSize = 0;
        mData.clear();
        mOffset = 0;
        mDosHeader = Region<IMAGE_DOS_HEADER>();
        mAfterDosData = Region<uint8>();
        mNtHeaders32 = Region<IMAGE_NT_HEADERS32>();
        mNtHeaders64 = Region<IMAGE_NT_HEADERS64>();
        mSectionHeaders = Region<IMAGE_SECTION_HEADER>();
    }

    Pe::Error Pe::ParseHeaders()
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

        //TODO: special case where DOS and PE header overlap (tinygui.exe)
        if (uint32(newOffset) < mOffset)
            return ErrorDosHeaderNtHeaderOffsetOverlap;

        //read & verify the data between the DOS header and the NT headers
        auto afterDosCount = newOffset - mOffset;
        mAfterDosData = readRegion<uint8>(afterDosCount);
        if (!mAfterDosData)
            return ErrorAfterDosHeaderData;

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
            if (!ioh)
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

        //check the SizeOfOptionalHeader field
        auto sizeOfIoh = ifh->SizeOfOptionalHeader;
        if (ifh->NumberOfSections && sizeOfIoh < realSizeOfIoh) //TODO: this can be valid in certain circumstances (nullSOH-XP)
            return ErrorNtFileHeaderSizeOfOptionalHeaderOverlap;

        //read data after the optional header (TODO: check if this is even possible)
        uint32 afterOptionalCount = sizeOfIoh > realSizeOfIoh ? sizeOfIoh - realSizeOfIoh : 0;
        mAfterOptionalData = readRegion<uint8>(afterOptionalCount);

        //read the section headers
        auto sectionCount = ifh->NumberOfSections;
        mSectionHeaders = readRegion<IMAGE_SECTION_HEADER>(sectionCount);

        return ErrorOk;
    }

    bool Pe::IsValidPe() const
    {
        return mSectionHeaders.Valid();
    }

    bool Pe::IsPe64() const
    {
        return IsValidPe() ? mNtHeaders64.Valid() : false;
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
    }
};