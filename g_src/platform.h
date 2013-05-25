#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#ifdef WIN32
#undef WINDOWS_LEAN_AND_MEAN
#define WINDOWS_LEAN_AND_MEAN
# include <windows.h>
#else

#define stricmp strcasecmp
#define strnicmp strncasecmp

enum {
	// NOTE: These probably don't match Windows values.
	MB_OK    = 0x01,
	MB_YESNO = 0x02,
	MB_ICONQUESTION    = 0x10,
	MB_ICONEXCLAMATION = 0x20,

	IDOK = 1,
	IDNO,
	IDYES,
};


typedef int HANDLE;
typedef HANDLE HINSTANCE;
typedef HANDLE HWND;
typedef HANDLE HDC;
typedef HANDLE HGLRC;

#ifndef HWND_DESKTOP
#define HWND_DESKTOP ((HWND)-1)
#endif


typedef int BOOL;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

typedef unsigned short WORD;
typedef unsigned long DWORD;

typedef unsigned int UINT;
typedef short SHORT;
typedef long LONG;
typedef long long LONGLONG;

typedef WORD WPARAM;
typedef DWORD LPARAM;


typedef struct {
	LONG x;
	LONG y;
} POINT;

typedef union {
	struct {
		DWORD LowPart;
		LONG HighPart;
	};
	struct {
		DWORD LowPart;
		LONG HighPart;
	} u;
	LONGLONG QuadPart;
} LARGE_INTEGER;

typedef struct {
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
	DWORD time;
	POINT pt;
} MSG;


DWORD GetTickCount();	// returns ms since system startup
BOOL CreateDirectory(const char* pathname, void*);
BOOL DeleteFile(const char* filename);
void ZeroMemory(void* dest, int len);
BOOL QueryPerformanceCounter(LARGE_INTEGER* performanceCount);
BOOL QueryPerformanceFrequency(LARGE_INTEGER* performanceCount);
int MessageBox(HWND *dummy, const char* text, const char* caption, UINT type);
char* itoa(int value, char* result, int base);

#endif // WIN32

/*
SHORT Enabler_GetKeyState(int virtKey);
int Enabler_ShowCursor(BOOL show);
*/

#endif // _PLATFORM_H_

