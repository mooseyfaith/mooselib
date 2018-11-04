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
        
        Texture *texture;
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
    
    Memory_Allocator *temporary_allocator;
    
    UI_Vertex_Array vertices;
    u16_array indices;
    UI_Command_Array commands;
    
    union { mat4x3f world_to_camera_transform, transform; };
    f32 depth;
    
    bool use_sissor_area;
    area2f sissor_area;
    
    struct {
        s16 left, right, bottom, top;
        s16 center_x, center_y;
        s16 width, height;
    };
    
    s16 border_thickness;
    
    u32 last_texture_command_index;
    Texture *texture;
    
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
        //u32 triggerd_id;
        u32 current_id;
        
        area2f cursor_area;
        bool cursor_was_pressed;
        bool cursor_was_released;
        
        u32 next_hot_id;
        f32 next_hot_priority;
    } control;
};

UI_Context make_ui_context(Memory_Allocator *internal_allocator) {
    UI_Context context = {};
    context.vertex_memory  = make_memory_growing_stack(internal_allocator);
    context.index_memory   = make_memory_growing_stack(internal_allocator);
    context.command_memory = make_memory_growing_stack(internal_allocator);
    context.vertices = {};
    context.indices  = {};
    context.commands = {};
    
    context.world_to_camera_transform = MAT4X3_IDENTITY;
    context.depth = 0.0f;
    
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
    //control->triggerd_id = -1;
    
    return context;
}

void ui_set_transform(UI_Context *context, Pixel_Dimensions resolution, f32 depth_scale = 1.0f, f32 depth_alignment = 0.0f)
{
    assert((depth_scale > 0.0f) && (depth_scale <= 1.0f));
    assert((depth_alignment >= 0.0f) && (depth_alignment <= 1.0f));
    
    f32 y_up_direction = 1.0f;
    
    // for now using y_up_direction == -1 dows not seem to work properly, it will reverse face drawing order and will be culled if glEnable(GL_CULL_FACE), wich is the default
    assert(y_up_direction == 1.0f);
    
    context->width  = resolution.width;
    context->height = resolution.height;
    
    context->transform.columns[0] = {  2.0f / resolution.width, 0,                      0           };
    context->transform.columns[1] = {  0,                     2.0f / resolution.height, 0           };
    context->transform.columns[2] = {  0,                     0,                      depth_scale };
    
    context->transform.columns[3] = { -1, -1, depth_alignment * (1.0f - depth_scale) };
    
    //context->y_up_direction = y_up_direction;
    
    context->left   = 0;
    context->right  = context->width;
    context->top    = (context->height) * interval_zero_to_one(y_up_direction);
    context->bottom = (context->height) - context->top;
    
    context->center_x = (context->left + context->right)  * 0.5f;
    context->center_y = (context->top  + context->bottom) * 0.5f;
}

// scale == 0 => pixel perfect font without scaling
void ui_set_font(UI_Context *context, Font *font, f32 scale = 1.0f) {
    context->font_rendering.font = font;
    
    context->font_rendering.scale = scale;
    
    context->font_rendering.line_spacing = (font->pixel_height + 1) * context->font_rendering.scale + 0.5f;
}

void ui_flush(UI_Context *context) {
    glBindBuffer(GL_ARRAY_BUFFER, context->vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, byte_count(context->vertices), context->vertices.data, GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glBindVertexArray(context->vertex_array_object);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->index_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, byte_count(context->indices), context->indices.data, GL_STREAM_DRAW);
}

