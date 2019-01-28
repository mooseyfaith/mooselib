#if !defined UI_H
#define UI_H

#include "render_queue.h"
#include "font.h"

#pragma pack(push, 1)
struct UI_Vertex {
    vec3f  position;
    rgba32 color;
    vec2f  uv;
};
#pragma pack(pop)


#define Template_Array_Name      UI_Vertex_Array
#define Template_Array_Data_Type UI_Vertex
#include "template_array.h"

enum UI_Control_State {
    UI_Control_State_Disabled,
    UI_Control_State_Idle,
    UI_Control_State_Hot,
    UI_Control_State_Active,
    UI_Control_State_Triggered
};

struct UI_Context {
    Render_Queue render_queue;
    Memory_Allocator *transient_allocator;
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
    
    usize last_texture_command_offset;
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

UI_Context make_ui_context(Memory_Allocator *allocator) {
    UI_Context context = {};
    
    Vertex_Attribute_Info attribute_infos[] = {
        { Vertex_Position_Index, 3, GL_FLOAT, GL_FALSE },
        { Vertex_Color_Index,    4, GL_UNSIGNED_BYTE, GL_TRUE },
        { Vertex_UV_Index,       2, GL_FLOAT, GL_FALSE },
    };
    context.render_queue = make_render_queue(allocator, ARRAY_WITH_COUNT(attribute_infos));
    
    context.world_to_camera_transform = MAT4X3_IDENTITY;
    context.depth = 0.0f;
    
    context.font_rendering.color = rgba32{ 0, 0, 0, 255 };
    context.font_rendering.font = null;
    context.font_rendering.alignment = { -1.0f, -1.0f }; // align left, bottom
    context.font_rendering.line_grow_direction = -1.0f;
    context.font_rendering.scale = 1.0f;
    context.font_rendering.do_render = true;
    
    auto control = &context.control;
    
    control->hot_id = -1;
    control->active_id = -1;
    control->next_hot_id = -1;
    
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

vec2f _uv_from_position(UI_Context *context, s16 origin_x, s16 origin_y, s16 x, s16 y)
{
    assert(context->texture);
    
    return { cast_v(f32, x - origin_x) / context->texture->resolution.width, cast_v(f32, y - origin_y) / context->texture->resolution.height };
    
}

Texture * ui_set_texture(UI_Context *context, Texture *texture)
{
    Render_Queue_Command *last_texture_command;
    Texture *old_texture;
    if (context->last_texture_command_offset == -1) {
        last_texture_command = null;
        old_texture = null;
    }
    else {
        last_texture_command = cast_p(Render_Queue_Command, context->render_queue.data + context->last_texture_command_offset);
        old_texture = last_texture_command->bind_texture.texture;
    }
    
    if (!old_texture || ((old_texture != texture) && (context->last_texture_command_offset != context->render_queue.data.count - sizeof(Render_Queue_Command))))
    {
        auto command = grow_item(context->render_queue.allocator, &context->render_queue.data, Render_Queue_Command);
        command->kind = Render_Queue_Command_Kind_Bind_Texture;
        command->bind_texture.texture = texture;
        command->bind_texture.index   = 0;
        
        context->last_texture_command_offset = index_of(context->render_queue.data, cast_p(u8, command));
    }
    else if (last_texture_command) {
        last_texture_command->bind_texture.texture = texture;
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

void ui_set_vertex(UI_Context 
                   *context, UI_Vertex *vertex, f32 x, f32 y, f32 u, f32 v, rgba32 color)
{
    vertex->position = transform_point(context->world_to_camera_transform, make_vec3(x, y, context->depth));
    vertex->color = color;
    vertex->uv = { u, v };
}

void ui_set_quad_vertices(UI_Context *context, UI_Vertex vertices[4], area2f draw_rect, area2f texture_rect, rgba32 color, f32 offset = 0.0f)
{
    vec2f texel_size = { 1.0f / context->texture->resolution.width, 1.0f / context->texture->resolution.height };
    
    // 3---2
    // |   |
    // |   |
    // 0---1
    
    // add vertices counter clock wise
    
    ui_set_vertex(context, vertices + 0,
                  draw_rect.x + offset,
                  draw_rect.y + offset,
                  texture_rect.x * texel_size.x,
                  texture_rect.y * texel_size.y,
                  color);
    
    ui_set_vertex(context, vertices + 1,
                  draw_rect.x + draw_rect.width - offset,
                  draw_rect.y + offset,
                  (texture_rect.x + texture_rect.width) * texel_size.x,
                  texture_rect.y * texel_size.y,
                  color);
    
    ui_set_vertex(context, vertices + 2,
                  draw_rect.x + draw_rect.width - offset,
                  draw_rect.y + draw_rect.height - offset,
                  (texture_rect.x + texture_rect.width) * texel_size.x,
                  (texture_rect.y + texture_rect.height) * texel_size.y,
                  color);
    
    ui_set_vertex(context, vertices + 3,
                  draw_rect.x + offset,
                  draw_rect.y + draw_rect.height - offset,
                  texture_rect.x * texel_size.x,
                  (texture_rect.y + texture_rect.height) * texel_size.y,
                  color);
}

void ui_set_quad_triangles_indices(u32 indices[4], u32 a, u32 b, u32 c, u32 d)
{
    indices[0] = a;
    indices[1] = b;
    indices[2] = c;
    
    indices[3] = a;
    indices[4] = c;
    indices[5] = d;
}

void ui_rect(UI_Context *context, area2f draw_rect, area2f texture_rect = {}, rgba32 color = { 255, 255, 255, 255 }, bool is_filled = true)
{
    s16 thickness = context->border_thickness;
    
    f32 offset;
    if (!is_filled && (thickness <= 1.0f))
        offset = 0.5f;
    else
        offset = 0.0f;
    
    auto command = grow_item(context->render_queue.allocator, &context->render_queue.data, Render_Queue_Command);
    command->kind = Render_Queue_Command_Kind_Draw;
    
    if (is_filled)
    {
        command->draw.mode = GL_TRIANGLES;
        command->draw.vertex_count = 4;
        command->draw.index_count = 6;
        
        // grow_items might invalidad comand, so we copy it
        Render_Queue_Command command_backup = *command;
        
        auto vertices = grow_items(context->render_queue.allocator, &context->render_queue.data, UI_Vertex, command_backup.draw.vertex_count);
        ui_set_quad_vertices(context, vertices, draw_rect, texture_rect, color, offset);
        
        // 3--2   l - counter clockwise (left)
        // |l/|
        // |/l|
        // 0--1
        
        auto indices =grow_items(context->render_queue.allocator, &context->render_queue.data, u32, command_backup.draw.index_count);
        
        ui_set_quad_triangles_indices(indices, 0, 1, 2, 3);
    }
    else if (thickness <= 1.0f)
    {
        command->draw.mode = GL_LINES;
        command->draw.vertex_count = 4;
        command->draw.index_count = 8;
        
        // grow_items might invalidad comand, so we copy it
        Render_Queue_Command command_backup = *command;
        
        auto vertices = grow_items(context->render_queue.allocator, &context->render_queue.data, UI_Vertex, command_backup.draw.vertex_count);
        ui_set_quad_vertices(context, vertices, draw_rect, texture_rect, color, offset);
        
        auto indices =grow_items(context->render_queue.allocator, &context->render_queue.data, u32, command_backup.draw.index_count);
        
        indices[0] = 0;
        indices[1] = 1;
        indices[2] = 1;
        indices[3] = 2;
        indices[4] = 2;
        indices[5] = 3;
        indices[6] = 3;
        indices[7] = 0;
    }
    else
    {
        command->draw.mode = GL_LINES;
        command->draw.vertex_count = 8;
        command->draw.index_count = 24;
        
        // grow_items might invalidad comand, so we copy it
        Render_Queue_Command command_backup = *command;
        
        auto vertices = grow_items(context->render_queue.allocator, &context->render_queue.data, UI_Vertex, command_backup.draw.vertex_count);
        ui_set_quad_vertices(context, vertices, draw_rect, texture_rect, color, offset);
        
#if 1        
        draw_rect.x += thickness;
        draw_rect.y += thickness;
        draw_rect.width  -= 2 * thickness;
        draw_rect.height -= 2 * thickness;
#else
        draw_rect.x += thickness;
        draw_rect.y += thickness;
        draw_rect.width  -= 2 * thickness; 
        draw_rect.height -= 2 * thickness;
#endif
        ui_set_quad_vertices(context, vertices + 4, draw_rect, texture_rect, color, offset);
        
        // 3-------2
        // | 7---6 |
        // | |   | |
        // | |   | |
        // | 4---5 |
        // 0-------1
        
        auto indices = grow_items(context->render_queue.allocator, &context->render_queue.data, u32, command_backup.draw.index_count);
        
        // bottom border
        ui_set_quad_triangles_indices(indices + 0, 0, 1, 5, 4);
        // right border
        ui_set_quad_triangles_indices(indices + 6, 5, 1, 2, 6);
        // top border
        ui_set_quad_triangles_indices(indices + 12, 7, 6, 2, 3);
        // left border
        ui_set_quad_triangles_indices(indices + 18, 0, 4, 7, 3);
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
    auto text = write_va(context->transient_allocator, format, params);
    defer { free(context->transient_allocator, text); };
    
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

void ui_clear(UI_Context *context) {
    clear(&context->render_queue);
    
    context->last_texture_command_offset = -1;
    context->texture = null;
}


#endif // UI_H