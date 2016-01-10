#include "Static.Pattern.h"

namespace GleeBug
{
    std::string Pattern::FormatPattern(const std::string & patterntext)
    {
        std::string result;
        result.reserve(patterntext.length());
        for (auto ch : patterntext)
        {
            if (ch >= '0' && ch <= '9' || ch >= 'A' && ch <= 'F' || ch >= 'a' && ch <= 'f' || ch == '?')
                result += ch;
        }
        return result;
    }

    std::vector<Pattern::Byte> Pattern::Transform(const std::string & patterntext)
    {
        std::vector<Byte> pattern;
        auto formattext = FormatPattern(patterntext);
        auto len = formattext.length();
        if (!len)
            return pattern;

        if (len % 2) //not a multiple of 2
        {
            formattext += '?';
            len++;
        }

        pattern.reserve(len / 2);

        auto hexChToInt = [](char ch)
        {
            if (ch >= '0' && ch <= '9')
                return ch - '0';
            if (ch >= 'A' && ch <= 'F')
                return ch - 'A' + 10;
            if (ch >= 'a' && ch <= 'f')
                return ch - 'a' + 10;
            return -1;
        };

        Byte newByte;
        auto j = 0;
        for (auto ch : formattext)
        {
            if (ch == '?') //wildcard
            {
                newByte.nibble[j].wildcard = true; //match anything
            }
            else //hex
            {
                newByte.nibble[j].wildcard = false;
                newByte.nibble[j].data = hexChToInt(ch) & 0xF;
            }

            j++;
            if (j == 2) //two nibbles = one byte
            {
                j = 0;
                pattern.push_back(newByte);
            }
        }
        return pattern;
    }

    size_t Pattern::Find(const uint8* data, size_t datasize, const std::vector<Byte> & pattern)
    {
        auto MatchByte = [](uint8 byte, const Byte & pbyte)
        {
            auto matched = 0;

            unsigned char n1 = (byte >> 4) & 0xF;
            if (pbyte.nibble[0].wildcard)
                matched++;
            else if (pbyte.nibble[0].data == n1)
                matched++;

            unsigned char n2 = byte & 0xF;
            if (pbyte.nibble[1].wildcard)
                matched++;
            else if (pbyte.nibble[1].data == n2)
                matched++;

            return matched == 2;
        };

        auto searchpatternsize = pattern.size();
        if (!searchpatternsize)
            return -1;
        for (size_t i = 0, pos = 0; i < datasize; i++)  //search for the pattern
        {
            if (MatchByte(data[i], pattern.at(pos)))  //check if our pattern matches the current byte
            {
                pos++;
                if (pos == searchpatternsize)  //everything matched
                    return i - searchpatternsize + 1;
            }
            else if (pos > 0)  //fix by Computer_Angel
            {
                i -= pos;
                pos = 0; //reset current pattern position
            }
        }
        return -1;
    }

    size_t Pattern::Find(const uint8* data, size_t datasize, const uint8* pattern, size_t patternsize)
    {
        if (!patternsize)
            return -1;
        if (patternsize > datasize)
            patternsize = datasize;
        for (size_t i = 0, pos = 0; i < datasize; i++)
        {
            if (data[i] == pattern[pos])
            {
                pos++;
                if (pos == patternsize)
                    return i - patternsize + 1;
            }
            else if (pos > 0)
            {
                i -= pos;
                pos = 0; //reset current pattern position
            }
        }
        return -1;
    }

    size_t Pattern::Find(const uint8* data, size_t datasize, const char* pattern)
    {
        return Find(data, datasize, Transform(pattern));
    }

    void Pattern::Write(uint8* data, size_t datasize, const char* pattern)
    {
        auto writepattern = Transform(pattern);
        if (!writepattern.size())
            return;

        auto writepatternsize = writepattern.size();
        if (writepatternsize > datasize)
            writepatternsize = datasize;

        auto WriteByte = [](uint8* byte, const Byte & pbyte)
        {
            unsigned char n1 = (*byte >> 4) & 0xF;
            unsigned char n2 = *byte & 0xF;
            if (!pbyte.nibble[0].wildcard)
                n1 = pbyte.nibble[0].data;
            if (!pbyte.nibble[1].wildcard)
                n2 = pbyte.nibble[1].data;
            *byte = ((n1 << 4) & 0xF0) | (n2 & 0xF);
        };

        for (size_t i = 0; i < writepatternsize; i++)
            WriteByte(&data[i], writepattern.at(i));
    }

    bool Pattern::SearchAndReplace(uint8* data, size_t datasize, const char* searchpattern, const char* replacepattern)
    {
        auto found = Find(data, datasize, searchpattern);
        if (found == -1)
            return false;
        Write(data + found, datasize - found, replacepattern);
        return true;
    }
};