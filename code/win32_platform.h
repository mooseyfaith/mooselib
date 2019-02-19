#include <Windows.h>
#include <Windowsx.h> // mouse message macros ... stupid>
#include <Xinput.h>
#include <dsound.h>
#include <intrin.h>

#include "gl.h"
#include "platform.h"
#include "memory_growing_stack.h"
#include "memory_list.h"
#include "memory_c_allocator.h"

struct Win32_Window {
    HWND handle;
    HDC device_context;
    
    string title;
    u32 id;
    bool is_active;
    bool was_destroyed;
    
    bool is_fullscreen;
    bool is_resizeable;
    f32 forced_width_over_height_or_zero;
    
    Pixel_Rectangle old_area;
    Pixel_Rectangle new_area;
    
    WINDOWPLACEMENT placement_backup;
};

#define Template_Array_Name      Win32_Window_Buffer
#define Template_Array_Data_Type Win32_Window
#define Template_Array_Is_Buffer
#include "template_array.h"

struct Application_Info {
    FILETIME last_dll_write_time;
    string dll_path;
    string dll_name;
    string init_name;
    string main_loop_name;
    HMODULE dll;
    App_Init_Function init;
    App_Main_Loop_Function main_loop;
    any data;
};

struct Win32_Worker_Thread_Info {
    Platform_Worker_Queue *queue;
    HANDLE handle;
    u32 index;
};

struct Platform_Worker_Queue {
    Win32_Worker_Thread_Info *thread_infos;
    u32 thread_count;
    HANDLE semaphore_handle;
    
    Platform_Work *works;
    volatile u32 next_index;
    volatile u32 done_count;
    volatile u32 count;
    u32 capacity;
};

struct Win32_Platform_API {
    Platform_API platform_api;
    
    Memory_Allocator allocator;
    Memory_Growing_Stack persistent_memory;
    Memory_Growing_Stack transient_memory;
    Win32_Window_Buffer window_buffer;
    Win32_Window *current_window;
    u32 swap_buffer_count;
    WNDCLASS window_class;
    HGLRC gl_context;
    HINSTANCE windows_instance;
    Game_Input input;
    Application_Info application_info;
    IDirectSoundBuffer *sound_buffer;
    LARGE_INTEGER last_time;
    LARGE_INTEGER ticks_per_second;
    Platform_Worker_Queue worker_queue;
    HANDLE pipe_read, pipe_write;
    string working_directory;
};

Win32_Platform_API *global_win32_api;

INTERNAL bool win32_register_class(WNDCLASS *window_class, char const *name, HINSTANCE instance, WNDPROC window_callback)
{
    *window_class = {};
    
    window_class->lpfnWndProc = window_callback;
    window_class->hInstance = instance;
    window_class->hbrBackground = (HBRUSH)(COLOR_BACKGROUND);
    window_class->lpszClassName = name;
    window_class->style = CS_OWNDC;
    window_class->hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (!RegisterClass(window_class)) {
        printf("RegisterClass error: %d\n", GetLastError());
        return false;
    }
    
    return true;
}

INTERNAL void win32_resize_window(HWND window_handle, int width, int height)
{
    RECT rect;
    GetClientRect(window_handle, &rect);
    int real_width = width * 2 - (rect.right - rect.left);
    int real_height = height * 2 - (rect.bottom - rect.top);
    SetWindowPos(window_handle, HWND_TOP, 0, 0, real_width, real_height, SWP_SHOWWINDOW | SWP_NOMOVE);
}

INTERNAL HWND win32_create_window(WNDCLASS *window_class, string title, s16 width, s16 height, HINSTANCE instance)
{
    char name_buffer[MAX_PATH];
    write_c_string(name_buffer, ARRAY_COUNT(name_buffer), title);
    HWND window = CreateWindow(window_class->lpszClassName, name_buffer, WS_MINIMIZE | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, cast_v(s32, width), cast_v(s32, height), 0, 0, instance, 0);
    if (!window)
        printf("CreateWindow error: %d\n", GetLastError());
    
    return window;
}

static void APIENTRY win32_gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void * user_param) {
    char *severity_text = NULL;
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
        severity_text = "high";
        break;
        
        case GL_DEBUG_SEVERITY_MEDIUM:
        severity_text = "medium";
        break;
        
        case GL_DEBUG_SEVERITY_LOW:
        severity_text = "low";
        break;
        
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        severity_text = "info";
        return;
        
        default:
        severity_text = "unkown severity";
    }
    
    char *type_text = NULL;
    switch (type)
    {
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        type_text = "depricated behavior";
        break;
        case GL_DEBUG_TYPE_ERROR:
        type_text = "error";
        break;
        case GL_DEBUG_TYPE_MARKER:
        type_text = "marker";
        break;
        case GL_DEBUG_TYPE_OTHER:
        type_text = "other";
        break;
        case GL_DEBUG_TYPE_PERFORMANCE:
        type_text = "performance";
        break;
        case GL_DEBUG_TYPE_POP_GROUP:	
        type_text = "pop group";
        break;
        case GL_DEBUG_TYPE_PORTABILITY:
        type_text = "portability";
        break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
        type_text = "push group";
        break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        type_text = "undefined behavior";
        break;
        default:
        type_text = "unkown type";
    }
    
    printf("[gl %s prio %s | %u ]: %s\n", severity_text, type_text, id, message);
    
    // filter harmless errors
    switch (id) {
        // shader compilation failed
        case 2000:
        return;
    }
    
    assert(type != GL_DEBUG_TYPE_ERROR);
}	

