#include "preprocessor_coroutines.h"

u32 factorial(u32 n);
s32 div(s32 *_out1, s32 a, s32 b);
void test();
COROUTINE_DEC(factorial_recursive);
COROUTINE_DEC(factorial_iterative);
u32 run_factorial(Memory_Growing_Stack* allocator, u32 n);

#line 1 "preprocessor_test.mcpp"

#line 2
u32 factorial(u32 n) {
#line 3
    if (n <= 1 ) {
        return 1;
    }
    
#line 4
#line 5
#line 6
    return factorial(n - 1) * n;
}



#line 7
#line 8
s32 div(s32 *_out1, s32 a, s32 b) {
#line 9
#line 10
    *_out1 = a % b;
    return a / b;
}



#line 11
#line 12
void test() {
#line 13
    s32 x;
    s32  y;
    x = div(&(y), 123, 3);
#line 14
#line 15
    {
        s32 _ignored1;
        div(&(_ignored1), 1, 2);
    }
#line 16
#line 17
    u32  z;
    z = factorial(3);
#line 18
}



#line 19
#line 20
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
    
#line 21
    if (CO_ENTRY(u32, 12) <= 1 ) {
        { // return
            CO_RESULT(u32, 4) = 1;
            CO_STATE_ENTRY = u32_max;
            call_stack->current_byte_index = CO_PREVIOUS_INDEX_ENTRY;
            return Coroutine_Continue;
        }
        
    }
    
#line 22
    if (CO_ENTRY(u32, 12) <= 1 ) {
        { // return
            CO_RESULT(u32, 4) = 1;
            CO_STATE_ENTRY = u32_max;
            call_stack->current_byte_index = CO_PREVIOUS_INDEX_ENTRY;
            return Coroutine_Continue;
        }
        
    }
    
#line 23
#line 24
    
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
    
#line 25
    { // return
#line 26
        CO_RESULT(u32, 4) = CO_ENTRY(u32, 12) * CO_ENTRY(u32 , 16);
        CO_STATE_ENTRY = u32_max;
        call_stack->current_byte_index = CO_PREVIOUS_INDEX_ENTRY;
        return Coroutine_Continue;
    }
    
    
    assert(0);
    return Coroutine_Abort;
}

#line 27
#line 28
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
    
#line 29
#line 30
    CO_ENTRY(u32 , 16) = 1;
#line 31
#line 32
    CO_ENTRY(u32 , 20) = CO_ENTRY(u32 , 16);
    while (CO_ENTRY(u32 , 16) < CO_ENTRY(u32, 12) ) {
#line 33
        { // yield
#line 34
            CO_RESULT(u32, 4) = CO_ENTRY(u32 , 20);
            CO_STATE_ENTRY = 1;
            return Coroutine_Wait;
        }
        
        co_label1:
        
#line 35
        CO_ENTRY(u32 , 16)++;
#line 36
        CO_ENTRY(u32 , 20) *= CO_ENTRY(u32 , 16);
    }
    
#line 37
#line 38
    { // return
#line 39
        CO_RESULT(u32, 4) = CO_ENTRY(u32 , 20);
        CO_STATE_ENTRY = u32_max;
        call_stack->current_byte_index = CO_PREVIOUS_INDEX_ENTRY;
        return Coroutine_Continue;
    }
    
    
    assert(0);
    return Coroutine_Abort;
}

#line 40
#line 41
u32 run_factorial(Memory_Growing_Stack* allocator, u32 n) {
#line 42
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
#line 43
#line 44
    return x;
}



#line 45
#line 46
#line 47
