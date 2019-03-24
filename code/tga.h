#if !defined(_ENGINE_TGA_H_)
#define _ENGINE_TGA_H_

#include "gl.h"
#include "platform.h"
//#include "image.h"

#pragma pack(push, 1)
struct TGA_Header {
    u8 id_length;
    u8 color_map_type;
    u8 image_type;
    
    struct Color_Map_Info {
        u16 first_index;
        u16 length;
        u8 bits_per_pixel;
    } color_map_info;
    
    struct Image_Info {
        u16 x; // lower left corner x
        u16 y; // lower left corner y
        u16 width;
        u16 height;
        u8 bits_per_pixel;
        struct {
            u8 rgba_mask : 4;
            u8 origin_is_right : 1;
            u8 origin_is_top : 1;
            u8 unused : 2;
        };
    } image_info;
};
#pragma pack(pop)

TGA_Header *tga_load(u8 **image_data, u8 **color_map, u8_array source) {
    if (!source.count)
        return NULL;
    
    // allocate wont change data
    // so calling it is like getting the pointer to the data
    u8_array data = { source.data, (u32)source.count };
    u8_array it = data;
    
    TGA_Header *header = next_item(&it, TGA_Header);
    
    // only type 1 and type 2 are supported for now
    if ((header->image_type != 1) && (header->image_type != 2))
        return NULL;
    
    u8 *id;
    if (header->id_length)
        id = next_items(&it, u8, header->id_length);
    else
        id = NULL;
    
    u32 color_map_size;
    
    if (header->image_type != 2 || header->color_map_type) {
        color_map_size = (header->color_map_info.length * header->color_map_info.bits_per_pixel) >> 3;
        
        if (color_map_size)
            *color_map = next_items(&it, u8, color_map_size);
        else
            *color_map = NULL;
    }
    else {
        *color_map = NULL;
        color_map_size = 0;
    }
    
    u32 image_size = (header->image_info.width * header->image_info.height * header->image_info.bits_per_pixel) >> 3;
    *image_data = next_items(&it, u8, image_size);
    
    return header;
}

u8 * tga_get_mapped_image_data(TGA_Header *header, u8 *image_data, u8 *color_map, Memory_Allocator *temporary_allocator)
{
    s32 y_advance;
    s32 y_end;
    if (!header->image_info.origin_is_top) {
        y_advance = -1;
        y_end = -1;
    }
    else {
        y_advance = 1;
        y_end = header->image_info.height;
    }
    
    s32 x_advance;
    s32 x_end;
    if (header->image_info.origin_is_right) {
        x_advance = -1;
        x_end = -1;
    } 
    else {
        x_advance = 1;
        x_end = header->image_info.width;
    }
    
    u8 image_bytes_per_pixel = header->image_info.bits_per_pixel >> 3;
    
    if ((header->image_type == 1) && header->color_map_type) {
        u8 map_bytes_per_pixel = header->color_map_info.bits_per_pixel >> 3;
        
        u8 *maped_image_data = ALLOCATE_ARRAY(temporary_allocator, u8, header->image_info.width * header->image_info.height * map_bytes_per_pixel);
        
        u32 index_buffer_count = header->image_info.width * header->image_info.height * image_bytes_per_pixel;
        Indices index_buffer = { { image_data, index_buffer_count, index_buffer_count }, image_bytes_per_pixel };
        
        u8 *maped_it = maped_image_data;
        for (s32 y = header->image_info.height - y_end * y_advance; y != y_end; y += y_advance) {
            for (s32 x = header->image_info.width - x_end * x_advance; x != x_end; x += x_advance) {
                
                u32 index = index_of(index_buffer, y * header->image_info.width + x);
                
                memcpy(maped_it, color_map + map_bytes_per_pixel * index, map_bytes_per_pixel);
                
                maped_it += map_bytes_per_pixel;
            }
        }
        
#if 0        
        for (u8 *it = image_data, *end = it + header->image_info.width * header->image_info.height, *maped_it = maped_image_data; it != end; it += image_bytes_per_pixel, maped_it += map_bytes_per_pixel) {
            
            u32 color;
            u32 index;
            switch (image_bytes_per_pixel) {
                case 1:
                index = *it;
                break;
                case 2:
                index = CAST_V(u32, *(it + 1)) << 8 | *it;
                break;
                default:
                // not supported
                assert(0);
            }
            
            memcpy(maped_it, color_map + map_bytes_per_pixel * index, map_bytes_per_pixel);
        }
#endif
        
        return maped_image_data;
    } else if (header->image_info.origin_is_right || header->image_info.origin_is_top) {
        u8 *maped_image_data = ALLOCATE_ARRAY(temporary_allocator, u8, header->image_info.width * header->image_info.height * image_bytes_per_pixel);
        
        u8 *maped_it = maped_image_data;
        for (s32 y = header->image_info.height - y_end * y_advance; y != y_end; y += y_advance) {
            for (s32 x = header->image_info.width - x_end * x_advance; x != x_end; x += x_advance) {
                
                memcpy(maped_it, image_data + (y * header->image_info.width + x) * image_bytes_per_pixel, image_bytes_per_pixel);
                
                maped_it += image_bytes_per_pixel;
            }
        }
        
        return maped_image_data;
    }
    
    return NULL;
}

