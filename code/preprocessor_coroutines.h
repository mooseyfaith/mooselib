// this will be inclued in generated files, using coroutines

#if !defined PREPROCESSOR_COROUTINES_H
#define PREPROCESSOR_COROUTINES_H

#include "u8_buffer.h"

struct Coroutine_Stack {
    Memory_Allocator *allocator;
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

#define CO_STATE_ENTRY CO_ENTRY(u32, sizeof(Coroutine_Function))

#define CO_PREVIOUS_INDEX_ENTRY CO_ENTRY(u32, -sizeof(u32))

#define CO_PUSH(type) (*grow_item(call_stack->allocator, &call_stack->buffer, type))

#define CO_POP(type) shrink_item(call_stack->allocator, &call_stack->buffer, type)

#define CO_PUSH_COROUTINE(coroutine) { \
    CO_PUSH(u32) = call_stack->current_byte_index; \
    call_stack->current_byte_index = byte_count_of(call_stack->buffer); \
    CO_PUSH(Coroutine_Function) = coroutine; \
    CO_PUSH(u32) = 0; \
}

#define CO_POP_COROUTINE { \
    shrink(call_stack->allocator, &call_stack->buffer, byte_count_of(call_stack->buffer) - call_stack->current_byte_index); \
    call_stack->current_byte_index = *cast_p(u32, one_past_last(call_stack->buffer) - sizeof(u32)); \
    CO_POP(u32); \
}

Coroutine_Stack begin(Memory_Allocator *allocator, Coroutine_Function coroutine) {
    Coroutine_Stack stack = { allocator, {}, 0 };
    // because macros whant call_stack as a pointer
    auto call_stack = &stack;
    CO_PUSH_COROUTINE(coroutine);
    
    return stack;
}

// push some arguments to coroutine

COROUTINE_DEC(step) {
    while (call_stack->current_byte_index != 0)
    {
        auto coroutine = CO_ENTRY(Coroutine_Function, 0);
        
        switch (coroutine(call_stack, user_data)) {
            case Coroutine_Wait:
            return Coroutine_Wait;
            
            case Coroutine_Abort:
            return Coroutine_Abort;
            
            case Coroutine_Continue:
            break;
            
            CASES_COMPLETE;
        }
    }
    
    return Coroutine_End;
}

// run step until its done
// check return values and state

void end(Coroutine_Stack call_stack) {
    free_array(call_stack.allocator, &call_stack.buffer);
}

#endif // PREPROCESSOR_COROUTINES_H