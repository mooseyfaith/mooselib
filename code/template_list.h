
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

static inline Template_List_Data_Type * _insert(Template_List_Name *list, u32 mark_index, Template_List_Name::Entry *entry, u32 insert_prev = 0)
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

static inline Template_List_Data_Type * _insert(Template_List_Name *list, u32 mark_index, Template_List_Data_Type value, Memory_Allocator *allocator, u32 insert_prev = 0)
{
    Template_List_Name::Entry *entry = ALLOCATE(allocator, Template_List_Name::Entry);
    entry->Template_List_Data_Name = value;
    return _insert(list, mark_index, entry, insert_prev);
}

static inline Template_List_Data_Type * insert_head(Template_List_Name *list, Template_List_Name::Entry *entry, u32 insert_prev = 0)
{
    return _insert(list, 0, entry, insert_prev);
}

static inline Template_List_Data_Type * insert_head(Template_List_Name *list, Template_List_Data_Type value, Memory_Allocator *allocator, u32 insert_after = 0)
{
    return _insert(list, 0, value, allocator, insert_after);
}

static inline Template_List_Name::Entry * _remove(Template_List_Name *list, u32 mark_index)
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

static inline Template_List_Data_Type _remove(Template_List_Name *list, u32 mark_index, Memory_Allocator *allocator)
{
    Template_List_Name::Entry *entry = _remove(list, mark_index);
    Template_List_Data_Type value = entry->Template_List_Data_Name;
    free(allocator, entry);
    
    return value;
}

static inline Template_List_Name::Entry * remove_head(Template_List_Name *list)
{
    return _remove(list, 0);
}

static inline Template_List_Data_Type remove_head(Template_List_Name *list, Memory_Allocator *allocator)
{
    return _remove(list, 0, allocator);
}

#if defined Template_List_With_Tail

static inline Template_List_Data_Type * insert_tail(Template_List_Name *list, Template_List_Name::Entry *entry, u32 insert_prev = 0)
{
    return _insert(list, 1, entry, insert_prev);
}

static inline Template_List_Data_Type * insert_tail(Template_List_Name *list, Template_List_Data_Type value, Memory_Allocator *allocator, u32 insert_prev = 0)
{
    return _insert(list, 1, value, allocator, insert_prev);
}

static inline Template_List_Name::Entry * remove_tail(Template_List_Name *list)
{
    return _remove(list, 1);
}

static inline Template_List_Data_Type remove_tail(Template_List_Name *list, Memory_Allocator *allocator)
{
    return _remove(list, 1, allocator);
}

#endif // Template_List_With_Tail

// NOTE: this will only work with a random access allocator
// like c_allocator or memory_map
static inline void clear(Template_List_Name *list, Memory_Allocator *allocator)
{
    while (list->head)
        remove_head(list, allocator);
}

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
