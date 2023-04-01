#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>

#define CALIBRATION_FRAME_COUNT 30
#define CHESSBOARD_INNER_COLUMN_COUNT 8
#define CHESSBOARD_INNER_ROW_COUNT 5
#define CHESSBOARD_FIELD_EDGE_IN_CM 4.25f

#include "../include/platform.h"
#include "../include/math.h"
#include "s_render.cpp"
#include "s_capture.cpp"
#include "s_analyzation.cpp"

enum ApplicationStatus
{
    ApplicationStatus_None,
    ApplicationStatus_Idle,
    ApplicationStatus_Calibrating,
    ApplicationStatus_Calibrated,
    ApplicationStatus_DisplayFrames,
    ApplicationStatus_Exiting
};

struct ApplicationState
{
    ApplicationStatus status;
    bool32 readNextFromFile;
    bool32 readPreviousFromFile;
};

struct CalibrationState
{
    i32 detectedFrames;
    std::vector<cv::Mat> imagePoints;
    Calibration calibration;
    bool32 takePicture;
    i32 pictureIndex;
    bool32 isCalibrated;
    
    bool32 displayLastFrame;
    bool32 saveLastFrame;
    cv::Mat lastImagePoints;
};

static bool32 CV_saveLastFrameAndCalibrate(CalibrationState* calibrationState,
                                           i32 innerRowCount,
                                           i32 innerColumnCount,
                                           i32 cornerCount)
{
    bool32 result = 0;
    
    calibrationState->imagePoints.push_back(calibrationState->lastImagePoints);
    calibrationState->detectedFrames++;
    
    if (calibrationState->detectedFrames >= CALIBRATION_FRAME_COUNT)
    {
        printf("Collected enough points, starting calibration\n");
        
        // Calculate world points
        r32 chessboardSizeInCm = CHESSBOARD_FIELD_EDGE_IN_CM;
        
        std::vector<std::vector<cv::Point3f>> allWorldPoints(1);
        allWorldPoints[0].resize(cornerCount);
        calculateChessboardWorldPoints(chessboardSizeInCm,
                                       innerRowCount, innerColumnCount,
                                       &allWorldPoints[0][0]);
        
        allWorldPoints.resize(CALIBRATION_FRAME_COUNT, allWorldPoints[0]);
        
        // Calibrate
        cv::Mat cameraMat = cv::Mat::zeros(3, 3, CV_64F);
        cv::Mat distortionMat = cv::Mat::zeros(5, 1, CV_64F);
        std::vector<cv::Mat> rvecs, tvecs;
        r64 reprojError = cv::calibrateCamera(allWorldPoints,
                                              calibrationState->imagePoints,
                                              cv::Size(WINDOW_WIDTH, 
                                                       WINDOW_HEIGHT),
                                              cameraMat,
                                              distortionMat,
                                              rvecs, tvecs);
        
        printf("reprojection error: %f\n", reprojError);
        r64 fx = cameraMat.at<r64>(0, 0);
        r64 fy = cameraMat.at<r64>(1, 1);
        r64 cx = cameraMat.at<r64>(0, 2);
        r64 cy = cameraMat.at<r64>(1, 2);
        printf("fx: %f, fy: %f, cx: %f, cy: %f\n",
               fx, fy, cx, cy);
        r64 k1 = distortionMat.at<r64>(0, 0);
        r64 k2 = distortionMat.at<r64>(1, 0);
        r64 p1 = distortionMat.at<r64>(2, 0);
        r64 p2 = distortionMat.at<r64>(3, 0);
        r64 k3 = distortionMat.at<r64>(4, 0);
        printf("distortion: %f, %f, %f, %f, %f\n",
               k1, k2, p1, p2, k3);
        
        calibrationState->calibration.fx = fx;
        calibrationState->calibration.fy = fy;
        calibrationState->calibration.cx = cx;
        calibrationState->calibration.cy = cy;
        calibrationState->calibration.distortion[0] = k1;
        calibrationState->calibration.distortion[1] = k2;
        calibrationState->calibration.distortion[2] = p1;
        calibrationState->calibration.distortion[3] = p2;
        calibrationState->calibration.distortion[4] = k3;
        
        calibrationState->isCalibrated = 1;
        
        result = 1;
    }
    
    return result;
}

