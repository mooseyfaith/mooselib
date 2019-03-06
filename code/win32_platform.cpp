#include "win32_platform.h"

#include <stdio.h>

int main(int argc, const char **args)
{
    init_Memory_Stack_allocators();
    init_Memory_Growing_Stack_allocators();
    init_Memory_List_allocators();
    init_c_allocator();
    
    Win32_Platform_API win32_platform_api;
    init_win32_api(&win32_platform_api);
    global_win32_api = &win32_platform_api;
    
    // if you need more than 8 windows at a time, change this
    Win32_Window _window_buffer[8];
    win32_platform_api.window_buffer = ARRAY_INFO(_window_buffer);
    
    u32 path_count = 0;
    for (u32 i = 0; args[0][i]; ++i) {
        if (args[0][i] == '\\')
            path_count = i + 1;
    }
    
    win32_platform_api.application_info.dll_path = { (u8 *)args[0], path_count };
    win32_platform_api.application_info.dll_name       = S(WIN32_DLL_NAME);
    win32_platform_api.application_info.init_name      = S(WIN32_INIT_FUNCTION_NAME);
    win32_platform_api.application_info.main_loop_name = S(WIN32_MAIN_LOOP_FUNCTION_NAME);
    
    win32_load_application(&win32_platform_api.transient_memory.allocator, &win32_platform_api.application_info);
    
    // try different xinput dlls for xbox360 controller support
    HMODULE xinput_dll = LoadLibraryA("xinput1_4.dll");
    
    if (!xinput_dll)
        xinput_dll = LoadLibraryA("xinput1_3.dll");
    
    if (!xinput_dll)
        xinput_dll = LoadLibraryA("xinput9_1_0.dll");
    
    XInput_GetState_Function XInputGetState;
    if (xinput_dll) {
        XInputGetState = (XInput_GetState_Function)GetProcAddress(xinput_dll, "XInputGetState");
        
        if (!XInputGetState)
            XInputGetState = XInputGetStateStub;
    }
    else {
        XInputGetState = XInputGetStateStub;
    }
    
    Input_Gamepad gamepads[XUSER_MAX_COUNT];
    
    win32_platform_api.input.gamepads = gamepads;
    win32_platform_api.input.gamepad_count = XUSER_MAX_COUNT;
    
    win32_create_worker_threads(&win32_platform_api.persistent_memory.allocator, &win32_platform_api.worker_queue);
    win32_platform_api.platform_api.worker_queue        = &win32_platform_api.worker_queue;
    win32_platform_api.platform_api.worker_thread_count = win32_platform_api.worker_queue.thread_count;
    
    const u32 sound_buffer_samples_per_second = 48000;
    const u32 sound_buffer_bytes_per_sample   = sizeof(s16) * 2;
    const u32 sound_buffer_scond_count        = 1;
    const u32 sound_buffer_byte_count = sound_buffer_samples_per_second * sound_buffer_bytes_per_sample * sound_buffer_scond_count;
    
    // create dummy window for open gl
    {
        Win32_Window dummy_window;
        win32_register_class(&win32_platform_api.window_class, "mooselib Window Class", win32_platform_api.windows_instance, DefWindowProc);
        dummy_window.handle = win32_create_window(&win32_platform_api.window_class, S("dummy"), 100, 100, win32_platform_api.windows_instance);
        dummy_window.device_context = GetDC(dummy_window.handle);
        
        win32_set_gl_pixel_format(&dummy_window);
        win32_platform_api.gl_context = wglCreateContext(dummy_window.device_context);
        
        wglMakeCurrent(dummy_window.device_context, win32_platform_api.gl_context);
        
        // check for gl core profile
        init_wgl(dummy_window.device_context);
        
        // create gl core profile context, if available
        if (wglCreateContextAttribsARB) {
            wglMakeCurrent(NULL, NULL);
            DestroyWindow(dummy_window.handle);
            wglDeleteContext(win32_platform_api.gl_context);
            
            dummy_window.handle = win32_create_window(&win32_platform_api.window_class, S("dummy"), 100, 100, win32_platform_api.windows_instance);
            dummy_window.device_context = GetDC(dummy_window.handle);
            
            win32_set_gl_core_pixel_format(&win32_platform_api, &dummy_window, 3, 3);
            
            int context_attributes[] = {
                WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
                0
            };
            win32_platform_api.gl_context = wglCreateContextAttribsARB(dummy_window.device_context, NULL, context_attributes);
            wglMakeCurrent(dummy_window.device_context, win32_platform_api.gl_context);
        }
        
        init_gl();
        wglSwapIntervalEXT(1); // enble v-sync
        
#if DEBUG
        if (glDebugMessageCallback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // message will be generatet in function call scope
            glDebugMessageCallback(win32_gl_debug_callback, NULL);
        }
#endif
        win32_platform_api.application_info.data = win32_platform_api.application_info.init(cast_p(Platform_API, &win32_platform_api), &win32_platform_api.input);
        
        win32_platform_api.sound_buffer = win32_init_dsound(dummy_window.handle, sound_buffer_samples_per_second, sound_buffer_byte_count);
        win32_platform_api.sound_buffer->Play(0, 0, DSBPLAY_LOOPING);
        
        // we only need the gl_context and the sound_buffer, so destroy the dummy window
        wglMakeCurrent(NULL, NULL);
        DestroyWindow(dummy_window.handle);
        
        // register window class with out real window_callback for user windows
        UnregisterClass(win32_platform_api.window_class.lpszClassName, win32_platform_api.windows_instance);
        win32_register_class(&win32_platform_api.window_class, win32_platform_api.window_class.lpszClassName, win32_platform_api.windows_instance, win32_window_callback);
    }
    
    QueryPerformanceFrequency(&win32_platform_api.ticks_per_second);
    QueryPerformanceCounter(&win32_platform_api.last_time);
    
    MSG msg;
    bool do_continue = true;
    while (do_continue)
    {
        clear(&win32_platform_api.transient_memory);
        
        Platform_Messages win32_messages = {};
        
#if 0
        const u32 Max_Message_Count = 3;
        u32 message_count = 0;
        
        static LARGE_INTEGER last_message_time;
        
        while ((message_count < Max_Message_Count) && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
#else
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
#endif
        
        {
            Win32_Window *window = cast_p(Win32_Window, GetWindowLongPtr(msg.hwnd, GWLP_USERDATA));
            
            switch (msg.message) {
                case WM_QUIT: {
                    do_continue = false;
                } break;
                
                case WM_CHAR: {
                    auto *message = ALLOCATE(&win32_platform_api.transient_memory.allocator, Platform_Message_Character);
                    message->kind = Platform_Message_Kind_Character;
                    message->code = msg.wParam;
                    
                    insert_tail(&win32_platform_api.transient_memory.allocator, &win32_messages)->value = cast_p(Platform_Message, message);
                } break;
                
                case WM_UNICHAR: {
                    auto *message = ALLOCATE(&win32_platform_api.transient_memory.allocator, Platform_Message_Character);
                    message->kind = Platform_Message_Kind_Character;
                    message->code = msg.wParam;
                    
                    insert_tail(&win32_platform_api.transient_memory.allocator, &win32_messages)->value = cast_p(Platform_Message, message);
                } break;
                
                case WM_SYSKEYDOWN:
                case WM_KEYDOWN: {
                    bool last_key_was_down = msg.lParam & (1 << 30);
                    
                    if (!last_key_was_down)
                        key_event_update(&win32_platform_api.input, (char)msg.wParam, true);
                    
                    {
                        // WARNING ignoring characters that are also send be WM_UNICHAR
                        // Platform_Character_Backspace = 8,
                        // Platform_Character_Tab       = 9,
                        // Platform_Character_Return    = 13,
                        // Platform_Character_Escape    = 27,
                        
                        u32 code;
                        switch (msg.wParam) {
                            case VK_RIGHT: {
                                code = Platform_Character_Right;
                            }break;
                            
                            case VK_UP: {
                                code = Platform_Character_Up;
                            } break;
                            
                            case VK_LEFT: {
                                code = Platform_Character_Left;
                            } break;
                            
                            case VK_DOWN: {
                                code = Platform_Character_Down;
                            } break;
                            
                            case VK_INSERT: {
                                code = Platform_Character_Insert;
                            } break;
                            
                            case VK_DELETE: {
                                code = Platform_Character_Delete;
                            } break;
                            
                            case VK_PRIOR: {
                                code = Platform_Character_Page_Up;
                            } break;
                            
                            case VK_NEXT: {
                                code = Platform_Character_Page_Down;
                            } break;
                            
                            case VK_END: {
                                code = Platform_Character_End;
                            } break;
                            
                            case VK_HOME: {
                                code = Platform_Character_Home;
                            } break;
                            
                            default:
                            code = 0;
                        }
                        
                        if (code) {
                            auto *message = ALLOCATE(&win32_platform_api.transient_memory.allocator, Platform_Message_Character);
                            message->kind = Platform_Message_Kind_Character;
                            message->code = code;
                            
                            insert_tail(&win32_platform_api.transient_memory.allocator, &win32_messages)->value = cast_p(Platform_Message, message);
                        }
                    }
                    
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                } break;
                
                case WM_SYSKEYUP:
                case WM_KEYUP: {
                    key_event_update(&win32_platform_api.input, (char)msg.wParam, false);
                    
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                } break;
                
#if 0                
                case WM_MOUSEMOVE: {
                    if (!win32_platform_api.input.mouse.is_inside_window) {
                        win32_platform_api.input.mouse.is_inside_window = true;
                        
#if 0                        
                        TRACKMOUSEEVENT track_mouse_event = {};
                        track_mouse_event.cbSize = sizeof(track_mouse_event);
                        track_mouse_event.dwFlags = TME_LEAVE;
                        track_mouse_event.hwndTrack = window->handle;
                        
                        TrackMouseEvent(&track_mouse_event);
#endif
                    }
                    
                    win32_platform_api.input.mouse.window_id = window->id;
                    win32_platform_api.input.mouse.window_position = vec2f{ CAST_V(f32, GET_X_LPARAM(msg.lParam)), CAST_V(f32, window->new_area.height - GET_Y_LPARAM(msg.lParam) - 1) };
                } break;
#endif
                
#if 0                
                case WM_MOUSELEAVE: {
                    win32_platform_api.input.mouse.is_inside_window = false;
                    win32_platform_api.input.mouse.window_id = -1;
                    
                    button_event_update(&win32_platform_api.input.mouse.left, false);
                    button_event_update(&win32_platform_api.input.mouse.middle, false);
                    button_event_update(&win32_platform_api.input.mouse.right, false);
                } break;
#endif
                
#if 0                
                case WM_LBUTTONDOWN: {
                    button_event_update(&win32_platform_api.input.mouse.left, true);
                } break;
                
                case WM_LBUTTONUP: {
                    button_event_update(&win32_platform_api.input.mouse.left, false);
                } break;
                
                case WM_MBUTTONDOWN: {
                    button_event_update(&win32_platform_api.input.mouse.middle, true);
                } break;
                
                case WM_MBUTTONUP: {
                    button_event_update(&win32_platform_api.input.mouse.middle, false);
                } break;
                
                case WM_RBUTTONDOWN: {
                    button_event_update(&win32_platform_api.input.mouse.right, true);
                } break;
                
                case WM_RBUTTONUP: {
                    button_event_update(&win32_platform_api.input.mouse.right, false);
                } break;
#endif
                
                case WM_MOUSEWHEEL: {
                    win32_platform_api.input.mouse.vertical_wheel_delta += GET_WHEEL_DELTA_WPARAM(msg.wParam) / cast_v(f32, WHEEL_DELTA);
                } break;
                
                case WM_MOUSEHWHEEL: {
                    win32_platform_api.input.mouse.horizontal_wheel_delta += GET_WHEEL_DELTA_WPARAM(msg.wParam) / cast_v(f32, WHEEL_DELTA);
                } break;
                
                case WM_DROPFILES: {
                    HDROP drop = (HDROP)msg.wParam;
                    
                    u32 file_count = DragQueryFileA(drop, 0xFFFFFFFF, null, 0);
                    
                    for (u32 i = 0; i < file_count; ++i) {
                        auto *message = ALLOCATE(&win32_platform_api.transient_memory.allocator, Platform_Message_File_Drop);
                        message->kind = Platform_Message_Kind_File_Drop;
                        
                        u32 full_path_count = DragQueryFileA(drop, i, null, 0);
                        
                        message->full_path = ALLOCATE_ARRAY_INFO(&win32_platform_api.transient_memory.allocator, u8, full_path_count + 1);
                        u32 debug_full_path_count = DragQueryFileA(drop, i, cast_p(char, message->full_path.data), full_path_count + 1);
                        assert(debug_full_path_count == full_path_count);
                        
                        DWORD attributes = GetFileAttributesA(cast_p(char, message->full_path.data));
                        
                        // drop terminating 0
                        message->full_path.count--;
                        REALLOCATE_ARRAY(&win32_platform_api.transient_memory.allocator, &message->full_path.data, message->full_path.count);
                        
                        {
                            
                            string name = {};
                            string it = message->full_path;
                            while (it.count) {
                                if (utf8_advance(&it) == '\\')
                                    name = it;
                            }
                            
                            message->name = name;
                            message->path = { message->full_path.data, cast_v(usize, name.data - message->full_path.data) };
                        }
                        
                        if (attributes & FILE_ATTRIBUTE_DIRECTORY)
                            message->is_directory = true;
                        else {
                            message->is_directory = false;
                            
                            string it = message->name;
                            
                            string extension = {};
                            while (it.count) {
                                if (utf8_advance(&it) == '.')
                                    extension = it;
                            }
                            
                            if (extension.count) {
                                message->extension = extension;
                                message->name.count -= extension.count + 1;
                            }
                        }
                        
                        insert_tail(&win32_platform_api.transient_memory.allocator, &win32_messages)->value = cast_p(Platform_Message, message);
                        
                        printf("file: %.*s\n", FORMAT_S(&message->full_path));
                    }
                    
                    DragFinish(drop);
                } break;
                
                /*case WM_INPUT:
                            {
                            UINT dwSize;
                            
                            GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
                            LPBYTE lpb = new BYTE[dwSize];
                            
                            if (lpb == NULL)
                            return 0;
                            
                            if (GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, lpb, &dwSize,
                            sizeof(RAWINPUTHEADER)) != dwSize)
                            OutputDebugString(TEXT("GetRawInputData does not return correct size !\n"));
                            
                            RAWINPUT* raw = (RAWINPUT*)lpb;
                            
                            if (raw->header.dwType == RIM_TYPEKEYBOARD)
                            {
                            hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"),
                            raw->data.keyboard.MakeCode,
                            raw->data.keyboard.Flags,
                            raw->data.keyboard.Reserved,
                            raw->Data.keyboard.ExtraInformation,
                            raw->data.keyboard.Message,
                            raw->data.keyboard.VKey);
                            if (FAILED(hResult))
                            {
                            // TODO: write error handler
                            }
                            OutputDebugString(szTempOutput);
                            }
                            else if (raw->header.dwType == RIM_TYPEMOUSE)
                            {
                            hResult = StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT("Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"),
                            raw->data.mouse.usFlags,
                            raw->data.mouse.ulButtons,
                            raw->data.mouse.usButtonFlags,
                            raw->data.mouse.usButtonData,
                            raw->data.mouse.ulRawButtons,
                            raw->data.mouse.lLastX,
                            raw->data.mouse.lLastY,
                            raw->data.mouse.ulExtraInformation);
                            
                            if (FAILED(hResult))
                            {
                            // TODO: write error handler
                            }
                            OutputDebugString(szTempOutput);
                            }
                            
                            delete[] lpb;
                            return 0;
                            }*/
                
                default: {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            
#if 0
            ++message_count;
            
            LARGE_INTEGER message_time;
            QueryPerformanceCounter(&message_time);
            
            printf("msg: %i (%f)\n", msg.message, (message_time.QuadPart - last_message_time.QuadPart) / (double)ticks_per_second.QuadPart);
            last_message_time = message_time;
#endif
        }
        
#if 0        
        if (message_count)
            printf("message count: %d\n", message_count);
#endif
        
        if (!do_continue)
            break;
        
        {
            LARGE_INTEGER time;
            QueryPerformanceCounter(&time);
            
            LARGE_INTEGER delta_time;
            delta_time.QuadPart = time.QuadPart - win32_platform_api.last_time.QuadPart;
            
            f64 delta_seconds = delta_time.QuadPart / cast_v(f64, win32_platform_api.ticks_per_second.QuadPart);
            win32_platform_api.last_time = time;
            
            if (win32_load_application(&win32_platform_api.transient_memory.allocator, &win32_platform_api.application_info))
            {
                // on dll relaod
                // ivalidate title string
                // it might point to old data, since we did not clone the string
                // this will force a renameing of the window
                for (u32 i = 0;
                     i < win32_platform_api.window_buffer.count;
                     ++i)
                {
                    win32_platform_api.window_buffer[i].title = {};
                }
                
                auto message = ALLOCATE(&win32_platform_api.transient_memory.allocator, Platform_Message);
                message->kind = Platform_Message_Kind_Reload;
                insert_tail(&win32_platform_api.transient_memory.allocator, &win32_messages)->value= cast_p(Platform_Message, message);
            }
            
            button_poll_update(&win32_platform_api.input.mouse.left, GetKeyState(VK_LBUTTON) & 0x8000);
            
            button_poll_update(&win32_platform_api.input.mouse.right, GetKeyState(VK_RBUTTON) & 0x8000);
            
            button_poll_update(&win32_platform_api.input.mouse.middle, GetKeyState(VK_MBUTTON) & 0x8000);
            
            button_poll_update(&win32_platform_api.input.left_shift, GetKeyState(VK_LSHIFT) & 0x8000);
            button_poll_update(&win32_platform_api.input.right_shift, GetKeyState(VK_RSHIFT) & 0x8000);
            
            button_poll_update(&win32_platform_api.input.left_alt, GetKeyState(VK_LMENU) & 0x8000);
            button_poll_update(&win32_platform_api.input.right_alt, GetKeyState(VK_RMENU) & 0x8000);
            
            button_poll_update(&win32_platform_api.input.left_control, GetKeyState(VK_LCONTROL) & 0x8000);
            button_poll_update(&win32_platform_api.input.right_control, GetKeyState(VK_RCONTROL) & 0x8000);
            
            for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
                XINPUT_STATE state = {};
                DWORD result = XInputGetState(i, &state);
                
                if (result == ERROR_SUCCESS) {
                    win32_platform_api.input.gamepads[i].is_enabled = true;
                    
                    win32_platform_api.input.gamepads[i].left_stick.raw_direction[0] = win32_gamepad_normalized_axis(state.Gamepad.sThumbLX, 32768.0f, 32767.0f);
                    win32_platform_api.input.gamepads[i].left_stick.raw_direction[1] = win32_gamepad_normalized_axis(state.Gamepad.sThumbLY, 32768.0f, 32767.0f);
                    
                    win32_platform_api.input.gamepads[i].right_stick.raw_direction[0] = win32_gamepad_normalized_axis(state.Gamepad.sThumbRX, 32768.0f, 32767.0f);
                    win32_platform_api.input.gamepads[i].right_stick.raw_direction[1] = win32_gamepad_normalized_axis(state.Gamepad.sThumbRY, 32768.0f, 32767.0f);
                    
                    float dead_zone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE / 32767.0f;
                    normalize_stick(win32_platform_api.input.gamepads + i, dead_zone);
                    
                    win32_update_button(&win32_platform_api.input.gamepads[i].digital_pad.up, state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_UP, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].digital_pad.down, state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].digital_pad.right, state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].digital_pad.left, state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT, cast_v(f32, delta_seconds));
                    
                    win32_update_button(&win32_platform_api.input.gamepads[i].action_pad.down, state.Gamepad.wButtons, XINPUT_GAMEPAD_A, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].action_pad.right, state.Gamepad.wButtons, XINPUT_GAMEPAD_B, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].action_pad.left, state.Gamepad.wButtons, XINPUT_GAMEPAD_X, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].action_pad.up, state.Gamepad.wButtons, XINPUT_GAMEPAD_Y, cast_v(f32, delta_seconds));
                    
                    win32_update_button(&win32_platform_api.input.gamepads[i].left_shoulder, state.Gamepad.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].right_shoulder, state.Gamepad.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER, cast_v(f32, delta_seconds));
                    
                    win32_update_button(&win32_platform_api.input.gamepads[i].left_stick_button, state.Gamepad.wButtons, XINPUT_GAMEPAD_LEFT_THUMB, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].right_stick_button, state.Gamepad.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB, cast_v(f32, delta_seconds));
                    
                    win32_update_button(&win32_platform_api.input.gamepads[i].select_button, state.Gamepad.wButtons, XINPUT_GAMEPAD_BACK, cast_v(f32, delta_seconds));
                    win32_update_button(&win32_platform_api.input.gamepads[i].start_button, state.Gamepad.wButtons, XINPUT_GAMEPAD_START, cast_v(f32, delta_seconds));
                }
                else
                    win32_platform_api.input.gamepads[i].is_enabled = false;
            }
            
            for (u32 i = 0; i < win32_platform_api.window_buffer.count; ++i) {
                win32_platform_api.window_buffer[i].is_active = false;
            }
            
            u8 _sound_output[sound_buffer_byte_count];
            
            Sound_Buffer output_sound_buffer       = {};
            output_sound_buffer.output             = ARRAY_INFO(_sound_output);
            output_sound_buffer.samples_per_second = sound_buffer_samples_per_second;
            output_sound_buffer.bytes_per_sample   = sound_buffer_bytes_per_sample;
            output_sound_buffer.channel_count      = 2;
            output_sound_buffer.frame_count        = sound_buffer_byte_count / sound_buffer_bytes_per_sample;
            
            void *sound_buffer_region_data[2];
            DWORD sound_buffer_region_count[2];
            
            static u64 sound_frame = 0;
            
            u64 delta_frames = delta_time.QuadPart * sound_buffer_samples_per_second / win32_platform_api.ticks_per_second.QuadPart;
            sound_frame = MOD(sound_frame + delta_frames, sound_buffer_byte_count / sound_buffer_bytes_per_sample);
            
            {
                DWORD sound_play_cursor;
                DWORD sound_write_cursor;
                
                if (SUCCEEDED(win32_platform_api.sound_buffer->GetCurrentPosition(&sound_play_cursor, &sound_write_cursor))) {
#if 1                  
                    u32 target_cursor = sound_frame * sound_buffer_bytes_per_sample;
                    u32 write_count = sound_buffer_samples_per_second * sound_buffer_bytes_per_sample / 2;
                    
                    bool do_reset = false;
                    if (sound_play_cursor <= sound_write_cursor) {
                        if (target_cursor < sound_write_cursor) {
                            do_reset = true;
                        }
                    }
                    else if ((target_cursor < sound_write_cursor) || (sound_play_cursor < target_cursor)) {
                        do_reset = true;
                    }
                    
                    if (do_reset) {
                        sound_frame   = MOD(sound_play_cursor / sound_buffer_bytes_per_sample + delta_frames, output_sound_buffer.frame_count);
                        
                        if (sound_frame * sound_buffer_bytes_per_sample < sound_write_cursor)
                            sound_frame = sound_write_cursor / sound_buffer_bytes_per_sample;
                        
                        target_cursor = sound_frame * sound_buffer_bytes_per_sample;
                    }
                    
                    // write half a second of sound ahead
#else
                    u32 target_cursor = sound_write_cursor;
                    
                    u32 write_count;
                    if (sound_write_cursor >= sound_play_cursor)
                        write_count = sound_buffer_byte_count - (sound_write_cursor - sound_play_cursor);
                    else 
                        write_count = sound_play_cursor - sound_write_cursor;
                    
                    sound_frame = sound_write_cursor / sound_buffer_bytes_per_sample;
#endif
                    
                    //if (sound_buffer->Lock(target_cursor, write_count, &sound_buffer_region_data[0], &sound_buffer_region_count[0], &sound_buffer_region_data[1], &sound_buffer_region_count[1], 0) == DS_OK)
                    {
                        output_sound_buffer.output.count = write_count;
                        output_sound_buffer.frame        = target_cursor / sound_buffer_bytes_per_sample;
                        
                        output_sound_buffer.debug_play_frame = sound_play_cursor / sound_buffer_bytes_per_sample;
                        output_sound_buffer.debug_write_frame = sound_write_cursor / sound_buffer_bytes_per_sample;
                    }
#if 0
                    else {
                        output_sound_buffer.output.count = 0;
                    }
#endif
                }
            }
            
            win32_platform_api.platform_api.messages = win32_messages;
            
