
#if !defined Template_Tree_Name
#   error Template_Tree_Name needs to be defined befor including template_tree.h
#endif

#if !defined Template_Tree_Data_Type
#   error Template_Tree_Data_Type needs to be defined befor including template_tree.h
#endif

#if !defined Template_Tree_Data_Name
#   define Template_Tree_Data_Name value
#endif

struct Template_Tree_Name;

struct CHAIN(Template_Tree_Name, _List) {
    union {
        struct { Template_Tree_Name *head, *tail; };
        Template_Tree_Name *marks[2];
    };
    
    usize count;
};

struct Template_Tree_Name {
    Template_Tree_Data_Type Template_Tree_Data_Name;
    Template_Tree_Name *parent;
    
    union {
        struct { Template_Tree_Name *next, *prev; };
        Template_Tree_Name *links[2];
    };
    
    CHAIN(Template_Tree_Name, _List) children;
};

#define Template_List_Name      CHAIN(Template_Tree_Name, _List)
#define Template_List_Data_Type Template_Tree_Data_Type
#define Template_List_Data_Name Template_Tree_Data_Name
#define Template_List_With_Double_Links
#define Template_List_With_Tail
#define Template_List_Struct_Is_Declared
#define Template_List_Custom_Entry_Name Template_Tree_Name
#include "template_list.h"

INTERNAL bool
is_ancesotor(Template_Tree_Name *ancestor, Template_Tree_Name *decendent)
{
    assert(ancestor != decendent);
    
    auto current = decendent;
    
    while (current) {
        if (current->parent == ancestor)
            return true;
        
        current = current->parent;
    }
    
    return false;
}

INTERNAL void
attach(Template_Tree_Name *new_parent, Template_Tree_Name *child)
{
    assert(!child->parent);
    
    insert_tail(&new_parent->children, child);
    child->parent = new_parent;
}

INTERNAL void
detach(Template_Tree_Name *child)
{
    assert(child->parent);
    remove(&child->parent->children, child);
    child->parent = null;
    child->prev = null;
    child->next = null;
}

INTERNAL void
move(Template_Tree_Name *new_parent, Template_Tree_Name *child)
{
    assert(!is_ancesotor(child, new_parent));
    
    if (child->parent)
        detach(child);
    
    attach(new_parent, child);
}

INTERNAL bool
next(Template_Tree_Name **iterator, usize *depth = null)
{
    if ((*iterator)->children.head)
    {
        *iterator = (*iterator)->children.head;
        
        if (depth)
            (*depth)++;
        
        return true;
    }
    else {
        do {
            if ((*iterator)->next)
            {
                *iterator = (*iterator)->next;
                return true;
            }
            else {
                *iterator = (*iterator)->parent;
                
                if (depth)
                    (*depth)--;
            }
        } while (*iterator);
        
        return false;
    }
}

INTERNAL void
advance(Template_Tree_Name **iterator, bool *did_enter, bool *did_leave, usize *depth = null) {
    if (!*did_leave && (*iterator)->children.head) {
        (*iterator) = (*iterator)->children.head;
        *did_enter = true;
        
        if (depth)
            (*depth)++;
    }
    else {
        if ((*iterator)->next) {
            (*iterator) = (*iterator)->next;
            *did_enter = true;
        }
        else {
            (*iterator) = (*iterator)->parent;
            *did_enter = false;
            *did_leave = true;
            
            if (depth)
                (*depth)--;
        }
    }
    
    // if iterator has no children, it enters and leaves in one go
    if (*iterator && *did_enter)
        *did_leave = !(*iterator)->children.head;
}

INTERNAL Template_Tree_Name *
find_next_node(Template_Tree_Name **iterator, Template_Tree_Data_Type Template_Tree_Data_Name, usize *depth = null) {
    assert(*iterator);
    
    while (*iterator) {
        // so we can advance iterator for the next call
        Template_Tree_Name *found = null;
        if (are_equal(&(*iterator)->Template_Tree_Data_Name, &Template_Tree_Data_Name, sizeof(Template_Tree_Data_Name)))
            found = (*iterator);
        
        next(iterator, depth);
        
        if (found)
            return found;
    }
    
    return null;
}

#undef Template_Tree_Name
#undef Template_Tree_Data_Type
#undef Template_Tree_Data_Name
