// this will be inclued in generated files, using coroutines

#if !defined PREPROCESSOR_COROUTINES_H
#define PREPROCESSOR_COROUTINES_H

#include "u8_buffer.h"
#include "memory_growing_stack.h"

#define Template_Array_Name             u8_array
#define Template_Array_Data_Type        u8
#define Template_Array_Append_Allocator Memory_Growing_Stack
#include "template_array.h"

struct Coroutine_Stack {
    Memory_Growing_Stack *allocator;
    u8_array buffer;
    u32 current_byte_index;
};

enum Coroutine_Directive {
    Coroutine_Continue,
    Coroutine_Wait,
    Coroutine_End,
    Coroutine_Abort,
};

#define COROUTINE_DEC(name) Coroutine_Directive name(Coroutine_Stack *call_stack, any user_data)

typedef COROUTINE_DEC((*Coroutine_Function));

#define CO_ENTRY(type, byte_offset) (*cast_p(type, call_stack->buffer.data + call_stack->current_byte_index + byte_offset))

#define CO_RESULT(type, byte_offset) \
(*cast_p(type, one_past_last(call_stack->buffer) - byte_offset))

#define CO_STATE_ENTRY CO_ENTRY(u32, sizeof(Coroutine_Function))

#define CO_PREVIOUS_INDEX_ENTRY CO_ENTRY(u32, -sizeof(u32))

//#define CO_PUSH(type) (*grow_item(call_stack->allocator, &call_stack->buffer, type))

//#define CO_POP(type) shrink_item(call_stack->allocator, &call_stack->buffer, type)

u8_array co_push_coroutine(Coroutine_Stack *call_stack, Coroutine_Function coroutine, u32 byte_count) {
    u8_array it;
    it.data = grow(call_stack->allocator, &call_stack->buffer, byte_count);
    it.count = byte_count;
    
    *next_item(&it, u32) = call_stack->current_byte_index;
    // points at Coroutine_Function
    call_stack->current_byte_index = byte_count_of(call_stack->buffer) - byte_count + sizeof(u32);
    
    *next_item(&it, Coroutine_Function) = coroutine;
    *next_item(&it, u32) = 0; // coroutine state
    return it;
}

void co_pop_coroutine(Coroutine_Stack *call_stack, u32 byte_count) {
    shrink(call_stack->allocator, &call_stack->buffer, byte_count);
}

INTERNAL bool run_without_yielding(Coroutine_Stack *call_stack) {
    while (call_stack->current_byte_index != 0) {
        auto coroutine = CO_ENTRY(Coroutine_Function, 0);
        switch (coroutine(call_stack, null)) {
            case Coroutine_Wait:
            case Coroutine_Continue:
            {
            }break;
            
            case Coroutine_Abort: {
            } return false;
            
            CASES_COMPLETE;
        }
    }
    
    return true;
}

#endif // PREPROCESSOR_COROUTINES_H