#if 0            
            POINT point;
            GetCursorPos(&point);
            
            for (u32 i = 0; i < win32_platform_api.window_buffer.count; ++i) {
                POINT p = point;
                if (ScreenToClient(win32_platform_api.window_buffer[i].handle, &p)) {
                    win32_platform_api.input.mouse.window_position = { cast_v(f32, p.x), cast_v(f32, win32_platform_api.window_buffer[i].new_area.height - p.y - 1) };
                }
            }
#endif
            LARGE_INTEGER frame_start;
            QueryPerformanceCounter(&frame_start);
            
            auto main_loop_result = win32_platform_api.application_info.main_loop(cast_p(Platform_API, &win32_platform_api), &win32_platform_api.input, &output_sound_buffer, win32_platform_api.application_info.data, delta_seconds);
            
            if (main_loop_result == Platform_Main_Loop_Quit)
                PostQuitMessage(0);
            
            LARGE_INTEGER frame_end;
            QueryPerformanceCounter(&frame_end);
            
            if (output_sound_buffer.output.count) {
                u32 output_offset = 0;
                
                if (win32_platform_api.sound_buffer->Lock(output_sound_buffer.frame * sound_buffer_bytes_per_sample, output_sound_buffer.output.count, &sound_buffer_region_data[0], &sound_buffer_region_count[0], &sound_buffer_region_data[1], &sound_buffer_region_count[1], 0) == DS_OK) {
                    
                    for (u32 region_index = 0; region_index < ARRAY_COUNT(sound_buffer_region_data); ++region_index) {
                        copy(sound_buffer_region_data[region_index], output_sound_buffer.output.data + output_offset, sound_buffer_region_count[region_index]);
                        output_offset += sound_buffer_region_count[region_index];
                    }
                    
                    win32_platform_api.sound_buffer->Unlock(sound_buffer_region_data[0], sound_buffer_region_count[0], sound_buffer_region_data[1], sound_buffer_region_count[1]);
                }
            }
            
            win32_platform_api.swap_buffer_count = 0;
            
            advance_input(&win32_platform_api.input, delta_seconds);
            
            auto render_time = (frame_end.QuadPart - frame_start.QuadPart) / cast_v(f64, win32_platform_api.ticks_per_second.QuadPart);
            
            {
                // wait a little shorter than we just required to render
                // assuming the render time is stable
                const f64 target_seconds_per_frame = 1.0 / 60.0;
                auto sleep_time = (target_seconds_per_frame - render_time * 1.2f);
                
                if (sleep_time > 0.0f)
                    Sleep(sleep_time * 1000);
            }
            
            if (win32_platform_api.current_window && win32_platform_api.current_window->is_active)
            {
                SwapBuffers(win32_platform_api.current_window->device_context);
                win32_platform_api.current_window = null;
            }
            
            if (main_loop_result == Platform_Main_Loop_Wait_For_Input)
                WaitMessage();
            
            {
                u32 i = 0;
                while (i < win32_platform_api.window_buffer.count) {
                    if (!win32_platform_api.window_buffer[i].is_active)
                        DestroyWindow(win32_platform_api.window_buffer[i].handle);
                    
                    if (win32_platform_api.window_buffer[i].was_destroyed) {
                        win32_platform_api.window_buffer[i] = win32_platform_api.window_buffer[win32_platform_api.window_buffer.count - 1];
                        win32_platform_api.window_buffer.count--;
                        
                        // update handle to window maping, after swap
                        if (i < win32_platform_api.window_buffer.count) {
                            SetWindowLongPtr(win32_platform_api.window_buffer[i].handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win32_platform_api.window_buffer.data + i));
                        }
                    }
                    else {
                        ++i;
                    }
                }
            }
            