static bool32 CV_findImagePoints(CalibrationState* calibrationState,
                                 Frame* grayscaleFrame,
                                 cv::Mat* imagePoints,
                                 i32 innerRowCount, i32 innerColumnCount,
                                 i32 cornerCount)
{
    bool32 result = 0;
    
    bool32 cornersFound = 
        CV_findChessboardCorners(grayscaleFrame, 
                                 innerRowCount, innerColumnCount,
                                 &imagePoints->at<cv::Point2f>(0, 0));
    
    if (cornersFound)
    {
        result = 1;
    }
    
    return result;
}

static void CV_drawChessboardCorners(RenderGroup* renderGroup,
                                     cv::Mat* cameraPoints,
                                     i32 cornerCount)
{
    // Rendering corners
    r32 cornerSize = 0.001f;
    for (i32 i = 0; i < cornerCount; i++)
    {
        cv::Point2f cameraPoint = cameraPoints->at<cv::Point2f>(i, 0);
        r32 x = cameraPoint.x / (r32)CAPTURE_FRAME_WIDTH;
        r32 y = cameraPoint.y / (r32)CAPTURE_FRAME_HEIGHT;
        
        V3 min = {x - cornerSize, y - cornerSize, 0.0f};
        V3 max = {x + cornerSize, y + cornerSize, 0.0f};
        
        if (i == 0)
        {
            pushRect(renderGroup, min, max, v4(0.0f, 1.0f, 0.0f, 1.0f));
        }
        else
        {
            pushRect(renderGroup, min, max, v4(1.0f, 0.0f, 0.0f, 1.0f));
        }
    }
    
    // Rendering corners
    cornerSize = 0.01f;
    for (i32 i = 0; i < cornerCount; i++)
    {
        cv::Point2f cameraPoint = cameraPoints->at<cv::Point2f>(i, 0);
        r32 x = cameraPoint.x / (r32)CAPTURE_FRAME_WIDTH;
        r32 y = cameraPoint.y / (r32)CAPTURE_FRAME_HEIGHT;
        
        V3 min = {x - cornerSize, y - cornerSize, 0.0f};
        V3 max = {x + cornerSize, y + cornerSize, 0.0f};
        
        if (i == 0)
        {
            pushRect(renderGroup, min, max, v4(0.0f, 1.0f, 0.0f, 1.0f));
        }
        else
        {
            pushRect(renderGroup, min, max, v4(1.0f, 0.0f, 0.0f, 1.0f));
        }
    }
}

static void handleInput(InputState* inputState, 
                        ApplicationState* applicationState,
                        CalibrationState* calibrationState)
{
    if (!inputState->buttonSpace.wasDown && inputState->buttonSpace.isDown)
    {
        printf("Taking picture\n");
        calibrationState->takePicture = 1;
        
        inputState->buttonSpace.wasDown = 1;
    }
    else if(inputState->buttonSpace.wasDown && !inputState->buttonSpace.isDown)
    {
        inputState->buttonSpace.wasDown = 0;
    }
    
    if (!inputState->buttonTab.wasDown && inputState->buttonTab.isDown)
    {
        if (applicationState->status == ApplicationStatus_DisplayFrames)
        {
            if (calibrationState->isCalibrated)
            {
                printf("Displaying camera feed\n");
                applicationState->status = ApplicationStatus_Calibrated;
                calibrationState->pictureIndex = 0;
            }
            else
            {
                printf("Starting calibration\n");
                applicationState->status = ApplicationStatus_Calibrating;
                *calibrationState = {};
            }
        }
        else
        {
            printf("Displaying frames\n");
            calibrationState->pictureIndex = 0;
            applicationState->status = ApplicationStatus_DisplayFrames;
            applicationState->readNextFromFile = 1;
        }
        
        inputState->buttonTab.wasDown = 1;
    }
    else if(inputState->buttonTab.wasDown && !inputState->buttonTab.isDown)
    {
        inputState->buttonTab.wasDown = 0;
    }
    
    if (!inputState->buttonReset.wasDown && inputState->buttonReset.isDown)
    {
        printf("Starting calibration\n");
        applicationState->status = ApplicationStatus_Calibrating;
        *calibrationState = {};
        
        inputState->buttonReset.wasDown = 1;
    }
    else if(inputState->buttonReset.wasDown && !inputState->buttonReset.isDown)
    {
        inputState->buttonReset.wasDown = 0;
    }
    
    if (!inputState->buttonEsc.wasDown && inputState->buttonEsc.isDown)
    {
        printf("Exiting\n");
        applicationState->status = ApplicationStatus_Exiting;
        
        inputState->buttonEsc.wasDown = 1;
    }
    else if (inputState->buttonEsc.wasDown && !inputState->buttonEsc.isDown)
    {
        inputState->buttonEsc.wasDown = 0;
    }
    
    if (!inputState->buttonLeft.wasDown && inputState->buttonLeft.isDown)
    {
        inputState->buttonLeft.wasDown = 1;
        applicationState->readPreviousFromFile = 1;
    }
    else if (inputState->buttonLeft.wasDown && !inputState->buttonLeft.isDown)
    {
        inputState->buttonLeft.wasDown = 0;
    }
    
    if (!inputState->buttonRight.wasDown && inputState->buttonRight.isDown)
    {
        applicationState->readNextFromFile = 1;
        inputState->buttonRight.wasDown = 1;
    }
    else if (inputState->buttonRight.wasDown && !inputState->buttonRight.isDown)
    {
        inputState->buttonRight.wasDown = 0;
    }
    
    if (!inputState->buttonYes.wasDown && inputState->buttonYes.isDown)
    {
        if (calibrationState->displayLastFrame)
        {
            printf("Saving frame\n");
            calibrationState->saveLastFrame = 1;
        }
        inputState->buttonYes.wasDown = 1;
    }
    else if (inputState->buttonYes.wasDown && !inputState->buttonYes.isDown)
    {
        inputState->buttonYes.wasDown = 0;
    }
    
    if (!inputState->buttonNo.wasDown && inputState->buttonNo.isDown)
    {
        if (calibrationState->displayLastFrame)
        {
            printf("Not saving frame\n");
            calibrationState->displayLastFrame = 0;
        }
        inputState->buttonNo.wasDown = 1;
    }
    else if (inputState->buttonNo.wasDown && !inputState->buttonNo.isDown)
    {
        inputState->buttonNo.wasDown = 0;
    }
}

