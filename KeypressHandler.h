#ifndef _KEYPRESS_HANDLER_H
#define _KEYPRESS_HANDLER_H

#include <windows.h>

//should not be static, maybe some games require longer keypress then other
// as i checked mouse movement is using 10 MS
#define SLEEP_BETWEEN_KEYPRESS	20

enum ScanCode
{
	SCANCODE_ESC		= 1,
	SCANCODE_CONSOLE	= 41,
	SCANCODE_ENTER		= 28,
	SCANCODE_ALT		= 56,
	SCANCODE_a			= 30,
	SCANCODE_w			= 17,
	SCANCODE_d			= 32,
	SCANCODE_s			= 31,
	SCANCODE_q			= 16,
	SCANCODE_e			= 18,
};

// http://msdn.microsoft.com/en-us/library/windows/desktop/ms646273%28v=vs.85%29.aspx

struct GlobalStateStore;

void SendKeyPress1( int code );
void SendKeyPress2( int code );
void SendKeyPress3( int code, int PressDownDelay = 0, int ReleaseDelay = 0 );
void SendMouseChange( int state, int flags, int Xchange, int Ychange, int PressDownDelay = 0, int ReleaseDelay = 0 );

DWORD WINAPI KeyScannerThread( LPVOID lpParam );

int CanClickWithMouse( int ClickFlags );

#endif