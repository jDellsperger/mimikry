#include "s_analyzation.h"

static void CV_undistortPoints(V2* cameraPoints, 
                               V2* undistPoints, 
                               i32 pointCount,
                               Calibration* calibration)
{
    cv::Mat cameraPointMat = cv::Mat(pointCount, 1, CV_32FC2, cameraPoints);
    cv::Mat undistPointMat = cv::Mat(pointCount, 1, CV_32FC2, undistPoints);
    
    cv::Mat cameraMat = cv::Mat::zeros(3, 3, CV_32F);
    cameraMat.at<r32>(0, 0) = calibration->fx;
    cameraMat.at<r32>(1, 1) = calibration->fy;
    cameraMat.at<r32>(0, 2) = calibration->cx;
    cameraMat.at<r32>(1, 2) = calibration->cy;
    cameraMat.at<r32>(2, 2) = 1.0f;
    
    cv::Mat distortionMat = cv::Mat(5, 1, CV_32F);
    for (i32 i = 0; i < 5; i++)
    {
        distortionMat.at<r32>(i, 0) = calibration->distortion[i];
    }
    
    cv::undistortPoints(cameraPointMat,
                        undistPointMat,
                        cameraMat,
                        distortionMat);
}

// NOTE(jan): expects an undistorted point
static V3 projectCameraToWorld(V2* cameraPoint,
                               M4x4* wTc,
                               Calibration* calibration)
{
    V3 result;
    
    V4 cP;
    cP.x = cameraPoint->x;
    cP.y = cameraPoint->y;
    cP.z = 1.0f;
    cP.w = 1.0f;
    
    V4 wP = multM4x4V4(*wTc, cP);
    
    // NOTE(jan): perspective division not necessary, as w is always 1
    result = v3(wP.x, wP.y, wP.z);
    
    return result;
}

static Ray castCameraToWorld(V2* cameraPoint,
                             V3* wC,
                             M4x4* wTc,
                             Calibration* calibration)
{
    Ray result = {};
    
    V3 wP = projectCameraToWorld(cameraPoint, wTc, calibration);
    result.origin = *wC;
    result.direction = normalizeV3(subV3(wP, *wC));
    
    return result;
}

static V2 projectWorldToCamera(V3* worldPoint,
                               M4x4* cTw,
                               Calibration* calibration)
{
    V2 result = {};
    
    V4 homogeneousWorldPoint = v4(*worldPoint, 1.0f);
    V4 homogeneousCameraPoint = multM4x4V4(*cTw, homogeneousWorldPoint);
    
    // NOTE(jan): perspective division not necessary, as w is always 1
    V3 cameraPoint = {
        homogeneousCameraPoint.x,
        homogeneousCameraPoint.y,
        homogeneousCameraPoint.z
    };
    
    M3x3 intrinsic = {
        (r32)calibration->fx, 0.0f, 0.0f,
        0.0f, (r32)calibration->fy, 0.0f,
        (r32)calibration->cx,
        (r32)calibration->cy,
        1.0f
    };
    V3 unprojectedPoint = multM3x3V3(intrinsic, cameraPoint);
    
    if (unprojectedPoint.z > 0.001f)
    {
        result.x = unprojectedPoint.x / unprojectedPoint.z;
        result.y = unprojectedPoint.y / unprojectedPoint.z;
    }
    
    return result;
}

static void calculateChessboardWorldPoints(r32 chessboardFieldEdgeInCm,
                                           i32 innerRowCount, i32 innerColumnCount,
                                           void* worldPoints)
{
    V3* point = (V3*)worldPoints;
    
    for (i32 row = 0;
         row < innerRowCount;
         row++)
    {
        for (i32 col = 0;
             col < innerColumnCount;
             col++)
        {
            point->x = (r32)col * chessboardFieldEdgeInCm;
            point->y = (r32)row * chessboardFieldEdgeInCm;
            point->z = 0;
            
            point++;
        }
    }
}

static bool32 CV_findChessboardCorners(Frame* grayscaleFrame,
                                       i32 innerRowCount, i32 innerColumnCount,
                                       void* corners)
{
    bool32 result = 0;
    
    i32 cornerCount = innerRowCount * innerColumnCount;
    cv::Size chessboardSize = cv::Size(innerColumnCount, innerRowCount);
    
    cv::Mat img = cv::Mat(grayscaleFrame->height,
                          grayscaleFrame->width,
                          CV_8UC1,
                          grayscaleFrame->memory,
                          grayscaleFrame->pitch);
    cv::Mat cameraPoints = cv::Mat(cornerCount, 1, CV_32FC2, corners);
    bool32 cornersFound = cv::findChessboardCorners(img,
                                                    chessboardSize,
                                                    cameraPoints,
                                                    cv::CALIB_CB_ADAPTIVE_THRESH |
                                                    cv::CALIB_CB_NORMALIZE_IMAGE |
                                                    cv::CALIB_CB_FAST_CHECK);
    
    if (cornersFound)
    {
        cv::cornerSubPix(img, 
                         cameraPoints,
                         cv::Size(11, 11),
                         cv::Size(-1, -1),
                         cv::TermCriteria(cv::TermCriteria::EPS + 
                                          cv::TermCriteria::COUNT, 
                                          30000, 
                                          0.01));
        result = 1;
    }
    
    return result;
}

