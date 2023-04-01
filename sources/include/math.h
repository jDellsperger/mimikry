#ifndef MATH_H

#include <math.h>

static inline r32 rad(r32 grad)
{
    r32 result = grad * (PI / 180.0f);
    
    return result;
}

static inline r32 grad(r32 rad)
{
    r32 result = rad / (PI / 180.0f);
    
    return result;
}

static inline i32 max(i32 a, i32 b)
{
    i32 result;
    
    if (a > b)
    {
        result = a;
    }
    else
    {
        result = b;
    }
    
    return result;
}

static inline i32 min(i32 a, i32 b)
{
    i32 result;
    
    if (a < b)
    {
        result = a;
    }
    else
    {
        result = b;
    }
    
    return result;
}

static inline r32 sq(r32 r)
{
    r32 result = r * r;
    
    return result;
}

static inline void swapR32(r32* a, r32* b)
{
    r32 tmp = *a;
    *a = *b;
    *b = tmp;
}

struct V2
{
    union
    {
        struct
        {
            r32 x, y;
        };
        struct
        {
            r32 u, v;
        };
        r32 e[2];
    };
};

static inline V2 v2(r32 x, r32 y)
{
    V2 result;
    
    result.x = x;
    result.y = y;
    
    return result;
}

static inline void printV2(V2* v)
{
    printf("%3.2f, %3.2f\n",
           v->e[0], v->e[1]);
}

static inline V2 multV2R(V2 v, r32 r)
{
    V2 result = {};
    
    result.x = v.x * r;
    result.y = v.y * r;
    
    return result;
}

static inline V2 subV2(V2 v1, V2 v2)
{
    V2 result;
    
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    
    return result;
}

static inline V2 addV2(V2 v1, V2 v2)
{
    V2 result;
    
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    
    return result;
}

static inline r32 dotV2(V2 v1, V2 v2)
{
    r32 result = v1.x * v2.x + v1.y * v2.y;
    
    return result;
}

static inline r32 lengthSqV2(V2 v)
{
    r32 result = dotV2(v, v);
    
    return result;
}

static inline r32 lengthV2(V2 v)
{
    r32 result = sqrt(lengthSqV2(v));
    
    return result;
}

static inline V2 normalizeV2(V2 v)
{
    V2 result;
    
    r32 length = lengthV2(v);
    r32 oneOverLength = 1.0f / length;
    
    result.x = v.x * oneOverLength;
    result.y = v.y * oneOverLength;
    
    return result;
}

static inline V2 lerpV2(V2 v1, V2 v2, r32 t)
{
    V2 result;
    
    V2 deltaV = subV2(v2, v1);
    deltaV = multV2R(deltaV, t);
    
    result = addV2(v1, deltaV);
    
    return result;
}

struct V3
{
    union
    {
        struct
        {
            r32 x, y, z;
        };
        struct
        {
            r32 r, g, b;
        };
        r32 e[3];
    };
};

static inline V3 v3(r32 x, r32 y, r32 z)
{
    V3 result;
    
    result.x = x;
    result.y = y;
    result.z = z;
    
    return result;
}

static inline V3 v3(V2 v, r32 z)
{
    V3 result;
    
    result.x = v.x;
    result.y = v.y;
    result.z = z;
    
    return result;
}

static inline void printV3(V3* v)
{
    printf("%3.2f, %3.2f, %3.2f\n",
           v->e[0], v->e[1], 
           v->e[2]);
}

static inline V3 multV3R(V3 v, r32 r)
{
    V3 result = {};
    
    result.x = v.x * r;
    result.y = v.y * r;
    result.z = v.z * r;
    
    return result;
}

static inline V3 addV3(V3 v1, V3 v2)
{
    V3 result;
    
    result.x = v1.x + v2.x;
    result.y = v1.y + v2.y;
    result.z = v1.z + v2.z;
    
    return result;
}

static inline V3 subV3(V3 v1, V3 v2)
{
    V3 result;
    
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    
    return result;
}

static inline r32 dotV3(V3 v1, V3 v2)
{
    r32 result = v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    
    return result;
}

