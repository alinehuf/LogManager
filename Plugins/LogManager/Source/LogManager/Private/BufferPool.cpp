#include "BufferPool.h"

DEFINE_LOG_CATEGORY(LogManagerMsg);

/* BufferPool class manages a pool of pre-allocated LogBuffer objects for efficient reuse. */
bool BufferPool::Init(uint32 numBuffers, uint32 bufferSize)
{
	// Should not be called more than once, but if it is, free the previous memory block to avoid leaks
    if (bigMemoryBlock)
    {
        UE_LOG(LogManagerMsg, Error, TEXT("Init called again in BufferPool!"));
        return false;
    }
    // allocate a single large memory block for all buffers to improve cache locality and reduce fragmentation
    bigMemoryBlock = static_cast<char*>(FMemory::Malloc(bufferSize * numBuffers));
    checkf(bigMemoryBlock, TEXT("Unable to allocate log buffer (%llu bytes)"), bufferSize * numBuffers);
    // cut bigMemoryBlock into pieces and add to free queue 
    buffers.SetNum(numBuffers); // create empty LogBuffer objects
    for (uint32 i = 0; i < numBuffers; ++i)
    {
        buffers[i].data = bigMemoryBlock + i * bufferSize;
        buffers[i].size = bufferSize;
    }
    return true;
}

uint32 BufferPool::GetNumBuffers() const {
    return buffers.Num(); 
}

LogBuffer* BufferPool::GetBuffer(uint32 index) {
    check(index < (uint32) buffers.Num());
    return &buffers[index];
}

BufferPool::~BufferPool()
{
    if (bigMemoryBlock)
    {
        FMemory::Free(bigMemoryBlock);
        bigMemoryBlock = nullptr;
    }
}