#include "../include/platform.h"
#include "../include/math.h"
#include "b_transmission.cpp"
#include "beholder.cpp"

i32 main(i32 argc, const char* argv[])
{
    void* baseAddress = (void*)terabytes(2);
    u64 memorySize = megabytes(5);
    u8* systemMemory = (u8*)mmap(baseAddress, 
                                 memorySize,
                                 PROT_READ | PROT_WRITE,
                                 MAP_ANON | MAP_PRIVATE,
                                 -1, 0);
    
    MemoryArena memoryArena;
    initMemoryArena(&memoryArena, memorySize,
                    (systemMemory));
    
    TransmissionState sendTransmissionState = {};
    initTransmissionState(&sendTransmissionState);
    
    openPushTransmissionChannel(&sendTransmissionState,
                                "localhost",
                                "5556",
                                0);
    
    IntersectionResult intersections = {};
    intersections.intersectionCount = 5;
    intersections.firstIntersection =
        (Intersection*)pushSize(&memoryArena,
                                intersections.intersectionCount * sizeof(Intersection));
#if 0
    // translation only
    insertIntersection(&intersections, 0, 0, 1, 3.0f, 3.0f, 0.0f);
    insertIntersection(&intersections, 1, 2, 3, 5.0f, 7.0f, 5.0f);
    insertIntersection(&intersections, 2, 4, 5, 6.0f, 5.0f, 5.0f);
    insertIntersection(&intersections, 3, 6, 7, 5.0f, 5.0f, 5.0f);
    insertIntersection(&intersections, 4, 8, 9, 5.0f, 10.0f, 0.0f);
#elif 0
    // rotation only
    insertIntersection(&intersections, 0, 0, 1, 3.0f, 3.0f, 0.0f);
    insertIntersection(&intersections, 1, 2, 3, 2.0f, 0.0f, 0.0f);
    insertIntersection(&intersections, 2, 4, 5, 0.0f, -1.0f, 0.0f);
    insertIntersection(&intersections, 3, 6, 7, 0.0f, 0.0f, 0.0f);
    insertIntersection(&intersections, 4, 8, 9, 5.0f, 10.0f, 0.0f);
#else 
    // translation + rotation
    insertIntersection(&intersections, 0, 0, 1, 3.0f, 3.0f, 0.0f);
    insertIntersection(&intersections, 1, 2, 3, 7.0f, 5.0f, 5.0f);
    insertIntersection(&intersections, 2, 4, 5, 5.0f, 5.0f, 6.0f);
    insertIntersection(&intersections, 3, 6, 7, 5.0f, 5.0f, 5.0f);
    insertIntersection(&intersections, 4, 8, 9, 5.0f, 10.0f, 0.0f);
#endif
    
    V3* payload = 0;
    i32 isectCount = 0;
    Intersection* isect = intersections.firstIntersection;
    for (u32 i = 0;
         i < intersections.intersectionCount;
         i++)
    {
        if (!isect->deleted)
        {
            V3* pos = pushStruct(&memoryArena, V3);
            *pos = isect->position;
            isectCount++;
            
            if (!payload)
            {
                payload = pos;
            }
        }
        
        isect++;
    }
    
    if (payload)
    {
        sendMessage(&memoryArena,
                    &sendTransmissionState,
                    MessageType_DebugIntersections,
                    payload,
                    sizeof(V3) * isectCount,
                    0);
    }
    
    Rig* triforceRig = pushStruct(&memoryArena, Rig);
    triforceRig->pointCount = 3;
    triforceRig->points = (V3*)pushSize(&memoryArena,
                                        triforceRig->pointCount * sizeof(V3));
    triforceRig->points[0] = v3(0.0f, 0.0f, 0.0f);
    triforceRig->points[1] = v3(1.0f, 0.0f, 0.0f);
    triforceRig->points[2] = v3(0.0f, 2.0f, 0.0f);
    
    Rig* triforceRigCand = pushStruct(&memoryArena, Rig);
    triforceRigCand->pointCount = 3;
    triforceRigCand->points = (V3*)pushSize(&memoryArena,
                                            triforceRig->pointCount * sizeof(V3));
    
    bool32 modelMatched = matchIntersectionsToRig(triforceRig,
                                                  &intersections,
                                                  triforceRigCand);
    if (modelMatched)
    {
        printf("model matched!\n");
        
        triforceRig->points[0] = triforceRigCand->points[0];
        triforceRig->points[1] = triforceRigCand->points[1];
        triforceRig->points[2] = triforceRigCand->points[2];
        
        sendMessage(&memoryArena,
                    &sendTransmissionState,
                    MessageType_Payload,
                    triforceRig->points,
                    triforceRig->pointCount * sizeof(V3),
                    0);
    }
    
    usleep(1000 * 1000);
    
    return 0;
}