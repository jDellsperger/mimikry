#include "beholder.h"

static void initApplication(MemoryArena* arena,
                            ApplicationState* state,
                            i32 maxRigRestrictionCount)
{
    *state = {};
    state->status = ApplicationStatus_None;
    state->rig = initializeHumanoidRig(arena, maxRigRestrictionCount);
}

static IntersectionVector mergeIntersections(MemoryArena* arena,
                                             IntersectionVector* v,
                                             r32 mergeDistThreshold)
{
    IntersectionVector result = initializeIntersectionVector(arena,
                                                             v->count);
    for (i32 i = 0;
         i < v->count;
         i++)
    {
        Intersection* i1 = &v->intersections[i];
        
        if (i1->deleted)
        {
            continue;
        }
        
        V3 p1 = i1->position;
        
        for (i32 j = i + 1;
             j < v->count;
             j++)
        {
            Intersection* i2 = &v->intersections[j];
            
            if (i2->deleted)
            {
                continue;
            }
            
            V3 p2 = i2->position;
            
            if (lengthV3(subV3(p1, p2)) < mergeDistThreshold)
            {
                V3 newPos = lerpV3(p1, p2, 0.5f);
                i1->position = newPos;
                i2->deleted = 1;
            }
        }
        
        pushIntersection(arena,
                         &result,
                         i1->position);
    }
    
    return result;
}

static IntersectionVector detectThreewayIntersections(MemoryArena* arena,
                                                      Bucket* buckets,
                                                      i32 cameraCount,
                                                      r32 rayIntersectionThreshold,
                                                      r32 mergeDistThreshold)
{
    IntersectionVector v = initializeIntersectionVector(arena, 20);
    
    for (i32 cameraIndex = 0;
         cameraIndex < cameraCount - 2;
         cameraIndex++)
    {
        Bucket* cameraRays = &buckets[cameraIndex];
        RayInfoIntersectionList* cameraIntersectionLists =
            (RayInfoIntersectionList*)pushSize(arena,
                                               cameraRays->used * 
                                               sizeof(RayInfoIntersectionList));
        
        for (i32 intersectionListIndex = 0;
             intersectionListIndex < cameraRays->used;
             intersectionListIndex++)
        {
            cameraIntersectionLists[intersectionListIndex] = {};
            cameraIntersectionLists[intersectionListIndex].first = 0;
        }
        
        for (i32 compareCameraIndex = cameraIndex + 1;
             compareCameraIndex < cameraCount;
             compareCameraIndex++)
        {
            Bucket* compareCameraRays = &buckets[compareCameraIndex];
            
            for (i32 rayIndex = 0;
                 rayIndex < cameraRays->used;
                 rayIndex++)
            {
                Ray* ray = &cameraRays->rays[rayIndex];
                RayInfoIntersectionList* rayIntersections =
                    &cameraIntersectionLists[rayIndex];
                
                for (i32 compareRayIndex = 0;
                     compareRayIndex < compareCameraRays->used;
                     compareRayIndex++)
                {
                    V3 intersection;
                    Ray* compareRay = &compareCameraRays->rays[compareRayIndex];
                    r32 t1, t2;
                    
                    if (intersectRayRay(ray, 
                                        compareRay, 
                                        &intersection,
                                        &t1, &t2,
                                        rayIntersectionThreshold))
                    {
                        insertRayInfoIntersectionSortedByT1(arena,
                                                            rayIntersections,
                                                            cameraIndex,
                                                            compareCameraIndex,
                                                            rayIndex, 
                                                            compareRayIndex,
                                                            t1, t2,
                                                            intersection);
                    }
                }
            }
        }
        
        for (i32 intersectionListIndex = 0;
             intersectionListIndex < cameraRays->used;
             intersectionListIndex++)
        {
            RayInfoIntersectionList* intersectionList =
                &cameraIntersectionLists[intersectionListIndex];
            RayInfoIntersectionListNode* iN1 = intersectionList->first;
            
            while (iN1)
            {
                RayInfoIntersection* i1 = &iN1->intersection;
                V3 p1 = i1->position;
                
                RayInfoIntersectionListNode* iN2 = iN1->next;
                
                while (iN2)
                {
                    RayInfoIntersection* i2 = &iN2->intersection;
                    V3 p2 = i2->position;
                    
                    r32 distP1P2 = lengthV3(subV3(p1, p2));
                    
                    if (distP1P2 <= rayIntersectionThreshold)
                    {
                        r32 t1 = 0.0f;
                        r32 t2 = 0.0f;
                        
                        V3 p3 = {};
                        Ray* r1 = &buckets[i1->cameraIndex2].rays[i1->rayIndex2];
                        Ray* r2 = &buckets[i2->cameraIndex2].rays[i2->rayIndex2];
                        intersectRayRay(r1, r2,
                                        &p3,
                                        &t1, &t2,
                                        rayIntersectionThreshold);
                        
                        r32 distP1P3 = lengthV3(subV3(p1, p3));
                        r32 distP2P3 = lengthV3(subV3(p2, p3));
                        
                        if (distP1P3 < rayIntersectionThreshold && 
                            distP2P3 < rayIntersectionThreshold)
                        {
                            V3 position = lerpV3(lerpV3(p1, p2, 0.5f), p3, 0.5f);
                            pushIntersection(arena,
                                             &v, 
                                             position);
#if 0
                            printf("Intersection found: r1->r2: %.2f, r1->r3: %.2f, r2->r3: %.2f\n",
                                   distP1P2, distP1P3, distP2P3);
                            printf("r1: %.2f, r2: %.2f, r3: %.2f\n", i1->t1, t1, t2);
#endif
                        }
                    }
                    
                    iN2 = iN2->next;
                }
                
                iN1 = iN1->next;
            }
        }
    }
    
    IntersectionVector result = mergeIntersections(arena,
                                                   &v,
                                                   mergeDistThreshold);
    
    return result;
}

