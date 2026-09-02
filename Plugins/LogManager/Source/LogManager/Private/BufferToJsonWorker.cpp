#include "BufferToJsonWorker.h"
#include "HAL/PlatformProcess.h"
#include "LogManager.h"

BufferToJsonWorker::BufferToJsonWorker(SPSCBufferQueue& inBuffersFree, SPSCBufferQueue& inBuffersFull, uint32 inCacheSize) : buffersFree(inBuffersFree), buffersFull(inBuffersFull), reader(inBuffersFree, inBuffersFull), jsonWriter(inCacheSize)
{
    // For a clean shutdown (wait end of worker thread)
    bShutdownRequested = false;
    bFinished = false;
    somethingToDoEvent = FPlatformProcess::GetSynchEventFromPool(false);
}

BufferToJsonWorker::~BufferToJsonWorker()
{
    if (somethingToDoEvent)
    {
        FPlatformProcess::ReturnSynchEventToPool(somethingToDoEvent);
        somethingToDoEvent = nullptr;
    }
}



uint32 BufferToJsonWorker::Run()
{
    UE_LOG(LogManagerMsg, Display, TEXT("BufferToJsonWorker: thread started"));

    while (true)
    {
        while (!reader.IsEmpty())
        {
            WriteBufferToJSon();
        }

        if (bShutdownRequested)
            break;

        somethingToDoEvent->Wait();
    }

    if (jsonWriter.IsOpen())
    {
        jsonWriter.Write("]\n");
        jsonWriter.Close();
    }

    UE_LOG(LogManagerMsg, Display, TEXT("BufferToJsonWorker: all buffers written, terminating"));

    bFinished = true;
    return 0;
}

void BufferToJsonWorker::NotifySomethingToDo()
{
    somethingToDoEvent->Trigger();
}

void BufferToJsonWorker::RequestShutdown()
{
    bShutdownRequested = true;
    somethingToDoEvent->Trigger();
    UE_LOG(LogManagerMsg, Display, TEXT("BufferToJsonWorker: shutdown requested"));
}

bool BufferToJsonWorker::IsFinished() const
{
    return bFinished;
}

bool BufferToJsonWorker::WriteBufferToJSon()
{
    uint8 type;

    while (reader.ReadByte(type))
    {
        if (type == T_NEWLOGFILE)
        {
            const char* filePath;
            if (!reader.ReadString(filePath))
                return false;
            UE_LOG(LogManagerMsg, Display, TEXT("WriteBufferToJSon: open file %s"), *FString(filePath));
            if (!jsonWriter.Open(FString(filePath)))
            {
                UE_LOG(LogManagerMsg, Error, TEXT("WriteBufferToJSon: failed to open json file %s"), *FString(filePath));
                return false;
            }
            if (!jsonWriter.Write("[\n"))
                return false;
        }
        else if (type == T_CLOSELOGFILE)
        {
            if (jsonWriter.IsOpen())
            {
                if (!jsonWriter.Write("]\n"))
                    return false;
                jsonWriter.Close();
            }
        }
        else if (type == T_EVENT)
        {
            const char* event;
            const char* gameTime;
            const char* unixTimeSeconds;
            const char* unixTimeMilliseconds;
            const char* frameCount;
            if (!reader.ReadString(event)) return false;
            if (!reader.ReadString(gameTime)) return false;
            if (!reader.ReadString(unixTimeSeconds)) return false;
            if (!reader.ReadString(unixTimeMilliseconds)) return false;
            if (!reader.ReadString(frameCount)) return false;
            if (!jsonWriter.WriteFormat("{\"event\":\"%s\",", event)) return false;
            if (!jsonWriter.WriteFormat("\"gameTime\":%s,", gameTime)) return false;
            if (!jsonWriter.WriteFormat("\"unixTimeSeconds\":%s,", unixTimeSeconds)) return false;
            if (!jsonWriter.WriteFormat("\"unixTimeMilliseconds\":%s,", unixTimeMilliseconds)) return false;
            if (!jsonWriter.WriteFormat("\"frameCount\":%s,", frameCount)) return false;
			// Write the data section of the event. This will handle nested structures and properties.
            if (!WriteData())
                return false;
            if (!jsonWriter.Write("}"))
                return false;
        }
        else if (type == T_CONFIG)
        {
			reader.Rewind(1); // Rewind the read head by 1 byte to include the T_CONFIG type in the data section.
			// Write configuration data. This will handle nested structures and properties.
            if (!WriteData())
                return false;
        }
        else
        {
            UE_LOG(LogManagerMsg, Warning, TEXT("WriteBufferToJSon: unexpected data type=%d (T_NEWLOGFILE=%d, T_CLOSELOGFILE=%d, T_EVENT=%d, T_CONFIG=%d)"), type, T_NEWLOGFILE, T_CLOSELOGFILE, T_EVENT, T_CONFIG);
            return false;
        }
    }

    return true;
}


bool BufferToJsonWorker::WriteData()
{
    int stack[10] = { 0 };
    int sizeStack = 0;
    int finish = 0;

    while (!finish)
    {
        uint8 type;
        if (!reader.ReadByte(type)) return false;

        if (type == T_COMPOSED || type == T_CONFIG)
        {
            uint8 nb;
            if (!reader.ReadByte(nb)) return false;

            if (type == T_COMPOSED) {
                const char* Key;
                if (!reader.ReadString(Key)) return false;
                if (!jsonWriter.WriteFormat("\"%s\":{", Key)) return false;
            }
            else {
                if (!jsonWriter.Write("{")) return false;
            }

            if (nb != 0) {
                checkf(sizeStack < UE_ARRAY_COUNT(stack), TEXT("WriteData: nesting depth exceeded"));
                stack[sizeStack++] = nb;
            }
            else {
                if (!jsonWriter.Write("}")) return false;
                while (sizeStack > 0)
                {
                    stack[sizeStack - 1]--;
                    if (stack[sizeStack - 1] > 0)
                    {
                        if (!jsonWriter.Write(",")) return false;
                        break;
                    }
                    sizeStack--;
                    if (!jsonWriter.Write("}")) return false;
                }
                finish = (sizeStack == 0);
            }
        }
        else
        {
            const char* Key;
            if (!reader.ReadString(Key)) return false;
            if (!jsonWriter.WriteFormat("\"%s\":", Key)) return false;

            const char* Value;

            switch (type)
            {
            case T_NUMBER:
            case T_BOOLEAN:
                if (!reader.ReadString(Value)) return false;
                if (!jsonWriter.Write(Value)) return false;
                break;
            case T_STRING:
                if (!reader.ReadString(Value)) return false;
                if (!jsonWriter.WriteFormat("\"%s\"", Value)) return false;
                break;
            default:
                UE_LOG(LogManagerMsg, Warning, TEXT("WriteData: unexpected type=%d (T_COMPOSED=%d, T_CONFIG=%d, T_NUMBER=%d, T_BOOLEAN=%d, T_STRING=%d)"), type, T_COMPOSED, T_CONFIG, T_NUMBER, T_BOOLEAN, T_STRING);
                return false;
            }

            while (sizeStack > 0)
            {
                stack[sizeStack - 1]--;
                if (stack[sizeStack - 1] > 0) {
                    if (!jsonWriter.Write(",")) return false;
                    break;
                }
                sizeStack--;
                if (!jsonWriter.Write("}")) return false;
            }
            finish = (sizeStack == 0);
        }
    }
    return true;
}