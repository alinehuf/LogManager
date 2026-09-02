#include "JsonFileWriter.h"

#include "Misc/FileHelper.h"
#include "Serialization/Archive.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>


FJsonFileWriter::FJsonFileWriter(uint32 InBufferSize)
{
    checkf(InBufferSize > 0, TEXT("FJsonFileWriter: buffer size must be greater than zero"));
    OutputBuffer.SetNumUninitialized(InBufferSize);
}


FJsonFileWriter::~FJsonFileWriter()
{
    Close();
}


bool FJsonFileWriter::Open(const FString& FilePath)
{
    Close();
    if (FilePath.IsEmpty())
        return false;
    Archive.Reset(IFileManager::Get().CreateFileWriter(*FilePath));
    if (!Archive)
        return false;
    OutputOffset = 0;
    return true;
}


void FJsonFileWriter::Close()
{
    if (!Archive)
        return;
    Flush();
    Archive.Reset();
    OutputOffset = 0;
}

bool FJsonFileWriter::IsOpen() const
{
    return Archive.IsValid();
}


bool FJsonFileWriter::FlushBuffer()
{
    if (!Archive)
        return false;
    if (OutputOffset == 0)
        return true;

    Archive->Serialize(OutputBuffer.GetData(), OutputOffset);
    if (Archive->IsError())
        return false;
    OutputOffset = 0;
    return true;
}


bool FJsonFileWriter::Flush()
{
    if (!Archive)
        return false;
    if (!FlushBuffer())
        return false;
    Archive->Flush();
    return !Archive->IsError();
}


bool FJsonFileWriter::Write(const char* Data, uint32 Length)
{
    if (!Archive || !Data || Length == 0)
        return false;

    const uint32 BufferSize = static_cast<uint32>(OutputBuffer.Num());

    // Large write:
    // if the data is larger than the complete output buffer,
    // flush what is already buffered and write directly to the archive.
    if (Length >= BufferSize)
    {
        if (!FlushBuffer())
            return false;
        Archive->Serialize(const_cast<char*>(Data), Length);
        return !Archive->IsError();
    }

    // Not enough room in the output buffer.
    if (OutputOffset + Length > BufferSize)
    {
        if (!FlushBuffer())
            return false;
    }
    FMemory::Memcpy(OutputBuffer.GetData() + OutputOffset, Data, Length);
    OutputOffset += Length;

    return true;
}


bool FJsonFileWriter::Write(const char* Text)
{
    if (!Text)
        return false;

    return Write(Text, static_cast<uint32>(FCStringAnsi::Strlen(Text)));
}


bool FJsonFileWriter::WriteFormat(const char* Format,...)
{
    if (!Archive || !Format)
        return false;
    const uint32 BufferSize = static_cast<uint32>(OutputBuffer.Num());

    /*
     * First attempt:
     *
     * Make a copy of the va_list because vsnprintf() consumes it.
     * The original Args remains available for a retry.
     */
    va_list Args;
    va_start(Args, Format);

    va_list ArgsCopy;
    va_copy(ArgsCopy, Args);

    const uint32 Remaining = BufferSize - OutputOffset;

    const int Written = vsnprintf(OutputBuffer.GetData() + OutputOffset, Remaining, Format, ArgsCopy);

    va_end(ArgsCopy);

    // Encoding / formatting error.
    if (Written < 0)
    {
        va_end(Args);
        return false;
    }

    /*
     * Written is the number of characters that WOULD have been written,
     * excluding the terminating '\0'.
     *
     * If Written < Remaining, everything fits in the output buffer.
     */
    if (static_cast<uint32>(Written) < Remaining) {
        OutputOffset += static_cast<uint32>(Written);
        va_end(Args);
        return true;
    }

    /*
     * The formatted string does not fit.
     *
     * The first vsnprintf() may have written a truncated string into
     * OutputBuffer, but OutputOffset has NOT changed, so those bytes
     * are still considered invalid.
     */

    if (!FlushBuffer()) {
        va_end(Args);
        return false;
    }

    /*
     * Try again in the now-empty output buffer.
     *
     * If it still doesn't fit, we need a temporary buffer.
     */
    const uint32 EmptyBufferSize = BufferSize;

    va_list ArgsRetry;
    va_copy(ArgsRetry, Args);

    const int RequiredAgain = vsnprintf(OutputBuffer.GetData(), EmptyBufferSize, Format, ArgsRetry);

    va_end(ArgsRetry);

    if (RequiredAgain < 0)
    {
        va_end(Args);
        return false;
    }

    if (static_cast<uint32>(RequiredAgain) < EmptyBufferSize)
    {
        OutputOffset = static_cast<uint32>(RequiredAgain);
        va_end(Args);
        return true;
    }

    /*
     * The formatted string is larger than the complete output buffer.
     *
     * Allocate a temporary buffer only for this exceptional case.
     */
    TArray<char> LargeBuffer;

    LargeBuffer.SetNumUninitialized(RequiredAgain + 1);

    va_list ArgsLarge;
    va_copy(ArgsLarge, Args);

    const int WrittenLarge = vsnprintf(LargeBuffer.GetData(), RequiredAgain + 1, Format, ArgsLarge);

    va_end(ArgsLarge);
    va_end(Args);

    if (WrittenLarge < 0)
        return false;

    return Write(LargeBuffer.GetData(), static_cast<uint32>(WrittenLarge));
}
