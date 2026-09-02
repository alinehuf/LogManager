#pragma once

#include "CoreMinimal.h"
#include "BufferToJsonWorker.h"
#include "LogManagerConstants.h"
#include "SPSCBufferQueue.h"

class LogManager
{
public:
    bool Init(const LogManagerConstants::LogBufferConfig& Config);
    ~LogManager();

    long long GetNextID(long long OriginalId);

    void RequestShutdown();
    bool IsShutdownFinished() const;

    bool OpenNewJsonFile(const FString& filepath);
    bool CloseJSonFile();

    bool NewJSonEvent(const char* e, float gameTime, signed long long unixTimeSeconds, int unixTimeMilliseconds, signed long long frameCount, unsigned int nbDatas);
    bool NewJsonConfigData(unsigned int nbDatas);

    bool AddStringData(const char* key, const char* value);
    bool AddIntData(const char* key, int value);
    bool AddFloatData(const char* key, float value);
    bool AddBoolData(const char* key, bool value);
    bool AddUIntData(const char* key, unsigned int value);
    bool AddLongLongData(const char* key, signed long long value);
    bool AddULongLongData(const char* key, unsigned long long value);
    bool AddComposedData(const char* key, unsigned int nbSubPairs);

private:
	// helper function
    bool SafeSnprintfAppend(const char* what, const char* format, ...);

	// worker and thread (FRunnable and FRunnableThread)
    TUniquePtr<BufferToJsonWorker> worker;
    TUniquePtr<FRunnableThread> thread;

    // buffers
	TUniquePtr<BufferPool> bufferPool;       // pre-allocated buffers
    TUniquePtr<SPSCBufferQueue> buffersFree; // for availlable pre-allocated buffers
    TUniquePtr<SPSCBufferQueue> buffersFull; // for filled buffers
	LogBuffer* currentBuffer = nullptr; // buffer currently being written to
};