i32 main(i32 argc, const char* argv[])
{
    printf("Calibrating camera with size %ix%i\n",
           CAPTURE_FRAME_WIDTH, CAPTURE_FRAME_HEIGHT);
    
    CalibrationState calibrationState = {};
    char calibrationFilePath[100];
    snprintf(calibrationFilePath,
             100,
             "calibrations/spotter_%ix%i.calib", 
             CAPTURE_FRAME_WIDTH, CAPTURE_FRAME_HEIGHT);
    ApplicationState applicationState = {};
    applicationState.status = ApplicationStatus_Calibrating;
    bool32 readFromFiles = 0;
    
    for (i32 i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-c") == 0)
        {
            snprintf(calibrationFilePath,
                     100,
                     "%s",
                     argv[i + 1]);
            i++;
            ReadFileResult calibrationFile;
            bool32 readResult = readEntireFile(calibrationFilePath,
                                               &calibrationFile);
            
            if (readResult)
            {
                if (calibrationFile.contentSize == sizeof(Calibration))
                {
                    printf("Calibration file %s read\n",
                           calibrationFilePath);
                    applicationState.status = ApplicationStatus_Calibrated;
                    calibrationState.calibration =
                        *((Calibration*)calibrationFile.content);
                    calibrationState.isCalibrated = 1;
                }
                
                freeFileMemory(&calibrationFile);
            }
            
            continue;
        }
        
        
        if (strcmp(argv[i], "-r") == 0)
        {
            readFromFiles = 1;
        }
    }
    
    void* baseAddress = (void*)terabytes(2);
    u64 permMemorySize = megabytes(100);
    u64 transMemorySize = megabytes(100);
    u64 totalMemorySize = permMemorySize + transMemorySize;
    u8* systemMemory = (u8*)mmap(baseAddress, 
                                 totalMemorySize,
                                 PROT_READ | PROT_WRITE,
                                 MAP_ANON | MAP_PRIVATE,
                                 -1, 0);
    
    MemoryArena permArena, transArena;
    initMemoryArena(&permArena, permMemorySize, (u8*)(systemMemory));
    initMemoryArena(&transArena, transMemorySize, 
                    (systemMemory + 
                     permMemorySize));
    
    CaptureState captureState = {};
    const char* devName = "/dev/video0";
    if (!startCapturing(&permArena,
                        &captureState,
                        CAPTURE_BUFFER_COUNT, 
                        devName))
    {
        printf("Could not initialize capture\n");
        return -1;
    }
    
    RenderState renderState = {};
    
    GLFWwindow* window = nullptr;
    
    i32 displayWidth = WINDOW_WIDTH + (WINDOW_WIDTH % 4);
    
    glfwInit();
    window = glfwCreateWindow(displayWidth, WINDOW_HEIGHT,
                              "Spotter Calibration", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if (!initRenderer(&renderState, displayWidth, WINDOW_HEIGHT))
    {
        return -1;
    }
    
    InputState inputState = {};
    
    Frame* frame = 0;
    Frame grayscaleFrame = initializeFrame(&permArena,
                                           WINDOW_WIDTH, WINDOW_HEIGHT,
                                           1);
    Frame displayFrame = initializeFrame(&permArena,
                                         displayWidth, WINDOW_HEIGHT,
                                         1);
    
    i32 innerColumnCount = CHESSBOARD_INNER_COLUMN_COUNT;
    i32 innerRowCount = CHESSBOARD_INNER_ROW_COUNT;
    i32 cornerCount = innerRowCount * innerColumnCount;
    
    if (readFromFiles)
    {
        Frame fullGrayscaleFrame = initializeFrame(&permArena,
                                                   CAPTURE_FRAME_WIDTH, CAPTURE_FRAME_HEIGHT,
                                                   1);
        frame = &fullGrayscaleFrame;
        
        for (i32 i = 0; 
             i < CALIBRATION_FRAME_COUNT;
             i++)
        {
            ReadFileResult frameFile;
            char filename[100];
            snprintf(filename,
                     100,
                     "calibration_frame_%ix%i_%i.calibFrame", 
                     fullGrayscaleFrame.width,
                     fullGrayscaleFrame.height,
                     i);
            printf("Reading calibration frame %s\n", filename);
            readEntireFile(filename,
                           &frameFile);
            readFromFileResult(&frameFile,
                               fullGrayscaleFrame.memory,
                               fullGrayscaleFrame.size);
            
            freeFileMemory(&frameFile);
            
            cv::Mat imagePoints = cv::Mat(cornerCount, 1, CV_32FC2);
            bool32 imagePointsFound =CV_findImagePoints(&calibrationState,
                                                        &fullGrayscaleFrame,
                                                        &imagePoints,
                                                        innerRowCount,
                                                        innerColumnCount,
                                                        cornerCount);
            
            if (imagePointsFound)
            {
                calibrationState.lastImagePoints = imagePoints;
                bool32 calibrated = CV_saveLastFrameAndCalibrate(&calibrationState,
                                                                 innerRowCount,
                                                                 innerColumnCount,
                                                                 cornerCount);
                
                if (calibrated)
                {
                    applicationState.status = ApplicationStatus_DisplayFrames;
                    
                    printf("Camera calibrated\n");
                    
                    applicationState.status = ApplicationStatus_Calibrated;
                    writeToFile(calibrationFilePath, 
                                &calibrationState.calibration,
                                sizeof(Calibration));
                }
            }
        }
    }
    
    while (applicationState.status != ApplicationStatus_Exiting)
    {
        RenderGroup* renderGroup = createRenderGroup(&transArena, 
                                                     &renderState,
                                                     megabytes(5));
        pushClear(renderGroup);
        
        switch (applicationState.status)
        {
            case ApplicationStatus_Calibrating: {
                bool32 calibrated = 0;
                
                if (!calibrationState.displayLastFrame)
                {
                    readFrame(&captureState, &frame);
                    CV_resizeFrame(frame,
                                   &grayscaleFrame);
                    
                    u8* inputRow = (u8*)grayscaleFrame.memory;
                    u8* outputRow = (u8*)displayFrame.memory;
                    for (i32 y = 0; 
                         y < grayscaleFrame.height; 
                         y++)
                    {
                        memcpy(outputRow, inputRow, grayscaleFrame.pitch);
                        
                        inputRow += grayscaleFrame.pitch;
                        outputRow += displayFrame.pitch;
                    }
                    
                    pushImageBuffer(renderGroup,
                                    displayFrame.memory,
                                    displayFrame.width,
                                    displayFrame.height,
                                    displayFrame.pitch,
                                    displayFrame.bytesPerPixel);
                    
                    if (calibrationState.takePicture)
                    {
                        cv::Mat imagePoints = cv::Mat(cornerCount, 1, CV_32FC2);
                        bool32 imagePointsFound = CV_findImagePoints(&calibrationState,
                                                                     frame,
                                                                     &imagePoints,
                                                                     innerRowCount,
                                                                     innerColumnCount,
                                                                     cornerCount);
                        
                        if (imagePointsFound)
                        {
                            CV_drawChessboardCorners(renderGroup,
                                                     &imagePoints,
                                                     cornerCount);
                            
                            calibrationState.displayLastFrame = 1;
                            calibrationState.lastImagePoints = imagePoints;
                            calibrationState.takePicture = 0;
                        }
                    }
                    
                    requeueBuffer(&captureState);
                }
                else
                {
                    u8* inputRow = (u8*)grayscaleFrame.memory;
                    u8* outputRow = (u8*)displayFrame.memory;
                    for (i32 y = 0; 
                         y < grayscaleFrame.height; 
                         y++)
                    {
                        memcpy(outputRow, inputRow, grayscaleFrame.pitch);
                        
                        inputRow += grayscaleFrame.pitch;
                        outputRow += displayFrame.pitch;
                    }
                    
                    pushImageBuffer(renderGroup,
                                    displayFrame.memory,
                                    displayFrame.width,
                                    displayFrame.height,
                                    displayFrame.pitch,
                                    displayFrame.bytesPerPixel);
                    CV_drawChessboardCorners(renderGroup,
                                             &calibrationState.lastImagePoints,
                                             cornerCount);
                    
                    if (calibrationState.saveLastFrame)
                    {
                        printf("saving frame %i / %i\n",
                               calibrationState.detectedFrames,
                               CALIBRATION_FRAME_COUNT);
                        
                        char filename[100];
                        snprintf(filename,
                                 100,
                                 "calibration_frame_%ix%i_%i.calibFrame", 
                                 frame->width,
                                 frame->height,
                                 calibrationState.pictureIndex);
                        bool32 writeResult = writeToFile(filename,
                                                         frame->memory,
                                                         frame->size);
                        
                        if (writeResult)
                        {
                            printf("frame written\n");
                        }
                        
                        calibrationState.pictureIndex++;
                        calibrationState.saveLastFrame = 0;
                        calibrationState.displayLastFrame = 0;
                        
                        calibrated = CV_saveLastFrameAndCalibrate(&calibrationState,
                                                                  innerRowCount,
                                                                  innerColumnCount,
                                                                  cornerCount);
                        
                        calibrationState.displayLastFrame = 0;
                    }
                }
                
                if (calibrated)
                {
                    printf("Camera calibrated\n");
                    
                    applicationState.status = ApplicationStatus_Calibrated;
                    writeToFile(calibrationFilePath, 
                                &calibrationState.calibration,
                                sizeof(Calibration));
                }
            } break;
            
            case ApplicationStatus_Calibrated: {
                
                Frame* frame = 0;
                readFrame(&captureState, &frame);
                
                CV_resizeFrame(frame,
                               &grayscaleFrame);
                
                u8* inputRow = (u8*)grayscaleFrame.memory;
                u8* outputRow = (u8*)displayFrame.memory;
                for (i32 y = 0; 
                     y < grayscaleFrame.height; 
                     y++)
                {
                    memcpy(outputRow, inputRow, grayscaleFrame.pitch);
                    
                    inputRow += grayscaleFrame.pitch;
                    outputRow += displayFrame.pitch;
                }
                
                pushImageBuffer(renderGroup,
                                displayFrame.memory,
                                displayFrame.width,
                                displayFrame.height,
                                displayFrame.pitch,
                                displayFrame.bytesPerPixel);
                
                cv::Mat imagePoints = cv::Mat(cornerCount, 1, CV_32FC2);
                bool32 imagePointsFound =CV_findImagePoints(&calibrationState,
                                                            frame,
                                                            &imagePoints,
                                                            innerRowCount,
                                                            innerColumnCount,
                                                            cornerCount);
                
                if (imagePointsFound)
                {
                    CV_drawChessboardCorners(renderGroup,
                                             &imagePoints,
                                             cornerCount);
                    
                    M4x4inv cTw = {};
                    CV_estimatePose(&transArena,
                                    &calibrationState.calibration,
                                    frame,
                                    &cTw);
                    printM4x4(&cTw.fwd);
                    printf("---------------\n");
                }
                
                requeueBuffer(&captureState);
            } break;
            
            case ApplicationStatus_DisplayFrames: {
                if (applicationState.readNextFromFile)
                {
                    calibrationState.pictureIndex =
                        (calibrationState.pictureIndex + 1) % CALIBRATION_FRAME_COUNT;
                    
                    ReadFileResult frameFile;
                    char filename[100];
                    snprintf(filename,
                             100,
                             "calibration_frame_%ix%i_%i.calibFrame", 
                             frame->width,
                             frame->height,
                             calibrationState.pictureIndex);
                    readEntireFile(filename,
                                   &frameFile);
                    readFromFileResult(&frameFile,
                                       frame->memory,
                                       frame->size);
                    
                    freeFileMemory(&frameFile);
                    
                    applicationState.readNextFromFile = 0;
                }
                else if (applicationState.readPreviousFromFile)
                {
                    calibrationState.pictureIndex--;
                    if (calibrationState.pictureIndex < 0)
                    {
                        calibrationState.pictureIndex = CALIBRATION_FRAME_COUNT - 1;
                    }
                    
                    ReadFileResult frameFile;
                    char filename[100];
                    snprintf(filename,
                             100,
                             "calibration_frame_%ix%i_%i.calibFrame", 
                             frame->width,
                             frame->height,
                             calibrationState.pictureIndex);
                    readEntireFile(filename,
                                   &frameFile);
                    readFromFileResult(&frameFile,
                                       frame->memory,
                                       frame->size);
                    
                    freeFileMemory(&frameFile);
                    
                    applicationState.readPreviousFromFile = 0;
                }
                
                u8* inputRow = (u8*)grayscaleFrame.memory;
                u8* outputRow = (u8*)displayFrame.memory;
                for (i32 y = 0; 
                     y < grayscaleFrame.height; 
                     y++)
                {
                    memcpy(outputRow, inputRow, grayscaleFrame.pitch);
                    
                    inputRow += grayscaleFrame.pitch;
                    outputRow += displayFrame.pitch;
                }
                
                pushImageBuffer(renderGroup,
                                displayFrame.memory,
                                displayFrame.width,
                                displayFrame.height,
                                displayFrame.pitch,
                                displayFrame.bytesPerPixel);
                
                cv::Mat imagePoints = cv::Mat(cornerCount, 1, CV_32FC2);
                bool32 imagePointsFound = CV_findImagePoints(&calibrationState,
                                                             frame,
                                                             &imagePoints,
                                                             innerRowCount,
                                                             innerColumnCount,
                                                             cornerCount);
                if (imagePointsFound)
                {
                    CV_drawChessboardCorners(renderGroup,
                                             &imagePoints,
                                             cornerCount);
                }
            } break;
        }
        
        drawRenderGroup(renderGroup);
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        i32 keyState = glfwGetKey(window, GLFW_KEY_SPACE);
        inputState.buttonSpace.isDown = (keyState == GLFW_PRESS);
        keyState = glfwGetKey(window, GLFW_KEY_TAB);
        inputState.buttonTab.isDown = (keyState == GLFW_PRESS);
        keyState = glfwGetKey(window, GLFW_KEY_ESCAPE);
        inputState.buttonEsc.isDown = (keyState == GLFW_PRESS);
        keyState = glfwGetKey(window, GLFW_KEY_R);
        inputState.buttonReset.isDown = (keyState == GLFW_PRESS);;
        keyState = glfwGetKey(window, GLFW_KEY_RIGHT);
        inputState.buttonRight.isDown = (keyState == GLFW_PRESS);
        keyState = glfwGetKey(window, GLFW_KEY_LEFT);
        inputState.buttonLeft.isDown = (keyState == GLFW_PRESS);
        keyState = glfwGetKey(window, GLFW_KEY_Y);
        inputState.buttonYes.isDown = (keyState == GLFW_PRESS);
        keyState = glfwGetKey(window, GLFW_KEY_N);
        inputState.buttonNo.isDown = (keyState == GLFW_PRESS);
        
        handleInput(&inputState, 
                    &applicationState, 
                    &calibrationState);
        
        if (glfwWindowShouldClose(window))
        {
            applicationState.status = ApplicationStatus_Exiting;
        }
        
        flushMemory(&transArena);
    }
    
    stopCapturing(&captureState);
    
    glfwTerminate();
    
    return 0;
}
