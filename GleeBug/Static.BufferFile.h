#ifndef STATIC_BUFFERFILE_H
#define STATIC_BUFFERFILE_H

#include "Static.File.h"

namespace GleeBug
{
    class BufferFile : public File
    {
    public:
        BufferFile(void* data, uint32 size);
        ~BufferFile();

        /**
        \brief Opens an existing file.
        \return true if the file was opened successfully, false otherwise.
        */
        bool Open() override;

        /**
        \brief Creates a new file.
        \param overwrite (Optional) true to overwrite, false to preserve fail if the file already exists.
        \return true if the file was created, false otherwise.
        */
        bool Create(bool overwrite = true) override;

        /**
        \brief Check if there is an open/created file.
        */
        bool IsOpen() const override;

        /**
        \brief Closes the file.
        */
        void Close() override;

        /**
        \brief Gets the size of the file.
        */
        uint32 GetSize() const override;

        /**
        \brief Reads from the open file.
        \param offset The offset to start reading from.
        \param [out] data Destination buffer.
        \param size The size to read.
        \param [out] bytesRead (Optional) If set, returns the number of bytes read (even on failure).
        \return true if the read was fully successful, false otherwise.
        */
        bool Read(uint32 offset, void* data, uint32 size, uint32* bytesRead = nullptr) const override;

        /**
        \brief Writes to the open file.
        \param offset The offset to start writing to. Everything after this offset will be truncated!
        \param data The data to write.
        \param size The size to write.
        \param [out] bytesWritten (Optional) If set, returns the number of bytes written (even on failure)
        \return true if the write was fully successful, false otherwise.
        */
        bool Write(uint32 offset, const void* data, uint32 size, uint32* bytesWritten = nullptr) override;

    private:
        void* mData;
        uint32 mSize;
    };
};

#endif //STATIC_BUFFERFILE_H