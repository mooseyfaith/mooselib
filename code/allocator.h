#if !defined ALLOCATOR_H
#define ALLOCATOR_H

const usize Max_Memory_Alignment = 16;

struct Allocator_Header {
    u32 kind;
};

typedef Allocator_Header *Allocator;

enum Allocator_Kind {
    Allocator_Kind_Platform = 0,
    Allocator_Kind_C,
    Allocator_Kind_Stack,
    Allocator_Kind_Chain,
    Allocator_Kind_Count,
};


#define Allocator_Resize_Interface(name, param) bool name(param, u8_array *chunk, usize new_byte_count, usize byte_alignment, bool allow_copy)

typedef Allocator_Resize_Interface((*Allocator_Resize_Function), Allocator allocator);

Allocator_Resize_Function global_allocator_resize_table[Allocator_Kind_Count];

Allocator_Resize_Interface(resize, Allocator allocator) {
    return global_allocator_resize_table[allocator->kind](allocator, chunk, new_byte_count, byte_alignment, allow_copy);
}


#define NEW(allocator, type) \
cast_p(type, allocate(allocator, sizeof(type), alignof(type)).data)

#define FREE(allocator, data) \
deallocate(allocator, { cast_p(u8, data), sizeof(*data) })

#define GROW(allocator, type, array, grow_count) { \
    u8_array chunk = { cast_p(u8, (array)->data), byte_count(*array) }; \
    reallocate(allocator, &chunk, ((array)->count + grow_count) * sizeof(type), alignof(type)); \
    (array)->data = cast_p(type, chunk.data); \
    (array)->count = chunk.count / sizeof(type); \
}

#define SHRINK(allocator, type, array, shrink_count) { \
    assert(shrink_count <= (array)->count); \
    u8_array chunk = { cast_p(u8, (array)->data), byte_count(*array) }; \
    reallocate(allocator, &chunk, ((array)->count - shrink_count) * sizeof(type), alignof(type)); \
    (array)->data = cast_p(type, chunk.data); \
    (array)->count = chunk.count / sizeof(type); \
}

INTERNAL u8_array allocate(Allocator allocator, usize byte_count, usize byte_alignment)
{
    u8_array chunk = {};
    auto ok = resize(allocator, &chunk, byte_count, byte_alignment, false);
    assert(ok);
    
    return chunk;
}

INTERNAL bool deallocate(Allocator allocator, u8_array data) {
    auto ok = resize(allocator, &data, 0, 0, false);
    assert(ok);
    return ok;
}

INTERNAL bool reallocate(Allocator allocator, u8_array *chunk, usize new_byte_count, usize byte_alignment = 0)
{
    bool ok = resize(allocator, chunk, new_byte_count, byte_alignment, true);
    assert(ok);
    return ok;
    
    assert(chunk->data || ok);
    
    if (!ok)
    {
        u8_array new_chunk = allocate(allocator, new_byte_count, byte_alignment);
        copy(new_chunk.data, chunk->data, min(new_chunk.count, chunk->count));
        deallocate(allocator, *chunk);
        
        *chunk = new_chunk;
    }
    
    return ok;
}

struct Stack_Allocator_Footer {
    usize total_byte_count;
    u16 data_padding;
};

struct Stack_Allocator : Allocator_Header {
    u8_buffer buffer;
    usize used_count;
};

Allocator_Resize_Interface(resize, Stack_Allocator *stack)
{
    assert(chunk->count || new_byte_count);
    
    u8 *low;
    if (chunk->count)
    {
        // check if *data is in valid bounds
        index_of(stack->buffer, chunk->data);
        
#if defined DEBUG
        
        auto top_footer = cast_p(Stack_Allocator_Footer, one_past_last(stack->buffer)) - 1;
        low = one_past_last(stack->buffer) - top_footer->total_byte_count;
        
        u8 *top_data = low + top_footer->data_padding;
        assert(top_data == chunk->data, "you can not free and allocate randomly\n\tyou have to free in inverse allocation order");
        
#else
        
        if (try_index_of(stack->buffer, chunk->data) == -1)
            return false;
        
        low = chunk->data;
        
#endif
        
    }
    else {
        low = one_past_last(stack->buffer);
    }
    
    auto freed_byte_count = cast_v(usize, one_past_last(stack->buffer) - low);
    
    if (new_byte_count)
    {
        assert(byte_alignment);
        
        auto high = low;
        auto padding = MOD(byte_alignment - MOD(memory_address(high), byte_alignment), byte_alignment);
        high += padding + new_byte_count;
        
        assert(high > low, "well, you passed a negative or very high new_byte_count");
        
#if defined DEBUG
        high += sizeof(Stack_Allocator_Footer);
#endif
        
        if (high <= stack->buffer.data + stack->buffer.capacity)
        {
            stack->buffer.count = cast_v(usize, high - stack->buffer.data);
            chunk->data = low + padding;
            
            stack->used_count -= freed_byte_count;
            stack->used_count += cast_v(usize, one_past_last(stack->buffer) - chunk->data);
            
#if defined DEBUG
            auto top_footer = cast_p(Stack_Allocator_Footer, one_past_last(stack->buffer)) - 1;
            top_footer->total_byte_count = high - low;
            top_footer->data_padding = padding;
            assert(top_footer->data_padding == padding);
#endif
            chunk->count = new_byte_count;
            return true;
        }
        
        // we did nothing
        return false;
    }
    else
    {
        // we freed *data
        stack->buffer.count = cast_v(usize, low - stack->buffer.data);
        stack->used_count -= freed_byte_count;
        
        if (stack->used_count == 0)
            stack->buffer.count = 0;
        
        chunk->count = new_byte_count;
        
        return true;
    }
}

