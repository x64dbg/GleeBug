#ifndef STATIC_PATTERN_H
#define STATIC_PATTERN_H

#include "Static.Global.h"

namespace GleeBug
{
    class Pattern
    {
    public:
        struct Byte
        {
            struct Nibble
            {
                uint8 data;
                bool wildcard;
            } nibble[2];
        };

        static std::string FormatPattern(const std::string & pattern);
        static bool Transform(const std::string & patterntext, std::vector<Byte> & pattern);
        static size_t Find(const uint8* data, size_t datasize, const std::vector<Byte> & pattern);
        static size_t Find(const uint8* data, size_t datasize, unsigned char* pattern, size_t patternsize);
        static size_t Find(const uint8* data, size_t datasize, const char* pattern);
        static void Write(uint8* data, size_t datasize, const char* pattern);
        static bool Snr(uint8* data, size_t datasize, const char* searchpattern, const char* replacepattern);
    };
};

#endif //STATIC_PATTERN_H