static inline V3 lerpV3(V3 v1, V3 v2, r32 t)
{
    V3 result;
    
    V3 deltaV = subV3(v2, v1);
    deltaV = multV3R(deltaV, t);
    
    result = addV3(v1, deltaV);
    
    return result;
}

static inline r32 lengthSqV3(V3 v)
{
    r32 result = dotV3(v, v);
    
    return result;
}

static inline r32 lengthV3(V3 v)
{
    r32 result = sqrt(lengthSqV3(v));
    
    return result;
}

static inline V3 normalizeV3(V3 v)
{
    V3 result;
    
    r32 length = lengthV3(v);
    r32 oneOverLength = 1.0f / length;
    
    result.x = v.x * oneOverLength;
    result.y = v.y * oneOverLength;
    result.z = v.z * oneOverLength;
    
    return result;
}

struct V4
{
    union
    {
        struct
        {
            r32 x, y, z, w;
        };
        struct
        {
            r32 r, g, b, a;
        };
        r32 e[4];
    };
};

static inline V4 v4(r32 x, r32 y, r32 z, r32 w)
{
    V4 result;
    
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    
    return result;
}

static inline V4 v4(V3 v, r32 w)
{
    V4 result;
    
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    result.w = w;
    
    return result;
}

static inline void printV4(V4* v)
{
    printf("%3.2f, %3.2f, %3.2f, %3.2f\n",
           v->e[0], v->e[1], 
           v->e[2], v->e[3]);
}

static inline V4 subV4(V4 v1, V4 v2)
{
    V4 result;
    
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    result.w = v1.w - v2.w;
    
    return result;
}

inline r32 dotV4(V4 v1, V4 v2)
{
    r32 result = 
        v1.x * v2.x + 
        v1.y * v2.y + 
        v1.z * v2.z +
        v1.w * v2.w;
    
    return result;
}

static inline r32 lengthSqV4(V4 v)
{
    r32 result = dotV4(v, v);
    
    return result;
}

static inline r32 lengthV4(V4 v)
{
    r32 result = sqrt(lengthSqV4(v));
    
    return result;
}

// NOTE(jan): Matrix is in column-major form
struct M3x3
{
    r32 e[3][3];
};

static inline void printM3x3(M3x3* m)
{
    for (i32 row = 0; row < 3; row++)
    {
        printf("%3.2f, %3.2f, %3.2f\n",
               m->e[0][row], m->e[1][row], m->e[2][row]);
    }
}

static inline V3 multM3x3V3(M3x3 m, V3 v)
{
    V3 result = {};
    
    for (i32 i = 0; i < 3; i++)
    {
        for (i32 j = 0; j < 3; j++)
        {
            result.e[i] += m.e[j][i] * v.e[j];
        }
    }
    
    return result;
}

static inline r32 detM3x3(M3x3 m)
{
    r32 a, b, c, d, e, f, g, h, i;
    a = m.e[0][0];
    b = m.e[1][0];
    c = m.e[2][0];
    d = m.e[0][1];
    e = m.e[1][1];
    f = m.e[2][1];
    g = m.e[0][2];
    h = m.e[1][2];
    i = m.e[2][2];
    
    r32 result = 
        a*e*i +
        b*f*g +
        c*d*h -
        c*e*g -
        b*d*i -
        a*f*h;
    
    return result;
}

static inline V3 crossV3(V3 v1, V3 v2)
{
    V3 result = {};
    
    result.x = v1.y*v2.z - v1.z*v2.y;
    result.y = v1.z*v2.x - v1.x*v2.z;
    result.z = v1.x*v2.y - v1.y*v2.x;
    
    return result;
}

struct M4x4 
{
    r32 e[4][4];
};

static inline void printM4x4(M4x4* m)
{
#if 0
    for (i32 row = 0; row < 4; row++)
    {
        printf("| ");
        for (i32 col = 0; col < 4; col++)
        {
            printf("% 05.2f | ",
                   m->e[col][row]);
        }
        printf("\n");
    }
#else
    for (i32 row = 0; row < 4; row++)
    {
        for (i32 col = 0; col < 4; col++)
        {
            printf("%.2f ",
                   m->e[col][row]);
        }
        printf("\n");
    }
#endif
}

