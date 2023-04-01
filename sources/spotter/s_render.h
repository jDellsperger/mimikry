#ifndef RENDER_H

const char* camVertexShaderSource =
"#version 300 es\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoord;\n"
"uniform mat4 transform;\n"
"out vec2 texCoord;\n"
"void main() {\n"
"gl_Position = transform * vec4(aPos, 0.0f, 1.0f);\n"
"texCoord = aTexCoord;\n"
"}\n";

const char* camFragmentShaderSource =
"#version 300 es\n"
"precision mediump float;\n"
"out vec4 fragColor;\n"
"in vec2 texCoord;\n"
"uniform sampler2D ourTexture;\n"
"void main() {\n"
"fragColor = texture(ourTexture, texCoord);\n"
"}\n";

const char* rectVertexShaderSource =
"#version 300 es\n"
"layout (location = 0) in vec2 aPos;\n"
"uniform mat4 transform;\n"
"void main() {\n"
"gl_Position = transform * vec4(aPos, 0.0f, 1.0f);\n"
"}\n";

const char* rectFragmentShaderSource =
"#version 300 es\n"
"precision mediump float;\n"
"out vec4 fragColor;\n"
"uniform vec4 color;\n"
"void main() {\n"
"fragColor = color;\n"
"}\n";

const char* lineVertexShaderSource =
"#version 300 es\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 cTw;\n"
"void main() {\n"
"gl_Position = vec4(aPos, 1.0f);\n"
"}\n";

const char* lineFragmentShaderSource =
"#version 300 es\n"
"precision mediump float;\n"
"uniform vec4 color;\n"
"out vec4 fragColor;\n"
"void main() {\n"
"fragColor = color;\n"
"}\n";

enum RenderStatus
{
    RenderStatus_None,
    RenderStatus_Camera,
    RenderStatus_Binarized,
    RenderStatus_Harris,
    RenderStatus_Last
};

struct RenderBuffer
{
    void* memory;
    u32 length;
    i32 width, height;
    i32 bytesPerPixel;
    i32 pitch;
};

struct RenderState
{
    // OpenGL stuff
    u32 camShaderProgram;
    u32 rectShaderProgram;
    u32 lineShaderProgram;
    u32 camVbo;
    u32 rectVbo;
    u32 texture;
    u32 rgbTexture;
    
    // General stuff
    RenderStatus status;
    u32 windowWidth;
    u32 windowHeight;
};

struct RenderGroup
{
    u32 bufferSize;
    u32 bufferUsed;
    u8* bufferBase;
    TemporaryMemory renderMemory;
    RenderState* state;
};

enum RenderElementType
{
    RenderElementType_RenderElementClear,
    RenderElementType_RenderElementRectangle,
    RenderElementType_RenderElementBuffer,
    RenderElementType_RenderElementLine
};

struct RenderElementClear
{
    RenderElementType type;
};

struct RenderElementRectangle
{
    RenderElementType type;
    V3 min, max;
    V4 color;
};

struct RenderElementBuffer
{
    RenderElementType type;
    u32 texture;
};

struct RenderElementLine
{
    RenderElementType type;
    V3 start, end;
    V4 color;
};

#define pushRenderElement(group, type) (type*)pushRenderElement_(group, sizeof(type), RenderElementType_##type)
inline void* pushRenderElement_(RenderGroup* group, 
                                u32 size, 
                                RenderElementType type)
{
    RenderElementType* result = 0;
    if ((group->bufferUsed + size) < group->bufferSize)
    {
        result = (RenderElementType*)(group->bufferBase + group->bufferUsed);
        *result = type;
        group->bufferUsed += size;
    }
    
    return result;
}

#define RENDER_H
#endif
