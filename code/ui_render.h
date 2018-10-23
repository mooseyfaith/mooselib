#if !defined UI_RENDER_H
#define UI_RENDER_H

#include "render_batch.h"
#include "font.h"

#pragma pack(push, 1)
struct UI_Vertex {
    vec3f position;
    rgba32 color;
    vec2f uv;
};
#pragma pack(pop)


#define Template_Array_Type      Render_Material_Buffer
#define Template_Array_Data_Type Render_Material *
#define Template_Array_Is_Buffer
#include "template_array.h"

#define Template_Array_Type      Texture_Buffer
#define Template_Array_Data_Type Texture *
#define Template_Array_Is_Buffer
#include "template_array.h"

struct UI_Render_Context {
    Render_Batch render_batch;
    Render_Material_Buffer material_buffer;
    Texture_Buffer texture_buffer;
    mat4x3 transform; 
    u32 buffer_index;
    f32 y_up_direction;
    
    struct {
        f32 left, right, top, bottom;
    } anchors;
    vec2f center;
    f32 scale;
    
    struct {
        Render_Material *material;
        Font *font;
        rgba32 color;
        f32 line_grow_direction;
        f32 scale;
        vec2f alignment; // -1 to 1 (left to right), (bottom to top)
        u8_buffer text_buffer;
    } font_rendering;
    
    struct {
        rgba32 color;
    } panel_rendering;
};

inline vec3f screen_to_ui_vec(UI_Render_Context *context, s16 left, s16 bottom) {
    return context->transform * vec3f{ CAST_V(f32, left), CAST_V(f32, bottom) };
}

inline vec2f pixel_perfect_point(vec2f point, vec2f pixel_size) {
    return { CAST_V(s16, point.x / pixel_size.x) * pixel_size.x, CAST_V(s16, point.y / pixel_size.y) * pixel_size.y };
}

void init_ui_render_context(UI_Render_Context *context, u32 quad_capacity, u32 command_capacity, u32 material_capacity, Memory_Allocator *allocator)
{
    Vertex_Attribute_Info attribute_infos[] = {
        { Vertex_Position_Index, 3, GL_FLOAT,         GL_FALSE },
        { Vertex_Color_Index,    4, GL_UNSIGNED_BYTE, GL_TRUE },
        { Vertex_UV_Index,       2, GL_FLOAT,         GL_FALSE },
    };
    
    const u32 vertex_size = get_vertex_size(ARRAY_WITH_COUNT(attribute_infos));
    const u32 vertex_capacity = quad_capacity * vertex_size * 4; // 4 vertices per quad
    const u32 index_capacity = quad_capacity * 6 * sizeof(u16); // 6 indices per quad (2 triangles)
    
    context->render_batch = make_render_batch(vertex_capacity, index_capacity, GL_UNSIGNED_SHORT, command_capacity, ARRAY_WITH_COUNT(attribute_infos), allocator);
    
    context->material_buffer = ALLOCATE_ARRAY_INFO(allocator, Render_Material *, material_capacity);
    context->texture_buffer = ALLOCATE_ARRAY_INFO(allocator, Texture *, material_capacity);
    context->buffer_index = -1;
    
    context->font_rendering.text_buffer = ALLOCATE_ARRAY_INFO(allocator, u8, 4 << 10);
    context->font_rendering.color = rgba32{ 0, 0, 0, 255 };
    context->font_rendering.font = NULL;
    context->font_rendering.alignment = { -1.0f, -1.0f }; // align left, bottom
    context->font_rendering.line_grow_direction = -1.0f;
    context->font_rendering.scale = 1.0f;
    
    context->panel_rendering.color = rgba32{ 255, 255, 255, 255 };
}