#if 0            
            LARGE_INTEGER message_time;
            QueryPerformanceCounter(&message_time);
            
            printf("loop time: %f\n", (message_time.QuadPart - last_message_time.QuadPart) / (double)ticks_per_second.QuadPart);
            last_message_time = message_time;
#endif
        }
    }
    
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(win32_platform_api.gl_context);
    
    //ToDo: call app_finish
    
    return (int)msg.wParam;
}

#if 0
LRESULT CALLBACK main_window_callback(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    Main_Window_Callback_User_Data *user_data = reinterpret_cast<Main_Window_Callback_User_Data *> (GetWindowLongPtr(window, GWLP_USERDATA));
    
    switch (message) {
        
        case WM_SIZE: {
            if (!user_data)
                return 0;
            
            s16 width = LOWORD(l_param);
            s16 height = HIWORD(l_param);
            user_data->window_resolution->width = width;
            user_data->window_resolution->height = height;
        } break;
        
        case WM_ACTIVATE:
        {
            if (!user_data)
                return 0;
            
            // handle window focus change in boarderless fullscreen mode
            if (*user_data->is_full_screen) {
                switch (w_param) {
                    case WA_ACTIVE:
                    {
                        SetWindowPos(*user_data->window_handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);
                        break;
                    }
                    case WA_INACTIVE:
                    {
                        SetWindowPos(*user_data->window_handle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
                        break;
                    }
                }
            }
            
            if (w_param == WA_INACTIVE) {
                for (u32 i = 0; i < ARRAY_COUNT(user_data->input_buffer->new_input.keys); ++i)
                    button_event_update(user_data->input_buffer->new_input.keys + i, false);
            }
            else {
                BYTE key_states[256];
                GetKeyboardState(key_states);
                
                for (u32 i = 0; i < ARRAY_COUNT(user_data->input_buffer->new_input.keys); ++i)
                    button_event_update(user_data->input_buffer->new_input.keys + i, key_states[i] & (1 << 7));
            }
        } break;
        
        case WM_SETFOCUS: {
            BYTE key_states[256];
            GetKeyboardState(key_states);
            
            for (u32 i = 0; i < ARRAY_COUNT(user_data->input_buffer->new_input.keys); ++i)
                button_event_update(user_data->input_buffer->new_input.keys + i, (key_states[i] & (1 << 7)));
        } break;
        
        case WM_KILLFOCUS: {
            for (u32 i = 0; i < ARRAY_COUNT(user_data->input_buffer->new_input.keys); ++i)
                button_event_update(user_data->input_buffer->new_input.keys + i, false);
        } break;
        
        case WM_DESTROY: {
            PostQuitMessage(0);
        } break;
        
        default:
        return DefWindowProc(window, message, w_param, l_param);
    }
    
    return 0;
}
#endif