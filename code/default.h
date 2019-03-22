
#include <platform.h>
#include <memory_growing_stack.h>
#include <memory_c_allocator.h>
#include <yaui.h>

#include <immediate_render.cpp>

enum {
    Camera_Uniform_Block_Index = 0,
    Lighting_Uniform_Block_Index,
};

// all aliginged to vec4, since layout (std140) sux!!!
struct Camera_Uniform_Block
{
    mat4f world_to_clip;
    // actually a 4x3 but after each vec3 there is 1 f32 padding
    //mat4f world_to_camera;
    vec3f world_position;
    f32 padding0;
};

#define MAX_BONE_COUNT 0

#define MAX_LIGHT_COUNT 10

struct Lighting_Uniform_Block {
    vec4f colors[MAX_LIGHT_COUNT];
    vec4f parameters[MAX_LIGHT_COUNT];
    vec4f global_ambient_color;
    u32 directional_light_count;
    u32 point_light_count;
    u32 padding[2];
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
    
    //Memory_Growing_Stack ui_memory;
    
    Font default_font;
    UI_Context ui;
    Immediate_Render_Context im;
    Pixel_Rectangle main_window_area;
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
    
    struct Default_Shader {
        GLuint program_object;
        
        union {
            struct { 
                struct {
                    GLint diffuse_color;
                    GLint specular_color;
                    GLint gloss;
                    GLint metalness;
                    GLint diffuse_map;
                    GLint normal_map;
                } Material;
                
                struct {
                    GLint world_to_shadow;
                    GLint map;
                } Shadow;
                
                struct {
                    GLint world_to_environment;
                    GLint map;
                    GLint level_of_detail_count;
                } Environment;
                
                GLint Object_To_World;
                GLint Animation_Bones[MAX_BONE_COUNT];
            } uniform;
            
            GLint uniforms[sizeof(uniform) / sizeof(GLint)];
        };
    };
    
    Default_Shader im_shader;
    Default_Shader default_shader;
    
    struct Shadow_Map_Shader {
        GLuint program_object;
        
        union {
            struct {
                GLint World_To_Shadow_Map;
                GLint Object_To_World;
            };
            
            GLint uniforms[2];
        };
    };
    
    Shadow_Map_Shader shadow_map_shader;
    
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
    auto persistent_memory = make_memory_growing_stack((platform_api)->allocator); \
    (state) = ALLOCATE(&persistent_memory.allocator, type); \
    (state)->persistent_memory = persistent_memory; \
    init(state, platform_api); \
}