void ui_set_transform(UI_Render_Context *context, Pixel_Dimensions resolution, f32 scale = 1.0f, f32 y_up_direction = 1.0f, f32 depth_scale = 1.0f, f32 depth_alignment = 0.0f)
{
    // for now using y_up_direction == -1 dows not seem to work properly, it will reverse face drawing order and will be culled if glEnable(GL_CULL_FACE), wich is the default
    assert(y_up_direction == 1.0f);
    
    context->transform.columns[0] = vec3f{ 2.0f * scale / resolution.width };
    context->transform.columns[1] = vec3f{ 0.0f, 2.0f * scale * y_up_direction / resolution.height };
    context->transform.columns[2] = vec3f{ 0.0f, 0.0f, depth_scale };
    context->transform.columns[3] = vec3f{ -1.0f, -1.0f * y_up_direction, depth_alignment * (1.0f - depth_scale) };
    context->y_up_direction = y_up_direction;
    
    context->anchors.left = 0.0f;
    context->anchors.right = resolution.width / scale;
    context->anchors.top = resolution.height / scale * interval_zero_to_one(y_up_direction);
    context->anchors.bottom = resolution.height / scale - context->anchors.top;
    
    context->center.x = (context->anchors.left + context->anchors.right) * 0.5f;
    context->center.y = (context->anchors.top + context->anchors.bottom) * 0.5f;
}

#define UI_SET_MATERIAL(context, material, ...) ui_set_material(context, CAST_P(Render_Material, material), __VA_ARGS__)

struct UI_Material {
    Render_Material *material;
    Texture *texture;
};

UI_Material ui_set_material(UI_Render_Context *context, Render_Material *material, Texture *texture = NULL)
{
    UI_Material old;
    if (context->buffer_index < context->material_buffer.count) {
        old.material = context->material_buffer[context->buffer_index];
        old.texture = context->texture_buffer[context->buffer_index];
    }
    else
        old = {};
    
    if (!material) {
        context->buffer_index = -1;
        return old;
    }
    
    for (u32 i = 0; i < context->material_buffer.count; ++i) {
        if ((context->material_buffer[i] == material) && (context->texture_buffer[i] == texture)) {
            context->buffer_index = i;
            return old;
        }
    }
    
    *push(&context->material_buffer) = material;
    *push(&context->texture_buffer)  = texture;
    context->buffer_index = context->material_buffer.count - 1;
    return old;
}

void ui_raw_push_quad(Render_Batch *batch, u32 material_index, vec3f bottom_left_corner, vec3f width, vec3f height, UV_Area uv_area, rgba32 color) {
    //u32 index_buffer_offset = packed_count(batch->vertex_buffer, UI_Vertex);
    u32 index_buffer_offset = item_count(batch->vertex_buffer, UI_Vertex);
    
    UI_Vertex vertex = {
        bottom_left_corner,
        color,
        { uv_area.min_corner.x, uv_area.min_corner.y }
    };
    pack_item(&batch->vertex_buffer, &vertex);
    
    vertex.position += width;
    vertex.uv[0] += uv_area.size.width;
    pack_item(&batch->vertex_buffer, &vertex);
    
    vertex.position = bottom_left_corner + height;
    vertex.uv = { uv_area.min_corner.x, uv_area.min_corner.y + uv_area.size.height };
    pack_item(&batch->vertex_buffer, &vertex);
    
    vertex.position = bottom_left_corner + width + height;
    vertex.uv = { uv_area.min_corner.x + uv_area.size.width, uv_area.min_corner.y + uv_area.size.height };
    pack_item(&batch->vertex_buffer, &vertex);
    
    Indices *indices = &batch->indices;
    push_index(indices, index_buffer_offset + 0);
    push_index(indices, index_buffer_offset + 1);
    push_index(indices, index_buffer_offset + 2);
    
    push_index(indices, index_buffer_offset + 2);
    push_index(indices, index_buffer_offset + 1);
    push_index(indices, index_buffer_offset + 3);
    
    push_render_command(batch, material_index, GL_TRIANGLES, 6);
}

void ui_push_quad(UI_Render_Context *context, Texture *texture, Pixel_Rectangle texture_rect, vec3f bottom_left_corner, rgba32 color = { 255, 255, 255, 255 }, f32 scale = 1.0f, f32 flip_y = 0.0f)
{
    UV_Area uv_area = {
        texture_rect.x / CAST_V(f32, texture->resolution.width),
        (flip_y * texture_rect.height + texture_rect.y) / CAST_V(f32, texture->resolution.height),
        texture_rect.width / CAST_V(f32, texture->resolution.width), 
        (1.0f - 2.0f * flip_y) * texture_rect.height / CAST_V(f32, texture->resolution.height),
    };
    
    ui_raw_push_quad(&context->render_batch, context->buffer_index, transform_point(context->transform, bottom_left_corner), transform_direction(context->transform, vec3f{ texture_rect.width * scale, 0.0f, 0.0f }), transform_direction(context->transform, vec3f{ 0.0f, texture_rect.height * scale, 0.0f }), uv_area, color);
}

