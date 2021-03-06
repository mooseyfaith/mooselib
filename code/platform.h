#if !defined PLATFORM_H
#define PLATFORM_H

#include "basic.h"
#include "input.h"
#include "mo_string.h"

#if defined(WIN32)

#if defined(WIN32_EXPORT)
#  define PLATFORM_SHARE extern "C" __declspec(dllexport)
#else
#  define PLATFORM_SHARE extern "C" __declspec(dllimport)
#endif

#else

#define PLATFORM_SHARE

#endif 

struct Platform_API;

#define PLATFORM_SYNC_ALLOCATORS_DEC(name) void name(Memory_Allocate_Function *allocate_functions, Memory_Reallocate_Function *reallocate_functions, Memory_Free_Function *free_functions)
typedef PLATFORM_SYNC_ALLOCATORS_DEC((*Platform_Sync_Allocators_Function));

#define PLATFORM_SYNC_ALLOCATORS() platform_api->sync_allocators(global_allocate_functions, global_reallocate_functions, global_free_functions)

#define PLATFORM_ALLOCATE_DEC(name) any name(usize size, usize alignment)
typedef PLATFORM_ALLOCATE_DEC((*Platform_Allocate_Function));

#define PLATFORM_FREE_DEC(name) void name(any data)
typedef PLATFORM_FREE_DEC((*Platform_Free_Function));

// file

struct Platform_File {
    u64 byte_count;
    u64 byte_offset;
};

#define PLATFORM_OPEN_FILE_DEC(name) Platform_File * name(Memory_Allocator *allocator, string file_path)
typedef PLATFORM_OPEN_FILE_DEC((*Platform_Open_File_Function));

#define PLATFORM_CLOSE_FILE_DEC(name) void name(Platform_File *file)
typedef PLATFORM_CLOSE_FILE_DEC((*Platform_Close_File_Function));

#define PLATFORM_READ_FILE_DEC(name) bool name(u8_buffer *buffer, bool *end_of_file, Platform_File *file)
typedef PLATFORM_READ_FILE_DEC((*Platform_Read_File_Function));

#define PLATFORM_READ_ENTIRE_FILE_DEC(name) u8_array name(string file_path, Memory_Allocator *allocator)
typedef PLATFORM_READ_ENTIRE_FILE_DEC((*Platform_Read_Entire_File_Function));

#define PLATFORM_WRITE_ENTIRE_FILE_DEC(name) bool name(string file_path, u8_array chunk)
typedef PLATFORM_WRITE_ENTIRE_FILE_DEC((*Platform_Write_Entire_File_Function));

// worker queue

//struct Platform_Worker_Queue;

#define PLATFORM_DO_WORK_DEC(name) void name(any data, u32 thread_index)
typedef PLATFORM_DO_WORK_DEC((*Platform_Do_Work_Function));

struct Platform_Work {
    Platform_Do_Work_Function do_work;
    any data;
};

#define PLATFORM_CREATE_WORKER_THREADS_DEC(name) void name(Platform_API *platform_api, Memory_Allocator *allocator, u32 thread_count, u32 work_entry_capacity)
typedef PLATFORM_CREATE_WORKER_THREADS_DEC((*Platform_Create_Worker_Threads_Function));

#define PLATFORM_ENQUEUE_WORK_DEC(name) void name(Platform_API *platform_api, Platform_Work work)
typedef PLATFORM_ENQUEUE_WORK_DEC((*Platform_Enqueue_Work_Function));

#define PLATFORM_DO_NEXT_WORK(name) bool name(Platform_API *platform_api, u32 thread_index)
typedef PLATFORM_DO_NEXT_WORK((*Platform_Do_Next_Work_Funtion));

#define PLATFORM_GET_FINISHED_WORK_DEC(name) bool name(Platform_API *platform_api, OUTPUT Platform_Work **work_entries, OUTPUT u32 *total_work_count, OUTPUT OPTIONAL u32 *done_work_count)
typedef PLATFORM_GET_FINISHED_WORK_DEC((*Platform_Get_Finished_Work_Function));