Allocator_Resize_Interface(stack_allocator_resize_wrapper, Allocator allocator) {
    return resize(cast_p(Stack_Allocator, allocator), chunk, new_byte_count, byte_alignment, allow_copy);
}

#define Template_List_Name      Allocator_List
#define Template_List_Data_Type Allocator
#define Template_List_Data_Name allocator
#include "template_list.h"

struct Chain_Allocator : Allocator_Header {
    Allocator internal_allocator;
    
    Allocator_List list;
    
    usize min_entry_count;
    
    usize count, capacity;
    usize max_count;
};

INTERNAL usize padding_of(u8 *data, usize byte_alignment) {
    usize padding = memory_address(data) % byte_alignment;
    if (padding)
        padding = byte_alignment - padding;
    
    return padding;
}

#define CHAIN_UPDATE_COUNT(allocator, stack) \
auto _old_count_## __LINE__ = (stack)->buffer.count; \
\
defer { \
    (allocator)->count -= _old_count_## __LINE__; \
    (allocator)->count += (stack)->buffer.count; \
    \
    if ((allocator)->count > (allocator)->max_count) \
    (allocator)->max_count = (allocator)->count; \
};

Allocator_Resize_Interface(resize, Chain_Allocator *allocator)
{
    if (allocator->list.head)
    {
        auto stack = cast_p(Stack_Allocator, allocator->list.head->allocator);
        
        bool ok;
        {
            CHAIN_UPDATE_COUNT(allocator, stack);
            ok = resize(stack, chunk, new_byte_count, byte_alignment, false);
        }
        
        if (ok) {
            if (stack->buffer.count == 0) {
                allocator->capacity -= stack->buffer.capacity;
                auto entry = remove_head(&allocator->list);
                deallocate(allocator->internal_allocator, { cast_p(u8, entry), 1 });
            }
            
            return true;
        }
        
        if (!allow_copy)
            return false;
    }
    
    {
        auto count = max(allocator->min_entry_count, new_byte_count + sizeof(Allocator_List::Entry) + sizeof(Stack_Allocator) + byte_alignment);
        
        auto entry_chunk = allocate(allocator->internal_allocator, count, 1);
        
        auto entry = next_item(&entry_chunk, Allocator_List::Entry);
        auto stack = next_item(&entry_chunk, Stack_Allocator);
        stack->buffer.data     = entry_chunk.data;
        stack->buffer.capacity = entry_chunk.count;
        stack->buffer.count    = 0;
        entry->allocator = cast_v(Allocator, stack);
        
        allocator->capacity += stack->buffer.capacity;
        
        u8_array new_chunk = {};
        bool ok;
        {
            CHAIN_UPDATE_COUNT(allocator, stack);
            ok = resize(stack, &new_chunk, new_byte_count, byte_alignment, false);
        }
        assert(ok);
        
        if (chunk->count && allow_copy)
        {
            assert(allocator->list.head);
            
            copy(new_chunk.data, chunk->data, min(new_chunk.count, chunk->count));
            
            // still uses old head
            auto stack = cast_p(Stack_Allocator, allocator->list.head->allocator);
            
            {
                CHAIN_UPDATE_COUNT(allocator, stack);
                deallocate(stack, *chunk);
            }
            
            if (stack->buffer.count == 0) {
                allocator->capacity -= stack->buffer.capacity;
                auto entry = remove_head(&allocator->list);
                deallocate(allocator->internal_allocator, { cast_p(u8, entry), 1 });
            }
        }
        
        *chunk = new_chunk;
        
        // new we add new head
        insert_head(&allocator->list, entry);
        
        return ok;
    }
}

Allocator_Resize_Interface(chain_allocator_resize_wrapper, Allocator allocator) {
    return resize(cast_p(Chain_Allocator, allocator), chunk, new_byte_count, byte_alignment, allow_copy);
}

Allocator_Header Win32_Allocator = { Allocator_Kind_Platform };

Allocator_Resize_Interface(win32_allocator_resize, Allocator allocator)
{
    u8 *new_data = null;
    if (new_byte_count)
    {
        if (chunk->count && !allow_copy)
            return false;
        
        new_data = cast_p(u8, VirtualAlloc(0, new_byte_count, MEM_COMMIT, PAGE_READWRITE));
        if (!new_data)
            return false;
        assert(padding_of(new_data, byte_alignment) == 0);
    }
    
    if (chunk->count && new_data)
        copy(new_data, chunk->data, min(new_byte_count, chunk->count));
    
    if ((new_byte_count == 0) || (chunk->count && allow_copy))
        VirtualFree(chunk->data, 0, MEM_RELEASE);
    
    if (new_data)
        chunk->data = new_data;
    
    chunk->count = new_byte_count;
    
    return true;
}

#endif // ALLOCATOR_H