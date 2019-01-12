
#include <platform.h>
#include <memory_growing_stack.h>
#include <memory_c_allocator.h>
#include <ui_queue.h>

#include <immediate_render.cpp>

enum {
    Camera_Uniform_Block_Index = 0,
    Lighting_Uniform_Block_Index,
};

// all aliginged to vec4, since layout (std140) sux!!!
struct Camera_Uniform_Block
{
    mat4f camera_to_clip_projection;
    // actually a 4x3 but after each vec3 there is 1 f32 padding
    mat4f world_to_camera;
    vec3f camera_world_position;
    f32 padding0;
};

#define MAX_LIGHT_COUNT 10

struct Lighting_Uniform_Block {
    vec4f global_ambient_color;
    
    vec4f global_light_direction;
    vec4f global_light_diffuse_color;
    vec4f global_light_specular_color;
    
    vec4f diffuse_colors[MAX_LIGHT_COUNT];
    vec4f specular_colors[MAX_LIGHT_COUNT];
    vec4f world_positions_and_attenuations[MAX_LIGHT_COUNT];
    u32 count;
    u32 padding[3];
};

struct Debug_Camera {
    union { mat4x3f to_world, inverse_view_matrix; };
    
    vec3f alpha_axis, beta_axis;
    f32 alpha, beta;
    f32 beta_min, beta_max;
};

struct Default_State
{
    Memory_Growing_Stack transient_memory;
    Memory_Growing_Stack persistent_memory;
    Memory_Growing_Stack work_memory;
    Memory_Stack *worker_thread_stacks;
    
    Memory_Growing_Stack ui_memory;
    
    FT_MemoryRec_ font_allocator;
    FT_Library font_library;
    Font default_font;
    UI_Context ui;
    Immediate_Render_Context im;
    bool main_window_is_fullscreen;
    
    union {
        struct {
            GLuint camera_uniform_buffer_object, light_uniform_buffer_object;
        };
        
        GLuint uniform_buffer_objects[2];
    };
    
    struct {
        
#define UI_SHADER_UNIFORMS u_diffuse_texture
        
        union {
            struct { GLint UI_SHADER_UNIFORMS; };
            GLint uniforms[1];
        };
        
        GLuint program_object;
    } ui_shader;
    
    
    struct {
        GLuint program_object;
        
        union {
            
#define IM_SHADER_UNIFORMS \
            u_object_to_world_transform, \
            u_diffuse_texture
            
            struct { GLint IM_SHADER_UNIFORMS; };
            GLint uniforms[2];
        };
    } im_shader;
    
    struct {
        GLuint program_object;
        
        union {
            
#define PHONG_SHADER_UNIFORMS \
            u_object_to_world_transform, \
            u_gloss, \
            u_diffuse_color, \
            u_specular_color, \
            u_diffuse_texture, \
            u_normal_map
            
            struct { GLint PHONG_SHADER_UNIFORMS; };
            GLint uniforms[6];
        };
    } phong_shader;
    
    Texture blank_texture;
    Texture blank_normal_map;
    
    struct {
        union { mat4x3f to_world, inverse_view_matrix; };
        mat4x3f world_to_camera;
        
        mat4f to_clip_projection;
        mat4f clip_to_camera_projection;
    } camera;
    
    struct {
        Debug_Camera camera;
        mat4x3f backup_camera_to_world;
        
        f32 speed = 1.0f;
        f32 backup_speed = 1.0f;
        bool is_paused;
        bool is_active;
        bool use_game_controls;
        vec2f last_mouse_window_position;
        f32 camera_mouse_sensitivity;
        f32 camera_movement_speed;
    } debug;
};

void default_reload_global_functions(Platform_API *platform_api)
{
    PLATFORM_SYNC_ALLOCATORS();
    init_gl();
}

#define DEFAULT_STATE_INIT(type, state, platform_api) { \
    default_reload_global_functions(platform_api); \
    \
    auto persistent_memory = make_memory_growing_stack(&(platform_api)->allocator); \
    (state) = ALLOCATE(&persistent_memory.allocator, type); \
    (state)->persistent_memory = persistent_memory; \
    init(state, platform_api); \
}