#define PLATFORM_WORK_RESET_DEC(name) void name(Platform_API *platform_api)
typedef PLATFORM_WORK_RESET_DEC((*Platform_Work_Reset_Function));

struct Platform_Window {
    vec2f mouse_position;
    Pixel_Rectangle area;
    bool mouse_is_inside;
    bool was_destroyed;
};

#define PLATFORM_DISPLAY_WINDOW_DEC(name) Platform_Window name(Platform_API *platform_api, u32 window_id, string title, Pixel_Rectangle *area, bool is_resizeable, bool is_fullscreen, f32 forced_width_over_height_or_zero)
typedef PLATFORM_DISPLAY_WINDOW_DEC((*Platform_Display_Window_Function));

#define PLATFORM_SKIP_WINDOW_UPDATE_DEC(name) void name(Platform_API *platform_api)
typedef PLATFORM_SKIP_WINDOW_UPDATE_DEC((*Platform_Skip_Window_Update_Function));


// mutex functions


struct Platform_Thread;
struct Platform_Mutex;

#define PLATFORM_THREAD_ENTRY_DEC(name) s32 name(any user_data)
typedef PLATFORM_THREAD_ENTRY_DEC((*Platform_Thread_Entry_Function));

#define PLATFORM_CREATE_THREAD(name) Platform_Thread * name(Memory_Allocator *allocator, Platform_Thread_Entry_Function thread_fuction, any user_data)
typedef PLATFORM_CREATE_THREAD((*Platform_Create_Thread_Function));

#define PLATFORM_MUTEX_CREATE_DEC(name) Platform_Mutex * name(Memory_Allocator *allocator)
typedef PLATFORM_MUTEX_CREATE_DEC((*Platform_Mutex_Create_Function));

#define PLATFORM_MUTEX_LOCK_DEC(name) bool name(Platform_Mutex *mutex)
typedef PLATFORM_MUTEX_LOCK_DEC((*Platform_Mutex_Lock_Function));

#define PLATFORM_MUTEX_UNLOCK_DEC(name) bool name(Platform_Mutex *mutex)
typedef PLATFORM_MUTEX_UNLOCK_DEC((*Platform_Mutex_Unlock_Function));

#define PLATFORM_MUTEX_DESTROY_DEC(name) bool name(Platform_Mutex *mutex)
typedef PLATFORM_MUTEX_DESTROY_DEC((*Platform_Mutex_Destroy_Function));

#define PLATFORM_RUN_COMMAND_DEC(name) string name(u32 *out_exit_code, bool *out_ok, Platform_API *platform_api, string command_line, Memory_Allocator *allocator)
typedef PLATFORM_RUN_COMMAND_DEC((*Platform_Run_Command_Function));

#define PLATFORM_GET_LOGICAL_PROCESSOR_COUNT_DEC(name) u32 name(Platform_API *platform_api)
typedef PLATFORM_GET_LOGICAL_PROCESSOR_COUNT_DEC((*Platform_Get_Logical_Processor_Count_Function));

#define PLATFORM_GET_WORKING_DIRECTORY_DEC(name) string name(Platform_API *platform_api)
typedef PLATFORM_GET_WORKING_DIRECTORY_DEC((*Platform_Get_Working_Directory_Function));

#define PLATFORM_GET_CLIPBOARD_TEXT_DEC(name) string name(Memory_Allocator *allocator, Platform_API *platform_api)
typedef PLATFORM_GET_CLIPBOARD_TEXT_DEC((*Platform_Get_Clipboard_Text_Function));

#define PLATFORM_COPY_TO_CLIPBOAD_DEC(name) void name(Platform_API *platform_api, string text)
typedef PLATFORM_COPY_TO_CLIPBOAD_DEC((*Platform_Copy_To_Clipboard_Function));

enum Platform_Message_Kind {
    Platform_Message_Kind_Reload,
    Platform_Message_Kind_Character,
    Platform_Message_Kind_File_Drop,
    Count,
};

struct Platform_Message {
    u32 kind;
};

#define Template_List_Name      Platform_Messages
#define Template_List_Data_Type Platform_Message *
#define Template_List_With_Double_Links
#define Template_List_With_Tail
#include "template_list.h"