static IntersectionVector detectIntersections(MemoryArena* arena,
                                              Bucket* buckets,
                                              i32 bucketCount,
                                              r32 maxDist,
                                              r32 mergeDistThreshold)
{
    IntersectionVector v = initializeIntersectionVector(arena, 20);
    i32 rayId = 0;
    
    for (i32 bucketIndex = 0;
         bucketIndex < bucketCount;
         bucketIndex++)
    {
        Bucket* bucket = &buckets[bucketIndex];
        
        for (i32 compareBucketIndex = bucketIndex + 1; 
             compareBucketIndex < bucketCount;
             compareBucketIndex++)
        {
            Bucket* compareBucket = &buckets[compareBucketIndex];
            
            for (i32 rayIndex = 0;
                 rayIndex < bucket->used;
                 rayIndex++)
            {
                Ray* ray = &bucket->rays[rayIndex];
                i32 compareRayId = 0;
                r32 minT1 = FLT_MAX;
                bool32 intersectionFound = 0;
                V3 firstIntersectionPosition = {};
                
                for (i32 compareRayIndex = 0;
                     compareRayIndex < compareBucket->used;
                     compareRayIndex++)
                {
                    V3 position;
                    Ray* compareRay = &compareBucket->rays[compareRayIndex];
                    
                    r32 t1, t2;
                    
                    if (intersectRayRay(ray, compareRay, 
                                        &position, 
                                        &t1, &t2, 
                                        maxDist))
                    {
#if 0
                        if (t1 < minT1)
                        {
                            intersectionFound = 1;
                            minT1 = t1;
                            firstIntersectionPosition = position;
                        }
#else
                        pushIntersection(arena,
                                         &v, 
                                         position);
#endif
                    }
                    
                    compareRayId++;
                }
                
#if 0
                if (intersectionFound)
                {
                    pushIntersection(arena,
                                     &v, 
                                     firstIntersectionPosition);
                }
#endif
                
                rayId += compareRayId;
            }
        }
    }
    
    IntersectionVector result = mergeIntersections(arena,
                                                   &v,
                                                   mergeDistThreshold);
    
    return result;
}

// NOTE(jan): quicksort algorithm stolen from wikipedia
static void sortIntersectionsByHeight(IntersectionVector* intersections,
                                      i32 low, i32 high)
{
    if (low < high)
    {
        Intersection* A = intersections->intersections;
        r32 pivot = A[high].position.z;
        
        i32 i = low - 1;
        for (i32 j = low;
             j < high;
             j++)
        {
            if (A[j].position.z < pivot)
            {
                if (i != j)
                {
                    i++;
                    Intersection tmp = {};
                    tmp = A[i];
                    A[i] = A[j];
                    A[j] = tmp;
                }
            }
        }
        
        i++;
        Intersection tmp = {};
        tmp = A[i];
        A[i] = A[high];
        A[high] = tmp;
        
        sortIntersectionsByHeight(intersections, low, i - 1);
        sortIntersectionsByHeight(intersections, i + 1, high);
    }
}

static bool32Vector markIntersectionsByDeleted(MemoryArena* arena,
                                               IntersectionVector* v)
{
    bool32Vector result =
        initializebool32Vector(arena, v->count);
    
    for (i32 intersectionIndex = 0;
         intersectionIndex < v->count;
         intersectionIndex++)
    {
        pushbool32(arena,
                   &result,
                   v->intersections[intersectionIndex].deleted);
    }
    
    return result;
}

static bool32Vector markIntersectionsWithinHeight(MemoryArena* arena,
                                                  IntersectionVector* v,
                                                  bool32Vector* deletedFlags,
                                                  r32 minZ, r32 maxZ)
{
    assert(v->count == deletedFlags->count);
    
    bool32Vector result =
        initializebool32Vector(arena, v->count);
    
    for (i32 intersectionIndex = 0;
         intersectionIndex < v->count;
         intersectionIndex++)
    {
        bool32 flaggedAsDeleted = deletedFlags->values[intersectionIndex];
        
        if (!flaggedAsDeleted)
        {
            Intersection intersection = 
                v->intersections[intersectionIndex];
            
            if (!intersection.deleted)
            {
                r32 z = -1.0f * intersection.position.z;
                
                if (z < minZ || z > maxZ)
                {
                    flaggedAsDeleted = 1;
                }
            }
            else
            {
                flaggedAsDeleted = 1;
            }
        }
        
        pushbool32(arena,
                   &result,
                   flaggedAsDeleted);
    }
    
    return result;
}

