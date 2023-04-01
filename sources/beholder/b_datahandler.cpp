#include "b_datahandler.h"

static void messageHandler(MemoryArena* listenerArena, 
                           TransmissionState* spotterReceiverTransmissionState, 
                           TransmissionState* spotterSenderTransmissionState,
                           RayBuckets* rayBuckets,
                           ClientList* clientList,
                           DebugInfos* _debugInfos,
                           bool32* _saveRaysToFile,
                           bool32* _matchModel,
                           DebugStatus* _debugStatus,
                           ApplicationState* _applicationState)
{
    Message msg = {};
    u64 permanentListenerMemorySize = megabytes(80);
    u64 flushListenerMemorySize = megabytes(20);
    
    assert(listenerArena->size >= 
           (permanentListenerMemorySize + flushListenerMemorySize));
    
    MemoryArena permanentListenerArena, flushListenerArena;
    initMemoryArena(&permanentListenerArena, permanentListenerMemorySize, 
                    (listenerArena->base));
    initMemoryArena(&flushListenerArena, flushListenerMemorySize, 
                    (listenerArena->base + permanentListenerMemorySize));
    
    while(_applicationState->status != ApplicationStatus_Exiting)
    {
        while (receiveMessageNonBlocking(&flushListenerArena,
                                         spotterReceiverTransmissionState,
                                         &msg))
        { 
            switch (msg.header.type)
            {
                case MessageType_Payload:
                {
                    if (msg.data)
                    {
                        u8 clientID = msg.header.spotterID;
                        u32 rayCount = msg.header.payloadSize/sizeof(Ray);
                        u8* data = (u8*)msg.data;
                        
                        i32 backIndex = rayBuckets->backIndex;
                        Bucket* bucket = &rayBuckets->buckets[backIndex][clientID - 1];
                        bucket->used = 0;
                        
                        for(u32 i = 0; i < rayCount && i < 100; i++) 
                        {
                            bucket->rays[i] = *((Ray*)data);
                            data += sizeof(Ray);
                            bucket->used++;
                        }
                        
                        rayBuckets->updatedClientsCount++;
                        bool32 grabFrameCommandCanBeSent =
                            rayBuckets->updatedClientsCount >= rayBuckets->clientCount;
                        
                        if (grabFrameCommandCanBeSent)
                        {
                            global_bucketMutex.lock();
                            rayBuckets->_allBucketsUpdatedSinceGet = 1;
                            rayBuckets->_frontIndex = backIndex;
                            global_bucketMutex.unlock();
                            
                            rayBuckets->backIndex = (backIndex + 1) % RAY_BUCKET_COUNT;
                            rayBuckets->updatedClientsCount = 0;
                            rayBuckets->clientCount = clientList->clientCount;
                            
                            CommandType commandType = CommandType_GrabFrame;
                            sendMessage(&flushListenerArena,
                                        spotterSenderTransmissionState,
                                        MessageType_Command,
                                        &commandType,
                                        sizeof(CommandType),
                                        0);
                        }
                        
                        if (rayCount)
                        {
                            global_debugInfoMutex.lock();
                            _debugInfos->updateFlags |= DebugUpdateFlags_Rays;
                            global_debugInfoMutex.unlock();
                        }
                    }
                } break;
                
                case MessageType_DebugCameraPose: {
                    M4x4* cameraPose = (M4x4*)msg.data;
                    
                    global_debugInfoMutex.lock();
                    
                    u8 index = msg.header.spotterID - 1;
                    _debugInfos->cameraLocations[index] = 
                        *cameraPose;
                    _debugInfos->updateFlags |= DebugUpdateFlags_Cameras;
                    _debugInfos->cameraUpdated[index] = 1;
                    
                    global_debugInfoMutex.unlock();
                } break;
                
                case MessageType_DebugFrame: {
                    // TODO(jan): assert that frame sizes match with previous frames?
                    u8 index = msg.header.spotterID - 1;
                    
                    global_debugInfoMutex.lock();
                    memcpy((u8*)_debugInfos->frames[index].memory,
                           msg.data,
                           msg.header.payloadSize);
                    _debugInfos->updateFlags |= DebugUpdateFlags_Frames;
                    _debugInfos->frameUpdated[index] = 1;
                    global_debugInfoMutex.unlock();
                } break;
                
                case MessageType_HelloReq: {
                    if (msg.data) 
                    {   
                        HelloPayload* payload = (HelloPayload*)msg.data;
                        std::string spotterIP = std::string(payload->ip);
                        u32 payloadSize = spotterIP.size() + 1;
                        u8 clientID = getKnownClientID(clientList, spotterIP);
                        if(clientID != 0) 
                        {
                            sendMessage(&flushListenerArena,
                                        spotterSenderTransmissionState, 
                                        MessageType_HelloRep,
                                        const_cast<char*>(spotterIP.c_str()),
                                        payloadSize,
                                        clientID);
                            
                            printf("Known Spotter %i reregistered\n", clientID);
                        }
                        else 
                        {
                            // TODO(jan): make sure we have enough space in clientList?
                            HelloPayload* payload = (HelloPayload*)msg.data;
                            std::string spotterIP = std::string(payload->ip);
                            u32 payloadSize = spotterIP.size() + 1;
                            Client* client = &clientList->clients[clientList->clientCount];
                            client->id = clientList->clientCount + 1;
                            client->ip = spotterIP;
                            clientList->clientCount++;
                            
                            i32 arrayIndex = client->id - 1;
                            
                            global_debugInfoMutex.lock();
                            _debugInfos->frames[arrayIndex] =
                                initializeFrame(&permanentListenerArena,
                                                payload->frameWidth,
                                                payload->frameHeight,
                                                1);
                            global_debugInfoMutex.unlock();
                            
                            if (clientList->clientCount == 1)
                            {
                                rayBuckets->clientCount = clientList->clientCount;
                                CommandType commandType = CommandType_GrabFrame;
                                sendMessage(&flushListenerArena,
                                            spotterSenderTransmissionState,
                                            MessageType_Command,
                                            &commandType,
                                            sizeof(CommandType),
                                            0);
                            }
                            
                            sendMessage(&flushListenerArena,
                                        spotterSenderTransmissionState, 
                                        MessageType_HelloRep,
                                        const_cast<char*>(spotterIP.c_str()), 
                                        payloadSize,
                                        client->id);
                            
                            printf("New Spotter %i registered with ip %s \n", 
                                   client->id, 
                                   client->ip.c_str());
                        }
                    } 
                } break;
                
                case MessageType_Command: {
                    if (msg.data)
                    {
                        CommandType commandType = *((CommandType*)msg.data);
                        printf("Received command  %i\n", commandType);
                        
                        if (commandType == CommandType_SaveRaysToFile)
                        {
                            global_flagMutex.lock();
                            *_saveRaysToFile = 1;
                            global_flagMutex.unlock();
                        }
                        else if (commandType == CommandType_MatchModel)
                        {
                            global_flagMutex.lock();
                            *_matchModel = 1;
                            global_flagMutex.unlock();
                        }
                        else if (commandType == CommandType_StartDebugging)
                        {
                            global_flagMutex.lock();
                            *_debugStatus = DebugStatus_All;
                            global_flagMutex.unlock();
                        }
                        else if (commandType == CommandType_StopDebugging)
                        {
                            global_flagMutex.lock();
                            *_debugStatus = DebugStatus_None;
                            global_flagMutex.unlock();
                        }
                        else if (commandType == CommandType_StopSystem)
                        {
                            global_flagMutex.lock();
                            _applicationState->status = ApplicationStatus_Exiting;
                            global_flagMutex.unlock();
                            
                            sendMessage(&flushListenerArena,
                                        spotterSenderTransmissionState,
                                        MessageType_Command,
                                        msg.data,
                                        msg.header.payloadSize,
                                        0);
                        }
                        else
                        {
                            sendMessage(&flushListenerArena,
                                        spotterSenderTransmissionState,
                                        MessageType_Command,
                                        msg.data,
                                        msg.header.payloadSize,
                                        0);
                        }
                    }
                } break;
            }
        }
        
        flushMemory(&flushListenerArena);
    }
}

static bool32 getBucketsIfAllUpdated(RayBuckets* src, 
                                     Bucket* dest, 
                                     i32 bucketCount)
{
    bool32 result = 0;
    
    global_bucketMutex.lock();
    if (src->_allBucketsUpdatedSinceGet)
    {
        result = 1;
        memcpy(dest, 
               src->buckets[src->_frontIndex],
               bucketCount * sizeof(Bucket));
        src->_allBucketsUpdatedSinceGet = 0;
    }
    global_bucketMutex.unlock();
    
    return result;
}

