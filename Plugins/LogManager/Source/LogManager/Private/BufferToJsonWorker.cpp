#include "BufferToJsonWorker.h"
#include "HAL/PlatformProcess.h"
#include "LogManager.h"

BufferToJsonWorker::BufferToJsonWorker(SPSCBufferQueue& inBuffersFree, SPSCBufferQueue& inBuffersFull, uint32 inCacheSize) : buffersFree(inBuffersFree), buffersFull(inBuffersFull), reader(inBuffersFree, inBuffersFull), jsonWriter(inCacheSize)
{
    // For a clean shutdown (wait end of worker thread)
    bShutdownRequested = false;
    bFinished = false;
}

//uint32 BufferToJsonWorker::Run()
//{
//    UE_LOG(LogManagerMsg, Display, TEXT("BufferToJsonWorker: thread started"));
//
//    while (true)
//    {
//        /*
//         * Process all currently available full buffers.
//         *
//         * This is deliberately not limited to one buffer per iteration.
//         * If the producer is generating data faster than the worker,
//         * processing the queue continuously is preferable.
//         */
//        while (buffersFull.Dequeue(currentBuffer))
//        {
//            WriteBufferToJSon();
//            // The buffer is now completely processed and can return to the producer.
//            currentBuffer->offset = 0; // Reset the offset for the free buffer
//            if (!buffersFree.Enqueue(currentBuffer))
//            {
//                UE_LOG(LogManagerMsg, Error, TEXT("BufferToJsonWorker: failed to return processed buffer to free queue"));
//                // Do not lose the pointer silently.
//                currentBuffer = nullptr;
//                // At this point the buffer cannot safely be reused.
//                bFinished = true;
//                jsonWriter.Close();
//                return 0;
//            }
//            currentBuffer = nullptr;
//        }
//        /*
//         * Shutdown condition:
//         * RequestShutdown() only asks the worker to stop.
//         * We stop only when BuffersFull is empty, which guarantees
//         * that all already-produced data has been processed.
//         * The producer is no longer supposed to add data after shutdown,
//         */
//        if (bShutdownRequested) break;
//
//        // Avoid a busy-spin when there is no work.
//        FPlatformProcess::Sleep(WORKER_TICK_SECONDS);
//        // TODO : 10 ms is only a fallback sleep. As an optimization later, this could be replaced by an event/semaphore.
//        // remplacer ce polling par un FEvent :
//        // 
//        // Game thread
//        //     │
//        //     │ Enqueue(buffer)
//        //     ▼
//        //     BuffersFull
//        //     │
//        //     └── Trigger()
//        //            │
//        //            ▼
//        //            Worker
//        //            │
//        //            └── Wait()
//    }
//
//    // At this point every full buffer has been processed.
//    // If a JSON file is still open, finalize it.
//    if (jsonWriter.IsOpen()) {
//        jsonWriter.Write("]\n");
//        jsonWriter.Close();
//    }
//    UE_LOG(LogManagerMsg, Display, TEXT("BufferToJsonWorker: all buffers written, terminating"));
//    bFinished = true;
//    return 0;
//}


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

        FPlatformProcess::Sleep(WORKER_TICK_SECONDS);
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

void BufferToJsonWorker::RequestShutdown()
{
    bShutdownRequested = true;
    UE_LOG(LogManagerMsg, Display, TEXT("BufferToJsonWorker: shutdown requested"));
}

bool BufferToJsonWorker::IsFinished() const
{
    return bFinished;
}


