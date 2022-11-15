#include "pch.h"

#ifdef _WIN32

#include "os.h"
#include "game.h"

#include <windows.h>

static bool window_class_initted = false;
static wchar_t *WINDOW_CLASS_NAME = L"ThingWin32WindowClass";

static Key_Code vk_code_to_key_code(u32 vk_code) {
    if (vk_code >= 48 && vk_code <= 90) return (Key_Code) vk_code;

    switch (vk_code) {
    case VK_BACK: return KEY_BACKSPACE;
    case VK_TAB: return KEY_TAB;
    case VK_ESCAPE: return KEY_ESCAPE;
    case VK_SPACE: return KEY_SPACE;

    case VK_F1: return KEY_F1;
    case VK_F2: return KEY_F2;
    case VK_F3: return KEY_F3;
    case VK_F4: return KEY_F4;
    case VK_F5: return KEY_F5;
    case VK_F6: return KEY_F6;
    case VK_F7: return KEY_F7;
    case VK_F8: return KEY_F8;
    case VK_F9: return KEY_F9;
    case VK_F10: return KEY_F10;
    case VK_F11: return KEY_F11;
    case VK_F12: return KEY_F12;

    case VK_RETURN: return KEY_ENTER;

    case VK_SHIFT: return KEY_SHIFT;
    case VK_CONTROL: return KEY_CTRL;
    case VK_MENU: return KEY_ALT;

    case VK_UP: return KEY_UP;
    case VK_DOWN: return KEY_DOWN;
    case VK_RIGHT: return KEY_RIGHT;
    case VK_LEFT: return KEY_LEFT;
    }

    return KEY_UNKNOWN;
}

static wchar_t *utf8_to_wstring(char *utf8, bool use_temporary_storage = true) {
    if (!utf8) return NULL;

    auto query_num_chars = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (query_num_chars <= 0) return NULL;

    wchar_t *name_bytes;
    if (use_temporary_storage) {
        name_bytes = static_cast <wchar_t *>(talloc((query_num_chars+1)*2));
    } else {
        name_bytes = new wchar_t[query_num_chars+1];
    }
    auto result_num_chars = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, name_bytes, query_num_chars);
    if (result_num_chars > 0) {
        assert(result_num_chars <= query_num_chars);
        name_bytes[result_num_chars] = 0;
        return name_bytes;
    }

    return NULL; // @Leak
}

static char *wstring_to_utf8(wchar_t *wstring, bool use_temporary_storage = true) {
    if (!wstring) return NULL;

    auto query_num_chars = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, NULL, 0, NULL, 0);
    if (query_num_chars <= 0) return NULL;

    char *name_bytes;
    if (use_temporary_storage) {
        name_bytes = static_cast <char *>(talloc((query_num_chars+1)*2));
    } else {
        name_bytes = new char[query_num_chars+1];
    }
    auto result_num_chars = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, name_bytes, query_num_chars, NULL, 0);
    if (result_num_chars > 0) {
        assert(result_num_chars <= query_num_chars);
        name_bytes[result_num_chars] = 0;
        return name_bytes;
    }

    return NULL; // @Leak
}

static LRESULT CALLBACK MyWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CLOSE: {
        Event event;
        event.type = EVENT_TYPE_QUIT;
        globals.events_this_frame.add(event);
        
        break;
    }

    case WM_SIZE: {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        Window_Resize_Record record = {};
        record.width = width;
        record.height = height;
        globals.window_resizes.add(record);
        
        break;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP: {
        u32 vk_code = (u32)wparam;
        Key_Code key_code = vk_code_to_key_code(vk_code);
        bool is_down = (lparam & (1 << 31)) == 0;
        bool alt_pressed = GetKeyState(VK_MENU) & 0x8000;

        Event event;
        event.type = EVENT_TYPE_KEYBOARD;
        event.key_code = key_code;
        event.key_pressed = is_down;
        event.alt_pressed = alt_pressed;
        globals.events_this_frame.add(event);
        
        break;
    }

    case WM_MOUSEMOVE: {
        POINTS pt = MAKEPOINTS(lparam);
        int x = pt.x;
        int y = pt.y;

        Event event = {};
        event.type = EVENT_TYPE_MOUSE_MOVE;
        event.x = x;
        event.y = y;
        globals.events_this_frame.add(event);
        
        break;
    }

    case WM_ACTIVATEAPP: {
        bool active = static_cast <bool>(wparam);
        //cursor_visible = !active;
        
        break;
    }
            
    case WM_LBUTTONDOWN: {
        bool alt_pressed = GetKeyState(VK_MENU) & 0x8000;
            
        Event event = {};
        event.type = EVENT_TYPE_KEYBOARD;
        event.key_code = MOUSE_BUTTON_LEFT;
        event.key_pressed = true;
        event.alt_pressed = alt_pressed;

        POINTS pt = MAKEPOINTS(lparam);
        int x = pt.x;
        int y = pt.y;

        event.x = x;
        event.y = y;
            
        globals.events_this_frame.add(event);
    } break;

    case WM_LBUTTONUP: {
        bool alt_pressed = GetKeyState(VK_MENU) & 0x8000;
            
        Event event = {};
        event.type = EVENT_TYPE_KEYBOARD;
        event.key_code = MOUSE_BUTTON_LEFT;
        event.key_pressed = false;
        event.alt_pressed = alt_pressed;

        POINTS pt = MAKEPOINTS(lparam);
        int x = pt.x;
        int y = pt.y;

        event.x = x;
        event.y = y;
            
        globals.events_this_frame.add(event);
    } break;

    case WM_RBUTTONDOWN: {
        bool alt_pressed = GetKeyState(VK_MENU) & 0x8000;
            
        Event event = {};
        event.type = EVENT_TYPE_KEYBOARD;
        event.key_code = MOUSE_BUTTON_RIGHT;
        event.key_pressed = true;
        event.alt_pressed = alt_pressed;

        POINTS pt = MAKEPOINTS(lparam);
        int x = pt.x;
        int y = pt.y;

        event.x = x;
        event.y = y;
            
        globals.events_this_frame.add(event);
    } break;

    case WM_RBUTTONUP: {
        bool alt_pressed = GetKeyState(VK_MENU) & 0x8000;
            
        Event event = {};
        event.type = EVENT_TYPE_KEYBOARD;
        event.key_code = MOUSE_BUTTON_LEFT;
        event.key_pressed = false;
        event.alt_pressed = alt_pressed;

        POINTS pt = MAKEPOINTS(lparam);
        int x = pt.x;
        int y = pt.y;

        event.x = x;
        event.y = y;
            
        globals.events_this_frame.add(event);
    } break;
        
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
        break;
    }
    
    return 0;
}