GLuint load_shader(Default_State *state, Platform_API *platform_api, GLint *uniforms, u32 uniform_count, string file, string options, string uniform_names)
{
    string shader_source = platform_api->read_entire_file(file, &state->transient_memory.allocator);
    assert(shader_source.count);
    
    defer { free(&state->transient_memory.allocator, shader_source.data); };
    
    Shader_Attribute_Info attributes[] = {
        { Vertex_Position_Index, "a_position" },
        { Vertex_Normal_Index,   "a_normal" },
        { Vertex_Tangent_Index,  "a_tangent" },
        { Vertex_UV_Index,       "a_uv" },
        { Vertex_Color_Index,    "a_color" },
    };
    
    u8_buffer define_buffer = {};
    write(&state->transient_memory.allocator, &define_buffer, S("#version 150\n"));
    defer { free(&state->transient_memory.allocator, &define_buffer); };
    
    {
        string options_it = options;
        skip_set(&options_it, S(" \t\b\r\n\0"));
        
        while (options_it.count)
        {
            string name = get_identifier(&options_it, S("_"));
            skip_set(&options_it, S(" \t\b\r\n\0"));
            
            string value = skip_until_first_in_set(&options_it, S(",\0"), true);
            skip_set(&options_it, S(" \t\b\r\n\0"));
            write(&state->transient_memory.allocator, &define_buffer, S("#define % %\n"), f(name), f(value));
        }
    }
    
    string defines = { define_buffer.data, define_buffer.count };
    
    string vertex_shader_sources[] = {
        defines,
        S("#define VERTEX_SHADER\n"),
        shader_source,
    };
    
    string fragment_shader_sources[] = {
        defines,
        S("#define FRAGMENT_SHADER\n"),
        shader_source,
    };
    
    GLuint shader_objects[2];
    shader_objects[0] = make_shader_object(GL_VERTEX_SHADER, ARRAY_WITH_COUNT(vertex_shader_sources), &state->transient_memory.allocator);
    shader_objects[1] = make_shader_object(GL_FRAGMENT_SHADER, ARRAY_WITH_COUNT(fragment_shader_sources), &state->transient_memory.allocator);
    
    // leak!!
    // leak!!
    // leak!!
    if (!shader_objects[0] || !shader_objects[1]) {
        UNREACHABLE_CODE;
        return 0;
    }
    
    GLuint program_object = make_shader_program(ARRAY_WITH_COUNT(shader_objects), true, ARRAY_WITH_COUNT(attributes), &state->transient_memory.allocator);
    
    if (!program_object) {
        UNREACHABLE_CODE;
        return 0;
    }
    
    get_uniforms(uniforms, uniform_count, program_object, uniform_names, &state->transient_memory.allocator);
    
    GLint camera_uniform_block = glGetUniformBlockIndex(program_object, "Camera_Uniform_Block");
    if (camera_uniform_block != -1)
        glUniformBlockBinding(program_object, camera_uniform_block, Camera_Uniform_Block_Index);
    
    GLint lighting_uniform_block = glGetUniformBlockIndex(program_object, "Lighting_Uniform_Block");
    if (lighting_uniform_block != -1)
        glUniformBlockBinding(program_object, lighting_uniform_block, Lighting_Uniform_Block_Index);
    
    return program_object;
}


void debug_update_camera(Debug_Camera *camera) {
    quatf rotation = make_quat(camera->alpha_axis, camera->alpha);
    rotation = multiply(rotation, make_quat(camera->beta_axis, camera->beta));
    
    camera->to_world = make_transform(rotation, camera->to_world.translation);
}

