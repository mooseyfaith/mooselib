#if !defined _ENGINE_RENDER_BATCH_H_
#define _ENGINE_RENDER_BATCH_H_

#include "gl.h"

enum Vertex_Attribut_Index {
    Vertex_Position_Index = 0,
    Vertex_Normal_Index,
    Vertex_Tangent_Index,
    Vertex_UV_Index,
    Vertex_Color_Index,
    Vertex_Bone_Index,
    Vertex_Bone_Weight,
    Vertex_First_Unused_Index,
};

struct Vertex_Attribute_Info {
    GLuint index;
    GLint length;
    GLenum type;
    GLboolean do_normalize;
    GLint padding_length;
};

inline u32 get_vertex_attribute_size(const Vertex_Attribute_Info *attribute_info) {
    switch (attribute_info->type) {
        case GL_FLOAT:
        return sizeof(GLfloat) * (attribute_info->length + attribute_info->padding_length);
        
        case GL_UNSIGNED_BYTE:
        return sizeof(GLubyte) * (attribute_info->length + attribute_info->padding_length);
        
        default:
        UNREACHABLE_CODE;
        return -1;
    }
}

u32 get_vertex_size(const Vertex_Attribute_Info *attribute_infos, u32 attribute_infos_length) {
    u32 vertex_size = 0;
    
    for (u32 i = 0; i != attribute_infos_length; ++i)
        vertex_size += get_vertex_attribute_size(attribute_infos + i);
    
    return vertex_size;
}

void set_vertex_attributes(GLuint vertex_buffer_object, const Vertex_Attribute_Info *attribute_infos, u32 attribute_infos_length) {
    GLsizei stride = (GLsizei)get_vertex_size(attribute_infos, attribute_infos_length);
    u8 *offset = 0;
    
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
    
    for (u32 i = 0; i != attribute_infos_length; ++i) {
        glEnableVertexAttribArray(attribute_infos[i].index);
        glVertexAttribPointer(
            attribute_infos[i].index,
            attribute_infos[i].length,
            attribute_infos[i].type,
            attribute_infos[i].do_normalize,
            stride,
            (const void *)offset
            );
        
        offset += get_vertex_attribute_size(attribute_infos + i);
    }
}

inline void set_bytes_per_index_by_gl_type(Indices *indices, GLenum type) {
    switch (type) {	
        case GL_UNSIGNED_BYTE:
        indices->bytes_per_index = sizeof(u8);
        break;
        
        case GL_UNSIGNED_SHORT:
        indices->bytes_per_index = sizeof(u16);
        break;
        
        case GL_UNSIGNED_INT:
        indices->bytes_per_index = sizeof(u32);
        break;
        
        default:
        UNREACHABLE_CODE;
    }
}

inline GLenum get_index_gl_type(Indices indices) {
    switch (indices.bytes_per_index) {	
        case sizeof(GLubyte):
        return GL_UNSIGNED_BYTE;
        
        case sizeof(GLushort):
        return GL_UNSIGNED_SHORT;
        
        case sizeof(GLuint):
        return GL_UNSIGNED_INT;
        
        default:
        UNREACHABLE_CODE;
        return GL_ZERO;
    }
}

void update_buffer_object_data(GLenum target, GLuint buffer_object, u8_buffer *buffer) {
    glBindBuffer(target, buffer_object);
    glBufferSubData(target, 0, buffer->count, buffer->data);
    buffer->count = 0;
}

typedef void(*Bind_Render_Material_Function)(any new_material, any old_material);

#define BIND_MATERIAL_DEC(name) void name(any new_material_pointer, any old_material_pointer)

struct Render_Material {
    Bind_Render_Material_Function bind_material;
};

struct Render_Batch_Command {
    u32 material_index;
    GLenum draw_mode;
    u32 index_count;
};


#define Template_Array_Type      Render_Batch_Command_Buffer
#define Template_Array_Data_Type Render_Batch_Command
#define Template_Array_Is_Buffer
#include "template_array.h"

