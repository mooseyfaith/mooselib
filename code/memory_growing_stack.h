#if !defined MEMORY_GROWING_STACK_H
#define MEMORY_GROWING_STACK_H

#include "memory_stack.h"

#define LIST_TEMPLATE_NAME      Memory_Stack_List
#define LIST_TEMPLATE_DATA_TYPE Memory_Stack
#define LIST_TEMPLATE_DATA_NAME memory_stack
#define LIST_TEMPLATE_HAS_TAIL
#define LIST_TEMPLATE_HAS_DOUBLE_LINKS
#include "list_template.h"

struct Memory_Growing_Stack {
    Memory_Allocator allocator;
    Memory_Allocator *internal_allocator;
    Memory_Stack_List list;
    
    usize min_chunk_capacity;
    usize count;
    usize capacity;
    usize max_count;
    usize frame_byte_count;
};

#define Template_Allocator_Type Memory_Growing_Stack
#define Template_Allocator_Name memory_growing_stack
#define Template_Allocator_Kind Memory_Allocator_Growing_Stack_Kind
#include "memory_allocator_template.h"

INTERNAL Memory_Growing_Stack make_memory_growing_stack(Memory_Allocator *internal_allocator, usize min_chunk_capacity = 4 << 10)
{
    Memory_Growing_Stack result = {};
    result.allocator = { Memory_Allocator_Growing_Stack_Kind };
    result.internal_allocator = internal_allocator;
    result.min_chunk_capacity = min_chunk_capacity;
    
    return result;
}

INTERNAL usize _count_change_begin(Memory_Stack_List::Entry *entry) {
    return entry->memory_stack.buffer.count;
}

INTERNAL void _count_change_end(Memory_Growing_Stack *stack, Memory_Stack_List::Entry *entry, usize old_count) {
    stack->count += entry->memory_stack.buffer.count - old_count;
    stack->max_count = MAX(stack->max_count, stack->count);
}

#define SCOPE_UPDATE_COUNT(stack, entry) \
usize _old_count ## __LINE__ = _count_change_begin(entry); \
defer { _count_change_end(stack, entry, _old_count ## __LINE__); };

INTERNAL ALLOCATE_DEC(Memory_Growing_Stack *stack)
{
    assert_bounds(*stack);
    
    bool allocate_new_entry = !stack->list.tail;
    
    if (!allocate_new_entry) {
        auto tail_stack = &stack->list.tail->memory_stack;
        
        auto info = debug_make_footer(*tail_stack, size, alignment);
        
        // TODO: check for overflow ...
#if defined DEBUG
        if (tail_stack->buffer.count + info.size + info.padding + sizeof(info) > tail_stack->buffer.capacity)
            allocate_new_entry = true;
#else
        if (tail_stack->buffer.count + info.size + info.padding > tail_stack->buffer.capacity)
            allocate_new_entry = true;
#endif
    }
    
    if (allocate_new_entry) {
#if defined DEBUG
        usize internal_size = MAX(stack->min_chunk_capacity, size + sizeof(Memory_Stack_List::Entry) + alignment + sizeof(DEBUG_Memory_Stack_Footer));
#else
        usize internal_size = MAX(stack->min_chunk_capacity, size + sizeof(Memory_Stack_List::Entry) + alignment);
#endif
        
        u8 *internal_data = cast_p(u8, allocate(stack->internal_allocator, internal_size, alignof(Memory_Stack_List::Entry)));
        
        Memory_Stack_List::Entry *entry = CAST_P(Memory_Stack_List::Entry, internal_data);
        internal_data += sizeof(Memory_Stack_List::Entry);
        internal_size -= sizeof(Memory_Stack_List::Entry);
        
        entry->memory_stack.buffer.data = internal_data;
        entry->memory_stack.buffer.count = 0;
        entry->memory_stack.buffer.capacity = internal_size;
        
        stack->capacity += internal_size;
        insert_tail(&stack->list, entry);
    }
    
    SCOPE_UPDATE_COUNT(stack, stack->list.tail);
    any data = allocate(&stack->list.tail->memory_stack, size, alignment);
    
    stack->frame_byte_count += size;
    
    return data;
}

INTERNAL void _free_empty_tails(Memory_Growing_Stack *stack, bool force = false) {
    if (!force && (stack->list.count <= 1))
        return;
    
    while (stack->list.tail && !stack->list.tail->memory_stack.buffer.count) {
        Memory_Stack_List::Entry *entry = remove_tail(&stack->list);
        
        stack->capacity -= entry->memory_stack.buffer.capacity;
        
        free(stack->internal_allocator, entry);
    }
}

INTERNAL FREE_DEC(Memory_Growing_Stack *stack) {
    assert_bounds(*stack);
    
    assert(stack->list.tail);
    
    {
        SCOPE_UPDATE_COUNT(stack, stack->list.tail);
        free(&stack->list.tail->memory_stack, data);
    }
    
    _free_empty_tails(stack);
}

INTERNAL REALLOCATE_DEC(Memory_Growing_Stack *stack) {
    assert_bounds(*stack);
    
    assert(stack->list.tail);
    
    usize old_size;
    {
        usize old_tail_count = _count_change_begin(stack->list.tail);
        defer { _count_change_end(stack, stack->list.tail, old_tail_count); };
        
        if (reallocate(&stack->list.tail->memory_stack, data, size, alignment))
            return true;
        
#if defined DEBUG
        auto info = debug_footer(stack->list.tail->memory_stack);
        old_size = info->size;
#endif
        
        // don't use own free or we might free empty stacks
        free(&stack->list.tail->memory_stack, *data);
        
#if !defined DEBUG
        old_size = old_tail_count - stack->list.tail->memory_stack.buffer.count;
#endif
    }
    
    auto new_data = allocate(stack, size, alignment);
    COPY(new_data, *data, MIN(old_size, size));
    *data = new_data;
    
    if (size > old_size)
        stack->frame_byte_count += size - old_size;
    
    // now its ok to free empty stacks
    auto real_tail = remove_tail(&stack->list);
    _free_empty_tails(stack, true);
    insert_tail(&stack->list, real_tail);
    
    return true;
}

// keeps the space, but tries to allocate only one chunk
INTERNAL void clear(Memory_Growing_Stack *stack)
{
    assert_bounds(*stack);
    
    defer { stack->frame_byte_count = 0; };
    
    if ((stack->capacity >= stack->max_count) && (stack->list.count <= 1)) {
        if (stack->list.head)
            stack->list.head->memory_stack.buffer.count = 0;
        
        stack->count    = 0;
        return;
    }
    
    while (stack->list.tail)
        remove_tail(&stack->list, stack->internal_allocator);
    
    // this will probaly fail, sicne there seems to be a bug in list_template
    assert(!stack->list.count && !stack->list.head && !stack->list.tail);
    
    usize capacity = stack->max_count;
    
    stack->count    = 0;
    stack->capacity = 0;
    allocate(stack, capacity, MEMORY_MAX_ALIGNMENT);
    // size is reserved but not used
    stack->count = 0;
    stack->list.tail->memory_stack.buffer.count = 0;
}

#undef SCOPE_UPDATE_COUNT

#endif // MEMORY_GROWING_STACK_H