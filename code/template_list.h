
#if !defined Template_List_Name
#   error Template_List_Name needs to be defined befor including list_template.h
#endif

#if !defined Template_List_Data_Type
#   error Template_List_Data_Type needs to be defined befor including list_template.h
#endif

#if !defined Template_List_Data_Name
#   define Template_List_Data_Name value
#endif

#if !defined Template_List_Without_Count
#   if !defined Template_List_Count_Type
#      define Template_List_Count_Type u32
#   endif
#else
#   if defined Template_List_Count_Type
#      error spedifying Template_List_Count_Type together with Template_List_Without_Count is not allowed
#   endif
#endif

struct Template_List_Name {
    
    struct Entry {
        Template_List_Data_Type Template_List_Data_Name;
        
        union {
            struct {
                Entry *next;
                
#if defined Template_List_With_Double_Links
                union { Entry *prev, *previous; };
#endif
            };
            
#if defined Template_List_With_Double_Links
            Entry *links[2];
#else
            Entry *links[1];
#endif
        };
        
        // maybe add optinal index?
    };
    
    union {
        struct {
            Entry *head;
            
#if defined Template_List_With_Tail
            Entry *tail;
#endif
        };
        
#if defined Template_List_With_Tail
        Entry *marks[2];
#else
        Entry *marks[1];
#endif
    };
    
#if defined Template_List_Count_Type
    Template_List_Count_Type count;
#endif
};

#define for_list_item(iterator, list) \
for (auto iterator = (list).head; iterator; iterator = iterator->next)

INTERNAL Template_List_Data_Type * 
_insert(Template_List_Name *list, u32 mark_index, Template_List_Name::Entry *entry, u32 insert_prev = 0)
{
    Template_List_Name::Entry **mark = list->marks + mark_index;
    u32 const forward = insert_prev;
    
#if defined Template_List_With_Double_Links
    u32 const backwards = 1 - insert_prev;
#else
    assert(insert_prev == 0);
#endif
    
    entry->links[forward] = null;
    
#if defined Template_List_With_Double_Links
    entry->links[backwards] = *mark;
#endif
    
    if (*mark) {
        (*mark)->links[forward] = entry;
        *mark = entry;
    }
    else {
        for (u32 i = 0; i < ARRAY_COUNT(list->marks); ++i)
            list->marks[i] = entry;
    }
    
#if defined Template_List_Count_Type
    ++list->count;
#endif
    
    return &entry->Template_List_Data_Name;
}

INTERNAL Template_List_Data_Type * 
insert_head(Template_List_Name *list, Template_List_Name::Entry *entry, u32 insert_prev = 0)
{
    return _insert(list, 0, entry, insert_prev);
}

INTERNAL Template_List_Name::Entry * 
_remove(Template_List_Name *list, u32 mark_index)
{
    Template_List_Name::Entry **mark = list->marks + mark_index;
    assert(*mark);
    
#if defined Template_List_With_Double_Links
    u32 const backwards = 1 - mark_index;
#else
    assert(mark_index == 0);
#endif
    
    u32 const forward = mark_index;
    
    Template_List_Name::Entry *entry = *mark;
    
    bool clear_ends = !list->head->next;
    
    *mark = (*mark)->links[forward];
    
#if defined Template_List_With_Double_Links
    if (*mark)
        (*mark)->links[backwards] = null;
#endif
    
    if (clear_ends) {
        for (u32 i = 0; i < ARRAY_COUNT(list->marks); ++i)
            list->marks[i] = null;
    }
    
#if defined Template_List_Count_Type
    --list->count;
#endif
    
    return entry;
}

INTERNAL Template_List_Name::Entry * 
remove_head(Template_List_Name *list)
{
    return _remove(list, 0);
}

#if defined Template_List_With_Tail

INTERNAL Template_List_Data_Type * 
insert_tail(Template_List_Name *list, Template_List_Name::Entry *entry, u32 insert_prev = 0)
{
    return _insert(list, 1, entry, insert_prev);
}

INTERNAL Template_List_Name::Entry * 
remove_tail(Template_List_Name *list)
{
    return _remove(list, 1);
}

#endif // Template_List_With_Tail

INTERNAL void 
insert_after(Template_List_Name *list, Template_List_Name::Entry *at_entry, Template_List_Name::Entry *new_entry)
{
    if (at_entry)
    {
        
#if defined Template_List_With_Double_Links
        if (at_entry->next)
            at_entry->next->prev = new_entry;
        
        new_entry->prev = at_entry;
#endif
        
        new_entry->next = at_entry->next;
        at_entry->next = new_entry;
    }
    else {
        new_entry->next = null;
        
        list->head = new_entry;
        
#if defined Template_List_With_Tail
        list->tail = new_entry;
#endif
        
    }
}

#if defined Template_List_With_Double_Links

INTERNAL void remove(Template_List_Name *list, Template_List_Name::Entry *it) {
    if (it->prev)
        it->prev->next = it->next;
    else
        list->head = it->next;
    
    if (it->next)
        it->next->prev = it->prev;
    
#if defined Template_List_With_Tail
    
    else
        list->tail = it->prev;
    
#endif
    
}

#endif // Template_List_With_Double_Links


#if defined MEMORY_H

INTERNAL Template_List_Data_Type * 
_insert(Memory_Allocator *allocator, Template_List_Name *list, u32 mark_index, Template_List_Data_Type value, u32 insert_prev = 0)
{
    Template_List_Name::Entry *entry = ALLOCATE(allocator, Template_List_Name::Entry);
    entry->Template_List_Data_Name = value;
    return _insert(list, mark_index, entry, insert_prev);
}

INTERNAL Template_List_Data_Type * 
insert_head(Memory_Allocator *allocator, Template_List_Name *list, Template_List_Data_Type value, u32 insert_after = 0)
{
    return _insert(allocator, list, 0, value, insert_after);
}


INTERNAL Template_List_Data_Type 
_remove(Memory_Allocator *allocator, Template_List_Name *list, u32 mark_index)
{
    Template_List_Name::Entry *entry = _remove(list, mark_index);
    Template_List_Data_Type value = entry->Template_List_Data_Name;
    free(allocator, entry);
    
    return value;
}

INTERNAL Template_List_Data_Type
remove_head(Memory_Allocator *allocator, Template_List_Name *list)
{
    return _remove(allocator, list, 0);
}

#  if defined Template_List_With_Tail

INTERNAL Template_List_Data_Type * 
insert_tail(Memory_Allocator *allocator, Template_List_Name *list, Template_List_Data_Type value, u32 insert_prev = 0)
{
    return _insert(allocator, list, 1, value, insert_prev);
}

#    if defined Template_List_With_Double_Links

INTERNAL Template_List_Data_Type 
remove_tail(Memory_Allocator *allocator, Template_List_Name *list)
{
    return _remove(allocator, list, 1);
}

#    endif

#  endif

// NOTE: this will only work with a random access allocator
// like c_allocator or memory_map
INTERNAL void
clear(Memory_Allocator *allocator, Template_List_Name *list)
{
    while (list->head)
        remove_head(allocator, list);
}

#endif // MEMORY_H


#undef Template_List_Name
#undef Template_List_Data_Type
#undef Template_List_Data_Name

#if defined Template_List_With_Tail
#   undef Template_List_With_Tail
#endif 

#if defined Template_List_With_Double_Links
#   undef Template_List_With_Double_Links
#endif

#if defined Template_List_Without_Count
#   undef Template_List_Without_Count
#else
#   undef Template_List_Count_Type
#endif
