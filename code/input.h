#if !defined _ENGINE_INPUT_H_
#define _ENGINE_INPUT_H_

//#include <Windows.h>

#include "mathdefs.h"

enum Keycode {
    Keycode_Return,
};

struct Input_Button {
    float idle_time;
    u8 up_count, down_count;
    bool is_active;
};

struct Input_Gamepad {
    struct Stick {
        vec2f raw_direction;
        vec2f direction;
    };
    
    bool is_enabled;
    
    union {
        struct {
            Stick left_stick;
            Stick right_stick;
        };
        Stick sticks[2];
    };
    
    union {
        struct {
            union {
                struct {
                    Input_Button left;
                    Input_Button right;
                    Input_Button down;
                    Input_Button up;
                };
                Input_Button buttons[4];
            } digital_pad;
            
            union {
                struct {
                    Input_Button left;
                    Input_Button right;
                    Input_Button down;
                    Input_Button up;
                };
                Input_Button buttons[4];
            } action_pad;
            
            Input_Button left_shoulder;
            Input_Button right_shoulder;
            Input_Button left_stick_button;
            Input_Button right_stick_button;
            
            Input_Button select_button;
            Input_Button start_button;
            Input_Button home_button;
        };
        Input_Button buttons[15];
    };
};

struct Game_Input {
    Input_Gamepad *gamepads;
    u32 gamepad_count;
    
    Input_Button keys[256];
    Input_Button left_shift, right_shift;
    Input_Button left_alt, right_alt;
    Input_Button left_control, right_control;
    
    struct {
        union {
            struct {
                Input_Button left, middle, right;
            };
            Input_Button buttons[3];
        };
        
        vec2f window_position;
        f32 horizontal_wheel_delta;
        f32 vertical_wheel_delta;
        u32 window_id;
        bool is_inside_window;
    } mouse;
};

inline void normalize_stick(Input_Gamepad *gamepad, float dead_zone) {
    for (u32 stick_index = 0; stick_index < ARRAY_COUNT(gamepad->sticks); ++stick_index) {
        Input_Gamepad::Stick *stick = gamepad->sticks + stick_index;
        
        for (u32 component = 0; component < 2; ++component) {
            if (fabs(stick->raw_direction[component]) > dead_zone)
                stick->direction[component] = stick->raw_direction[component];
            else
                stick->direction[component] = 0.0f;
        }
        
        if (squared_length(stick->direction) > 1.0f)
            stick->direction = normalize(stick->direction);
    }
}

inline void button_poll_update(Input_Button *button, bool is_active) {
    if (button->is_active != is_active) {
        button->idle_time = 0.0f;
        button->is_active = is_active;
    }
}

inline void button_poll_advance(Input_Button *button, float delta_seconds) {
    button->idle_time += delta_seconds;
}

inline void button_event_update(Input_Button *button, bool is_active)
{
    if (is_active) {
        ++button->down_count;
        assert(button->down_count, "overflow, too many button down events between 2 frames");
    }
    else {
        ++button->up_count;
        assert(button->up_count, "overflow, too many button up events between 2 frames");
    }
    
    button->is_active = is_active;
    button->idle_time = 0.0f;
}

inline void button_event_advance(Input_Button *button, float delta_seconds) {
    button->up_count   = 0;
    button->down_count = 0;
    button->idle_time += delta_seconds;
}

inline bool was_pressed(Input_Button button, float max_duration = 0.0f) {
    return button.down_count || (button.is_active && (button.idle_time <= max_duration));
}

inline bool was_released(Input_Button button, float max_duration = 0.0f) {
    return button.up_count || (!button.is_active && (button.idle_time <= max_duration));
}

inline vec2f get_relative_mouse_position(vec2f screen_mouse_position, const Pixel_Dimensions *window) {
    return screen_mouse_position / vec2f{ (float)window->width, (float)-window->height } * 2.0f - vec2f{ 1.0f, 1.0f };
}

inline void key_event_update(Game_Input *input, u8 key_code, bool is_down)
{
    button_event_update(input->keys + key_code, is_down);
}

inline bool is_active(Game_Input *input, u8 keycode) {
    return input->keys[keycode].is_active;
}

inline bool was_pressed(Game_Input *input, u8 keycode, float max_duration = 0.0f) {
    return was_pressed(input->keys[keycode], max_duration);
}

inline bool was_released(Game_Input *input, u8 keycode, float max_duration = 0.0f) {
    return was_released(input->keys[keycode], max_duration);
}

inline void advance_input(Game_Input *input, f32 delta_seconds)
{
#if 0
    for (u32 i = 0; i != ARRAY_COUNT(input->mouse.buttons); ++i)
        button_event_advance(input->mouse.buttons + i, CAST_V(f32, delta_seconds));
#endif
    
    for (u32 i = 0; i != ARRAY_COUNT(input->keys); ++i)
        button_event_advance(input->keys + i, delta_seconds);
    
    button_poll_advance(&input->mouse.left, delta_seconds);
    button_poll_advance(&input->mouse.right, delta_seconds);
    button_poll_advance(&input->mouse.middle, delta_seconds);
    
    button_poll_advance(&input->left_shift, delta_seconds);
    button_poll_advance(&input->right_shift, delta_seconds);
    
    button_poll_advance(&input->left_alt, delta_seconds);
    button_poll_advance(&input->right_alt, delta_seconds);
    
    button_poll_advance(&input->left_control, delta_seconds);
    button_poll_advance(&input->right_control, delta_seconds);
    
    input->mouse.horizontal_wheel_delta = 0.0f;
    input->mouse.vertical_wheel_delta = 0.0f;
}

#endif // _ENGINE_INPUT_

