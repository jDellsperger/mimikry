#ifndef ANALYZATION_H
#define ANALYZATION_H

#include <opencv2/opencv.hpp>

struct Region
{
    V2 origin;
    V2 max;
    V2 min;
    V2 center;
    r32 width, height;
    u8 label;
    i32 size;
};

struct RegionLabelingResult
{
    Region* region;
    i32 regionCount;
};

struct Calibration
{
    r64 fx, fy, cx, cy;
    union
    {
        struct
        {
            r64 k1, k2, p1, p2, k3;
        };
        r64 distortion[5];
    };
};

struct BlobVector
{
    V2* blobs;
    i32 count;
    i32 maxCount;
};

static inline BlobVector initializeBlobVector(MemoryArena* arena,
                                              i32 maxCount)
{
    BlobVector result = {};
    
    result.blobs = (V2*)pushSize(arena, sizeof(V2) * maxCount);
    result.maxCount = maxCount;
    
    return result;
}

static inline void pushBlob(MemoryArena* arena,
                            BlobVector* vector,
                            V2 blob)
{
    if(vector->count >= vector->maxCount)
    {
        V2* oldBlobs = vector->blobs;
        i32 oldMaxCount = vector->maxCount;
        
        vector->maxCount *= 2;
        vector->blobs = 
            (V2*)pushSize(arena, vector->maxCount * sizeof(V2));
        memcpy(vector->blobs, 
               oldBlobs, 
               oldMaxCount * sizeof(V2));
    }
    
    V2* blobPtr = &vector->blobs[vector->count];
    *blobPtr = blob;
    
    vector->count++;
}

#endif

