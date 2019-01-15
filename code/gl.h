#if !defined ENGINE_OPEN_GL_H
#define ENGINE_OPEN_GL_H

#include <stdio.h>
#include <windows.h>
#include <gl\GL.h>
#include <gl\glext.h>
#include <gl\wglext.h>

#include "basic.h"
#include "mathdefs.h"
#include "mo_string.h"

#define GL_LOAD_PROC(type, name) { name = reinterpret_cast<type> (wglGetProcAddress(# name)); assert(name); }

PFNGLGETSTRINGIPROC glGetStringi = NULL;

PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = NULL;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;

PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = NULL;

PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = NULL;
PFNGLDRAWBUFFERSPROC glDrawBuffers = NULL;

PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = NULL;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = NULL;

PFNGLGENBUFFERSPROC    glGenBuffers = NULL;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
PFNGLBINDBUFFERPROC    glBindBuffer = NULL;
PFNGLMAPBUFFERPROC     glMapBuffer = null;
PFNGLUNMAPBUFFERPROC   glUnmapBuffer = null;

PFNGLBUFFERDATAPROC glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glBufferSubData = NULL;

PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;

PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;

PFNGLCREATESHADERPROC glCreateShader = NULL;
PFNGLDELETESHADERPROC glDeleteShader = NULL;
PFNGLSHADERSOURCEPROC glShaderSource = NULL;
PFNGLCOMPILESHADERPROC glCompileShader = NULL;
PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;

PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
PFNGLATTACHSHADERPROC glAttachShader = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
PFNGLUSEPROGRAMPROC glUseProgram = NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
PFNGLGETUNIFORMINDICESPROC glGetUniformIndices = NULL;
PFNGLGETACTIVEUNIFORMNAMEPROC glGetActiveUniformName = NULL;
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;

PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = NULL;

PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = NULL;

PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f = null;
PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv = null;

PFNGLUNIFORM1IPROC  glUniform1i = NULL;
PFNGLUNIFORM1UIPROC glUniform1ui = NULL;

PFNGLUNIFORM1FPROC glUniform1f = NULL;
PFNGLUNIFORM2FPROC glUniform2f = NULL;
PFNGLUNIFORM3FPROC glUniform3f = NULL;
PFNGLUNIFORM4FPROC glUniform4f = NULL;

PFNGLUNIFORM2FVPROC glUniform2fv = NULL;
PFNGLUNIFORM3FVPROC glUniform3fv = NULL;
PFNGLUNIFORM4FVPROC glUniform4fv = NULL;

PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv = NULL;

PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex = null;
PFNGLBINDBUFFERBASEPROC       glBindBufferBase = null;
PFNGLUNIFORMBLOCKBINDINGPROC  glUniformBlockBinding = null;

void init_wgl(HDC window_device_context) {
    GL_LOAD_PROC(PFNGLGETSTRINGIPROC, glGetStringi);
    
    GLint extensions_count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);
    
    struct {
        const GLubyte *extension_name;
        bool is_available;
    } search_extensions[] = {
        { (const GLubyte *) "WGL_ARB_create_context", false }
    };
    
    GL_LOAD_PROC(PFNWGLGETEXTENSIONSSTRINGARBPROC, wglGetExtensionsStringARB);
    
    const char *wgl_extensions_string = wglGetExtensionsStringARB(window_device_context);
    
    for (GLuint i = 0; i != extensions_count; ++i) {
        const GLubyte *ext_name = glGetStringi(GL_EXTENSIONS, i);
        //printf("extension %i %s\n", i, ext_name);
        
        for (u32 j = 0; j < 1; ++j)
            if (strcmp((const char*)ext_name, (const char*)search_extensions[j].extension_name) == 0) {
            search_extensions[j].is_available = true;
            //printf("found extension %s (%u)", ext_name, (unsigned int)j);
        }
    }
    
    //printf("found %i GL extensions\n", extensions_count);
    
    // TODO: check if extensions are available
    GL_LOAD_PROC(PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB);
    GL_LOAD_PROC(PFNWGLCREATECONTEXTATTRIBSARBPROC, wglCreateContextAttribsARB);
    GL_LOAD_PROC(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT);
}

