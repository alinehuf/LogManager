#include "LogManager.h"

bool LogManager::Init(const LogManagerConstants::LogBufferConfig& Config)
{
    bufferPool = MakeUnique<BufferPool>();
    // Initialize the buffer pool with bufferCount buffers of size bufferSize
    if (!bufferPool->Init(Config.bufferSize, Config.bufferCount)) {
        bufferPool.Reset();
        UE_LOG(LogManagerMsg, Error, TEXT("Failed to create BufferPool during initialization"));
        return false;
    }
    // Initialize the single-producer, single-consumer buffer queues with a capacity of queueCapacity
    buffersFree->Init(Config.queueCapacity); 
    buffersFull->Init(Config.queueCapacity);
    // place buffers in the free queue
    for (uint32 i = 0; i < bufferPool->GetNumBuffers(); ++i) {
        LogBuffer* buffer = bufferPool->GetBuffer(i);
        if (!buffersFree->Enqueue(buffer)) {
            UE_LOG(LogManagerMsg, Error, TEXT("Failed to enqueue buffer %d into free queue during initialization"), i);
            return false;
        }
    }
    // create the worker thread that will handle writing buffers to JSON files
    worker = MakeUnique<BufferToJsonWorker>(*buffersFree, *buffersFull, Config.writeCacheSize);
    thread.Reset(FRunnableThread::Create(worker.Get(), TEXT("BufferToJsonWorker")));
    return true;
}

long long LogManager::GetNextID(long long OriginalId)
{
    unsigned long long  z = static_cast<unsigned long long>(OriginalId);

    z += 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z = z ^ (z >> 31);

    return static_cast<long long>(z);
}

bool LogManager::SafeSnprintfAppend(const char* what, const char* format, ...)
{
    if (!format)
    {
        UE_LOG(LogManagerMsg, Error, TEXT("SafeSnprintfAppend: invalid format when writing %s"), *FString(what));
        return false;
    }
    if (!currentBuffer && !buffersFree->Dequeue(currentBuffer))
    {
        UE_LOG(LogManagerMsg, Error, TEXT("SafeSnprintfAppend: no free buffer available"));
        return false;
    }
    
    va_list args;
    va_start(args, format);
    va_list argsRetry;
    va_copy(argsRetry, args);
    // Try to write into the buffer
    int written = vsnprintf(currentBuffer->data + currentBuffer->offset, currentBuffer->size - currentBuffer->offset, format, args);
    va_end(args);
    // Check for encoding error
    if (written < 0)
    {
        UE_LOG(LogManagerMsg, Error, TEXT("SafeSnprintfAppend: encoding error when writing %s"), *FString(what));
        va_end(argsRetry);
        return false;
    }
    // Check if the written data fits in the current buffer
    if (currentBuffer->offset + written + 1 > currentBuffer->size)
    {
        // Not enough space in the current buffer, we need to send it to the worker and get a new one
        if (currentBuffer->offset > 0)
        {
            if (!buffersFull->Enqueue(currentBuffer))
            {
                va_end(argsRetry);
                UE_LOG(LogManagerMsg, Error, TEXT("SafeSnprintfAppend: can't enqueue full buffer"));
                return false;
            }
        }
        // Get a new buffer from the free queue
        if (!buffersFree->Dequeue(currentBuffer))
        {
            va_end(argsRetry);
            UE_LOG(LogManagerMsg, Error, TEXT("SafeSnprintfAppend: no free buffer available"));
            return false;
        }
        currentBuffer->offset = 0; // Reset the offset for the new buffer
        // An empty buffer should be large enough to hold the data, if not, it's an error
        if (uint32(written + 1) > currentBuffer->size)
        {
            va_end(argsRetry);
            UE_LOG(LogManagerMsg, Error, TEXT("SafeSnprintfAppend: Fatal Error ! data is larger than an entire buffer when writing %s. Increase the buffer size!"), *FString(what));
            return false;
        }
        // Retry writing into the new buffer
        written = vsnprintf(currentBuffer->data + currentBuffer->offset, currentBuffer->size - currentBuffer->offset, format, argsRetry);
        va_end(argsRetry);

        if (written < 0)
        {
            UE_LOG(LogManagerMsg, Error, TEXT("SafeSnprintfAppend: encoding error when writing %s"), *FString(what));
            return false;
        }
    }
    currentBuffer->offset += static_cast<uint32>(written) + 1;
    return true;
}