static bool32 CV_estimatePose(MemoryArena* arena,
                              Calibration* calibration,
                              Frame* grayscaleFrame,
                              M4x4inv* cTw)
{
    bool32 result = 0;
    
    i32 innerColumnCount = CHESSBOARD_INNER_COLUMN_COUNT;
    i32 innerRowCount = CHESSBOARD_INNER_ROW_COUNT;
    r32 chessboardSizeInCm = CHESSBOARD_FIELD_EDGE_IN_CM;
    i32 cornerCount = innerColumnCount * innerRowCount;
    
    cv::Mat cameraPoints = cv::Mat(cornerCount, 1, CV_32FC2);
    bool32 cornersFound = 
        CV_findChessboardCorners(grayscaleFrame,
                                 innerRowCount, innerColumnCount,
                                 &cameraPoints.at<cv::Point2f>(0, 0));
    
    if (cornersFound)
    {
        cv::Mat cameraMat = cv::Mat::zeros(3, 3, CV_32F);
        cameraMat.at<r32>(0, 0) = calibration->fx;
        cameraMat.at<r32>(1, 1) = calibration->fy;
        cameraMat.at<r32>(0, 2) = calibration->cx;
        cameraMat.at<r32>(1, 2) = calibration->cy;
        cameraMat.at<r32>(2, 2) = 1.0f;
        
        // Calculate world points
        cv::Mat worldPoints = cv::Mat(cornerCount, 1, CV_32FC3);
        calculateChessboardWorldPoints(chessboardSizeInCm,
                                       innerRowCount, innerColumnCount,
                                       &worldPoints.at<cv::Point3f>(0, 0));
        
        cv::Mat rot;
        cv::Mat trans;
        
        cv::Mat distortionMat = cv::Mat(5, 1, CV_32F);
        for (i32 i = 0; i < 5; i++)
        {
            distortionMat.at<r32>(i, 0) = calibration->distortion[i];
        }
        
        bool32 poseEstimated = cv::solvePnP(worldPoints, 
                                            cameraPoints, 
                                            cameraMat, 
                                            distortionMat,
                                            rot, 
                                            trans);
        
        cv::Mat rotEuler = cv::Mat(3, 3, CV_32F);
        cv::Rodrigues(rot, rotEuler);
        
        M4x4 invRot = identityM4x4();
        M4x4 invTrans = identityM4x4();
        
        if (poseEstimated)
        {
            for (i32 col = 0; col < 3; col++)
            {
                for (i32 row = 0; row < 3; row++)
                {
                    r32 r = rotEuler.at<r32>(row, col);
                    cTw->fwd.e[col][row] = r;
                    // NOTE(jan): inverse of rotation matrix is its transpose
                    invRot.e[row][col] = r;
                }
            }
            
            for (i32 row = 0; row < 3; row++)
            {
                r32 t = trans.at<r32>(row, 0);
                cTw->fwd.e[3][row] = t;
                invTrans.e[3][row] = -1.0f * t;
            }
            
            cTw->fwd.e[3][3] = 1.0f;
            
            cTw->inv = multM4x4(invRot, invTrans);
            cTw->inv.e[3][3] = 1.0f;
            
            result = 1;
        }
    }
    
    return result;
}

static void CV_resizeFrame(Frame* inputFrame,
                           Frame* outputFrame)
{
    cv::Mat inputFrameMat = cv::Mat(inputFrame->height,
                                    inputFrame->width,
                                    CV_8UC1,
                                    inputFrame->memory,
                                    inputFrame->pitch);
    cv::Mat outputMat = cv::Mat(outputFrame->height,
                                outputFrame->width,
                                CV_8UC1,
                                outputFrame->memory,
                                outputFrame->pitch);
    
    cv::resize(inputFrameMat,
               outputMat,
               cv::Size(outputFrame->width, outputFrame->height));
    
    assert((i32)*outputMat.step.p == outputFrame->pitch);
}