void ui_push_quad(UI_Render_Context *context, s16 x, s16 y, s16 width, s16 height, Texture *texture, Pixel_Rectangle texture_rect, rgba32 color = { 255, 255, 255, 255 }, f32 depth = 0.0f)
{
    UV_Area uv_area = {
        texture_rect.x / cast_v(f32, texture->resolution.width),
        texture_rect.y / cast_v(f32, texture->resolution.height),
        texture_rect.width / cast_v(f32, texture->resolution.width), 
        texture_rect.height / cast_v(f32, texture->resolution.height),
    };
    
    vec3f bottom_left_corner = { cast_v(f32, x), cast_v(f32, y), depth };
    
    ui_raw_push_quad(&context->render_batch, context->buffer_index, transform_point(context->transform, bottom_left_corner), transform_direction(context->transform, vec3f{ cast_v(f32, width), 0.0f, 0.0f }), transform_direction(context->transform, vec3f{ 0.0f, cast_v(f32, height), 0.0f }), uv_area, color);
}

struct UI_Panel {
    s16 *horizontal_slices, *vertical_slices;
    f32 *horizontal_slice_scales, *vertical_slice_scales;
    Texture *texture;
    s16 x, y;
    s16 min_width, min_height;
    s16 draw_x_offset, draw_y_offset;
    u32 horizontal_slice_count, vertical_slice_count;
};

UI_Panel make_ui_panel(Memory_Allocator *allocator, Texture *texture, s16 x, s16 y, s16 *horizontal_slices, f32 *horizontal_slice_scales, u32 horizontal_slice_count, s16 *vertical_slices, f32 *vertical_slice_scales, u32 vertical_slice_count)
{
    UI_Panel result;
    result.texture = texture;
    result.x = x;
    result.y = y;
    result.horizontal_slice_count = horizontal_slice_count;
    result.vertical_slice_count = vertical_slice_count;
    
    result.horizontal_slices = ALLOCATE_ARRAY(allocator, s16, horizontal_slice_count + vertical_slice_count);
    result.vertical_slices = result.horizontal_slices + result.horizontal_slice_count;
    
    result.horizontal_slice_scales = ALLOCATE_ARRAY(allocator, f32, horizontal_slice_count + vertical_slice_count);
    result.vertical_slice_scales = result.horizontal_slice_scales + result.horizontal_slice_count;
    result.draw_x_offset = 0;
    result.draw_y_offset = 0;
    
    result.min_width = 0;
    for (u32 i = 0; i < result.horizontal_slice_count; ++i) {
        result.horizontal_slices[i] = horizontal_slices[i];
        result.horizontal_slice_scales[i] = horizontal_slice_scales[i];
        
        if (horizontal_slice_scales[i] == 0.0f)
            result.min_width += horizontal_slices[i];
    }
    
    result.min_height = 0;
    for (u32 i = 0; i < result.vertical_slice_count; ++i) {
        result.vertical_slices[i] = vertical_slices[i];
        result.vertical_slice_scales[i] = vertical_slice_scales[i];
        
        if (vertical_slice_scales[i] == 0.0f)
            result.min_height += vertical_slices[i];
    }
    
    return result;
}

inline f32 get_panel_slice_width(UI_Panel *panel, f32 x_scale, u32 slice_index) {
    assert(slice_index < panel->horizontal_slice_count);
    
    if (panel->horizontal_slice_scales[slice_index] == 0.0f)
        return panel->horizontal_slices[slice_index];
    else
        return panel->horizontal_slice_scales[slice_index] * (x_scale - panel->min_width);
}


