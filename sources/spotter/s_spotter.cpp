#include "s_spotter.h"

static void initApplication(ApplicationState* state,
                            MemoryArena* arena,
                            u32 windowWidth,
                            u32 windowHeight,
                            Calibration* calibration,
                            std::string ip)
{
    state->status = ApplicationStatus_Idle;
    state->windowWidth = windowWidth;
    state->windowHeight = windowHeight;
    state->ip = ip;
    state->binarizationThreshold = 180;
    
    state->grayscaleFrame = initializeFrame(arena,
                                            windowWidth, windowHeight,
                                            1);
    state->binarizedFrame = initializeFrame(arena,
                                            windowWidth, windowHeight,
                                            1);
    state->calibration = {};
    if (calibration)
    {
        state->calibration = *calibration;
    }
    else
    {
        state->calibration.fx = 468.9f;
        state->calibration.fy = 468.9f;
        state->calibration.cx = CAPTURE_FRAME_WIDTH / 2;
        state->calibration.cy = CAPTURE_FRAME_HEIGHT / 2;
    }
}

static void writeToFrame(void* context, void* data, int size)
{
    Frame* frame = (Frame*)context;
    memcpy(((u8*)frame->memory) + frame->size,
           data,
           size);
    frame->size += size;
}