PLATFORM_ALLOCATE_DEC(win32_allocate) {
    return VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
}

_ALLOCATE_WRAPPER_DEC(win32)
{
    return win32_allocate(size, alignment);
}

PLATFORM_FREE_DEC(win32_free) {
    VirtualFree(data, 0, MEM_RELEASE); // ToDo: handle errors, check return value?
}

_FREE_WRAPPER_DEC(win32)
{
    return win32_free(data);
}

void * win32_reallocate(void *data, usize size)
{
    void *new_data = win32_allocate(size, -1);
    memcpy(new_data, data, size);	
    win32_free(data);
    
    return new_data;
}

void init_win32_allocator() {
    assert_no_clash(global_allocate_functions[Memory_Allocator_Plattform_Kind], _ALLOCATE_WRAPPER_FUNCTION_NAME(win32));
    assert_no_clash(global_free_functions[Memory_Allocator_Plattform_Kind], _FREE_WRAPPER_FUNCTION_NAME(win32));
    
    global_allocate_functions[Memory_Allocator_Plattform_Kind]   = _ALLOCATE_WRAPPER_FUNCTION_NAME(win32);
    global_reallocate_functions[Memory_Allocator_Plattform_Kind] = null;
    global_free_functions[Memory_Allocator_Plattform_Kind]       = _FREE_WRAPPER_FUNCTION_NAME(win32);
}

PLATFORM_SYNC_ALLOCATORS_DEC(win32_sync_allocators) {
    copy(allocate_functions,   global_allocate_functions,   sizeof(global_allocate_functions));
    copy(reallocate_functions, global_reallocate_functions, sizeof(global_reallocate_functions));
    copy(free_functions,       global_free_functions,       sizeof(global_free_functions));
}

Memory_Allocator make_win32_allocator() {
    return { Memory_Allocator_Plattform_Kind };
}

u64 win32_get_file_size(c_const_string file_path) {
    WIN32_FILE_ATTRIBUTE_DATA attribute_data;
    if (!GetFileAttributesEx(file_path, GetFileExInfoStandard, &attribute_data))
        return -(u64)1;
    
    return (cast_v(u64, attribute_data.nFileSizeHigh) << 32) | cast_v(u64, attribute_data.nFileSizeLow);
}

struct Win32_Platform_File {
    Platform_File platform_file;
    HANDLE handle;
};

PLATFORM_OPEN_FILE_DEC(win32_open_file) {
    char c_file_path[MAX_PATH];
    write_c_string(ARRAY_WITH_COUNT(c_file_path), file_path);
    
    HANDLE handle = CreateFile(c_file_path, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, null, null);
    
    if (handle == INVALID_HANDLE_VALUE)
        return null;
    
    u64 file_size = win32_get_file_size(c_file_path);
    if (file_size == usize_max)
        return null;
    
    auto result = ALLOCATE(allocator, Win32_Platform_File);
    result->platform_file.byte_count = file_size;
    result->platform_file.byte_offset = 0;
    result->handle = handle;
    
    return &result->platform_file;
}

PLATFORM_READ_FILE_DEC(win32_read_file) {
    auto win32_file = cast_p(Win32_Platform_File, file);
    
    assert(win32_file->handle != INVALID_HANDLE_VALUE);
    
    if (file->byte_count == file->byte_offset){
        printf("Error (platform_read_file): allready at end of file\n");
        *end_of_file = true;
        return false;
    }
    
    while ((file->byte_count - file->byte_offset) && remaining_byte_count_of(*buffer)) {
        DWORD read_count;
        
        if (file->byte_count - file->byte_offset > u32_max)
            read_count = u32_max;
        else
            read_count = cast_v(u32, file->byte_count - file->byte_offset);
        
        read_count = MIN(read_count, remaining_byte_count_of(*buffer));
        
        DWORD bytes_read;
        
        OVERLAPPED overlapped = {};
        overlapped.Offset     = cast_v(u32, file->byte_offset);
        overlapped.OffsetHigh = cast_v(u32, file->byte_offset >> 32);
        if (!ReadFile(win32_file->handle, buffer->data + buffer->count, read_count, &bytes_read, &overlapped) || (read_count != bytes_read)) {
            printf("Error (platform_read_file): could not read file, error code: %d\n",  GetLastError());
            UNREACHABLE_CODE;
            return false;
        }
        
        file->byte_offset += read_count;
        buffer->count     += read_count;
    }
    
    *end_of_file = (file->byte_offset == file->byte_count);
    
    return true;
}