void tga_get_gl_format(GLint *internal_format, GLenum *format, TGA_Header *header) {
    u32 bytes_per_pixel;
    if ((header->image_type == 1) && (header->color_map_type == 1))
        bytes_per_pixel = header->color_map_info.bits_per_pixel >> 3;
    else
        bytes_per_pixel = header->image_info.bits_per_pixel >> 3;
    
    switch (bytes_per_pixel) {
        case 1: {
            *internal_format = GL_R8;
            *format = GL_RED;
        } break;
        case 2: {
            *internal_format = GL_RG8;
            *format = GL_RG;
        } break;
        case 3: {
            *internal_format = GL_RGB8;
            *format = GL_BGR;
        } break;
        case 4: {
            *internal_format = GL_RGBA8;
            *format = GL_BGRA;
        } break;
        default: {
            // @Incomplete
            UNREACHABLE_CODE;
            *internal_format = -1;
            *format = GL_INVALID_ENUM;
        }
    }
}

TGA_Header * tga_load_sub_texture(OUTPUT Pixel_Rectangle *area, Texture *texture, s16 x, s16 y, u8_array source, Memory_Allocator *temporary_allocator) {
    u8 *image_data, *color_map;
    TGA_Header *header = tga_load(&image_data, &color_map, source);
    
    if (!header)
        return NULL;
    
    GLint internal_format;
    GLenum format;
    tga_get_gl_format(&internal_format, &format, header);
    assert(format == texture->format);
    
    glBindTexture(GL_TEXTURE_2D, texture->object);
    
    u8 *mapped_image_data = tga_get_mapped_image_data(header, image_data, color_map, temporary_allocator);
    if (mapped_image_data) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, header->image_info.width, header->image_info.height, format, GL_UNSIGNED_BYTE, mapped_image_data);
        
        free(temporary_allocator, mapped_image_data);
    }
    else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, header->image_info.width, header->image_info.height, format, GL_UNSIGNED_BYTE, image_data);
    }
    
    *area = { x, y, cast_v(s16, header->image_info.width), cast_v(s16, header->image_info.height) };
    
    return header;
}

TGA_Header * tga_load_texture(Texture *out_texture, u8_array source, Memory_Allocator *temporary_allocator, GLuint texture_object = 0, GLenum texture_target = GL_TEXTURE_2D, GLenum sub_texture_target = -1)
{
    u8 *image_data, *color_map;
    TGA_Header *header = tga_load(&image_data, &color_map, source);
    
    if (!header)
        return NULL;
    
    bool generate_texture_object = !texture_object;
    if (!texture_object)
        glGenTextures(1, &texture_object);
    
    glBindTexture(texture_target, texture_object);
    
    if (sub_texture_target == -1)
        sub_texture_target = texture_target;
    
    GLint internal_format;
    GLenum format;
    tga_get_gl_format(&internal_format, &format, header);
    
    switch (internal_format) {
        case GL_R8: {
            GLint swizzleMask[] = { GL_ONE, GL_ONE, GL_ONE, GL_RED };
            glTexParameteriv(sub_texture_target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        } break;
        case GL_RG8: {
            GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_GREEN };
            glTexParameteriv(sub_texture_target, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
        } break;
    }
    
    u8 *mapped_image_data = tga_get_mapped_image_data(header, image_data, color_map, temporary_allocator);
    
    if (mapped_image_data) {
        glTexImage2D(sub_texture_target, 0, internal_format, header->image_info.width, header->image_info.height, 0, format, GL_UNSIGNED_BYTE, mapped_image_data);
        
        free(temporary_allocator, mapped_image_data);
    }
    else {
        glTexImage2D(sub_texture_target, 0, internal_format, header->image_info.width, header->image_info.height, 0, format, GL_UNSIGNED_BYTE, image_data);
    }
    
    if (generate_texture_object) {
        glTexParameteri(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    *out_texture = {
        texture_object,
        (s16)header->image_info.width,
        (s16)header->image_info.height,
        format
    };
    
    return header;
}

bool tga_load_texture(OUTPUT Texture *out_texture, string file_path, Platform_Read_Entire_File_Function read_entire_file, Memory_Allocator *temporary_allocator, GLuint texture_object = 0, GLenum texture_target = GL_TEXTURE_2D,
                      GLenum sub_texture_target = -1) {
    u8_array source = read_entire_file(file_path, temporary_allocator);
    TGA_Header *header = tga_load_texture(out_texture, source, temporary_allocator, texture_object, texture_target, sub_texture_target);
    
    // also clears source, its the same data
    if (header)
        free(temporary_allocator, header);
    
    return header != NULL;
}

#if 0
Image tga_load_image(u8_array source, Memory_Allocator *allocator) {
    u8 *image_data, *color_map;
    TGA_Header *header = tga_load(&image_data, &color_map, source);
    
    u32 bytes_per_pixel;
    if ((header->image_type == 1) && (header->color_map_type == 1))
        bytes_per_pixel = header->color_map_info.bits_per_pixel >> 3;
    else
        bytes_per_pixel = header->image_info.bits_per_pixel >> 3;
    
    assert(bytes_per_pixel == 4);
    
    Image result;
    
    u8 *mapped_image_data = tga_get_mapped_image_data(header, image_data, color_map, allocator);
    if (mapped_image_data)
        result.pixels.data = CAST_P(rgba32, mapped_image_data);
    else
        result.pixels.data = CAST_P(rgba32, image_data);
    
    result.resolution.width = header->image_info.width;
    result.resolution.height = header->image_info.height;
    result.pixels.capacity = result.resolution.width * result.resolution.height;
    
    return result;
}
#endif

#endif
