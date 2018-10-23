#if !defined MEMORY_QUEUE_H
#define MEMORY_QUEUE_H

// Memory_Stack ---------------------------------------------------------------

#define TEMPLATE_NAME      Memory_Queue
#define TEMPLATE_DATA_TYPE u8
#define TEMPLATE_SIZE_TYPE memory_units
#include "buffer.h"

#define TEMPLATE_ALLOCATOR_TYPE Memory_Queue
#define TEMPLATE_ALLOCATOR_NAME memory_queue
#include "memory_template.h"

// Info: You can also use any array_info to initialize a memory_stack, make shure the array data has the same lifetime

inline Memory_Stack make_memory_stack(Memory_Allocator *allocator, memory_units size, memory_units alignment = MAX_ALIGNMENT) {
    return { CAST_P(u8, allocate(allocator, size, alignment)), size };
}

inline Memory_Stack begin_packed_allocation(Memory_Allocator *allocator, memory_units max_size, memory_units alignment = MAX_ALIGNMENT) {
    memory_chunk chunk = GET_FREE_CHUNK_FUNCTION_NAME(allocator, max_size, alignment);
    return { chunk.data, chunk.count };
}

inline any end_packed_allocation(Memory_Allocator *allocator, Memory_Stack *begin_packed_allocation_stack, memory_units alignment = MAX_ALIGNMENT) {
    if (begin_packed_allocation_stack->count == 0)
        return NULL;
    
    memory_chunk chunk = { begin_packed_allocation_stack->data, begin_packed_allocation_stack->count };
    
    allocate_free_chunk(allocator, chunk, alignment);
    return begin_packed_allocation_stack->data;
}

inline ALLOCATE_DEC(ALLOCATE_FUNCTION_NAME, Memory_Stack *stack) {
    assert(size && alignment);
    
    if (stack->count % alignment)
        stack->count += alignment - (stack->count % alignment);
    
    assert(stack->count + size <= stack->capacity);
    u8 *data = get_end(stack);
    stack->count += size;
    
    return data;
}

inline FREE_DEC(FREE_FUNCTION_NAME, Memory_Stack *stack) {
    assert(stack->data <= data && data < stack->data + stack->count); // you have to use free in inverse order of allocatetion to work properly with Memory_Stack
    stack->count = CAST_V(memory_units, CAST_P(u8, data) - stack->data);
}

inline GET_FREE_CHUNK_DEC(GET_FREE_CHUNK_FUNCTION_NAME, Memory_Stack *stack) {
    assert(alignment);
    
    memory_chunk chunk = { get_end(stack), stack->capacity - stack->count };
    
    if (stack->count % alignment) {
        memory_units offset = alignment - (stack->count % alignment);
        chunk.data += offset;
        chunk.count -= offset;
    }
    
    if (chunk.count > max_size)
        chunk.count= max_size;	
    
    return chunk;
}

inline ALLOCATE_FREE_CHUNK_DEC(ALLOCATE_FREE_CHUNK_FUNCTION_NAME, Memory_Stack *stack) {
    assert(alignment);
    any debug_data = ALLOCATE_FUNCTION_NAME(stack, chunk.count, alignment);
    assert(debug_data == chunk.data);
}

inline memory_units get_remaining_size(Memory_Stack *stack) {
    return stack->capacity - stack->count;
}

#endif // MEMORY_STACK_H

