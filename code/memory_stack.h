#if !defined MEMORY_STACK_H
#define MEMORY_STACK_H

#include "u8_buffer.h"

struct Memory_Stack {
    Memory_Allocator allocator;
    u8_buffer buffer;
};

#define Template_Allocator_Name Memory_Stack
#define Template_Allocator_Kind Memory_Allocator_Stack_Kind
#include "template_memory_allocator.h"

// Info: You can also use any array_info to initialize a memory_stack, make shure the array data has the same lifetime
// memory_stack = ARRAY_WITH_COUNT(my_static_array);
// memory_stack = { data, count };

// free and reallocate can only be used on the last allocated data
// so you have to free in reverse order of your allocations
// remember to specifier the allocator with free, like free(my_allocator, my_data) instead of just free(my_data)

#define assert_bounds(buffer) defer { assert((buffer).count <= (buffer).capacity); };

struct Debug_Memory_Stack_Footer {
    usize size;
    u16 padding;
};

INTERNAL Debug_Memory_Stack_Footer debug_make_footer(Memory_Stack stack, usize size, usize alignment)
{
    Debug_Memory_Stack_Footer result;
    result.size = size;
    
    usize padding = (alignment - (MEMORY_ADDRESS(one_past_last(stack.buffer)) % alignment)) % alignment;
    result.padding = padding;
    assert(result.padding == padding);
    
    return result;
}

INTERNAL Memory_Stack make_memory_stack(Memory_Allocator *allocator, usize size, usize alignment = MEMORY_MAX_ALIGNMENT)
{
    return { Memory_Allocator_Stack_Kind, { CAST_P(u8, allocate(allocator, size, alignment)), size } };
}

INTERNAL ALLOCATE_DEC(Memory_Stack *stack)
{
    assert_bounds(stack->buffer);
    assert(size && alignment);
    
    auto info = debug_make_footer(*stack, size, alignment);
    push(&stack->buffer, info.padding);
    u8 *data = push(&stack->buffer, size);
    //u8 *data = one_past_last(stack->buffer); + info.padding;
    
#if defined DEBUG
    
    //assert(stack->buffer.count + info.size + info.padding + sizeof(info) <= stack->buffer.capacity);
    //stack->buffer.count += info.size + info.padding + sizeof(info);
    //*debug_footer(*stack) = info;
    *push_item(&stack->buffer, Debug_Memory_Stack_Footer) = info;
    
    //assert(stack->buffer.count >= info.size + info.padding + sizeof(info));
    
#else
    
    //assert(stack->buffer.count + info.size + info.padding <= stack->buffer.capacity);
    //stack->buffer.count += info.size + info.padding;
    //push(&stack->buffer, info.padding);
    
#endif
    
    return data;
}

Debug_Memory_Stack_Footer debug_get_footer(Memory_Stack stack, u8 *data) {
    auto info = peek_item(stack.buffer, Debug_Memory_Stack_Footer);
    assert(data + info->size == cast_p(u8, info), "you possibly override the footer information, maybe you copy to much?");
    assert(stack.buffer.count >= info->size + info->padding + sizeof(*info));
    return *info;
}

INTERNAL REALLOCATE_DEC(Memory_Stack *stack)
{
    assert_bounds(stack->buffer);
    assert(size);
    assert((MEMORY_ADDRESS(*data) % alignment) == 0);
    
#if defined DEBUG
    
    auto info = debug_get_footer(*stack, cast_p(u8, *data));
    
    if (remaining_count(stack->buffer) + info.size >= size) {
        
        {
            pop_item(&stack->buffer, Debug_Memory_Stack_Footer);
            pop(&stack->buffer, info.size);
            
            info.size = size;
            push(&stack->buffer, size);
            *push_item(&stack->buffer, Debug_Memory_Stack_Footer) = info;
        }
        
        // just checks if is okay
        debug_get_footer(*stack, cast_p(u8, *data));
        
        return true;
    }
    
    return false;
    
#else
    
    if (index_of(stack->buffer, cast_p(u8, *data)) + size > stack->buffer.capacity)
        return false;
    
    free(stack, *data);
    *data = allocate(stack, size, alignment);
    
#endif
    
    return true;
}

INTERNAL FREE_DEC(Memory_Stack *stack) {
    assert_bounds(stack->buffer);
    assert(data);
    
    u8 *u8_data = cast_p(u8, data);
    
#if defined DEBUG
    
    auto info = debug_get_footer(*stack, u8_data);
    
    usize byte_count = info.size + info.padding;
    pop_item(&stack->buffer, Debug_Memory_Stack_Footer);
    pop(&stack->buffer, byte_count);
    
#else
    
    stack->buffer.count = index_of(stack->buffer, u8_data);
    
#endif
}

#endif // MEMORY_STACK_H