static void CV_convertCaptureFrameToGrayscale(Frame* captureFrame,
                                              Frame* outputFrame)
{
    cv::Mat captureFrameMat = cv::Mat(captureFrame->height,
                                      captureFrame->width,
                                      CV_8UC2,
                                      captureFrame->memory,
                                      captureFrame->pitch);
    cv::Mat outputFrameMat = cv::Mat(outputFrame->height,
                                     outputFrame->width,
                                     CV_8UC1,
                                     outputFrame->memory,
                                     outputFrame->pitch);
    
    int from_to[] = {0,0};
    cv::mixChannels(&captureFrameMat, 1, &outputFrameMat, 1, from_to, 1);
}

static void floodFill(MemoryArena* arena,
                      Frame* binarizedFrame,
                      i32 u, i32 v,
                      u8 filledValue, 
                      u8 label,
                      Region* r)
{
    TemporaryMemory tempMem = beginTemporaryMemory(arena);
    V2* q = pushStruct(arena, V2);
    V2* first = q;
    first->x = u;
    first->y = v;
    
    i32 maxX = u;
    i32 maxY = v;
    i32 minX = u;
    i32 minY = v;
    
    u32 stackSize = 1;
    u32 regionSize = 1;
    while (stackSize)
    {
        stackSize--;
        V2* p = q++;
        
        i32 x = p->x;
        i32 y = p->y;
        if (x >= 0 && x < binarizedFrame->width &&
            y >= 0 && y < binarizedFrame->height)
        {
            u8* val = (u8*)binarizedFrame->memory + y*binarizedFrame->pitch + x*binarizedFrame->bytesPerPixel;
            if (*val == filledValue)
            {
                maxX = max(x, maxX);
                maxY = max(y, maxY);
                minX = min(x, minX);
                minY = min(y, minY);
                
                *val = label;
                V2* pN = pushStruct(arena, V2);
                pN->x = x+1;
                pN->y = y;
                stackSize++;
                pN = pushStruct(arena, V2);
                pN->x = x-1;
                pN->y = y;
                stackSize++;
                pN = pushStruct(arena, V2);
                pN->x = x;
                pN->y = y+1;
                stackSize++;
                pN = pushStruct(arena, V2);
                pN->x = x;
                pN->y = y-1;
                stackSize++;
                
                regionSize++;
            }
        }
    }
    
    r->size = regionSize;
    r->label = label;
    r->max = {maxX / (r32)binarizedFrame->width, maxY / (r32)binarizedFrame->height};
    r->min = {minX / (r32)binarizedFrame->width, minY / (r32)binarizedFrame->height};
    
    r->width =
        (maxX - minX) / (r32)binarizedFrame->width;
    r->height =
        (maxY - minY) / (r32)binarizedFrame->height;
    
    r->center.x = (maxX + minX) / 2.0f;
    r->center.y = (maxY + minY) / 2.0f;
    
    endTemporaryMemory(tempMem);
}

static RegionLabelingResult regionLabeling(MemoryArena* arena,
                                           Frame* binarizedFrame)
{
    RegionLabelingResult result = {};
    
    u8 label = 250;
    
    u8* row = (u8*)binarizedFrame->memory;
    i32 regionIndex = 0;
    for (i32 y = 0; y < binarizedFrame->height; y++)
    {
        u8* pixel = row;
        for (i32 x = 0; x < binarizedFrame->width; x++)
        {
            if (*pixel == 255)
            {
                Region* region = pushStruct(arena, Region);
                
                if (!result.region)
                {
                    result.region = region;
                }
                
                floodFill(arena, binarizedFrame, x, y, 255, label, region);
                result.regionCount = result.regionCount + 1;
            }
            
            pixel += binarizedFrame->bytesPerPixel;
        }
        
        row += binarizedFrame->pitch;
    }
    
    return result;
}

static BlobVector CV_detectBlobs(MemoryArena* arena,
                                 Frame* inputFrame,
                                 i32 threshold)
{
    BlobVector result = initializeBlobVector(arena,
                                             20);
    
    cv::Mat inputFrameMat = cv::Mat(inputFrame->height,
                                    inputFrame->width,
                                    CV_8UC1,
                                    inputFrame->memory,
                                    inputFrame->pitch);
    
    cv::SimpleBlobDetector::Params params;
    
    params.minThreshold = threshold;
    params.maxThreshold = threshold + 1;
    params.minRepeatability = 1;
    
    params.filterByColor = 1;
    params.blobColor = 255;
    
    params.filterByCircularity = 0;
    //params.minCircularity = 0.7f;
    
    params.filterByInertia = 0;
    params.filterByConvexity = 0;
    
    params.filterByArea = 1;
    params.minArea = 1;
    params.maxArea = 500;
    
    // have to use cv::Ptr because OpenCV hates raw pointers... >.<
    cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(params);
    
    std::vector<cv::KeyPoint> keypoints;
    detector->detect(inputFrameMat, keypoints);
    
    for (cv::KeyPoint keypoint : keypoints)
    {
        pushBlob(arena,
                 &result,
                 v2(keypoint.pt.x, keypoint.pt.y));
    }
    
    return result;
}