void init_gl() {
    GL_LOAD_PROC(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback);
    
    GL_LOAD_PROC(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers);
    GL_LOAD_PROC(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer);
    GL_LOAD_PROC(PFNGLFRAMEBUFFERTEXTUREPROC, glFramebufferTexture);
    GL_LOAD_PROC(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D);
    GL_LOAD_PROC(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer);
    GL_LOAD_PROC(PFNGLDRAWBUFFERSPROC, glDrawBuffers);
    
    GL_LOAD_PROC(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers);
    GL_LOAD_PROC(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer);
    GL_LOAD_PROC(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage);
    
    GL_LOAD_PROC(PFNGLGENBUFFERSPROC, glGenBuffers);
    GL_LOAD_PROC(PFNGLDELETEBUFFERSPROC, glDeleteBuffers);
    GL_LOAD_PROC(PFNGLBINDBUFFERPROC, glBindBuffer);
    GL_LOAD_PROC(PFNGLMAPBUFFERPROC, glMapBuffer);
    GL_LOAD_PROC(PFNGLUNMAPBUFFERPROC, glUnmapBuffer);
    
    
    GL_LOAD_PROC(PFNGLBUFFERDATAPROC, glBufferData);
    GL_LOAD_PROC(PFNGLBUFFERSUBDATAPROC, glBufferSubData);
    
    GL_LOAD_PROC(PFNGLCREATEVERTEXARRAYSPROC, glGenVertexArrays);
    GL_LOAD_PROC(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray);
    
    GL_LOAD_PROC(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray);
    GL_LOAD_PROC(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray);
    GL_LOAD_PROC(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer);
    
    GL_LOAD_PROC(PFNGLCREATESHADERPROC, glCreateShader);
    GL_LOAD_PROC(PFNGLDELETESHADERPROC, glDeleteShader);
    
    GL_LOAD_PROC(PFNGLSHADERSOURCEPROC, glShaderSource);
    GL_LOAD_PROC(PFNGLCOMPILESHADERPROC, glCompileShader);
    GL_LOAD_PROC(PFNGLGETSHADERIVPROC, glGetShaderiv);
    GL_LOAD_PROC(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog);
    
    GL_LOAD_PROC(PFNGLCREATEPROGRAMPROC, glCreateProgram);
    GL_LOAD_PROC(PFNGLDELETEPROGRAMPROC, glDeleteProgram);
    GL_LOAD_PROC(PFNGLATTACHSHADERPROC, glAttachShader);
    GL_LOAD_PROC(PFNGLLINKPROGRAMPROC, glLinkProgram);
    GL_LOAD_PROC(PFNGLUSEPROGRAMPROC, glUseProgram);
    GL_LOAD_PROC(PFNGLGETPROGRAMIVPROC, glGetProgramiv);
    GL_LOAD_PROC(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog);
    GL_LOAD_PROC(PFNGLGETUNIFORMINDICESPROC, glGetUniformIndices);
    GL_LOAD_PROC(PFNGLGETACTIVEUNIFORMNAMEPROC, glGetActiveUniformName);
    GL_LOAD_PROC(PFNGLGETACTIVEUNIFORMPROC, glGetActiveUniform);
    GL_LOAD_PROC(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation);
    
    GL_LOAD_PROC(PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation);
    
    GL_LOAD_PROC(PFNGLACTIVETEXTUREPROC, glActiveTexture);
    GL_LOAD_PROC(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap);
    
    GL_LOAD_PROC(PFNGLVERTEXATTRIB3FPROC, glVertexAttrib3f);
    GL_LOAD_PROC(PFNGLVERTEXATTRIB4FPROC, glVertexAttrib4f);
    GL_LOAD_PROC(PFNGLVERTEXATTRIB4FVPROC, glVertexAttrib4fv);
    
    GL_LOAD_PROC(PFNGLUNIFORM1IPROC,  glUniform1i);
    GL_LOAD_PROC(PFNGLUNIFORM1UIPROC, glUniform1ui);
    
    GL_LOAD_PROC(PFNGLUNIFORM1FPROC, glUniform1f);
    GL_LOAD_PROC(PFNGLUNIFORM2FPROC, glUniform2f);
    GL_LOAD_PROC(PFNGLUNIFORM3FPROC, glUniform3f);
    GL_LOAD_PROC(PFNGLUNIFORM4FPROC, glUniform4f);
    GL_LOAD_PROC(PFNGLUNIFORM2FVPROC, glUniform2fv);
    GL_LOAD_PROC(PFNGLUNIFORM3FVPROC, glUniform3fv);
    GL_LOAD_PROC(PFNGLUNIFORM4FVPROC, glUniform4fv);
    GL_LOAD_PROC(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv);
    GL_LOAD_PROC(PFNGLUNIFORMMATRIX4X3FVPROC, glUniformMatrix4x3fv);
    
    GL_LOAD_PROC(PFNGLGETUNIFORMBLOCKINDEXPROC, glGetUniformBlockIndex);
    GL_LOAD_PROC(PFNGLBINDBUFFERBASEPROC, glBindBufferBase);
    GL_LOAD_PROC(PFNGLUNIFORMBLOCKBINDINGPROC, glUniformBlockBinding);
}

