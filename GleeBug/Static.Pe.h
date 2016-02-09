#ifndef STATIC_PE_H
#define STATIC_PE_H

#include "Static.File.h"
#include "Static.Region.h"
#include "Static.Pe.Section.h"

namespace GleeBug
{
    class Pe
    {
    public:
        enum Error
        {
            ErrorOk,
            ErrorDosHeaderRead,
            ErrorDosHeaderMagic,
            ErrorDosHeaderNtHeaderOffset,
            ErrorDosHeaderNtHeaderOffsetOverlap,
            ErrorAfterDosHeaderData,
            ErrorNtSignatureRead,
            ErrorNtSignatureMagic,
            ErrorNtFileHeaderRead,
            ErrorNtFileHeaderSizeOfOptionalHeaderOverlap,
            ErrorNtFileHeaderUnsupportedMachine,
            ErrorNtFileHeaderUnsupportedMachineOptionalHeaderRead,
            ErrorNtFileHeaderUnsupportedMachineNtHeadersRegionSize,
            ErrorNtOptionalHeaderRead,
            ErrorNtOptionalHeaderMagic,
            ErrorAfterOptionalHeaderDataRead,
            ErrorNtHeadersRegionSize,
            ErrorSectionHeadersRead,
            ErrorAfterSectionHeadersDataRead,
            ErrorBeforeSectionDataRead,
            ErrorSectionDataRead
        };

        const int HeaderSection = -1;

        typedef std::map<Range, int, RangeCompare> RangeSectionMap;

        explicit Pe(File & file);

        void Clear();
        const char* ErrorText(Error error) const;
        bool IsValidPe() const;
        bool IsPe64() const;
        Error Parse(bool allowOverlap = false);
        uint32 ConvertOffsetToRva(uint32 offset);
        uint32 ConvertRvaToOffset(uint32 rva);

        const Region<IMAGE_DOS_HEADER> & GetDosHeader() const { return mDosHeader; }
        bool GetDosNtOverlap() const { return mDosNtOverlap; }
        const Region<uint8> & GetAfterDosData() const { return mAfterDosData; }
        const Region<IMAGE_NT_HEADERS32> & GetNtHeaders32() const { return mNtHeaders32; }
        const Region<IMAGE_NT_HEADERS64> & GetNtHeaders64() const { return mNtHeaders64; }
        const Region<uint8> & GetAfterOptionalData() const { return mAfterOptionalData; }
        const Region<IMAGE_SECTION_HEADER> & GetSectionHeaders() const { return mSectionHeaders; }
        const Region<uint8> & GetAfterSectionHeadersData() const { return mAfterSectionHeadersData; }
        const std::vector<Section> & GetSections() const { return mSections; }
        const RangeSectionMap & GetOffsetSectionMap() const { return mOffsetSectionMap; }
        const RangeSectionMap & GetRvaSectionMap() const { return mRvaSectionMap; }

    private:
        Error parseSections(uint16 count, uint32 alignment = 0x1000);
        uint32 readData(uint32 size);
        void setupErrorMap();

        template<typename T>
        T alignAdjustSize(T size, uint32 alignment) //TODO: check this
        {
            return size + (alignment - 1) & ~(alignment - 1);
        }

        template<typename T>
        T alignAdjustAddress(T address, uint32 alignment) //TODO: check this
        {
            return address & ~(alignment - 1);
        }

        template<typename T>
        Region<T> readRegion(uint32 count = 1)
        {
            return Region<T>(&mData, readData(sizeof(T) * count), count);
        }

        std::unordered_map<Error, const char*> mErrorMap;

        File & mFile;
        std::vector<uint8> mData;
        uint32 mOffset;
        bool mIsPe64;

        Region<IMAGE_DOS_HEADER> mDosHeader;
        bool mDosNtOverlap;
        Region<uint8> mAfterDosData;
        Region<IMAGE_NT_HEADERS32> mNtHeaders32;
        Region<IMAGE_NT_HEADERS64> mNtHeaders64;
        Region<uint8> mAfterOptionalData;
        Region<IMAGE_SECTION_HEADER> mSectionHeaders;
        Region<uint8> mAfterSectionHeadersData;
        std::vector<Section> mSections;
        RangeSectionMap mOffsetSectionMap;
        RangeSectionMap mRvaSectionMap;
    };
};

#endif //STATIC_PE_H