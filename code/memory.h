#if !defined MEMORY_H
#define MEMORY_H

#if !defined BASIC_H
#error please include "basic.h" befor including any "memory"-prefixed file
#endif

#define MEMORY_MAX_ALIGNMENT 16

// WARNING about free,
// since its spelled the same as c's free, it can be a pitfall, that you might forget to pass the allocator
// e.g. you might use free(pointer) instead of free(allocator, pointer)
// while this will cause an exception or crash, there is no helpful message from the compiler ...

// function interfaces

#define _ALLOCATE_NAMED_DEC(name, allocator_param) \
any name(allocator_param, usize size, usize alignment)

#define _REALLOCATE_NAMED_DEC(name, allocator_param) \
bool name(allocator_param, any *data, usize size, usize alignment)

#define _FREE_NAMED_DEC(name, allocator_param) \
void name(allocator_param, any data)

// allocator interfaces

#define ALLOCATE_DEC(allocator_param) \
_ALLOCATE_NAMED_DEC(allocate, allocator_param)

#define REALLOCATE_DEC(allocator_param) \
_REALLOCATE_NAMED_DEC(reallocate, allocator_param)

#define FREE_DEC(allocator_param) \
_FREE_NAMED_DEC(free, allocator_param)

// type macros

#define ALLOCATE(allocator, type) \
CAST_P(type, allocate(allocator, sizeof(type), alignof(type)))

#define ALLOCATE_ARRAY(allocator, type, count) \
CAST_P(type, allocate(allocator, sizeof(type) * (count), alignof(type)))

#define ALLOCATE_ARRAY_WITH_COUNT(allocator, type, count) \
ALLOCATE_ARRAY(allocator, type, count), count

#define ALLOCATE_ARRAY_INFO(allocator, type, count) \
{ ALLOCATE_ARRAY_WITH_COUNT(allocator, type, count) }

#define REALLOCATE_ARRAY(allocator, data, count) \
reallocate(allocator, CAST_P(any, data), sizeof(**(data)) * (count), alignof(decltype(**(data))))

// wrappers

#define _ALLOCATE_WRAPPER_FUNCTION_NAME(allocator_name) \
CHAIN(allocator_name, _allocate_wrapper)

#define _REALLOCATE_WRAPPER_FUNCTION_NAME(allocator_name) \
CHAIN(allocator_name, _reallocate_wrapper)

#define _FREE_WRAPPER_FUNCTION_NAME(allocator_name) \
CHAIN(allocator_name, _free_wrapper)

#define _ALLOCATE_WRAPPER_DEC(allocator_name) \
_ALLOCATE_NAMED_DEC(_ALLOCATE_WRAPPER_FUNCTION_NAME(allocator_name), Memory_Allocator *allocator)

#define _REALLOCATE_WRAPPER_DEC(allocator_name) \
_REALLOCATE_NAMED_DEC(_REALLOCATE_WRAPPER_FUNCTION_NAME(allocator_name), Memory_Allocator *allocator)

#define _FREE_WRAPPER_DEC(allocator_name) \
_FREE_NAMED_DEC(_FREE_WRAPPER_FUNCTION_NAME(allocator_name), Memory_Allocator *allocator)

// Memory_Allocator

// register new allocators here for the global dispatch table
enum Memory_Allocator_Kind {
    Memory_Allocator_Plattform_Kind,
    Memory_Allocator_C_Kind,
    Memory_Allocator_Stack_Kind,
    Memory_Allocator_Growing_Stack_Kind,
    Memory_Allocator_List_Kind,
    Memory_Allocator_Kind_Count
};

struct Memory_Allocator {
    u32 kind;
};

typedef _ALLOCATE_NAMED_DEC((*Memory_Allocate_Function), Memory_Allocator *allocator);
typedef _REALLOCATE_NAMED_DEC((*Memory_Reallocate_Function), Memory_Allocator *allocator);
typedef _FREE_NAMED_DEC((*Memory_Free_Function), Memory_Allocator *allocator);

Memory_Allocate_Function   global_allocate_functions[Memory_Allocator_Kind_Count];
Memory_Reallocate_Function global_reallocate_functions[Memory_Allocator_Kind_Count];
Memory_Free_Function       global_free_functions[Memory_Allocator_Kind_Count];

void assert_no_clash(any destination, any source) {
    assert(!destination || (destination == source)); 
}

void init_allocators(u32 kind, Memory_Allocate_Function allocate, Memory_Reallocate_Function reallocate, Memory_Free_Function free) {
    // just increase MEMORY_ALLOCATOR_COUNT in basic.h
    assert(kind < Memory_Allocator_Kind_Count);
    
    // possibly two allocators have the same id
    assert_no_clash(global_allocate_functions[kind], allocate);
    assert_no_clash(global_reallocate_functions[kind], reallocate);
    assert_no_clash(global_free_functions[kind], free);
    
    global_allocate_functions[kind]   = allocate;
    global_reallocate_functions[kind] = reallocate;
    global_free_functions[kind]       = free;
}

ALLOCATE_DEC(Memory_Allocator *allocator) {
    assert(allocator->kind < Memory_Allocator_Kind_Count);
    return global_allocate_functions[allocator->kind](allocator, size, alignment);
}

REALLOCATE_DEC(Memory_Allocator *allocator) {
    assert(allocator->kind < Memory_Allocator_Kind_Count);
    return global_reallocate_functions[allocator->kind](allocator, data, size, alignment);
}

FREE_DEC(Memory_Allocator *allocator) {
    assert(allocator->kind < Memory_Allocator_Kind_Count);
    global_free_functions[allocator->kind](allocator, data);
}

#endif // MEMORY_H