struct rgb32 {
    union {
        struct {
            u8 red, green, blue;
        };
        struct {
            u8 r, g, b;
        };
        u8 values[3];
    };
};

inline rgb32 make_rgb32(float gray) {
    return rgb32{ CAST_V(u8, 255 * gray), CAST_V(u8, 255 * gray), CAST_V(u8, 255 * gray) };
}

inline rgb32 make_rgb32(f32 red, f32 green, f32 blue) {
    return rgb32{ CAST_V(u8, 255 * red), CAST_V(u8, 255 * green), CAST_V(u8, 255 * blue) };
}

inline rgb32 make_rgb32(vec3f color) {
    return make_rgb32(color.r, color.g, color.b);
}

struct rgba32 {
    union {
        struct {
            u8 red, green, blue, alpha;
        };
        struct {
            u8 r, g, b, a;
        };
        u32 value;
    };
};

inline bool operator==(rgba32 a, rgba32 b) {
    return a.value == b.value;
}

inline bool operator!=(rgba32 a, rgba32 b) {
    return a.value != b.value;
}

inline rgba32 make_rgba32(f32 gray) {
    return rgba32{ CAST_V(u8, 255 * gray), CAST_V(u8, 255 * gray), CAST_V(u8, 255 * gray), 255 };
}

inline rgba32 make_rgba32(f32 red, f32 green, f32 blue, f32 alpha = 1.0f) {
    return rgba32{ CAST_V(u8, 255 * red), CAST_V(u8, 255 * green), CAST_V(u8, 255 * blue), CAST_V(u8, 255 * alpha)  };
}

inline rgba32 make_rgba32(vec3f color, f32 alpha = 1.0f) {
    return make_rgba32(color.r, color.g, color.b, alpha);
}

inline rgba32 make_rgba32(vec4f color) {
    return make_rgba32(color.r, color.g, color.b, color.a);
}

struct Texture {
    GLuint object;
    Pixel_Dimensions resolution;
    //GLint internal_format;
    GLenum format;
};

union area2f {
    struct {
        vec2f min, size;
    };
    
    struct {
        f32 x, y;
        f32 width, height;
    };
    
};

#define Template_Area_Name area2f
#define Template_Area_Struct_Is_Declared
#define Template_Area_Vector_Type vec2f
#include "template_area.h"

area2f make_uv_rect(Texture *texture, Pixel_Rectangle rect) {
    return {
        rect.x / CAST_V(f32, texture->resolution.width), 
        rect.y / CAST_V(f32, texture->resolution.height), 
        rect.width / CAST_V(f32, texture->resolution.width), 
        rect.height / CAST_V(f32, texture->resolution.height)
    };
}

INTERNAL f32 left(area2f area) {
    return area.min.x;
}

INTERNAL f32 right(area2f area) {
    return area.min.x + area.size.x;
}

INTERNAL f32 bottom(area2f area) {
    return area.min.y;
}

INTERNAL f32 top(area2f area) {
    return area.min.y + area.size.y;
}

