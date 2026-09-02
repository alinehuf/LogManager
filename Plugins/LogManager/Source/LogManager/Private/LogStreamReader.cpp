#include "LogStreamReader.h"
#include "LogManager.h"

LogStreamReader::LogStreamReader(SPSCBufferQueue& inBuffersFree, SPSCBufferQueue& inBuffersFull) : buffersFree(inBuffersFree), buffersFull(inBuffersFull)
{
}

LogStreamReader::~LogStreamReader()
{
    if (currentBuffer)
    {
        checkf(readHead >= currentBuffer->offset, TEXT("LogStreamReader destroyed with unread data"));
        ReleaseCurrentBuffer();
    }
}

bool LogStreamReader::ReadByte(uint8& outValue)
{
    if (!EnsureBuffer()) return false;
    outValue = static_cast<uint8>(currentBuffer->data[readHead++]);
    return true;
}

bool LogStreamReader::ReadString(const char*& outString)
{
    if (!EnsureBuffer()) return false;

    const uint32 remaining = currentBuffer->offset - readHead;
    const char* stringStart = currentBuffer->data + readHead;
    const size_t length = strnlen(stringStart, remaining);
    if (length >= remaining)
    {
        UE_LOG(LogManagerMsg, Error, TEXT("LogStreamReader: string is not null-terminated inside current buffer"));
        return false;
    }
    outString = stringStart;
    readHead += static_cast<uint32>(length) + 1;
    return true;
}

bool LogStreamReader::HasData() const
{
    return currentBuffer && readHead < currentBuffer->offset;
}

bool LogStreamReader::IsEmpty() const
{
    return currentBuffer == nullptr && buffersFull.IsEmpty();
}

void LogStreamReader::Rewind(uint32 nbBytes)
{
	readHead -= nbBytes;
    if (readHead < 0)
    {
        UE_LOG(LogManagerMsg, Error, TEXT("LogStreamReader: Rewind called with nbBytes=%d larger than current read head=%d"), nbBytes, readHead);
        readHead = 0;
	}
}

bool LogStreamReader::EnsureBuffer()
{
	// If we have a current buffer and the read head is still within its bounds, we can continue reading from it.
    if (currentBuffer && readHead < currentBuffer->offset)
        return true;
	// The current buffer is exhausted, we need to release it and acquire the next one.
    if (currentBuffer)
        ReleaseCurrentBuffer();
	// Try to acquire the next buffer from the full queue. If there are no more buffers, return false.
    return AcquireNextBuffer();
}

bool LogStreamReader::AcquireNextBuffer()
{
    readHead = 0;
    if (!buffersFull.Dequeue(currentBuffer))
    {
        currentBuffer = nullptr;
        return false;
    }
    return true;
}

void LogStreamReader::ReleaseCurrentBuffer()
{
    check(currentBuffer != nullptr);
    check(readHead >= currentBuffer->offset);

    currentBuffer->offset = 0;
    if (!buffersFree.Enqueue(currentBuffer))
        UE_LOG(LogManagerMsg, Error, TEXT("LogStreamReader: failed to return processed buffer to free queue"));
    
    currentBuffer = nullptr;
    readHead = 0;
}