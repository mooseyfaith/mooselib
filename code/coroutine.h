#if !defined(_ENGINE_COROUTINE_H_)
#define _ENGINE_COROUTINE_H_

#include "basic.h"

// WARNING: TROUBLESHOOTING:
// does not work with vs edit and continue debug info (settings->c/c++->general->debug info)

// Thanks to Simon Tatham
// see https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html

// set to 0, makes it easy for default initialazation
#define CO_INITIAL_STATE 0

// set to 1, will never be a valid line number inside a reasonable formated source file ...
// setting to -1 might break the code because of casting from int to CO_STATE type
// indicates that the coroutine has finished
#define CO_INVALID_STATE 1

#if 0
// heap allocated state coroutines ... ----------------------------------------

struct Coroutine_State_Header {
    void *local_memory;
    unsigned int local_memory_size;
    unsigned short line_nr;
};

#define _CO_STRUCT_NAME(name) Coroutine_ ## name ## _State

#define _CO_FUNC_DEC(return_type, name, ...) return_type name(Coroutine_State_Header *_co_state, ## __VA_ARGS__)

#define COROUTINE_DEC(return_type, name, ...) \
typedef _CO_FUNC_DEC(return_type, (*Coroutine_ ## name ## _Function), ## __VA_ARGS__); \
struct _CO_STRUCT_NAME(name) { \
    Coroutine_State_Header header; \
    Coroutine_ ## name  ## _Function call; \
}; \
_CO_FUNC_DEC(return_type, name, ## __VA_ARGS__)

#define COROUTINE_IMP(return_type, name, ...) _CO_FUNC_DEC(return_type, name, __VA_ARGS__)

#define CO_STATE_DEC(coroutine_name) _CO_STRUCT_NAME(coroutine_name) // = { {}; coroutine_name; };

#define CO_ALLOCATE(co_state, coroutine_name, allocator, ...) { \
    (co_state)->header = {}; \
    (co_state)->call = coroutine_name; \
    (co_state)->call((Coroutine_State_Header *)(co_state), ## __VA_ARGS__); \
    (co_state)->header.local_memory = (void *)ALLOCATE_ARRAY(allocator, unsigned char, (co_state)->header.local_memory_size); \
}

#define CO_FREE(co_state, allocator) { free(allocator, (co_state)->header.local_memory); (co_state)->header = {});
    
#define CO_RESUME(co_state, ...) (co_state)->call((Coroutine_State_Header *)(co_state), __VA_ARGS__)
    
#define CO_EX_BEGIN() \
    char _co_stack_dump_begin; \
    if (_co_state->local_memory) \
    { memcpy((void *)&_co_stack_dump_begin, _co_state->local_memory, _co_state->local_memory_size); } \
    else \
    { goto _co_init_memory_size; } \
    switch (_co_state->line_nr) { case CO_INITIAL_STATE:
        
        // never use inside a switch statement!
#define CO_EX_YIELD(...) { \
            _co_state->line_nr = (__LINE__); \
            assert(_co_state->local_memory); \
            memcpy(_co_state->local_memory, (void *)&_co_stack_dump_begin, _co_state->local_memory_size); \
            return __VA_ARGS__; \
            case (__LINE__):; \
        }
        
#define CO_EX_END() \
        _co_state->line_nr = CO_INVALID_STATE; \
        break; \
        case CO_INVALID_STATE: \
        default: \
        assert(0); } \
    char _co_stack_dump_end; \
    _co_init_memory_size: \
    _co_state->local_memory_size = (unsigned int)(&_co_stack_dump_end - &_co_stack_dump_begin) - 1;// - sizeof(unsigned int);
    
#endif
    
#define CO_BEGIN(state) { \
        auto _co_state = state; \
        switch (*_co_state) { \
            case CO_INVALID_STATE: \
            default:					\
            assert(0);				\
            case CO_INITIAL_STATE:			
            
#define CO_YIELD(...) {		    \
                *_co_state = __LINE__;	    \
                return __VA_ARGS__;	    \
                case __LINE__:;		    \
            }
            
#define CO_RETURN(...) {			\
                *_co_state = CO_INVALID_STATE;	\
                return __VA_ARGS__;	       	\
            }
            
#define CO_CHECKPOINT { \
                *_co_state = __LINE__; \
                case __LINE__:; }
            
            // continues from last yield or last check_point
#define CO_CONTINUE(...) return __VA_ARGS__;
            
            // would really love to get rid of this ...
#define CO_END  *_co_state = CO_INVALID_STATE; }}
    
#endif
    