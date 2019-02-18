#if !defined YAUI_H
#define YAUI_H

#include "memory_growing_stack.h"
#include "render_batch.h"
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

struct UI_Command {
    enum Kind {
        Kind_draw,
        Kind_Count,
    } kind;
    
    union {
        struct {
            GLenum mode;
            UI_Vertex_Array vertices;
            u32_array       indices;
        } draw;
    };
};

#define Template_List_Name      UI_Command_List
#define Template_List_Data_Type UI_Command
#define Template_List_Data_Name command
#define Template_List_With_Tail
#include "template_list.h"

struct UI_Group {
    Texture *texture;
    
    UI_Command_List commands;
};

#define Template_List_Name      UI_Group_List
#define Template_List_Data_Type UI_Group
#define Template_List_Data_Name group
#define Template_List_With_Tail
#include "template_list.h"

struct UI_Layout {
    
};

struct UI_Context {
    GLuint vertex_array_object;
    union {
        struct { GLuint vertex_buffer_object, index_buffer_object; };
        GLuint buffer_objects[2];
    };
    
    Memory_Growing_Stack memory_stack;
    Memory_Allocator *transient_allocator; // for text formatting and rendering
    
    UI_Group_List groups;
    UI_Group *current_group;
    
    // area of all successive draws
    area2f current_area;
    
    union { mat4x3f world_to_camera_transform, transform; };
    f32 depth;
    
    area2f sissor_area;
    
    struct {
        s16 left, right, bottom, top;
        s16 center_x, center_y;
        s16 width, height;
    };
    
    s16 border_thickness;
    
    usize last_texture_command_offset;
    //Texture *texture;
    
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
    
#if 0
    u32 layout_count;
    UI_Layout *current_layout;
    
    struct {
        u32 command_byte_index;
    } current_item;
    
    area2f current_item_area;
    u32 render_queue_byte_index;
    bool has_cashed_commands;
#endif
    
};

Texture * ui_set_texture(UI_Context *context, Texture *texture) {
    assert(texture);
    
    UI_Group *group = null;
    for_list_item(it, context->groups) {
        if (it->group.texture == texture) {
            group = &it->group;
            break;
        }
    }
    
    if (!group) {
        group = insert_tail(&context->memory_stack.allocator, &context->groups);
        *group = {};
        group->texture = texture;
    }
    
    Texture *old_texture;
    if (context->current_group)
        old_texture = context->current_group->texture;
    else
        old_texture = null;
    
    context->current_group = group;
    
    return old_texture;
}


UI_Context make_ui_context(Memory_Allocator *internal_allocator) {
    UI_Context context = {};
    
    Vertex_Attribute_Info attribute_infos[] = {
        { Vertex_Position_Index, 3, GL_FLOAT, GL_FALSE },
        { Vertex_Color_Index,    4, GL_UNSIGNED_BYTE, GL_TRUE },
        { Vertex_UV_Index,       2, GL_FLOAT, GL_FALSE },
    };
    
    glGenVertexArrays(1, &context.vertex_array_object);
    glGenBuffers(COUNT_WITH_ARRAY(context.buffer_objects));
    
    glBindVertexArray(context.vertex_array_object);
    
    set_vertex_attributes(context.vertex_buffer_object, ARRAY_WITH_COUNT(attribute_infos));
    assert(get_vertex_byte_count( ARRAY_WITH_COUNT(attribute_infos)) == sizeof(UI_Vertex));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    //context.render_queue = make_render_queue(allocator, ARRAY_WITH_COUNT(attribute_infos));
    context.memory_stack = make_memory_growing_stack(internal_allocator);
    
    context.world_to_camera_transform = MAT4X3_IDENTITY;
    context.depth = 0.0f;
    
    context.font_rendering.color = rgba32{ 0, 0, 0, 255 };
    context.font_rendering.font = null;
    context.font_rendering.alignment = { -1.0f, -1.0f }; // align left, bottom
    context.font_rendering.line_grow_direction = -1.0f;
    context.font_rendering.scale = 1.0f;
    context.font_rendering.do_render = true;
    
    context.last_texture_command_offset = -1;
    //context.texture = null;
    //context.has_cashed_commands = false;
    //context.render_queue_byte_index = 0;
    
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
    assert(context->current_group && context->current_group->texture);
    auto texture = context->current_group->texture;
    
    return { cast_v(f32, x - origin_x) / texture->resolution.width, cast_v(f32, y - origin_y) / texture->resolution.height };
    
}

