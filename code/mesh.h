#if !defined _ENGINE_MESH_H_
#define _ENGINE_MESH_H_

#include "render_batch.h"
#include "mo_string.h"
#include "dictionary.h"

struct Bone {
    string name;
    
    quatf rotation;
    vec3f translation;
    vec3f scale;
    
    f32 translation_total_blend;
    f32 scale_total_blend;
    
    mat4x3 pre_transform;
    mat4x3 post_transform;
    mat4x3 animated_transform;
    
    Bone *parent;
};

struct Sampled_Bone_Animation {
    struct Bone_Info {
        bool animateTranslation;
        bool animateRotation;
        bool animateScale;
        Bone *bone;
    } *bone_infos;
    u32 bone_info_count;
    
    //f32 frame_tick_time;
    u32 frame_rate;
    u32 key_frame_count;
    u32 key_frame_stride;
    f32 *key_frame_data;
};

#if 0
struct Mesh {
    GLuint vao;
    GLuint ibo;
    GLenum index_type;
    
    u32 vertex_buffer_count;
    struct Vertex_Buffer {
        GLuint object;
        u32 vertex_attribute_info_count;
        Vertex_Attribute_Info *vertex_attribute_infos;
    } *vertex_buffers;
    
    u32 render_command_count;
    struct Render_Command {
        // Render_Material_Header *material
        GLenum draw_mode;
        u32 index_offset;
        u32 index_count;
    } *render_commands;
    
    u32 bone_count;
    Bone *bones;
};
#endif

static inline void skip_white_space(string *it) {
    const char *_all_white_spaces = " \t\n\r";
    const string all_white_spaced = make_string(_all_white_spaces);
    
    const char *_new_line = "\n";
    const string new_line = make_string(_new_line);
    
    LOOP {
        string skipped = skip_set(it, all_white_spaced);
        
        if (it->data[0] == '#') { // ignore comment lines
            advance(it);
            skip_until_first_in_set(it, new_line);
            continue;			
        }
        
        if (!skipped.count)
            break;
    }
}

#define MESH_PARSE_STRING_DEC(text) static string const string_ ## text = S(# text)
#define MESH_PARSE_STRING(text) string_ ## text

struct Mesh {
    struct Vertex_Buffer {
        Vertex_Attribute_Info *vertex_attribute_infos;
        u32 vertex_attribute_info_count;
        u32 vertex_stride;
        GLuint object;
    } *vertex_buffers;
    u32 vertex_buffer_count;
    
    Bone *bones;
    u32 bone_count;
    
    Static_Render_Batch batch;
};

