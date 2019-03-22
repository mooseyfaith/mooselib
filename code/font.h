#if !defined FONT_H
#define FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftmodapi.h>

#include "gl.h"
#include "mo_string.h"

//ToDo: free objects

struct Font_Glyph {
    u32 character_code; // utf-32
    
    Pixel_Rectangle rect;
    
    s16 draw_x_offset;
    s16 draw_y_offset;
    f32 draw_x_advance;
};

struct Font {
    Texture texture;
    s16 pixel_height;
    u32 glyph_count;
    Font_Glyph *glyphs;
};

#define FT_ALLOC_DEC(name) void * name(FT_Memory memory, long size)
#define FT_FREE_DEC(name)  void name(FT_Memory memory, void *block)
#define FT_REALLOC_DEC(name) void * name(FT_Memory memory, long cur_size, long new_size, void *block)

FT_ALLOC_DEC(ft_allocate) {
    return allocate(cast_p(Memory_Allocator, memory->user), size, MEMORY_MAX_ALIGNMENT);
}

FT_FREE_DEC(ft_free) {
    free(cast_p(Memory_Allocator, memory->user), block);
}

FT_REALLOC_DEC(ft_reallocate) {
    reallocate(cast_p(Memory_Allocator, memory->user), &block, new_size, MEMORY_MAX_ALIGNMENT);
    return block;
}

// since this struct is maintained by freetype lib,
// you have to manually make shure that it remains valid after a dll reaload
// just call this function again with the same arguments
void init_font_allocator(FT_MemoryRec_ *memory_rec, Memory_Allocator *allocator) {
    FT_MemoryRec_ result;
    memory_rec->user = allocator;
    memory_rec->alloc = ft_allocate;
    memory_rec->free  = ft_free;
    memory_rec->realloc = ft_reallocate;
}

FT_Library begin_font_loading() {
    FT_Library library;
    
#if 1
    
    FT_Error error = FT_Init_FreeType(&library);
    
#else    
    
    FT_Error error = FT_New_Library(allocator, &library);
    FT_Add_Default_Modules(library);
    FT_Set_Default_Properties(library);
    
#endif
    
    if (error) {
        printf("Error: %s failed\n", __FUNCTION__);
        return {};
    }
    
    return library;
}

void end_font_loading(FT_Library ft_library) {
    FT_Error error = FT_Done_FreeType(ft_library);
    
    if (error) {
        printf("Error: %s failed\n", __FUNCTION__);
    }
}