#if 0
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
        
        auto command = ui_item(context, Render_Queue_Command);
        *command = make_kind(Render_Queue_Command, bind_texture, texture, 0);
        
        context->last_texture_command_offset = index_of(context->render_queue.data, cast_p(u8, command));
    }
    else if (last_texture_command) {
        last_texture_command->bind_texture.texture = texture;
    }
    
    context->texture = texture;
    
    return old_texture;
}
#endif

bool ui_clip(UI_Context *context, area2f *draw_area, area2f *texture_area = null) {
    if (context->sissor_area.is_valid) {
        vec2f texture_scale;
        
        if (texture_area)
            texture_scale = { texture_area->width / draw_area->width, texture_area->height / draw_area->height };
        
        area2f new_area = intersection(*draw_area, context->sissor_area);
        
        if (!new_area.is_valid)
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
    result.is_valid = true;
    
    return result;
}

INTERNAL area2f border(s16 left, s16 bottom, s16 right, s16 top, s16 margin = 0)
{
    area2f result;
    result.x = left   + margin;
    result.y = bottom + margin;
    result.width  = right - left - 2 * margin;
    result.height = top - bottom - 2 * margin;
    result.is_valid = true;
    
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
    assert(context->current_group && context->current_group->texture);
    auto texture = context->current_group->texture;
    
    vec2f texel_size = { 1.0f / texture->resolution.width, 1.0f / texture->resolution.height };
    
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
    context->current_area = merge(context->current_area, draw_rect);
    
    s16 thickness = context->border_thickness;
    
    f32 offset;
    if (!is_filled && (thickness <= 1.0f))
        offset = 0.5f;
    else
        offset = 0.0f;
    
    auto command = insert_tail(&context->memory_stack.allocator, &context->current_group->commands);
    *command = make_kind(UI_Command, draw);
    
    if (is_filled)
    {
        command->draw.mode = GL_TRIANGLES;
        command->draw.vertices = ALLOCATE_ARRAY_INFO(&context->memory_stack, UI_Vertex, 4);
        command->draw.indices = ALLOCATE_ARRAY_INFO(&context->memory_stack, u32, 6);
        
        ui_set_quad_vertices(context, command->draw.vertices, draw_rect, texture_rect, color, offset);
        
        // 3--2   l - counter clockwise (left)
        // |l/|
        // |/l|
        // 0--1
        
        ui_set_quad_triangles_indices(command->draw.indices, 0, 1, 2, 3);
    }
    else if (thickness <= 1.0f)
    {
        command->draw.mode = GL_LINES;
        command->draw.vertices = ALLOCATE_ARRAY_INFO(&context->memory_stack, UI_Vertex, 4);
        command->draw.indices = ALLOCATE_ARRAY_INFO(&context->memory_stack, u32, 8);
        
        ui_set_quad_vertices(context, command->draw.vertices, draw_rect, texture_rect, color, offset);
        
        command->draw.indices[0] = 0;
        command->draw.indices[1] = 1;
        command->draw.indices[2] = 1;
        command->draw.indices[3] = 2;
        command->draw.indices[4] = 2;
        command->draw.indices[5] = 3;
        command->draw.indices[6] = 3;
        command->draw.indices[7] = 0;
    }
    else
    {
        command->draw.mode = GL_TRIANGLES;
        command->draw.vertices = ALLOCATE_ARRAY_INFO(&context->memory_stack, UI_Vertex, 8);
        command->draw.indices = ALLOCATE_ARRAY_INFO(&context->memory_stack, u32, 24);
        
        ui_set_quad_vertices(context, command->draw.vertices, draw_rect, texture_rect, color, offset);
        
        draw_rect.x += thickness;
        draw_rect.y += thickness;
        draw_rect.width  -= 2 * thickness;
        draw_rect.height -= 2 * thickness;
        
        ui_set_quad_vertices(context, command->draw.vertices + 4, draw_rect, texture_rect, color, offset);
        
        // 3-------2
        // | 7---6 |
        // | |   | |
        // | |   | |
        // | 4---5 |
        // 0-------1
        
        // bottom border
        ui_set_quad_triangles_indices(command->draw.indices + 0, 0, 1, 5, 4);
        // right border
        ui_set_quad_triangles_indices(command->draw.indices + 6, 5, 1, 2, 6);
        // top border
        ui_set_quad_triangles_indices(command->draw.indices + 12, 7, 6, 2, 3);
        // left border
        ui_set_quad_triangles_indices(command->draw.indices + 18, 0, 4, 7, 3);
    }
}

struct UI_Text_Info {
    area2f area;
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
    
    area2f area = {};
    
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
        
        area2f draw_area;
#if 0
        = {
            cast_v(f32, info->current_x + glyph->draw_x_offset * context->font_rendering.scale),
            cast_v(f32, line_y + glyph->draw_y_offset * context->font_rendering.scale),
            cast_v(f32, glyph->rect.width) * context->font_rendering.scale,
            cast_v(f32, glyph->rect.height) * context->font_rendering.scale
        };
#endif
        
        draw_area.x = info->current_x + glyph->draw_x_offset * context->font_rendering.scale;
        draw_area.y = line_y + glyph->draw_y_offset * context->font_rendering.scale;
        draw_area.width = glyph->rect.width * context->font_rendering.scale;
        draw_area.height = glyph->rect.height * context->font_rendering.scale;
        draw_area.is_valid = true;
        
        area2f texture_area = { 
            cast_v(f32, glyph->rect.x), 
            cast_v(f32, glyph->rect.y), 
            cast_v(f32, glyph->rect.width),
            cast_v(f32, glyph->rect.height)
        };
        
        bool do_merge = ui_clip(context, &draw_area, &texture_area);
        if (do_merge) {
            area = merge(area, draw_area);
            
            if (do_render)
                ui_rect(context, draw_area, texture_area, color);
        }
        
        info->current_x += glyph->draw_x_advance * context->font_rendering.scale;
    }
    
    info->area = merge(info->area, area);
    
    return area;
}

