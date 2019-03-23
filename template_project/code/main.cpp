
#include "default.h"

struct State : Default_State {
};

APP_INIT_DEC(application_init) {
    State *state;
    DEFAULT_STATE_INIT(State, state, platform_api);
    
    // load config:
    // - main window state
    // - camera state
    // - debug camera state
    {
        auto config = platform_api->read_entire_file(S("config.bin"), &state->transient_memory.allocator);
        if (config.count) {
            defer { free_array(&state->transient_memory.allocator, &config); };
            
            auto it = config;
            state->main_window_area = *next_item(&it, Pixel_Rectangle);
            state->camera.to_world = *next_item(&it, mat4x3f);
            state->debug.camera.to_world = *next_item(&it, mat4x3f);
        }
        else {
            state->main_window_area = { -1, -1, 1280, 720 };
        }
    }
    
    return state;
}

u32 const Main_Window_ID = 0;

APP_MAIN_LOOP_DEC(application_main_loop) {
    auto state = cast_p(State, app_data_ptr);
    auto ui = &state->ui;
    auto transient_allocator = &state->transient_memory.allocator;
    
    bool do_quit = !default_init_frame(state, input, platform_api, &delta_seconds);
    
    {
        Platform_Window window = platform_api->display_window(platform_api, Main_Window_ID, S("Track Fighter"), &state->main_window_area, true, state->main_window_is_fullscreen, 0.0f);
        
        do_quit |= window.was_destroyed;
        
        if (do_quit) {
            // save congif (see init)
            u8_array config = {};
            *grow_item(transient_allocator, &config, Pixel_Rectangle) = state->main_window_area;
            
            if (state->debug.is_active)
                *grow_item(transient_allocator, &config, mat4x3f) = state->debug.backup_camera_to_world;
            else
                *grow_item(transient_allocator, &config, mat4x3f) = state->camera.to_world;
            
            *grow_item(transient_allocator, &config, mat4x3f) = state->debug.camera.to_world;
            
            platform_api->write_entire_file(S("config.bin"), config);
            
            return Platform_Main_Loop_Quit;
        }
        
        vec4f clear_color = vec4f{ 0.0, 0.5, 0.5, 1.0 };
        default_window_begin(state, window, clear_color);
        defer { default_window_end(state); };
    }
    
    return Platform_Main_Loop_Continue;
}