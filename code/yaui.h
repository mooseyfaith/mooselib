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
        bool active_is_done;
        //u32 current_id;
        
        area2f cursor_area;
        bool cursor_was_pressed;
        bool cursor_was_released;
        
        u32 next_hot_id;
        f32 next_hot_priority;
        
        string tooltip;
        f32 tooltip_countdown;
        bool tooltip_is_active;
        
        vec2f drag_start_position;
        
        area2f selection_area;
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
        group = &insert_tail(&context->memory_stack.allocator, &context->groups)->group;
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
    
    set_vertex_attributes(context.vertex_buffer_object, ARRAY_INFO(attribute_infos));
    assert(vertex_byte_count_of(ARRAY_INFO(attribute_infos)) == sizeof(UI_Vertex));
    
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

UI_Command * ui_begin_draw(UI_Context *context, GLenum mode, u32 vertex_count, u32 index_count) {
    auto command = &insert_tail(&context->memory_stack.allocator, &context->current_group->commands)->command;
    *command = make_kind(UI_Command, draw);
    
    command->draw.mode = mode;
    command->draw.vertices = ALLOCATE_ARRAY_INFO(&context->memory_stack, UI_Vertex, vertex_count);
    command->draw.indices = ALLOCATE_ARRAY_INFO(&context->memory_stack, u32, index_count);
    
    return command;
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
    
    auto command = &insert_tail(&context->memory_stack.allocator, &context->current_group->commands)->command;
    *command = make_kind(UI_Command, draw);
    
    if (is_filled)
    {
        command->draw.mode = GL_TRIANGLES;
        command->draw.vertices = ALLOCATE_ARRAY_INFO(&context->memory_stack, UI_Vertex, 4);
        command->draw.indices = ALLOCATE_ARRAY_INFO(&context->memory_stack, u32, 6);
        
        ui_set_quad_vertices(context, command->draw.vertices.data, draw_rect, texture_rect, color, offset);
        
        // 3--2   l - counter clockwise (left)
        // |l/|
        // |/l|
        // 0--1
        
        ui_set_quad_triangles_indices(command->draw.indices.data, 0, 1, 2, 3);
    }
    else if (thickness <= 1.0f)
    {
        command->draw.mode = GL_LINES;
        command->draw.vertices = ALLOCATE_ARRAY_INFO(&context->memory_stack, UI_Vertex, 4);
        command->draw.indices = ALLOCATE_ARRAY_INFO(&context->memory_stack, u32, 8);
        
        ui_set_quad_vertices(context, command->draw.vertices.data, draw_rect, texture_rect, color, offset);
        
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
        
        ui_set_quad_vertices(context, command->draw.vertices.data, draw_rect, texture_rect, color, offset);
        
        draw_rect.x += thickness;
        draw_rect.y += thickness;
        draw_rect.width  -= 2 * thickness;
        draw_rect.height -= 2 * thickness;
        
        ui_set_quad_vertices(context, command->draw.vertices.data + 4, draw_rect, texture_rect, color, offset);
        
        // 3-------2
        // | 7---6 |
        // | |   | |
        // | |   | |
        // | 4---5 |
        // 0-------1
        
        // bottom border
        ui_set_quad_triangles_indices(command->draw.indices.data + 0, 0, 1, 5, 4);
        // right border
        ui_set_quad_triangles_indices(command->draw.indices.data + 6, 5, 1, 2, 6);
        // top border
        ui_set_quad_triangles_indices(command->draw.indices.data + 12, 7, 6, 2, 3);
        // left border
        ui_set_quad_triangles_indices(command->draw.indices.data + 18, 0, 4, 7, 3);
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
        
        bool do_merge = !do_render || ui_clip(context, &draw_area, &texture_area);
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

UI_Text_Info ui_fitt_text(UI_Context *context, area2f area, vec2f alignment, string text, rgba32 color = { 0, 0, 0, 255 }, f32 margin = 0, bool sissor_area = true)
{
    auto info = ui_text(context, 0, 0, text, false);
    
    area.is_valid = true;
    SCOPE_PUSH(context->sissor_area, sissor_area ? area : context->sissor_area);
    
    return ui_text(context, area.x - info.area.x + margin + MAX(0, area.width - 2 * margin - info.area.width) * alignment.x, area.y - info.area.y + margin + MAX(0, area.height - 2 * margin - info.area.height) * alignment.y, text, true, color);
}


area2f ui_write_va(UI_Context *context, UI_Text_Info *info, string format, va_list params)
{
    string output = {};
    defer { free_array(context->transient_allocator, &output); };
    
    auto text = write_va(context->transient_allocator, &output, format, params);
    
    area2f result = ui_text(context, info, text, context->font_rendering.do_render, context->font_rendering.color);
    
    return result;
}

INTERNAL area2f ui_write(UI_Context *context, s16 x, s16 y, string format, ...) {
    va_list params;
    va_start(params, format);
    
    auto info = ui_text(context, x, y, {}, false);
    area2f result = ui_write_va(context, &info, format, params);
    va_end(params);
    
    return result;
}

INTERNAL area2f ui_write(UI_Context *context, s16 x, s16 y, rgba32 color, string format, ...) {
    va_list params;
    va_start(params, format);
    SCOPE_PUSH(context->font_rendering.color, color);
    
    auto info = ui_text(context, x, y, {}, false);
    area2f result = ui_write_va(context, &info, format, params);
    va_end(params);
    
    return result;
}

INTERNAL area2f ui_write(UI_Context *context, UI_Text_Info *info, string format, ...) {
    va_list params;
    va_start(params, format);
    area2f result = ui_write_va(context, info, format, params);
    va_end(params);
    
    return result;
}

INTERNAL area2f ui_write(UI_Context *context, UI_Text_Info *info, rgba32 color, string format, ...) {
    va_list params;
    va_start(params, format);
    SCOPE_PUSH(context->font_rendering.color, color);
    area2f result = ui_write_va(context, info, format, params);
    va_end(params);
    
    return result;
}

INTERNAL void ui_circle(UI_Context *context, vec2f center, f32 radius, rgba32 color, bool is_filled = true, u32 corner_count = 32) {
    
    UI_Command *command;
    UI_Vertex *vertices;
    u32 *indices;
    u32 index_count = 0;
    u32 vertex_count;
    if (!is_filled) {
        command = ui_begin_draw(context, GL_LINE_LOOP, corner_count, corner_count);
        vertices = command->draw.vertices.data;
        indices  = command->draw.indices.data;
        vertex_count = corner_count;
    }
    else {
        command = ui_begin_draw(context, GL_TRIANGLE_FAN, corner_count + 2, corner_count + 2);
        vertices = command->draw.vertices.data;
        indices  = command->draw.indices.data;
        
        ui_set_vertex(context, vertices++, center.x, center.y, 0, 0, color);
        *(indices++) = index_count++;
        
        vertex_count = corner_count + 1;
    }
    
    for (u32 i = 0; i < vertex_count; i++) {
        mat4x3f transform = make_transform(make_quat(VEC3_Z_AXIS, 2 * Pi32 * i / vertex_count), make_vec3(center), { radius, radius, radius });
        
        auto corner = transform_point(transform, vec3f{ 0, 1, 0 });
        ui_set_vertex(context, vertices++, corner.x, corner.y, 0, 0, color);
        *(indices++) = index_count++;
    }
}

vec2f * sample_curve(Memory_Allocator *allocator, vec2f *points, u32 point_count, u32 sample_count) {
    assert(point_count >= 2);
    assert(sample_count >= 2);
    
    auto samples = ALLOCATE_ARRAY(allocator, vec2f, sample_count);
    auto primes = ALLOCATE_ARRAY(allocator, vec2f, point_count);
    defer { free(allocator, primes); };
    
    samples[0] = points[0];
    
    for (u32 i = 1; i < sample_count - 1; i++) {
        copy(primes, points, sizeof(*points) * point_count);
        
        f32 t = (i / cast_v(f32, sample_count - 1));
        
        for (u32 a = 0; a < point_count - 1; a++) {
            for (u32 b = 0; b < point_count - a - 1; b++) {
                primes[b] = linear_interpolation(primes[b], primes[b + 1], t);
            }
        }
        
        samples[i] = primes[0];
    }
    
    samples[sample_count - 1] = points[point_count - 1];
    
    return samples;
}

INTERNAL void ui_curve(UI_Context *context, vec2f *points, u32 point_count, rgba32 color, u32 vertex_count = 32) {
    //assert(point_count >= 2);
    
    //u32 prime_count = point_count;
    //auto primes = ALLOCATE_ARRAY(context->transient_allocator, vec2f, point_count);
    //defer { free(context->transient_allocator, primes); };
    
    //u32 sample_count = vertex_count * 3;
    u32 sample_count = vertex_count;
    auto samples = sample_curve(context->transient_allocator, points, point_count, sample_count);
    defer { free(context->transient_allocator, samples); };
    
#if 0    
    auto arc_lengths = ALLOCATE_ARRAY(context->transient_allocator, f32, sample_count);
    defer { free(context->transient_allocator, arc_lengths); };
    
    arc_lengths[0] = 0;
    for (u32 i = 0; i < sample_count - 1; i++)
        arc_lengths[i + 1] = arc_lengths[i] + length(samples[i + 1] - samples[i]);
    
    f32 segment_length = arc_lengths[sample_count - 1] / vertex_count;
#endif
    
    auto command = ui_begin_draw(context, GL_LINE_STRIP, vertex_count, vertex_count);
    
    ui_set_vertex(context, command->draw.vertices.data + 0, samples[0].x, samples[0].y, 0, 0, color);
    command->draw.indices[0] = 0;
    
#if 0    
    f32 t = 0;
    u32 sample_index = 0;
    for (u32 i = 1; i < vertex_count - 1; i++) {
        t += segment_length;
        
        for (; sample_index < sample_count; sample_index++) {
            if (arc_lengths[sample_index + 1] > t)
                break;
        }
        
        auto l = (arc_lengths[sample_index + 1] - t) / (arc_lengths[sample_index + 1] - arc_lengths[sample_index]);
        
        auto pos = linear_interpolation(samples[sample_index], samples[sample_index + 1], l);
        
        ui_set_vertex(context, command->draw.vertices.data + i + 1, pos.x, pos.y, 0, 0, color);
        command->draw.indices[i + 1] = i + 1;
    }
#else
    for (u32 i = 1; i < vertex_count - 1; i++) {
        ui_set_vertex(context, command->draw.vertices.data + i, samples[i].x, samples[i].y, 0, 0, color);
        command->draw.indices[i] = i;
    }
    
#endif
    
    ui_set_vertex(context, command->draw.vertices.data + vertex_count - 1, samples[sample_count - 1].x, samples[sample_count - 1].y, 0, 0, color);
    command->draw.indices[vertex_count - 1] = vertex_count - 1;
}

INTERNAL bool ui_control_start(UI_Context *context, f32 delta_seconds, bool cursor_was_pressed, bool cursor_was_released, bool force_update = true)
{
    bool needs_update = false;
    auto control = &context->control;
    
    if ((control->hot_id != -1)) {
        if (control->hot_id == control->next_hot_id) {
            control->tooltip_countdown -= delta_seconds;
            if (control->tooltip_countdown <= 0.0f) {
                control->tooltip_is_active = true;
                
                needs_update = true;
            }
        }
    }
    else {
        control->tooltip = {};
    }
    
    if (control->active_id == -1) {
        control->hot_id = control->next_hot_id;
        control->tooltip_countdown = 1.5f;
        control->tooltip_is_active = false;
        needs_update = true;
    }
    
    if (control->active_is_done) {
        control->active_is_done = false;
        control->active_id = -1;
        needs_update = true;
    }
    
    control->next_hot_id = -1;
    control->cursor_was_pressed  = cursor_was_pressed;
    control->cursor_was_released = cursor_was_released;
    
    return needs_update || cursor_was_pressed || cursor_was_released;
}

INTERNAL void ui_set_cursor(UI_Context *context, area2f cursor_area, bool cursor_area_is_active) {
    context->control.cursor_area = cursor_area;
}

bool ui_request_hot(UI_Context *context, u32 id, f32 priority) {
    if ((context->control.next_hot_id == -1) || (context->control.next_hot_priority > priority))
    {
        context->control.next_hot_id = id;
        context->control.next_hot_priority = priority;
        return true;
    }
    
    return false;
}

bool ui_request_active(UI_Context *context, u32 id) {
    if (context->control.hot_id == id) {
        context->control.active_id = id;
        context->control.hot_id = -1;
        return true;
    }
    
    return false;
}

void ui_release_active(UI_Context *context) {
    context->control.active_is_done = true;
}

INTERNAL UI_Control_State ui_state_of(UI_Context *context, u32 id) {
    if (context->control.active_id == id) {
        return UI_Control_State_Active;
    }
    else if (context->control.hot_id == id) {
        return UI_Control_State_Hot;
    }
    
    return UI_Control_State_Idle;
}

INTERNAL bool ui_drag(UI_Context *context, u32 id, area2f area, vec2f anchor, vec2f *delta, f32 priority = 0.0f) {
    auto control = &context->control;
    
    bool was_triggered = false;
    *delta = {};
    
    if (control->active_id == id) {
        *delta = control->drag_start_position + control->cursor_area.min - anchor;
        
        if (control->cursor_was_released) {
            was_triggered = true;
            ui_release_active(context);
        }
    }
    else if (control->cursor_was_pressed && ui_request_active(context, id)) {
        control->drag_start_position = anchor - control->cursor_area.min;
    }
    
    if (intersects(area, control->cursor_area))
        ui_request_hot(context, id, priority);
    
    return was_triggered;
}

INTERNAL bool ui_button(UI_Context *context, u32 id, area2f button_area, f32 priority = 0.0f)
{
    auto control = &context->control;
    
    bool was_triggered = false;
    bool is_hot = intersects(button_area, control->cursor_area);
    
    if (control->active_id == id) {
        if (control->cursor_was_released) {
            if (is_hot)
                was_triggered = true;
            
            ui_release_active(context);
        }
    }
    else if (control->cursor_was_pressed) {
        ui_request_active(context, id);
    }
    
    if (is_hot)
        ui_request_hot(context, id, priority);
    
    return was_triggered;
}

// apply_selection will be passed, so it can not be accidentally forgotten
void ui_area_selector(UI_Context *context, u32 id, area2f area, bool *apply_selection, f32 priority) {
    vec2f delta;
    bool was_triggerd = ui_drag(context, id, area, { 0, 0 }, &delta, priority);
    *apply_selection |= was_triggerd;
    
    if (context->control.active_id == id) {
        context->control.selection_area.min  = context->control.drag_start_position;
        context->control.selection_area.size = delta;
        context->control.selection_area.is_valid = true;
        
        for (u32 i = 0; i < 2; i++) {
            if (context->control.selection_area.size[i] < 0) {
                context->control.selection_area.min[i] += context->control.selection_area.size[i];
                context->control.selection_area.size[i] = -context->control.selection_area.size[i];
            }
        }
    }
}

bool ui_selectable(UI_Context *context, area2f area) {
    return (context->control.selection_area.is_valid && intersects(area, context->control.selection_area));
}

bool ui_slider(UI_Context *context, u32 id, area2f area, f32 *value, vec2f influence, f32 min_value, f32 max_value, f32 priority) {
    vec2f delta;
    bool is_done = ui_drag(context, id, area, { 0, 0 }, &delta, priority);
    
    if (context->control.active_id == id) {
        *value = CLAMP(dot(context->control.drag_start_position + delta, influence), min_value, max_value);
    }
    
    return is_done;
}

#if 0
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
#endif

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
                    copy(dest, draw->vertices.data, byte_count_of(draw->vertices));
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
                    copy(dest, draw->indices.data, byte_count_of(draw->indices));
                    
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
                    } else if (
                        (current_draw_command.draw.mode == draw->mode) && (draw->mode != GL_LINE_STRIP) && (draw->mode != GL_TRIANGLE_STRIP)
                        ) {
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