
#if !defined TEMPLATE_DEFINES_H
#define TEMPLATE_DEFINES_H

#define TEMPLATE_LINK_DEC(entry_type) \
union { \
    entry_type *next; \
    entry_type *links[1]; \
};

#define TEMPLATE_DOUBLE_LINKS_DEC(entry_type) \
union { \
    struct { \
        entry_type *next; \
        union { entry_type *prev, *previous; }; \
    }; \
    entry_type *links[2]; \
};

#define TEMPLATE_HEAD_AND_TAIL_DEC(entry_type) \
union { \
    struct { entry_type *head, *tail; }; \
    entry_type*marks[2]; \
};

#define TEMPLATE_HEAD_DEC(entry_type) \
union { \
    entry_type *head; \
    entry_type *marks[1]; \
};

#define TEMPLATE_PARENT(node_type) \
node_type *parent;

#define TEMPLATE_TREE_DEC(tree_type) \
struct Children { \
    TEMPLATE_HEAD_AND_TAIL_DEC(tree_type); \
    usize count; \
}; \
\
struct _Dummy {} _dummy; \
tree_type *parent; \
TEMPLATE_DOUBLE_LINKS_DEC(tree_type); \
Children children;

#endif // TEMPLATE_DEFINES_H