struct Frame_Buffer {
    GLuint object; 
    GLuint color_attachment0_texture_object;
    GLuint depth_attachment_texture_object;
    //GLuint depth_render_buffer_object;
    Pixel_Dimensions resolution;
};

struct Shader_Attribute_Info {
    GLuint index;
    GLchar *attribute_name;
};

enum Texture_Filter_Level {
    Texture_Filter_Level_Nearest,
    Texture_Filter_Level_Linear,
};

void set_texture_filter_level(GLuint texture_object, u32 filter_level) {
    assert(texture_object);
    
    glBindTexture(GL_TEXTURE_2D, texture_object);
    
    GLenum filter;
    switch (filter_level) {
        case Texture_Filter_Level_Nearest:
        filter = GL_NEAREST;
        break;
        
        case Texture_Filter_Level_Linear:
        filter = GL_LINEAR;
        break;
        
        default:
        UNREACHABLE_CODE;
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

Texture make_alpha_texture(GLsizei width, GLsizei height, u32 filter_level = Texture_Filter_Level_Linear) {
    Texture result;
    result.resolution = { CAST_V(s16, width), CAST_V(s16, height) };
    
    glGenTextures(1, &result.object);
    glBindTexture(GL_TEXTURE_2D, result.object);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);
    //glGenerateMipmap(GL_TEXTURE_2D);
    
    GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    
    set_texture_filter_level(result.object, filter_level);
    
    return result;
}

Texture make_blank_texture() {
    Texture result;
    result.resolution = { 1, 1 };
    
    glGenTextures(1, &result.object);
    glBindTexture(GL_TEXTURE_2D, result.object);
    
    u8 color = 255;
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1, 1, 0, GL_RED, GL_UNSIGNED_BYTE, &color);
    
    GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
    
    set_texture_filter_level(result.object, Texture_Filter_Level_Nearest);
    
    return result;
}

Texture make_single_color_texture(rgb32 color) {
    Texture result;
    result.resolution = { 1, 1 };
    
    glGenTextures(1, &result.object);
    glBindTexture(GL_TEXTURE_2D, result.object);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, &color);
    
    set_texture_filter_level(result.object, Texture_Filter_Level_Nearest);
    
    return result;
}

Frame_Buffer make_frame_buffer(Pixel_Dimensions resolution, bool create_color_texture = true) {
    Frame_Buffer frame_buffer;
    frame_buffer.resolution = resolution;
    
    glGenFramebuffers(1, &frame_buffer.object);
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.object);
    
    if (create_color_texture)
    {
        glGenTextures(1, &frame_buffer.color_attachment0_texture_object);
        glBindTexture(GL_TEXTURE_2D, frame_buffer.color_attachment0_texture_object);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, resolution.width, resolution.height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        
        //	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, frame_buffer.color_attachment0_texture_object, 0);
    }
    else {
        frame_buffer.color_attachment0_texture_object = 0;
    }
    
    // depth texture
    
    glGenTextures(1, &frame_buffer.depth_attachment_texture_object);
    glBindTexture(GL_TEXTURE_2D, frame_buffer.depth_attachment_texture_object);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution.width, resolution.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    
    
#if 0    
    glGenRenderbuffers(1, &frame_buffer.depth_render_buffer_object);
    glBindRenderbuffer(GL_RENDERBUFFER, frame_buffer.depth_render_buffer_object);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, resolution.width, resolution.height);
#endif
    
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, frame_buffer.depth_attachment_texture_object, 0);
    
#if 0    
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, frame_buffer.depth_render_buffer_object);
#endif
    
    GLenum color_buffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(ARRAY_COUNT(color_buffers), color_buffers);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return frame_buffer;
}

