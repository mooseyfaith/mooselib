#if !defined MEMORY_LIST_H
#define MEMORY_LIST_H

#define Template_List_Name      Chunk_List
#define Template_List_Data_Type u8_array
#define Template_List_Data_Name chunk
#define Template_List_With_Tail
#define Template_List_With_Double_Links
#include "template_list.h"

struct Memory_List {
    Memory_Allocator allocator;
    Memory_Allocator *internal_allocator;
    
    Chunk_List used_list, free_list;
};

#define Template_Allocator_Name Memory_List
#define Template_Allocator_Kind Memory_Allocator_List_Kind
#include "template_memory_allocator.h"

INTERNAL Memory_List make_memory_list(Memory_Allocator *internal_allocator)
{
    Memory_List result = {};
    result.allocator.kind     = Memory_Allocator_List_Kind;
    result.internal_allocator = internal_allocator;
    
    return result;
}

INTERNAL void insert_to_free_list(Memory_List *list, Chunk_List::Entry *new_entry)
{
    auto prev_entry = list->free_list.head;
    
    for_list_item(it, list->free_list)
    {
        if (it->chunk.data + it->chunk.count == new_entry->chunk.data)
        {
            it->chunk.count += new_entry->chunk.count;
            
            auto next = it->next;
            if (next && (it->chunk.data + it->chunk.count == next->chunk.data))
            {
                it->chunk.count += next->chunk.count;
                remove(&list->free_list, next);
            }
            
            return;
        }
        else if (new_entry->chunk.data + new_entry->chunk.count == it->chunk.data)
        {
            it->chunk.data = new_entry->chunk.data;
            it->chunk.count += new_entry->chunk.count;
            return;
        }
        
        if (!it->next || (new_entry <= it->next)) {
            prev_entry = it;
            break;
        }
    }
    
    _insert(&list->free_list, prev_entry, new_entry, 1);
    //insert_after(&list->free_list, prev_entry, new_entry);
}

INTERNAL ALLOCATE_DEC(Memory_List *list)
{
    for_list_item(it, list->free_list)
    {
        usize required_size = size + sizeof(Chunk_List::Entry);
        auto data = it->chunk.data + sizeof(Chunk_List::Entry);
        
        usize offset = MOD(alignment - MOD(memory_address(data), alignment), alignment);
        required_size += offset;
        data += offset;
        
        if (it->chunk.count >= required_size)
        {
            remove(&list->free_list, it);
            insert_head(&list->used_list, it);
            
            if (it->chunk.count >= required_size)
            {
                auto free_entry = cast_p(Chunk_List::Entry, it->chunk.data + required_size);
                free_entry->chunk.data  = cast_p(u8, free_entry);
                free_entry->chunk.count = it->chunk.count - required_size;
                it->chunk.count -= required_size;
                
                insert_to_free_list(list, free_entry);
            }
            
            return data;
        }
    }
    
    u8_array new_chunk = ALLOCATE_ARRAY_INFO(list->internal_allocator, u8, size + sizeof(Chunk_List::Entry) + alignment);
    
    auto new_entry = cast_p(Chunk_List::Entry, new_chunk.data);
    
    new_entry->chunk = new_chunk;
    insert_head(&list->used_list, new_entry);
    
    auto data = new_entry->chunk.data + sizeof(Chunk_List::Entry);
    data += MOD(alignment - MOD(memory_address(data), alignment), alignment);
    
    return data;
}

INTERNAL FREE_DEC(Memory_List *list)
{
    for_list_item(it, list->used_list)
    {
        if (try_index_of(it->chunk, cast_p(u8, data)) != -1)
        {
            remove(&list->used_list, it);
            insert_to_free_list(list, it);
            return;
        }
    }
    
    assert(0, "data was not allocated by this allocator");
}

INTERNAL REALLOCATE_DEC(Memory_List *list)
{
    bool is_big_enough = false;
    u8 *data_end = null;
    
    Chunk_List::Entry *used_entry;
    
    for_list_item(it, list->used_list)
    {
        usize index = try_index_of(it->chunk, cast_p(u8, data));
        
        if (index != -1)
        {
            data_end = it->chunk.data + index + size;
            if (it->chunk.data + it->chunk.count >= data_end)
                is_big_enough = true;
            
            used_entry = it;
            break;
        }
    }
    
    assert(used_entry, "data was not allocated by this allocator");
    
    if (!is_big_enough)
    {
        for_list_item(it, list->free_list)
        {
            if (used_entry->chunk.data + used_entry->chunk.count == it->chunk.data)
            {
                remove(&list->free_list, it);
                used_entry->chunk.count += it->chunk.count;
                
                if (used_entry->chunk.data + used_entry->chunk.count >= data_end)
                    is_big_enough = true;
                
                break;
            }
            else if (!it || (used_entry >= it->next))
                break;
        }
    }
    
    if (is_big_enough) 
    {
        auto new_size = index_of(used_entry->chunk, data_end);
        
        usize remaining_size = used_entry->chunk.count - new_size;
        
        if (remaining_size > sizeof(Chunk_List::Entry))
        {
            auto new_entry = cast_p(Chunk_List::Entry, used_entry->chunk.data + new_size);
            new_entry->chunk.data  = cast_p(u8, new_entry);
            new_entry->chunk.count = remaining_size;
            
            used_entry->chunk.count = new_size;
            
            insert_to_free_list(list, new_entry);
        }
        
        return true;
    }
    
    used_entry->chunk.count = cast_v(usize, data_end - used_entry->chunk.data);
    bool ok = reallocate(list->internal_allocator, &cast_any(used_entry->chunk.data), used_entry->chunk.count, 1);
    if (!ok)
        return false;
    
    if (used_entry->prev)
        used_entry->prev->next = used_entry;
    else
        list->used_list.head = used_entry;
    
    if (used_entry->next)
        used_entry->next->prev = used_entry;
    else
        list->used_list.tail = used_entry;
    
    return true;
}

#endif // MEMORY_LIST_H