UI_Text_Info ui_text(UI_Context *context, s16 x, s16 y, string text, bool do_render = true, rgba32 color = { 0, 0, 0, 255 })
{
    assert(context->font_rendering.font);
    auto font = context->font_rendering.font;
    
    UI_Text_Info info;
    info.area = {};
    info.line_count = 1;
    info.start_x = x;
    info.start_y = y;
    info.current_x = x;
    info.is_initialized = true; //volume(info.area) > 0.0f
    
    // because ui_text will merge areas but first area is empty
    info.area = ui_text(context, &info, text, do_render, color);
    
    return info;
}

UI_Text_Info ui_fitt_text(UI_Context *context, area2f area, vec2f alignment, string text, rgba32 color = { 0, 0, 0, 255 }, bool sissor_area = true)
{
    auto info = ui_text(context, 0, 0, text, false);
    
    area.is_valid = true;
    SCOPE_PUSH(context->sissor_area, sissor_area ? area : context->sissor_area);
    
    return ui_text(context, area.x - info.area.x + MAX(0, area.width - info.area.width) * alignment.x, area.y - info.area.y + MAX(0, area.height - info.area.height) * alignment.y, text, true, color);
}


area2f ui_write_va(UI_Context *context, UI_Text_Info *info, string format, va_list params)
{
    string output = {};
    auto text = write_va(context->transient_allocator, &output, format, params);
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
    
    bool needs_update = force_update || cursor_was_pressed || cursor_was_released || !are_equal(&cursor_area, &control->cursor_area, sizeof(cursor_area));
    
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

void ui_end(UI_Context *context) {
    
    // upload vertices to GPU
    {
        u8_array vertices = {};
        defer { free_array(&context->memory_stack.allocator, &vertices); };
        
        for_list_item(group_it, context->groups) {
            for_list_item(command_it, group_it->group.commands) {
                auto draw = try_kind_of(&command_it->command, draw);
                if (draw) {
                    auto dest = grow(&context->memory_stack.allocator, &vertices, byte_count_of(draw->vertices));
                    copy(dest, draw->vertices, byte_count_of(draw->vertices));
                }
            }
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, context->vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, byte_count_of(vertices), vertices.data, GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    // upload indices to GPU
    {
        u32_array indices = {};
        defer { free_array(&context->memory_stack.allocator, &indices); };
        
        u32 vertex_offset = 0;
        
        for_list_item(group_it, context->groups) {
            for_list_item(command_it, group_it->group.commands) {
                auto draw = try_kind_of(&command_it->command, draw);
                if (draw) {
                    auto dest = grow(&context->memory_stack.allocator, &indices, draw->indices.count);
                    copy(dest, draw->indices, byte_count_of(draw->indices));
                    
                    for (u32 i = 0; i < draw->indices.count; i++)
                        dest[i] += vertex_offset;
                    
                    vertex_offset += draw->vertices.count;
                }
            }
        }
        
        glBindVertexArray(context->vertex_array_object);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->index_buffer_object);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, byte_count_of(indices), indices.data, GL_STREAM_DRAW);
    }
    
    // render
    {
        glBindVertexArray(context->vertex_array_object);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->index_buffer_object);
        
        usize index_offset = 0;
        auto current_draw_command = make_kind(UI_Command, draw);
        current_draw_command.draw.mode = -1;
        
        glActiveTexture(GL_TEXTURE0 + 0);
        
        for_list_item(group_it, context->groups) {
            if (current_draw_command.draw.mode != -1) {
                glDrawElements(current_draw_command.draw.mode, current_draw_command.draw.indices.count, GL_UNSIGNED_INT, cast_p(GLvoid, index_offset));
                index_offset += byte_count_of(current_draw_command.draw.indices);
                current_draw_command.draw.mode = -1;
            }
            
            glBindTexture(GL_TEXTURE_2D, group_it->group.texture->object);
            
            for_list_item(command_it, group_it->group.commands) {
                auto draw = try_kind_of(&command_it->command, draw);
                if (draw) {
                    if (current_draw_command.draw.mode == -1) {
                        current_draw_command = command_it->command;
                    } else if (current_draw_command.draw.mode == draw->mode) {
                        // ok since we are not using the actual data, we only need the count
                        current_draw_command.draw.vertices.count += draw->vertices.count;
                        current_draw_command.draw.indices.count += draw->indices.count;
                    } else {
                        glDrawElements(current_draw_command.draw.mode, current_draw_command.draw.indices.count, GL_UNSIGNED_INT, cast_p(GLvoid, index_offset));
                        index_offset += byte_count_of(current_draw_command.draw.indices);
                        current_draw_command = command_it->command;
                    }
                }
            }
        }
        
        if (current_draw_command.draw.mode != -1) {
            glDrawElements(current_draw_command.draw.mode, current_draw_command.draw.indices.count, GL_UNSIGNED_INT, cast_p(GLvoid, index_offset));
        }
    }
    
    clear(&context->memory_stack);
    //render(context->render_queue);
    
    //assert(!context->layout_count, "forget a ui_end_layout");
    //clear(&context->render_queue);
    
    context->last_texture_command_offset = -1;
    context->current_group = null;
    context->groups = {};
    //context->texture = null;
    //context->has_cashed_commands = true;
    //context->render_queue_byte_index = 0;
}

