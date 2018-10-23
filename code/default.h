
#include <platform.h>
#include <memory_growing_stack.h>
#include <memory_c_allocator.h>
#include <ui.h>

enum {
    Camera_Uniform_Block_Index = 0,
};

// all aliginged to vec4, since layout (std140) sux!!!
struct Camera_Uniform_Block
{
    mat4f camera_to_clip_projection;
    // actually a 4x3 but after each vec3 there is 1 f32 padding
    mat4f world_to_camera_transform;
    vec3f camera_world_position;
    f32 padding0;
};

struct Default_State
{
    Memory_Growing_Stack transient_memory;
    Memory_Growing_Stack persistent_memory;
    Memory_Growing_Stack work_memory;
    Memory_Stack *worker_thread_stacks;
    
    FT_Library font_library;
    Font default_font;
    UI_Context ui;
    
    union {
        struct {
            GLuint camera_uniform_buffer_object;
        };
        
        GLuint uniform_buffer_objects[1];
    };
    
    struct {
        
#define UI_SHADER_UNIFORMS u_diffuse_texture
        
        union {
            struct { GLint UI_SHADER_UNIFORMS; };
            GLint uniforms[1];
        };
        
        GLuint program_object;
        GLuint camera_uniform_block;
        
    } ui_shader;
    
    Texture blank_texture;
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

void init(Default_State *state, Platform_API *platform_api, usize worker_thread_memory_count = MEGA(256))
{
    state->transient_memory = make_memory_growing_stack(&platform_api->allocator);
    
    {
        state->worker_thread_stacks = ALLOCATE_ARRAY(&state->persistent_memory.allocator, Memory_Stack, platform_api->worker_thread_count);
        for (u32 i = 0; i < platform_api->worker_thread_count; ++i)
            state->worker_thread_stacks[i] = make_memory_stack(&state->persistent_memory.allocator, worker_thread_memory_count / platform_api->worker_thread_count);
        
        state->work_memory = make_memory_growing_stack(&platform_api->allocator);
    }
    
    state->font_library = make_font_library();
    
    state->default_font = make_font(state->font_library, S("C:/Windows/Fonts/consola.ttf"), 12, ' ', 256 - ' ', platform_api->read_entire_file, &state->persistent_memory.allocator);
    
    state->blank_texture = make_blank_texture();
    state->ui = make_ui_context(&platform_api->allocator, &state->blank_texture);
    
    {
        glGenBuffers(COUNT_WITH_ARRAY(state->uniform_buffer_objects));
        glBindBuffer(GL_UNIFORM_BUFFER, state->camera_uniform_buffer_object);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(Camera_Uniform_Block), null, GL_STREAM_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, Camera_Uniform_Block_Index, state->camera_uniform_buffer_object);
        
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
    
    {
        string shader_source = platform_api->read_entire_file(S("shaders/transparent_textured.shader.txt"), &state->transient_memory.allocator);
        
        assert(shader_source.count);
        
        defer { free(&state->transient_memory.allocator, shader_source.data); };
        
        Shader_Attribute_Info attributes[] = {
            { Vertex_Position_Index,     "a_position" },
            { Vertex_Color_Index,        "a_color" },
            { Vertex_UV_Index,           "a_uv" },
            { Vertex_First_Unused_Index, "a_saturation" },
        };
        
        string uniform_names = S(STRINGIFY(UI_SHADER_UNIFORMS));
        
        GLuint shader_objects[2];
        
        string shader_options =
            S(
            "#version 150\n"
            "#define WITH_VERTEX_COLOR\n"
            "#define WITH_DIFFUSE_TEXTURE\n"
            //"#define WITH_SATURATION\n"
            "#define ALPHA_THRESHOLD 0.025\n"
            );
        
        string vertex_sources[] = {
            shader_options,
            S("#define VERTEX_SHADER\n"),
            shader_source
        };
        
        shader_objects[0] = make_shader_object(GL_VERTEX_SHADER, ARRAY_WITH_COUNT(vertex_sources), &state->transient_memory.allocator);
        
        string fragment_sources[] = {
            shader_options,
            S("#define FRAGMENT_SHADER\n"),
            shader_source
        };
        
        shader_objects[1] = make_shader_object(GL_FRAGMENT_SHADER, ARRAY_WITH_COUNT(fragment_sources), &state->transient_memory.allocator);
        
        GLuint program_object = make_shader_program(ARRAY_WITH_COUNT(shader_objects), false, ARRAY_WITH_COUNT(attributes), uniform_names, ARRAY_WITH_COUNT(state->ui_shader.uniforms), &state->transient_memory.allocator);
        
        assert(program_object);
        
        if (state->ui_shader.program_object)
            glDeleteProgram(state->ui_shader.program_object);
        
        state->ui_shader.camera_uniform_block = glGetUniformBlockIndex(program_object, "Camera_Uniform_Block");
        glUniformBlockBinding(program_object, state->ui_shader.camera_uniform_block, Camera_Uniform_Block_Index);
        
        state->ui_shader.program_object = program_object;
    }
}

void default_begin_frame(Default_State *state, Platform_API *platform_api)
{
    clear(&state->transient_memory);
    
    for (auto it = platform_api->messages.head; it; it = it->next) {
        if (it->value->kind == Platform_Message_Kind_Reload) {
            default_reload_global_functions(platform_api);
            break;
        }
    }
}

UI_Context * default_begin_ui(Default_State *state, bool *do_update, Platform_Window window,Pixel_Rectangle *window_rect, Pixel_Rectangle new_window_rect, bool cursor_was_pressed, bool cursor_was_released, vec4f clear_color, f32 scale = 1.0f)
{
    *do_update |= !ARE_EQUAL(window_rect, &new_window_rect, sizeof(*window_rect));
    *window_rect = new_window_rect;
    
    Pixel_Dimensions render_resolution = window_rect->size; //get_auto_render_resolution(state->main_window_area.size, Reference_Resolution);
    
    if (render_resolution.width == 0 || render_resolution.height == 0) {
        do_update = false;
        return null;
    }
    
    set_auto_viewport(render_resolution, render_resolution, clear_color);
    
    auto ui = &state->ui;
    
    if (!ui_control(ui, { window.mouse_position, { 1, 1 } }, cursor_was_pressed, cursor_was_released) && !*do_update)
    {
        //platform_api->skip_window_update(platform_api);
        //return true;
    }
    
    ui_clear(ui);
    
    ui_set_transform(ui, render_resolution, scale);
    ui_texture(ui, &state->blank_texture);
    ui->font_rendering.font = &state->default_font;
    ui->font_rendering.line_spacing = (state->default_font.pixel_height + 1) * ui->font_rendering.scale;
    
    return ui;
}

void default_end_frame(Default_State *state)
{
    glUseProgram(state->ui_shader.program_object);
    
    glBindBuffer(GL_UNIFORM_BUFFER, state->camera_uniform_buffer_object);
    auto camera_block = cast_p(Camera_Uniform_Block, glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY));
    camera_block->camera_to_clip_projection = MAT4_IDENTITY;
    camera_block->world_to_camera_transform = MAT4_IDENTITY;
    camera_block->camera_world_position = vec3f{};
    
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    ui_flush(&state->ui);
    ui_draw(&state->ui);
}