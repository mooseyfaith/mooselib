#include "preprocessor_coroutines.h"

u32 factorial(u32 n);
s32 div(s32 *_out1, s32 a, s32 b);
void test();
COROUTINE_DEC(factorial_recursive);
COROUTINE_DEC(factorial_iterative);
u32 run_factorial(Memory_Growing_Stack* allocator, u32 n);

u32 factorial(u32 n) {
    if (n <= 1 ) {
        return 1;
    }
    
    return factorial(n - 1) * n;
}



s32 div(s32 *_out1, s32 a, s32 b) {
    *_out1 = a % b;
    return a / b;
}



void test() {
    s32 x;
    s32  y;
    x = div(&(y), 123, 3);
    {
        s32 _ignored1;
        div(&(_ignored1), 1, 2);
    }
    u32  z;
    z = factorial(3);
}



COROUTINE_DEC(factorial_recursive) {
    switch (CO_STATE_ENTRY) {
        case 0:
        goto co_label0;
        
        case 1:
        goto co_label1;
        
        default:
        assert(0);
    }
    
    // argument: def n: u32;
    co_label0:
    
    if (CO_ENTRY(u32, 12) <= 1 ) {
        { // return
            CO_RESULT(u32, 4) = 1;
            CO_STATE_ENTRY = u32_max;
            call_stack->current_byte_index = CO_PREVIOUS_INDEX_ENTRY;
            return Coroutine_Continue;
        }
        
    }
    
    
    { // call factorial_recursive
        CO_STATE_ENTRY = 1;
        u32 arg0 = CO_ENTRY(u32, 12) - 1;
        u8_array it = co_push_coroutine(call_stack, factorial_recursive, 28);
        *next_item(&it, u32) = arg0;
        return Coroutine_Continue;
    }
    co_label1:
    
    { // read results
        CO_ENTRY(u32 , 16) = CO_RESULT(u32, 4);
        co_pop_coroutine(call_stack, 28);
    }
    
    { // return
        CO_RESULT(u32, 4) = CO_ENTRY(u32, 12) * CO_ENTRY(u32 , 16);
        CO_STATE_ENTRY = u32_max;
        call_stack->current_byte_index = CO_PREVIOUS_INDEX_ENTRY;
        return Coroutine_Continue;
    }
    
    
    assert(0);
    return Coroutine_Abort;
}

COROUTINE_DEC(factorial_iterative) {
    switch (CO_STATE_ENTRY) {
        case 0:
        goto co_label0;
        
        case 1:
        goto co_label1;
        
        default:
        assert(0);
    }
    
    // argument: def n: u32;
    co_label0:
    
    CO_ENTRY(u32 , 16) = 1;
    CO_ENTRY(u32 , 20) = CO_ENTRY(u32 , 16);
    while (CO_ENTRY(u32 , 16) < CO_ENTRY(u32, 12) ) {
        { // yield
            CO_RESULT(u32, 4) = CO_ENTRY(u32 , 20);
            CO_STATE_ENTRY = 1;
            return Coroutine_Wait;
        }
        
        co_label1:
        
        CO_ENTRY(u32 , 16)++;
        CO_ENTRY(u32 , 20) *= CO_ENTRY(u32 , 16);
    }
    
    { // return
        CO_RESULT(u32, 4) = CO_ENTRY(u32 , 20);
        CO_STATE_ENTRY = u32_max;
        call_stack->current_byte_index = CO_PREVIOUS_INDEX_ENTRY;
        return Coroutine_Continue;
    }
    
    
    assert(0);
    return Coroutine_Abort;
}

u32 run_factorial(Memory_Growing_Stack* allocator, u32 n) {
    u32  x;
    { 
        Memory_Growing_Stack *_allocator = allocator;
        Coroutine_Stack _call_stack = { _allocator };
        auto call_stack = &_call_stack;
        auto it = co_push_coroutine(call_stack, factorial_recursive, 28);
        *next_item(&it, u32) = n;
        
        if (!run_without_yielding(call_stack)) {
            assert(0, "todo: handle Coroutine_Abort");
        }
        
        { // read results
            x = CO_RESULT(u32, 4);
            co_pop_coroutine(call_stack, 28);
        }
        
        free_array(_allocator, &call_stack->buffer);
    }
    return x;
}