static inline M4x4 identityM4x4()
{
    M4x4 result;
    
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (i == j) 
            {
                result.e[i][j] = 1.0f;
            }
            else 
            {
                result.e[i][j] = 0.0f;
            }
        }
    }
    
    return result;
}

static inline M4x4 multM4x4(M4x4 m1, M4x4 m2)
{
    M4x4 result = {};
    
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 4; k++)
            {
                r32 v1 = m1.e[k][j];
                r32 v2 = m2.e[i][k];
                result.e[i][j] += v1 * v2;
            }
        }
    }
    
    return result;
}

static inline V4 multM4x4V4(M4x4 m, V4 v)
{
    V4 result = {};
    
    for (i32 i = 0; i < 4; i++)
    {
        for (i32 j = 0; j < 4; j++)
        {
            result.e[i] += m.e[j][i] * v.e[j];
        }
    }
    
    return result;
}

static inline r32* elementPtr(M4x4* m)
{
    r32* result = &m->e[0][0];
    
    return result;
}

static inline M4x4 translateM4x4(V3 v)
{
    M4x4 result = identityM4x4();
    
    result.e[3][0] += v.x;
    result.e[3][1] += v.y;
    result.e[3][2] += v.z;
    
    return result;
}

static inline M4x4 scaleM4x4(V3 v)
{
    M4x4 result = identityM4x4();
    
    result.e[0][0] = v.x;
    result.e[1][1] = v.y;
    result.e[2][2] = v.z;
    
    return result;
}

inline M4x4 rotateXM4x4(r32 theta)
{
    M4x4 result = identityM4x4();
    result.e[1][1] = cos(theta);
    result.e[1][2] = sin(theta);
    result.e[2][1] = -1 * sin(theta);
    result.e[2][2] = cos(theta);
    
    return result;
}

inline M4x4 rotateYM4x4(r32 theta)
{
    M4x4 result = identityM4x4();
    
    result.e[0][0] = cos(theta);
    result.e[2][0] = sin(theta);
    result.e[0][2] = -1 * sin(theta);
    result.e[2][2] = cos(theta);
    
    return result;
}

inline M4x4 rotateZM4x4(r32 theta)
{
    M4x4 result = identityM4x4();
    
    result.e[0][0] = cos(theta);
    result.e[1][0] = -1 * sin(theta);
    result.e[0][1] = sin(theta);
    result.e[1][1] = cos(theta);
    
    return result;
}
struct M4x4inv
{
    M4x4 fwd;
    M4x4 inv;
};

// TODO(jan): M4x4 doesn't make any sense, use M4x3 and V3 as result
static V4 gaussianEliminationM4x4(M4x4 m)
{
    V4 result = {};
    
    // Bring system in row-echellon form
    for (i32 eliminatingRow = 0; 
         eliminatingRow < 3; 
         eliminatingRow++)
    {
        for (i32 rowBeingEliminated = eliminatingRow + 1;
             rowBeingEliminated < 3;
             rowBeingEliminated++)
        {
            i32 columnBeingEliminated = eliminatingRow;
            
            if (m.e[columnBeingEliminated][rowBeingEliminated] > 0.01f)
            {
                r32 coefficient = -1.0f *
                    m.e[eliminatingRow][rowBeingEliminated] /
                    m.e[columnBeingEliminated][eliminatingRow];
                
                for (i32 column = columnBeingEliminated;
                     column < 4;
                     column++)
                {
                    m.e[column][rowBeingEliminated] += coefficient *
                        m.e[column][eliminatingRow];
                }
            }
        }
    }
    
    // Solve system
    for (i32 rowBeingSolved = 2;
         rowBeingSolved >= 0;
         rowBeingSolved--)
    {
        i32 startingColumn = rowBeingSolved;
        r32 rowResult = m.e[3][rowBeingSolved];
        
        for (i32 column = startingColumn + 1;
             column < 3;
             column++)
        {
            rowResult -= m.e[column][rowBeingSolved] * result.e[column];
        }
        
        result.e[rowBeingSolved] = rowResult / m.e[startingColumn][rowBeingSolved];
    }
    
    printM4x4(&m);
    printf("\n");
    
    return result;
}

