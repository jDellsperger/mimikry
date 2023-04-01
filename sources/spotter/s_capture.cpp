#include "s_capture.h"

#define printErrno() _printErrno(__FILE__, __LINE__)
static void _printErrno(const char* file, i32 line)
{
    printf("%s, line %i - errno %d: %s\n", file, line, errno,
           strerror(errno));
}

static i32 xioctl(i32 fd, i32 request, void *arg)
{
    i32 result = 0;
    i32 ioResult;
    
    do 
    {
        ioResult = ioctl(fd, request, arg);
    } 
    while (ioResult == -1 && errno == EINTR);
    
    if (ioResult != -1)
    {
        result = 1;
    }
    
    return result;
}

static bool32 readFrame(CaptureState* state,
                        Frame** outputFrame)
{
    bool32 result = 0;
    
    v4l2_buffer buf = {};
    buf.type = state->type;
    buf.memory = state->memoryType;
    
#if 1
    while (!xioctl(state->fd, VIDIOC_DQBUF, &buf))
    {
        if (errno == EAGAIN)
        {
            usleep(10);
            continue;
        }
        else
        {
            printErrno();
            return result;
        }
    }
    
    assert(buf.index < state->bufferCount);
    result = 1;
    *outputFrame = &state->buffers[buf.index];
    state->readBufferIndex = buf.index;
#else
    while (xioctl(state->fd, VIDIOC_DQBUF, &buf))
    {
        if (errno == EAGAIN)
        {
            usleep(10);
            continue;
        }
        else
        {
            printErrno();
            return result;
        }
    }
    
    assert(buf.index < state->bufferCount);
    outputFrame = &state->buffers[buf.index];
    state->readBufferIndex = buf.index;
#endif
    
    return result;
}

static bool32 requeueBuffer(CaptureState* state)
{
    bool32 result = 1;
    
    Frame* buffer = &state->buffers[state->readBufferIndex];
    
    v4l2_buffer buf = {};
    buf.type = state->type;
    buf.memory = state->memoryType;
    buf.m.userptr = (u64)buffer->memory;
    buf.length = buffer->size;
    
    if (!xioctl(state->fd, VIDIOC_QBUF, &buf))
    {
        printErrno();
        result = 0;
    }
    
    return result;
}

static i32 stopCapturing(CaptureState* state)
{
    // NOTE(jan): turn capture stream off
    if (!xioctl(state->fd, VIDIOC_STREAMOFF, &state->type))
    {
        printErrno();
        return 0;
    }
    
    // NOTE(jan): close device
    if (close(state->fd) == -1)
    {
        printf("Cannot close device\n");
        printErrno();
        return 0;
    }
    
    state->fd = -1;
    
    return 1;
}