static bool32Vector markIntersectionsWithinHeight(MemoryArena* arena,
                                                  IntersectionVector* v,
                                                  bool32Vector* deletedFlags,
                                                  V3 referencePoint,
                                                  r32 maxHeightDist)
{
    assert(v->count == deletedFlags->count);
    
    r32 referenceZ = -1.0f * referencePoint.z;
    r32 minZ = referenceZ - maxHeightDist;
    r32 maxZ = referenceZ + maxHeightDist;
    
    bool32Vector result =
        markIntersectionsWithinHeight(arena,
                                      v, deletedFlags,
                                      minZ, maxZ);;
    
    return result;
}

static bool32Vector markIntersectionsWithinDistance(MemoryArena* arena,
                                                    IntersectionVector* v,
                                                    bool32Vector* deletedFlags,
                                                    V3 referencePoint,
                                                    r32 minDist,
                                                    r32 maxDist)
{
    assert(v->count == deletedFlags->count);
    
    bool32Vector result = 
        initializebool32Vector(arena, v->count);
    
    for (i32 intersectionIndex = 0;
         intersectionIndex < v->count;
         intersectionIndex++)
    {
        bool32 flaggedAsDeleted = deletedFlags->values[intersectionIndex];
        
        if (!flaggedAsDeleted)
        {
            Intersection intersection = 
                v->intersections[intersectionIndex];
            
            if (!intersection.deleted)
            {
                V3 intersectionPosition = intersection.position;
                r32 dist = lengthV3(subV3(referencePoint, intersectionPosition));
                
                if (dist < minDist || dist > maxDist)
                {
                    flaggedAsDeleted = 1;
                }
            }
            else
            {
                flaggedAsDeleted = 1;
            }
        }
        
        pushbool32(arena,
                   &result,
                   flaggedAsDeleted);
    }
    
    return result;
}

static
bool32Vector markIntersectionsWithinVerticalCylinder(MemoryArena* arena,
                                                     IntersectionVector* v,
                                                     bool32Vector* deletedFlags,
                                                     V3 referencePoint,
                                                     r32 maxDist)
{
    assert(v->count == deletedFlags->count);
    
    bool32Vector result = 
        initializebool32Vector(arena, v->count);
    
    V2 referenceXY = v2(referencePoint.x, referencePoint.y);
    
    for (i32 intersectionIndex = 0;
         intersectionIndex < v->count;
         intersectionIndex++)
    {
        bool32 flaggedAsDeleted = deletedFlags->values[intersectionIndex];
        
        if (!flaggedAsDeleted)
        {
            Intersection intersection = 
                v->intersections[intersectionIndex];
            
            if (!intersection.deleted)
            {
                V3 p = intersection.position;
                V2 candXY = v2(p.x, p.y);
                
                r32 dist = lengthV2(subV2(referenceXY, candXY));
                
                if (dist > maxDist)
                {
                    flaggedAsDeleted = 1;
                }
            }
            else
            {
                flaggedAsDeleted = 1;
            }
        }
        
        pushbool32(arena,
                   &result,
                   flaggedAsDeleted);
    }
    
    return result;
}

static bool32Vector markIntersectionsOnAxis(MemoryArena* arena,
                                            IntersectionVector* v,
                                            bool32Vector* deletedFlags,
                                            V3 referencePoint,
                                            V2 referenceAxis,
                                            r32 maxAngleDeg)
{
    assert(v->count == deletedFlags->count);
    
    bool32Vector result =
        initializebool32Vector(arena, v->count);
    
    r32 maxAngle = 1.0f - cos(rad(maxAngleDeg));
    V2 referenceXY = v2(referencePoint.x, referencePoint.y);
    
    for (i32 intersectionIndex = 0;
         intersectionIndex < v->count;
         intersectionIndex++)
    {
        bool32 flaggedAsDeleted = deletedFlags->values[intersectionIndex];
        
        if (!flaggedAsDeleted)
        {
            Intersection intersection = 
                v->intersections[intersectionIndex];
            
            if (!intersection.deleted)
            {
                V3 p = intersection.position;
                V2 candXY = v2(p.x, p.y);
                
                V2 candAxis = normalizeV2(subV2(referenceXY, candXY));
                if ((1.0f - abs(dotV2(candAxis, referenceAxis))) > maxAngle)
                {
                    flaggedAsDeleted = 1;
                }
            }
            else
            {
                flaggedAsDeleted = 1;
            }
        }
        
        pushbool32(arena,
                   &result,
                   flaggedAsDeleted);
    }
    
    return result;
}

static i32 findNearestIntersection(IntersectionVector* v,
                                   bool32Vector* deletedFlags,
                                   V3 rigPoint,
                                   r32 maxDist = FLT_MAX)
{
    i32 result = -1;
    
    r32 minDist = maxDist;
    
    for (i32 intersectionIndex = 0;
         intersectionIndex< v->count;
         intersectionIndex++)
    {
        Intersection* intersection = &v->intersections[intersectionIndex];
        
        if (!intersection->deleted && !deletedFlags->values[intersectionIndex])
        {
            V3 intersectionPosition = intersection->position;
            r32 dist = lengthV3(subV3(rigPoint, intersectionPosition));
            
            if (dist < minDist)
            {
                result = intersectionIndex;
                minDist = dist;
            }
        }
    }
    
    return result;
}