GLuint load_shader(Default_State *state, Platform_API *platform_api, GLint *uniforms, u32 uniform_count, string file, string options, string uniform_names)
{
    string shader_source = platform_api->read_entire_file(file, &state->transient_memory.allocator);
    assert(shader_source.count);
    
    defer { free_array(&state->transient_memory.allocator, &shader_source); };
    
    Shader_Attribute_Info attributes[] = {
        { Vertex_Position_Index, "a_position" },
        { Vertex_Normal_Index,   "a_normal" },
        { Vertex_Tangent_Index,  "a_tangent" },
        { Vertex_UV_Index,       "a_uv" },
        { Vertex_Color_Index,    "a_color" },
        
        { Vertex_Position_Index, "vertex_position" },
        { Vertex_Normal_Index,   "vertex_normal" },
        { Vertex_Tangent_Index,  "vertex_tangent" },
        { Vertex_UV_Index,       "vertex_uv" },
        { Vertex_Color_Index,    "vertex_color" },
    };
    
    string define_buffer = {};
    write(&state->transient_memory.allocator, &define_buffer, S("#version 150\n"));
    defer { free_array(&state->transient_memory.allocator, &define_buffer); };
    
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

void init(Default_State *state, Platform_API *platform_api)
{
    state->transient_memory = make_memory_growing_stack(platform_api->allocator);
    
    {
        auto ft_library = begin_font_loading();
        
        auto source = platform_api->read_entire_file(S("C:/Windows/Fonts/consola.ttf"), &state->transient_memory.allocator);
        defer { free_array(&state->transient_memory.allocator, &source); };
        
        state->default_font = make_font(&state->persistent_memory.allocator, ft_library, source, 16, ' ', 256 - ' ');
        
        end_font_loading(ft_library);
    }
    
    state->blank_texture = make_blank_texture();
    state->blank_normal_map = make_single_color_texture({ 128, 128, 255 });
    
    
    //state->ui_memory = make_memory_growing_stack(&platform_api->allocator);
    state->ui = make_ui_context(platform_api->allocator);
    
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
    
    state->ui_shader.program_object = load_shader(state, platform_api, ARRAY_WITH_COUNT(state->ui_shader.uniforms), S(MOOSELIB_PATH "shaders/transparent_textured.shader.txt"), S("WITH_VERTEX_COLOR, WITH_DIFFUSE_TEXTURE, ALPHA_THRESHOLD 0.025"), S(STRINGIFY(UI_SHADER_UNIFORMS)));
    
    auto default_shader_uniforms = S("Material.diffuse_color, Material.specular_color, Material.gloss, Material.metalness, Material.diffuse_map, Material.normal_map, "
                                     "Shadow.world_to_shadow, Shadow.map, "
                                     "Environment.world_to_environment, Environment.map, Environment.level_of_detail_count, "
                                     "Object_To_World");
    
    state->im_shader.program_object = load_shader(state, platform_api, ARRAY_WITH_COUNT(state->im_shader.uniforms), S(MOOSELIB_PATH "shaders/default.shader.txt"), S("WITH_VERTEX_COLOR"), 
                                                  default_shader_uniforms);
    
    state->default_shader.program_object = load_shader(state, platform_api, ARRAY_WITH_COUNT(state->default_shader.uniforms), S(MOOSELIB_PATH "shaders/default.shader.txt"), 
                                                       //S("WITH_SHADOW_MAP, WITH_ENVIRONMENT_MAP, WITH_DIFFUSE_COLOR, MAX_LIGHT_COUNT 10"), 
                                                       S("WITH_VERTEX_COLOR, WITH_DIFFUSE_COLOR, MAX_LIGHT_COUNT 10"), 
                                                       default_shader_uniforms);
    
    state->shadow_map_shader.program_object = load_shader(state, platform_api, ARRAY_WITH_COUNT(state->shadow_map_shader.uniforms), S(MOOSELIB_PATH "shaders/shadow_map.shader.txt"), S("WITH_DIFFUSE_COLOR"),
                                                          S("World_To_Shadow_Map, Object_To_World"));
    
    init(&state->im);
    
    state->camera.to_world = make_transform(make_quat(VEC3_X_AXIS, Pi32 * -0.25f));
    state->camera.to_world.translation = transform_direction(state->camera.to_world, vec3f{0, 0, 40});
    
    state->debug.speed = 1.0f;
    state->debug.backup_speed = 1.0f;
    
    state->debug.camera_movement_speed    = 10.0f;
    state->debug.camera_mouse_sensitivity = 2.0f * Pi32 / 2048.0f;;
    
    state->debug.camera.alpha_axis = VEC3_Y_AXIS;
    state->debug.camera.beta_axis = VEC3_X_AXIS;
    state->debug.camera.beta_min = -Pi32 * 0.5f;
    state->debug.camera.beta_max =  Pi32 * 0.5f;
    
    state->debug.camera.alpha = 0.0f;
    state->debug.camera.beta = Pi32 * -0.25f;
    debug_update_camera(&state->debug.camera);
    state->debug.camera.to_world.translation = transform_direction(state->debug.camera.to_world, vec3f{ 0, 0, 20.f });
}

void default_init_worker_queue_memory(Default_State *state, Platform_API *platform_api, usize total_byte_count, u32 stack_count) {
    state->worker_thread_stacks = ALLOCATE_ARRAY(&state->persistent_memory.allocator, Memory_Stack, stack_count);
    for (u32 i = 0; i < stack_count; ++i)
        state->worker_thread_stacks[i] = make_memory_stack(&state->persistent_memory.allocator, total_byte_count / stack_count);
    
    state->work_memory = make_memory_growing_stack(platform_api->allocator);
}

bool default_init_frame(Default_State *state, const Game_Input *input, Platform_API *platform_api, f64 *delta_seconds)
{
    clear(&state->transient_memory);
    
    for (auto it = platform_api->messages.head; it; it = it->next) {
        if (it->value->kind == Platform_Message_Kind_Reload) {
            default_reload_global_functions(platform_api);
            break;
        }
    }
    
#if defined DEBUG_EDITOR
    
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
    
    ui_control_start(&state->ui, *delta_seconds, was_pressed(input->mouse.left), was_released(input->mouse.left));
    state->ui.transient_allocator = &state->transient_memory.allocator;
    
    return true;
}

void upload_camera_block(GLuint camera_uniform_buffer_object, mat4x3f world_to_camera, mat4f camera_to_clip, vec3f camera_position) {
    glBindBuffer(GL_UNIFORM_BUFFER, camera_uniform_buffer_object);
    auto camera_block = cast_p(Camera_Uniform_Block, glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY));
    //camera_block->camera_to_clip = camera_to_clip;
    camera_block->world_to_clip = camera_to_clip * world_to_camera;
    
    //for (u32 i = 0; i < 4; ++i)
    //camera_block->world_to_camera.columns[i] = make_vec4(world_to_camera.columns[i], 0.0f);
    
    camera_block->world_position = camera_position;
    
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void upload_camera_block(Default_State *state) {
    upload_camera_block(state->camera_uniform_buffer_object, state->camera.world_to_camera, state->camera.to_clip_projection, state->camera.to_world.translation);
    
}

bool default_window_begin(Default_State *state, Platform_Window window, vec4f clear_color, f32 scale = 1.0f) {
    Pixel_Dimensions render_resolution = window.area.size; //get_auto_render_resolution(state->main_window_area.size, Reference_Resolution);
    
    if (render_resolution.width == 0 || render_resolution.height == 0)
        return false;
    
    set_auto_viewport(render_resolution, render_resolution, clear_color);
    
    auto ui = &state->ui;
    ui_set_cursor(ui, { window.mouse_position, { 1, 1 } }, window.mouse_is_inside);
    
    ui_set_transform(ui, render_resolution, scale);
    ui_set_texture(ui, &state->blank_texture);
    ui_set_font(ui, &state->default_font);
    
    draw_begin(&state->im, C_Allocator, MAT4X3_IDENTITY);
    
    state->camera.to_clip_projection = make_perspective_fov_projection((f32)(60 * Degree_To_Radian32), width_over_height(render_resolution));
    state->camera.clip_to_camera_projection = make_inverse_perspective_projection(state->camera.to_clip_projection);
    
    upload_camera_block(state->camera_uniform_buffer_object, state->camera.world_to_camera, state->camera.to_clip_projection, state->camera.to_world.translation);
    
    return true;
}

#if 0
UI_Context * default_begin_ui(Default_State *state, const Game_Input *input, f64 delta_seconds, bool allways_update, Platform_Window window, Pixel_Rectangle *window_rect, Pixel_Rectangle new_window_rect, bool cursor_was_pressed, bool cursor_was_released, vec4f clear_color, f32 scale = 1.0f)
{
    bool do_update = !are_equal(window_rect, &new_window_rect, sizeof(*window_rect));
    *window_rect = new_window_rect;
    
    Pixel_Dimensions render_resolution = window_rect->size; //get_auto_render_resolution(state->main_window_area.size, Reference_Resolution);
    
    if (render_resolution.width == 0 || render_resolution.height == 0) {
        return null;
    }
    
    set_auto_viewport(render_resolution, render_resolution, clear_color);
    
    auto ui = &state->ui;
    
    do_update &= ui_control_start(ui, delta_seconds, cursor_was_pressed, cursor_was_released);
    
    //if (!allways_update && !do_update)
    //return null;
    
    //ui_clear(ui);
    
    ui_set_transform(ui, render_resolution, scale);
    ui_set_texture(ui, &state->blank_texture);
    ui_set_font(ui, &state->default_font);
    
    ui->transient_allocator = &state->transient_memory.allocator;
    
    draw_begin(&state->im, C_Allocator, MAT4X3_IDENTITY);
    
    state->camera.to_clip_projection = make_perspective_fov_projection((f32)(60 * Degree_To_Radian32), width_over_height(render_resolution));
    state->camera.clip_to_camera_projection = make_inverse_perspective_projection(state->camera.to_clip_projection);
    
    upload_camera_block(state->camera_uniform_buffer_object, state->camera.world_to_camera, state->camera.to_clip_projection, state->camera.to_world.translation);
    
    return ui;
}
#endif

void default_debug_camera(Default_State *state, Game_Input const * input, Platform_Window window, f32 delta_seconds) {
    
#if defined DEBUG_EDITOR
    
    if (was_pressed(input->keys[VK_F1])) {
        state->debug.is_active = !state->debug.is_active;
        
        if (state->debug.is_active)
            state->debug.backup_camera_to_world = state->camera.to_world;
        else
            state->camera.to_world = state->debug.backup_camera_to_world;
    }
    
    if (was_pressed(input->keys[VK_F2])) {
        if (state->debug.is_active)
            state->debug.backup_camera_to_world = state->camera.to_world;
        else
            state->debug.camera.to_world = state->camera.to_world;
    }
    
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
}

void default_window_end(Default_State *state)
{
    // im
    {
        glUseProgram(state->im_shader.program_object);
        
        glUniformMatrix4x3fv(state->im_shader.uniform.Object_To_World, 1, GL_FALSE, MAT4X3_IDENTITY);
        
        draw_end(&state->im);
    }
    
    // ui
    {
        glUseProgram(state->ui_shader.program_object);
        
        upload_camera_block(state->camera_uniform_buffer_object, MAT4X3_IDENTITY, MAT4_IDENTITY, {});
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //glDisable(GL_DEPTH_TEST);
        
        ui_end(&state->ui);
        
        glDisable(GL_BLEND);
        //glEnable(GL_DEPTH_TEST);
    }
}