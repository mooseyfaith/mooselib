#if !defined IMMEDIATE_RENDER_H
#define IMMEDIATE_RENDER_H

#include "render_batch.h"

#pragma pack(push, 1)
struct Immediate_Render_Vertex {
    vec3f position;
    rgba32 color;
};
#pragma pack(pop)

#define Template_Array_Type      Immediate_Render_Vertex_Buffer
#define Template_Array_Data_Type Immediate_Render_Vertex
#define Template_Array_Is_Buffer
#include "template_array.h"

struct Immediate_Render_Context {
    Memory_Allocator *allocator;
    
    Immediate_Render_Vertex_Buffer  vertices;
    u16_buffer                      triangle_indices;
    u16_buffer                      line_indices;
    
    mat4x3f world_to_camera_transform;
    
    GLuint vertex_array_object;
    union {
        struct { GLuint vertex_buffer_object, index_buffer_object; };
        GLuint buffer_objects[2];
    };
};

extern Immediate_Render_Context *Global_Immediate_Render_Context;

void
init(Immediate_Render_Context *context);

void
draw_line(vec3f from, vec3f to, rgba32 from_color, rgba32 to_color);

INTERNAL void
draw_line(vec3f from, vec3f to, rgba32 color);

void
draw_rect(vec3f bottom_left, vec3f width, vec3f height, rgba32 color, bool is_filled = false);

void
draw_circle(vec3f center, f32 radius, vec3f plane_normal, rgba32 color, bool is_filled = false, u32 corner_count = 32);

INTERNAL void
draw_circle(vec3f center, f32 radius, rgba32 color, bool is_filled = false, u32 corner_count = 32);

void 
draw_begin(Immediate_Render_Context *context, Memory_Allocator *allocator, mat4x3f world_to_camera_transform);

void
draw_end(Immediate_Render_Context *context);

#endif // IMMEDIATE_RENDER_H