PLATFORM_CLOSE_FILE_DEC(win32_close_file) {
    auto win32_file = cast_p(Win32_Platform_File, file);
    assert(win32_file->handle != INVALID_HANDLE_VALUE);
    CloseHandle(win32_file->handle);
}

PLATFORM_READ_ENTIRE_FILE_DEC(win32_read_entire_file) {
    char c_file_path[MAX_PATH];
    write_c_string(ARRAY_WITH_COUNT(c_file_path), file_path);
    
    HANDLE file = CreateFile(c_file_path, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, null, null);
    
    if (file == INVALID_HANDLE_VALUE)
        return {};
    
    u64 file_size = win32_get_file_size(c_file_path);
    if (file_size == usize_max)
        return {};
    
    // test if file size is too large for current architecuture memory space
    usize test_size = cast_v(usize, file_size);
    if (test_size != file_size)
        return {};
    
    u8_array chunk = ALLOCATE_ARRAY_INFO(allocator, u8, file_size);
    
    DWORD read_count;
    if (file_size > u32_max)
        read_count = u32_max;
    else
        read_count = cast_v(u32, file_size);
    
    OVERLAPPED overlapped = {};
    usize offset = 0;
    
    while (file_size) {
        DWORD bytes_read;
        
        if (!ReadFile(file, chunk.data + offset, read_count, &bytes_read, &overlapped) || (read_count != bytes_read)) {
            printf("Error (platform_read_file): could not read file %s, error code: %d\n", c_file_path,  GetLastError());
            free(allocator, chunk.data);
            chunk = {};
        }
        
        file_size -= read_count;
        offset    += read_count;
        
        if (file_size > u32_max)
            read_count = u32_max;
        else
            read_count = cast_v(u32, file_size);
    }
    
    CloseHandle(file);
    
    return chunk;
}

