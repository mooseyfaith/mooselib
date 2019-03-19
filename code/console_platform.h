
#if defined WIN32

#include "win32_platform.h"

typedef Win32_Platform_API Console_Platform_API;

Platform_API * init_console_platform(Console_Platform_API *win32_api) {
    init_win32_api(win32_api);
    auto platform_api = &win32_api->platform_api;
    
    init_c_allocator();
    init_Memory_Growing_Stack_allocators();
    
    return platform_api;
}

#endif