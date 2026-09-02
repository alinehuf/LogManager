#pragma once

#include "CoreMinimal.h"
#include "BufferPool.h"
#include <atomic>

/* Single Producer, Single Consumer Buffer Queue for filled buffers */
class SPSCBufferQueue
{
public:
    void Init(int32 InCapacity);
    bool Enqueue(LogBuffer* Item);
    bool Dequeue(LogBuffer*& OutItem);

    bool IsEmpty() const;
    bool IsFull() const;

private:
    TArray<LogBuffer*> bufferQueue;
    int32 capacity = 64;
    std::atomic<uint32> head {0}; // write index
    std::atomic<uint32> tail {0}; // read index
};