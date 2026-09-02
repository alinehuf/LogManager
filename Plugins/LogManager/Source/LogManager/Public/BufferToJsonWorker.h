#pragma once

#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/Event.h"
#include "SPSCBufferQueue.h"
#include "LogStreamReader.h"
#include "JsonFileWriter.h"

class BufferToJsonWorker : public FRunnable
{
public:
    explicit BufferToJsonWorker(SPSCBufferQueue& inBuffersFree, SPSCBufferQueue& inBuffersFull, uint32 inCacheSize);
    virtual ~BufferToJsonWorker() override;

    virtual uint32 Run() override;

	void NotifySomethingToDo(); // Wake up the worker thread if it is waiting for work (a buffer in BuffersFull or a shutdown request).
 
	void RequestShutdown(); // Request a clean shutdown : The worker will continue processing every buffer already present in BuffersFull before terminating.
	bool IsFinished() const; // Returns true if shutdown is requested and the worker has finished processing all buffers.
private:
    bool WriteBufferToJSon();
    bool WriteData();
    //const char* ReadString(uint32* ReadHead);

private:

    static constexpr float WORKER_TICK_SECONDS = 0.01f;

    FThreadSafeBool bShutdownRequested; // Set by the manager thread : The worker keeps processing buffers until BuffersFull is empty.
    FThreadSafeBool bFinished; // Set by the worker itself immediately before Run() returns.

    SPSCBufferQueue& buffersFree; // SPSC queue : Worker -> Game/producer
    SPSCBufferQueue& buffersFull; // SPSC queue : Game/producer -> Worker
    
    LogStreamReader reader;
    FJsonFileWriter jsonWriter; // JSON file writer. Used exclusively by this worker thread.

	FEvent* somethingToDoEvent = nullptr; // Event to signal the worker thread that there is something to do (a buffer has been enqueued in BuffersFull or shutdown is requested). This event is set by the producer thread and waited on by the worker thread.
};