
#if 0

// usage:

#define Template_List_Name      Node_List
#define Template_List_Data_Type Node *
#define Template_List_Data_Name node // optional
#define Template_List_With_Tail // optional
#define Template_List_With_Double_Links // optional
#define Template_List_Count_Type usize  // optional
#include "template_list.h"

// or

struct Node {
    u32 size;
    string name;
    // ...
    
    Node *parent;
    
    // required
    union {
        struct { Node *next, *prev; };
        Node *links[2];
    }
};

struct Node_List {
    // ...
    
    // required
    union {
        struct { Node *head, tail; };
        Node *marks[2];
    }
    
    usize count;
};

#define Template_List_Name      Node_List
#define Template_List_Data_Type Node *
#define Template_List_Data_Name parent
#define Template_List_With_Tail
#define Template_List_With_Double_Links
#define Template_List_Count_Type usize
#define Template_List_Struct_Is_Declared
#define Template_List_Entry_Name Node

#include "template_list.h"

#endif

#if !defined Template_List_Name
#   error Template_List_Name needs to be defined befor including template_list.h
#endif

#if !defined Template_List_Without_Count
#   if !defined Template_List_Count_Type
#      define Template_List_Count_Type usize
#   endif
#else
#   if defined Template_List_Count_Type
#      error spedifying Template_List_Count_Type together with Template_List_Without_Count is not allowed
#   endif
#endif

#if !defined Template_List_Struct_Is_Declared

#if !defined Template_List_Data_Type
#   error Template_List_Data_Type needs to be defined befor including template_list.h
#endif

#if !defined Template_List_Data_Name
#   define Template_List_Data_Name value
#endif

struct Template_List_Name {
    
#if !defined Template_List_Custom_Entry_Name
    
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
        
        // maybe add optioal index?
    };
    
#  define Template_List_Custom_Entry_Name Template_List_Name::Entry
    
#endif // Template_List_Custom_Entry_Name
    
    
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

#else

#  undef Template_List_Struct_Is_Declared

#  if !defined Template_List_Custom_Entry_Name
#    error Template_List_Custom_Entry_Name musst be defined if Template_List_Struct_Is_Declared is defined.
#  endif

#endif // Template_List_Struct_Is_Declared

#define for_list_item(iterator, list) \
for (auto iterator = (list).head; iterator; iterator = iterator->next)

#if defined Template_List_With_Double_Links

INTERNAL void
insert(Template_List_Name *list, Template_List_Custom_Entry_Name *at, Template_List_Custom_Entry_Name *entry, u32 insert_after_at = 0)
{
    if (!list->head) {
        assert(at == null);
        for (u32 i = 0; i < ARRAY_COUNT(list->marks); i++) {
            assert(!list->marks[i]);
            list->marks[i] = entry;
        }
        
        entry->next = null;
        entry->prev = null;
    }
    else {
        u32 next = insert_after_at;
        u32 prev = 1 - next;
        
        if (at) {
            if (at->links[prev])
                at->links[prev]->links[next] = entry;
            
            entry->links[prev] = at->links[prev];
            at->links[prev] = entry;
        }
        else {
            entry->links[prev] = null;
        }
        
        entry->links[next] = at;
        
        if (entry->next == list->head)
            list->head = entry;
        
#if defined Template_List_With_Tail
        assert(list->tail);
        
        if (list->tail->next)
            list->tail = list->tail->next;
#endif
    }
    
#if defined Template_List_Count_Type
    list->count++;
#endif
    
    //return &entry->Template_List_Data_Name;
}

#endif // Template_List_With_Double_Links

INTERNAL void
insert_head(Template_List_Name *list, Template_List_Custom_Entry_Name *entry, u32 insert_after_head = 0)
{
    
#if defined Template_List_With_Double_Links
    
    insert(list, list->head, entry, insert_after_head);
    
#else
    
    assert(insert_after_head == 0, "singly linked list only support insertion before head, not after");
    
    entry->next = list->head;
    list->head = entry;
    
#if defined Template_List_With_Tail
    if (!list->tail)
        list->tail = entry;
#endif
    
#if defined Template_List_Count_Type
    list->count++;
#endif
    
    //return &entry->Template_List_Data_Name;
    
#endif // Template_List_With_Double_Links
    
}

