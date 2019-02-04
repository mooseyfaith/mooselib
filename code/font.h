#if !defined(_ENGINE_FONT_H_)
#define _ENGINE_FONT_H_

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

FT_Library make_font_library(FT_MemoryRec_ *allocator) {
    FT_Library library;
    //FT_Error error = FT_Init_FreeType(&library);
    
    FT_Error error = FT_New_Library(allocator, &library);
    FT_Add_Default_Modules(library);
    FT_Set_Default_Properties(library);
    
    if (error) {
        printf("Error (init_fonts): could not init freetype2 lib\n");
        return {};
    }
    
    return library;
}

Font make_font(FT_Library ft_library, string file_path, s16 height, u32 first_char, u32 char_count, Platform_Read_Entire_File_Function read_file, Memory_Allocator *allocator)
{
    Font result = {};
    result.pixel_height = height;
    result.texture = {};
    
    u8_array chunk = read_file(file_path, allocator);
    if (!chunk.count) {
        printf("Error (create_font): could not open file %.*s\n", FORMAT_S(&file_path));
        return {};
    }
    
    FT_Face face;
    FT_Error error = FT_New_Memory_Face(ft_library, cast_p(const FT_Byte, chunk.data), cast_v(FT_Long, chunk.count), 0, &face);
    
    if (error == FT_Err_Unknown_File_Format)
        printf("Error (create_font): unsupported font format from file %.*s\n", FORMAT_S(&file_path));
    else if (error) 
        printf("Error (create_font): could not open file %.*s as a font\n", FORMAT_S(&file_path));
    
    error = FT_Set_Pixel_Sizes(face, 0, height);
    if (error)
        printf("Error (create_font): could not set font size to %i\n", cast_v(s32, height));
    
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
    
    bool did_not_fit;
    do {
        did_not_fit = false;
        
        Pixel_Dimensions resolution;
        resolution.height = 1 << bit_count(2 * height);
        resolution.width = 1 << bit_count(estimated_area / resolution.height + 1);
        
        Pixel_Dimensions min_resolution = resolution;
        u32 min_area = resolution.width * resolution.height;
        u32 min_side_diff = abs(resolution.width - resolution.height);
        while (resolution.width > height * 2) {
            resolution.height <<= 1;
            resolution.width = 1 << bit_count(estimated_area / resolution.height + 1);
            u32 area = resolution.width * resolution.height;
            u32 side_diff = abs(resolution.width - resolution.height);
            if ((area < min_area) || ((area == min_area) && (side_diff < min_side_diff))) {
                min_resolution = resolution;
                min_area = area;
                min_side_diff = side_diff;
            }
        }
        
        if (result.texture.object) {
            glBindTexture(GL_TEXTURE_2D, 0);
            glDeleteTextures(1, &result.texture.object);
        }
        
        result.texture = make_alpha_texture(min_resolution.width, min_resolution.height, Texture_Filter_Level_Nearest);
        //TODO: check if texture is valid
        glBindTexture(GL_TEXTURE_2D, result.texture.object);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        s16 x = 0;
        s16 y = 0;
        s16 current_max_char_height = 0;
        u32 fill_count = 0;
        
        for (u32 c = 0; c < char_count; ++c) {
            
#define BLACK_AND_WHITE 0
            
#if BLACK_AND_WHITE
            if (FT_Load_Char(face, first_char + c, FT_LOAD_RENDER| FT_LOAD_MONOCHROME)) 
#else
                if (FT_Load_Char(face, first_char + c, FT_LOAD_RENDER)) 
#endif
            {
                printf("could not load char %u\n", first_char + c);
                continue;
            }
            
            if (y + cast_v(s16, glyph_slot->bitmap.rows) >= result.texture.resolution.height) {
                did_not_fit= true;
                break;
            }
            
            if (x + cast_v(s16, glyph_slot->bitmap.width) >= result.texture.resolution.width) {
                x = 0;
                y += current_max_char_height + 1;
                current_max_char_height = 0;
                
                if (y + cast_v(s16, glyph_slot->bitmap.rows) >= result.texture.resolution.height) {
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
#if BLACK_AND_WHITE                
                u8 *buffer = ALLOCATE_ARRAY(allocator, u8, glyph_slot->bitmap.width * glyph_slot->bitmap.rows);
                for (u32 y = 0; y < glyph_slot->bitmap.rows; ++y) {
                    
                    for (u32 x = 0; x < glyph_slot->bitmap.width; ++x) {
                        u32 p = glyph_slot->bitmap.width - x - 1;
                        if (glyph_slot->bitmap.buffer[y * glyph_slot->bitmap.pitch + p/8] & (1 << ((8 - p - 1) % 8))) {
                            buffer[y * glyph_slot->bitmap.width + glyph_slot->bitmap.width - x - 1] = 255;
                        }
                        else {
                            buffer[y * glyph_slot->bitmap.width + glyph_slot->bitmap.width - x - 1] = 0;
                        }
                    }
                }
                
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, glyph_slot->bitmap.width, glyph_slot->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, buffer);
                free(allocator, buffer);
                
#else
                // flip bitmap
                u8 *bitmap = ALLOCATE_ARRAY(allocator, u8, glyph_slot->bitmap.pitch * glyph_slot->bitmap.rows);
                
                for (u32 y = 0; y < glyph_slot->bitmap.rows; ++y) {
                    copy(bitmap + glyph_slot->bitmap.pitch * y, glyph_slot->bitmap.buffer + (glyph_slot->bitmap.rows - y - 1) * glyph_slot->bitmap.pitch, glyph_slot->bitmap.pitch);
                }
                
                glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, glyph_slot->bitmap.width, glyph_slot->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, bitmap);
                free(allocator, bitmap);
#endif
                
                x += cast_v(s16, glyph_slot->bitmap.width) + 1;
                ++fill_count;
            }
            
            result.glyphs[c] = glyph;
        }
        
        estimated_area = estimated_area * 1.2f;
    } while (did_not_fit);
    
    
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
    
    const u8 *it = text, *end = one_past_last(text);
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