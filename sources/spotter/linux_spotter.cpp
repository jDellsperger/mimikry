#define CHESSBOARD_INNER_COLUMN_COUNT 8
#define CHESSBOARD_INNER_ROW_COUNT 11
#define CHESSBOARD_FIELD_EDGE_IN_CM 12.1f

#define POSE_ESTIMATION_COUNT 1
#define POSE_ESTIMATION_TIMEOUT_MS 10

#define USE_CV_ANALYZATION 1

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBIW_ASSERT(x)
#include "../include/stb_image_write.h"

#include "../include/platform.h"
#include "../include/math.h"
#include "s_brightpiController.cpp"
#include "s_capture.cpp"
#include "s_analyzation.cpp"
#include "s_transmission.cpp"
#include "s_spotter.cpp"

i32 main(i32 argc, const char* argv[])
{
    printf("capture size: %i x %i, window size: %i x %i\n",
           CAPTURE_FRAME_WIDTH, CAPTURE_FRAME_HEIGHT,
           WINDOW_WIDTH, WINDOW_HEIGHT);
    std::string serverIp = "127.0.0.1";
    std::string portNumber = "5557";
    std::string handshakePortNumber = "5560";
    
    char calibrationFilePath[100];
    snprintf(calibrationFilePath,
             100,
             "calibrations/spotter_%ix%i.calib", 
             WINDOW_WIDTH, WINDOW_HEIGHT);
    
    ReadFileResult loadedFrames;
    bool32 readFramesFromFile = 0;
    
    ReadFileResult loadedPose;
    bool32 loadPoseFromFile = 0;
    
    for (i32 i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-s") == 0)
        {
            serverIp = argv[i + 1];
            i++;
            continue;
        }
        
        if (strcmp(argv[i], "-r") == 0)
        {
            readFramesFromFile = 1;
            std::string loadFilename = argv[i + 1];
            i++;
            
            bool32 readResult = readEntireFile(loadFilename.c_str(),
                                               &loadedFrames);
            if (!readResult)
            {
                printf("Could not read frames from file %s\n",
                       loadFilename.c_str());
                return 1;
            }
            
            continue;
        }
        
        if (strcmp(argv[i], "-p") == 0)
        {
            loadPoseFromFile = 1;
            std::string loadFilename = argv[i + 1];
            i++;
            
            bool32 readResult = readEntireFile(loadFilename.c_str(),
                                               &loadedPose);
            if (!readResult)
            {
                printf("Could not load pose from file %s\n",
                       loadFilename.c_str());
                return 1;
            }
            
            continue;
        }
    }
    
    Calibration calibration = {};
    ReadFileResult calibrationFile;
    Calibration* calibrationPtr = 0;
    bool32 readResult = readEntireFile(calibrationFilePath,
                                       &calibrationFile);
    if (readResult)
    {
        if (calibrationFile.contentSize == sizeof(Calibration))
        {
            printf("Calibration file %s read\n",
                   calibrationFilePath);
            calibration =
                *((Calibration*)calibrationFile.content);
            calibrationPtr = &calibration;
        }
        
        freeFileMemory(&calibrationFile);
    }
    
    void* baseAddress = (void*)terabytes(2);
    u64 permanentMemorySize = megabytes(50);
    u64 flushMemorySize = megabytes(50);
    u64 totalMemorySize = permanentMemorySize + flushMemorySize;
    u8* systemMemory = (u8*)mmap(baseAddress, 
                                 totalMemorySize,
                                 PROT_READ | PROT_WRITE,
                                 MAP_ANON | MAP_PRIVATE,
                                 -1, 0);
    
    MemoryArena permanentArena, flushArena;
    initMemoryArena(&permanentArena, permanentMemorySize,
                    (systemMemory));
    initMemoryArena(&flushArena, flushMemorySize,
                    (systemMemory+permanentMemorySize));
    
    CaptureState captureState = {};
    const char* devName = "/dev/video0";
    if (!startCapturing(&permanentArena,
                        &captureState,
                        CAPTURE_BUFFER_COUNT, 
                        devName))
    {
        printf("Could not initialize capture\n");
        return -1;
    }
    
    I2CBus brightPi = {};
    initI2C(&brightPi);
    
    std::string localIp = getLocalIP();
    ApplicationState applicationState = {};
    initApplication(&applicationState,
                    &permanentArena,
                    CAPTURE_FRAME_WIDTH,
                    CAPTURE_FRAME_HEIGHT,
                    calibrationPtr,
                    localIp);
    
    if (loadPoseFromFile)
    {
        readFromFileResult(&loadedPose,
                           &applicationState.cTw,
                           sizeof(applicationState.cTw));
        
        V4 cC = v4(0.0f, 0.0f, 0.0f, 1.0f);
        V4 wC = multM4x4V4(applicationState.cTw.inv, cC);
        applicationState.cameraOrigin = v3(wC.x, wC.y, wC.z);
        
        printf("Camera origin: ");
        printV3(&applicationState.cameraOrigin);
        
        applicationState.poseLoadedFromFile = 1;
    }
    
    TransmissionState senderTransmissionState = {};
    TransmissionState receiverTransmissionState = {};
    initTransmissionState(&senderTransmissionState);
    initTransmissionState(&receiverTransmissionState);
    
    openPushTransmissionChannel(&senderTransmissionState,
                                serverIp,
                                portNumber,
                                0);
    
    openSubscriberTransmissionChannel(&receiverTransmissionState,
                                      serverIp,
                                      handshakePortNumber);
    
    usleep(1000*1000);
    
    //NOTE(dave): Send Handshake
    HelloPayload helloPayload = {};
    snprintf(helloPayload.ip, 
             arrayLength(helloPayload.ip),
             "%s",
             localIp.c_str());
    helloPayload.frameWidth = WINDOW_WIDTH;
    helloPayload.frameHeight = WINDOW_HEIGHT;
    sendMessage(&flushArena,
                &senderTransmissionState,
                MessageType_HelloReq,
                &helloPayload,
                sizeof(helloPayload),
                0);
    
    printf("Handshake sent\n");
    
    InputState inputState = {};
    
    bool32 saveFramesToFile = 0;
    
    while (applicationState.status != ApplicationStatus_Exiting)
    {
        u64 startTime = getWallclockTimeInMs();
        
        u64 startMessageHandlingTime = getWallclockTimeInMs();
        
        Message msg = {};
        while(receiveMessageNonBlocking(&flushArena, 
                                        &receiverTransmissionState, 
                                        &msg))
        {
            switch(msg.header.type)
            {
                case MessageType_HelloRep:
                {
                    std::string ip = std::string((char*)msg.data);
                    if(ip == applicationState.ip)
                    {
                        receiverTransmissionState.spotterID = msg.header.spotterID;
                        senderTransmissionState.spotterID = msg.header.spotterID;
                        
                        printf("SpotterID set: %i\n",
                               receiverTransmissionState.spotterID);
                        
                        applicationState.status = ApplicationStatus_Idle;
                    }
                } break;
                
                case MessageType_Command:
                {
                    CommandType commandType = *((CommandType*)msg.data);
                    
                    if (commandType != 1)
                    {
                        printf("Command received %i\n", commandType);
                    }
                    
                    if (commandType == CommandType_GrabFrame)
                    {
                        applicationState.grabFrame = 1;
                    }
                    else if (commandType == CommandType_EstimatePose)
                    {
                        switchLEDsOff(&brightPi);
                        switchVisableOn(&brightPi);
                        applicationState.poseEstimationIndex = 0;
                        applicationState.status = ApplicationStatus_EstimatingPose;
                    }
                    else if (commandType == CommandType_SendGrayscale)
                    {
                        senderTransmissionState.sendBufferType =
                            SendBufferType_Grayscale;
                    }
                    else if (commandType == CommandType_SendBinarized)
                    {
                        senderTransmissionState.sendBufferType =
                            SendBufferType_Binarized;
                    }
                    else if (commandType == CommandType_NoFrames)
                    {
                        senderTransmissionState.sendBufferType =
                            SendBufferType_None;
                    }
                    else if (commandType == CommandType_SaveFramesToFile)
                    {
                        saveFramesToFile = 1;
                        char timeString[20];
                        getTimeString(timeString, 100);
                        char filename[100];
                        snprintf(filename, 100, "debug_frames_%ix%i_%s.spot", 
                                 CAPTURE_FRAME_WIDTH,
                                 CAPTURE_FRAME_HEIGHT,
                                 timeString);
                        printf("saving frames to file %s\n", filename);
                        captureState.saveFileDescriptor = openFileForWriting(filename);
                    }
                    else if (commandType == CommandType_BinarizationThreshold)
                    {
                        u8 threshold = *(((u8*)msg.data) + sizeof(commandType));
                        printf("threshold: %i\n", threshold);
                        applicationState.binarizationThreshold = threshold;
                        printf("Threshold set\n");
                    }
                    else if (commandType == CommandType_StopSystem)
                    {
                        applicationState.status = ApplicationStatus_Exiting;
                    }
                } break;
            }
        }
        
        u64 endMessageHandlingTime = getWallclockTimeInMs();
        u64 messageHandlingTime = endMessageHandlingTime - startMessageHandlingTime;
        
        // NOTE(jan): if the grab frame command has been received,
        // handle the frame and send results to beholder
        u64 captureTime = 0;
        u64 frameSendingTime = 0;
        u64 handlingTime = 0;
        if (applicationState.grabFrame)
        {
            u64 startCaptureTime = getWallclockTimeInMs();
            
            Frame* frame = 0;
            
            if (readFramesFromFile)
            {
                usleep(33000);
                *frame = initializeFrame(&flushArena,
                                         CAPTURE_FRAME_WIDTH, CAPTURE_FRAME_HEIGHT,
                                         2);
                readFromFileResult(&loadedFrames,
                                   frame->memory,
                                   frame->pitch * frame->height);
            }
            else
            {
#if 0
                // TODO(jan): rethink this
                // NOTE(jan): always grab frames to prevent them from queing up
                while (!(readFrame(&captureState, &frame))) 
                {
                    if (errno != EAGAIN)
                    {
                        printErrno();
                        return -1;
                    }
                    else
                    {
                        usleep(10);
                    }
                }
#else
                readFrame(&captureState, &frame);
#endif
            }
            
            u64 endCaptureTime = getWallclockTimeInMs();
            captureTime = endCaptureTime - startCaptureTime;
            
            u64 startFrameSendingTime = getWallclockTimeInMs();
            
            if (senderTransmissionState.status == TransmissionStatus_Ok &&
                senderTransmissionState.sendBufferType != SendBufferType_None)
            {
                Frame downsampleFrame = initializeFrame(&flushArena,
                                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                                        1);
                
                if (senderTransmissionState.sendBufferType == SendBufferType_Grayscale)
                {
                    CV_resizeFrame(frame,
                                   &downsampleFrame);
                }
                else if (senderTransmissionState.sendBufferType == SendBufferType_Binarized)
                {
                    CV_binarize(frame,
                                &applicationState.binarizedFrame,
                                applicationState.binarizationThreshold);
                    CV_resizeFrame(&applicationState.binarizedFrame,
                                   &downsampleFrame);
                }
                
                Frame transmissionFrame = initializeFrame(&flushArena,
                                                          WINDOW_WIDTH, WINDOW_HEIGHT,
                                                          1);
                transmissionFrame.size = 0;
                stbi_write_jpg_to_func(&writeToFrame,
                                       &transmissionFrame,
                                       downsampleFrame.width,
                                       downsampleFrame.height,
                                       1,
                                       downsampleFrame.memory,
                                       10);
                sendMessage(&flushArena,
                            &senderTransmissionState,
                            MessageType_DebugFrame,
                            transmissionFrame.memory,
                            transmissionFrame.size,
                            senderTransmissionState.spotterID);
            }
            
            u64 endFrameSendingTime = getWallclockTimeInMs();
            frameSendingTime = endFrameSendingTime - startFrameSendingTime;
            
            u64 startHandlingTime = getWallclockTimeInMs();
            
            void* payload = 0;
            i32 payloadSize = 0;
            
            switch (applicationState.status)
            {
                case ApplicationStatus_EstimatingPose: {
                    if (applicationState.poseLoadedFromFile)
                    {
                        applicationState.poseLoadedFromFile = 0;
                        
                        sendMessage(&flushArena,
                                    &senderTransmissionState, 
                                    MessageType_DebugCameraPose,
                                    &applicationState.cTw.inv,
                                    sizeof(M4x4),
                                    senderTransmissionState.spotterID);
                        
                        applicationState.status = ApplicationStatus_Detecting;
                        
                        switchLEDsOff(&brightPi);
                        switchIROn(&brightPi);
                    }
                    else
                    {
                        if (applicationState.poseEstimationIndex == 0 || 
                            (applicationState.timeOfLastCapture + POSE_ESTIMATION_TIMEOUT_MS) < getWallclockTimeInMs())
                        {
                            M4x4inv cTw = {};
                            bool32 poseEstimated = CV_estimatePose(&flushArena,
                                                                   &applicationState.calibration,
                                                                   frame,
                                                                   &cTw);
                            
                            if (poseEstimated)
                            {
                                applicationState.poseEstimations[applicationState.poseEstimationIndex]
                                    = cTw;
                                applicationState.poseEstimationIndex++;
                                
                                printf("Pose estimation %i\n", applicationState.poseEstimationIndex);
                                printM4x4(&cTw.inv);
                                printf("-------------------\n");
                                
                                if (applicationState.poseEstimationIndex >= POSE_ESTIMATION_COUNT)
                                {
                                    M4x4inv cTw = {};
                                    
                                    r32 oneOverPoseEstimationCount = 1.0f / POSE_ESTIMATION_COUNT;
                                    
                                    for (i32 i = 0;
                                         i < POSE_ESTIMATION_COUNT;
                                         i++)
                                    {
                                        M4x4inv* m = &applicationState.poseEstimations[i];
                                        
                                        for (i32 col = 0;
                                             col < 4;
                                             col++)
                                        {
                                            for (i32 row = 0;
                                                 row < 4;
                                                 row++)
                                            {
                                                cTw.fwd.e[col][row] += m->fwd.e[col][row];
                                                cTw.inv.e[col][row] += m->inv.e[col][row];
                                            }
                                        }
                                    }
                                    
                                    for (i32 col = 0;
                                         col < 4;
                                         col++)
                                    {
                                        for (i32 row = 0;
                                             row < 4;
                                             row++)
                                        {
                                            cTw.fwd.e[col][row] *= oneOverPoseEstimationCount;
                                            cTw.inv.e[col][row] *= oneOverPoseEstimationCount;
                                        }
                                    }
                                    
                                    applicationState.cTw = cTw;
                                    
                                    printf("Pose estimated with chessboard of size %f\n",
                                           CHESSBOARD_FIELD_EDGE_IN_CM);
                                    printM4x4(&applicationState.cTw.fwd);
                                    printf("-------------------\n");
                                    
                                    V4 cC = v4(0.0f, 0.0f, 0.0f, 1.0f);
                                    V4 wC = multM4x4V4(applicationState.cTw.inv,
                                                       cC);
                                    applicationState.cameraOrigin = v3(wC.x, wC.y, wC.z);
                                    
                                    printf("Sending camera pose: \n");
                                    printM4x4(&applicationState.cTw.inv);
                                    
                                    printf("-------------------\n");
                                    sendMessage(&flushArena,
                                                &senderTransmissionState, 
                                                MessageType_DebugCameraPose,
                                                &applicationState.cTw.inv,
                                                sizeof(M4x4),
                                                senderTransmissionState.spotterID);
                                    
                                    writeToFile("pose.spot",
                                                &applicationState.cTw,
                                                sizeof(applicationState.cTw));
                                    
                                    applicationState.status = ApplicationStatus_Detecting;
                                    
                                    switchLEDsOff(&brightPi);
                                    switchIROn(&brightPi);
                                    
                                    applicationState.timeOfLastCapture = getWallclockTimeInMs();
                                }
                            }
                        }
                    }
                } break;
                
                case ApplicationStatus_Detecting: {
                    BlobVector blobVector = CV_detectBlobs(&flushArena,
                                                           frame,
                                                           applicationState.binarizationThreshold);
                    i32 pointCount = blobVector.count;
                    if (pointCount)
                    {
                        //printf("%i blobs detected\n", pointCount);
                        
                        payloadSize = pointCount * sizeof(Ray);
                        payload = pushSize(&flushArena, payloadSize);
                        
                        Ray* rays = (Ray*)payload;
                        Ray* rayPtr = rays;
                        
                        V2* undistPoints =
                            (V2*)pushSize(&flushArena, sizeof(V2) * pointCount);
                        
                        CV_undistortPoints(blobVector.blobs, 
                                           undistPoints, 
                                           pointCount, 
                                           &applicationState.calibration);
                        
                        V2* pointPtr = undistPoints;
                        for (i32 i = 0; i < pointCount; i++)
                        {
                            Ray ray = castCameraToWorld(pointPtr,
                                                        &applicationState.cameraOrigin,
                                                        &applicationState.cTw.inv,
                                                        &applicationState.calibration);
                            *rayPtr = ray;
                            rayPtr++;
                            pointPtr++;
                        }
                    }
                } break;
                
                default: {
                } break;
            }
            
            sendMessage(&flushArena,
                        &senderTransmissionState,
                        MessageType_Payload,
                        payload, payloadSize,
                        senderTransmissionState.spotterID);
            
            applicationState.grabFrame = 0;
            
            u64 endHandlingTime = getWallclockTimeInMs();
            handlingTime = endHandlingTime - startHandlingTime;
            
            if (saveFramesToFile)
            {
                writeToFile(captureState.saveFileDescriptor,
                            frame->memory,
                            frame->pitch * frame->height);
            }
            
            if (!readFramesFromFile)
            {
                requeueBuffer(&captureState);
            }
        }
        
        u64 endTime = getWallclockTimeInMs();
        u64 dT = endTime - startTime;
#if 0
        printf("dT: %" PRIu64 "\n"
               "\tmessage:  %" PRIu64 "\n"
               "\tcapture: %" PRIu64 "\n"
               "\tsending: %" PRIu64 "\n"
               "\thandling: %" PRIu64 "\n", 
               dT, 
               messageHandlingTime,
               captureTime,
               frameSendingTime,
               handlingTime);
#endif
        
        flushMemory(&flushArena);
    }
    
    closeTransmissionChannel(&senderTransmissionState);
    closeTransmissionChannel(&receiverTransmissionState);
    stopCapturing(&captureState);
    
    return 0;
}