static void init_window_class() {
    WNDCLASSEXW wc = {};

    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = MyWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIconW(NULL, L"APPICON");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = static_cast <HBRUSH>(WHITE_BRUSH);

    wc.lpszMenuName = NULL;
    wc.lpszClassName = WINDOW_CLASS_NAME;

    if (RegisterClassExW(&wc) == 0) {
        log_error("Failure in init_window_class(): RegisterClassExW returned 0.\n");
        return;
    }

    window_class_initted = true;
}

Window_Type create_window(int width, int height, char *title) {
    if (!window_class_initted) {
        init_window_class();
    }

    DWORD style = WS_OVERLAPPEDWINDOW;

    RECT wr = {};
    wr.right = width;
    wr.bottom = height;
    AdjustWindowRect(&wr, style, FALSE);

    int window_width = wr.right - wr.left;
    int window_height = wr.bottom - wr.top;

    Window_Type window = CreateWindowExW(0, WINDOW_CLASS_NAME, utf8_to_wstring(title),
                                         style, 0, 0, window_width, window_height,
                                         NULL, NULL, GetModuleHandleW(NULL), NULL);
    if (window == NULL) {
        log_error("Error in create_window: CreateWindowExW returned 0.\n");
        return NULL;
    }

    UpdateWindow(window);
    ShowWindow(window, SW_SHOW);

    return window;
}

void update_window_events() {
    globals.events_this_frame.count = 0;
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

bool file_exists(char *fullpath) {
    return GetFileAttributesW(utf8_to_wstring(fullpath)) != INVALID_FILE_ATTRIBUTES;
}

void init_colors_and_utf8() {
    SetConsoleOutputCP(CP_UTF8);
    HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD mode = 0;
    BOOL result = GetConsoleMode(stdout_handle, &mode);
    if (result) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(stdout_handle, mode);
    }
}

char *get_path_of_running_executable() {
    wchar_t *wide_result = (wchar_t *)talloc(MAX_PATH);
    GetModuleFileNameW(nullptr, wide_result, MAX_PATH);
    
    char *result = wstring_to_utf8(wide_result, false);
    for (char *at = result; *at; at++) {
        if (at[0] == '\\') {
            at[0] = '/';
        }
    }
    return result;
}

void setcwd(char *path) {
    wchar_t *wide_dir = utf8_to_wstring(path);
    for (wchar_t *at = wide_dir; *at; at++) {
        if (at[0] == '/') {
            at[0] = '\\';
        }
    }
    SetCurrentDirectoryW(wide_dir);
}

bool get_file_last_write_time(char *filepath, u64 *modtime) {
    HANDLE file = CreateFileW(utf8_to_wstring(filepath), 0,//GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    defer { CloseHandle(file); };

    FILETIME ft_create, ft_access, ft_write;
    if (!GetFileTime(file, &ft_create, &ft_access, &ft_write)) return false;
    
    ULARGE_INTEGER uli;
    uli.LowPart = ft_write.dwLowDateTime;
    uli.HighPart = ft_write.dwHighDateTime;
    
    if (modtime) *modtime = uli.QuadPart;
    
    return true;
}

double get_time() {
    s64 perf_freq;
    QueryPerformanceFrequency((LARGE_INTEGER * )&perf_freq);
    s64 perf_counter;
    QueryPerformanceCounter((LARGE_INTEGER * )&perf_counter);
    return (double)perf_counter / (double)perf_freq;
}

#endif
