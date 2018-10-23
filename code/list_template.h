
#if !defined LIST_TEMPLATE_NAME
#   error LIST_TEMPLATE_NAME needs to be defined befor including list_template.h
#endif

#if !defined LIST_TEMPLATE_DATA_TYPE
#   error LIST_TEMPLATE_DATA_TYPE needs to be defined befor including list_template.h
#endif

#if !defined LIST_TEMPLATE_DATA_NAME
#   define LIST_TEMPLATE_DATA_NAME value
#endif

#if !defined LIST_TEMPLATE_WITHOUT_COUNT
#   if !defined LIST_TEMPLATE_COUNT_TYPE
#      define LIST_TEMPLATE_COUNT_TYPE u32
#   endif
#else
#   if defined LIST_TEMPLATE_COUNT_TYPE
#      error spedifying LIST_TEMPLATE_COUNT_TYPE together with LIST_TEMPLATE_WITHOUT_COUNT is not allowed
#   endif
#endif

struct LIST_TEMPLATE_NAME {
    
    struct Entry {
        LIST_TEMPLATE_DATA_TYPE LIST_TEMPLATE_DATA_NAME;
        
        union {
            struct {
                Entry *next;
                
#if defined LIST_TEMPLATE_HAS_DOUBLE_LINKS
                union { Entry *prev, *previous; };
#endif
            };
            
#if defined LIST_TEMPLATE_HAS_DOUBLE_LINKS
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
            
#if defined LIST_TEMPLATE_HAS_TAIL
            Entry *tail;
#endif
        };
        
#if defined LIST_TEMPLATE_HAS_TAIL
        Entry *marks[2];
#else
        Entry *marks[1];
#endif
    };
    
#if defined LIST_TEMPLATE_COUNT_TYPE
    LIST_TEMPLATE_COUNT_TYPE count;
#endif
};

static inline LIST_TEMPLATE_DATA_TYPE * _insert(LIST_TEMPLATE_NAME *list, u32 mark_index, LIST_TEMPLATE_NAME::Entry *entry, u32 insert_prev = 0)
{
    LIST_TEMPLATE_NAME::Entry **mark = list->marks + mark_index;
    u32 const forward = insert_prev;
    
#if defined LIST_TEMPLATE_HAS_DOUBLE_LINKS
    u32 const backwards = 1 - insert_prev;
#else
    assert(insert_prev == 0);
#endif
    
    entry->links[forward] = null;
    
#if defined LIST_TEMPLATE_HAS_DOUBLE_LINKS
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
    
#if defined LIST_TEMPLATE_COUNT_TYPE
    ++list->count;
#endif
    
    return &entry->LIST_TEMPLATE_DATA_NAME;
}

static inline LIST_TEMPLATE_DATA_TYPE * _insert(LIST_TEMPLATE_NAME *list, u32 mark_index, LIST_TEMPLATE_DATA_TYPE value, Memory_Allocator *allocator, u32 insert_prev = 0)
{
    LIST_TEMPLATE_NAME::Entry *entry = ALLOCATE(allocator, LIST_TEMPLATE_NAME::Entry);
    entry->LIST_TEMPLATE_DATA_NAME = value;
    return _insert(list, mark_index, entry, insert_prev);
}

static inline LIST_TEMPLATE_DATA_TYPE * insert_head(LIST_TEMPLATE_NAME *list, LIST_TEMPLATE_NAME::Entry *entry, u32 insert_prev = 0)
{
    return _insert(list, 0, entry, insert_prev);
}

static inline LIST_TEMPLATE_DATA_TYPE * insert_head(LIST_TEMPLATE_NAME *list, LIST_TEMPLATE_DATA_TYPE value, Memory_Allocator *allocator, u32 insert_after = 0)
{
    return _insert(list, 0, value, allocator, insert_after);
}

static inline LIST_TEMPLATE_NAME::Entry * _remove(LIST_TEMPLATE_NAME *list, u32 mark_index)
{
    LIST_TEMPLATE_NAME::Entry **mark = list->marks + mark_index;
    assert(*mark);
    
#if defined LIST_TEMPLATE_HAS_DOUBLE_LINKS
    u32 const backwards = 1 - mark_index;
#else
    assert(mark_index == 0);
#endif
    
    u32 const forward = mark_index;
    
    LIST_TEMPLATE_NAME::Entry *entry = *mark;
    
    bool clear_ends = !list->head->next;
    
    *mark = (*mark)->links[forward];
    
#if defined LIST_TEMPLATE_HAS_DOUBLE_LINKS
    if (*mark)
        (*mark)->links[backwards] = null;
#endif
    
    if (clear_ends) {
        for (u32 i = 0; i < ARRAY_COUNT(list->marks); ++i)
            list->marks[i] = null;
    }
    
#if defined LIST_TEMPLATE_COUNT_TYPE
    --list->count;
#endif
    
    return entry;
}

static inline LIST_TEMPLATE_DATA_TYPE _remove(LIST_TEMPLATE_NAME *list, u32 mark_index, Memory_Allocator *allocator)
{
    LIST_TEMPLATE_NAME::Entry *entry = _remove(list, mark_index);
    LIST_TEMPLATE_DATA_TYPE value = entry->LIST_TEMPLATE_DATA_NAME;
    free(allocator, entry);
    
    return value;
}

static inline LIST_TEMPLATE_NAME::Entry * remove_head(LIST_TEMPLATE_NAME *list)
{
    return _remove(list, 0);
}

static inline LIST_TEMPLATE_DATA_TYPE remove_head(LIST_TEMPLATE_NAME *list, Memory_Allocator *allocator)
{
    return _remove(list, 0, allocator);
}

#if defined LIST_TEMPLATE_HAS_TAIL

static inline LIST_TEMPLATE_DATA_TYPE * insert_tail(LIST_TEMPLATE_NAME *list, LIST_TEMPLATE_NAME::Entry *entry, u32 insert_prev = 0)
{
    return _insert(list, 1, entry, insert_prev);
}

static inline LIST_TEMPLATE_DATA_TYPE * insert_tail(LIST_TEMPLATE_NAME *list, LIST_TEMPLATE_DATA_TYPE value, Memory_Allocator *allocator, u32 insert_prev = 0)
{
    return _insert(list, 1, value, allocator, insert_prev);
}

static inline LIST_TEMPLATE_NAME::Entry * remove_tail(LIST_TEMPLATE_NAME *list)
{
    return _remove(list, 1);
}

static inline LIST_TEMPLATE_DATA_TYPE remove_tail(LIST_TEMPLATE_NAME *list, Memory_Allocator *allocator)
{
    return _remove(list, 1, allocator);
}

#endif // LIST_TEMPLATE_HAS_TAIL

// NOTE: this will only work with a random access allocator
// like c_allocator or memory_map
static inline void clear(LIST_TEMPLATE_NAME *list, Memory_Allocator *allocator)
{
    while (list->head)
        remove_head(list, allocator);
}

#undef LIST_TEMPLATE_NAME
#undef LIST_TEMPLATE_DATA_TYPE
#undef LIST_TEMPLATE_DATA_NAME

#if defined LIST_TEMPLATE_HAS_TAIL
#   undef LIST_TEMPLATE_HAS_TAIL
#endif 

#if defined LIST_TEMPLATE_HAS_DOUBLE_LINKS
#   undef LIST_TEMPLATE_HAS_DOUBLE_LINKS
#endif

#if defined LIST_TEMPLATE_WITHOUT_COUNT
#   undef LIST_TEMPLATE_WITHOUT_COUNT
#else
#   undef LIST_TEMPLATE_COUNT_TYPE
#endif