#if 0
struct UI_Command {
    enum Kind {
        Kind_layout,
        Kind_offset,
    } kind;
    
    union {
        struct {
            char *function_name; 
            u32 line;
        } layout;
        
        vec2f offset;
    };
};

#define ui_begin_layout(context, layout, x, y, margin) _ui_begin_layout(context, layout, x, y, margin, __FUNCTION__, __LINE__)

#if 0
void ui_begin_item(UI_Context *context, UI_Layout *layout) {
    
    context->current_item_area = {};
    
    auto command = ui_item(context, Render_Queue_Command);
    if (!context->has_cashed_commands)
        *command = make_kind(Render_Queue_Command, custom, sizeof(UI_Command));
    
    assert(is_kind(*command, custom) && (command->custom.byte_count == sizeof(UI_Command)));
    
    auto offset_command = ui_item(context, UI_Command);
    if (!context->has_cashed_commands) {
        *offset_command = make_kind(UI_Command, offset);
        offset_command->offset = {};
    }
    
    assert(is_kind(*offset_command, offset));
    
    context->transform.translation += transform_direction(context->transform, make_vec3(offset_command->offset));
}

void ui_end_item(UI_Context *context, UI_Layout *layout) {
    layout->area = merge(layout->area, context->current_item_area);
    
    auto delta = context->current_item_area.min - layout->cursor - vec2f{ 0.0f, context->current_item_area.height };
    
    layout->cursor += vec2f{ 0.0f, context->current_item_area.height + layout->margin };
    
    if (layout->item_count == 0)
        layout->offset += delta;
    
    auto command = cast_p(Render_Queue_Command, context->render_queue.data + context->current_item.command_byte_index);
    assert(is_kind(*command, custom) && (command->custom.byte_count == sizeof(UI_Command)));
    
    auto offset_command = cast_p(UI_Command, command + 1);
    assert(is_kind(*offset_command, offset));
    offset_command->offset = delta;
    
    layout->item_count++;
}
#endif

