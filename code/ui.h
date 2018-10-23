#if !defined UI_H
#define UI_H

#include "memory_growing_stack.h"
#include "font.h"
#include "render_batch.h"

#pragma pack(push, 1)
struct UI_Vertex {
    vec3f  position;
    rgba32 color;
    vec2f  uv;
};
#pragma pack(pop)

enum UI_Command_Kind {
    UI_Command_Kind_Draw,
    UI_Command_Kind_Texture,
};

struct UI_Command {
    u32 kind;
    
    union {
        struct {
            GLenum mode;
            u32 index_count;
        } draw;
        
        struct {
            GLuint object;
        } texture;
    };
    
};

#define Template_Array_Type      UI_Vertex_Array
#define Template_Array_Data_Type UI_Vertex
#include "template_array.h"

#define Template_Array_Type      UI_Command_Array
#define Template_Array_Data_Type UI_Command
#include "template_array.h"

enum UI_Control_State {
    UI_Control_State_Disabled,
    UI_Control_State_Idle,
    UI_Control_State_Hot,
    UI_Control_State_Active,
    UI_Control_State_Triggered
};

struct UI_Context {
    Memory_Growing_Stack vertex_memory;
    Memory_Growing_Stack index_memory;
    Memory_Growing_Stack command_memory;
    
    UI_Vertex_Array vertices;
    u16_array indices;
    UI_Command_Array commands;
    
    union { mat4x3f world_to_camera_transform, transform; };
    f32 depth;
    f32 scale;
    
    bool use_sissor_area;
    UV_Area sissor_area;
    
    struct {
        s16 left, right, bottom, top;
        s16 center_x, center_y;
        s16 width, height;
    };
    
    Texture *texture;
    Texture *blank_texture;
    
    UI_Command *previous_texture_command, *current_texture_command;
    
    struct {
        Font *font;
        rgba32 color;
        s16 line_spacing;
        f32 line_grow_direction;
        f32 scale;
        vec2f alignment; // -1 to 1 (left to right), (bottom to top)
        bool do_render;
    } font_rendering;
    
    union {
        struct { GLuint vertex_buffer_object, index_buffer_object; };
        GLuint buffer_objects[2];
    };
    
    GLuint vertex_array_object;
    
    struct {
        u32 hot_id;
        u32 active_id;
        u32 triggerd_id;
        u32 current_id;
        
        UV_Area cursor_area;
        bool cursor_was_pressed;
        bool cursor_was_released;
        
        u32 next_hot_id;
        f32 next_hot_priority;
    } control;
};

