#if !defined IMAGE_PROCCESSING_H
#define IMAGE_PROCCESSING_H

#define Template_Array_Type      rgba32_array
#define Template_Array_Data_Type rgba32
#include "template_array.h"

struct Image {
    rgba32_array pixels;
    Pixel_Dimensions resolution;
    
    //ARRAY_OPERATORS_DEC(pixels.data, pixels.capacity);
};

inline rgba32 * color_at(Image *image, s16 x, s16 y) {
    return image->pixels + y * image->resolution.width + x;
}

#define Template_Array_Type      Pixel_Position_Buffer
#define Template_Array_Data_Type Pixel_Position
#define Template_Array_Is_Buffer
#include "template_array.h"

struct Pixel_Queue {
    u8 *states;
    Pixel_Position_Buffer queue;
};

#define Template_Array_Type      Pixel_Rectangle_Buffer
#define Template_Array_Data_Type Pixel_Rectangle
#define Template_Array_Is_Buffer
#include "template_array.h"

#define Template_Array_Type      UI_Panel_Buffer
#define Template_Array_Data_Type UI_Panel
#define Template_Array_Is_Buffer
#include "template_array.h"

#if 0
struct Get_Tiles_And_Panels_Result {
    Pixel_Rectangle *tiles;
    u32 tile_count;
    UI_Panel *panels;
    u32 panel_count;
};


inline bool is_unvisited_pixel(Image *image, Pixel_Queue *pixel_queue, s16 x, s16 y, rgba32 panel_slice_color, rgba32 tile_anchor_color)
{
    rgba32 color = *color_at(image, x, y);
    return !pixel_queue->states[y * image->resolution.width + x] && color.a && (color != panel_slice_color) && (color != tile_anchor_color);
}

bool get_connected_tile(OUTPUT Pixel_Rectangle *tile, Image *image, Pixel_Queue *pixel_queue, s16 start_x, s16 start_y, rgba32 panel_slice_color, rgba32 tile_anchor_color)
{
    pixel_queue->queue.count = 0;
    
    if (!is_unvisited_pixel(image, pixel_queue, start_x, start_y, panel_slice_color, tile_anchor_color))
        return false;
    
    //{
    //rgba32 color = { 255, 0, 255, 255 }; 
    //assert((color != panel_slice_color) && (color != tile_anchor_color));
    //}
    
    Pixel_Rectangle result = { start_x, start_y, 1, 1 };
    
    Pixel_Position pos = { start_x, start_y };
    *push(&pixel_queue->queue) = pos;
    pixel_queue->states[start_y * image->resolution.width + start_x] = 1;
    
    while (pixel_queue->queue.count) {
        Pixel_Position pos = pixel_queue->queue[pixel_queue->queue.count - 1];
        pop(&pixel_queue->queue);
        
        for (s16 y = pos.y - 1; y < pos.y + 2; ++y) {
            for (s16 x = pos.x - 1; x < pos.x + 2; ++x) {
                if (!contains({ 0, 0, image->resolution.width, image->resolution.height }, x, y))
                    continue;
                
                if (is_unvisited_pixel(image, pixel_queue, x, y, panel_slice_color, tile_anchor_color))
                {
                    Pixel_Position pos = { x, y };
                    *push(&pixel_queue->queue) = pos;
                    pixel_queue->states[y * image->resolution.width + x] = 1;
                    
                    if (x < result.x) {
                        result.width += result.x - x;
                        result.x = x;
                    }
                    
                    if (x > result.x + result.width - 1)
                        result.width = x - result.x + 1;
                    
                    if (y < result.y) {
                        result.height += result.y - y;
                        result.y = y;
                    }
                    
                    if (y > result.y + result.height - 1)
                        result.height = y - result.y + 1;
                }
            }
        }
    }
    
    *tile = result;
    return true;
}

