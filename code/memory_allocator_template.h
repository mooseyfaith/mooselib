#include "memory.h"

#if !defined Template_Allocator_Type
#error Template_Allocator_Type needs to be defined befor including memory_allocator_template.h
#endif

#if !defined Template_Allocator_Name
#error Template_Allocator_Name needs to be defined befor including memory_allocator_template.h
#endif

#if !defined Template_Allocator_Kind
#error Template_Allocator_Kind needs to be defined befor including memory_allocator_template.h, \
a unique number from 0 to Memory_Allocator_Kind_Count - 1
#endif

ALLOCATE_DEC(Template_Allocator_Type *Template_Allocator_Name);
REALLOCATE_DEC(Template_Allocator_Type *Template_Allocator_Name);
FREE_DEC(Template_Allocator_Type *Template_Allocator_Name);

// wrapper functions

_ALLOCATE_WRAPPER_DEC(Template_Allocator_Name)
{
    return allocate(cast_p(Template_Allocator_Type, allocator), size, alignment);
}

_REALLOCATE_WRAPPER_DEC(Template_Allocator_Name)
{
    return reallocate(cast_p(Template_Allocator_Type, allocator), data, size, alignment);
}

_FREE_WRAPPER_DEC(Template_Allocator_Name)
{
    free(cast_p(Template_Allocator_Type, allocator), data);
}

void CHAIN(CHAIN(init_, Template_Allocator_Name), _allocators)()
{
    init_allocators(Template_Allocator_Kind, _ALLOCATE_WRAPPER_FUNCTION_NAME(Template_Allocator_Name),_REALLOCATE_WRAPPER_FUNCTION_NAME(Template_Allocator_Name), _FREE_WRAPPER_FUNCTION_NAME(Template_Allocator_Name));
}

#undef Template_Allocator_Type
#undef Template_Allocator_Name
#undef Template_Allocator_Kind
