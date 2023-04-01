#ifndef PLATFORM_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <zmq.h>

#if BUILD_DEBUG
#define assert(expression) if(!(expression)) {*(int*)0 = 0;}
#else
#define assert(expression)
#endif

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;

typedef i32 bool32;

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define terabytes(value) (gigabytes(value)*1024LL)

#define PI 3.14159265

#define arrayLength(array) (sizeof(array)/sizeof(array[0]))

struct MemoryArena
{
    size_t size;
    size_t used;
    uint8_t* base;
    uint32_t tempCount;
};

inline void initMemoryArena(MemoryArena* arena, size_t size, void* base)
{
    arena->size = size;
    arena->base = (uint8_t*)base;
    arena->used = 0;
    arena->tempCount = 0;
}

#define pushStruct(arena, type) (type*)pushSize_(arena, sizeof(type))
#define pushSize(arena, size) pushSize_(arena, size)
inline void* pushSize_(MemoryArena* arena, size_t size)
{
    assert((arena->used + size) <= arena->size);
    
    void* result = arena->base + arena->used;
    arena->used += size;
    
    return result;
}

struct TemporaryMemory
{
    MemoryArena* arena;
    size_t usedAtStart;
};

inline TemporaryMemory beginTemporaryMemory(MemoryArena* arena)
{
    TemporaryMemory result;
    
    result.arena = arena;
    result.usedAtStart = arena->used;
    
    arena->tempCount++;
    
    return result;
}

inline void endTemporaryMemory(TemporaryMemory temp)
{
    MemoryArena* arena = temp.arena;
    
    assert(arena->used >= temp.usedAtStart);
    arena->used = temp.usedAtStart;
    
    assert(arena->tempCount > 0);
    arena->tempCount--;
}

inline void flushMemory(MemoryArena* arena) 
{
    arena->used = 0;
}

struct TransientState
{
    bool32 isInitialized;
    MemoryArena arena;
};

struct ButtonState
{
    bool32 isDown;
    bool32 wasDown;
};

struct InputState
{
    ButtonState buttonSpace;
    ButtonState buttonTab;
    ButtonState buttonEsc;
    ButtonState buttonReset;
    ButtonState buttonLeft;
    ButtonState buttonRight;
    ButtonState buttonYes;
    ButtonState buttonNo;
};

struct ReadFileResult
{
    const char* filename;
    i32 contentSize;
    void* content;
    i32 readIndex;
};

// Returns the length of a zero ended c-style string
static i32 stringLength(const char* str)
{
    i32 result = 0;
    
    const char* readPtr = str;
    while (*readPtr != '\0')
    {
        result++;
        readPtr++;
    }
    
    return result;
}

static void freeFileMemory(ReadFileResult* file)
{
    file->contentSize = 0;
    free(file->content);
}

static bool32 readEntireFile(const char* filename,
                             ReadFileResult* file)
{
    bool32 result = 0;
    
    i32 fileDescriptor = open(filename,
                              O_RDONLY);
    if (fileDescriptor != -1)
    {
        struct stat fileStats = {};
        bool32 statResult = fstat(fileDescriptor, &fileStats);
        
        if (statResult == -1)
        {
            printf("Could not stat file %s\n", filename);
        }
        else
        {
            file->content = (u8*)malloc(fileStats.st_size);
            if (!file->content)
            {
                freeFileMemory(file);
            }
            else
            {
                file->filename = filename;
                file->contentSize = fileStats.st_size;
                i32 bytesToRead = file->contentSize;
                u8* buffer = (u8*)file->content;
                while (bytesToRead)
                {
                    i32 bytesRead = read(fileDescriptor, buffer, bytesToRead);
                    if (bytesRead == -1)
                    {
                        freeFileMemory(file);
                        break;
                    }
                    
                    bytesToRead -= bytesRead;
                    buffer += bytesRead;
                    
                    printf("%i bytes read\n", bytesRead);
                }
                
                if (bytesToRead > 0)
                {
                    freeFileMemory(file);
                }
                else
                {
                    file->readIndex = 0;
                    result = 1;
                }
            }
        }
        
        close(fileDescriptor);
    }
    else
    {
        printf("Could not open file %s\n", filename);
    }
    
    return result;
}

static void readFromFileResult(ReadFileResult* file,
                               void* buffer,
                               i32 bytesToRead,
                               bool32 resetToBeginning = 1)
{
    assert(file->contentSize >= (file->readIndex + bytesToRead));
    assert(file->content);
    
    memcpy(buffer,
           ((u8*)file->content) + file->readIndex,
           bytesToRead);
    
    file->readIndex += bytesToRead;
    
    if (resetToBeginning && file->contentSize == file->readIndex)
    {
        file->readIndex = 0;
    }
}

