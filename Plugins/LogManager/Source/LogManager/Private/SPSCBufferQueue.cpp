#include "SPSCBufferQueue.h"

/* Single Producer, Single Consumer Buffer Queue for filled buffers */
void SPSCBufferQueue::Init(int32 InCapacity)
{
    capacity = InCapacity;
	bufferQueue.SetNum(capacity + 1); // keep one slot empty to distinguish between full and empty states
    head.store(0);
    tail.store(0);
}

bool SPSCBufferQueue::Enqueue(LogBuffer* Item) {
	const uint32 currentHead = head.load(std::memory_order_relaxed); // Load the current head index without synchronization
	const uint32 currentTail = tail.load(std::memory_order_acquire); // Ensure we see the latest tail value
    const uint32 nextHead = (currentHead + 1) % (capacity+1);
    if (nextHead == currentTail)
        return false; // queue full
    bufferQueue[currentHead] = Item;
    head.store(nextHead, std::memory_order_release); // Ensure the new head value is visible to the consumer
    return true;
}

bool SPSCBufferQueue::Dequeue(LogBuffer*& OutItem) {
    const uint32 currentTail = tail.load(std::memory_order_relaxed);
    const uint32 currentHead = head.load(std::memory_order_acquire);
    if (currentTail == currentHead)
        return false; // queue empty
    OutItem = bufferQueue[currentTail];
    const uint32 nextTail = (currentTail + 1) % (capacity+1);
    tail.store(nextTail, std::memory_order_release);
    return true;
}

bool SPSCBufferQueue::IsEmpty() const {
    const uint32 currentTail = tail.load(std::memory_order_relaxed);
    const uint32 currentHead = head.load(std::memory_order_acquire);
	return currentTail == currentHead;
}
bool  SPSCBufferQueue::IsFull() const {
    const uint32 currentHead = head.load(std::memory_order_relaxed);
    const uint32 currentTail = tail.load(std::memory_order_acquire);
    const uint32 nextHead = (currentHead + 1) % (capacity+1);
	return nextHead == currentTail;
}