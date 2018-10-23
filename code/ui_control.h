#include "basic.h"

struct UI_Control_Context {
    u32 selected_id;
    u32 active_id;
    u32 hot_id;
    
    struct {
        u32 id;
        float priority;
    } next_hot;
    
    bool cursor_down, cursor_up, select_down;
};

inline void ui_start(UI_Control_Context *context, bool cursor_down, bool cursor_up, bool select_down) {
    assert(!(cursor_down && cursor_up));
    
    context->cursor_down = cursor_down;
    context->cursor_up = cursor_up;
    context->select_down = select_down;
    
    if (context->active_id)
        context->hot_id = 0;
    else
        context->hot_id = context->next_hot.id;
    
    context->next_hot.id = 0;
}

inline void ui_set_active(UI_Control_Context *context, u32 id) {
    assert(!context->active_id);
    context->active_id = id;
}

inline void ui_update_hot(UI_Control_Context *context, u32 id, float priority) {
    if ((!context->next_hot.id) || (context->next_hot.priority > priority))
        context->next_hot = { id, priority };
}

inline void ui_set_selected(UI_Control_Context *context, u32 id) {
    if (!context->active_id)
        context->selected_id = id;
}

inline bool is_active(UI_Control_Context *context, u32 id) {
    return context->active_id == id;
}

inline bool ui_is_hot(UI_Control_Context *context, u32 id) {
    return context->hot_id == id;
}

inline bool ui_is_selected(UI_Control_Context *context, u32 id) {
    return context->selected_id == id;
}

inline bool ui_button(UI_Control_Context *context, u32 id, bool is_hot = false, float hot_priority = 0.0f) {
    bool result = false;
    
    if (is_active(context, id)) {
        if (context->cursor_up) {
            if (ui_is_hot(context, id))
                result = true;
            
            ui_set_active(context, 0);
            ui_set_selected(context, id);
        }
    }
    else if (ui_is_hot(context, id) && context->cursor_down)
        ui_set_active(context, id);
    else if (ui_is_selected(context, id) && context->select_down)
        result = true;
    
    if (is_hot)
        ui_update_hot(context, id, hot_priority);
    
    return result;
}
