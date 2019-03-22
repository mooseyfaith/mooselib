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

#define Template_Array_Name      Vertex_Attributes
#define Template_Array_Data_Type Vertex_Attribute_Info
#include "template_array.h"

struct Vertex_Buffer {
    Vertex_Attributes attributes;
    u32 vertex_byte_count;
    GLuint object;
};

#define Template_Array_Name      Vertex_Buffers
#define Template_Array_Data_Type Vertex_Buffer
#include "template_array.h"

inline u32 vertex_attribute_byte_count_of(const Vertex_Attribute_Info *attribute_info) {
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

u32 vertex_byte_count_of(Vertex_Attributes attributes) {
    u32 byte_count = 0;
    
    for (u32 i = 0; i != attributes.count; ++i)
        byte_count+= vertex_attribute_byte_count_of(attributes.data + i);
    
    return byte_count;
}

void set_vertex_attributes(GLuint vertex_buffer_object, Vertex_Attributes attributes) {
    GLsizei stride = (GLsizei)vertex_byte_count_of(attributes);
    u8 *offset = 0;
    
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
    
    for (u32 i = 0; i != attributes.count; ++i) {
        glEnableVertexAttribArray(attributes[i].index);
        glVertexAttribPointer(
            attributes[i].index,
            attributes[i].length,
            attributes[i].type,
            attributes[i].do_normalize,
            stride,
            (const void *)offset
            );
        
        offset += vertex_attribute_byte_count_of(attributes.data + i);
    }
}

u32 vertex_attribute_byte_offet_of(Vertex_Attributes attributes, u32 index) {
    u32 byte_offset = 0;
    for (u32 i = 0; i != attributes.count; ++i) {
        if (attributes[i].index == index)
            return byte_offset;
        
        byte_offset += vertex_attribute_byte_count_of(attributes.data + i);
    }
    
    return -1;
}

u32 vertex_attribute_byte_offet_of(u32 *out_buffer_index, Vertex_Buffers buffers, u32 index) {
    
    for (u32 b = 0; b < buffers.count; b++) {
        u32 byte_offset = vertex_attribute_byte_offet_of(buffers[b].attributes, index);
        if (byte_offset != -1) {
            *out_buffer_index = b;
            return byte_offset;
        }
    }
    
    *out_buffer_index = -1;
    return -1;
}


#include "u8_buffer.h"

struct Indices {
    u8_buffer buffer;
    u32 index_byte_count;
};

INTERNAL void push_index(Indices *indices, u32 index) {
    switch (indices->index_byte_count) {
        case 1:
        push(&indices->buffer, cast_v(u8, index));
        break;
        
        case 2:
        *push_item(&indices->buffer, u16) = cast_v(u16, index);
        break;
        
        case 4:
        *push_item(&indices->buffer, u32) = cast_v(u32, index);
        break;
        
        default:
        UNREACHABLE_CODE;
    }
}

INTERNAL u32 index_of(Indices indices, u32 offset) {
    switch (indices.index_byte_count) {
        case 1:
        return indices.buffer[offset];
        
        case 2:
        //return *PACKED_ITEM_AT(&indices.buffer, u16, offset);
        //return *packed_front(indices.buffer, u16, offset);
        return *item_at(indices.buffer, u16, offset);
        
        case 4:
        //return *PACKED_ITEM_AT(&indices.buffer, u32, offset);
        //return *packed_front(indices.buffer, u32, offset);
        return *item_at(indices.buffer, u32, offset);
        
        default:
        UNREACHABLE_CODE;
        return -1;
    }
}

INTERNAL u32 index_count_of(Indices indices) {
    return indices.buffer.count / indices.index_byte_count;
}

inline void set_bytes_per_index_by_gl_type(Indices *indices, GLenum type) {
    switch (type) {	
        case GL_UNSIGNED_BYTE:
        indices->index_byte_count = sizeof(u8);
        break;
        
        case GL_UNSIGNED_SHORT:
        indices->index_byte_count = sizeof(u16);
        break;
        
        case GL_UNSIGNED_INT:
        indices->index_byte_count = sizeof(u32);
        break;
        
        default:
        UNREACHABLE_CODE;
    }
}

inline GLenum index_gl_type_of(Indices indices) {
    switch (indices.index_byte_count) {	
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


struct Batch {
    Vertex_Buffers vertex_buffers;
    GLuint vertex_array_object;
    GLuint index_buffer_object;
    GLuint index_type;
    u32 index_byte_count;
};

INTERNAL void init_batch_objects(Batch *batch) {
    glGenVertexArrays(1, &batch->vertex_array_object);
    glBindVertexArray(batch->vertex_array_object);
    
    for_array_item(buffer, batch->vertex_buffers) {
        glGenBuffers(1, &buffer->object);
        buffer->vertex_byte_count = vertex_byte_count_of(buffer->attributes);
        set_vertex_attributes(buffer->object, buffer->attributes);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    glGenBuffers(1, &batch->index_buffer_object);
    
    glBindVertexArray(0);
}

INTERNAL void upload_vertex_buffer(Vertex_Buffer buffer, u8_array data) {
    glBindBuffer(GL_ARRAY_BUFFER, buffer.object);
    glBufferData(GL_ARRAY_BUFFER, byte_count_of(data), data.data, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

INTERNAL void upload_index_buffer(Batch batch, u8_array data) {
    glBindVertexArray(batch.vertex_array_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.index_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, byte_count_of(data), data.data, GL_STREAM_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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

#define Template_Array_Name      Render_Batch_Commands
#define Template_Array_Data_Type Render_Batch_Command
#include "template_array.h"

#define Template_Array_Name      Render_Batch_Command_Buffer
#define Template_Array_Data_Type Render_Batch_Command
#define Template_Array_Is_Buffer
#include "template_array.h"


void push_render_command(Memory_Allocator *allocator, Render_Batch_Commands *commands, u32 material_index,
                         GLenum draw_mode,
                         u32 index_count)
{
    //Render_Batch_Command *last_command = back_or_null(batch->command_buffer);
    Render_Batch_Command *last_command = commands->count ? commands->data + commands->count - 1 : null;
    
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
        *grow(allocator, commands) = { material_index, draw_mode, index_count };
    }
}


struct Render_Batch {
    u8 *memory;
    u8_buffer vertex_buffer;
    Indices indices;
    Render_Batch_Command_Buffer command_buffer;
    
    GLuint vertex_array_object;
    GLuint vertex_buffer_object;
    GLuint index_buffer_object;
};

Render_Batch make_render_batch(Memory_Allocator *allocator, u32 vertex_buffer_size, u32 index_buffer_size,
                               GLenum index_type, u32 render_command_count,
                               Vertex_Attributes attributes)
{
    Render_Batch batch;
    
    Memory_Stack stack = make_memory_stack(allocator, vertex_buffer_size + index_buffer_size + render_command_count * sizeof(Render_Batch_Command) + 3 * (sizeof(usize) + MEMORY_MAX_ALIGNMENT));
    
    batch.memory = stack.buffer.data;
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
    
    set_vertex_attributes(batch.vertex_buffer_object, attributes);
    
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
    Render_Batch_Command *last_command = batch->command_buffer.count ? batch->command_buffer.data + batch->command_buffer.count - 1 : null;
    
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
    Render_Batch_Commands commands;
    GLuint vertex_array_object;
    GLuint index_buffer_object;
    GLuint index_type;
    u32 index_byte_count;
};

Static_Render_Batch flush(Render_Batch *batch) {
    Static_Render_Batch result;
    result.commands = { batch->command_buffer.data, batch->command_buffer.count };
    
    result.vertex_array_object = batch->vertex_array_object;
    result.index_buffer_object = batch->index_buffer_object;
    result.index_type          = index_gl_type_of(batch->indices);
    result.index_byte_count     = batch->indices.index_byte_count;
    
    update_buffer_object_data(GL_ARRAY_BUFFER, batch->vertex_buffer_object, &batch->vertex_buffer);
    update_buffer_object_data(GL_ELEMENT_ARRAY_BUFFER, batch->index_buffer_object, &batch->indices.buffer);
    batch->command_buffer.count = 0;
    
    return result;
}

Static_Render_Batch make_static_render_batch(Render_Batch *batch, Memory_Allocator *allocator) {
    Static_Render_Batch result;
    result.commands = ALLOCATE_ARRAY_INFO(allocator, Render_Batch_Command, batch->command_buffer.count);
    
    for (u32 i = 0; i < result.commands.count; ++i)
        result.commands[i] = batch->command_buffer[i];
    
    result.vertex_array_object = batch->vertex_array_object;
    result.index_buffer_object = batch->index_buffer_object;
    result.index_type = index_gl_type_of(batch->indices);
    result.index_byte_count = batch->indices.index_byte_count;
    
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
    for_array_item(it, batch->commands) {
        if (it->material_index != last_material_index) {
            Render_Material *material = materials[it->material_index];
            
            if (material->bind_material != last_material->bind_material)
                material->bind_material(material, NULL);
            else
                material->bind_material(material, last_material);
            
            last_material = material;
        }
        
        glDrawElements(it->draw_mode, cast_v(GLsizei, it->index_count), batch->index_type, cast_p(GLvoid, index_offset));
        index_offset += it->index_count * batch->index_byte_count;
    }
}

void draw(Static_Render_Batch batch, u32 material_index) {	
    glBindVertexArray(batch.vertex_array_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.index_buffer_object);
    
    usize index_offset = 0;
    for_array_item(it, batch.commands) {
        if (it->material_index == material_index)
            glDrawElements(it->draw_mode, cast_v(GLsizei, it->index_count), batch.index_type, cast_p(GLvoid, index_offset));
        
        index_offset += it->index_count * batch.index_byte_count;
    }
}


void draw(Batch batch, Render_Batch_Commands commands, u32 material_index) {	
    glBindVertexArray(batch.vertex_array_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.index_buffer_object);
    
    usize index_offset = 0;
    for_array_item(it, commands) {
        if (it->material_index == material_index)
            glDrawElements(it->draw_mode, cast_v(GLsizei, it->index_count), batch.index_type, cast_p(GLvoid, index_offset));
        
        index_offset += it->index_count * batch.index_byte_count;
    }
}


void draw(Render_Batch *batch, Render_Material **materials, u32 material_count) { 
    auto static_batch = flush(batch);
    draw(&static_batch, materials, material_count);
}

#endif