static void CV_binarize(Frame* inputFrame,
                        Frame* outputFrame,
                        i32 threshold)
{
    assert(inputFrame->bytesPerPixel == 1);
    assert(outputFrame->bytesPerPixel == 1);
    
    cv::Mat inputFrameMat = cv::Mat(inputFrame->height,
                                    inputFrame->width,
                                    CV_8UC1,
                                    inputFrame->memory,
                                    inputFrame->pitch);
    cv::Mat outputFrameMat = cv::Mat(outputFrame->height,
                                     outputFrame->width,
                                     CV_8UC1,
                                     outputFrame->memory,
                                     outputFrame->pitch);
    
    cv::threshold(inputFrameMat,
                  outputFrameMat,
                  threshold,
                  255,
                  cv::THRESH_BINARY);
}

static void CV_convertYUYVtoYAndBinarize(MemoryArena* arena,
                                         Frame* inputFrame,
                                         Frame* grayscaleFrame,
                                         Frame* binarizedFrame,
                                         i32 threshold)
{
    CV_convertCaptureFrameToGrayscale(inputFrame,
                                      grayscaleFrame);
    CV_binarize(grayscaleFrame,
                binarizedFrame,
                threshold);
}

static void downSample(Frame* inputFrame,
                       Frame* outputFrame)
{
    assert(inputFrame->bytesPerPixel == 1);
    assert(outputFrame->bytesPerPixel == 1);
    
    i32 samplingRateX = inputFrame->width / outputFrame->width;
    i32 samplingRateY = inputFrame->height / outputFrame->height;
    
    u8* inputRow = (u8*)inputFrame->memory;
    u8* outputRow = (u8*)outputFrame->memory;
    
    r32 oneOverSamplingRate = 1.0f / (samplingRateX * samplingRateY);
    
    for (i32 y = 0; 
         y < outputFrame->height; 
         y++)
    {
        u8* inputPixel = inputRow;
        u8* outputPixel = outputRow;
        
        for (i32 x = 0; 
             x < outputFrame->width; 
             x++)
        {
            u32 inputVal = 0;
            for (i32 sampleY = 0;
                 sampleY < samplingRateY;
                 sampleY++)
            {
                for (i32 sampleX = 0;
                     sampleX < samplingRateX;
                     sampleX++)
                {
                    inputVal += *(inputPixel + 
                                  sampleY * inputFrame->pitch +
                                  sampleX * inputFrame->bytesPerPixel);
                }
            }
            
            u8 val = (r32)inputVal * oneOverSamplingRate;
            
            *outputPixel = val;
            
            inputPixel += (samplingRateX * inputFrame->bytesPerPixel);
            outputPixel += outputFrame->bytesPerPixel;
        }
        
        inputRow += (samplingRateY * inputFrame->pitch);
        outputRow += outputFrame->pitch;
    }
}

static void convertYUYVtoYAndBinarize(Frame* inputFrame,
                                      Frame* grayscaleFrame,
                                      Frame* binarizedFrame,
                                      i32 threshold)
{
    assert(binarizedFrame->bytesPerPixel == 1);
    assert(grayscaleFrame->bytesPerPixel == 1);
    assert(inputFrame->bytesPerPixel == 2);
    
    u8* inputRow = (u8*)inputFrame->memory;
    u8* binRow = (u8*)binarizedFrame->memory;
    u8* yRow = (u8*)grayscaleFrame->memory;
    
    for (i32 y = 0; 
         y < grayscaleFrame->height; 
         y++)
    {
        u8* inputPixel = inputRow;
        u8* binPixel = binRow;
        u8* yPixel = yRow;
        
        for (i32 x = 0; 
             x < grayscaleFrame->width; 
             x++)
        {
            u8 avg = (r32)(*inputPixel);
            u8 val = (avg < threshold) ? 0 : 255;
            
            *binPixel = val;
            *yPixel = avg;
            
            inputPixel += inputFrame->bytesPerPixel;
            binPixel += binarizedFrame->bytesPerPixel;
            yPixel += grayscaleFrame->bytesPerPixel;
        }
        
        inputRow += inputFrame->pitch;
        binRow += binarizedFrame->pitch;
        yRow += grayscaleFrame->pitch;
    }
}
