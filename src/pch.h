#pragma once

#include "general.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
typedef struct HWND__ *Window_Type;
#endif

#ifdef _WIN32
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>

#define SafeRelease(ptr) if (ptr) { ptr->Release(); ptr = NULL; } else {}

#ifdef _DEBUG

#define AssertHR(hr) if (hr != ERROR_SUCCESS) { print_hr(hr); assert(false); } else {}

inline void print_hr(HRESULT hr) {
    wchar_t buf[BUFSIZ] = {};
    DWORD num_chars = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, hr, 0, buf, BUFSIZ, NULL);
    fprintf(stderr, "Error value: %d Message %ws\n", hr, num_chars ? buf : L"Error message not found");
}

#else
#define AssertHR(hr)
#endif

#endif
