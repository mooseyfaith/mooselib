#if !defined(_MEMORY_LIST_H_)
#define _MEMORY_LIST_H_

struct Memory_List {
    Memory_Allocator *backup_allocator;
    
    struct Item {
        any data;
        u32 size; // not memory_units (save space)
        Item *next;
    } *free_list_head, *allocated_list_head;
};


#define TEMPLATE_ALLOCATOR_TYPE Memory_List
#define TEMPLATE_ALLOCATOR_NAME memory_list
#include "memory_template.h"

inline Memory_List make_memory_list(Memory_Allocator *backup_allocator) {
    return { backup_allocator };
}

ALLOCATE_DEC(ALLOCATE_FUNCTION_NAME, Memory_List *list) {
    auto it = &list->free_list_head;
    while (*it) {
        if (((*it)->size == size) && ((MEMORY_ADDRESS((*it)->data) % alignment) == 0)) {
            auto data = (*it)->data;
            *it = (*it)->next;
            return data;
        }
        
        it = &((*it)->next);
    }
    
    auto item = ALLOCATE(list->backup_allocator, Memory_List::Item);	
    *item = { ALLOCATE_FUNCTION_NAME(list->backup_allocator, size, alignment), CAST_V(u32, size), list->allocated_list_head };	
    list->allocated_list_head = item;
    
    return item->data;	
}

FREE_DEC(FREE_FUNCTION_NAME, Memory_List *list) {
    auto it = &list->allocated_list_head;
    while (*it) {
        if ((*it)->data == data) {
            auto item = *it;
            *it = (*it)->next;
            
            item->next = list->free_list_head;
            list->free_list_head = item;
            return;
        }
        
        it = &((*it)->next);
    }
    
    assert(0); // data is not in allocated_list_head
}

GET_FREE_CHUNK_DEC(GET_FREE_CHUNK_FUNCTION_NAME, Memory_List *list) {
    max_size += sizeof(Memory_List::Item);
    if (sizeof(Memory_List::Item) % alignment)
        max_size += alignment - (sizeof(Memory_List::Item) % alignment);
    
    auto it = &list->free_list_head;
    while (*it) {
        if (((*it)->size == max_size) && ((MEMORY_ADDRESS((*it)->data) % alignment) == 0)) {
            *data = (*it)->data;
            *size = max_size;
            return;
        }
        
        it = &((*it)->next);
    }
    
    GET_FREE_CHUNK_FUNCTION_NAME(list->backup_allocator, data, size, max_size, alignof(Memory_List::Item));
    u8 *u8_data = CAST_P(u8, *data);
    u8_data += sizeof(Memory_List::Item);
    if (sizeof(Memory_List::Item) % alignment) {
        u8_data += alignment - (sizeof(Memory_List::Item) % alignment);
        *size -= alignment - (sizeof(Memory_List::Item) % alignment);
    }
    *data = u8_data;
}

ALLOCATE_FREE_CHUNK_DEC(ALLOCATE_FREE_CHUNK_FUNCTION_NAME, Memory_List *list) {
    auto it = &list->free_list_head;
    while (*it) {
        if ((*it)->data == data) {
            assert((MEMORY_ADDRESS((*it)->data) % alignment) == 0);
            assert(size <= (*it)->size);
            *it = (*it)->next;
            return;
        }
        
        it = &((*it)->next);
    }
    
    auto total_data = CAST_P(u8, data) - sizeof(Memory_List::Item);
    memory_units total_size = size + sizeof(Memory_List::Item); 
    if (sizeof(Memory_List::Item) % alignment) {
        total_data -= alignment - (sizeof(Memory_List::Item) % alignment);	
        total_size += alignment - (sizeof(Memory_List::Item) % alignment);		
    }
    
    auto item = CAST_P(Memory_List::Item, total_data);
    *item = { data, CAST_V(u32, size), list->allocated_list_head };	
    list->allocated_list_head = item;
    
    ALLOCATE_FREE_CHUNK_FUNCTION_NAME(list->backup_allocator, total_data, total_size, alignment);
}

#endif // _MEMORY_LIST_H_