void bind_frame_buffer(Frame_Buffer frame_buffer) {
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer.object);
    glViewport(0, 0, frame_buffer.resolution.width, frame_buffer.resolution.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

GLuint make_shader_object(GLenum type, string *shader_sources, u32 source_count, Memory_Allocator *temporary_allocator)
{
    GLuint shader_object = glCreateShader(type);
    
    GLchar **sources = ALLOCATE_ARRAY(temporary_allocator, GLchar *, source_count);
    GLint *source_lengths = ALLOCATE_ARRAY(temporary_allocator, GLint, source_count);
    for (u32 source_index = 0; source_index < source_count; ++source_index) {
        sources[source_index] = CAST_P(GLchar, shader_sources[source_index].data);
        source_lengths[source_index] = CAST_V(GLint, shader_sources[source_index].count);
    }
    
    glShaderSource(shader_object, source_count, sources, source_lengths);
    
    free(temporary_allocator, source_lengths);
    free(temporary_allocator, sources);
    
    glCompileShader(shader_object);
    GLint is_compiled;
    glGetShaderiv(shader_object, GL_COMPILE_STATUS, &is_compiled);
    if (is_compiled == GL_FALSE) {
        GLint info_length;
        glGetShaderiv(shader_object, GL_INFO_LOG_LENGTH, &info_length);
        
        char *info = ALLOCATE_ARRAY(temporary_allocator, char, info_length);
        glGetShaderInfoLog(shader_object, info_length, &info_length, info);
        
        if (type == GL_VERTEX_SHADER)
            printf("could not compile vertex shader:\n %s\n", info);
        else
            printf("could not compile fragment shader:\n %s\n", info);
        
        free(temporary_allocator, info);
        glDeleteShader(shader_object);
        return 0;
    }
    
    return shader_object;
}

#define UNIFORM_NAMES_(uniform_list) S(STRINGIFY(uniform_list))
#define UNIFORM_NAMES(uniform_list)  UNIFORM_NAMES_(STRINGIFY(uniform_list)))

void  get_uniforms(GLint *uniform_locations, u32 uniform_count, GLuint program_object, string uniform_names, Memory_Allocator *temporary_allocator)
{
    string uniform_it = uniform_names;
    
    for (u32 i = 0; i < uniform_count; ++i) {
        skip_set(&uniform_it, S(" \t\b\r\n\0"));
        string uniform_name = skip_until_first_in_set(&uniform_it, S(",\0"), true);
        assert(uniform_name.count);
        skip_set(&uniform_it, S(" \t\b\r\n\0"));
        
        // convert to c string
        // add c string terminator
        char *c_uniform_name = ALLOCATE_ARRAY(temporary_allocator, char, uniform_name.count + 1);
        COPY(c_uniform_name, uniform_name.data, uniform_name.count);
        c_uniform_name[uniform_name.count] = '\0';
        
        uniform_locations[i] = glGetUniformLocation(program_object, c_uniform_name);
        
        free(temporary_allocator, c_uniform_name);
    }
}

GLuint make_shader_program(GLuint *shader_objects, u32 shader_object_count, bool delete_shader_objects, Shader_Attribute_Info *attribute_infos, u32 attribute_info_count, Memory_Allocator *temporary_allocator)
{
    GLuint program_object = glCreateProgram();
    
    for (u32 i = 0; i < shader_object_count; ++i)
        glAttachShader(program_object, shader_objects[i]);
    
    if (delete_shader_objects) {
        for (u32 i = 0; i < shader_object_count; ++i) {
            glDeleteShader(shader_objects[i]);
            shader_objects[i] = -1;
        }
    }
    
    for (u32 i = 0; i < attribute_info_count; ++i)
        glBindAttribLocation(program_object, attribute_infos[i].index, attribute_infos[i].attribute_name);
    
    glLinkProgram(program_object);
    
    GLint is_linked;
    glGetProgramiv(program_object, GL_LINK_STATUS, &is_linked);
    if (is_linked == GL_FALSE) {
        GLint info_length;
        glGetProgramiv(program_object, GL_INFO_LOG_LENGTH, &info_length);
        
        char *info = ALLOCATE_ARRAY(temporary_allocator, char, info_length);
        glGetProgramInfoLog(program_object, info_length, &info_length, info);
        
        printf("could not link gl program: %s\n", info);
        free(temporary_allocator, info);
        glDeleteProgram(program_object);
        
        return 0;
    }
    
    return program_object;
}

#if 0
GLuint make_shader_program(string vertex_shader_source, string fragment_shader_source, Shader_Attribute_Info *attribute_infos, u32 attribute_info_count, string uniform_names, OUTPUT GLint *uniform_locations, u32 uniform_count, Memory_Allocator *temporary_allocator)
{
    GLuint shader_objects[2];
    if (!(shader_objects[0] = make_shader_object(GL_VERTEX_SHADER, &vertex_shader_source, 1, temporary_allocator)))
        return 0;
    
    defer { glDeleteShader(shader_objects[0]); };
    
    if (!(shader_objects[1] = make_shader_object(GL_FRAGMENT_SHADER, &fragment_shader_source, 1, temporary_allocator)))
        return 0;
    
    defer { glDeleteShader(shader_objects[1]); };
    
    return make_shader_program(ARRAY_WITH_COUNT(shader_objects), false, attribute_infos, attribute_info_count, uniform_names, uniform_locations, uniform_count, temporary_allocator);
}
#endif

Pixel_Dimensions get_auto_render_resolution(Pixel_Dimensions window_resolution, Pixel_Dimensions reference_resolution) {
    assert(reference_resolution.width && reference_resolution.height);
    
    if (window_resolution.width == 0 || window_resolution.height == 0)
        return {};
    
    Pixel_Dimensions render_resolution;
    
    {
        f32 height_ratio = reference_resolution.height / CAST_V(f32, window_resolution.height);
        f32 width_ratio  = reference_resolution.width  / CAST_V(f32, window_resolution.width);
        
        f32 referenceaspect_ratio = width_over_height(reference_resolution);
        f32 window_aspect_ratio = width_over_height(window_resolution);
        
        if (height_ratio >= width_ratio) {
            render_resolution.width = CAST_V(s16, window_resolution.width * referenceaspect_ratio / window_aspect_ratio + 0.5f);
            render_resolution.height = window_resolution.height;
            
            // if the difference results in a 1 pixel border on both sides, ignore it
            if ((render_resolution.width - window_resolution.width) % 4)
                render_resolution.width -= (render_resolution.width - window_resolution.width) % 4;
        }
        else {
            render_resolution.width = window_resolution.width;
            render_resolution.height = CAST_V(s16, window_resolution.height * window_aspect_ratio / referenceaspect_ratio + 0.5f);
            
            // if the difference results in a 1 pixel border on both sides, ignore it
            if ((render_resolution.height - window_resolution.height) % 4)
                render_resolution.height -= (render_resolution.height - window_resolution.height) % 4;
        }
    }
    
    return render_resolution;
}

void set_auto_viewport(Pixel_Dimensions window_resolution, Pixel_Dimensions render_resolution, vec4f clear_color, vec4f border_color = vec4f{0.05f, 0.05f, 0.05f, 1.0f})
{
    s16 border_width  = (window_resolution.width - render_resolution.width) >> 1;
    s16 border_height = (window_resolution.height - render_resolution.height) >> 1;
    
    if (border_width || border_height) {
        glClearColor(border_color.red, border_color.green, border_color.blue, border_color.alpha);
        glViewport(0, 0, window_resolution.width, window_resolution.height);
        glDisable(GL_SCISSOR_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    glClearColor(clear_color.red, clear_color.green, clear_color.blue, clear_color.alpha);
    glViewport(border_width, border_height, render_resolution.width, render_resolution.height);
    glScissor(border_width, border_height, render_resolution.width, render_resolution.height);
    
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

vec2f get_clip_mouse_position(vec2f window_mouse_position, Pixel_Dimensions window_resolution, Pixel_Dimensions render_resolution) {
    vec2f viewport_mouse_pos = window_mouse_position - vec2f{ CAST_V(f32, window_resolution.width - render_resolution.width >> 1), CAST_V(f32, window_resolution.height - render_resolution.height >> 1) };
    
    vec2f relative_mouse_pos = ((viewport_mouse_pos - vec2f{ CAST_V(f32, render_resolution.width), CAST_V(f32, render_resolution.height) } * 0.5f) / CAST_V(f32, render_resolution.height)) * vec2f { 2.0f, -2.0f };
    
    return viewport_mouse_pos / vec2f{ CAST_V(f32, render_resolution.width), CAST_V(f32, -render_resolution.height) } * 2.0f - vec2f{ 1.0f, -1.0f };
}

#endif // ENGINE_OPEN_GL_H