INTERNAL Template_List_Custom_Entry_Name * 
remove(Template_List_Name *list, Template_List_Custom_Entry_Name *at)
{
    assert(at);
    
#if defined Template_List_With_Double_Links
    
    if (at->prev)
        at->prev->next = at->next;
    
    if (at->next)
        at->next->prev = at->prev;
    
    if (list->head == at)
        list->head = at->next;
    
    if (list->tail == at)
        list->tail = at->prev;
    
#else
    
    // for singly linked list only removing at head is allowed
    // removing at tail would require iteration through the whole list
    assert(at == list->head, "singly linked list can only remove head");
    list->head = list->head->next;
    
#endif
    
#if defined Template_List_Count_Type
    list->count--;
#endif
    
    return at;
}

INTERNAL Template_List_Custom_Entry_Name * 
remove_head(Template_List_Name *list)
{
    return remove(list, list->head);
}

#if defined Template_List_With_Tail

INTERNAL void
insert_tail(Template_List_Name *list, Template_List_Custom_Entry_Name *entry, u32 insert_after_tail = 1)
{
    
#if defined Template_List_With_Double_Links
    
    insert(list, list->tail, entry, insert_after_tail);
    
#else
    
    assert(insert_after_tail == 1, "singly linked list only support insertion after tail, not before");
    
    entry->next = null;
    
    if (list->tail) {
        assert(list->head);
        list->tail->next = entry;
    }
    else {
        assert(!list->head);
        list->head = entry;
    }
    
    list->tail = entry;
    
#if defined Template_List_Count_Type
    list->count++;
#endif
    
    //return &entry->Template_List_Data_Name;
    
#endif // Template_List_With_Double_Links
    
}

#  if defined Template_List_With_Double_Links

INTERNAL Template_List_Custom_Entry_Name * 
remove_tail(Template_List_Name *list)
{
    return remove(list, list->tail);
}

#  endif // Template_List_With_Double_Links

#endif // Template_List_With_Tail


#if defined MEMORY_H

#  if defined Template_List_With_Double_Links

INTERNAL Template_List_Custom_Entry_Name * 
insert(Memory_Allocator *allocator, Template_List_Name *list, Template_List_Custom_Entry_Name *at, u32 insert_after_at = 0)
{
    Template_List_Custom_Entry_Name *entry = ALLOCATE(allocator, Template_List_Custom_Entry_Name);
    insert(list, at, entry, insert_after_at);
    return entry;
}

#  endif // Template_List_With_Double_Links

INTERNAL Template_List_Custom_Entry_Name *
insert_head(Memory_Allocator *allocator, Template_List_Name *list, u32 insert_after_head = 0)
{
    Template_List_Custom_Entry_Name *entry = ALLOCATE(allocator, Template_List_Custom_Entry_Name);
    insert_head(list, entry, insert_after_head);
    return entry;
}

INTERNAL void 
remove(Memory_Allocator *allocator, Template_List_Name *list, Template_List_Custom_Entry_Name *at)
{
    Template_List_Custom_Entry_Name *entry = remove(list, at);
    assert(entry == at);
    free(allocator, entry);
}

INTERNAL void
remove_head(Memory_Allocator *allocator, Template_List_Name *list)
{
    remove(allocator, list, list->head);
}

#  if defined Template_List_With_Tail

INTERNAL Template_List_Custom_Entry_Name *
insert_tail(Memory_Allocator *allocator, Template_List_Name *list, u32 insert_after_tail = 1)
{
    Template_List_Custom_Entry_Name *entry = ALLOCATE(allocator, Template_List_Custom_Entry_Name);
    insert_tail(list, entry, insert_after_tail);
    return entry;
}

#    if defined Template_List_With_Double_Links

INTERNAL void
remove_tail(Memory_Allocator *allocator, Template_List_Name *list)
{
    remove(allocator, list, list->tail);
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
#undef Template_List_Custom_Entry_Name

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