static i32 openFileForWriting(const char* filename)
{
    i32 result = open(filename,
                      O_WRONLY|O_CREAT|O_TRUNC,
                      S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    
    return result;
}

static void closeFile(i32 fileDescriptor)
{
    close(fileDescriptor);
}

static bool32 writeToFile(i32 fileDescriptor,
                          const void* buffer,
                          i32 bytesToWrite)
{
    assert(fileDescriptor != -1);
    
    bool32 result = 1;
    
    u8* writePtr = (u8*)buffer;
    while (bytesToWrite)
    {
        i32 bytesWritten = write(fileDescriptor, buffer, bytesToWrite);
        if (bytesWritten == -1)
        {
            result = 0;
            break;
        }
        
        bytesToWrite -= bytesWritten;
        writePtr += bytesWritten;
        
        printf("%i bytes written\n", bytesWritten);
    }
    
    return result;
}

static bool32 writeToFile(const char* filename,
                          const void* buffer,
                          i32 bytesToWrite)
{
    bool32 result = 1;
    
    i32 fileDescriptor = openFileForWriting(filename);
    if (fileDescriptor != -1)
    {
        result = writeToFile(fileDescriptor,
                             buffer,
                             bytesToWrite);
        closeFile(fileDescriptor);
    }
    else
    {
        printf("Could not open file %s\n", filename);
    }
    
    return result;
}

static u64 getWallclockTimeInMs()
{
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    u64 result = t.tv_sec * 1000 + t.tv_nsec / 1000000;
    
    return result;
}

static void getTimeString(char* buf,
                          int bufferSize)
{
    time_t t = time(NULL);
    tm* time = localtime(&t);
    
    snprintf(buf, bufferSize, "%i-%i-%i_%i-%i-%i",
             time->tm_year + 1900,
             time->tm_mon + 1,
             time->tm_mday,
             time->tm_hour,
             time->tm_min,
             time->tm_sec);
}

struct Frame
{
    void* memory;
    i32 width, height;
    i32 bytesPerPixel;
    i32 pitch;
    i32 size;
};

static inline Frame initializeFrame(MemoryArena* arena,
                                    i32 width, i32 height,
                                    i32 bytesPerPixel)
{
    Frame result = {};
    
    result.width = width;
    result.height = height;
    result.bytesPerPixel = bytesPerPixel;
    result.pitch = width * bytesPerPixel;
    result.size = result.pitch * height;
    result.memory = pushSize(arena, result.size);
    
    return result;
}

static inline Frame initializeFrame(MemoryArena* arena,
                                    i32 width, i32 height,
                                    i32 bytesPerPixel,
                                    i32 pitch,
                                    i32 size)
{
    assert((width * bytesPerPixel * height) <= size);
    Frame result = {};
    
    result.width = width;
    result.height = height;
    result.bytesPerPixel = bytesPerPixel;
    result.pitch = pitch;
    result.size = size;
    result.memory = pushSize(arena, size);
    
    return result;
}

static inline Frame initializeFrame(void* memory,
                                    i32 width, i32 height,
                                    i32 bytesPerPixel,
                                    i32 pitch,
                                    i32 size)
{
    assert((width * bytesPerPixel * height) <= size);
    Frame result = {};
    
    result.width = width;
    result.height = height;
    result.bytesPerPixel = bytesPerPixel;
    result.pitch = pitch;
    result.size = size;
    result.memory = memory;
    
    return result;
}

struct bool32Vector
{
    bool32* values;
    i32 count;
    i32 maxCount;
};

static inline bool32Vector initializebool32Vector(MemoryArena* arena,
                                                  i32 maxCount)
{
    bool32Vector result;
    
    result.count = 0;
    result.maxCount = maxCount;
    result.values = 
        (bool32*)pushSize(arena, maxCount * sizeof(bool32));
    
    return result;
}

static inline void pushbool32(MemoryArena* arena,
                              bool32Vector* vector,
                              bool32 value)
{
    if(vector->count >= vector->maxCount)
    {
        bool32* oldValues = vector->values;
        i32 oldMaxCount = vector->maxCount;
        
        vector->maxCount *= 2;
        vector->values = 
            (bool32*)pushSize(arena, vector->maxCount * sizeof(bool32));
        memcpy(vector->values, 
               oldValues, 
               oldMaxCount * sizeof(bool32));
    }
    
    vector->values[vector->count] = value;
    
    vector->count++;
}

#define PLATFORM_H
#endif