void ui_draw(UI_Context *context)
{
    usize offset = 0;
    
    glActiveTexture(GL_TEXTURE0 + 0);
    
    Texture *current_texture = null;
    
    for (auto it = context->commands + 0, end = it + context->commands.count; it != end; ++it)
    {
        switch (it->kind) {
            case UI_Command_Kind_Draw: {
                glDrawElements(it->draw.mode, it->draw.index_count, GL_UNSIGNED_SHORT, cast_p(GLvoid, offset));
                offset += it->draw.index_count * sizeof(u16);
            } break;
            
            case UI_Command_Kind_Texture: {
                if (current_texture != it->texture) {
                    current_texture = it->texture;
                    glBindTexture(GL_TEXTURE_2D, it->texture->object);
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
    
    context->last_texture_command_index = -1;
    context->texture = null;
}

void _push_quad_as_triangles(UI_Context *context, u32 vertex_offset, u32 a, u32 b, u32 c, u32 d, bool only_edge = false)
{
    
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

vec2f _uv_from_position(UI_Context *context, s16 origin_x, s16 origin_y, s16 x, s16 y)
{
    assert(context->texture);
    
    return { cast_v(f32, x - origin_x) / context->texture->resolution.width, cast_v(f32, y - origin_y) / context->texture->resolution.height };
    
}

Texture * ui_set_texture(UI_Context *context, Texture *texture)
{
    UI_Command *last_texture_command;
    Texture *old_texture;
    if (context->last_texture_command_index == -1) {
        last_texture_command = null;
        old_texture = null;
    }
    else {
        last_texture_command = context->commands + context->last_texture_command_index;
        old_texture = last_texture_command->texture;
    }
    
    if (!old_texture || ((old_texture != texture) && (context->last_texture_command_index != context->commands.count - 1)))
    {
        auto command = grow(&context->command_memory.allocator, &context->commands);
        command->kind = UI_Command_Kind_Texture;
        command->texture = texture;
        
        context->last_texture_command_index = index_of(context->commands, command);
    }
    else if (last_texture_command) {
        last_texture_command->texture = texture;
    }
    
    context->texture = texture;
    
    return old_texture;
}

bool ui_clip(UI_Context *context, area2f *draw_area, area2f *texture_area = null) {
    if (context->use_sissor_area) {
        vec2f texture_scale;
        
        if (texture_area)
            texture_scale = { texture_area->width / draw_area->width, texture_area->height / draw_area->height };
        
        bool is_valid;
        area2f new_area = intersection(&is_valid, *draw_area, context->sissor_area);
        
        if (!is_valid)
            return false;
        
        if (texture_area) {
            texture_area->min.x += (new_area.x - draw_area->min.x) * texture_scale.x;
            texture_area->min.y += (new_area.y - draw_area->min.y) * texture_scale.y;
            texture_area->width   += (new_area.width   - draw_area->width)   * texture_scale.x;
            texture_area->height  += (new_area.height  - draw_area->height)  * texture_scale.y;
        }
        
        *draw_area = new_area;
    }
    
    return true;
}

INTERNAL area2f rect(s16 x, s16 y, s16 width, s16 height)
{
    area2f result;
    result.x = x;
    result.y = y;
    result.width = width;
    result.height = height;
    
    return result;
}

INTERNAL area2f border(s16 left, s16 bottom, s16 right, s16 top, s16 margin = 0)
{
    area2f result;
    result.x = left   + margin;
    result.y = bottom + margin;
    result.width  = right - left - 2 * margin;
    result.height = top - bottom - 2 * margin;
    
    return result;
}

void ui_vertex(UI_Context *context, f32 x, f32 y, f32 u, f32 v, rgba32 color)
{
    UI_Vertex vertex;
    vertex.position = transform_point(context->world_to_camera_transform, make_vec3(x, y, context->depth));
    vertex.color = color;
    vertex.uv = { u, v };
    
    *grow(&context->vertex_memory.allocator, &context->vertices) = vertex;
}

void ui_quad(UI_Context *context, area2f draw_rect, area2f texture_rect, rgba32 color, f32 offset = 0.0f)
{
    vec2f texel_size = { 1.0f / context->texture->resolution.width, 1.0f / context->texture->resolution.height };
    
    // 3---2
    // |   |
    // |   |
    // 0---1
    
    // add vertices counter clock wise
    
    ui_vertex(context,
              draw_rect.x + offset,
              draw_rect.y + offset,
              texture_rect.x * texel_size.x,
              texture_rect.y * texel_size.y,
              color);
    
    ui_vertex(context,
              draw_rect.x + draw_rect.width - offset,
              draw_rect.y + offset,
              (texture_rect.x + texture_rect.width) * texel_size.x,
              texture_rect.y * texel_size.y,
              color);
    
    ui_vertex(context,
              draw_rect.x + draw_rect.width - offset,
              draw_rect.y + draw_rect.height - offset,
              (texture_rect.x + texture_rect.width) * texel_size.x,
              (texture_rect.y + texture_rect.height) * texel_size.y,
              color);
    
    ui_vertex(context,
              draw_rect.x + offset,
              draw_rect.y + draw_rect.height - offset,
              texture_rect.x * texel_size.x,
              (texture_rect.y + texture_rect.height) * texel_size.y,
              color);
}

void ui_rect(UI_Context *context, area2f draw_rect, area2f texture_rect = {}, rgba32 color = { 255, 255, 255, 255 }, bool is_filled = true)
{
    s16 thickness = context->border_thickness;
    
    u32 vertex_offset = context->vertices.count;
    
    f32 offset;
    if (!is_filled && (thickness <= 1.0f))
        offset = 0.5f;
    else
        offset = 0.0f;
    
#if 0    
    // scale is only applyed to offset, not to size
    draw_rect.x = draw_rect.x * context->scale;
    draw_rect.y = draw_rect.y * context->scale;
#endif
    
    ui_quad(context, draw_rect, texture_rect, color, offset);
    
    if (is_filled) {
        
        // 3---2
        // |   |
        // |   |
        // 0---1
        
        _push_quad_as_triangles(context, vertex_offset, 0, 1, 2, 3);
    }
    else if (thickness <= 1.0f) {
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
        
#if 1        
        draw_rect.x += thickness;
        draw_rect.y += thickness;
        draw_rect.width  -= 2 *thickness;
        draw_rect.height -= 2 *thickness;
#else
        draw_rect.x += thickness;
        draw_rect.y += thickness;
        draw_rect.width  -= 2 *thickness;
        draw_rect.height -= 2 *thickness;
#endif
        
        ui_quad(context, draw_rect, texture_rect, color);
        
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
    area2f text_area;
    u32 line_count;
    f32 start_x, start_y;
    f32 current_x;
    bool is_initialized;
};

area2f ui_text(UI_Context *context, UI_Text_Info *info, string text, bool do_render = true, rgba32 color = { 0, 0, 0, 255 })
{
    assert(info && info->is_initialized);
    assert(context->font_rendering.font);
    auto font = context->font_rendering.font;
    
    Texture *backup;
    if (do_render)
        backup = ui_set_texture(context, &font->texture);
    
    defer { if (do_render) ui_set_texture(context, backup); };
    
    bool text_area_is_initialized = false;
    area2f text_area;
    
    s16 line_y = info->start_y + (info->line_count - 1) * (context->font_rendering.line_spacing) * context->font_rendering.line_grow_direction;
    
    for (auto it = text; it.count;) {
        u32 code = utf8_advance(&it);
        if (code == '\n') {
            ++info->line_count;
            
            line_y = info->start_y + (info->line_count - 1) * (context->font_rendering.line_spacing) * context->font_rendering.line_grow_direction;
            
            info->current_x = info->start_x;
            
            continue;
        }
        
        Font_Glyph *glyph = get_font_glyph(font, code);
        if (!glyph)
            continue;
        
        area2f draw_area = {
            cast_v(f32, info->current_x + glyph->draw_x_offset * context->font_rendering.scale),
            cast_v(f32, line_y + glyph->draw_y_offset * context->font_rendering.scale),
            cast_v(f32, glyph->rect.width) * context->font_rendering.scale,
            cast_v(f32, glyph->rect.height) * context->font_rendering.scale
        };
        
        draw_area.x = info->current_x + glyph->draw_x_offset * context->font_rendering.scale;
        draw_area.y = line_y + glyph->draw_y_offset * context->font_rendering.scale;
        draw_area.width = glyph->rect.width * context->font_rendering.scale;
        draw_area.height = glyph->rect.height * context->font_rendering.scale;
        
        area2f texture_area = { 
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
    
    if (text_area_is_initialized)
        info->text_area = merge(info->text_area, text_area);
    else
        info->text_area = {};
    
    return text_area;
}

UI_Text_Info ui_text(UI_Context *context, s16 x, s16 y, string text,bool do_render = true, rgba32 color = { 0, 0, 0, 255 })
{
    assert(context->font_rendering.font);
    auto font = context->font_rendering.font;
    
    Texture *backup;
    if (do_render)
        backup = ui_set_texture(context, &font->texture);
    
    defer { if (do_render) ui_set_texture(context, backup); };
    
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


UI_Text_Info ui_text(UI_Context *context, area2f area, vec2f alignment, string text, rgba32 color = { 0, 0, 0, 255 })
{
    auto info = ui_text(context, 0, 0, text, false);
    
    return ui_text(context, area.x - info.text_area.x + (area.width - info.text_area.width) * alignment.x, area.y - info.text_area.y + (area.height - info.text_area.height) * alignment.y, text, true, color);
}


area2f ui_write_va(UI_Context *context, UI_Text_Info *info, string format, va_list params)
{
    u8_buffer buffer = {};
    auto text = write_va(context->temporary_allocator, &buffer, format, params);
    defer { free(context->temporary_allocator, &buffer); };
    
    area2f result = ui_text(context, info, text, context->font_rendering.do_render, context->font_rendering.color);
    
    return result;
}

inline area2f ui_write(UI_Context *context, s16 x, s16 y, string format, ...) {
    va_list params;
    va_start(params, format);
    
    auto info = ui_text(context, x, y, {}, false);
    area2f result = ui_write_va(context, &info, format, params);
    va_end(params);
    
    return result;
}

inline area2f ui_write(UI_Context *context, s16 x, s16 y, rgba32 color, string format, ...) {
    va_list params;
    va_start(params, format);
    SCOPE_PUSH(context->font_rendering.color, color);
    
    auto info = ui_text(context, x, y, {}, false);
    area2f result = ui_write_va(context, &info, format, params);
    va_end(params);
    
    return result;
}

inline area2f ui_write(UI_Context *context, UI_Text_Info *info, string format, ...) {
    va_list params;
    va_start(params, format);
    area2f result = ui_write_va(context, info, format, params);
    va_end(params);
    
    return result;
}

inline area2f ui_write(UI_Context *context, UI_Text_Info *info, rgba32 color, string format, ...) {
    va_list params;
    va_start(params, format);
    SCOPE_PUSH(context->font_rendering.color, color);
    area2f result = ui_write_va(context, info, format, params);
    va_end(params);
    
    return result;
}

INTERNAL bool ui_control(UI_Context *context, area2f cursor_area, bool cursor_was_pressed, bool cursor_was_released, bool force_update = true)
{
    auto control = &context->control;
    
    if (control->cursor_was_released)
        control->active_id = -1;
    
    bool needs_update = force_update || cursor_was_pressed || cursor_was_released || !ARE_EQUAL(&cursor_area, &control->cursor_area, sizeof(cursor_area));
    
    if (needs_update)
    {
        control->hot_id = control->next_hot_id;
        control->next_hot_id = -1;
        //control->triggerd_id = -1;
        control->cursor_area = cursor_area;
        control->cursor_was_pressed  = cursor_was_pressed;
        control->cursor_was_released = cursor_was_released;
        control->current_id = -1;
        
        if ((control->hot_id != -1) && cursor_was_pressed)
            control->active_id = control->hot_id;
        
#if 0
        else if (cursor_was_released)
        {
            if (control->hot_id == control->active_id)
                control->triggerd_id = control->active_id;
            
            control->active_id = -1;
        }
#endif
    }
    
    return needs_update;
}

INTERNAL UI_Control_State ui_button(UI_Context *context, area2f button_area, f32 priority = 0.0f, bool is_enabled = true)
{
    auto control = &context->control;
    control->current_id++;
    
    if (((control->next_hot_id == -1) || (control->next_hot_priority > priority)) && intersects(button_area, control->cursor_area)) {
        control->next_hot_priority = priority;
        control->next_hot_id = control->current_id;
    }
    
    if (!is_enabled)
        return UI_Control_State_Disabled;
    
    //else if (control->triggerd_id == control->current_id)
    
    else if (control->active_id == control->current_id)
    {
        if (control->cursor_was_released)
        {
            if (control->active_id == control->hot_id)
                return UI_Control_State_Triggered;
            else
                return UI_Control_State_Idle;
        }
        
        return UI_Control_State_Active;
    }
    
    else if ((control->active_id == -1) && (control->hot_id == control->current_id))
        return UI_Control_State_Hot;
    
    return UI_Control_State_Idle;
}

INTERNAL UI_Control_State ui_selectable(UI_Context *context, area2f area, u32 selection_id, bool *is_selected, bool invert_selection = false)
{
    auto control = &context->control;
    control->current_id++;
    
    bool next_is_selected;
    if (invert_selection)
        next_is_selected = *is_selected;
    else
        next_is_selected = false;
    
    if (intersects(area, control->cursor_area))
    {
        if ((selection_id != -1) && (selection_id == control->active_id))
        {
            next_is_selected = !next_is_selected;
            
            if (control->cursor_was_released)
            {
                *is_selected = next_is_selected;
                
                return UI_Control_State_Triggered;
            }
            
            if (!next_is_selected)
                return UI_Control_State_Idle;
            
            return UI_Control_State_Active;
        }
        
        if (!next_is_selected)
            return UI_Control_State_Idle;
        
        return UI_Control_State_Hot;
    }
    
    return UI_Control_State_Idle;
}

#endif // UI_H