static bool32 startCapturing(MemoryArena* arena,
                             CaptureState* state,
                             u32 bufferCount,
                             const char* devName)
{
    assert(bufferCount <= CAPTURE_BUFFER_COUNT);
    
    state->bufferCount = bufferCount;
    state->fd = -1;
    state->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
#if CAPTURE_MEM_TYPE_USERPTR
    state->memoryType = V4L2_MEMORY_USERPTR;
#else
    state->memoryType = V4L2_MEMORY_MMAP;
#endif
    
    // NOTE(jan): open device
    struct stat st;
    if (stat(devName, &st) == -1)
    {
        printf("Cannot stat device %s: errno %d, %s\n", 
               devName, errno, strerror(errno));
        return 0;
    }
    
    if (!S_ISCHR(st.st_mode))
    {
        printf("Device is no devicen %s\n", devName);
    }
    
    state->fd = open(devName, O_RDWR | O_NONBLOCK);
    if (state->fd == -1)
    {
        printf("Cannot open device %s: errno %d, %s\n",
               devName, errno, strerror(errno));
        return 0;
    }
    
    // NOTE(jan): query capabilities of capture device
    // and assure they have the required ones (video capture, 
    // streaming I/O) set
    v4l2_capability cap = {};
    if (!xioctl(state->fd, VIDIOC_QUERYCAP, &cap))
    {
        if (errno == EINVAL)
        {
            printf("No V4L2 device\n");
        }
        else
        {
            printf("VIDIOC_QUERYCAP errno: %d, %s\n", errno, strerror(errno));
        }
        return 0;
    }
    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        printf("Device is no video capture device\n");
        return 0;
    }
    
    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        printf("Device does not support streaming i/o\n");
        return 0;
    }
    
    // NOTE(jan): set required capture format
    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    fmt.fmt.pix.width = CAPTURE_FRAME_WIDTH;
    fmt.fmt.pix.height = CAPTURE_FRAME_HEIGHT;
    //fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;
    
    if (!xioctl(state->fd, VIDIOC_S_FMT, &fmt))
    {
        printf("VIDIOC_S_FMT errno: %d, %s\n", errno, strerror(errno));
    }
    
    // NOTE(jan): find and set highest possible frametime
    v4l2_streamparm parm = {};
    parm.type = state->type;
    if (!xioctl(state->fd, VIDIOC_G_PARM, &parm))
    {
        printf("Could not get streaming parameters\n");
        printErrno();
        return 0;
    }
    
    // NOTE(jan): check if adjusting frametime is supported
    if (!(parm.parm.capture.capability | V4L2_CAP_TIMEPERFRAME))
    {
        printf("Setting frametime not supported\n");
        printErrno();
        return 0;
    }
    
    v4l2_frmivalenum frameIntervals = {};
    //frameIntervals.pixel_format = V4L2_PIX_FMT_YUYV;
    frameIntervals.pixel_format = V4L2_PIX_FMT_GREY;
    frameIntervals.width = CAPTURE_FRAME_WIDTH;
    frameIntervals.height = CAPTURE_FRAME_HEIGHT;
    
    i32 intervalIndex = 0;
    i32 bestIntervalIndex = 0;
    u32 maxDenominator = 0;
    
    // NOTE(jan): find the highest possible frametime
    while (xioctl(state->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameIntervals))
    {
        intervalIndex++;
        frameIntervals.index = intervalIndex;
        
        if (frameIntervals.discrete.denominator > maxDenominator)
        {
            bestIntervalIndex = intervalIndex;
        }
        
        printf("Possible interval: %i / %i\n", 
               frameIntervals.discrete.numerator,
               frameIntervals.discrete.denominator);
    }
    
    // NOTE(jan): set the frametime to the highest possible found
    frameIntervals.index = bestIntervalIndex;
    xioctl(state->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameIntervals);
    
    frameIntervals.discrete.numerator = 1;
    frameIntervals.discrete.denominator = 40;
    printf("Best interval: %i / %i\n", 
           frameIntervals.discrete.numerator,
           frameIntervals.discrete.denominator);
    
    parm.parm.capture.timeperframe = frameIntervals.discrete;
    if (!xioctl(state->fd, VIDIOC_S_PARM, &parm))
    {
        printf("Could not set streaming parameters\n");
        printErrno();
        return 0;
    }
    
    xioctl(state->fd, VIDIOC_S_PARM, &parm);
    
    // NOTE(jan): request buffers
    v4l2_requestbuffers req = {};
    
    req.count = state->bufferCount;
    req.type = state->type;
    req.memory = state->memoryType;
    
    if (!xioctl(state->fd, VIDIOC_REQBUFS, &req))
    {
        printf("No support for user pointer i/o\n");
        printErrno();
        return 0;
    }
    
    state->bufferCount = req.count;
    printf("Got %i camera buffers\n", req.count);
    
    // NOTE(jan): init buffers
    for (u32 i = 0; i < state->bufferCount; i++)
    {
        Frame* buffer = &state->buffers[i];
        
        v4l2_buffer buf = {};
        
        buf.type = state->type;
        buf.index = i;
        buf.memory = state->memoryType;
        
#if CAPTURE_MEM_TYPE_USERPTR
        *buffer = initializeFrame(arena,
                                  CAPTURE_FRAME_WIDTH,
                                  CAPTURE_FRAME_HEIGHT,
                                  1,
                                  fmt.fmt.pix.bytesperline,
                                  fmt.fmt.pix.sizeimage);
        
        buf.m.userptr = (u64)buffer->memory;
        buf.length = buffer->size;
#else
        if (!xioctl(state->fd,
                    VIDIOC_QUERYBUF,
                    &buf))
        {
            printErrno();
            return 0;
        }
        
        void* frameMemory = mmap(0,
                                 buf.length,
                                 PROT_READ | PROT_WRITE,
                                 MAP_SHARED,
                                 state->fd, buf.m.offset);
        *buffer = initializeFrame(frameMemory,
                                  CAPTURE_FRAME_WIDTH,
                                  CAPTURE_FRAME_HEIGHT,
                                  1,
                                  fmt.fmt.pix.bytesperline,
                                  buf.length);
#endif
        
        if (i == 0)
        {
            if (!xioctl(state->fd, VIDIOC_QBUF, &buf))
            {
                printErrno();
                return 0;
            }
        }
    }
    
    // NOTE(jan): turn capturestream on
    if (!xioctl(state->fd, VIDIOC_STREAMON, &state->type))
    {
        printErrno();
        return 0;
    }
    
    return 1;
}
