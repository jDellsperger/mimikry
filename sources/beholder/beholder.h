#ifndef BEHOLDER_H

#define RAY_BUCKET_COUNT 2
#define CAMERA_COUNT 6
#define TIMEWALK_COUNT 3

enum ApplicationStatus
{
    ApplicationStatus_None,
    ApplicationStatus_Exiting
};

enum DebugStatus
{
    DebugStatus_None = 0,
    DebugStatus_All = 1
};

enum DebugUpdateFlags
{
    DebugUpdateFlags_None = 0,
    DebugUpdateFlags_Rays = 1,
    DebugUpdateFlags_Cameras = 2,
    DebugUpdateFlags_Frames = 4
};

struct DebugInfos
{
    u8 updateFlags;
    bool32 cameraUpdated[CAMERA_COUNT];
    M4x4 cameraLocations[CAMERA_COUNT];
    
    Frame frames[CAMERA_COUNT];
    bool32 frameUpdated[CAMERA_COUNT];
};

struct Intersection
{
    V3 position;
    bool32 deleted;
};

struct i32Vector
{
    i32* values;
    i32 count;
    i32 maxCount;
};

static inline i32Vector initializei32Vector(MemoryArena* arena,
                                            i32 maxCount)
{
    i32Vector result;
    
    result.count = 0;
    result.maxCount = maxCount;
    result.values = 
        (i32*)pushSize(arena, maxCount * sizeof(i32));
    
    return result;
}

static inline void pushi32(MemoryArena* arena,
                           i32Vector* vector,
                           i32 value)
{
    if(vector->count >= vector->maxCount)
    {
        i32* oldValues = vector->values;
        i32 oldMaxCount = vector->maxCount;
        
        vector->maxCount *= 2;
        vector->values = 
            (i32*)pushSize(arena, vector->maxCount * sizeof(i32));
        memcpy(vector->values, 
               oldValues, 
               oldMaxCount * sizeof(i32));
    }
    
    vector->values[vector->count] = value;
    
    vector->count++;
}

static inline void cleari32Vector(i32Vector* v)
{
    v->count = 0;
}

struct IntersectionVector
{
    Intersection* intersections;
    i32 count;
    i32 maxCount;
};

static inline IntersectionVector initializeIntersectionVector(MemoryArena* arena,
                                                              i32 maxCount)
{
    IntersectionVector result;
    
    result.count = 0;
    result.maxCount = maxCount;
    result.intersections = 
        (Intersection*)pushSize(arena, maxCount * sizeof(Intersection));
    
    return result;
}

static inline void pushIntersection(MemoryArena* arena,
                                    IntersectionVector* vector,
                                    V3 position,
                                    bool32 deleted = 0)
{
    if(vector->count >= vector->maxCount)
    {
        Intersection* oldIntersections = vector->intersections;
        i32 oldMaxCount = vector->maxCount;
        
        vector->maxCount *= 2;
        vector->intersections = 
            (Intersection*)pushSize(arena, vector->maxCount * sizeof(Intersection));
        memcpy(vector->intersections, 
               oldIntersections, 
               oldMaxCount * sizeof(Intersection));
    }
    
    Intersection* intersection = &vector->intersections[vector->count];
    intersection->position = position;
    intersection->deleted = deleted;
    
    vector->count++;
}

struct RayInfoIntersection
{
    V3 position;
    bool32 deleted;
    
    i32 cameraIndex1, cameraIndex2;
    i32 rayIndex1, rayIndex2;
    r32 t1, t2;
};

struct RayInfoIntersectionListNode
{
    RayInfoIntersectionListNode* next = 0;
    RayInfoIntersection intersection;
};

struct RayInfoIntersectionList
{
    RayInfoIntersectionListNode* first;
};

static inline 
void insertRayInfoIntersectionSortedByT1(MemoryArena* arena,
                                         RayInfoIntersectionList* intersectionList,
                                         i32 cameraIndex1, i32 cameraIndex2,
                                         i32 rayIndex1, i32 rayIndex2,
                                         r32 t1, r32 t2,
                                         V3 position)
{
    RayInfoIntersectionListNode* previous = 0;
    RayInfoIntersectionListNode* next = intersectionList->first;
    
    while (next && next->intersection.t1 < t1)
    {
        previous = next;
        next = next->next;
    }
    
    RayInfoIntersectionListNode* current =
        pushStruct(arena, RayInfoIntersectionListNode);
    
    if (previous)
    {
        previous->next = current;
    }
    else
    {
        intersectionList->first = current;
    }
    
    current->next = next;
    
    current->intersection.cameraIndex1 = cameraIndex1;
    current->intersection.cameraIndex2 = cameraIndex2;
    current->intersection.rayIndex1 = rayIndex1;
    current->intersection.rayIndex2 = rayIndex2;
    current->intersection.t1 = t1;
    current->intersection.t2 = t2;
    current->intersection.deleted = 0;
    current->intersection.position = position;
}

enum RigRestrictionType
{
    RigRestrictionType_None,
    RigRestrictionType_Distance
};

struct RigRestriction
{
    RigRestrictionType type;
    i32 referencePointIndex;
    r32 minVal, maxVal;
};

struct RigRestrictionArray
{
    RigRestriction* values;
    i32 count;
    i32 maxCount;
};

