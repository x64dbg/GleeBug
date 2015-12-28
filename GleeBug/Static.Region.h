#ifndef _STATIC_REGION_H
#define _STATIC_REGION_H

#include "Static.Global.h"

namespace GleeBug
{
    /**
    \brief An immutable region object. Used to indicate a region of data in a vector.
    \tparam T Type of the data in the region.
    */
    template<typename T>
    class Region
    {
    public:
        /**
        \brief Default constructor (constructs an invalid region).
        */
        explicit Region()
            : Region(nullptr, INVALID_VALUE, INVALID_VALUE)
        {
        }

        /**
        \brief Constructor (constructs a valid region).
        \param [in] data Pointer to the vector that holds the actual data. Use nullptr to create an invalid region.
        \param offset The offset. Use INVALID_VALUE to create an invalid region.
        \param count (Optional) Number of Ts in the region. Use INVALID_VALUE to create an invalid region.
        */
        explicit Region(std::vector<uint8>* data, uint32 offset, uint32 count = 1)
            : _data(data),
            _offset(offset),
            _count(count)
        {
        }

        /**
        \brief Returns a pointer inside the data to the start of this region.
        \return nullptr if the region is invalid, a pointer to the data otherwise.
        */
        T* Data() const
        {
            if (!Valid())
                return nullptr;
            return (T*)(_data->data() + _offset);
        }

        /**
        \brief Gets the offset in the data of this region.
        */
        uint32 Offset() const
        {
            return _offset;
        }

        /**
        \brief Gets the number of Ts in this region.
        */
        uint32 Count() const
        {
            return _count;
        }

        /**
        \brief Gets the size of this region in bytes. Basically: sizeof(T) * Count().
        */
        uint32 Size() const
        {
            return Valid() ? _count * sizeof(T) : INVALID_VALUE;
        }

        /**
        \brief Returns if this region is valid.
        */
        bool Valid() const
        {
            return _offset != INVALID_VALUE &&
                _count != INVALID_VALUE &&
                _data && _data->data() &&
                _offset + _count * sizeof(T) <= _data->size();
        }

        /**
        \brief Returns Valid().
        */
        operator bool() const
        {
            return Valid();
        }

        /**
        \brief Returns Data().
        */
        T* operator ()() const
        {
            return Data();
        }

    private:
        std::vector<uint8>* _data;
        uint32 _offset;
        uint32 _count;
    };
};

#endif //_STATIC_REGION_H