inline f32 get_panel_slice_height(UI_Panel *panel, f32 y_scale, u32 slice_index) {
    assert(slice_index < panel->vertical_slice_count);
    
    if (panel->horizontal_slice_scales[slice_index] == 0.0f)
        return panel->vertical_slices[slice_index];
    else
        return panel->vertical_slice_scales[slice_index] * (y_scale - panel->min_height);
}

Pixel_Rectangle get_ui_panel_rect(UI_Panel *panel, f32 x_scale, f32 y_scale) {
    Pixel_Rectangle rect = {};
    
    for (u32 i = 0; i < panel->horizontal_slice_count; ++i)
        rect.width += get_panel_slice_width(panel, x_scale, i);
    
    for (u32 i = 0; i < panel->vertical_slice_count; ++i)
        rect.height += get_panel_slice_height(panel, y_scale, i);
    
    return rect;
}

Pixel_Rectangle get_ui_panel_texture_rect(UI_Panel *panel) {
    Pixel_Rectangle rect = { panel->x, panel->y };
    
    for (u32 i = 0; i < panel->horizontal_slice_count; ++i)
        rect.width += panel->horizontal_slices[i];
    
    for (u32 i = 0; i < panel->vertical_slice_count; ++i)
        rect.height += panel->vertical_slices[i];
    
    return rect;
}


Pixel_Rectangle get_ui_panel_min_rect(UI_Panel *panel) {
    return get_ui_panel_rect(panel, 0.0f, 0.0f);
}

#if 0
Pixel_Rectangle get_ui_panel_tile_rect(UI_Panel *panel, const f32 *horizontal_scales, const f32 *vertical_scales, u8 tile_left, u8 tile_bottom, u8 horizontal_tile_count = 1, u8 vertical_tile_count = 1)
{
    assert((tile_left + horizontal_tile_count) <= panel->horizontal_tile_count && (tile_bottom + vertical_tile_count) <= panel->vertical_tile_count);
    
    Pixel_Rectangle rect = {};
    
    for (u8 i = 0; i < tile_left; ++i)
        rect.x += (s16)(panel->horizontal_tile_sizes[i] * horizontal_scales[i]);
    
    for (u8 i = tile_left; i < tile_left + horizontal_tile_count; ++i)
        rect.width = (s16)(panel->horizontal_tile_sizes[i] * horizontal_scales[i]);
    
    for (u8 i = 0; i < tile_bottom; ++i)
        rect.y += (s16)(panel->vertical_tile_sizes[i] * vertical_scales[i]);
    
    for (u8 i = tile_bottom; i < tile_bottom + vertical_tile_count; ++i)
        rect.height = (s16)(panel->vertical_tile_sizes[i] * vertical_scales[i]);
    
    return rect;
}
#endif