u32 get_slices(Image *image, Pixel_Rectangle tile, bool horizontal, rgba32 panel_slice_color, rgba32 tile_anchor_color, OUTPUT s16 *slices = NULL, OUTPUT f32 *scales = NULL, OUTPUT s16 *min_length = NULL)
{
    s16 const margin = 1;
    
    s16 x;
    s16 y;
    
    // indirectly change x or y
    s16 *index;
    s16 end;
    if (horizontal) {
        if (tile.width < 2)
            return 0;
        
        x = tile.x;
        y = tile.y + tile.height + margin;
        index = &x;
        end = tile.x + tile.width;
    }
    else {
        if (tile.height < 2)
            return 0;
        
        x = tile.x + tile.width + margin;
        y = tile.y;
        index = &y;
        end = tile.y + tile.height;
    }
    
    rgba32 color = *color_at(image, x, y);
    
    if (color.a && (color != panel_slice_color) && (color != tile_anchor_color))
        return 0;
    
    s16 slice_length = 1;
    bool is_slice_color = (color == panel_slice_color);
    u32 count = 0;
    
    if (min_length)
        *min_length = 0;
    
    for (++(*index); *index != end; ++(*index)) {
        rgba32 color = *color_at(image, x, y);
        
        if (color.a && (color != panel_slice_color) && (color != tile_anchor_color))
            return 0;
        
        if (is_slice_color == (color == panel_slice_color)) {
            ++slice_length;
        } else {
            if (slices) {
                slices[count] = slice_length;
                assert(scales);
                scales[count] = is_slice_color ? 0.0f : 1.0f;
            }
            
            if (is_slice_color && min_length)
                *min_length += slice_length;
            
            is_slice_color = !is_slice_color;
            
            ++count;
            slice_length = 1;
        }
    }
    
    if (count) {
        if (slices) {
            slices[count] = slice_length;
            assert(scales);
            scales[count] = is_slice_color ? 0.0f : 1.0f;
        }
        
        if (is_slice_color && min_length)
            *min_length += slice_length;
        
        ++count;
    }
    
    return count;
}