Font make_font(Memory_Allocator *allocator, FT_Library ft_library, u8_array source, s16 height, u32 first_char, u32 char_count, bool smooth = true)
{
    Font result = {};
    result.pixel_height = height;
    result.texture = {};
    
    FT_Face face;
    FT_Error error = FT_New_Memory_Face(ft_library, cast_p(const FT_Byte, source.data), cast_v(FT_Long, source.count), 0, &face);
    
    if (error == FT_Err_Unknown_File_Format)
        printf("Error: %s failed, unsupported font format from source.\n", __FUNCTION__);
    else if (error) 
        printf("Error: %s failed, could not open source as a font\n", __FUNCTION__);
    
    error = FT_Set_Pixel_Sizes(face, 0, height);
    if (error)
        printf("Error: %s failed, could not set font size to %i\n", __FUNCTION__, cast_v(s32, height));
    
    result.glyph_count = char_count;
    result.glyphs = ALLOCATE_ARRAY(allocator, Font_Glyph, result.glyph_count);
    
    FT_GlyphSlot glyph_slot = face->glyph;
    
    u32 estimated_area = 0;
    for (u32 c = 0; c < char_count; ++c) {
        if (FT_Load_Char(face, first_char + c, FT_LOAD_RENDER))  {
            printf("could not load char %u\n", first_char + c);
            continue;
        }
        
        if (glyph_slot->bitmap.width && glyph_slot->bitmap.rows)
            estimated_area += (glyph_slot->bitmap.width + 1) * (glyph_slot->bitmap.rows + 1);
    }
    
    u8_array bitmap = {};
    defer { free_array(allocator, &bitmap); };
    Pixel_Dimensions bitmap_resolution = {};
    
    bool did_not_fit;
    do {
        did_not_fit = false;
        
        Pixel_Dimensions resolution;
        resolution.height = 1 << bit_count_of(2 * height);
        resolution.width = 1 << bit_count_of(estimated_area / resolution.height + 1);
        
        bitmap_resolution = resolution;
        u32 min_area = resolution.width * resolution.height;
        u32 min_side_diff = abs(resolution.width - resolution.height);
        while (resolution.width > height * 2) {
            resolution.height <<= 1;
            resolution.width = 1 << bit_count_of(estimated_area / resolution.height + 1);
            u32 area = resolution.width * resolution.height;
            u32 side_diff = abs(resolution.width - resolution.height);
            if ((area < min_area) || ((area == min_area) && (side_diff < min_side_diff))) {
                bitmap_resolution = resolution;
                min_area = area;
                min_side_diff = side_diff;
            }
        }
        
        free_array(allocator, &bitmap);
        grow(allocator, &bitmap, bitmap_resolution.width * bitmap_resolution.height);
        reset(bitmap.data, byte_count_of(bitmap));
        
        s16 x = 0;
        s16 y = 0;
        s16 current_max_char_height = 0;
        u32 fill_count = 0;
        
        u32 flags = FT_LOAD_RENDER;
        if (!smooth)
            flags |= FT_LOAD_MONOCHROME;
        
        for (u32 c = 0; c < char_count; ++c)
        {
            
            if (FT_Load_Char(face, first_char + c, flags))
            {
                printf("could not load char %u\n", first_char + c);
                continue;
            }
            
            if (y + cast_v(s16, glyph_slot->bitmap.rows) >= bitmap_resolution.height) {
                did_not_fit = true;
                break;
            }
            
            if (x + cast_v(s16, glyph_slot->bitmap.width) >= bitmap_resolution.width) {
                x = 0;
                y += current_max_char_height + 1;
                current_max_char_height = 0;
                
                if (y + cast_v(s16, glyph_slot->bitmap.rows) >= bitmap_resolution.height) {
                    did_not_fit= true;
                    break;
                }
            }
            
            if (current_max_char_height < cast_v(s16, glyph_slot->bitmap.rows))
                current_max_char_height = cast_v(s16, glyph_slot->bitmap.rows);
            
            Font_Glyph glyph = {
                first_char + c,
                { x, y, cast_v(s16, glyph_slot->bitmap.width), cast_v(s16, glyph_slot->bitmap.rows) },
                cast_v(s16, glyph_slot->bitmap_left), cast_v(s16, glyph_slot->bitmap_top - glyph_slot->bitmap.rows), glyph_slot->advance.x / 64.0f,
            };
            
            if (glyph_slot->bitmap.buffer) {
                
                if (!smooth) 
                {
                    // copy flipped and expand bits to bytes
                    for (u32 row = 0; row < glyph_slot->bitmap.rows; ++row) {
                        for (u32 glyph_x = 0; glyph_x < glyph_slot->bitmap.width; ++glyph_x) {
                            u32 p = glyph_slot->bitmap.width - glyph_x - 1;
                            u8 value;
                            if (glyph_slot->bitmap.buffer[row * glyph_slot->bitmap.pitch + p / 8] & (1 << ((8 - p - 1) % 8)))
                                value = 255;
                            else 
                                value = 0;
                            
                            bitmap[x + p + (y + glyph_slot->bitmap.rows - row - 1) * bitmap_resolution.width] = value;
                        }
                    }
                    
                } else {
                    
                    // copy flipped
                    for (u32 row = 0; row < glyph_slot->bitmap.rows; row++) {
                        copy(bitmap.data + x + (y + row) * bitmap_resolution.width, glyph_slot->bitmap.buffer + (glyph_slot->bitmap.rows - row - 1) * glyph_slot->bitmap.pitch, glyph_slot->bitmap.pitch);
                    }
                    
                }
                
                x += cast_v(s16, glyph_slot->bitmap.width) + 1;
                ++fill_count;
            }
            
            result.glyphs[c] = glyph;
        }
        
        estimated_area = estimated_area * 1.2f;
    } while (did_not_fit);
    
    result.texture = make_alpha_texture(bitmap_resolution.width, bitmap_resolution.height, Texture_Filter_Level_Nearest);
    glBindTexture(GL_TEXTURE_2D, result.texture.object);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bitmap_resolution.width, bitmap_resolution.height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data);
    
    FT_Done_Face(face);
    
    return result;
}