void ui_panel(UI_Render_Context *context, UI_Panel *panel, s16 x, s16 y, f32 x_scale, f32 y_scale, rgba32 color) {
    assert(panel->horizontal_slice_count && panel->vertical_slice_count);	
    
    //u32 index_offset = packed_count(context->render_batch.vertex_buffer, UI_Vertex);
    u32 index_offset = item_count(context->render_batch.vertex_buffer, UI_Vertex);
    
    vec2f texture_pixel_scale = { 1.0f / (panel->texture->resolution.width), 1.0f / (panel->texture->resolution.height) };
    
    UI_Vertex vertex;
    vertex.color = color;
    
    s16 v = panel->y;
    vertex.uv.v = v * texture_pixel_scale.v;
    
    vec3 left_bottom = { CAST_V(f32, x), CAST_V(f32, y) };
    vec3 vertical_offset = left_bottom + vec3f{ 0.0f, CAST_V(f32, panel->draw_y_offset), 0.0f };
    
    for (u8 y = 0; y != panel->vertical_slice_count + 1; ++y) {
        if (y) {
            vertical_offset += vec3f{ 0.0f, get_panel_slice_height(panel, y_scale, y - 1), 0.0f };
            v += panel->vertical_slices[y - 1];
            vertex.uv.v = v * texture_pixel_scale.v;
        }
        
        vertex.position = transform_point(context->transform, vertical_offset + vec3f{ CAST_V(f32, panel->draw_x_offset), 0.0f, 0.0f });
        s16 u = panel->x;
        vertex.uv.u = u * texture_pixel_scale.u;
        pack_item(&context->render_batch.vertex_buffer, &vertex);
        
        for (u8 x = 1; x != panel->horizontal_slice_count + 1; ++x) {
            vertex.position += transform_direction(context->transform, vec3f{ get_panel_slice_width(panel, x_scale, x - 1), 0.0f, 0.0f});
            u += panel->horizontal_slices[x - 1];
            vertex.uv.u = (f32)u * texture_pixel_scale.u;
            pack_item(&context->render_batch.vertex_buffer, &vertex);
        }
    }
    
    Indices *indices = &context->render_batch.indices;
    
    for (u8 y = 1; y != panel->vertical_slice_count + 1; ++y) {
        for (u8 x = 0; x != panel->horizontal_slice_count + 1; ++x) {
            push_index(indices, index_offset + y * (panel->horizontal_slice_count + 1) + x);
            push_index(indices, index_offset + (y - 1) * (panel->horizontal_slice_count + 1) + x);
        }
        
        if (y < panel->vertical_slice_count) { // if not last strip
            push_index(indices, index_offset + y * (panel->horizontal_slice_count + 1) - 1); // repeat last vertex of current strip						
            push_index(indices, index_offset + (y + 1) * (panel->horizontal_slice_count + 1)); // repeat first vertex of next strip
        }
    }	
    
    push_render_command(&context->render_batch, context->buffer_index, GL_TRIANGLE_STRIP, CAST_V(u32, 2 * panel->vertical_slice_count * (panel->horizontal_slice_count + 2) - 2));
}

void ui_set_font(UI_Render_Context *context, Font *font, Render_Material *material) {
    context->font_rendering.font = font;
    context->font_rendering.material = material;
}

struct UI_Push_Text_Info {
    UV_Area text_area;
    bool first_char_at_line;
    f32 line_left;
    f32 line_right;
    f32 line_bottom;
    f32 line_top;
    u32 line_count;
    f32 line_y;
    f32 x, y;
    bool is_initialized;
};

UV_Area ui_push_text(UI_Render_Context *context, s16 x, s16 y, string text, f32 depth = 0.0f, bool do_render = true, UI_Push_Text_Info *info = null) {
    Font *font;
    UI_Material backup;
    if (do_render) {
        font = context->font_rendering.font;
        backup = ui_set_material(context, context->font_rendering.material, &context->font_rendering.font->texture);
    }
    
    f32 flip_y = interval_zero_to_one(context->y_up_direction);
    UV_Area text_area;
    bool first_char_at_line = true;
    f32 line_left;
    f32 line_right;
    f32 line_bottom;
    f32 line_top;
    u32 line_count = 1;
    f32 line_y = y;
    
    if (info && info->is_initialized) {
        text_area          = info->text_area;
        first_char_at_line = info->first_char_at_line;
        line_left          = info->line_left;
        line_right         = info->line_right;
        line_bottom        = info->line_bottom;
        line_top           = info->line_top;
        line_count         = info->line_count;
        line_y             = info->line_y;
    }
    
    for (u32 i = 0; i != text.count; ++i) {
        if (text[i] == '\n') {
            line_y = y + line_count * (font->pixel_height + 1) * context->font_rendering.line_grow_direction;
            first_char_at_line = true;
            
            if (line_count == 1)
                text_area = { line_left, line_bottom, line_right - line_left, line_top - line_bottom };
            else
                text_area = merge(text_area, { line_left, line_bottom, line_right - line_left, line_top - line_bottom });
            
            ++line_count;
            continue;
        }
        
        Font_Glyph *glyph = get_font_glyph(font, text[i]);
        if (!glyph)
            continue;
        
        if (first_char_at_line) {
            line_left = x + glyph->draw_x_offset;
            line_right = line_left;
            line_bottom = line_y + glyph->draw_y_offset;
            line_top    = line_y + glyph->draw_y_offset + glyph->rect.height;
            first_char_at_line = false;
        }
        else {
            line_right += glyph->draw_x_offset;
            line_bottom = MIN(line_bottom, line_y + glyph->draw_y_offset);
            line_top    = MAX(line_top, line_y + glyph->draw_y_offset + glyph->rect.height);
        }
        
        f32 y_offset = glyph->draw_y_offset * context->y_up_direction - glyph->rect.height * flip_y;
        
        if (do_render)
            ui_push_quad(context, &context->font_rendering.font->texture, glyph->rect, vec3f{ CAST_V(f32, line_right), line_y + y_offset, depth }, context->font_rendering.color, context->font_rendering.scale, flip_y);
        
        line_right += glyph->draw_x_advance;
    }
    
    if (line_count == 1)
        text_area = { line_left, line_bottom, line_right - line_left, line_top - line_bottom };
    else
        text_area = merge(text_area, { line_left, line_bottom, line_right - line_left, line_top - line_bottom });
    
    if (do_render)
        ui_set_material(context, backup.material, backup.texture);
    
    if (info) {
        info->text_area          = text_area;
        info->first_char_at_line = first_char_at_line;
        info->line_left          = line_left;
        info->line_right         = line_right;
        info->line_bottom        = line_bottom;
        info->line_top           = line_top;
        info->line_count         = line_count;
        info->line_y             = line_y;
        info->x                  = x;
        info->y                  = y;
        info->is_initialized     = true;
    }
    
    return text_area;
}