Get_Tiles_And_Panels_Result get_tiles_and_panels(Image *image, Memory_Allocator *persistent_allocator, Memory_Allocator *temporary_allocator, rgba32 panel_slice_color = { 255, 0, 255, 255 }, rgba32 tile_anchor_color = { 128, 128, 0, 255 })
{
    Pixel_Queue pixel_queue;
    pixel_queue.queue = ALLOCATE_ARRAY_INFO(temporary_allocator, Pixel_Position, image->pixels.capacity);
    pixel_queue.states = ALLOCATE_ARRAY(temporary_allocator, u8, image->pixels.capacity);
    reset(pixel_queue.states, image->pixels.capacity);
    
    // if every second pixel was clear, we would need 0.25 * #pixels tiles
    Pixel_Rectangle_Buffer tiles = ALLOCATE_ARRAY_INFO(temporary_allocator, Pixel_Rectangle, CAST_V(u32, (image->resolution.width + 1) * (image->resolution.height + 1)) / 4);
    
    for (s32 y = 0; y < image->resolution.height; ++y) {
        for (s32 x = 0; x < image->resolution.width; ++x) {
            Pixel_Rectangle tile;
            if (!get_connected_tile(&tile, image, &pixel_queue, x, y, panel_slice_color, tile_anchor_color))
                continue;
            
            bool do_push = true;
            for (u32 tile_index = 0; tile_index < tiles.count; ++tile_index) {
                if (contains(tile, tiles[tile_index].x, tiles[tile_index].y) || contains(tiles[tile_index], tile.x, tile.y)) {
                    do_push = false;
                    break;
                }
            }
            
            if (do_push)
                *push(&tiles) = tile;
        }
    }
    
    u32 panel_count = 0;
    for (u32 tile_index = 0; tile_index < tiles.count; ++tile_index) {
        Pixel_Rectangle tile = tiles[tile_index];
        
        u32 horizontal_slice_count = get_slices(image, tile, true, panel_slice_color, tile_anchor_color);
        u32 vertical_slice_count = get_slices(image, tile, false, panel_slice_color, tile_anchor_color);
        
        // is panel
        if (horizontal_slice_count || vertical_slice_count)
            ++panel_count;
    }
    
    Get_Tiles_And_Panels_Result result;
    
    if (panel_count) {
        UI_Panel_Buffer panels = ALLOCATE_ARRAY_INFO(persistent_allocator, UI_Panel, panel_count);
        
        u32 slice_count = 0;
        u32 tile_index = 0;
        while (tile_index < tiles.count) {
            Pixel_Rectangle tile = tiles[tile_index];
            
            u32 horizontal_slice_count = get_slices(image, tile, true, panel_slice_color, tile_anchor_color);
            u32 vertical_slice_count = get_slices(image, tile, false, panel_slice_color, tile_anchor_color);
            
            // is panel
            if (horizontal_slice_count || vertical_slice_count) {
                UI_Panel panel;
                panel.horizontal_slice_count = horizontal_slice_count;
                panel.vertical_slice_count = vertical_slice_count;
                panel.x = tile.x;
                panel.y = tile.y;
                panel.draw_x_offset = 0;
                panel.draw_y_offset = 0;
                
                panel.horizontal_slices = ALLOCATE_ARRAY(temporary_allocator, s16, horizontal_slice_count + vertical_slice_count);
                panel.vertical_slices = panel.horizontal_slices + horizontal_slice_count;
                
                panel.horizontal_slice_scales = ALLOCATE_ARRAY(temporary_allocator, f32, horizontal_slice_count + vertical_slice_count);
                panel.vertical_slice_scales = panel.horizontal_slice_scales + horizontal_slice_count;
                
                u32 count = get_slices(image, tile, true, panel_slice_color, tile_anchor_color, panel.horizontal_slices, panel.horizontal_slice_scales, &panel.min_width);
                assert(count == horizontal_slice_count);
                
                count = get_slices(image, tile, false, panel_slice_color, tile_anchor_color, panel.vertical_slices, panel.vertical_slice_scales, &panel.min_height);
                assert(count == vertical_slice_count);
                
                *push(&panels) = panel;
                
                slice_count += horizontal_slice_count + vertical_slice_count;
                
                tiles[tile_index] = tiles[tiles.count - 1];
                tiles.count--;
            }
            else {
                ++tile_index;
            }
        }
        
        s16 *slices       = ALLOCATE_ARRAY(persistent_allocator, s16, slice_count);
        f32 *slice_scales = ALLOCATE_ARRAY(persistent_allocator, f32, slice_count);
        
        // free in reverse order, cause we can
        for (s64 panel_index = panels.count - 1; panel_index; --panel_index) {
            UI_Panel *panel = panels + panel_index;
            
            s16 *old_slices = panel->horizontal_slices;
            
            copy(slices, panel->horizontal_slices, panel->horizontal_slice_count * sizeof(s16));
            panel->horizontal_slices = slices;
            slices += panel->horizontal_slice_count;
            
            copy(slices, panel->vertical_slices, panel->vertical_slice_count * sizeof(s16));
            panel->vertical_slices = slices;
            slices += panel->vertical_slice_count;
            
            f32 *old_slice_scales = panel->horizontal_slice_scales;
            
            copy(slice_scales, panel->horizontal_slice_scales, panel->horizontal_slice_count * sizeof(f32));
            panel->horizontal_slice_scales = slice_scales;
            slice_scales += panel->horizontal_slice_count;
            
            copy(slice_scales, panel->vertical_slice_scales, panel->vertical_slice_count * sizeof(f32));
            panel->vertical_slice_scales = slice_scales;
            slice_scales += panel->vertical_slice_count;
            
            free(temporary_allocator, old_slice_scales);
            free(temporary_allocator, old_slices);
        }
        
        result.panels = panels;
        result.panel_count = panels.count;
    }
    else {
        result.panels = NULL;
        result.panel_count = 0;
    }
    
    if (tiles.count)
        result.tiles = ALLOCATE_ARRAY(persistent_allocator, Pixel_Rectangle, tiles.count);
    else
        result.tiles = NULL;
    
    result.tile_count = tiles.count;
    
    for (u32 tile_index = 0; tile_index < tiles.count; ++tile_index)
        result.tiles[tile_index] = tiles[tile_index];
    
    free(temporary_allocator, tiles.data);
    free(temporary_allocator, pixel_queue.states);
    free(temporary_allocator, pixel_queue.queue.data);
    
    return result;
}
#endif

#endif // IMAGE_PROCCESSING_H