Font_Glyph * get_font_glyph(const Font *font, u32 character_code) {
    for (Font_Glyph *it = font->glyphs, *end = font->glyphs + font->glyph_count; it != end; ++it) {
        if (it->character_code == character_code)
            return it;
    }
    
    return NULL;
}

//	if (line_spaceing == 0)
//		line_spaceing = font->pixel_height + 1;
area2f get_text_rect(u32 *line_count, const Font *font, string text, s16 line_spaceing = 0) {
    if (!text.count) {
        *line_count = 1;
        return {};
    }
    
    struct Rect {
        f32 left, right;
        f32 bottom, top;
    };
    
    const u8 *it = text.data, *end = one_past_last(text);
    Rect text_rect;
    for (; it != end; ++it) {
        Font_Glyph *glyph = get_font_glyph(font, *it);
        if (!glyph)
            continue;
        
        text_rect.left = glyph->draw_x_offset;
        text_rect.right = text_rect.left - glyph->rect.width;
        text_rect.top = glyph->draw_y_offset;
        text_rect.bottom = text_rect.top - glyph->rect.height;
        *line_count = 0;
        break;
    }
    
    for (; it != end; ++it) { // per line		
        if (*it == '\n') { // line starts with '\n' (new line)
            ++(*line_count);
            continue;
        }
        
        Rect line_rect;
        
        { // check first character of line
            Font_Glyph *glyph = get_font_glyph(font, *it);
            if (!glyph)
                continue;
            
            line_rect.left = glyph->draw_x_offset;
            line_rect.right = glyph->draw_x_advance;
            line_rect.top = glyph->draw_y_offset;
            line_rect.bottom = glyph->draw_y_offset - glyph->rect.height;
            
            ++it;
            if (it == end) {
                break;
            }
        }
        
        s16 right_space; // space after last character
        for (; (it != end) && (*it != '\n'); ++it) {
            Font_Glyph *glyph = get_font_glyph(font, *it);
            if (!glyph)
                continue;
            
            line_rect.right += glyph->draw_x_advance;
            
            if (line_rect.top < glyph->draw_y_offset)
                line_rect.top = glyph->draw_y_offset;
            
            if (line_rect.bottom > glyph->draw_y_offset - glyph->rect.height)
                line_rect.bottom = glyph->draw_y_offset - glyph->rect.height;
            
            right_space = glyph->draw_x_advance - glyph->draw_x_offset - glyph->rect.width;
        }
        
        line_rect.right -= right_space;
        
        if (text_rect.left > line_rect.left)
            text_rect.left = line_rect.left;
        
        if (text_rect.right < line_rect.right)
            text_rect.right = line_rect.right;
        
        if (*line_count == 0)
            text_rect.top = line_rect.top;
        
        ++(*line_count);
        
        if (it == end) { // last line height
            text_rect.bottom = line_rect.bottom;
            break;
        }
    }
    
    if (line_spaceing == 0)
        line_spaceing = font->pixel_height + 1;
    
    return {
        text_rect.left,
        text_rect.top,
        text_rect.right - text_rect.left,
        text_rect.top - text_rect.bottom + (s16)(*line_count - 1) * line_spaceing,
    };
}

// see Pixel_Rectangle get_text_rect(u32 *line_count, const Font *font, u32 text_length, const u8 *text, s16 line_spaceing = 0)
area2f get_text_rect(const Font *font, string text, s16 line_spaceing = 0) {
    u32 line_count;
    return get_text_rect(&line_count, font, text, line_spaceing);
}

#endif