//bool BufferToJsonWorker::WriteBufferToJSon()
//{
//    checkf(currentBuffer != nullptr, TEXT("WriteBufferToJSon: currentBuffer is null"));
//
//    uint32 readHead = 0;
//    uint32 remaining = currentBuffer->offset;
//    size_t len = 0;
//    while (readHead < currentBuffer->offset)
//    {
//        const int type = currentBuffer->data[readHead];
//        if (type == T_NEWLOGFILE) { 
//            readHead++;
//            // ----- new log file -----
//            remaining = currentBuffer->offset - readHead;
//            len = strnlen(currentBuffer->data + readHead, remaining);
//            if (len == remaining) return false;
//            const char* filepath = currentBuffer->data + readHead;
//            UE_LOG(LogManagerMsg, Display, TEXT("WriteBufferToJSon: open file %s\n"), *FString(filepath));
//            // Opening JSON file in write mode
//            if (!JsonWriter.Open(FString(filepath)))
//            {
//                UE_LOG(LogManagerMsg, Error, TEXT("WriteBufferToJSon : fail to open json file %s\n"), *FString(filepath));
//                return false;
//            }
//            UE_LOG(LogManagerMsg, Display, TEXT("WriteBufferToJSon: open successful %s\n"), *FString(filepath));
//
//            if (!JsonWriter.Write("[\n")) return false;
//            readHead += static_cast<uint32>(len) + 1;
//        }
//        else if (type == T_CLOSELOGFILE)
//        {
//            readHead++;
//            // ----- close log file -----
//			if (JsonWriter.IsOpen()) // TODO : verify if it is necessary to check if the file is open before closing it. It should be open if we received a T_CLOSELOGFILE event.
//            {
//                JsonWriter.Write("]\n");
//                JsonWriter.Close();
//            }
//        }
//        else if (type == T_EVENT)// TODO : gros probleme de conception : maintenant, les données de T_EVENT peuvent être à cheval sur deux buffer... 
//        {
//            readHead++;
//            // ----- event -----
//            remaining = currentBuffer->offset - readHead;
//            len = strnlen(currentBuffer->data + readHead, remaining);
//			if (len == remaining) return false;
//			if (!JsonWriter.WriteFormat("{\"event\":\"%s\",", currentBuffer->data + readHead)) return false;
//            readHead += static_cast<uint32>(len) + 1;
//            // ----- gameTime -----
//            remaining = currentBuffer->offset - readHead;
//            len = strnlen(currentBuffer->data + readHead, remaining);
//            if (len == remaining) return false;
//            if (!JsonWriter.WriteFormat("\"gameTime\":%s,", currentBuffer->data + readHead)) return false;
//            readHead += static_cast<uint32>(len) + 1;
//            // ----- unixTimeSeconds -----
//            remaining = currentBuffer->offset - readHead;
//            len = strnlen(currentBuffer->data + readHead, remaining);
//            if (len == remaining) return false;
//            if (!JsonWriter.WriteFormat("\"unixTimeSeconds\":%s,", currentBuffer->data + readHead)) return false;
//            readHead += static_cast<uint32>(len) + 1;
//            // ----- unixTimeMilliseconds -----
//            remaining = currentBuffer->offset - readHead;
//            len = strnlen(currentBuffer->data + readHead, remaining);
//            if (len == remaining) return false;
//            if (!JsonWriter.WriteFormat("\"unixTimeMilliseconds\":%s,", currentBuffer->data + readHead)) return false;
//            readHead += static_cast<uint32>(len) + 1;
//            // ----- frameCount -----
//            remaining = currentBuffer->offset - readHead;
//            len = strnlen(currentBuffer->data + readHead, remaining);
//            if (len == remaining) return false;
//            if (!JsonWriter.WriteFormat("\"frameCount\":%s,", currentBuffer->data + readHead)) return false;
//            readHead += static_cast<uint32>(len) + 1;
//            // ----- data -----
//            if (!WriteData(&readHead)) return false;
//            if (!JsonWriter.Write("}")) return false;
//            if ((readHead < currentBuffer->offset) || (!bShutdownRequested))
//            {
//                if (!JsonWriter.Write(",\n")) return false;
//            }
//        }
//        else if (type == T_CONFIG)
//        {
//            // ----- configuration data -----
//            if (!WriteData(&readHead)) return false;
//            if ((readHead < currentBuffer->offset) || (!bShutdownRequested))
//            {
//                if (!JsonWriter.Write(",\n")) return false;
//            }
//        }
//        else
//        {
//            UE_LOG(LogManagerMsg, Warning, TEXT("WriteBufferToJSon unexpected data type=%d (file open=%d, file close=%d, event=%d, config data=%d)"), type, T_NEWLOGFILE, T_CLOSELOGFILE, T_EVENT, T_CONFIG);
//            return false;
//        }
//    }
//    return true;
//}
//

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


//bool BufferToJsonWorker::WriteData(uint32* readHead)
//{
//    int stack[10] = { 0 };
//    int sizeStack = 0;
//    int type, nb;
//    int finish = 0;
//    int hasNext;
//
//    while (!finish) {
//        type = currentBuffer->data[*readHead];
//        (*readHead)++;
//        if (type == T_COMPOSED || type == T_CONFIG) {
//            nb = currentBuffer->data[*readHead];
//            (*readHead)++;
//            if (type == T_COMPOSED)
//            {
//                if (!JsonWriter.WriteFormat("\"%s\":{", ReadString(readHead))) return false;
//            }
//            else {
//                if (!JsonWriter.Write("{")) return false;
//                (*readHead)++;
//            }
//            if (nb != 0) {
//                checkf(sizeStack < UE_ARRAY_COUNT(stack), TEXT("WriteData: nesting depth exceeded"));
//                stack[sizeStack] = nb;
//                sizeStack++;
//            }
//            else {
//                hasNext = 0;
//                if (!JsonWriter.Write("}")) return false;
//                while ((!hasNext) && (sizeStack > 0)) {
//                    stack[sizeStack - 1]--;
//                    if (stack[sizeStack - 1] > 0)
//                    {
//                        hasNext = 1;
//                        if (!JsonWriter.Write(",")) return false;
//                    }
//                    else
//                    {
//                        sizeStack--;
//                        if (!JsonWriter.Write("}")) return false;
//                    }
//                }
//                finish = (sizeStack == 0);
//            }
//        }
//        else {
//            // Property name
//            if (!JsonWriter.WriteFormat("\"%s\":", ReadString(readHead))) return false;
//            // Property value
//            switch (type)
//            {
//            case T_NUMBER:
//                if (!JsonWriter.Write(ReadString(readHead))) return false;
//                break;
//            case T_BOOLEAN:
//                if (!JsonWriter.Write(ReadString(readHead))) return false;
//                break;
//            case T_STRING:
//                if (!JsonWriter.WriteFormat("\"%s\"", ReadString(readHead))) return false;
//                break;
//            default:
//                break;
//            }
//            hasNext = 0;
//            while ((!hasNext) && (sizeStack > 0)) {
//                stack[sizeStack - 1]--;
//                if (stack[sizeStack - 1] > 0) {
//                    hasNext = 1;
//                    if (!JsonWriter.Write(",")) return false;
//                }
//                else {
//                    sizeStack--;
//                    if (!JsonWriter.Write("}")) return false;
//                }
//            }
//            finish = (sizeStack == 0);
//        }
//    }
//    return true;
//}

//const char* BufferToJsonWorker::ReadString(uint32* readHead)
//{
//    const char* result = currentBuffer->data + (*readHead);
//    while (currentBuffer->data[*readHead])
//        (*readHead)++;
//    (*readHead)++;
//    return result;
//}
//