UI_Context make_ui_context(Memory_Allocator *internal_allocator, Texture *blank_texture) {
    UI_Context context = {};
    context.vertex_memory  = make_memory_growing_stack(internal_allocator);
    context.index_memory   = make_memory_growing_stack(internal_allocator);
    context.command_memory = make_memory_growing_stack(internal_allocator);
    context.vertices = {};
    context.indices  = {};
    context.commands = {};
    
    assert(blank_texture);
    context.texture = blank_texture;
    context.world_to_camera_transform = MAT4X3_IDENTITY;
    context.depth = 0.0f;
    context.scale = 1.0f;
    
    context.font_rendering.color = rgba32{ 0, 0, 0, 255 };
    context.font_rendering.font = null;
    context.font_rendering.alignment = { -1.0f, -1.0f }; // align left, bottom
    context.font_rendering.line_grow_direction = -1.0f;
    context.font_rendering.scale = 1.0f;
    context.font_rendering.do_render = true;
    
    {
        glGenVertexArrays(1, &context.vertex_array_object);
        glGenBuffers(COUNT_WITH_ARRAY(context.buffer_objects));
        
        glBindVertexArray(context.vertex_array_object);
        
        Vertex_Attribute_Info attribute_infos[] = {
            { Vertex_Position_Index, 3, GL_FLOAT, GL_FALSE },
            { Vertex_Color_Index,    4, GL_UNSIGNED_BYTE, GL_TRUE },
            { Vertex_UV_Index,       2, GL_FLOAT, GL_FALSE },
        };
        
        set_vertex_attributes(context.vertex_buffer_object, ARRAY_WITH_COUNT(attribute_infos));
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    auto control = &context.control;
    
    control->hot_id = -1;
    control->active_id = -1;
    control->next_hot_id = -1;
    control->triggerd_id = -1;
    
    return context;
}

void ui_set_transform(UI_Context *context, Pixel_Dimensions resolution, f32 scale = 1.0f, f32 depth_scale = 1.0f, f32 depth_alignment = 0.0f)
{
    assert((depth_scale > 0.0f) && (depth_scale <= 1.0f));
    assert((depth_alignment >= 0.0f) && (depth_alignment <= 1.0f));
    
    f32 y_up_direction = 1.0f;
    
    // for now using y_up_direction == -1 dows not seem to work properly, it will reverse face drawing order and will be culled if glEnable(GL_CULL_FACE), wich is the default
    assert(y_up_direction == 1.0f);
    
    context->transform.columns[0] = {  2 * scale / (resolution.width), 0,                               0           };
    context->transform.columns[1] = {  0,                                  2 * scale / (resolution.height), 0           };
    context->transform.columns[2] = {  0,                                  0,                               depth_scale };
    //context->transform.columns[3] = {  scale / resolution.width - 1,       scale / resolution.height - 1,   depth_alignment * (1.0f - depth_scale) };
    context->transform.columns[3] = { -1,                                 -1,   depth_alignment * (1.0f - depth_scale) };
    
#if 0                                                                                                                          
    
    context->transform.columns[0] = vec3f{ 2.0f * scale / resolution.width };
    context->transform.columns[1] = vec3f{ 0.0f, 2.0f * scale * y_up_direction / resolution.height };
    context->transform.columns[2] = vec3f{ 0.0f, 0.0f, depth_scale };
    //context->transform.columns[2] = vec3f{ scale / (resolution.width - 1), scale / (resolution.height - 1), depth_scale };
    //context->transform.columns[3] = vec3f{ -1.f, -1.0f * y_up_direction, depth_alignment * (1.0f - depth_scale) };
    
    context->transform.columns[3] = vec3f{
        -1.0f + context->transform.columns[0][0] * 0.5f,
        -1.0f * y_up_direction + context->transform.columns[1][1] * 0.5f,
        depth_alignment * (1.0f - depth_scale) 
    };
#endif
    
    context->scale = scale;
    //context->y_up_direction = y_up_direction;
    
    context->left   = 0.0f;
    context->right  = (resolution.width - 1) / scale;
    context->top    = (resolution.height - 1) / scale * interval_zero_to_one(y_up_direction);
    context->bottom = (resolution.height - 1) / scale - context->top;
    
    context->width  = resolution.width / scale;
    context->height = resolution.height / scale;
    
    context->center_x = (context->left + context->right)  * 0.5f;
    context->center_y = (context->top  + context->bottom) * 0.5f;
}

void ui_flush(UI_Context *context) {
    glBindBuffer(GL_ARRAY_BUFFER, context->vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, byte_count(context->vertices), context->vertices.data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glBindVertexArray(context->vertex_array_object);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->index_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, byte_count(context->indices), context->indices.data, GL_STREAM_DRAW);
}

void ui_draw(UI_Context *context) {
    
    usize offset = 0;
    
    glActiveTexture(GL_TEXTURE0 + 0);
    
    GLuint current_texture_object = 0;
    
    for (auto it = context->commands + 0, end = it + context->commands.count; it != end; ++it)
    {
        switch (it->kind) {
            case UI_Command_Kind_Draw: {
                glDrawElements(it->draw.mode, it->draw.index_count, GL_UNSIGNED_SHORT, cast_p(GLvoid, offset));
                offset += it->draw.index_count * sizeof(u16);
            } break;
            
            case UI_Command_Kind_Texture: {
                if (current_texture_object != it->texture.object) {
                    current_texture_object = it->texture.object;
                    glBindTexture(GL_TEXTURE_2D, it->texture.object);
                }
            } break;
            
            default:
            UNREACHABLE_CODE;
        }
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ui_clear(UI_Context *context) {
    clear(&context->vertex_memory);
    clear(&context->index_memory);
    clear(&context->command_memory);
    
    context->vertices = {};
    context->indices  = {};
    context->commands = {};
    
    context->previous_texture_command = null;
    context->current_texture_command = null;
    context->texture = null;
}

void _push_quad_as_triangles(UI_Context *context, u32 vertex_offset, u32 a, u32 b, u32 c, u32 d, bool only_edge = false) {
    
    GLenum draw_mode;
    u32 index_count;
    
    if (only_edge) {
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + a;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + b;
        
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + b;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + c;
        
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + c;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + d;
        
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + d;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + a;
        
        draw_mode = GL_LINES;
        index_count = 8;
    }
    else {
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + a;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + b;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + d;
        
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + d;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + b;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + c;
        
        draw_mode = GL_TRIANGLES;
        index_count = 6;
    }
    
    UI_Command *command;
    if (context->commands.count && (context->commands[context->commands.count - 1].kind == UI_Command_Kind_Draw) && (context->commands[context->commands.count - 1].draw.mode == draw_mode))
    {
        command = context->commands + context->commands.count - 1;
    }
    else {
        command = grow(&context->command_memory.allocator, &context->commands);
        command->kind = UI_Command_Kind_Draw;
        command->draw.mode = draw_mode;
        command->draw.index_count = 0;
    }
    
    command->draw.index_count += index_count;
}

vec2f _uv_from_position(UI_Context *context, s16 origin_x, s16 origin_y, s16 x, s16 y) {
    return { cast_v(f32, x - origin_x) / context->texture->resolution.width, cast_v(f32, y - origin_y) / context->texture->resolution.height };
    
}

Texture * ui_texture(UI_Context *context, Texture *texture) {
    if (context->texture == texture)
        return texture;
    
    auto old_texture = context->texture;
    context->texture = texture;
    
    if (!context->commands.count || (context->commands[context->commands.count - 1].kind != UI_Command_Kind_Texture))
    {
        auto command = grow(&context->command_memory.allocator, &context->commands);
        command->kind = UI_Command_Kind_Texture;
    }
    
    context->commands[context->commands.count - 1].texture.object = texture->object;
    
    return old_texture;
}

bool ui_clip(UI_Context *context, UV_Area *draw_area, UV_Area *texture_area = null) {
    if (context->use_sissor_area) {
        vec2f texture_scale;
        
        if (texture_area)
            texture_scale = { texture_area->size.width / draw_area->size.width, texture_area->size.height / draw_area->size.height };
        
        bool is_valid;
        UV_Area new_area = intersection(&is_valid, *draw_area, context->sissor_area);
        
        if (!is_valid)
            return false;
        
        if (texture_area) {
            texture_area->min.x += (new_area.min.x - draw_area->min.x) * texture_scale.x;
            texture_area->min.y += (new_area.min.y - draw_area->min.y) * texture_scale.y;
            texture_area->size.width   += (new_area.size.width   - draw_area->size.width)   * texture_scale.x;
            texture_area->size.height  += (new_area.size.height  - draw_area->size.height)  * texture_scale.y;
        }
        
        *draw_area = new_area;
    }
    
    return true;
}

UV_Area rect_from_size(s16 x, s16 y, s16 width, s16 height) {
    UV_Area result = {
        cast_v(f32, x),
        cast_v(f32, y),
        cast_v(f32, width),
        cast_v(f32, height),
    };
    
    return result;
}

UV_Area rect_from_border(s16 left, s16 right, s16 bottom, s16 top) {
    UV_Area result = {
        cast_v(f32, left),
        cast_v(f32, bottom),
        cast_v(f32, right - left + 1),
        cast_v(f32, top - bottom + 1),
    };
    
    return result;
}

void ui_rect(UI_Context *context, UV_Area rect, UV_Area texture_rect = {}, rgba32 color = { 255, 255, 255, 255 }, bool is_filled = true, s16 thickness = 1)
{
    u32 vertex_offset = context->vertices.count;
    
    f32 offset;
    if (!is_filled && (thickness * context->scale <= 1.0f))
        offset = 0.5f;
    else
        offset = 0.0f;
    
    UI_Vertex vertex;
    vertex.color = color;
    
    // add vertices counter clock wise
    
    vec2f texel_size = { 1.0f / context->texture->resolution.width, 1.0f / context->texture->resolution.height };
    
    // bottom left
    vertex.uv = texture_rect.min * texel_size;
    vertex.position = transform_point(context->world_to_camera_transform, make_vec3(rect.min, context->depth) + vec3f { offset, offset, 0.0f });
    *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
    
    // bottom right
    vertex.uv = (texture_rect.min + vec2f{ texture_rect.size.width, 0 }) * texel_size;
    vertex.position = transform_point(context->world_to_camera_transform, make_vec3(rect.min + vec2f{ rect.size.width, 0 }, context->depth) + vec3f { -offset, offset, 0.0f });
    *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
    
    // top right
    vertex.uv = (texture_rect.min + texture_rect.size) * texel_size;
    vertex.position = transform_point(context->world_to_camera_transform, make_vec3(rect.min + rect.size, context->depth) + vec3f { -offset, -offset, 0.0f }); 
    *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
    
    // top left
    vertex.uv = (texture_rect.min + vec2f{ 0, texture_rect.size.height }) * texel_size;
    vertex.position = transform_point(context->world_to_camera_transform, make_vec3(rect.min + vec2f{ 0, rect.size.height }, context->depth) + vec3f { offset, -offset, 0.0f });
    *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
    
    if (is_filled) {
        
        // 3---2
        // |   |
        // |   |
        // 0---1
        
        _push_quad_as_triangles(context, vertex_offset, 0, 1, 2, 3);
    }
    else if (thickness * context->scale <= 1.0f) {
        // push as 1 pixel thin lines
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + 0;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + 1;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + 1;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + 2;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + 2;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + 3;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + 3;
        *grow(&context->index_memory.allocator, &context->indices) = vertex_offset + 0;
        
        auto command = grow(&context->command_memory.allocator, &context->commands);
        command->kind = UI_Command_Kind_Draw;
        command->draw.index_count = 8;
        command->draw.mode = GL_LINES;
    }
    else {
        rect.min.x += thickness;
        rect.min.y += thickness;
        rect.size.width  -= 2 * thickness;
        rect.size.height -= 2 * thickness;
        
        // bottom left border
        vertex.uv = texture_rect.min * texel_size;
        vertex.position = transform_point(context->world_to_camera_transform, make_vec3(rect.min, context->depth));
        *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
        
        // bottom right border
        vertex.uv = (texture_rect.min + vec2f{ texture_rect.size.width, 0 }) * texel_size;
        vertex.position = transform_point(context->world_to_camera_transform, make_vec3(rect.min + vec2f{ rect.size.width, 0 }, context->depth));
        *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
        
        // top right border
        vertex.uv = (texture_rect.min + texture_rect.size) * texel_size;
        vertex.position = transform_point(context->world_to_camera_transform, make_vec3(rect.min + rect.size, context->depth));
        *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
        
        // top left border
        vertex.uv = (texture_rect.min + vec2f{ 0, texture_rect.size.height }) * texel_size;
        vertex.position = transform_point(context->world_to_camera_transform, make_vec3(rect.min + vec2f{ 0, rect.size.height }, context->depth));
        *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
        
        // 3-------2
        // | 7---6 |
        // | |   | |
        // | |   | |
        // | 4---5 |
        // 0-------1
        
        // bottom border
        _push_quad_as_triangles(context, vertex_offset, 0, 1, 5, 4);
        // right border
        _push_quad_as_triangles(context, vertex_offset, 5, 1, 2, 6);
        // top border
        _push_quad_as_triangles(context, vertex_offset, 7, 6, 2, 3);
        // left border
        _push_quad_as_triangles(context, vertex_offset, 0, 4, 7, 3);
    }
}

struct UI_Text_Info {
    UV_Area text_area;
    u32 line_count;
    f32 start_x, start_y;
    f32 current_x;
    bool is_initialized;
};

UV_Area ui_text(UI_Context *context, UI_Text_Info *info, string text,bool do_render = true, rgba32 color = { 0, 0, 0, 255 })
{
    assert(info && info->is_initialized);
    assert(context->font_rendering.font);
    auto font = context->font_rendering.font;
    
    Texture *backup;
    if (do_render)
        backup = ui_texture(context, &font->texture);
    
    defer { if (do_render) ui_texture(context, backup); };
    
    bool text_area_is_initialized = false;
    UV_Area text_area;
    
    s16 line_y = info->start_y + (info->line_count - 1) * (context->font_rendering.line_spacing * context->font_rendering.scale) * context->font_rendering.line_grow_direction;
    
    for (auto it = text; it.count;) {
        u32 code = utf8_advance(&it);
        if (code == '\n') {
            ++info->line_count;
            
            line_y = info->start_y + (info->line_count - 1) * (context->font_rendering.line_spacing * context->font_rendering.scale) * context->font_rendering.line_grow_direction;
            
            info->current_x = info->start_x;
            
            continue;
        }
        
        Font_Glyph *glyph = get_font_glyph(font, code);
        if (!glyph)
            continue;
        
        UV_Area draw_area = {
            cast_v(f32, info->current_x + glyph->draw_x_offset * context->font_rendering.scale),
            cast_v(f32, line_y + glyph->draw_y_offset * context->font_rendering.scale),
            cast_v(f32, glyph->rect.width) * context->font_rendering.scale,
            cast_v(f32, glyph->rect.height) * context->font_rendering.scale
        };
        
        UV_Area texture_area = { 
            cast_v(f32, glyph->rect.x), 
            cast_v(f32, glyph->rect.y), 
            cast_v(f32, glyph->rect.width),
            cast_v(f32, glyph->rect.height)
        };
        
        bool do_merge = ui_clip(context, &draw_area, &texture_area);
        if (do_merge) {
            if (!text_area_is_initialized) {
                text_area = draw_area;
                text_area_is_initialized = true;
            }
            else {
                text_area = merge(text_area, draw_area);
            }
            
            if (do_render)
                ui_rect(context, draw_area, texture_area, color);
        }
        
        info->current_x += glyph->draw_x_advance * context->font_rendering.scale;
    }
    
    info->text_area = merge(info->text_area, text_area);
    
    return text_area;
}

UI_Text_Info ui_text(UI_Context *context, s16 x, s16 y, string text,bool do_render = true, rgba32 color = { 0, 0, 0, 255 })
{
    assert(context->font_rendering.font);
    auto font = context->font_rendering.font;
    
    Texture *backup;
    if (do_render)
        backup = ui_texture(context, &font->texture);
    
    defer { if (do_render) ui_texture(context, backup); };
    
    UI_Text_Info info;
    info.line_count = 1;
    info.start_x = x;
    info.start_y = y;
    info.current_x = x;
    info.is_initialized = true;
    
    // because ui_text will merge text_areas but first text_area is empty
    info.text_area = ui_text(context, &info, text, do_render, color);
    
    return info;
}


UI_Text_Info ui_text(UI_Context *context, UV_Area area, vec2f alignment, string text, rgba32 color = { 0, 0, 0, 255 })
{
    auto info = ui_text(context, 0, 0, text, false);
    
    return ui_text(context, area.min.x - info.text_area.min.x + (area.size.x - info.text_area.size.x) * alignment.x, area.min.y - info.text_area.min.y + (area.size.y - info.text_area.size.y) * alignment.y, text, true, color);
}


UV_Area ui_write_va(UI_Context *context, UI_Text_Info *info, string format, va_list params)
{
    bool is_escaping = false;
    
    UV_Area result;
    bool area_is_init = false;
    
    while (format.count) {
        u8 _buffer[1024];
        u8_buffer write_buffer = ARRAY_INFO(_buffer);
        write(&write_buffer, &format, params, &is_escaping);
        
        string text;
        text.data = write_buffer.data;
        text.count = write_buffer.count;
        UV_Area area = ui_text(context, info, text, context->font_rendering.do_render, context->font_rendering.color);
        
        if (!area_is_init) {
            result = area;
            area_is_init = true;
        }
        else {
            result = merge(result, area);
        }
    }
    
    return result;
}

inline UV_Area ui_write(UI_Context *context, s16 x, s16 y, string format, ...) {
    va_list params;
    va_start(params, format);
    
    auto info = ui_text(context, x, y, {}, false);
    UV_Area result = ui_write_va(context, &info, format, params);
    va_end(params);
    
    return result;
}

inline UV_Area ui_write(UI_Context *context, s16 x, s16 y, rgba32 color, string format, ...) {
    va_list params;
    va_start(params, format);
    SCOPE_PUSH(context->font_rendering.color, color);
    
    auto info = ui_text(context, x, y, {}, false);
    UV_Area result = ui_write_va(context, &info, format, params);
    va_end(params);
    
    return result;
}

inline UV_Area ui_write(UI_Context *context, UI_Text_Info *info, string format, ...) {
    va_list params;
    va_start(params, format);
    UV_Area result = ui_write_va(context, info, format, params);
    va_end(params);
    
    return result;
}

inline UV_Area ui_write(UI_Context *context, UI_Text_Info *info, rgba32 color, string format, ...) {
    va_list params;
    va_start(params, format);
    SCOPE_PUSH(context->font_rendering.color, color);
    UV_Area result = ui_write_va(context, info, format, params);
    va_end(params);
    
    return result;
}

INTERNAL bool ui_control(UI_Context *context, UV_Area cursor_area, bool cursor_was_pressed, bool cursor_was_released, bool force_update = true)
{
    auto control = &context->control;
    
    bool needs_update = force_update || cursor_was_pressed || cursor_was_released || !ARE_EQUAL(&cursor_area, &control->cursor_area, sizeof(cursor_area));
    
    if (needs_update) {
        control->hot_id = control->next_hot_id;
        control->next_hot_id = -1;
        control->triggerd_id = -1;
        control->cursor_area = cursor_area;
        control->cursor_was_pressed  = cursor_was_pressed;
        control->cursor_was_released = cursor_was_released;
        control->current_id = 0;
        
        if ((control->hot_id != -1) && cursor_was_pressed)
            control->active_id = control->hot_id;
        else if (cursor_was_released) {
            if (control->hot_id == control->active_id) {
                control->triggerd_id = control->active_id;
            }
            
            control->active_id = -1;
        }
    }
    
    return needs_update;
}

INTERNAL UI_Control_State ui_button(UI_Context *context, UV_Area button_area, f32 priority = 0.0f, bool is_enabled = true) {
    auto control = &context->control;
    
    if (((control->next_hot_id == -1) || (control->next_hot_priority > priority)) && intersects(button_area, control->cursor_area)) {
        control->next_hot_priority = priority;
        control->next_hot_id = control->current_id;
    }
    
    defer { control->current_id++; };
    
    if (!is_enabled)
        return UI_Control_State_Disabled;
    
    else if (control->triggerd_id == control->current_id) {
        return UI_Control_State_Triggered;
    }
    
    else if (control->active_id == control->current_id)
        return UI_Control_State_Active;
    
    else if ((control->active_id == -1) && (control->hot_id == control->current_id))
        return UI_Control_State_Hot;
    
    return UI_Control_State_Idle;
}

#endif // UI_H