Mesh make_mesh(string mesh_source, Memory_Allocator *allocator, OUTPUT u8_array **out_vertex_buffers = NULL, OUTPUT u32 *out_vertex_count = NULL, OUTPUT Indices *out_indices = NULL)
{
    Mesh mesh;
    
    string it = mesh_source;
    
    skip_white_space(&it);
    
    skip(&it, S("vertex_buffers")); skip_white_space(&it);
    
    mesh.vertex_buffer_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
    
    mesh.vertex_buffers = ALLOCATE_ARRAY(allocator, Mesh::Vertex_Buffer, mesh.vertex_buffer_count);
    u32 vertex_buffer_count = 0;
    
    if (out_vertex_buffers)
        *out_vertex_buffers = ALLOCATE_ARRAY(allocator, u8_array, mesh.vertex_buffer_count);
    
    skip(&it, S("{")); skip_white_space(&it);
    
    glGenVertexArrays(1, &mesh.batch.vertex_array_object);
    glBindVertexArray(mesh.batch.vertex_array_object);
    
    u32 last_vertex_count = 0;
    
    // parse all vertex buffers
    while (it.count) {
        // all vertex buffers parsed
        if (try_skip(&it, S("}"))) { 
            skip_white_space(&it);
            break;
        }
        
        assert(vertex_buffer_count < mesh.vertex_buffer_count);
        
        if (try_skip(&it, S("vertex_buffer"))) {
            skip_white_space(&it);
            
            mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_info_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
            mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_infos = ALLOCATE_ARRAY(allocator, Vertex_Attribute_Info, mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_info_count);
            Vertex_Attribute_Info *vertex_attribute_infos = mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_infos;
            u32 vertex_attribute_info_count = 0;
            
            skip(&it, S("{")); skip_white_space(&it);
            
            while (it.count) {
                if (try_skip(&it, S("}"))) {
                    skip_white_space(&it);
                    break;
                }
                
                assert(vertex_attribute_info_count < mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_info_count);
                
                if (try_skip(&it, S("position"))) {
                    skip_white_space(&it);
                    vertex_attribute_infos[vertex_attribute_info_count].index = Vertex_Position_Index;
                }
                else if (try_skip(&it, S("normal"))) {
                    skip_white_space(&it);
                    vertex_attribute_infos[vertex_attribute_info_count].index = Vertex_Normal_Index;
                }
                else if (try_skip(&it, S("tangent"))) {
                    skip_white_space(&it);
                    vertex_attribute_infos[vertex_attribute_info_count].index = Vertex_Tangent_Index;
                }
                else if (try_skip(&it, S("uv0"))) {
                    skip_white_space(&it);
                    vertex_attribute_infos[vertex_attribute_info_count].index = Vertex_UV_Index;
                }
                else if (try_skip(&it, S("color"))) {
                    skip_white_space(&it);
                    vertex_attribute_infos[vertex_attribute_info_count].index = Vertex_Color_Index;
                }
                else if (try_skip(&it, S("bone_index"))) {
                    skip_white_space(&it);
                    vertex_attribute_infos[vertex_attribute_info_count].index = Vertex_Bone_Index;
                }
                else if (try_skip(&it, S("bone_weight"))) {
                    skip_white_space(&it);
                    vertex_attribute_infos[vertex_attribute_info_count].index = Vertex_Bone_Weight;
                }
                else {
                    assert(0);  // vertex attribute index not supported or implemented yet
                }
                
                vertex_attribute_infos[vertex_attribute_info_count].length = CAST_V(GLint, PARSE_UNSIGNED_INTEGER(&it, 32));
                skip_white_space(&it);
                
                if (try_skip(&it, S("f32"))) {
                    vertex_attribute_infos[vertex_attribute_info_count].type = GL_FLOAT;
                }
                else if (try_skip(&it, S("u8"))) {
                    vertex_attribute_infos[vertex_attribute_info_count].type = GL_UNSIGNED_BYTE;
                }
                else {
                    assert(0); // vertex attribute type not supported or implemented yet
                }
                skip_white_space(&it);
                
                vertex_attribute_infos[vertex_attribute_info_count].do_normalize = parse_unsigned_integer(&it, 1) ? GL_TRUE : GL_FALSE;
                skip_white_space(&it);
                
                vertex_attribute_infos[vertex_attribute_info_count].padding_length = CAST_V(GLint, PARSE_UNSIGNED_INTEGER(&it, 32));
                skip_white_space(&it);
                
                ++vertex_attribute_info_count;
            }
            
            u32 vertex_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
            skip(&it, S("{")); skip_white_space(&it);
            
            // make shure last vertex buffer had the same vertex_count
            assert(!last_vertex_count || (last_vertex_count == vertex_count));
            last_vertex_count = vertex_count;
            
            if (out_vertex_count)
                *out_vertex_count = vertex_count;
            
            mesh.vertex_buffers[vertex_buffer_count].vertex_stride = get_vertex_size(mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_infos, mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_info_count);
            
            u32 data_length = vertex_count * mesh.vertex_buffers[vertex_buffer_count].vertex_stride;
            u8_buffer buffer = ALLOCATE_ARRAY_INFO(allocator, u8, data_length);
            
            while (it.count) {
                if (try_skip(&it, S("}"))) {
                    skip_white_space(&it);
                    break;
                }
                
                if (try_skip(&it, S("u8"))) {
                    skip_white_space(&it);
                    
                    u32 index_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
                    assert(index_count <= 4);
                    
                    for (u32 i = 0; i < index_count; ++i) {
                        u8 value = PARSE_UNSIGNED_INTEGER(&it, 8); skip_white_space(&it);
                        *push(&buffer) = value;
                    }
                    
                    for (u32 i = index_count; i < 4; ++i)
                        *push(&buffer) = 0;
                    
                    continue;
                }
                
                f32 value = CAST_V(f32, parse_f64(&it)); skip_white_space(&it);
                pack_item(&buffer, value);
            }
            
            assert(buffer.count == buffer.capacity);
            
            glGenBuffers(1, &mesh.vertex_buffers[vertex_buffer_count].object);
            glBindBuffer(GL_ARRAY_BUFFER, mesh.vertex_buffers[vertex_buffer_count].object);
            glBufferData(GL_ARRAY_BUFFER, data_length, buffer.data, GL_STATIC_DRAW);
            
            set_vertex_attributes(
                mesh.vertex_buffers[vertex_buffer_count].object,
                mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_infos,
                mesh.vertex_buffers[vertex_buffer_count].vertex_attribute_info_count
                );
            
            if (out_vertex_buffers)
                (*out_vertex_buffers)[vertex_buffer_count] = {buffer.data, buffer.count };
            else
                free(allocator, buffer.data);
            
            ++vertex_buffer_count;
        } // one vertex buffer
    } // all vertex buffers
    
    skip(&it, S("indices")); skip_white_space(&it);
    
    u32 index_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
    skip(&it, S("{")); skip_white_space(&it);
    
    // opengl driver does not like such small indices ...
#if 0    
    if (last_vertex_count < (1 << 8)) {
        mesh.batch.index_type      = GL_UNSIGNED_BYTE;
        mesh.batch.index_type_size = 1;
    }
    else
#endif
    
        if (last_vertex_count < (1 << 16)) {
        mesh.batch.index_type      = GL_UNSIGNED_SHORT;
        mesh.batch.bytes_per_index = 2;
    } 
    else {
        mesh.batch.index_type      = GL_UNSIGNED_INT;
        mesh.batch.bytes_per_index = 4;
    }
    
    Indices indices;
    indices.bytes_per_index = mesh.batch.bytes_per_index;
    indices.buffer    = ALLOCATE_ARRAY_INFO(allocator, u8, index_count * indices.bytes_per_index);
    
    while (it.count) {
        if (try_skip(&it, S("}"))) {
            skip_white_space(&it);
            break;
        }
        
        push_index(&indices, CAST_V(u32, parse_unsigned_integer(&it, indices.bytes_per_index << 3)));
        skip_white_space(&it);
    }
    
    glGenBuffers(1, &mesh.batch.index_buffer_object);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.batch.index_buffer_object);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)index_count * indices.bytes_per_index, indices.buffer.data, GL_STATIC_DRAW);
    
    if (out_indices)
        *out_indices = indices;
    else
        free(allocator, indices.buffer.data);
    
    if (try_skip(&it, S("bones"))) {
        skip_white_space(&it);
        u32 bone_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
        
        mesh.bones = ALLOCATE_ARRAY(allocator, Bone, bone_count);
        mesh.bone_count = bone_count;
        
        Bone *bone_it = mesh.bones;
        Bone *bone_end = bone_it + bone_count;
        
        skip(&it, S("{")); skip_white_space(&it);
        while (it.count) {
            if (try_skip(&it, S("}"))) {
                skip_white_space(&it);
                break;
            }
            
            assert(bone_it != bone_end);
            
            skip(&it, S("bone")); skip_white_space(&it);
            
            bone_it->name = parse_quoted_string(&it); skip_white_space(&it);
            
            if (try_skip(&it, S("root")))
                bone_it->parent = NULL;
            else {
                skip(&it, S("parent"));
                string parent_name = parse_quoted_string(&it);
                
                Bone *parent = NULL;
                for (Bone *parent_it = mesh.bones; parent_it != bone_it; ++parent_it) {
                    if (parent_it->name == parent_name) {
                        parent = parent_it;
                        break;
                    }
                }
                assert(parent);
                
                bone_it->parent = parent;
            }
            skip_white_space(&it);
            
            f32 *transform_it = bone_it->post_transform;
            f32 *transform_end = transform_it + 12;
            while (transform_it != transform_end) {
                assert(it.count);
                *transform_it++ = CAST_V(f32, parse_f64(&it)); skip_white_space(&it);
            }
            
            //if (bone_it->parent) {
            //bone_it->post_transform = bone_it->parent->pre_transform * bone_it->post_transform;
            
            //bone_it->pre_transform = bone_it->parent->post_transform * make_inverse_transform(bone_it->post_transform);
            //}
            //else {
            
            //TODO add real inverse
            //bone_it->pre_transform = make_inverse_transform(bone_it->post_transform);
            
            //}
            
            skip_white_space(&it);
            ++bone_it;
        }
    }
    else {
        mesh.bone_count = 0;
        mesh.bones = NULL;
    }
    
    skip(&it, S("draw_calls")); skip_white_space(&it);
    mesh.batch.command_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
    mesh.batch.commands = ALLOCATE_ARRAY(allocator, Render_Batch_Command, mesh.batch.command_count);
    u32 render_command_count = 0;
    
    skip(&it, S("{")); skip_white_space(&it);
    
    while (it.count) {
        if (try_skip(&it, S("}"))) {
            skip_white_space(&it);
            break;
        }
        
        assert(render_command_count < mesh.batch.command_count);
        
        if (try_skip(&it, S("triangles"))) {
            mesh.batch.commands[render_command_count].draw_mode = GL_TRIANGLES;
        }
        else {
            assert(0); // unsupported or implemented draw primitove type
        }
        skip_white_space(&it);
        
        // might be needed, but not used for now
        //mesh.render_commands[render_command_count].index_offset = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
        
        PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
        mesh.batch.commands[render_command_count].index_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
        mesh.batch.commands[render_command_count].material_index = 0;
        ++render_command_count;
    }
    
    return mesh;
}