// WARNING make shure this don't overlap
// also they dont overlap with visible characters
enum Platform_Character {
    Platform_Character_Backspace = 8,
    Platform_Character_Tab       = 9,
    Platform_Character_Return    = 13,
    
    Platform_Character_Left,
    Platform_Character_Right,
    Platform_Character_Down,
    Platform_Character_Up,
    
    Platform_Character_Page_Up,
    Platform_Character_Page_Down,
    
    Platform_Character_End,
    Platform_Character_Home,
    Platform_Character_Insert,
    
    Platform_Character_Delete,
    
    Platform_Character_Escape    = 27,
};

struct Platform_Message_Character {
    u32 kind;
    u32 code;
};

struct Platform_Message_File_Drop {
    u32 kind;
    string full_path;
    string path;
    string name;
    string extension;
    bool is_directory;
};

struct Platform_API {
    Memory_Allocator                  *allocator;
    Platform_Sync_Allocators_Function sync_allocators;
    
    Platform_Open_File_Function         open_file;
    Platform_Close_File_Function        close_file;
    Platform_Read_File_Function         read_file;
    Platform_Read_Entire_File_Function  read_entire_file;
    Platform_Write_Entire_File_Function write_entire_file;
    
    Platform_Display_Window_Function     display_window;
    Platform_Skip_Window_Update_Function skip_window_update;
    
    Platform_Get_Logical_Processor_Count_Function get_logical_processor_count;
    
    Platform_Create_Worker_Threads_Function create_worker_threads;
    Platform_Enqueue_Work_Function          enqueue_work;
    Platform_Do_Next_Work_Funtion           do_next_work;
    //Platform_Work_All_Done_Function       work_all_done;
    Platform_Get_Finished_Work_Function     get_finished_work;
    Platform_Work_Reset_Function            work_reset;
    
    Platform_Create_Thread_Function   create_thread;
    
    Platform_Mutex_Create_Function    mutex_create;
    Platform_Mutex_Destroy_Function   mutex_destroy;
    Platform_Mutex_Lock_Function      mutex_lock;
    Platform_Mutex_Unlock_Function    mutex_unlock;
    
    Platform_Run_Command_Function           run_command;
    Platform_Get_Working_Directory_Function get_working_directory;
    
    Platform_Get_Clipboard_Text_Function get_clipboard_text;
    Platform_Copy_To_Clipboard_Function  copy_to_clipboard;
    
    u32 worker_thread_count;
    
    Platform_Messages messages;
};

#define _APP_INIT_DEC(name) any name(Platform_API *platform_api, Game_Input const *input)
typedef _APP_INIT_DEC((*App_Init_Function));

struct Sound_Buffer {
    u8_array output;
    u32 channel_count;
    u32 bytes_per_sample;
    u32 samples_per_second;
    u64 frame;
    u64 frame_count;
    u64 debug_play_frame;
    u64 debug_write_frame;
};

enum Platform_Main_Loop_Result {
    Platform_Main_Loop_Quit = 0,
    Platform_Main_Loop_Continue,
    Platform_Main_Loop_Wait_For_Input,
};

#define _APP_MAIN_LOOP_DEC(name) Platform_Main_Loop_Result name(Platform_API *platform_api, Game_Input const *input, Sound_Buffer *output_sound_buffer, any app_data_ptr, double delta_seconds)
typedef _APP_MAIN_LOOP_DEC((*App_Main_Loop_Function));


#define _APP_UPDATE_SOUND(name) void name(Platform_API *platform_api, u8_array buffer, u32 samples_per_second, u32 channel_count, u32 bytes_per_sample)
typedef _APP_UPDATE_SOUND((*Platform_Update_Sound));

#define APP_INIT_DEC(name)      PLATFORM_SHARE _APP_INIT_DEC(name)
#define APP_MAIN_LOOP_DEC(name) PLATFORM_SHARE _APP_MAIN_LOOP_DEC(name)
#define APP_UPDATE_SOUND(name)  PLATFORM_SHARE _APP_UPDATE_SOUND(name)

#endif // PLATFORM_H