inline void ui_printf_va(UI_Render_Context *context, s16 x, s16 y, string format, va_list params, f32 depth = 0.0f)
{
    bool escaping = false;
    string text = write(&context->font_rendering.text_buffer, &format, params, &escaping);
    
    //u32 vertex_offset = packed_count(context->render_batch.vertex_buffer, UI_Vertex);
    u32 vertex_offset = item_count(context->render_batch.vertex_buffer, UI_Vertex);
    
    
    UV_Area text_area = ui_push_text(context, x, y, text, depth);
    context->font_rendering.text_buffer.count;
    
    s16 x_delta = (text_area.size.width + text_area.min_corner.x - x) * interval_zero_to_one(context->font_rendering.alignment.x);
    s16 y_delta = text_area.size.height * interval_zero_to_one(context->font_rendering.alignment.y);
    
    //u32 vertex_end = packed_count(context->render_batch.vertex_buffer, UI_Vertex);
    u32 vertex_end = item_count(context->render_batch.vertex_buffer, UI_Vertex);
    
    
    vec3f delta = transform_direction(context->transform,  make_vec3(x_delta, y_delta, 0.0f));
    for (u32 vertex_index = vertex_offset; vertex_index < vertex_end; ++vertex_index) {
        item_at(context->render_batch.vertex_buffer, UI_Vertex, vertex_index)->position -= delta;
        //packed_front(context->render_batch.vertex_buffer, UI_Vertex, vertex_index)->position -= delta;
    }
}

inline void ui_printf(UI_Render_Context *context, s16 x, s16 y, f32 depth, string format, ...) {
    va_list params;
    va_start(params, format);
    ui_printf_va(context, x, y, format, params, depth);
    va_end(params);
}

inline void ui_printf(UI_Render_Context *context, s16 x, s16 y, string format, ...) {
    va_list params;
    va_start(params, format);
    ui_printf_va(context, x, y, format, params);
    va_end(params);
}

