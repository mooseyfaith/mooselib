#if !defined(_MEMORY_MAP_H_)
#define _MEMORY_MAP_H_

enum Memory_Page_State {
    Page_State_Empty_Mask = 0,
    Page_State_Used_Flag = 1,
    Page_State_Chained_Flag = 2,
    Page_State_Used_Mask = Page_State_Used_Flag,
    Page_State_Chained_Mask = Page_State_Chained_Flag | Page_State_Used_Flag,
};

struct Memory_Map {
    u8 *page_states;
    u8 *data;
    memory_units page_size;
    memory_units page_count;
};

#define MEMORY_ALLOCATOR_TEMPLATE_TYPE          Memory_Map
#define MEMORY_ALLOCATOR_TEMPLATE_INTERNAL_NAME memory_map
#include "memory_allocator_template.h"

// you really shouldn't thouch the alignment here ...
Memory_Map make_memory_map(Memory_Allocator *allocator, memory_units page_count, memory_units page_size, memory_units alignment = MEMORY_MAX_ALIGNMENT) {
    assert(page_count && page_size);
    assert((page_size % alignment) == 0);
    
    Memory_Map map;
    map.page_count = page_count;
    map.page_size = page_size;
    
    memory_units total_size = page_count * (page_size + 1);
    
    // ensure that map.data is aligned, by inserting some space after map.page_states if nessesary
    memory_units padding;
    if (page_count % alignment) {
        padding = alignment - (page_count % alignment);
        total_size += padding;
    }
    else {
        padding = 0;
    }
    
    // if page_states and data are aligned the same way, we can guarantee the padding size
    // otherwiese we coul allways add 1 * alignment to the totalsize, wich might waste a little more in the end
    // either way we will not throw more than 2 * alignment bytes away
    map.page_states = CAST_P(u8, allocate(allocator, sizeof(u8) * total_size, alignment));
    for (u32 i = 0; i < map.page_count; ++i)
        map.page_states[i] = Page_State_Empty_Mask;
    
    map.data = map.page_states + page_count + padding;
    
    return map;
}

inline memory_units size_to_page_count(Memory_Map *map, memory_units size) {
    memory_units chunk_count = size / map->page_size;
    if (size % map->page_size)
        ++chunk_count;
    
    return chunk_count;
}

static inline void _mark_chained_used_pages(Memory_Map *map, memory_units first_page, memory_units page_count) {
    u8 *it = map->page_states + first_page;
    for (u8 *end = it + page_count - 1; it != end; ++it) {
        assert((*it & Page_State_Chained_Mask) == Page_State_Empty_Mask);
        *it = Page_State_Chained_Mask; // next chunk is chained to this
    }
    
    assert((*it & Page_State_Chained_Mask) == Page_State_Empty_Mask);
    *it = Page_State_Used_Mask; // mark as last in chain
}

ALLOCATE_DEC(Memory_Map *map) {
    assert(size && alignment);
    
    memory_units required_page_count = size_to_page_count(map, size);
    
    memory_units free_page_count = 0;
    memory_units first_free_page;
    
    for (u8 *it = map->page_states, *end = it + map->page_count; it != end; ++it) {
        if ((*it & Page_State_Chained_Mask) == Page_State_Empty_Mask) {
            if (free_page_count == 0)
                first_free_page = CAST_V(memory_units, it - map->page_states);
            
            ++free_page_count;
            
            if (free_page_count == required_page_count)
                break;
        }
        else {
            free_page_count = 0;
        }
    }
    
    assert(free_page_count == required_page_count);
    _mark_chained_used_pages(map, first_free_page, required_page_count);
    
    u8 *pointer = map->data + first_free_page * map->page_size;
    assert((MEMORY_ADDRESS(pointer) % alignment) == 0); // data will allways be aligned with chunk begin, check your make_memory_map alignment
    
    return pointer;
}

// Note: maybe save map alignment in struct
FREE_DEC(Memory_Map *map) {
    memory_units first_chunk = CAST_V(memory_units, CAST_P(u8, data) - map->data) / map->page_size;
    
    assert(first_chunk < map->page_count);
    assert((((u8 *)data - map->data) % map->page_size) == 0); // data begins at oage, note maybe not good?
    
    // clear marked chunks, search for chained chunks
    {
        u8 *it = map->page_states + first_chunk;
        assert(*it & Page_State_Used_Flag);
        
        while (CHECK_MASK(*it, Page_State_Chained_Mask)) {
            *(it++) = Page_State_Empty_Mask;
            assert(it < map->page_states + map->page_count);
        }
        
        assert(*it & Page_State_Used_Flag);
        *it = Page_State_Empty_Mask;
    }
}

REALLOCATE_DEC(Memory_Map *map) {
    memory_units first_used_chunk = CAST_V(memory_units, CAST_P(u8, *data) - map->data) / map->page_size;
    
    assert(map->page_states[first_used_chunk] & Page_State_Used_Flag);
    
    memory_units required_page_count = size_to_page_count(map, size);
    
    memory_units used_page_count = 0;
    for (memory_units page_index = first_used_chunk;page_index < map->page_count && map->page_states[page_index] & Page_State_Used_Flag; ++page_index)
    {
        ++used_page_count;
    }
    
    memory_units free_page_count = used_page_count;
    for (memory_units page_index = first_used_chunk + used_page_count;page_index < map->page_count && !(map->page_states[page_index] & Page_State_Used_Flag); ++page_index)
    {
        ++free_page_count;
    }
    
    memory_units first_free_chunk = first_used_chunk;
    while (first_free_chunk > 0 && (map->page_states[first_free_chunk - 1] == Page_State_Empty_Mask))
        --first_free_chunk;
    
    free_page_count += first_used_chunk -  first_free_chunk;
    
    if (required_page_count <= used_page_count) {
        map->page_states[first_used_chunk + required_page_count - 1] = Page_State_Used_Mask;
        
        for (memory_units page_index = first_used_chunk + required_page_count; page_index < first_used_chunk + used_page_count; ++page_index)
            map->page_states[page_index] = Page_State_Empty_Mask;
        
        return;
    }
    
    if (required_page_count <= free_page_count - (first_used_chunk - first_free_chunk))
    {
        for (memory_units page_index = first_used_chunk + used_page_count - 1; page_index < first_used_chunk + required_page_count - 1; ++page_index)
            map->page_states[page_index] = Page_State_Chained_Mask;
        
        map->page_states[first_used_chunk + required_page_count - 1] = Page_State_Used_Mask;
        
        return;
    }
    
    if (required_page_count <= free_page_count) {
        COPY(map->data + first_free_chunk * map->page_size, *data, used_page_count * map->page_size);
        
        memory_units page_index = first_free_chunk;
        for (; page_index < first_free_chunk + required_page_count - 1; ++page_index)
            map->page_states[page_index] = Page_State_Chained_Mask;
        
        map->page_states[page_index] = Page_State_Used_Mask;
        
        for (; page_index < first_used_chunk + used_page_count; ++page_index)
            map->page_states[page_index] = Page_State_Empty_Mask;
        
        return;
    }
    
    any new_data = allocate(map, size, alignment);
    COPY(new_data, *data, used_page_count * map->page_size);
    free(map, *data);
    *data = new_data;
}

#endif // _MEMORY_MAP_H_