static bool32 matchIntersectionToRigPoint(IntersectionVector* intersectionsHC,
                                          IntersectionVector* intersectionsLC,
                                          bool32Vector* deletedFlagsHC,
                                          bool32Vector* deletedFlagsLC,
                                          V3* rigPoint,
                                          r32 maxDist = FLT_MAX)
{
    bool32 result = false;
    
    i32 closestIntersectionIndex  = findNearestIntersection(intersectionsHC,
                                                            deletedFlagsHC,
                                                            *rigPoint,
                                                            maxDist);
    
    if (closestIntersectionIndex != -1)
    {
        Intersection* i = &intersectionsHC->intersections[closestIntersectionIndex];
        *rigPoint = i->position;
        i->deleted = 1;
        
        intersectionsHC->intersections[closestIntersectionIndex].deleted = true;
        
        result = true;
    }
    else
    {
        closestIntersectionIndex = findNearestIntersection(intersectionsLC,
                                                           deletedFlagsLC,
                                                           *rigPoint,
                                                           maxDist);
        
        if (closestIntersectionIndex != -1)
        {
            Intersection* i =
                &intersectionsLC->intersections[closestIntersectionIndex];
            *rigPoint = i->position;
            i->deleted = 1;
            
            intersectionsLC->intersections[closestIntersectionIndex].deleted = true;
            
            result = true;
        }
    }
    
    return result;
}