static inline RigRestrictionArray initializeRigRestrictionArray(MemoryArena* arena,
                                                                i32 maxCount)
{
    RigRestrictionArray result;
    
    result.count = 0;
    result.maxCount = maxCount;
    result.values = 
        (RigRestriction*)pushSize(arena, maxCount * sizeof(RigRestriction));
    
    return result;
}

static inline void pushRigRestriction(RigRestrictionArray* array,
                                      RigRestriction value)
{
    assert(array->count < array->maxCount);
    
    array->values[array->count] = value;
    array->count++;
}

static inline void pushRigRestriction(RigRestrictionArray* array,
                                      RigRestrictionType type,
                                      i32 referencePointIndex,
                                      r32 minVal, r32 maxVal)
{
    RigRestriction restriction = {};
    
    restriction.type = type;
    restriction.referencePointIndex = referencePointIndex;
    restriction.minVal = minVal;
    restriction.maxVal = maxVal;
    
    pushRigRestriction(array, restriction);
}

#define HUMANOID_RIG_INDEX_HIP 0
#define HUMANOID_RIG_INDEX_CHEST 1
#define HUMANOID_RIG_INDEX_HEAD 2
#define HUMANOID_RIG_INDEX_SHOULDER_L 3
#define HUMANOID_RIG_INDEX_SHOULDER_R 4
#define HUMANOID_RIG_INDEX_ELBOW_L 5
#define HUMANOID_RIG_INDEX_ELBOW_R 6
#define HUMANOID_RIG_INDEX_HAND_L 7
#define HUMANOID_RIG_INDEX_HAND_R 8
#define HUMANOID_RIG_INDEX_KNEE_L 9
#define HUMANOID_RIG_INDEX_KNEE_R 10
#define HUMANOID_RIG_INDEX_FOOT_L 11
#define HUMANOID_RIG_INDEX_FOOT_R 12
#define HUMANOID_RIG_POINT_COUNT 13
struct HumanoidRig
{
    union
    {
        struct
        {
            V3 hip; // 0
            V3 chest; // 1
            V3 head; // 2
            V3 shoulder_l; // 3
            V3 shoulder_r; // 4
            V3 elbow_l; // 5
            V3 elbow_r; // 6
            V3 hand_l; // 7
            V3 hand_r; // 8
            V3 knee_l; // 9
            V3 knee_r; // 10
            V3 foot_l; // 11
            V3 foot_r; // 12
        };
        V3 points[HUMANOID_RIG_POINT_COUNT];
    };
    union
    {
        struct
        {
            RigRestrictionArray hipRestrictions; // 0
            RigRestrictionArray chestRestrictions; // 1
            RigRestrictionArray headRestrictions; // 2
            RigRestrictionArray shoulder_lRestrictions; // 3
            RigRestrictionArray shoulder_rRestrictions; // 4
            RigRestrictionArray elbow_lRestrictions; // 5
            RigRestrictionArray elbow_rRestrictions; // 6
            RigRestrictionArray hand_lRestrictions; // 7
            RigRestrictionArray hand_rRestrictions; // 8
            RigRestrictionArray knee_lRestrictions; // 9
            RigRestrictionArray knee_rRestrictions; // 10
            RigRestrictionArray foot_lRestrictions; // 11
            RigRestrictionArray foot_rRestrictions; // 12
        };
        RigRestrictionArray restrictions[HUMANOID_RIG_POINT_COUNT];
    };
};

static inline HumanoidRig initializeHumanoidRig(MemoryArena* arena,
                                                i32 maxRestrictionCount)
{
    HumanoidRig result = {};
    
    memset(result.points, 0, arrayLength(result.points) * sizeof(V3));
    
    for (i32 i = 0; i < arrayLength(result.restrictions); i++)
    {
        result.restrictions[i] = 
            initializeRigRestrictionArray(arena,
                                          maxRestrictionCount);
    }
    
    return result;
}

static inline void addRestrictionToRig(HumanoidRig* rig,
                                       RigRestrictionType type,
                                       i32 pointIndex,
                                       i32 referencePointIndex,
                                       r32 minVal, r32 maxVal)
{
    RigRestrictionArray* restrictions = &rig->restrictions[pointIndex];
    
    pushRigRestriction(restrictions,
                       type,
                       referencePointIndex,
                       minVal, maxVal);
}

struct Bucket
{
    Ray rays[100];
    i32 used;
    bool32 updatedSinceGet = 0;
};

struct RayBuckets
{
    // NOTE(jan): only the messaging thread is allowed to swap the indices,
    // the main thread can however read the front index
    i32 _frontIndex;
    i32 backIndex;
    
    // NOTE(jan): all threads are allowed to switch the buckets updated flag on/off
    bool32 _allBucketsUpdatedSinceGet = 0;
    
    Bucket buckets[RAY_BUCKET_COUNT][CAMERA_COUNT];
    i32 updatedClientsCount = 0;
    i32 clientCount = 0;
};

//NOTE(dave): For single thread use only
inline static void flushBucket(Bucket* bucket)
{
    bucket->used = 0;
}

inline static void initRayBuckets(RayBuckets* rayBuckets)
{
    *rayBuckets = {};
}

struct ApplicationState
{
    ApplicationStatus status;
    HumanoidRig rig;
};

#define BEHOLDER_H
#endif
