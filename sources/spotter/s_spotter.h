#ifndef SPOTTER_H

enum ApplicationStatus
{
    ApplicationStatus_None,
    ApplicationStatus_Idle,
    ApplicationStatus_Calibrating,
    ApplicationStatus_EstimatingPose,
    //ApplicationStatus_DrawChessboardRays,
    ApplicationStatus_Detecting,
    ApplicationStatus_Exiting
};

struct ApplicationState
{
    ApplicationStatus status;
    std::string ip;
    
    bool32 grabFrame = 0;
    
    Frame grayscaleFrame;
    Frame binarizedFrame;
    u8 binarizationThreshold;
    
    u32 windowWidth;
    u32 windowHeight;
    
    M4x4inv cTw;
    Calibration calibration;
    V3 cameraOrigin = {};
    
    M4x4inv poseEstimations[POSE_ESTIMATION_COUNT];
    i32 poseEstimationIndex = 0;
    u64 timeOfLastCapture;
    
    bool32 poseLoadedFromFile = 0;
};

#define SPOTTER_H
#endif