struct Render_Batch {
    u8 *memory;
    u8_buffer vertex_buffer;
    Indices indices;
    Render_Batch_Command_Buffer command_buffer;
    
    GLuint vertex_array_object;
    GLuint vertex_buffer_object;
    GLuint index_buffer_object;
};

Render_Batch make_render_batch(u32 vertex_buffer_size, u32 index_buffer_size,
                               GLenum index_type, u32 render_command_count,
                               const Vertex_Attribute_Info *attribute_infos, u32 attribute_infos_count, Memory_Allocator *allocator)
{
    Render_Batch batch;
    
    Memory_Stack stack = make_memory_stack(allocator, vertex_buffer_size + index_buffer_size + render_command_count * sizeof(Render_Batch_Command) + 3 * (sizeof(usize) + MEMORY_MAX_ALIGNMENT));
    
    batch.memory = stack.buffer + 0;
    batch.vertex_buffer = ALLOCATE_ARRAY_INFO(&stack, u8, vertex_buffer_size);
    
    batch.indices.buffer = ALLOCATE_ARRAY_INFO(&stack, u8, index_buffer_size);
    set_bytes_per_index_by_gl_type(&batch.indices, index_type);
    
    batch.command_buffer = ALLOCATE_ARRAY_INFO(&stack, Render_Batch_Command, render_command_count);
    
    GLuint buffer_objects[2];
    glGenBuffers(ARRAY_COUNT(buffer_objects), buffer_objects);
    batch.vertex_buffer_object = buffer_objects[0];
    batch.index_buffer_object = buffer_objects[1];
    
    glGenVertexArrays(1, &batch.vertex_array_object);
    glBindVertexArray(batch.vertex_array_object);
    
    set_vertex_attributes(batch.vertex_buffer_object, attribute_infos, attribute_infos_count);
    
    glBufferData(GL_ARRAY_BUFFER, batch.vertex_buffer.capacity, batch.vertex_buffer.data, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.index_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, batch.indices.buffer.capacity, batch.indices.buffer.data, GL_DYNAMIC_DRAW);
    
    return batch;
}

void free_render_batch(Memory_Allocator *allocator, Render_Batch *batch) {
    free(allocator, batch->memory);
}

void push_render_command(Render_Batch *batch, u32 material_index,
                         GLenum draw_mode,
                         u32 index_count)
{
    //Render_Batch_Command *last_command = back_or_null(batch->command_buffer);
    Render_Batch_Command *last_command = batch->command_buffer.count ? batch->command_buffer + batch->command_buffer.count - 1 : null;
    
    if (last_command &&
        (last_command->material_index == material_index) &&
        (last_command->draw_mode == draw_mode) &&
        (last_command->draw_mode != GL_LINE_LOOP) &&
        (last_command->draw_mode != GL_TRIANGLE_FAN) &&
        (last_command->draw_mode != GL_TRIANGLE_STRIP)
        )
    {
        last_command->index_count += index_count;
    } else {
        *push(&batch->command_buffer) = { material_index, draw_mode, index_count };
    }
}

struct Render_Batch_Checkpoint {
    u32 vertex_buffer_count;
    u32 index_buffer_count;
    u32 command_buffer_count;
    u32 command_buffer_index_count;
};

Render_Batch_Checkpoint get_checkpoint(Render_Batch *batch) {
    Render_Batch_Checkpoint result;
    result.vertex_buffer_count        = batch->vertex_buffer.count;
    result.index_buffer_count         = batch->indices.buffer.count;
    result.command_buffer_count       = batch->command_buffer.count;
    result.command_buffer_index_count = batch->command_buffer[batch->command_buffer.count - 1].index_count;
    return result;
}

