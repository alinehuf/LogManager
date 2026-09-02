#pragma once

#include "CoreMinimal.h"

/** Declares the log category used throughout the LogManager plugin. */
DECLARE_LOG_CATEGORY_EXTERN(LogManagerMsg, Log, All);

struct LogBuffer
{
    char* data = nullptr;
    uint32 size = 0;
    uint32 offset = 0;
}; 
static enum LogType { T_NUMBER = 1, T_STRING = 2, T_BOOLEAN = 3, T_COMPOSED = 4, T_EVENT = 5, T_CONFIG = 6, T_NEWLOGFILE = 7, T_CLOSELOGFILE = 8 };

/* BufferPool class manages a pool of pre-allocated LogBuffer objects for efficient reuse. */
class BufferPool
{
public:
    bool Init(uint32 numBuffers, uint32 bufferSize);
    uint32 GetNumBuffers() const;
    LogBuffer* GetBuffer(uint32 index);
    ~BufferPool();

private:
    char* bigMemoryBlock = nullptr;
    TArray<LogBuffer> buffers;
};