Sampled_Bone_Animation animation_load_sampled(string source, Dictionary *bones, Memory_Allocator *allocator) {
    Sampled_Bone_Animation animation;
    
    string it = source;
    
    skip_white_space(&it);
    
    skip(&it, S("framerate")); skip_white_space(&it);
    u32 frame_rate = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
    
    skip(&it, S("animations")); skip_white_space(&it);
    u32 animation_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
    
    skip(&it, S("{")); skip_white_space(&it);
    
    while (it.count) {
        if (try_skip(&it, S("}"))) {
            skip_white_space(&it);
            break;
        }
        
        skip(&it, S("animation")); skip_white_space(&it);
        
        // ignore name for now		
        skip_until_first_in_set(&it, S("\""));
        advance(&it);
        skip_until_first_in_set(&it, S("\""));
        advance(&it);
        skip_white_space(&it);
        
        u32 bone_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
        
        animation.bone_infos = ALLOCATE_ARRAY(allocator, Sampled_Bone_Animation::Bone_Info, bone_count);
        animation.bone_info_count = bone_count;
        
        animation.frame_rate = frame_rate;
        
        Sampled_Bone_Animation::Bone_Info *bone_info_it = animation.bone_infos;
        Sampled_Bone_Animation::Bone_Info *bone_info_end = animation.bone_infos + bone_count;
        
        u32 frame_stride = 0;
        
        skip(&it, S("{")); skip_white_space(&it);
        
        while (it.count) {
            if (try_skip(&it, S("}"))) {
                skip_white_space(&it);
                break;
            }
            
            assert(bone_info_it != bone_info_end);
            
            // ignore name for now
            string bone_name = parse_quoted_string(&it);	skip_white_space(&it);
            
            bone_info_it->bone = GET_DICTIONARY_ENTRY(NULL, bones, bone_name, Bone);
            
            u32 channel_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
            
            skip(&it, S("{")); skip_white_space(&it);
            
            if (try_skip(&it, S("translation"))) {
                bone_info_it->animateTranslation = true;
                frame_stride += 3;
                skip_white_space(&it);
            }
            else {
                bone_info_it->animateTranslation = false;
            }
            
            if (try_skip(&it, S("rotation"))) {
                bone_info_it->animateRotation = true;
                frame_stride += 4;
                skip_white_space(&it);
            }
            else {
                bone_info_it->animateRotation = false;
            }
            
            if (try_skip(&it, S("scale"))) {
                bone_info_it->animateScale = true;
                frame_stride += 3;
                skip_white_space(&it);
            }
            else {
                bone_info_it->animateScale = false;
            }
            
            skip_white_space(&it);
            
            skip(&it, S("}")); skip_white_space(&it);
            
            ++bone_info_it;
        }
        
        skip(&it, S("frames")); skip_white_space(&it);
        u32 first_frame = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
        u32 frame_count = PARSE_UNSIGNED_INTEGER(&it, 32); skip_white_space(&it);
        
        animation.key_frame_count = frame_count;
        animation.key_frame_stride = frame_stride;
        animation.key_frame_data = ALLOCATE_ARRAY(allocator, f32, frame_count * frame_stride);
        
        f32 *data_it = animation.key_frame_data;
        f32 *data_end = data_it + frame_count * frame_stride;
        
        skip(&it, S("{")); skip_white_space(&it);
        
        while (it.count) {
            if (try_skip(&it, S("}"))) {
                skip_white_space(&it);
                break;
            }
            
            assert(data_it != data_end);
            *data_it++ = CAST_V(f32, parse_f64(&it));
            skip_white_space(&it);
        }
    }
    
    return animation;
    
}

