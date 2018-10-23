#if !defined PHONG_MATERIAL_H
#define PHONG_MATERIAL_H

#include "gl.h"
#include "render_batch.h"

struct Phong_Shader {
    GLuint program_object;
    union {
        struct {
            GLint u_camera_to_clip_projection;
            GLint u_world_to_camera_transform;
            GLint u_object_to_world_transform;
            GLint u_bone_transforms;
            GLint u_camera_world_position;
            GLint u_light_world_position;
            GLint u_texture;
        };
        GLint uniforms[7];
    };
    u32 bone_count;
    
    bool debug_is_bound;
    bool debug_uniform_states[7];
};

struct Phong_Material {
    Bind_Render_Material_Function bind_material;
    Phong_Shader *shader;
    GLuint texture_object;
    mat4f *camera_to_clip_projection;
    mat4x3f *world_to_camera_transform;
    mat4x3f *object_to_world_transform;
    mat4x3f *bone_transforms;
};

const Vertex_Attribute_Info Phong_Vertex_Attribute_Infos[] = {
    { Vertex_Position_Index, 3, GL_FLOAT, GL_FALSE },
    { Vertex_Normal_Index,   3, GL_FLOAT, GL_FALSE },
    { Vertex_UV_Index,       2, GL_FLOAT, GL_FALSE },
    { Vertex_Bone_Index,     4, GL_UNSIGNED_BYTE, GL_FALSE },
    { Vertex_Bone_Weight,    4, GL_FLOAT, GL_FALSE },
};

const u32 Phong_Vertex_Attribute_Info_Count = 3;
const u32 Phong_Vertex_Animated_Attribute_Info_Count = ARRAY_COUNT(Phong_Vertex_Attribute_Infos);

#pragma pack(push, 1)
struct Phong_Vertex {
    vec3f position;
    vec3f normal;
    vec2f uv;
};

struct Phong_Vertex_Animated {
    Phong_Vertex vertex;
    u8 bone_indices[4];
    float bone_weights[4];
};
#pragma pack(pop)

inline void bind_phong_shader(Phong_Shader *shader) {
    glUseProgram(shader->program_object);
    glUniform1i(shader->u_texture, 0);
    
    shader->debug_is_bound = true;
    reset(shader->debug_uniform_states, sizeof(shader->debug_uniform_states), 0);
}

inline void set_camera_to_clip_projection(Phong_Shader *shader, mat4f camera_to_clip_projection) {
    assert(shader->debug_is_bound);
    shader->debug_uniform_states[&shader->u_camera_to_clip_projection - shader->uniforms] = true;
    glUniformMatrix4fv(shader->u_camera_to_clip_projection, 1, GL_FALSE, camera_to_clip_projection);
}

inline void set_world_to_camera_transform(Phong_Shader *shader, mat4x3f world_to_camera_transform) {
    assert(shader->debug_is_bound);
    shader->debug_uniform_states[&shader->u_world_to_camera_transform - shader->uniforms] = true;
    glUniformMatrix4x3fv(shader->u_world_to_camera_transform, 1, GL_FALSE, world_to_camera_transform);
}

inline void set_texture(u32 texture_slot, GLuint texture_object) {
    glActiveTexture(GL_TEXTURE0 + texture_slot);
    glBindTexture(GL_TEXTURE_2D, texture_object);
}

inline void set_diffuse_texture(Phong_Shader *shader, GLuint texture_object) {
    assert(shader->debug_is_bound);
    shader->debug_uniform_states[&shader->u_texture - shader->uniforms] = true;
    set_texture(0, texture_object);
}

inline void set_object_to_world_transform(Phong_Shader *shader, mat4x3f object_to_world_transform) {
    assert(shader->debug_is_bound);
    shader->debug_uniform_states[&shader->u_object_to_world_transform - shader->uniforms] = true;
    glUniformMatrix4x3fv(shader->u_object_to_world_transform, 1, GL_FALSE, object_to_world_transform);
}

inline void set_light_position(Phong_Shader *shader, vec3f position) {
    assert(shader->debug_is_bound);
    shader->debug_uniform_states[&shader->u_light_world_position - shader->uniforms] = true;
    glUniform3fv(shader->u_light_world_position, 1, position);
}

void bind_phong_material(any new_material_pointer, any old_material_pointer) {
    Phong_Material *new_material = CAST_P(Phong_Material, new_material_pointer);
    
    if (!old_material_pointer) {
        glUseProgram(new_material->shader->program_object);
        glActiveTexture(GL_TEXTURE0 + 0);
        glUniform1i(new_material->shader->u_texture, 0);
        
        glUniformMatrix4fv(new_material->shader->u_camera_to_clip_projection, 1, GL_FALSE, *new_material->camera_to_clip_projection);
        
        glUniformMatrix4x3fv(new_material->shader->u_world_to_camera_transform, 1, GL_FALSE,
                             *new_material->world_to_camera_transform);
    }
    
    glBindTexture(GL_TEXTURE_2D, new_material->texture_object);
    
    GLint u_bone_transform = new_material->shader->u_bone_transforms;
    for (u32 i = 0; i < new_material->shader->bone_count; ++i)
        glUniformMatrix4fv(u_bone_transform++, 1, GL_FALSE,  new_material->bone_transforms[i]);
    
    glUniformMatrix4x3fv(new_material->shader->u_object_to_world_transform, 1, GL_FALSE, *new_material->object_to_world_transform);
}

Phong_Shader make_phong_shader(string vertex_shader_source, string fragment_shader_source, Memory_Allocator *temporary_allocator)
{
    Shader_Attribute_Info attributes[] = {
        { Vertex_Position_Index, "a_position" },
        { Vertex_Normal_Index,   "a_normal" },
        { Vertex_UV_Index,       "a_uv" },
        { Vertex_Bone_Index,     "a_bone_indices" },
        { Vertex_Bone_Weight,    "a_bone_weights" }
    };
    
    const char *uniform_names[] = {
        "u_camera_to_clip_projection",
        "u_world_to_camera_transform",
        "u_object_to_world_transform",
        "u_bone_transforms",
        "u_camera_world_position",
        "u_light_world_position",
        "u_texture",
    };
    
    Phong_Shader result;
    assert(ARRAY_COUNT(uniform_names) == ARRAY_COUNT(result.uniforms));
    
    result.bone_count = 0;
    result.program_object = make_shader_program(vertex_shader_source, fragment_shader_source, ARRAY_WITH_COUNT(attributes), uniform_names, ARRAY_WITH_COUNT(result.uniforms), temporary_allocator);
    
    return result;
}

#endif // PHONG_MATERIAL_H