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

        /**
        \brief Formats pattern string to only contain [0-9A-Fa-f\?].
        \param pattern Pattern to format.
        \return The formatted pattern.
        */
        static std::string FormatPattern(const std::string & pattern);

        /**
        \brief Transforms a string pattern to a nibble structure.
        \param patterntext The pattern string to find.
        \param [out] pattern Transformed pattern.
        \return true if it succeeds, false otherwise.
        */
        static bool Transform(const std::string & patterntext, std::vector<Byte> & pattern);

        /**
        \brief Finds the first occurrence of a pattern in a buffer.
        \param data The buffer to search in.
        \param datasize The size of the buffer.
        \param pattern The pattern to find.
        \return Offset of the first occurrence found. -1 when not found.
        */
        static size_t Find(const uint8* data, size_t datasize, const std::vector<Byte> & pattern);

        /**
        \brief Finds the first occurrence of a pattern in a buffer.
        \param data The buffer to search in.
        \param datasize The size of the buffer.
        \param pattern The pattern to find. The pattern supports wildcards (1? ?? ?6 78).
        \return Offset of the first occurrence found. -1 when not found.
        */
        static size_t Find(const uint8* data, size_t datasize, const char* pattern);

        /**
        \brief Finds the first occurrence of a pattern in a buffer.
        \param data The buffer to search in.
        \param datasize The size of the buffer.
        \param pattern The pattern to find.
        \param patternsize The size of the pattern to find.
        \return Offset of the first occurrence found. -1 when not found.
        */
        static size_t Find(const uint8* data, size_t datasize, const uint8* pattern, size_t patternsize);

        /**
        \brief Writes a pattern in a buffer. This function writes as many bytes as possible from the pattern.
        \param [in,out] data The buffer to write the pattern in.
        \param datasize The size of the buffer.
        \param pattern Specifies the pattern. The pattern supports wildcards (1? ?? ?6 78).
        */
        static void Write(uint8* data, size_t datasize, const char* pattern);

        /**
        \brief Search and replace a pattern in a buffer.
        \param [in,out] data The buffer to search and replace in.
        \param datasize The size of the buffer.
        \param searchpattern The pattern to find. The pattern supports wildcards (1? ?? ?6 78).
        \param replacepattern The pattern to replace the found occurrence with. The pattern supports wildcards (1? ?? ?6 78).
        \return true if it succeeds, false if it fails.
        */
        static bool Snr(uint8* data, size_t datasize, const char* searchpattern, const char* replacepattern);
    };
};

#endif //STATIC_PATTERN_H