void revert_to_checkpoint(Render_Batch *batch, Render_Batch_Checkpoint *checkpoint) {
    assert(batch->vertex_buffer.count                                         >= checkpoint->vertex_buffer_count);
    batch->vertex_buffer.count                                                = checkpoint->vertex_buffer_count;
    
    assert(batch->indices.buffer.count                                        >= checkpoint->index_buffer_count);
    batch->indices.buffer.count                                               = checkpoint->index_buffer_count;
    
    assert(batch->command_buffer.count                                        >= checkpoint->command_buffer_count);
    batch->command_buffer.count                                               = checkpoint->command_buffer_count;
    
    assert(batch->command_buffer[batch->command_buffer.count - 1].index_count >= checkpoint->command_buffer_index_count);
    batch->command_buffer[batch->command_buffer.count - 1].index_count        = checkpoint->command_buffer_index_count;
}

struct Static_Render_Batch {
    Render_Batch_Command *commands;
    u32 command_count;
    GLuint vertex_array_object;
    GLuint index_buffer_object;
    GLuint index_type;
    u32 bytes_per_index;
};

Static_Render_Batch flush(Render_Batch *batch) {
    Static_Render_Batch result;
    result.commands = batch->command_buffer;
    result.command_count = batch->command_buffer.count;
    
    result.vertex_array_object = batch->vertex_array_object;
    result.index_buffer_object = batch->index_buffer_object;
    result.index_type          = get_index_gl_type(batch->indices);
    result.bytes_per_index     = batch->indices.bytes_per_index;
    
    update_buffer_object_data(GL_ARRAY_BUFFER, batch->vertex_buffer_object, &batch->vertex_buffer);
    update_buffer_object_data(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer_object, &batch->indices.buffer);
    batch->command_buffer.count = 0;
    
    return result;
}

Static_Render_Batch make_static_render_batch(Render_Batch *batch, Memory_Allocator *allocator) {
    Static_Render_Batch result;
    result.commands = ALLOCATE_ARRAY(allocator, Render_Batch_Command, batch->command_buffer.count);
    result.command_count = batch->command_buffer.count;
    
    for (u32 i = 0; i < result.command_count; ++i)
        result.commands[i] = batch->command_buffer[i];
    
    result.vertex_array_object = batch->vertex_array_object;
    result.index_buffer_object = batch->index_buffer_object;
    result.index_type = get_index_gl_type(batch->indices);
    result.bytes_per_index = batch->indices.bytes_per_index;
    
    update_buffer_object_data(GL_ARRAY_BUFFER, batch->vertex_buffer_object, &batch->vertex_buffer);
    update_buffer_object_data(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer_object, &batch->indices.buffer);
    batch->command_buffer.count = 0;
    
    return result;
}

void draw(Static_Render_Batch *batch, Render_Material **materials, u32 material_count) {	
    glBindVertexArray(batch->vertex_array_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer_object);
    
    // ensures last_material is allways a valid pointer, removes redundent check for NULL
    Render_Material empty_material = { NULL };
    Render_Material *last_material = &empty_material;
    u32 last_material_index = -1;
    
    u8 *index_offset = 0;
    for (Render_Batch_Command *it = batch->commands, *end = it + batch->command_count; it != end; ++it) {
        if (it->material_index != last_material_index) {
            Render_Material *material = materials[it->material_index];
            
            if (material->bind_material != last_material->bind_material)
                material->bind_material(material, NULL);
            else
                material->bind_material(material, last_material);
            
            last_material = material;
        }
        
        glDrawElements(it->draw_mode, CAST_V(GLsizei, it->index_count), batch->index_type, CAST_P(GLvoid, index_offset));
        index_offset += it->index_count * batch->bytes_per_index;
    }
}

void draw(Static_Render_Batch *batch, u32 material_index) {	
    glBindVertexArray(batch->vertex_array_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer_object);
    
    usize index_offset = 0;
    for (Render_Batch_Command *it = batch->commands,
         *end = it + batch->command_count;
         it != end;
         ++it
         )
    {
        if (it->material_index == material_index)
            glDrawElements(it->draw_mode, CAST_V(GLsizei, it->index_count), batch->index_type, CAST_P(GLvoid, index_offset));
        
        index_offset += it->index_count * batch->bytes_per_index;
    }
}

void draw(Render_Batch *batch, Render_Material **materials, u32 material_count) { 
    auto static_batch = flush(batch);
    draw(&static_batch, materials, material_count);
}

#endif
