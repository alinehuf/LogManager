#pragma once

#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"

#include "SPSCBufferQueue.h"
#include "LogStreamReader.h"
#include "JsonFileWriter.h"

class BufferToJsonWorker : public FRunnable
{
public:
    explicit BufferToJsonWorker(SPSCBufferQueue& inBuffersFree, SPSCBufferQueue& inBuffersFull, uint32 inCacheSize);
    virtual ~BufferToJsonWorker() override = default;

    virtual uint32 Run() override;
 
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
};