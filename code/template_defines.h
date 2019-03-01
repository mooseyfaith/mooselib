
#if !defined TEMPLATE_DEFINES_H
#define TEMPLATE_DEFINES_H

#define TEMPLATE_LINK_DEC(entry_type) \
union { \
    entry_type *next; \
    entry_type *links[1]; \
}

#define TEMPLATE_DOUBLE_LINKS_DEC(entry_type) \
union { \
    struct { \
        entry_type *next; \
        union { entry_type *prev, *previous; }; \
    }; \
    entry_type *links[2]; \
}

#define TEMPLATE_HEAD_DEC(entry_type) \
union { \
    entry_type *head; \
    entry_type *marks[1]; \
}

#define TEMPLATE_HEAD_AND_TAIL_DEC(entry_type) \
union { \
    struct { entry_type *head, *tail; }; \
    entry_type *marks[2]; \
}

#define TEMPLATE_DOUBLE_LIST_TYPE_DEC(name, entry_type) \
struct name { \
    TEMPLATE_HEAD_AND_TAIL_DEC(entry_type); \
    usize count; \
}

#define TEMPLATE_PARENT_DEC(type) type *parent

#define TEMPLATE_TREE_LEAF_DEC(type) \
TEMPLATE_PARENT_DEC(type); \
TEMPLATE_DOUBLE_LINKS_DEC(type);

#define TEMPLATE_TREE_DEC(type) \
TEMPLATE_TREE_LEAF_DEC(type); \
TEMPLATE_DOUBLE_LIST_TYPE_DEC(Children, type) children;

#endif // TEMPLATE_DEFINES_H