PLATFORM_WRITE_ENTIRE_FILE_DEC(win32_write_entire_file) {
    //assert(size <= 0xFFFFFFFF);
    char file_path_buffer[MAX_PATH];
    
    for (u32 i = 0; i < file_path.count; ++i)
        file_path_buffer[i] = file_path[i];
    file_path_buffer[file_path.count] = '\0';
    
    HANDLE file = CreateFile(file_path_buffer, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
    
    bool result = false;
    
    if (file != INVALID_HANDLE_VALUE) {
        DWORD written_bytes;
        
        if (!WriteFile(file, chunk.data, (DWORD)chunk.count, &written_bytes, NULL) || (chunk.count != written_bytes))
            printf("Error (win32_write_file): could not write file %.*s\n", FORMAT_S(&file_path));
        else
            result = true;
        
        CloseHandle(file);
    }
    else
        printf("Error (win32_write_file): could not create file %.*s\n", FORMAT_S(&file_path));
    
    return result;
}

inline static float win32_gamepad_normalized_axis(SHORT value, float neg_range, float pos_range)
{
    if (value >= 0)
        return value / pos_range;
    else
        return value / neg_range;
}

void win32_set_gl_pixel_format(Win32_Window *window)
{
    PIXELFORMATDESCRIPTOR pixel_format_descriptor =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
        32,                        //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                        //Number of bits for the depthbuffer
        8,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    
    int  pixel_format;
    pixel_format = ChoosePixelFormat(window->device_context, &pixel_format_descriptor);
    if (!pixel_format)
        printf("ChoosePixelFormat error: %u\n", GetLastError());
    
    if (!SetPixelFormat(window->device_context, pixel_format, &pixel_format_descriptor))
        printf("could not set pixel format, error: %u\n", GetLastError());
}

void win32_set_gl_core_pixel_format(Win32_Platform_API *api, Win32_Window *window, int major_version = 3, int minor_version = 3)
{
    const int pixel_format_attributes[] =
    {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
#if 0 // multi sample anti aliasing
        WGL_SAMPLE_BUFFERS_ARB, 1, //Number of buffers (must be 1 at time of writing)
        WGL_SAMPLES_ARB, 8,        //Number of samples
#endif 
        0,        //End
    };
    
    int pixelFormat;
    UINT numFormats;
    
    wglChoosePixelFormatARB(window->device_context, pixel_format_attributes, NULL, 1, &pixelFormat, &numFormats);
    SetPixelFormat(window->device_context, pixelFormat, NULL);
}

INTERNAL void win32_set_window_area(HWND window_handle, Pixel_Rectangle *area) {
    RECT client_rect;
    GetClientRect(window_handle, &client_rect);
    
    RECT window_rect;
    GetWindowRect(window_handle, &window_rect);
    
    s16 border_width  = (window_rect.right - window_rect.left) - (client_rect.right - client_rect.left);
    s16 border_height = (window_rect.bottom - window_rect.top) - (client_rect.bottom - client_rect.top);
    
    if (area->x == -1) area->x = CW_USEDEFAULT;
    if (area->y == -1) area->y = CW_USEDEFAULT;
    
    SetWindowPos(window_handle, HWND_TOP, area->x, area->y, area->width + border_width, area->height + border_height, SWP_SHOWWINDOW);
}

INTERNAL void win32_set_window_title(HWND window_handle, string title) {
    char name_buffer[MAX_PATH];
    write_c_string(name_buffer, ARRAY_COUNT(name_buffer), title);
    SetWindowText(window_handle, name_buffer);
}

#define X_INPUT_GET_STATE_DEC(name) DWORD name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE_DEC((*XInput_GetState_Function));

X_INPUT_GET_STATE_DEC(XInputGetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}

static void win32_update_button(Input_Button *button, WORD xinput_buttons, WORD xinput_button_flag, float delta_seconds) {
    button_poll_advance(button, delta_seconds);
    
    if (xinput_buttons & xinput_button_flag)
        button_poll_update(button, true);
    else
        button_poll_update(button, false);
}


bool win32_load_application(Memory_Allocator *temporary_allocator, Application_Info *application_info)
{
    String_Buffer name_buffer = new_write_buffer(temporary_allocator, S("%compile_dll_lock.tmp\0"), f(application_info->dll_path));
    
    defer { free(&name_buffer); };
    
    WIN32_FILE_ATTRIBUTE_DATA ignored;
    if (GetFileAttributesEx(TO_C_STR(&name_buffer.buffer), GetFileExInfoStandard, &ignored))
        return false;
    
    free(&name_buffer);
    string name = write(&name_buffer, S("%%\0"), f(application_info->dll_path), f(application_info->dll_name));
    
    
    WIN32_FIND_DATA find_data;
    HANDLE find_handle = FindFirstFile(TO_C_STR(&name), &find_data);
    
    bool dll_reloaded = false;
    if (find_handle != INVALID_HANDLE_VALUE) {
        if (CompareFileTime(&find_data.ftLastWriteTime, &application_info->last_dll_write_time) == 1)
        {
            //printf("(re-)loading dll\n");
            
            if (application_info->dll) {
                //return; // disbales relaod
                
                BOOL result = FreeLibrary(application_info->dll);
                assert(result);
            }
            
            string tmp_file_name = write(&name_buffer, S("%tmp_app.dll\0"), f(application_info->dll_path));
            
            BOOL result = CopyFile(TO_C_STR(&name), TO_C_STR(&tmp_file_name), FALSE);
            if (!result) {
                DWORD error;
                switch (error = GetLastError()) {
                    case 32:
                    printf("could not copy %.*s, still in use, retrying next frame.\n", FORMAT_S(&name));
                    break;
                    
                    default:
                    printf("could not copy %.*s (error: %u), retrying next frame\n", FORMAT_S(&name), error);
                }
            }
            else {
                application_info->last_dll_write_time = find_data.ftLastWriteTime;
            }
            
            application_info->dll = LoadLibraryA(TO_C_STR(&tmp_file_name));
            assert(application_info->dll);
            
            string init_function_name = write(&name_buffer, S("%\0"), f(application_info->init_name));
            application_info->init = (App_Init_Function)GetProcAddress(application_info->dll, TO_C_STR(&init_function_name));
            assert(application_info->init);
            
            string main_loop_function_name =write(&name_buffer, S("%\0"), f(application_info->main_loop_name));
            application_info->main_loop = (App_Main_Loop_Function)GetProcAddress(application_info->dll, TO_C_STR(&main_loop_function_name));
            assert(application_info->main_loop);
            
            dll_reloaded = true;
        }
        
        FindClose(find_handle);
    }
    
    return dll_reloaded;
}

LRESULT CALLBACK win32_window_callback(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    Win32_Window *window = cast_p(Win32_Window, GetWindowLongPtr(window_handle, GWLP_USERDATA));
    
    switch (message)
    {
        case WM_MOVE:
        {
            if (!window)
                return 0;
            
            window->new_area.x = LOWORD(l_param);
            window->new_area.y = HIWORD(l_param);
        }
        break;
        
        case WM_SIZE:
        {
            if (!window)
                return 0;
            
            RECT rect;
            GetClientRect(window->handle, &rect);
            
            window->new_area.x = rect.right;
            window->new_area.y = rect.top;
            window->new_area.width  = rect.right - rect.left;
            window->new_area.height = rect.bottom - rect.top;
        }
        break;
        
        case WM_SIZING: {
            if (!window)
                return 0;
            
            if (window->is_resizeable) {
                
                s16 border_width, border_height;
                {
                    RECT rect;
                    GetClientRect(window_handle, &rect);
                    
                    s16 width = (rect.right - rect.left);
                    s16 height = (rect.bottom - rect.top);
                    
                    GetWindowRect(window_handle, &rect);
                    border_width  = (rect.right - rect.left) - width;
                    border_height = (rect.bottom - rect.top) - height;
                }
                
                assert(!window->is_fullscreen);
                
                RECT *window_rect = cast_p(RECT, l_param);
                
                s16 width  = window_rect->right - window_rect->left - border_width;
                s16 height = window_rect->bottom - window_rect->top - border_height;
                
                if (window->forced_width_over_height_or_zero != 0.0f) {
                    s16 dx = 0;
                    s16 dy = 0;
                    
                    WPARAM border = w_param;
                    
                    switch (border) {
                        case WMSZ_LEFT:
                        case WMSZ_RIGHT: {
                            dy = cast_v(s16, width / window->forced_width_over_height_or_zero + 0.5f) - height;
                            height += dy;
                            
                            window_rect->bottom += dy;
                        } break;
                        
                        default: {
                            dx = cast_v(s16, height * window->forced_width_over_height_or_zero + 0.5f) - width;
                            width += dx;
                            
                            if (border >= WMSZ_BOTTOM)
                                border -= WMSZ_BOTTOM;
                            else if (border >= WMSZ_TOP)
                                border -= WMSZ_TOP;
                            
                            if (border == WMSZ_LEFT) {
                                window_rect->left -= dx;
                            }
                            else {
                                window_rect->right += dx;
                            }
                        }
                    }
                } 
                
                window->new_area.width = width;
                window->new_area.height = height;
                
                return TRUE;
            }
        } break;
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(window_handle, &ps); 
            EndPaint(window_handle, &ps);
        }; break;
        
        case WM_ACTIVATE:
        {
            if (!window)
                return 0;
            
            // handle window focus change in boarderless fullscreen mode
            if (window->is_fullscreen) {
                switch (w_param) {
                    case WA_ACTIVE:
                    {
                        SetWindowPos(window_handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
                        break;
                    }
                    case WA_INACTIVE:
                    {
                        SetWindowPos(window_handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
                        break;
                    }
                }
            }
        }
        break;
        
        case WM_DESTROY: {
            DragAcceptFiles(window->handle, FALSE);
            window->was_destroyed = true;
        } break;
        
        default:
        return DefWindowProc(window_handle, message, w_param, l_param);
    }
    
    return 0;
}

#if 0
void trigger_event(HANDLE event) {
    if (!SetEvent(event)) {
        printf("SetEvent(%p) failed, (error %d)\n", event, GetLastError());
        UNREACHABLE_CODE;
    }
}

void wait_for_event(HANDLE event) {
    DWORD result;
    switch(result = WaitForSingleObject(event, INFINITE)) {
        case WAIT_OBJECT_0:
        break;
        
        case WAIT_FAILED:
        printf("WaitForSingleObject(%p) failed, (error %d)\n", event, GetLastError());
        assert(0);
        break;
        
        default:
        printf("WaitForSingleObject(%p) unreachable code, result %x, (error %d)\n", event, result, GetLastError());
        UNREACHABLE_CODE;
    }
}
#endif

PLATFORM_DISPLAY_WINDOW_DEC(win32_display_window) {
    auto api = cast_p(Win32_Platform_API, platform_api);
    
    Platform_Window result;
    
    if (api->current_window) {
        api->swap_buffer_count++;
        SwapBuffers(api->current_window->device_context);
        api->current_window = NULL;
    }
    
    Win32_Window *window = null;
    for (u32 i = 0; i < api->window_buffer.count; ++i) {
        if (api->window_buffer[i].id == window_id) {
            window = api->window_buffer + i;
            break;
        }
    }
    
    if (!window) {
        window = push(&api->window_buffer);
        *window = {};
        
        window->title = title;
        window->was_destroyed = false;
        window->id = window_id;
        window->is_active = false;
        
        // will be checked later and set accordingly
        window->is_fullscreen = false;
        window->is_resizeable = true;
        
        window->handle = win32_create_window(&api->window_class, window->title, window->old_area.width, window->old_area.height, api->windows_instance);
        window->device_context = GetDC(window->handle);
        
        DragAcceptFiles(window->handle, TRUE);
        
        if (wglCreateContextAttribsARB)
            win32_set_gl_core_pixel_format(api, window);
        else 
            win32_set_gl_pixel_format(window);
        
        wglSwapIntervalEXT(1); // enble v-sync
        
        ShowWindow(window->handle, SW_SHOWNORMAL);
        // set userdata after initial resize, otherwise requestet size would be overwriten by WM_SIZE message
        SetWindowLongPtr(window->handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    
    assert(!window->is_active, "tried to draw on the same window twice in on frame");
    
    if (window->was_destroyed) {
        result.was_destroyed = true;
        return result;
    }
    
    assert(window->handle);
    
    wglMakeCurrent(window->device_context, api->gl_context);
    
    if (is_resizeable != window->is_resizeable) {
        window->is_resizeable = is_resizeable;
        DWORD style = GetWindowLong(window->handle, GWL_STYLE);
        
        if (is_resizeable)
            SetWindowLong(window->handle, GWL_STYLE, style | WS_THICKFRAME);
        else
            SetWindowLong(window->handle, GWL_STYLE, style & ~WS_THICKFRAME);
        
        //SetWindowPos(window->handle, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
    
    window->forced_width_over_height_or_zero = forced_width_over_height_or_zero;
    
    if (!window->is_fullscreen) {
        if (window->old_area != *area) {
            win32_set_window_area(window->handle, area);
        } else { // otherwise return the new_area updated by window system
            *area = window->new_area;
        }
        
        window->old_area = *area;
        window->new_area = window->old_area;
    }
    else {
        *area = window->old_area;
    }
    
    if (is_fullscreen != window->is_fullscreen) {
        DWORD style = GetWindowLong(window->handle, GWL_STYLE);
        if (style & WS_OVERLAPPEDWINDOW) {
            assert(!window->is_fullscreen);
            
            MONITORINFO monitor_info = { sizeof(monitor_info) };
            if (GetWindowPlacement(window->handle, &window->placement_backup) &&
                GetMonitorInfo(MonitorFromWindow(window->handle, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
            {
                window->old_area.width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
                window->old_area.height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
                
                SetWindowLong(window->handle, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
                SetWindowPos(window->handle, HWND_TOP,
                             monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                             monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                             monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                
                *area = window->old_area;
                window->new_area = window->old_area;
                
                window->is_fullscreen = true;
            }
        }
        else {
            assert(window->is_fullscreen);
            
            style |= WS_OVERLAPPEDWINDOW;
            
            if (!window->is_resizeable)
                style &= ~WS_THICKFRAME;
            
            SetWindowLong(window->handle, GWL_STYLE, style);
            SetWindowPlacement(window->handle, &window->placement_backup);
            SetWindowPos(window->handle, NULL, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            
            *area = window->old_area;
            
            window->is_fullscreen = false;
        }
    }
    
    if (title != window->title)
        win32_set_window_title(window->handle, title);
    
    api->current_window = window;
    
    // prevents window from being destroyed
    window->is_active = true;
    
    POINT point;
    GetCursorPos(&point);
    
    result.was_destroyed = false;
    ScreenToClient(window->handle, &point);
    result.mouse_position = { cast_v(f32, point.x), cast_v(f32, window->new_area.height - point.y - 1) };
    
    result.mouse_is_inside = contains(window->new_area, result.mouse_position.x, result.mouse_position.y);
    
    return result;
}

PLATFORM_SKIP_WINDOW_UPDATE_DEC(win32_skip_window_update)
{
    auto api = cast_p(Win32_Platform_API, platform_api);
    assert(api->current_window);
    
    // will skip SwapBuffers for this window
    api->current_window = null;
}

#define DIRECT_SOUND_CREATE_DEC(name) HRESULT name(LPGUID lpGuid, LPDIRECTSOUND * ppDS, LPUNKNOWN  pUnkOuter)
typedef DIRECT_SOUND_CREATE_DEC((*DirectSoundCreate_Function));

INTERNAL LPDIRECTSOUNDBUFFER win32_init_dsound(HWND window_handle, u32 samples_per_second, u32 buffer_byte_count) 
{
    HMODULE dsound_dll = LoadLibraryA("dsound.dll");
    if (!dsound_dll) {
        return null;
        
        // TODO: create sound stub
    }
    
    DirectSoundCreate_Function DirectSoundCreate = reinterpret_cast<DirectSoundCreate_Function> (GetProcAddress(dsound_dll, "DirectSoundCreate"));
    
    LPDIRECTSOUND direct_sound;
    if (!DirectSoundCreate || !SUCCEEDED(DirectSoundCreate(null, &direct_sound, null)))
        return null;
    
    if (!SUCCEEDED(direct_sound->SetCooperativeLevel(window_handle, DSSCL_PRIORITY)))
        return null;
    
    DSBUFFERDESC primary_buffer_discription = {};
    primary_buffer_discription.dwSize  = sizeof(primary_buffer_discription);
    primary_buffer_discription.dwFlags = DSBCAPS_PRIMARYBUFFER;
    
    
    LPDIRECTSOUNDBUFFER prmary_buffer;
    if (!SUCCEEDED(direct_sound->CreateSoundBuffer(&primary_buffer_discription, &prmary_buffer, 0)))
        return null;
    
    WAVEFORMATEX wave_format    = {};
    wave_format.wFormatTag      = WAVE_FORMAT_PCM;
    wave_format.nChannels       = 2;
    wave_format.nSamplesPerSec  = samples_per_second;
    wave_format.wBitsPerSample  = 16;
    wave_format.nBlockAlign     = (wave_format.nChannels * wave_format.wBitsPerSample) / 8; // in bytes
    wave_format.nAvgBytesPerSec = wave_format.nBlockAlign * wave_format.nSamplesPerSec;
    wave_format.cbSize          = 0;
    
    if (!SUCCEEDED(prmary_buffer->SetFormat(&wave_format)))
        return null;
    
    DSBUFFERDESC secondary_buffer_discription  = {};
    secondary_buffer_discription.dwSize        = sizeof(secondary_buffer_discription);
    secondary_buffer_discription.dwFlags       = 0;
    secondary_buffer_discription.lpwfxFormat   = &wave_format;
    secondary_buffer_discription.dwBufferBytes = buffer_byte_count;
    
    LPDIRECTSOUNDBUFFER secondary_buffer;
    if (!SUCCEEDED(direct_sound->CreateSoundBuffer(&secondary_buffer_discription, &secondary_buffer, 0)))
        return null;
    
    return secondary_buffer;
}

#define THREAD_PROC_DEC(name) DWORD WINAPI name(LPVOID parameter)

bool win32_do_work(Platform_Worker_Queue *queue, u32 thread_index) {
    if (queue->next_index < queue->count)
    {
        u32 index = InterlockedIncrement(&queue->next_index) - 1;
        _ReadBarrier();
        auto work = queue->works + index;
        
        work->do_work(work->data, thread_index);
        InterlockedIncrement(&queue->done_count);
        return true;
    }
    
    return false;
}

THREAD_PROC_DEC(win32_worker_thread)
{
    auto info = cast_p(Win32_Worker_Thread_Info, parameter);
    auto queue = info->queue;
    
    //printf("worker thread %i started\n", info->index);
    
    LOOP {
        if (!win32_do_work(queue, info->index)) {
            //printf("worker thread %i sleeping...\n", info->index);
            WaitForSingleObjectEx(queue->semaphore_handle, INFINITE, FALSE);
            //printf("worker thread %i woke up\n", info->index);
        }
    }
}

void win32_create_worker_threads(Memory_Allocator *allocator, Platform_Worker_Queue *queue)
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    queue->thread_count = system_info.dwNumberOfProcessors;
    printf("logical processor count: %u\n", queue->thread_count);
    
    queue->thread_count--;
    queue->next_index = 0;
    queue->capacity = 1024;
    queue->count = 0;
    queue->works = ALLOCATE_ARRAY(allocator, Platform_Work, queue->capacity);
    
    queue->thread_infos = ALLOCATE_ARRAY(allocator, Win32_Worker_Thread_Info, queue->thread_count);
    
    queue->semaphore_handle = CreateSemaphoreExA(null, 0, queue->thread_count, "Worker Queue Semaphore", 0, SEMAPHORE_ALL_ACCESS);
    
    for (u32 thread_index = 0; thread_index < queue->thread_count; ++thread_index)
    {
        queue->thread_infos[thread_index].queue = queue;
        queue->thread_infos[thread_index].index = thread_index;
        queue->thread_infos[thread_index].handle = CreateThread(null, 0, win32_worker_thread, queue->thread_infos + thread_index, 0, null);
    }
}

PLATFORM_WORK_ENQUEUE_DEC(win32_work_enqueue)
{
    assert(queue->count < queue->capacity);
    queue->works[queue->count] = work;
    
    _WriteBarrier();
    
    queue->count++;
    ReleaseSemaphore(queue->semaphore_handle, 1, 0);
}

PLATFORM_GET_DONE_WORK_DEC(win32_get_done_work)
{
    u32 my_total_work_count = queue->count;
    u32 my_done_work_count  = queue->done_count;
    
    bool all_work_done = (my_done_work_count == my_total_work_count);
    
    if (all_work_done)
        *works = queue->works;
    else
        *works = null;
    
    if (total_work_count)
        *total_work_count = my_total_work_count;
    
    if (done_work_count)
        *done_work_count = my_done_work_count;
    
    return all_work_done;
}

PLATFORM_WORK_RESET_DEC(win32_work_reset)
{
    assert(queue->done_count == queue->count);
    queue->next_index = 0;
    queue->done_count = 0;
    queue->count = 0;
}

struct Platform_Mutex {
    HANDLE handle;
};

PLATFORM_MUTEX_CREATE_DEC(win32_mutex_create) {
    auto mutex = ALLOCATE(allocator, Platform_Mutex);
    
    mutex->handle = CreateMutex(null, FALSE, null);
    
    if (!mutex->handle) {
        printf("could not create mutex, error code: %u\n", GetLastError());
        free(allocator, mutex);
        UNREACHABLE_CODE;
        return null;
    }
    
    return mutex;
}

PLATFORM_MUTEX_DESTROY_DEC(win32_mutex_destroy) {
    if (!CloseHandle(mutex->handle)) {
        printf("could not destroy mutex, error code: %u\n", GetLastError());
        assert(0);
        return false;
    }
    
    return true;
}

PLATFORM_MUTEX_LOCK_DEC(win32_mutex_lock) {
    switch(WaitForSingleObject(mutex->handle, INFINITE)) {
        case WAIT_ABANDONED:
        printf("mutex was not released by locking thread, access granted\n");
        case WAIT_OBJECT_0: // everything ok
        return true;
        
        case WAIT_FAILED:
        printf("could not lock mutex, error code: %u\n", GetLastError());
        
        default:
        UNREACHABLE_CODE;
        
        return false;
    }
}

PLATFORM_MUTEX_UNLOCK_DEC(win32_mutex_unlock) {
    if (!ReleaseMutex(mutex->handle)) {
        printf("could not unlock mutex, error code: %u\n", GetLastError());
        UNREACHABLE_CODE;
        return false;
    }
    
    return true;
}

PLATFORM_RUN_COMMAND(win32_run_command) {
    auto win32_api = cast_p(Win32_Platform_API, platform_api);
    
    if (win32_api->pipe_read == 0) {
        SECURITY_ATTRIBUTES sa = {};
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        
        if (!CreatePipe(&win32_api->pipe_read, &win32_api->pipe_write, &sa, 0)) {
            printf("could not create pipe, error: %u\n", GetLastError());
            *out_ok = false;
            return {};
        }
        
        SetHandleInformation(win32_api->pipe_read, HANDLE_FLAG_INHERIT, 0);
    }
    
    STARTUPINFO startup_info = {};
    
    startup_info.cb = sizeof(startup_info); 
    startup_info.dwFlags = STARTF_USESTDHANDLES;
    startup_info.hStdError  = win32_api->pipe_write;
    startup_info.hStdOutput = win32_api->pipe_write;
    startup_info.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    
    PROCESS_INFORMATION process_information = {};
    
    {
        string c_command = new_write(allocator, S("%\0"), f(command_line));
        defer { free_array(allocator, &c_command); };
        
        *out_ok = CreateProcess(
            null,
            cast_p(char, c_command.data),
            null,          // process security attributes 
            null,          // primary thread security attributes 
            TRUE,          // handles are inherited 
            0,             // creation flags 
            null,          // use parent's environment 
            null,          // use parent's current directory 
            &startup_info,  // STARTUPINFO pointer 
            &process_information);  // receives PROCESS_INFORMATION 
        
        if (!*out_ok) {
            printf("could not run %.*s, error: %u\n", FORMAT_S(&command_line), GetLastError());
            return {};
        }
    }
    
    printf("starting %.*s ..\n", FORMAT_S(&command_line));
    
    WaitForSingleObject(process_information.hProcess, INFINITE);
    
    DWORD total_bytes_available;
    if (!PeekNamedPipe(win32_api->pipe_read, null, 0, null, &total_bytes_available, null)) {
        printf("could not peek pipe, error code: %u\n", GetLastError());
        total_bytes_available = 0;
    }
    
    string output = {};
    
    if (total_bytes_available) {
        LOOP
        { 
            u8 _buffer[KILO(4)];
            u8_buffer buffer = ARRAY_INFO(_buffer);
            
            DWORD bytes_read;
            if (!ReadFile(win32_api->pipe_read, buffer.data, buffer.capacity, &bytes_read, null))
            {
                printf("failed to read process output, error: %u\n", GetLastError());
                break;
            }
            
            if (bytes_read) {
                auto dest = grow(allocator, &output, bytes_read);
                copy(dest, _buffer, bytes_read);
            }
            
            if (bytes_read < buffer.capacity)
                break;
        }
    }
    
    GetExitCodeProcess(process_information.hProcess, cast_p(DWORD, out_exit_code));
    
    CloseHandle(process_information.hProcess);
    CloseHandle(process_information.hThread);
    
    //printf("%.*s done, exit code: %i\n", FORMAT_S(&command_line), *out_exit_code);
    
    return output;
}

PLATFORM_GET_WORKING_DIRECTORY(win32_get_working_directory) {
    auto win32_api = cast_p(Win32_Platform_API, platform_api);
    
    if (!win32_api->working_directory.count) {
        string result;
        result.count = GetCurrentDirectory(0, NULL);
        if (!result.count) {
            printf("could not get working directory, error: %u\n", GetLastError());
            return {};
        }
        
        result.data = ALLOCATE_ARRAY(&win32_api->persistent_memory.allocator, u8, result.count);
        if (!GetCurrentDirectory(cast_v(DWORD, result.count), (LPTSTR)result.data)) {
            printf("could not get working directory, error: %u\n", GetLastError());
            free(&win32_api->persistent_memory.allocator, result.data);
            return {};
        }
        
        result.count--; // exclude 0-terminating character
        
        win32_api->working_directory = result;
    }
    
    return win32_api->working_directory;
}

void init_win32_api(Win32_Platform_API *win32_api) {
    init_win32_allocator();
    
    *win32_api = {};
    
    win32_api->allocator = make_win32_allocator();
    win32_api->transient_memory = make_memory_growing_stack(&win32_api->allocator);
    win32_api->persistent_memory = make_memory_growing_stack(&win32_api->allocator);
    
    win32_api->windows_instance = GetModuleHandle(NULL);
    
    win32_api->platform_api = {
        &win32_api->allocator,
        win32_sync_allocators,
        win32_open_file,
        win32_close_file,
        win32_read_file,
        win32_read_entire_file,
        win32_write_entire_file,
        win32_display_window,
        win32_skip_window_update,
        win32_work_enqueue,
        win32_get_done_work,
        win32_work_reset,
        win32_mutex_create,
        win32_mutex_destroy,
        win32_mutex_lock,
        win32_mutex_unlock,
        win32_run_command,
        win32_get_working_directory,
    };
}