struct V9
{
    r32 e[9];
};

static inline void printV9(V9* v)
{
    for (i32 i = 0; i < 8; i++)
    {
        printf("%3.2f, ", v->e[i]);
    }
    printf("%3.2f\n", v->e[8]);
}

struct M10x9
{
    r32 e[10][9];
};

static inline void printM10x9(M10x9* m)
{
    for (i32 row = 0; row < 9; row++)
    {
        printf("| ");
        for (i32 col = 0; col < 10; col++)
        {
            printf("% 05.2f | ",
                   m->e[col][row]);
        }
        printf("\n");
    }
}

static V9 gaussianEliminationM10x9(M10x9* m)
{
    V9 result = {};
    
    //printM10x9(m);
    //printf("----------------------------\n");
    
    // Bring system in row-echellon form
    for (i32 eliminatingRow = 0; 
         eliminatingRow < 9; 
         eliminatingRow++)
    {
        i32 columnBeingEliminated = eliminatingRow;
        r32 denominator = m->e[columnBeingEliminated][eliminatingRow];
        
        if (denominator < 0.00001f && denominator > -0.00001f)
        {
            for (i32 swapCandidateRow = eliminatingRow + 1;
                 swapCandidateRow < 9;
                 swapCandidateRow++)
            {
                r32 swapDenominator = m->e[columnBeingEliminated][swapCandidateRow];
                if (swapDenominator > 0.00001f || swapDenominator < -0.00001f)
                {
                    for (i32 swapCol = columnBeingEliminated;
                         swapCol < 10;
                         swapCol++)
                    {
                        swapR32(&m->e[swapCol][eliminatingRow],
                                &m->e[swapCol][swapCandidateRow]);
                    }
                    
                    denominator = m->e[columnBeingEliminated][eliminatingRow];
                    break;
                }
            }
        }
        
        r32 oneOverDenominator = 1.0f / denominator;
        
        for (i32 rowBeingEliminated = eliminatingRow + 1;
             rowBeingEliminated < 9;
             rowBeingEliminated++)
        {
            if (m->e[columnBeingEliminated][rowBeingEliminated] > 0.01f)
            {
                r32 coefficient = -1.0f *
                    m->e[eliminatingRow][rowBeingEliminated] * oneOverDenominator;
                
                for (i32 column = columnBeingEliminated;
                     column < 10;
                     column++)
                {
                    m->e[column][rowBeingEliminated] += coefficient *
                        m->e[column][eliminatingRow];
                }
            }
        }
    }
    
    //printM10x9(m);
    //printf("----------------------------\n");
    
    // Solve system
    for (i32 rowBeingSolved = 8;
         rowBeingSolved >= 0;
         rowBeingSolved--)
    {
        i32 startingColumn = rowBeingSolved;
        r32 rowResult = m->e[9][rowBeingSolved];
        
        for (i32 column = startingColumn + 1;
             column < 9;
             column++)
        {
            rowResult -= m->e[column][rowBeingSolved] * result.e[column];
        }
        
        if (m->e[startingColumn][rowBeingSolved] >  0.00001f || 
            m->e[startingColumn][rowBeingSolved] < -0.00001f)
        {
            result.e[rowBeingSolved] =
                rowResult / m->e[startingColumn][rowBeingSolved];
        }
        else
        {
            result.e[rowBeingSolved] = 0.0f;
        }
    }
    
    //printM10x9(m);
    //printf("----------------------------\n");
    
    return result;
}

struct Transform
{
    V3 translation;
    V3 scale;
    r32 rotationThetaY = 0.0f;
    r32 rotationThetaX = 0.0f;
    r32 rotationThetaZ = 0.0f;
};

static inline M4x4 transformationMatrix(Transform* t)
{
    M4x4 result = identityM4x4();
    
    M4x4 mS = scaleM4x4(t->scale);
    
    M4x4 mRX = rotateXM4x4(t->rotationThetaX);
    M4x4 mRY = rotateYM4x4(t->rotationThetaY);
    M4x4 mRZ = rotateZM4x4(t->rotationThetaZ);
    M4x4 mR = multM4x4(mRX, mRY);
    mR = multM4x4(mRZ, mR);
    mR = multM4x4(mR, mS);
    M4x4 mT = translateM4x4(t->translation);
    
    result = multM4x4(mT, mR);
    return result;
}

