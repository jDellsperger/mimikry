#include <mutex>
#include <thread>

#include <float.h>

#include "../include/platform.h"
#include "../include/math.h"
#include "b_transmission.cpp"
#include "beholder.cpp"
#include "b_datahandler.cpp"

#define MULTITHREADED 1

i32 main(i32 argc, const char* argv[])
{
    printf("Starting initialisation \n");
    
    DebugStatus _debugStatus = DebugStatus_All;
    bool32 _saveRaysToFile = 0;
    bool32 _matchModel = 0;
    bool32 loadRaysFromFile = 0;
    ReadFileResult loadedBuckets = {};
    
    for (i32 i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            _saveRaysToFile = 1;
            continue;
        }
        
        if (strcmp(argv[i], "-r") == 0)
        {
            loadRaysFromFile = 1;
            _matchModel = 1;
            std::string loadFilename = argv[i + 1];
            i++;
            
            bool32 readResult = readEntireFile(loadFilename.c_str(),
                                               &loadedBuckets);
            if (!readResult)
            {
                printf("Could not read rays from file %s\n",
                       loadFilename.c_str());
                return 1;
            }
            continue;
        }
    }
    
    ClientList clientlist = {};
    
    void* baseAddress = (void*)terabytes(2);
    u64 permanentMemorySize = megabytes(100);
    u64 flushMemorySize = megabytes(50);
    u64 listenerMemorySize = megabytes(100);
    u64 totalMemorySize = permanentMemorySize + flushMemorySize + listenerMemorySize;
    u8* systemMemory = (u8*)mmap(baseAddress, 
                                 totalMemorySize,
                                 PROT_READ | PROT_WRITE,
                                 MAP_ANON | MAP_PRIVATE,
                                 -1, 0);
    
    MemoryArena permanentArena, flushArena, listenerArena;
    initMemoryArena(&permanentArena, permanentMemorySize,
                    (systemMemory));
    initMemoryArena(&flushArena, flushMemorySize,
                    (systemMemory + permanentMemorySize));
    initMemoryArena(&listenerArena, listenerMemorySize,
                    (systemMemory + permanentMemorySize + flushMemorySize));
    
    ApplicationState _applicationState = {};
    i32 maxRigRestrictionCount = 10;
    initApplication(&permanentArena, &_applicationState, maxRigRestrictionCount);
    
    TransmissionState spotterReceiverTransmissionState = {};
    TransmissionState sendTransmissionState = {};
    TransmissionState spotterSenderTransmissionState = {};
    
    initTransmissionState(&spotterReceiverTransmissionState);
    initTransmissionState(&sendTransmissionState);
    initTransmissionState(&spotterSenderTransmissionState);
    
    RayBuckets* _rayBuckets = pushStruct(&permanentArena, RayBuckets);
    initRayBuckets(_rayBuckets);
    
    DebugInfos* _debugInfos = pushStruct(&permanentArena, DebugInfos);
    *_debugInfos = {};
    
    printf("Everything initiated \n");
    
    flushMemory(&flushArena);
    
    openPullTransmissionChannel(&spotterReceiverTransmissionState,
                                "5557");
    openPublisherTransmissionChannel(&spotterSenderTransmissionState,
                                     "5560");
    openPushTransmissionChannel(&sendTransmissionState,
                                "localhost",
                                "5556",
                                0);
    
    std::thread listener(messageHandler, 
                         &listenerArena, 
                         &spotterReceiverTransmissionState, 
                         &spotterSenderTransmissionState, 
                         _rayBuckets, 
                         &clientlist, 
                         _debugInfos, 
                         &_saveRaysToFile, 
                         &_matchModel, 
                         &_debugStatus,
                         &_applicationState);
    
    i32 fileDescriptor = -1;
    
    bool32 modelMatched = 0;
    i32 loopIndex = 0;
    
    i32 frameIndex = 0;
    
    while (_applicationState.status != ApplicationStatus_Exiting)
    {
        printf("frame %i\n", frameIndex++);
        
        if (frameIndex == 100)
        {
            printf("bREEEEEEAKKKKKK!!\n");
        }
        
        u64 starttime = getWallclockTimeInMs();
        
        i32 bucketCount = CAMERA_COUNT;
        Bucket buckets[bucketCount] = {};
        
        u64 startGetDebugInfoTime = getWallclockTimeInMs();
        
        DebugInfos debugInfos = {};
        
        global_debugInfoMutex.lock();
        memcpy(&debugInfos, _debugInfos, sizeof(DebugInfos));
        
        _debugInfos->updateFlags = DebugUpdateFlags_None;
        
        for (u32 i = 0; i < arrayLength(_debugInfos->cameraUpdated); i++)
        {
            _debugInfos->cameraUpdated[i] = 0;
        }
        
        for (u32 i = 0; i < arrayLength(_debugInfos->frameUpdated); i++)
        {
            _debugInfos->frameUpdated[i] = 0;
        }
        global_debugInfoMutex.unlock();
        
        u64 endGetDebugInfoTime = getWallclockTimeInMs();
        u64 getDebugInfoTime = endGetDebugInfoTime - startGetDebugInfoTime;
        
        u64 startGetRaysTime = getWallclockTimeInMs();
        
        if (loadRaysFromFile)
        {
            readFromFileResult(&loadedBuckets,
                               &buckets,
                               bucketCount * sizeof(Bucket));
            debugInfos.updateFlags |= DebugUpdateFlags_Rays;
        }
        else
        {
            while (!getBucketsIfAllUpdated(_rayBuckets, buckets, bucketCount))
            {
                if (_applicationState.status == ApplicationStatus_Exiting)
                {
                    break;
                }
                usleep(10);
            }
        }
        
        u64 endGetRaysTime = getWallclockTimeInMs();
        u64 getRaysTime = endGetRaysTime - startGetRaysTime;
        
        u64 startDetectIntersectionsTime = getWallclockTimeInMs();
        
        IntersectionVector intersectionsHC = {};
        IntersectionVector intersectionsLC = {};
        
        // high confidence intersections (threeway)
        r32 mergeDistThreshold = 3.0f;
        r32 hcIntersectionDistThreshold = 1.0f;
        intersectionsHC = detectThreewayIntersections(&flushArena,
                                                      buckets,
                                                      bucketCount,
                                                      hcIntersectionDistThreshold,
                                                      mergeDistThreshold);
        
        // low confidence intersections (twoway)
        r32 lcIntersectionDistThreshold = 0.7f;
        intersectionsLC = 
            detectIntersections(&flushArena,
                                buckets,
                                bucketCount,
                                lcIntersectionDistThreshold,
                                mergeDistThreshold);
        
        u64 endDetectIntersectionsTime = getWallclockTimeInMs();
        u64 detectIntersectionsTime =
            endDetectIntersectionsTime - startDetectIntersectionsTime;
        
        handleAndSendDebugInfos(&flushArena,
                                &sendTransmissionState,
                                &debugInfos,
                                buckets,
                                bucketCount,
                                &intersectionsHC);
        
        u64 startHandleModelTime = getWallclockTimeInMs();
        
        global_flagMutex.lock();
        bool32 matchModel = _matchModel;
        global_flagMutex.unlock();
        
        if (matchModel)
        {
            modelMatched = matchIntersectionsToRig(&flushArena,
                                                   &_applicationState.rig,
                                                   &intersectionsHC);
            
            if (modelMatched)
            {
                global_flagMutex.lock();
                _matchModel = 0;
                global_flagMutex.unlock();
                
                sendMessage(&flushArena,
                            &sendTransmissionState,
                            MessageType_Payload,
                            _applicationState.rig.points,
                            13 * sizeof(V3),
                            0);
            }
        }
        else if (modelMatched)
        {
            trackRig(&flushArena,
                     &_applicationState.rig,
                     &intersectionsHC,
                     &intersectionsLC);
            
            sendMessage(&flushArena,
                        &sendTransmissionState,
                        MessageType_Payload,
                        _applicationState.rig.points,
                        13 * sizeof(V3),
                        0);
        }
        
        u64 endHandleModelTime = getWallclockTimeInMs();
        u64 handleModelTime = endHandleModelTime - startHandleModelTime;
        
        u64 startSaveRaysTime = getWallclockTimeInMs();
        
        global_flagMutex.lock();
        bool32 saveRaysToFile = _saveRaysToFile;
        global_flagMutex.unlock();
        
        if (saveRaysToFile)
        {
            if (fileDescriptor == -1)
            {
                char timeString[100];
                getTimeString(timeString, 100);
                char filename[120];
                snprintf(filename, 120, "debug_rays_%s.behold", timeString);
                printf("saving rays to file %s\n", filename);
                fileDescriptor = openFileForWriting(filename);
            }
            
            writeToFile(fileDescriptor,
                        buckets,
                        bucketCount * sizeof(Bucket));
        }
        
        u64 endSaveRaysTime = getWallclockTimeInMs();
        u64 saveRaysTime = endSaveRaysTime - startSaveRaysTime;
        
        u64 startHandleDebugInfoTime = getWallclockTimeInMs();
        
        global_flagMutex.lock();
        DebugStatus debugStatus = _debugStatus;
        global_flagMutex.unlock();
        
#if 0
        if(debugStatus)
        {
            handleAndSendDebugInfos(&flushArena,
                                    &sendTransmissionState,
                                    &debugInfos,
                                    buckets,
                                    bucketCount,
                                    &mergedIntersections);
        }
#endif
        
        u64 endHandleDebugInfoTime = getWallclockTimeInMs();
        u64 handleDebugInfoTime = endHandleDebugInfoTime - startHandleDebugInfoTime;
        
        flushMemory(&flushArena);
        
        u64 endtime = getWallclockTimeInMs();
        u64 dT = endtime - starttime;
        i64 sleeptime = 25000 - (dT * 1000);
        
#if 0
        printf("dT: %" PRIu64 "\n"
               "\tget debuginfo:  %" PRIu64 "\n"
               "\tget rays: %" PRIu64 "\n"
               "\tdetect intersections: %" PRIu64 "\n"
               "\tmodel: %" PRIu64 "\n"
               "\tsave rays: %" PRIu64 "\n"
               "\thandle debuginfo: %" PRIu64 "\n"
               "sleeping for: %" PRIu64 "\n"
               "---------------------------\n",
               dT, 
               getDebugInfoTime,
               getRaysTime,
               detectIntersectionsTime,
               handleModelTime,
               saveRaysTime,
               handleDebugInfoTime,
               sleeptime);
#endif
        
        while (sleeptime > 0)
        {
            usleep(sleeptime);
            
            endtime = getWallclockTimeInMs();
            dT = endtime - starttime;
            
            sleeptime -= (dT * 1000);
        }
    }
    
    listener.join();
    
    closeTransmissionChannel(&spotterReceiverTransmissionState);
    closeTransmissionChannel(&sendTransmissionState);
    closeTransmissionChannel(&spotterSenderTransmissionState);
    destroyTransmissionState(&spotterReceiverTransmissionState);
    destroyTransmissionState(&sendTransmissionState);
    destroyTransmissionState(&spotterSenderTransmissionState);
    
    return 0;
}

