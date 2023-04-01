#include "s_render.h"

static RenderGroup* createRenderGroup(MemoryArena* arena,
                                      RenderState* renderState,
                                      u32 bufferSize)
{
    RenderGroup* result = pushStruct(arena, RenderGroup);
    result->state = renderState;
    result->bufferSize = bufferSize;
    result->bufferUsed = 0;
    result->bufferBase = (uint8_t*)pushSize(arena, bufferSize);
    return result;
}

static void pushClear(RenderGroup* group)
{
    pushRenderElement(group, RenderElementClear);
}

static void pushRect(RenderGroup* group, 
                     V3 min, V3 max, 
                     V4 color)
{
    RenderElementRectangle* result;
    result = pushRenderElement(group, RenderElementRectangle);
    
    result->max = max;
    result->min = min;
    result->color = color;
}

static void pushLine(RenderGroup* group,
                     V3 start, V3 end,
                     V4 color)
{
    RenderElementLine* result;
    result = pushRenderElement(group, RenderElementLine);
    
    result->start = start;
    result->end = end;
    result->color = color;
}

static void pushImageBuffer(RenderGroup* group,
                            void* bufferMemory,
                            i32 bufferWidth,
                            i32 bufferHeight,
                            i32 bufferPitch,
                            i32 bufferBytesPerPixel)
{
    assert(bufferBytesPerPixel == 1 || 
           bufferBytesPerPixel == 3);
    
    RenderElementBuffer* result;
    result = pushRenderElement(group, RenderElementBuffer);
    
    if (bufferBytesPerPixel == 1)
    {
        result->texture = group->state->texture;
        glBindTexture(GL_TEXTURE_2D, result->texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                        bufferWidth, bufferHeight,
                        GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        bufferMemory);
    }
    else if (bufferBytesPerPixel == 3)
    {
        result->texture = group->state->rgbTexture;
        glBindTexture(GL_TEXTURE_2D, result->texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                        bufferWidth, bufferHeight,
                        GL_RGB, GL_UNSIGNED_BYTE,
                        bufferMemory);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void clear()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

static void drawLine(u32 shaderProgram,
                     V3 start, V3 end,
                     V4 color)
{
    // NOTE(jan): flip y axis
    start.y = 1.0f - start.y;
    end.y = 1.0f - end.y;
    
    // NOTE(jan): convert coordinates to NDC
    V3 s = v3((2.0f * start.x) - 1.0f,
              (2.0f * start.y) - 1.0f,
              0.0f);
    V3 e = v3((2.0f * end.x) - 1.0f,
              (2.0f * end.y) - 1.0f,
              0.0f);
    r32 vertices[] = {
        s.x, s.y, s.z,
        e.x, e.y, e.z
    };
    
    glUseProgram(shaderProgram);
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(r32), 0);
    glEnableVertexAttribArray(0);
    
    u32 colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform4f(colorLoc, color.r, color.g, color.b, color.a);
    
    glDrawArrays(GL_LINES, 0, 2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

static void drawRect(u32 shaderProgram, 
                     u32 vbo, 
                     V3 min, V3 max, 
                     V4 color)
{
    // NOTE(jan): flip y axis
    min.y = 1.0f - min.y;
    max.y = 1.0f - max.y;
    
    // NOTE(jan): convert coordinates to NDC
    V3 center = v3((min.x + max.x) - 1.0f,
                   (min.y + max.y) - 1.0f,
                   0.0f);
    r32 width = max.x - min.x;
    r32 height = max.y - min.y;
    Transform t;
    t.translation = center;
    t.scale = v3(width, height, 1.0f);
    
    glUseProgram(shaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    
    u32 colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform4f(colorLoc, color.r, color.g, color.b, color.a);
    
    M4x4 transform = transformationMatrix(&t);
    u32 transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, elementPtr(&transform));
    
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

static void drawBufferTexture(u32 shaderProgram, 
                              u32 vbo, 
                              u32 texture)
{
    glUseProgram(shaderProgram);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 
                          4 * sizeof(r32), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
                          4 * sizeof(r32), (void*)(2 * sizeof(r32)));
    glEnableVertexAttribArray(1);
    
    Transform t;
    t.scale = v3(1.0f, -1.0f, 1.0f);
    t.translation = v3(0.0f, 0.0f, 0.0f);
    M4x4 transform = transformationMatrix(&t);
    u32 transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, elementPtr(&transform));
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

static void drawRenderGroup(RenderGroup* group)
{
    uint32_t rendered = 0;
    while (rendered < group->bufferUsed)
    {
        RenderElementType* elementType =
            (RenderElementType*)(group->bufferBase + rendered);
        
        switch (*elementType)
        {
            case RenderElementType_RenderElementClear: {
                RenderElementClear* element = 
                    (RenderElementClear*)elementType;
                clear();
                rendered += sizeof(*element);
            } break;
            
            case RenderElementType_RenderElementRectangle: {
                RenderElementRectangle* element =
                    (RenderElementRectangle*)elementType;
                drawRect(group->state->rectShaderProgram, 
                         group->state->rectVbo,
                         element->min, element->max,
                         element->color);
                rendered += sizeof(*element);
            } break;
            
            case RenderElementType_RenderElementBuffer: {
                RenderElementBuffer* element =
                    (RenderElementBuffer*)elementType;
                drawBufferTexture(group->state->camShaderProgram,
                                  group->state->camVbo,
                                  element->texture);
                rendered += sizeof(*element);
            } break;
            
            case RenderElementType_RenderElementLine: {
                RenderElementLine* element =
                    (RenderElementLine*)elementType;
                drawLine(group->state->lineShaderProgram,
                         element->start, element->end,
                         element->color);
                rendered += sizeof(*element);
            } break;
        }
    }
}

static bool32 compileShader(GLenum shaderType,
                            const char* shaderSource,
                            GLuint* shader)
{
    bool32 success;
    GLchar infoLog[512];
    *shader = glCreateShader(shaderType);
    glShaderSource(*shader, 1, &shaderSource, 0);
    glCompileShader(*shader);
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &success);
    
    if (!success)
    {
        glGetShaderInfoLog(*shader, 512, NULL, infoLog);
        printf("Error when compiling shader of type %i - %s",
               shaderType, infoLog);
    }
    
    return success;
}

static bool32 linkShaderProgram(GLuint* shaderProgram,
                                const char* vertexShaderSource,
                                const char* fragmentShaderSource)
{
    bool32 success = 0;
    
    GLuint vertexShader, fragmentShader;
    success = compileShader(GL_FRAGMENT_SHADER,
                            fragmentShaderSource, &fragmentShader);
    if (!success)
    {
        printf("Failed to compile fragment shader\n");
    }
    else
    {
        success = compileShader(GL_VERTEX_SHADER, 
                                vertexShaderSource, &vertexShader);
        if (!success)
        {
            printf("Failed to compile vertex shader\n");
        }
        else
        {
            GLchar infoLog[512];
            *shaderProgram = glCreateProgram();
            glAttachShader(*shaderProgram, fragmentShader);
            glAttachShader(*shaderProgram, vertexShader);
            glLinkProgram(*shaderProgram);
            glGetProgramiv(*shaderProgram, GL_LINK_STATUS, &success);
            
            if (!success)
            {
                glGetProgramInfoLog(*shaderProgram, 512, NULL, infoLog);
                printf("Shader program linking failed - %s\n", infoLog);
            }
        }
        
        glDeleteShader(vertexShader);
    }
    
    glDeleteShader(fragmentShader);
    
    return success;
}

static bool32 initRenderer(RenderState* state, 
                           u32 windowWidth, u32 windowHeight)
{
    bool32 success = 1;
    
    state->status = RenderStatus_Camera;
    state->windowWidth = windowWidth;
    state->windowHeight = windowHeight;
    
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    
    state->camShaderProgram = glCreateProgram();
    success = linkShaderProgram(&state->camShaderProgram, 
                                camVertexShaderSource, 
                                camFragmentShaderSource);
    if (!success)
    {
        printf("Failed to link shader program %i at %s, %i\n",
               state->camShaderProgram, __FILE__, __LINE__);
    }
    else
    {
        glUseProgram(state->camShaderProgram);
        
        r32 camVertices[] = {
            // positions      // texture coords
            1.0f, 1.0f, 1.0f, 1.0f,   // top right
            1.0f, -1.0f, 1.0f, 0.0f,   // bottom right
            -1.0f, -1.0f, 0.0f, 0.0f,   // bottom left
            1.0f, 1.0f, 1.0f, 1.0f,   // top right 
            -1.0f, -1.0f, 0.0f, 0.0f,   // bottom left
            -1.0f, 1.0f, 0.0f, 1.0f    // top left
        };
        
        glGenBuffers(1, &state->camVbo);
        glBindBuffer(GL_ARRAY_BUFFER, state->camVbo);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(camVertices), 
                     camVertices, GL_DYNAMIC_DRAW);
        
        state->rectShaderProgram = glCreateProgram();
        success = linkShaderProgram(&state->rectShaderProgram,
                                    rectVertexShaderSource,
                                    rectFragmentShaderSource);
        if (!success)
        {
            printf("Failed to link shader program at %s, %i\n",
                   __FILE__, __LINE__);
        }
        
        glUseProgram(state->rectShaderProgram);
        
        r32 rectVertices[] = {
            // positions
            1.0f, 1.0f, // top right
            1.0f, -1.0f, // bottom right
            -1.0f, -1.0f, // bottom left
            -1.0f, 1.0f, // top left
        };
        
        glGenBuffers(1, &state->rectVbo);
        glBindBuffer(GL_ARRAY_BUFFER, state->rectVbo);
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), 
                     rectVertices, GL_DYNAMIC_DRAW);
        
        // TODO(jan): is this necessary?
        glBindBuffer(GL_ARRAY_BUFFER, state->camVbo);
        
        glGenTextures(1, &state->texture);
        glBindTexture(GL_TEXTURE_2D, state->texture);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, windowWidth, windowHeight,
                     0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glGenTextures(1, &state->rgbTexture);
        glBindTexture(GL_TEXTURE_2D, state->rgbTexture);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowWidth, windowHeight,
                     0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glUseProgram(0);
        
        state->lineShaderProgram = glCreateProgram();
        success = linkShaderProgram(&state->lineShaderProgram,
                                    lineVertexShaderSource,
                                    lineFragmentShaderSource);
        if (!success)
        {
            printf("Failed to link shader program at %s, %i\n",
                   __FILE__, __LINE__);
        }
        
    }
    
    return success;
}
