
#include "template_defines.h"

#if !defined Template_Tree_Name
#   error Template_Tree_Name needs to be defined befor including template_tree.h
#endif

#if !defined Template_Tree_Struct_Is_Declared

#  if !defined Template_Tree_Data_Type
#    error Template_Tree_Data_Type needs to be defined befor including template_tree.h
#  endif

#  if !defined Template_Tree_Data_Name
#    define Template_Tree_Data_Name value
#  endif

#  if defined Template_Tree_With_Leafs
#    error Template_Tree_With_Leafs must not be defined when Template_Tree_Struct_Is_Declared is not defined
#  endif

struct Template_Tree_Name {
    Template_Tree_Data_Type Template_Tree_Data_Name;
    
    TEMPLATE_TREE_DEC(Template_Tree_Name);
};

#  define Template_List_Name               Template_Tree_Name::Children
#  define Template_List_Data_Type          Template_Tree_Data_Type
#  define Template_List_Data_Name          Template_Tree_Data_Name
#  define Template_List_With_Double_Links
#  define Template_List_With_Tail
#  define Template_List_Struct_Is_Declared
#  define Template_List_Custom_Entry_Name  Template_Tree_Name
#  include "template_list.h"

#else  // !Template_Tree_Struct_Is_Declared

#  if defined Template_Tree_Data_Type
#    error Template_Tree_Data_Type must not be defined if Template_Tree_Struct_Is_Declared is defined
#  endif

#  if defined Template_Tree_Data_Name
#    error Template_Tree_Data_Name must not be defined if Template_Tree_Struct_Is_Declared is defined
#  endif

#  define Template_List_Name               Template_Tree_Name::Children
#  define Template_List_With_Double_Links
#  define Template_List_With_Tail
#  define Template_List_Struct_Is_Declared
#  define Template_List_Custom_Entry_Name  Template_Tree_Name
#  include "template_list.h"

#  undef Template_Tree_Struct_Is_Declared

#endif // !Template_Tree_Struct_Is_Declared

INTERNAL bool
is_ancesotor(Template_Tree_Name *ancestor, Template_Tree_Name *decendent)
{
    assert(ancestor && decendent && (ancestor != decendent));
    
    auto current = decendent;
    
    while (current) {
        if (current->parent == ancestor)
            return true;
        
        current = current->parent;
    }
    
    return false;
}

#if !defined Template_Tree_With_Leafs

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

#else  // !Template_Tree_With_Leafs

INTERNAL void
attach(Template_Tree_Name *new_parent, Template_Tree_Name::Children *new_parent_children, Template_Tree_Name *child)
{
    assert(!child->parent);
    
    insert_tail(new_parent_children, child);
    child->parent = new_parent;
}

INTERNAL void
detach(Template_Tree_Name::Children *parent_children, Template_Tree_Name *child)
{
    assert(child->parent);
    assert(!parent_children->head || (parent_children->head->parent == child->parent));
    
    remove(parent_children, child);
    child->parent = null;
    child->prev = null;
    child->next = null;
}

INTERNAL void
move(Template_Tree_Name *new_parent, Template_Tree_Name::Children *new_parent_children, Template_Tree_Name *child, Template_Tree_Name::Children *parent_children = null)
{
    assert(!is_ancesotor(child, new_parent));
    
    attach(new_parent, new_parent_children, child);
    
    if (child->parent) {
        assert(parent_children);
        detach(parent_children, child);
    }
    
    attach(new_parent, new_parent_children, child);
}

void advance_up_or_right(Template_Tree_Name **iterator, bool *did_enter, usize *depth = null) {
    if ((*iterator)->next) {
        (*iterator) = (*iterator)->next;
        *did_enter = true;
    }
    else {
        (*iterator) = (*iterator)->parent;
        *did_enter = false;
        
        if (depth)
            (*depth)--;
    }
}

void next_up_or_right(Template_Tree_Name **iterator, usize *depth = null, Template_Tree_Name *stop_node = null) {
    bool did_enter;
    do {
        advance_up_or_right(iterator, &did_enter, depth);
    } while ((*iterator != stop_node) && !did_enter);
}

INTERNAL void
advance(Template_Tree_Name **iterator, Template_Tree_Name *iterator_children_head, bool *did_enter, usize *depth = null) {
    if (*did_enter && iterator_children_head) {
        (*iterator) = iterator_children_head;
        *did_enter = true;
        
        if (depth)
            (*depth)++;
    }
    else {
        advance_up_or_right(iterator, did_enter, depth);
    }
}

#  undef Template_Tree_With_Leafs

#endif // !Template_Tree_With_Leafs

#if defined Template_Tree_Data_Type

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

#  undef Template_Tree_Data_Type
#  undef Template_Tree_Data_Name

#endif

#undef Template_Tree_Name