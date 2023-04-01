#ifndef CAPTURE_H

#define CAPTURE_BUFFER_COUNT 3
#define CAPTURE_MEM_TYPE_USERPTR 1

struct CaptureState
{
    i32 fd;
    v4l2_buf_type type;
    u32 memoryType;
    u32 bufferCount;
    Frame buffers[CAPTURE_BUFFER_COUNT];
    i32 readBufferIndex;
    i32 saveFileDescriptor;
};

#define CAPTURE_H
#endif