void init(Default_State *state, Platform_API *platform_api, usize worker_thread_memory_count = MEGA(256))
{
    state->transient_memory = make_memory_growing_stack(&platform_api->allocator);
    
    {
        state->worker_thread_stacks = ALLOCATE_ARRAY(&state->persistent_memory.allocator, Memory_Stack, platform_api->worker_thread_count);
        for (u32 i = 0; i < platform_api->worker_thread_count; ++i)
            state->worker_thread_stacks[i] = make_memory_stack(&state->persistent_memory.allocator, worker_thread_memory_count / platform_api->worker_thread_count);
        
        state->work_memory = make_memory_growing_stack(&platform_api->allocator);
    }
    
    init_font_allocator(&state->font_allocator, C_Allocator);
    state->font_library = make_font_library(&state->font_allocator);
    
    state->default_font = make_font(state->font_library, S("C:/Windows/Fonts/consola.ttf"), 16, ' ', 256 - ' ', platform_api->read_entire_file, &state->persistent_memory.allocator);
    
    state->blank_texture = make_blank_texture();
    state->blank_normal_map = make_singe_color_texture({ 128, 128, 255 });
    
    
    state->ui_memory = make_memory_growing_stack(&platform_api->allocator);
    state->ui = make_ui_context(&state->ui_memory.allocator);
    
    {
        glGenBuffers(COUNT_WITH_ARRAY(state->uniform_buffer_objects));
        glBindBuffer(GL_UNIFORM_BUFFER, state->camera_uniform_buffer_object);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Camera_Uniform_Block), null, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, Camera_Uniform_Block_Index, state->camera_uniform_buffer_object);
        
        glBindBuffer(GL_UNIFORM_BUFFER, state->light_uniform_buffer_object);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Lighting_Uniform_Block), null, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, Lighting_Uniform_Block_Index, state->light_uniform_buffer_object);
        
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    
    state->ui_shader.program_object = load_shader(state, platform_api, ARRAY_WITH_COUNT(state->ui_shader.uniforms), S("shaders/transparent_textured.shader.txt"), S("WITH_VERTEX_COLOR, WITH_DIFFUSE_TEXTURE, ALPHA_THRESHOLD 0.025"), S(STRINGIFY(UI_SHADER_UNIFORMS)));
    
    state->im_shader.program_object = load_shader(state, platform_api, ARRAY_WITH_COUNT(state->im_shader.uniforms), S("shaders/phong.shader.txt"), S("WITH_VERTEX_COLOR"), S(STRINGIFY(IM_SHADER_UNIFORMS)));
    
    state->phong_shader.program_object = load_shader(state, platform_api, ARRAY_WITH_COUNT(state->phong_shader.uniforms), S("shaders/phong.shader.txt"), S("WITH_DIFFUSE_COLOR, MAX_LIGHT_COUNT 10"), S(STRINGIFY(PHONG_SHADER_UNIFORMS)));
    
    init(&state->im);
    
    state->camera.to_world = make_transform(make_quat(VEC3_X_AXIS, PIf * -0.25f));
    state->camera.to_world.translation = transform_direction(state->camera.to_world, vec3f{0, 0, 40});
    
    state->debug.speed = 1.0f;
    state->debug.backup_speed = 1.0f;
    
    state->debug.camera_movement_speed    = 50.0f;
    state->debug.camera_mouse_sensitivity = 2.0f * PIf / 2048.0f;;
    
    state->debug.camera.alpha_axis = VEC3_Y_AXIS;
    state->debug.camera.beta_axis = VEC3_X_AXIS;
    state->debug.camera.beta_min = -PIf * 0.5f;
    state->debug.camera.beta_max =  PIf * 0.5f;
    
    state->debug.camera.alpha = 0.0f;
    state->debug.camera.beta = PIf * -0.25f;
    debug_update_camera(&state->debug.camera);
    state->debug.camera.to_world.translation = transform_direction(state->debug.camera.to_world, vec3f{ 0, 0, 20.f });
}

bool default_begin_frame(Default_State *state, const Game_Input *input, Platform_API *platform_api, f64 *delta_seconds)
{
    clear(&state->transient_memory);
    
    for (auto it = platform_api->messages.head; it; it = it->next) {
        if (it->value->kind == Platform_Message_Kind_Reload) {
            default_reload_global_functions(platform_api);
            init_font_allocator(&state->font_allocator, cast_p(Memory_Allocator, state->font_allocator.user));
            break;
        }
    }
    
#if defined DEBUG_EDITOR
    
    if (was_pressed(input->keys[VK_F1])) {
        state->debug.is_active = !state->debug.is_active;
        
        if (state->debug.is_active)
            state->debug.backup_camera_to_world = state->camera.to_world;
        else
            state->camera.to_world = state->debug.backup_camera_to_world;
    }
    
    if (was_pressed(input->keys[VK_F5])) {
        state->debug.speed = MAX(1.0f / 16.0f, state->debug.speed * 0.5f);
        state->debug.backup_speed = 1.0f;
    }
    
    if (was_pressed(input->keys[VK_F6])) {
        state->debug.speed = MIN(128, state->debug.speed * 2.0f);
        state->debug.backup_speed = 1.0f;
    }
    
    if (was_pressed(input->keys[VK_F7])) {
        f32 temp = state->debug.backup_speed;
        state->debug.backup_speed = state->debug.speed;
        state->debug.speed = temp;
    }
    
    *delta_seconds *= state->debug.speed;
    
    // toggle game pause
    if (was_pressed(input->keys[VK_F2]))
        state->debug.is_paused = !state->debug.is_paused;
    
#endif
    
    // alt + F4 close application
    if (input->left_alt.is_active && was_pressed(input->keys[VK_F4]))
        return false;
    
    // toggle fullscreen, this may freez the app for about 5 seconds
    if (input->left_alt.is_active && was_pressed(input->keys[VK_RETURN]))
        state->main_window_is_fullscreen = !state->main_window_is_fullscreen;
    
    return true;
}