bool LogManager::NewJSonEvent(const char* e,
    float gameTime,
    signed long long unixTimeSeconds,
    int unixTimeMilliseconds,
    signed long long frameCount,
    unsigned int nbDatas) {
    if (!SafeSnprintfAppend("event", "%c%s", T_EVENT, e))
        return false;
    if (!SafeSnprintfAppend("gameTime", "%f", gameTime))
        return false;
    if (!SafeSnprintfAppend("unixTimeSeconds", "%lld", unixTimeSeconds))
        return false;
    if (!SafeSnprintfAppend("unixTimeMilliseconds", "%d", unixTimeMilliseconds))
        return false;
    if (!SafeSnprintfAppend("frameCount", "%lld", frameCount))
        return false;
    if (!SafeSnprintfAppend("nbDatas", "%c%cdata", T_COMPOSED, (char)nbDatas))
        return false;
    return true;
}

bool LogManager::NewJsonConfigData(unsigned int nbDatas) {
    if (nbDatas > 255) {
        UE_LOG(LogManagerMsg, Error, TEXT("NewJsonConfigData: nbDatas (%d) exceeds the maximum allowed value of 255"), nbDatas);
        return false;
	}
    if (!SafeSnprintfAppend("config", "%c%c", T_CONFIG, (char)nbDatas))
        return false;
    return true;
}

bool LogManager::OpenNewJsonFile(const FString& filepath) {
    FTCHARToUTF8 utf8path(*filepath);
    if (!SafeSnprintfAppend("newFile", "%c%s", T_NEWLOGFILE, utf8path.Get()))
        return false;
    return true;
}

bool LogManager::CloseJSonFile()
{
    if (!SafeSnprintfAppend("closeFile", "%c", T_CLOSELOGFILE))
        return false;
    return true;
}


bool LogManager::AddStringData(const char* key, const char* value) {
    if (!SafeSnprintfAppend("addStringData", "%c%s%c%s", T_STRING, key, 0, value))
        return false;
    return true;
}

bool LogManager::AddIntData(const char* key, int value) {
    if (!SafeSnprintfAppend("addIntData", "%c%s%c%d", T_NUMBER, key, 0, value))
        return false;
    return true;
}

bool LogManager::AddFloatData(const char* key, float value) {
    if (!SafeSnprintfAppend("addFloatData", "%c%s%c%f", T_NUMBER, key, 0, value))
        return false;
    return true;
}

bool LogManager::AddBoolData(const char* key, bool value) {
    if (!SafeSnprintfAppend("addBoolData", "%c%s%c%s", T_BOOLEAN, key, 0, (value ? "true" : "false")))
        return false;
    return true;
}

bool LogManager::AddUIntData(const char* key, unsigned int value) {
    if (!SafeSnprintfAppend("addUIntData", "%c%s%c%u", T_NUMBER, key, 0, value))
        return false;
    return true;
}

bool LogManager::AddLongLongData(const char* key, signed long long value) {
    if (!SafeSnprintfAppend("addLongLongData", "%c%s%c%lld", T_NUMBER, key, 0, value))
        return false;
    return true;
}

bool LogManager::AddULongLongData(const char* key, unsigned long long value) {
    if (!SafeSnprintfAppend("addULongLongData", "%c%s%c%llu", T_NUMBER, key, 0, value))
        return false;
    return true;
}

bool LogManager::AddComposedData(const char* key, unsigned int nbSubPairs) {
    if (!SafeSnprintfAppend("addComposedData", "%c%c%s", T_COMPOSED, nbSubPairs, key))
        return false;
    return true;
}

void LogManager::RequestShutdown() 
{
    // Wait for the worker thread to finish before destructing
    worker->RequestShutdown();
}

bool LogManager::IsShutdownFinished() const
{
    return !worker || worker->IsFinished();
}

LogManager::~LogManager() 
{   
    // Wait for the worker thread to finish
    if (worker)
    {
        worker->RequestShutdown();
    }
    if (thread)
    {
        thread->WaitForCompletion();
    }
    // Clean up the worker and thread objects
    thread.Reset();
    worker.Reset();
    // free buffers memory
    bufferPool.Reset();
}