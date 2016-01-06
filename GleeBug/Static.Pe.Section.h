#ifndef STATIC_PE_SECTION_H
#define STATIC_PE_SECTION_H

#include "Static.Region.h"

namespace GleeBug
{
    class Section : public Region < uint8 >
    {
    public:
        explicit Section()
            : Region()
        {
        }

        explicit Section(std::vector<uint8>* data, uint32 offset, uint32 size, PIMAGE_SECTION_HEADER header)
            : Region(data, offset, size),
            mHeader(header)
        {
        }

        PIMAGE_SECTION_HEADER GetHeader() { return mHeader; }
        uint32 GetVirtualAddress() { return mHeader->VirtualAddress; }
        uint32 GetVirtualSize() { return mHeader->Misc.VirtualSize; }
        uint32 GetRawAddress() { return mHeader->PointerToRawData; }
        uint32 GetRawSize() { return mHeader->SizeOfRawData; }

    private:
        PIMAGE_SECTION_HEADER mHeader;
    };
};

#endif //STATIC_PE_SECTION_H