struct CoordinateSystem
{
    V3 o, xAxis, yAxis, zAxis;
};

static CoordinateSystem coordinateSystem(V3 o, V3 x, V3 y, V3 z)
{
    CoordinateSystem result;
    
    result.o = o;
    result.xAxis = x;
    result.yAxis = y;
    result.zAxis = z;
    
    return result;
}

struct Ray
{
    V3 origin;
    V3 direction;
};

static inline void printRay(Ray* ray)
{
    printf("Ray - origin: %f / %f / %f - dir: %f / %f / %f\n",
           ray->origin.x, ray->origin.y, ray->origin.z,
           ray->direction.x, ray->direction.y, ray->direction.z);
}

static inline V3 pointOnRay(Ray* ray, r32 t)
{
    V3 result;
    
    result = addV3(ray->origin, multV3R(ray->direction, t));
    
    return result;
}

// NOTE(jan): http://www.realtimerendering.com/intersections.html
static inline bool32 intersectRayRay(Ray* r1, Ray* r2, 
                                     V3* pIntersect,
                                     r32* t1, r32* t2,
                                     r32 maxDist)
{
    bool32 result = 0;
    
    V3 cross = crossV3(r1->direction, r2->direction);
    r32 den = lengthSqV3(cross);
    
    // TODO(jan): parallel threshold is arbitrary
    if (den > 0.000001f)
    {
        V3 diffO = subV3(r2->origin, r1->origin);
        
        M3x3 m1 = {};
        m1.e[0][0] = diffO.x;
        m1.e[0][1] = diffO.y;
        m1.e[0][2] = diffO.z;
        m1.e[1][0] = r2->direction.x;
        m1.e[1][1] = r2->direction.y;
        m1.e[1][2] = r2->direction.z;
        m1.e[2][0] = cross.x;
        m1.e[2][1] = cross.y;
        m1.e[2][2] = cross.z;
        
        M3x3 m2 = {};
        m2.e[0][0] = diffO.x;
        m2.e[0][1] = diffO.y;
        m2.e[0][2] = diffO.z;
        m2.e[1][0] = r1->direction.x;
        m2.e[1][1] = r1->direction.y;
        m2.e[1][2] = r1->direction.z;
        m2.e[2][0] = cross.x;
        m2.e[2][1] = cross.y;
        m2.e[2][2] = cross.z;
        
        r32 det1 = detM3x3(m1);
        r32 det2 = detM3x3(m2);
        
        *t1 = det1/den;
        *t2 = det2/den;
        
        V3 p1 = pointOnRay(r1, *t1);
        V3 p2 = pointOnRay(r2, *t2);
        
        Ray rIntersect;
        rIntersect.origin = p2;
        
        // NOTE(jan): not normalized on purpose, 
        // so that we can easier find the mid-way point
        rIntersect.direction = subV3(p1, p2);
        r32 rayDist = lengthV3(rIntersect.direction);
        //printf("dist: %f\n", dist);
        
        if (*t1 >= 0.0f && *t2 >= 0.0f && rayDist <= maxDist)
        {
            V3 rayPoint = pointOnRay(&rIntersect, 0.5f);
            
            // Don't consider points under the floor
            // TODO(jan): arbitrary threshold
            if (rayPoint.z < 3.0f)
            {
                *pIntersect = rayPoint;
                result = 1;
            }
        }
    }
    
    return result;
}

// returns positive if a,b,c are wound in ccw order, negative otherwise
// https://www.quora.com/What-is-the-signed-Area-of-the-triangle
static inline r32 signedAreaV2(V2 a, V2 b, V2 c)
{
    V2 ac = subV2(a, c);
    V2 bc = subV2(b, c);
    
    r32 result = 0.5f * (bc.x*ac.y - ac.x*bc.y);
    
    return result;
}

#define MATH_H
#endif

