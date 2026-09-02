#pragma once

#include "SPSCBufferQueue.h"

class LogStreamReader
{
public:
    LogStreamReader(SPSCBufferQueue& inBuffersFree, SPSCBufferQueue& inBuffersFull);
    ~LogStreamReader();

    bool ReadByte(uint8& OutValue);
    bool ReadString(const char*& OutString);
	bool HasData() const; // true if there is data available to read in the current buffer
	bool IsEmpty() const; // nothing to read, no buffer available in the queue
    void Rewind(uint32 nbBytes);

private:
    bool EnsureBuffer();
    bool AcquireNextBuffer();
    void ReleaseCurrentBuffer();

private:
    SPSCBufferQueue& buffersFree;
    SPSCBufferQueue& buffersFull;

    LogBuffer* currentBuffer = nullptr;
    uint32 readHead = 0;
};