static bool32 matchIntersectionsToRig(MemoryArena* arena,
                                      HumanoidRig* rig,
                                      IntersectionVector* intersections)
{
    bool32 result = 0;
    
    // Conditions
    // TODO(jan): completely random numbers...
    // TODO(jan): scale max height by modelheight
    r32 maxFootHeightDist = 5.0f;
    r32 minShoulderSpan = 25.0f;
    r32 maxShoulderSpan = 50.0f;
    r32 maxKneeHeightDist = 5.0f;
    r32 maxKneeHeight = 60.0f;
    r32 maxKneeFootAngle = 10.0f;
    r32 minKneeFootDist = 10.0f;
    r32 minHipKneeDist = 10.0f;
    r32 maxHipChestRadius = 10.0f;
    r32 maxChestRadius = 15.0f;
    r32 maxHipHeight = 100.0f;
    r32 maxShoulderFootAngle = 30.0f;
    r32 maxShoulderHeightDist = 5.0f;
    r32 minShoulderChestDist = 5.0f;
    r32 maxElbowShoulderHeightDist = 10.0f;
    r32 maxHandElbowHeightDist = 10.0f;
    r32 maxShoulderRadius = 30.0f;
    r32 maxElbowFootAngle = 30.0f;
    r32 maxHandFootAngle = 30.0f;
    
    // 1. sort intersections by height
    sortIntersectionsByHeight(intersections, 
                              0,
                              intersections->count - 1);
    bool32Vector deletedFlags = markIntersectionsByDeleted(arena,
                                                           intersections);
    
    // 2. find feet intersections -> two lowest at same height, ~ 20-50cm apart
    bool32 feetFound = 0;
    i32 foot1Index = -1;
    i32 foot2Index = -1;
    V2 xyFoot1 = {};
    V2 xyFoot2 = {};
    V2 footAxis = {};
    V3 foot1 = {};
    V3 foot2 = {};
    
    for (i32 i = intersections->count - 1;
         i > 0;
         i--)
    {
        V3 footCand1 = intersections->intersections[i].position;
        
        bool32Vector footCandidateFlags =
            markIntersectionsWithinHeight(arena,
                                          intersections,
                                          &deletedFlags,
                                          footCand1,
                                          maxFootHeightDist);
        footCandidateFlags =
            markIntersectionsWithinDistance(arena,
                                            intersections,
                                            &footCandidateFlags,
                                            footCand1,
                                            minShoulderSpan,
                                            maxShoulderSpan);
        footCandidateFlags.values[i] = 1;
        
        i32 cand2Index = findNearestIntersection(intersections, 
                                                 &footCandidateFlags,
                                                 footCand1);
        
        if (cand2Index != -1)
        {
            foot1 = footCand1;
            foot2 = intersections->intersections[cand2Index].position;
            
            foot1Index = i;
            foot2Index = cand2Index;
            xyFoot1 = v2(footCand1.x, footCand1.y);
            xyFoot2 = v2(intersections->intersections[cand2Index].position.x, 
                         intersections->intersections[cand2Index].position.y);
            footAxis = normalizeV2(subV2(xyFoot1, xyFoot2));
            feetFound = 1;
            
            break;
        }
    }
    
    if (!feetFound)
    {
        printf("No feet found\n");
        return result;
    }
    
    intersections->intersections[foot1Index].deleted = 1;
    intersections->intersections[foot2Index].deleted = 1;
    
    // 3. Knees, Hip, chest, shoulders and head must all be whithin cylinder defined by 
    // center between feet and max shoulder span
    V3 center = lerpV3(foot1, foot2, 0.5f);
    center.z = 0.0f;
    bool32Vector bodyCandidateFlags =
        markIntersectionsWithinVerticalCylinder(arena,
                                                intersections,
                                                &deletedFlags,
                                                center,
                                                maxShoulderSpan / 2.0f);
    
    // 4. find head intersection -> single highest, ~ between feet intersections
    bool32 headFound = 0;
    i32 headIndex = -1;
    
    for (i32 headCandidateIndex = 0;
         headCandidateIndex < intersections->count;
         headCandidateIndex++)
    {
        if (!bodyCandidateFlags.values[headCandidateIndex])
        {
            headIndex = headCandidateIndex;
            headFound = 1;
            
            break;
        }
    }
    
    if (!headFound)
    {
        printf("No head found\n");
        return result;
    }
    
    intersections->intersections[headIndex].deleted = 1;
    
    // 5. find knees
    
    bool32 kneesFound = 0;
    i32 knee1Index, knee2Index;
    r32 knee1Z = 0.0f;
    r32 knee2Z = 0.0f;
    V2 xyKnee1, xyKnee2;
    
    bool32Vector kneeCandidateFlags =
        markIntersectionsWithinHeight(arena,
                                      intersections,
                                      &bodyCandidateFlags,
                                      0.0f, maxKneeHeight);
    
    for (i32 kneeCand1Index = min(foot1Index, foot2Index) - 1;
         kneeCand1Index > 0;
         kneeCand1Index--)
    {
        if (kneeCandidateFlags.values[kneeCand1Index]) continue;
        
        V3 kneeCand1 = intersections->intersections[kneeCand1Index].position;
        bool32Vector kneeCandidate2Flags = 
            markIntersectionsWithinHeight(arena,
                                          intersections,
                                          &kneeCandidateFlags,
                                          kneeCand1,
                                          maxKneeHeightDist);
        kneeCandidate2Flags =
            markIntersectionsOnAxis(arena,
                                    intersections,
                                    &kneeCandidate2Flags,
                                    kneeCand1,
                                    footAxis,
                                    maxKneeFootAngle);
        kneeCandidate2Flags.values[kneeCand1Index] = 1;
        
        i32 kneeCand2Index = findNearestIntersection(intersections,
                                                     &kneeCandidate2Flags,
                                                     kneeCand1);
        if (kneeCand2Index != -1)
        {
            V3 kneeCand2 =  intersections->intersections[kneeCand2Index].position;
            
            knee1Index = kneeCand1Index;
            knee2Index = kneeCand2Index;
            kneesFound = 1;
            xyKnee1 = v2(kneeCand1.x, kneeCand1.y);
            xyKnee2 = v2(kneeCand2.x, kneeCand2.y);
            knee1Z = kneeCand1.z;
            knee2Z = kneeCand2.z;
            break;
        }
    }
    
    if (!kneesFound)
    {
        printf("No knees found\n");
        return result;
    }
    
    intersections->intersections[knee1Index].deleted = 1;
    intersections->intersections[knee2Index].deleted = 1;
    
    // 6. shoulder, elbows and hands
    
    bool32 armsFound = 0;
    i32 shoulder1Index, shoulder2Index;
    i32 elbow1Index, elbow2Index;
    i32 hand1Index, hand2Index;
    
    V2 xyShoulder1, xyShoulder2;
    r32 shoulder1Z, shoulder2Z;
    V3 shoulderCenter;
    
    for (i32 shoulderCand1Index = knee2Index - 1;
         shoulderCand1Index > 0;
         shoulderCand1Index--)
    {
        if (bodyCandidateFlags.values[shoulderCand1Index]) continue;
        
        V3 shoulderCand1 =
            intersections->intersections[shoulderCand1Index].position;
        shoulder1Z = -1.0f * shoulderCand1.z;
        xyShoulder1 = v2(shoulderCand1.x, shoulderCand1.y);
        
        bool32Vector shoulder2CandidateFlags =
            markIntersectionsWithinHeight(arena,
                                          intersections,
                                          &bodyCandidateFlags,
                                          shoulderCand1,
                                          maxShoulderHeightDist);
        shoulder2CandidateFlags =
            markIntersectionsOnAxis(arena,
                                    intersections,
                                    &shoulder2CandidateFlags,
                                    shoulderCand1,
                                    footAxis,
                                    maxShoulderFootAngle);
        shoulder2CandidateFlags.values[shoulderCand1Index] = 1;
        
        for (i32 shoulderCand2Index = shoulderCand1Index - 1;
             shoulderCand2Index> 0;
             shoulderCand2Index--)
        {
            if (shoulder2CandidateFlags.values[shoulderCand2Index]) continue;
            
            V3 shoulderCand2 =
                intersections->intersections[shoulderCand2Index].position;
            shoulder2Z = -1.0f * shoulderCand2.z;
            xyShoulder2 = v2(shoulderCand2.x, shoulderCand2.y);
            
            shoulderCenter = lerpV3(shoulderCand1, shoulderCand2, 0.5f);
            
            // NOTE(jan): Find elbow candidates
            r32 minDistShoulderElbow1 = FLT_MAX;
            r32 minDistShoulderElbow2 = FLT_MAX;
            
            bool32Vector elbowCandidateFlags =
                markIntersectionsWithinHeight(arena,
                                              intersections,
                                              &deletedFlags,
                                              shoulderCenter,
                                              maxElbowShoulderHeightDist);
            elbowCandidateFlags =
                markIntersectionsOnAxis(arena,
                                        intersections,
                                        &elbowCandidateFlags,
                                        shoulderCenter,
                                        footAxis,
                                        maxElbowFootAngle);
            elbowCandidateFlags.values[shoulderCand1Index] = 1;
            elbowCandidateFlags.values[shoulderCand2Index] = 1;
            
            i32 elbowCand1Index = findNearestIntersection(intersections,
                                                          &elbowCandidateFlags,
                                                          shoulderCand1);
            
            if (elbowCand1Index == -1)
            {
                continue;
            }
            
            i32 elbowCand2Index = findNearestIntersection(intersections,
                                                          &elbowCandidateFlags,
                                                          shoulderCand2);
            
            if (elbowCand2Index == -1)
            {
                continue;
            }
            
            V3 elbowCand1 = intersections->intersections[elbowCand1Index].position;
            V3 elbowCand2 = intersections->intersections[elbowCand2Index].position;
            V3 elbowCenter = lerpV3(elbowCand1, elbowCand2, 0.5f);
            
            // NOTE(jan): Find hand candidates
            
            bool32Vector handCandidate1Flags =
                markIntersectionsWithinHeight(arena,
                                              intersections,
                                              &deletedFlags,
                                              elbowCand1,
                                              maxHandElbowHeightDist);
            handCandidate1Flags =
                markIntersectionsOnAxis(arena,
                                        intersections,
                                        &handCandidate1Flags,
                                        shoulderCenter,
                                        footAxis,
                                        maxHandFootAngle);
            handCandidate1Flags.values[shoulderCand1Index] = 1;
            handCandidate1Flags.values[shoulderCand2Index] = 1;
            handCandidate1Flags.values[elbowCand1Index] = 1;
            handCandidate1Flags.values[elbowCand2Index] = 1;
            
            i32 handCand1Index = findNearestIntersection(intersections,
                                                         &handCandidate1Flags,
                                                         elbowCand1);
            
            if (handCand1Index == -1)
            {
                continue;
            }
            
            bool32Vector handCandidate2Flags =
                markIntersectionsWithinHeight(arena,
                                              intersections,
                                              &deletedFlags,
                                              elbowCand1,
                                              maxHandElbowHeightDist);
            handCandidate2Flags =
                markIntersectionsOnAxis(arena,
                                        intersections,
                                        &handCandidate2Flags,
                                        shoulderCenter,
                                        footAxis,
                                        maxHandFootAngle);
            handCandidate2Flags.values[shoulderCand1Index] = 1;
            handCandidate2Flags.values[shoulderCand2Index] = 1;
            handCandidate2Flags.values[elbowCand1Index] = 1;
            handCandidate2Flags.values[elbowCand2Index] = 1;
            handCandidate2Flags.values[handCand1Index] = 1;
            
            i32 handCand2Index = findNearestIntersection(intersections,
                                                         &handCandidate2Flags,
                                                         elbowCand2);
            
            if (handCand2Index == -1)
            {
                continue;
            }
            
            armsFound = 1;
            shoulder1Index = shoulderCand1Index;
            shoulder2Index = shoulderCand2Index;
            elbow1Index = elbowCand1Index;
            elbow2Index = elbowCand2Index;
            hand1Index = handCand1Index;
            hand2Index = handCand2Index;
            
            break;
        }
        
        if (armsFound)
        {
            break;
        }
    }
    
    if (!armsFound)
    {
        printf("No arms found\n");
        return result;
    }
    
    intersections->intersections[shoulder1Index].deleted = 1;
    intersections->intersections[shoulder2Index].deleted = 1;
    intersections->intersections[elbow1Index].deleted = 1;
    intersections->intersections[elbow2Index].deleted = 1;
    intersections->intersections[hand1Index].deleted = 1;
    intersections->intersections[hand2Index].deleted = 1;
    
    // 7. find chest
    bool32Vector chestCandidateFlags =
        markIntersectionsWithinHeight(arena,
                                      intersections,
                                      &bodyCandidateFlags,
                                      max(knee1Z, knee2Z) + minHipKneeDist,
                                      min(shoulder1Z, shoulder2Z));
    i32 chestIndex = findNearestIntersection(intersections,
                                             &chestCandidateFlags,
                                             shoulderCenter);
    
    if (chestIndex == -1)
    {
        printf("No chest found\n");
        return result;
    }
    
    V3 chest = intersections->intersections[chestIndex].position;
    V2 xyChest = v2(chest.x, chest.y);
    r32 chestZ = -1.0f * chest.z;
    
    intersections->intersections[chestIndex].deleted = 1;
    
    // 8. find hip
    bool32Vector hipCandidateFlags =
        markIntersectionsWithinHeight(arena,
                                      intersections,
                                      &bodyCandidateFlags,
                                      max(knee1Z, knee2Z) + minHipKneeDist,
                                      chestZ);
    i32 hipIndex = 
        findNearestIntersection(intersections,
                                &hipCandidateFlags,
                                intersections->intersections[chestIndex].position);
    
    if (hipIndex == -1)
    {
        printf("No hip found\n");
        return result;
    }
    
    // 9. find front, sort left and right
    V2 shoulder1ToChest = subV2(xyChest, xyShoulder1);
    V2 shoulder2ToChest = subV2(xyChest, xyShoulder2);
    V2 front = normalizeV2(addV2(shoulder1ToChest, shoulder2ToChest));
    V2 pFront = addV2(xyChest, front);
    
    r32 saKnee1 = signedAreaV2(xyChest, pFront, xyKnee1); // positive if left knee
    if (saKnee1 > 0.0f)
    {
        rig->knee_r = intersections->intersections[knee1Index].position;
        rig->knee_l = intersections->intersections[knee2Index].position;
    }
    else
    {
        rig->knee_r = intersections->intersections[knee2Index].position;
        rig->knee_l = intersections->intersections[knee1Index].position;
    }
    
    r32 saFoot1 = signedAreaV2(xyChest, pFront, xyFoot1); // positive if left foot
    if (saFoot1 > 0.0f)
    {
        rig->foot_r = intersections->intersections[foot1Index].position;
        rig->foot_l = intersections->intersections[foot2Index].position;
    }
    else
    {
        rig->foot_r = intersections->intersections[foot2Index].position;
        rig->foot_l = intersections->intersections[foot1Index].position;
    }
    
    r32 saShoulder1 = signedAreaV2(xyChest, pFront, xyShoulder1); // positive if left shoulder
    if (saShoulder1 > 0.0f)
    {
        rig->shoulder_r = intersections->intersections[shoulder1Index].position;
        rig->shoulder_l = intersections->intersections[shoulder2Index].position;
        rig->elbow_r = intersections->intersections[elbow1Index].position;
        rig->elbow_l = intersections->intersections[elbow2Index].position;
        rig->hand_r = intersections->intersections[hand1Index].position;
        rig->hand_l = intersections->intersections[hand2Index].position;
    }
    else
    {
        rig->shoulder_r = intersections->intersections[shoulder2Index].position;
        rig->shoulder_l = intersections->intersections[shoulder1Index].position;
        rig->elbow_r = intersections->intersections[elbow2Index].position;
        rig->elbow_l = intersections->intersections[elbow1Index].position;
        rig->hand_r = intersections->intersections[hand2Index].position;
        rig->hand_l = intersections->intersections[hand1Index].position;
    }
    
    rig->hip =
        intersections->intersections[hipIndex].position;
    rig->chest =
        intersections->intersections[chestIndex].position;
    rig->head = 
        intersections->intersections[headIndex].position;
    
    r32 footKneeDistL = lengthV3(subV3(rig->foot_l, rig->knee_l));
    r32 footKneeDistR = lengthV3(subV3(rig->foot_r, rig->knee_r));
    r32 elbowShoulderDistL = lengthV3(subV3(rig->elbow_l, rig->shoulder_l));
    r32 elbowShoulderDistR = lengthV3(subV3(rig->elbow_r, rig->shoulder_r));
    r32 handElbowDistL = lengthV3(subV3(rig->hand_l, rig->elbow_l));
    r32 handElbowDistR = lengthV3(subV3(rig->hand_r, rig->elbow_r));
    
    r32 maxDistOffset = 5.0f;
    addRestrictionToRig(rig,
                        RigRestrictionType_Distance,
                        HUMANOID_RIG_INDEX_FOOT_L,
                        HUMANOID_RIG_INDEX_KNEE_L,
                        footKneeDistL - maxDistOffset,
                        footKneeDistL + maxDistOffset);
    addRestrictionToRig(rig,
                        RigRestrictionType_Distance,
                        HUMANOID_RIG_INDEX_FOOT_R,
                        HUMANOID_RIG_INDEX_KNEE_R,
                        footKneeDistR - maxDistOffset,
                        footKneeDistR + maxDistOffset);
    addRestrictionToRig(rig,
                        RigRestrictionType_Distance,
                        HUMANOID_RIG_INDEX_ELBOW_L,
                        HUMANOID_RIG_INDEX_SHOULDER_L,
                        elbowShoulderDistL - maxDistOffset,
                        elbowShoulderDistL + maxDistOffset);
    addRestrictionToRig(rig,
                        RigRestrictionType_Distance,
                        HUMANOID_RIG_INDEX_ELBOW_R,
                        HUMANOID_RIG_INDEX_SHOULDER_R,
                        elbowShoulderDistR - maxDistOffset,
                        elbowShoulderDistR + maxDistOffset);
    addRestrictionToRig(rig,
                        RigRestrictionType_Distance,
                        HUMANOID_RIG_INDEX_HAND_L,
                        HUMANOID_RIG_INDEX_ELBOW_L,
                        handElbowDistL - maxDistOffset,
                        handElbowDistL + maxDistOffset);
    
    result = 1;
    return result;
}

