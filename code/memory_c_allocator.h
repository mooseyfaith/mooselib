#if !defined MEMORY_C_ALLOCATOR_H
#define MEMORY_C_ALLOCATOR_H

#include <stdlib.h>

#include "memory.h"

Memory_Allocator _C_Allocator = {
    Memory_Allocator_C_Kind,
};

Memory_Allocator *C_Allocator = &_C_Allocator;

_ALLOCATE_WRAPPER_DEC(c)
{
    assert(allocator->kind == Memory_Allocator_C_Kind);
    u8 *data = CAST_P(u8, malloc(size));
    assert(data, "malloc failed, out of memmory");
    assert((MEMORY_ADDRESS(data) % alignment) == 0, "malloc can not provide your required alignment");
    return data;
}

_REALLOCATE_WRAPPER_DEC(c)
{
    assert(allocator->kind == Memory_Allocator_C_Kind);
    *data = realloc(*data, size);
    assert(*data, "realloc failed, out of memmory");
    return *data;
}

_FREE_WRAPPER_DEC(c)
{
    assert(allocator->kind == Memory_Allocator_C_Kind);
    assert(data);
    free(data);
}

void init_c_allocator() {
    init_allocators(Memory_Allocator_C_Kind, _ALLOCATE_WRAPPER_FUNCTION_NAME(c), _REALLOCATE_WRAPPER_FUNCTION_NAME(c), _FREE_WRAPPER_FUNCTION_NAME(c));
}

#endif // MEMORY_C_ALLOCATOR_H