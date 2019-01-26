#if !defined RENDER_QUEUE_H
#define RENDER_QUEUE_H

#include "render_batch.h"

enum Render_Queue_Command_Kind {
    Render_Queue_Command_Kind_Draw,
    Render_Queue_Command_Kind_Bind_Texture,
};

struct Render_Queue_Command {
    u32 kind;
    
    union {
        struct {
            GLenum mode;
            u32 vertex_count;
            u32 index_count;
        } draw;
        
        struct {
            Texture *texture;
            u32 index;
        } bind_texture;
    };
};

struct Render_Queue {
    Memory_Allocator *allocator;
    u8_array data;
    GLuint vertex_array_object;
    union {
        struct { GLuint vertex_buffer_object, index_buffer_object; };
        GLuint buffer_objects[2];
    };
    
    u8 vertex_byte_count;
};

Render_Queue make_render_queue(Memory_Allocator *allocator,Vertex_Attribute_Info *attribute_infos, usize attribute_info_count)
{
    Render_Queue queue = {};
    queue.allocator = allocator;
    
    glGenVertexArrays(1, &queue.vertex_array_object);
    glGenBuffers(COUNT_WITH_ARRAY(queue.buffer_objects));
    
    glBindVertexArray(queue.vertex_array_object);
    
    set_vertex_attributes(queue.vertex_buffer_object, attribute_infos, attribute_info_count);
    queue.vertex_byte_count = get_vertex_byte_count(attribute_infos, attribute_info_count);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return queue;
}

void upload(Render_Queue *queue)
{
    if (!queue->data.count)
        return;
    
    {
        u8_array vertices = {};
        
        auto it = queue->data;
        
        while (it.count)
        {
            auto command = next_item(&it, Render_Queue_Command);
            
            switch (command->kind) {
                case Render_Queue_Command_Kind_Draw:
                {
                    usize byte_count = queue->vertex_byte_count * command->draw.vertex_count;
                    
                    auto dest = grow(queue->allocator, &vertices, byte_count);
                    auto src = advance(&it, byte_count);
                    copy(dest, src, byte_count);
                    
                    next_items(&it, u32, command->draw.index_count);
                } break;
                
                case Render_Queue_Command_Kind_Bind_Texture:
                break;
                
                default:
                UNREACHABLE_CODE;
            }
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, queue->vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, byte_count_of(vertices), vertices.data, GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        free(queue->allocator, &vertices);
    }
    
    {
        u32_array indices = {};
        
        auto it = queue->data;
        u32 vertex_offset = 0;
        while (it.count)
        {
            auto command = next_item(&it, Render_Queue_Command);
            
            switch (command->kind) {
                case Render_Queue_Command_Kind_Draw:
                {
                    advance(&it, queue->vertex_byte_count * command->draw.vertex_count);
                    
                    auto src = next_items(&it, u32, command->draw.index_count);
                    
                    for (u32 i = 0; i < command->draw.index_count; i++)
                        src[i] += vertex_offset;
                    
                    auto dest = grow(queue->allocator, &indices, command->draw.index_count);
                    copy(dest, src, command->draw.index_count * sizeof(u32));
                    
                    vertex_offset += command->draw.vertex_count;
                } break;
                
                case Render_Queue_Command_Kind_Bind_Texture:
                break;
                
                default:
                UNREACHABLE_CODE;
            }
        }
        
        glBindVertexArray(queue->vertex_array_object);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, queue->index_buffer_object);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, byte_count_of(indices), indices.data, GL_STREAM_DRAW);
        
        free(queue->allocator, &indices);
    }
}

void render(Render_Queue *queue)
{
    glBindVertexArray(queue->vertex_array_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, queue->index_buffer_object);
    
    auto it = queue->data;
    usize index_offset = 0;
    
    Render_Queue_Command current_draw_command;
    current_draw_command.draw.mode = -1;
    
    while (it.count)
    {
        auto command = next_item(&it, Render_Queue_Command);
        
        switch (command->kind) {
            case Render_Queue_Command_Kind_Draw:
            {
                if (current_draw_command.draw.mode == -1) {
                    current_draw_command = *command;
                } else if (current_draw_command.draw.mode == command->draw.mode) {
                    current_draw_command.draw.vertex_count += command->draw.vertex_count;
                    current_draw_command.draw.index_count  += command->draw.index_count;
                }
                else {
                    glDrawElements(current_draw_command.draw.mode, current_draw_command.draw.index_count, GL_UNSIGNED_INT, cast_p(GLvoid, index_offset));
                    index_offset += current_draw_command.draw.index_count * sizeof(u32);
                    current_draw_command = *command;
                }
                
                advance(&it, queue->vertex_byte_count * command->draw.vertex_count);
                next_items(&it, u32, command->draw.index_count);
            } break;
            
            case Render_Queue_Command_Kind_Bind_Texture:
            {
                if (current_draw_command.draw.mode != -1) {
                    glDrawElements(current_draw_command.draw.mode, current_draw_command.draw.index_count, GL_UNSIGNED_INT, cast_p(GLvoid, index_offset));
                    index_offset += current_draw_command.draw.index_count * sizeof(u32);
                    current_draw_command.draw.mode = -1;
                }
                
                glActiveTexture(GL_TEXTURE0 + command->bind_texture.index);
                glBindTexture(GL_TEXTURE_2D, command->bind_texture.texture->object);
            } break;
            
            default:
            UNREACHABLE_CODE;
        }
    }
    
    if (current_draw_command.draw.mode != -1)
        glDrawElements(current_draw_command.draw.mode, current_draw_command.draw.index_count, GL_UNSIGNED_INT, cast_p(GLvoid, index_offset));
}

void clear(Render_Queue *queue)
{
    //glBindVertexArray(0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    free(queue->allocator, &queue->data);
    queue->data = {};
}

void upload_render_and_clear(Render_Queue *queue)
{
    upload(queue);
    render(queue);
    clear(queue);
}

#endif // RENDER_QUEUE_H