UI_Context * default_begin_ui(Default_State *state, const Game_Input *input, f64 delta_seconds, bool allways_update, Platform_Window window, Pixel_Rectangle *window_rect, Pixel_Rectangle new_window_rect, bool cursor_was_pressed, bool cursor_was_released, vec4f clear_color, f32 scale = 1.0f)
{
    bool do_update = !ARE_EQUAL(window_rect, &new_window_rect, sizeof(*window_rect));
    *window_rect = new_window_rect;
    
    Pixel_Dimensions render_resolution = window_rect->size; //get_auto_render_resolution(state->main_window_area.size, Reference_Resolution);
    
    if (render_resolution.width == 0 || render_resolution.height == 0) {
        return null;
    }
    
    set_auto_viewport(render_resolution, render_resolution, clear_color);
    
    auto ui = &state->ui;
    
    do_update &= ui_control(ui, { window.mouse_position, { 1, 1 } }, cursor_was_pressed, cursor_was_released);
    
    //if (!allways_update && !do_update)
    //return null;
    
    ui_clear(ui);
    
    ui_set_transform(ui, render_resolution, scale);
    ui_set_texture(ui, &state->blank_texture);
    ui_set_font(ui, &state->default_font);
    
    ui->transient_allocator = &state->transient_memory.allocator;
    
    draw_begin(&state->im, C_Allocator, MAT4X3_IDENTITY);
    
    state->camera.to_clip_projection = make_perspective_fov_projection(60.0f, width_over_height(render_resolution));
    state->camera.clip_to_camera_projection = make_inverse_perspective_projection(state->camera.to_clip_projection);
    
    glBindBuffer(GL_UNIFORM_BUFFER, state->camera_uniform_buffer_object);
    auto camera_block = cast_p(Camera_Uniform_Block, glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY));
    camera_block->camera_to_clip_projection = state->camera.to_clip_projection;
    
    for (u32 i = 0; i < 4; ++i)
        camera_block->world_to_camera.columns[i] = make_vec4(state->camera.world_to_camera.columns[i], 0.0f);
    
    camera_block->camera_world_position = state->camera.to_world.translation;
    
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
#if defined DEBUG_EDITOR
    // update debug camera
    if (state->debug.is_active) {
        if (was_pressed(input->keys[VK_F3]))
            state->debug.use_game_controls = !state->debug.use_game_controls;
        
        f32 debug_delta_seconds = delta_seconds / state->debug.speed;
        
        if (!state->debug.use_game_controls) {
            if (was_pressed(input->mouse.right))
                state->debug.last_mouse_window_position = window.mouse_position;
            else if (input->mouse.right.is_active) {
                vec2f mouse_delta =  window.mouse_position - state->debug.last_mouse_window_position;
                state->debug.last_mouse_window_position = window.mouse_position;
                
                state->debug.camera.alpha -= mouse_delta.x * state->debug.camera_mouse_sensitivity;
                
                state->debug.camera.beta -= mouse_delta.y * -state->debug.camera_mouse_sensitivity;
                state->debug.camera.beta = CLAMP(state->debug.camera.beta, state->debug.camera.beta_min, state->debug.camera.beta_max);
                
                debug_update_camera(&state->debug.camera);
            }
            
            vec3f direction = {};
            
            if (input->keys['W'].is_active)
                direction.z -= 1.0f;
            
            if (input->keys['S'].is_active)
                direction.z += 1.0f;
            
            if (input->keys['A'].is_active)
                direction.x -= 1.0f;
            
            if (input->keys['D'].is_active)
                direction.x += 1.0f;
            
            if (input->keys['Q'].is_active)
                direction.y -= 1.0f;
            
            if (input->keys['E'].is_active)
                direction.y += 1.0f;
            
            direction = normalize_or_zero(direction);
            
            state->debug.camera.to_world.translation +=  transform_direction(state->debug.camera.to_world, direction) * debug_delta_seconds * state->debug.camera_movement_speed;
        }
        
        state->camera.to_world = state->debug.camera.to_world;
    }
#endif
    
    state->camera.world_to_camera = make_inverse_unscaled_transform(state->camera.to_world);
    
    return ui;
}

void default_end_frame(Default_State *state)
{
    // im
    {
        glUseProgram(state->im_shader.program_object);
        
        glUniformMatrix4x3fv(state->im_shader.u_object_to_world_transform, 1, GL_FALSE, MAT4X3_IDENTITY);
        
        glDisable(GL_BLEND);
        draw_end(&state->im);
    }
    
    // ui
    {
        glUseProgram(state->ui_shader.program_object);
        
        glBindBuffer(GL_UNIFORM_BUFFER, state->camera_uniform_buffer_object);
        auto camera_block = cast_p(Camera_Uniform_Block, glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY));
        camera_block->camera_to_clip_projection = MAT4_IDENTITY;
        camera_block->world_to_camera = MAT4_IDENTITY;
        camera_block->camera_world_position = vec3f{};
        
        glUnmapBuffer(GL_UNIFORM_BUFFER);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        upload_render_and_clear(&state->ui.render_queue);
    }
}