inline void ui_rect(UI_Render_Context *context, s16 x, s16 y, s16 width, s16 height, rgba32 color = { 255, 255, 255, 255 }, bool is_filled = true, f32 depth = 0.0f)
{
    u8_buffer *vertex_buffer = &context->render_batch.vertex_buffer;
    //u32 index_buffer_offset = packed_count(*vertex_buffer, UI_Vertex);
    u32 index_buffer_offset = item_count(context->render_batch.vertex_buffer, UI_Vertex);
    
    
    vec3f bottom_left_corner = { CAST_V(f32, x), CAST_V(f32, y), depth };
    
    UI_Vertex vertex = {
        transform_point(context->transform, bottom_left_corner),
        color,
        { } // ignored
    };
    pack_item(vertex_buffer, vertex);
    
    vertex.position += transform_direction(context->transform, { CAST_V(f32, width), 0.0f, 0.0f });
    pack_item(vertex_buffer, vertex);
    
    vertex.position = transform_point(context->transform, bottom_left_corner + vec3{ 0.0f, CAST_V(f32, height), 0.0f });
    pack_item(vertex_buffer, vertex);
    
    vertex.position = transform_point(context->transform, bottom_left_corner + vec3{ CAST_V(f32, width), CAST_V(f32, height), 0.0f });
    pack_item(vertex_buffer, vertex);
    
    Indices *indices = &context->render_batch.indices;
    
    if (is_filled) {
        push_index(indices, index_buffer_offset + 0);
        push_index(indices, index_buffer_offset + 1);
        push_index(indices, index_buffer_offset + 2);
        
        push_index(indices, index_buffer_offset + 2);
        push_index(indices, index_buffer_offset + 1);
        push_index(indices, index_buffer_offset + 3);
        
        push_render_command(&context->render_batch, context->buffer_index, GL_TRIANGLES, 6);
    }
    else {
        push_index(indices, index_buffer_offset + 0);
        push_index(indices, index_buffer_offset + 1);
        
        push_index(indices, index_buffer_offset + 1);
        push_index(indices, index_buffer_offset + 3);
        
        push_index(indices, index_buffer_offset + 3);
        push_index(indices, index_buffer_offset + 2);
        
        push_index(indices, index_buffer_offset + 2);
        push_index(indices, index_buffer_offset + 0);
        
        push_render_command(&context->render_batch, context->buffer_index, GL_LINES, 8);
    }
    
    
}

inline void ui_tile(UI_Render_Context *context, s16 x, s16 y, Pixel_Rectangle rect, rgba32 color = { 255, 255, 255, 255 }, vec2 alignment = { -1.0f, -1.0f }, s16 width_scale = 1, s16 height_scale = 1, f32 depth = 0.0f)
{
    u8_buffer *vertex_buffer = &context->render_batch.vertex_buffer;
    //u32 index_buffer_offset = packed_count(*vertex_buffer, UI_Vertex);
    u32 index_buffer_offset = item_count(context->render_batch.vertex_buffer, UI_Vertex);
    
    f32 width = rect.width * width_scale;
    f32 height = rect.height * height_scale;
    
    vec3f bottom_left_corner = { CAST_V(f32, x), CAST_V(f32, y), depth };
    bottom_left_corner.x -= width * interval_zero_to_one(alignment.x);
    bottom_left_corner.y -= height * interval_zero_to_one(alignment.y);
    
    UV_Area uv_area = make_uv_rect(context->texture_buffer[context->buffer_index], rect);
    
    UI_Vertex vertex = {
        transform_point(context->transform, bottom_left_corner),
        color,
        { uv_area.min_corner.x, uv_area.min_corner.y }
    };
    pack_item(vertex_buffer, vertex);
    
    vertex.position += transform_direction(context->transform, { width, 0.0f, 0.0f });
    vertex.uv.x += uv_area.size.width;
    pack_item(vertex_buffer, vertex);
    
    vertex.position = transform_point(context->transform, bottom_left_corner + vec3{ 0.0f, height, 0.0f });
    vertex.uv = { uv_area.min_corner.x, uv_area.min_corner.y + uv_area.size.height };
    pack_item(vertex_buffer, vertex);
    
    vertex.position = transform_point(context->transform, bottom_left_corner + vec3{ width, height, 0.0f });
    vertex.uv = { uv_area.min_corner.x + uv_area.size.width, uv_area.min_corner.y + uv_area.size.height };
    pack_item(vertex_buffer, vertex);
    
    Indices *indices = &context->render_batch.indices;
    
    push_index(indices, index_buffer_offset + 0);
    push_index(indices, index_buffer_offset + 1);
    push_index(indices, index_buffer_offset + 2);
    
    push_index(indices, index_buffer_offset + 2);
    push_index(indices, index_buffer_offset + 1);
    push_index(indices, index_buffer_offset + 3);
    
    push_render_command(&context->render_batch, context->buffer_index, GL_TRIANGLES, 6);
}

void draw(UI_Render_Context *context) {
    draw(&context->render_batch, context->material_buffer.data, context->material_buffer.count);
    context->material_buffer.count = 0;
    context->texture_buffer.count = 0;
};

#endif