static void trackRig(MemoryArena* arena,
                     HumanoidRig* rig,
                     IntersectionVector* intersectionsHC, // high confidence
                     IntersectionVector* intersectionsLC  // low confidence
                     )
{
    // TODO(jan): max dist per point? hands move faster than hip e.g.
    r32 maxDistPerFrame = 10.0f;
    
    for (i32 pointIndex = 0; 
         pointIndex < arrayLength(rig->points); 
         pointIndex++)
    {
        RigRestrictionArray* restrictions = &rig->restrictions[pointIndex];
        
        bool32Vector deletedFlagsHC = markIntersectionsByDeleted(arena,
                                                                 intersectionsHC);
        bool32Vector deletedFlagsLC = markIntersectionsByDeleted(arena,
                                                                 intersectionsLC);
        
        for (i32 restrictionIndex = 0; 
             restrictionIndex < restrictions->count;
             restrictionIndex++)
        {
            RigRestriction* restriction = &restrictions->values[restrictionIndex];
            V3 referencePoint = rig->points[restriction->referencePointIndex];
            
            switch (restriction->type)
            {
                case RigRestrictionType_Distance: {
                    deletedFlagsHC =
                        markIntersectionsWithinDistance(arena,
                                                        intersectionsHC,
                                                        &deletedFlagsHC,
                                                        referencePoint,
                                                        restriction->minVal,
                                                        restriction->maxVal);
                    // TODO(jan): restrictions for LC intersections
                } break;
                
                case RigRestrictionType_None:
                default: {
                } break;
            }
        }
        
        matchIntersectionToRigPoint(intersectionsHC,
                                    intersectionsLC,
                                    &deletedFlagsHC,
                                    &deletedFlagsLC,
                                    &rig->points[pointIndex],
                                    maxDistPerFrame);
    }
}

