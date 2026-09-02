#pragma once
#include "CoreMinimal.h"

class FArchive;

/**
 * High-throughput text file writer.
 *
 * The class maintains an internal output buffer and only writes to the
 * underlying file when that buffer becomes full, or when Flush()/Close()
 * is called.
 *
 * The writer is NOT thread-safe.
 * It is intended to be owned and used exclusively by the worker thread.
 */
class FJsonFileWriter
{
public:

    explicit FJsonFileWriter(uint32 InBufferSize = 256 * 1024); // 256 KiB JSON output buffer
    ~FJsonFileWriter();

    // Non-copyable
    FJsonFileWriter(const FJsonFileWriter&) = delete;
    FJsonFileWriter& operator=(const FJsonFileWriter&) = delete;

    /**
     * Open a file. Existing file contents are replaced.
     */
    bool Open(const FString& FilePath);

    /**
     * Flush pending data and close the file.
     */
    void Close();

    /**
     * Flush the internal output buffer to disk.
     *
     * Note that this asks the archive to flush its data;
     * it does not necessarily mean a physical disk cache flush.
     */
    bool Flush();

    bool IsOpen() const;

    /**
     * Append raw bytes to the internal output buffer.
     */
    bool Write(const char* Data, uint32 Length);

    /**
     * Append a null-terminated string.
     */
    bool Write(const char* Text);

    /**
     * Append formatted text using printf-style formatting.
     *
     * The normal path performs one vsnprintf() directly into the
     * output buffer and does not allocate memory.
     */
    bool WriteFormat(const char* Format, ...);

private:

    /**
     * Flush only the internal memory buffer.
     *
     * Does not call Archive->Flush().
     */
    bool FlushBuffer();

    /**
     * Underlying Unreal file archive.
     */
    TUniquePtr<FArchive> Archive;

    /**
     * Output buffer.
     */
    TArray<char> OutputBuffer;

    /**
     * Number of valid bytes currently stored in OutputBuffer.
     */
    uint32 OutputOffset = 0;
};