void begin_blend(Bone *bones, u32 bone_count) {
    for (Bone *it = bones, *end = it + bone_count; it != end; ++it) {
        it->rotation = quatf{};
        it->translation = vec3f{};
        it->scale = vec3f{};
        it->translation_total_blend = 0.0f;
        it->scale_total_blend = 0.0f;
    }
}

void blend(Sampled_Bone_Animation *animation, f32 time, bool loop, f32 animation_blend) {
    f32 between_frame_blend = time * animation->frame_rate;
    u32 lower_frame = between_frame_blend;
    between_frame_blend -= (f32)lower_frame;
    
    if (loop) {
        lower_frame = (lower_frame % (animation->key_frame_count - 1));
    }
    else {
        if (lower_frame > animation->key_frame_count - 2) {
            lower_frame = animation->key_frame_count - 2;
            between_frame_blend = 1.0f;
        }
    }
    
    f32 *lower_frame_data = animation->key_frame_data + animation->key_frame_stride * lower_frame;
    f32 *upper_frame_data = animation->key_frame_data + animation->key_frame_stride * (lower_frame + 1);
    
    for (Sampled_Bone_Animation::Bone_Info *it = animation->bone_infos, *end = it + animation->bone_info_count; it != end; ++it) {
        if (it->animateTranslation) {
            vec3f a = { *lower_frame_data++, *lower_frame_data++, *lower_frame_data++ };
            vec3f b = { *upper_frame_data++, *upper_frame_data++, *upper_frame_data++ };
            
            it->bone->translation += linear_interpolation(a, b, between_frame_blend) * animation_blend;
            it->bone->translation_total_blend += animation_blend;
        }
        
        if (it->animateRotation) {
            quatf a = { *lower_frame_data++, *lower_frame_data++, *lower_frame_data++, *lower_frame_data++ };
            quatf b = { *upper_frame_data++, *upper_frame_data++, *upper_frame_data++, *upper_frame_data++ };
            
            // check quaternion neighbourhood
            f32 sign;
            if (dot(a, b) < 0.0f)
                sign = -1.0f;
            else
                sign = 1.0f;
            
            //it->bone->rotation += linear_interpolation(a, b, between_frame_blend) * animation_blend * sign;
            it->bone->rotation += (a * (1.0f - between_frame_blend) +  b * between_frame_blend * sign) * animation_blend;
        }
        
        if (it->animateScale) {
            vec3f a = { *lower_frame_data++, *lower_frame_data++, *lower_frame_data++ };
            vec3f b = { *upper_frame_data++, *upper_frame_data++, *upper_frame_data++ };
            
            it->bone->scale += linear_interpolation(a, b, between_frame_blend) * animation_blend;
            it->bone->scale_total_blend += animation_blend;
        }
    }
}

void end_blend(Bone *bones, u32 bone_count) {
    for (Bone *it = bones, *end = it + bone_count; it != end; ++it) {
        // normalize quaternions
        if (squared_length(it->rotation) == 0.0f)
            it->rotation = QUAT_IDENTITY;
        else
            it->rotation = normalize(it->rotation);
        
        if (it->translation_total_blend == 0.0f)
            it->translation = VEC3_ZERO;
        else
            it->translation /= it->translation_total_blend;
        
        if (it->scale_total_blend == 0.0f)
            it->scale = VEC3_ONE;
        else
            it->scale /= it->scale_total_blend;
        
#if 0
        mat4x3 local_transform =  make_transform(it->rotation, it->translation, it->scale) * it->pre_transform;
#else
        mat4x3 local_transform = make_transform(it->rotation, it->translation, it->scale);
#endif
#if 1
        if (it->parent)
            it->animated_transform = it->parent->animated_transform * local_transform;
        else
#endif
            it->animated_transform =local_transform;
    }
    
#if 1
    for (Bone *it = bones, *end = it + bone_count; it != end; ++it)
        it->animated_transform = it->post_transform * it->animated_transform * it->pre_transform;
#endif
}

#endif
