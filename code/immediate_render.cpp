#include "immediate_render.h"

Immediate_Render_Context *Global_Immediate_Render_Context = null;

void init(Immediate_Render_Context *context) {
    *context = {};
    
    glGenVertexArrays(1, &context->vertex_array_object);
    glGenBuffers(COUNT_WITH_ARRAY(context->buffer_objects));
    
    glBindVertexArray(context->vertex_array_object);
    
    Vertex_Attribute_Info attribute_infos[] = {
        { Vertex_Position_Index, 3, GL_FLOAT, GL_FALSE },
        { Vertex_Color_Index,    4, GL_UNSIGNED_BYTE, GL_TRUE },
    };
    
    set_vertex_attributes(context->vertex_buffer_object, ARRAY_INFO(attribute_infos));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void
draw_line(vec3f from, vec3f to, rgba32 from_color, rgba32 to_color) {
    assert(Global_Immediate_Render_Context);
    
    auto context = Global_Immediate_Render_Context;
    
    u32 vertex_offset = Global_Immediate_Render_Context->vertices.count;
    
    *push(context->allocator, &context->vertices) = { transform_point(context->world_to_camera_transform, from), from_color };
    *push(context->allocator, &context->vertices) = { transform_point(context->world_to_camera_transform, to),   to_color };
    
    *push(context->allocator, &context->line_indices) = vertex_offset + 0;
    *push(context->allocator, &context->line_indices) = vertex_offset + 1;
}

INTERNAL void
draw_line(vec3f from, vec3f to, rgba32 color) {
    draw_line(from, to, color, color);
}

void
draw_rect(vec3f bottom_left, vec3f width, vec3f height, rgba32 color, bool is_filled)
{
    assert(Global_Immediate_Render_Context);
    auto context = Global_Immediate_Render_Context;
    u32 vertex_offset = Global_Immediate_Render_Context->vertices.count;
    
    *push(context->allocator, &context->vertices) = { transform_point(context->world_to_camera_transform, bottom_left), color };
    *push(context->allocator, &context->vertices) = { transform_point(context->world_to_camera_transform, bottom_left + width), color };
    *push(context->allocator, &context->vertices) = { transform_point(context->world_to_camera_transform, bottom_left + width + height), color };
    *push(context->allocator, &context->vertices) = { transform_point(context->world_to_camera_transform, bottom_left + height), color };
    
    if (is_filled) {
        *push(context->allocator, &context->triangle_indices) = vertex_offset + 0;
        *push(context->allocator, &context->triangle_indices) = vertex_offset + 1;
        *push(context->allocator, &context->triangle_indices) = vertex_offset + 3;
        
        *push(context->allocator, &context->triangle_indices) = vertex_offset + 3;
        *push(context->allocator, &context->triangle_indices) = vertex_offset + 1;
        *push(context->allocator, &context->triangle_indices) = vertex_offset + 2;
    }
    else {
        *push(context->allocator, &context->line_indices) = vertex_offset + 0;
        *push(context->allocator, &context->line_indices) = vertex_offset + 1;
        
        *push(context->allocator, &context->line_indices) = vertex_offset + 1;
        *push(context->allocator, &context->line_indices) = vertex_offset + 2;
        
        *push(context->allocator, &context->line_indices) = vertex_offset + 2;
        *push(context->allocator, &context->line_indices) = vertex_offset + 3;
        
        *push(context->allocator, &context->line_indices) = vertex_offset + 3;
        *push(context->allocator, &context->line_indices) = vertex_offset + 0;
    }
}

void
draw_circle(vec3f center, f32 radius, vec3f plane_normal, rgba32 color, bool is_filled, u32 corner_count)
{
    assert(Global_Immediate_Render_Context);
    assert(corner_count > 2);
    auto context = Global_Immediate_Render_Context;
    u32 vertex_offset = Global_Immediate_Render_Context->vertices.count;
    
    mat4x3f rotation = make_transform(make_quat(plane_normal, Pi32 * 2.0f / cast_v(f32, corner_count)));
    
    vec3f corner;
    if (are_close(abs(dot(plane_normal, VEC3_Y_AXIS)), 1.0f))
        corner = normalize(cross(plane_normal, -VEC3_Z_AXIS));
    else
        corner = normalize(cross(plane_normal, VEC3_Y_AXIS));
    
    for (u32 i = 0; i != corner_count; ++i) {
        *push(context->allocator, &context->vertices) = { transform_point(context->world_to_camera_transform, corner * radius + center), color };
        corner = transform_direction(rotation, corner);
    }
    
    if (is_filled) {
        *push(context->allocator, &context->triangle_indices) = vertex_offset + 0;
        *push(context->allocator, &context->triangle_indices) = vertex_offset + 1;
        *push(context->allocator, &context->triangle_indices) = vertex_offset + corner_count - 1;
        
        for (u16 i = 1; i < corner_count/2; ++i) {
            
            *push(context->allocator, &context->triangle_indices) = vertex_offset + i - 1;
            *push(context->allocator, &context->triangle_indices) = vertex_offset + corner_count - i - 1;
            *push(context->allocator, &context->triangle_indices) = vertex_offset + i;
            
            *push(context->allocator, &context->triangle_indices) = vertex_offset + corner_count - i - 1;
            *push(context->allocator, &context->triangle_indices) = vertex_offset + i;
            *push(context->allocator, &context->triangle_indices) = vertex_offset + corner_count - i;
        }
        
        *push(context->allocator, &context->triangle_indices) = vertex_offset + corner_count/2;
        
        // if corner_count is odd, add one more index
        if (corner_count & 1) 
            *push(context->allocator, &context->triangle_indices) = vertex_offset + corner_count/2 + 1;
    }
    else {
        for (u16 i = 0; i != corner_count; ++i) {
            *push(context->allocator, &context->line_indices) = vertex_offset + i;
            *push(context->allocator, &context->line_indices) = vertex_offset + MOD(i + 1, corner_count);
        }
    }
}

INTERNAL void
draw_circle(vec3f center, f32 radius, rgba32 color, bool is_filled, u32 corner_count) {
    assert(Global_Immediate_Render_Context);
    
    vec3f plane_normal = {
        Global_Immediate_Render_Context->world_to_camera_transform.columns[0][2],
        Global_Immediate_Render_Context->world_to_camera_transform.columns[1][2],
        Global_Immediate_Render_Context->world_to_camera_transform.columns[2][2]
    };
    
    draw_circle(center, radius, plane_normal, color, is_filled, corner_count);
}

void draw_begin(Immediate_Render_Context *context, Memory_Allocator *allocator, mat4x3f world_to_camera_transform) {
    assert(!Global_Immediate_Render_Context);
    
    context->world_to_camera_transform = world_to_camera_transform;
    context->allocator = allocator;
    
    if (context->vertices.capacity) {
        context->vertices.data = ALLOCATE_ARRAY(allocator, Immediate_Render_Vertex, context->vertices.capacity);
        context->vertices.count = 0;
    }
    
    if (context->triangle_indices.capacity) {
        context->triangle_indices.data = ALLOCATE_ARRAY(allocator, u16, context->triangle_indices.capacity);
        context->triangle_indices.count = 0;
    }
    
    if (context->line_indices.capacity) {
        context->line_indices.data = ALLOCATE_ARRAY(allocator, u16, context->line_indices.capacity);
        context->line_indices.count = 0;
    }
    
    Global_Immediate_Render_Context = context;
}

void draw_end(Immediate_Render_Context *context) {
    assert(Global_Immediate_Render_Context);
    Global_Immediate_Render_Context = null;
    
    defer {
        // free data but keep count
        if (context->line_indices.count)
            free(context->allocator, context->line_indices.data);
        
        if (context->triangle_indices.count)
            free(context->allocator, context->triangle_indices.data);
        
        if (context->vertices.count)
            free(context->allocator, context->vertices.data);
    };
    
    if (context->triangle_indices.count + context->line_indices.count == 0)
        return;
    
    glBindBuffer(GL_ARRAY_BUFFER, context->vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, byte_count_of(context->vertices), context->vertices.data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glBindVertexArray(context->vertex_array_object);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->index_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, byte_count_of(context->triangle_indices) + byte_count_of(context->line_indices), null, GL_STREAM_DRAW);
    
    {
        GLsizei offset = 0;
        
        if (context->triangle_indices.count) {
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, byte_count_of(context->triangle_indices), context->triangle_indices.data);
            offset += byte_count_of(context->triangle_indices);
        }
        
        if (context->line_indices.count) {
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, byte_count_of(context->line_indices), context->line_indices.data);
            offset += byte_count_of(context->line_indices);
        }
    }
    
    {
        usize offset = 0;
        
        if (context->triangle_indices.count) {
            glDrawElements(GL_TRIANGLES, context->triangle_indices.count, GL_UNSIGNED_SHORT, cast_p(GLvoid, offset));
            offset += byte_count_of(context->triangle_indices);
        }
        
        if (context->line_indices.count) {
            glDrawElements(GL_LINES, context->line_indices.count, GL_UNSIGNED_SHORT, cast_p(GLvoid, offset));
            offset += byte_count_of(context->line_indices);
        }
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