static void handleAndSendDebugInfos(MemoryArena* flushArena,
                                    TransmissionState* transmissionState,
                                    DebugInfos* debugInfos,
                                    Bucket* buckets,
                                    i32 bucketCount,
                                    IntersectionVector* intersections)
{
    if (!debugInfos->updateFlags)
    {
        return;
    }
    
    // NOTE(jan): sending rays to mimic for debug visualization
    if (debugInfos->updateFlags & DebugUpdateFlags_Rays)
    {
        u8* rays = 0;
        i32 payloadSize = 0;
        for (i32 i = 0; i < bucketCount; i++)
        {
            i32 used = buckets[i].used;
            i32 size = sizeof(Ray) * used;
            payloadSize += size;
            u8* raysPtr = (u8*)pushSize(flushArena, size);
            
            if (!rays)
            {
                rays = raysPtr;
            }
            
            memcpy(raysPtr, buckets[i].rays, size);
        }
        
        if (payloadSize)
        {
            sendMessage(flushArena,
                        transmissionState,
                        MessageType_DebugRays,
                        rays,
                        payloadSize,
                        0);
        }
    }
    
    // NOTE(jan): sending camera positions to mimic for debug visualization
    if (debugInfos->updateFlags & DebugUpdateFlags_Cameras)
    {
        for (u32 i = 0; i < arrayLength(debugInfos->cameraLocations); i++)
        {
            if (debugInfos->cameraUpdated[i])
            {
                u8 spotterId = i + 1;
                i32 payloadSize = sizeof(M4x4);
                u8* payload = (u8*)pushSize(flushArena, payloadSize);
                M4x4 cameraPose = debugInfos->cameraLocations[i];
                *((M4x4*)payload) = cameraPose;
                printf("-------------------\n");
                printf("Camera pose received from spotter %i:\n", i);
                printM4x4(&cameraPose);
                printf("-------------------\n");
                printf("Sending camera pose to mimic\n");
                sendMessage(flushArena,
                            transmissionState,
                            MessageType_DebugCameraPose,
                            payload,
                            payloadSize,
                            spotterId);
            }
        }
    }
    
    // NOTE(jan): sending camera frames to mimic for debug visualization
    if (debugInfos->updateFlags & DebugUpdateFlags_Frames)
    {
        for (u32 i = 0; i < arrayLength(debugInfos->frames); i++)
        {
            if (debugInfos->frameUpdated[i])
            {
                u8 spotterId = i + 1;
                Frame* frame = &debugInfos->frames[i];
                i32 payloadSize = sizeof(DebugFrameInfo) + frame->size;
                u8* payload = (u8*)pushSize(flushArena, payloadSize);
                DebugFrameInfo* infos = (DebugFrameInfo*)payload;
                infos->width = frame->width;
                infos->height = frame->height;
                infos->bytesPerPixel = frame->bytesPerPixel;
                
                memcpy(payload + sizeof(DebugFrameInfo),
                       frame->memory,
                       frame->pitch * frame->height);
                
                sendMessage(flushArena,
                            transmissionState,
                            MessageType_DebugFrame,
                            payload,
                            payloadSize,
                            spotterId);
            }
        }
    }
    
    // NOTE(jan): sending intersections to mimic for debug visualization
    i32 isectCount = 0;
    V3* payload = 0;
    Intersection* isect = intersections->intersections;
    for (i32 i = 0;
         i < intersections->count;
         i++)
    {
        if (!isect->deleted)
        {
            V3* pos = pushStruct(flushArena, V3);
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
        sendMessage(flushArena,
                    transmissionState,
                    MessageType_DebugIntersections,
                    payload,
                    sizeof(V3) * isectCount,
                    0);
    }
}