void _ui_begin_layout(UI_Context *context, UI_Layout *layout, s16 x, s16 y, s16 margin, char *function_name, u32 line) {
    auto layout = insert_tail(&context->memory_stack.allocator, &context->layouts);
    
    layout->render_queue_byte_index = context->render_queue_byte_index;
    
    layout->margin = margin;
    layout->offset = { (f32)x, (f32)y };
    layout->cursor = layout->offset;
    
    auto command = ui_item(context, Render_Queue_Command);
    *command = make_kind(Render_Queue_Command, custom, sizeof(UI_Command));
    auto layout_command = ui_item(context, UI_Command);
    
    if (context->has_cashed_commands && is_kind(*layout_command, layout)) {
        if ((layout_command->layout.function_name != function_name) || (layout_command->layout.line != line)) {
            context->has_cashed_commands = false;
        }
    }
    
    if (!context->has_cashed_commands) {
        *layout_command = make_kind(UI_Command, layout);
        layout_command->layout.function_name = function_name;
        layout_command->layout.line = line;
    }
    
    ui_begin_item(context, layout);
}

void ui_layout_item(UI_Context *context, UI_Layout *layout) {
    ui_end_item(context, layout);
    ui_begin_item(context, layout);
}

void ui_end_layout(UI_Context *context, UI_Layout *layout) {
    ui_end_item(context, layout);
    
    assert(layout->render_queue_byte_index < byte_count_of(context->render_queue.data));
    auto command = cast_p(Render_Queue_Command, context->render_queue.data + layout->render_queue_byte_index);
    auto layout_command = cast_p(UI_Command, command + 1);
    
    context->transform.translation -= transform_direction(context->transform, vec3f{ 0.0f, -layout->area.height });
    
    context->current_item_area = layout->area;
}

#endif

#endif // YAUI_H