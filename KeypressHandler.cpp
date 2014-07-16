#include "StdAfx.h"
#include <windows.h>

//does not work
//http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
void SendKeyPress1( int code )
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
//			ip.ki.wVk = 0x45; // E
	ip.ki.wVk = 0x12; // Alt
	ip.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &ip, sizeof(INPUT));
	Sleep( SLEEP_BETWEEN_KEYPRESS );
	ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ip, sizeof(INPUT));
}

//does not work
void SendKeyPress2( int code )
{
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0;
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
	ip.ki.wVk = 0; 
	ip.ki.wScan = MapVirtualKey( 0x12, MAPVK_VK_TO_VSC);
	ip.ki.dwFlags = KEYEVENTF_SCANCODE; // 0 for key press
	SendInput(1, &ip, sizeof(INPUT));
	Sleep( SLEEP_BETWEEN_KEYPRESS );
	ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ip, sizeof(INPUT));
}

void SendKeyPress3( int code, int PressDownDelay, int ReleaseDelay )
{
	InterceptionKeyStroke kstroke;
	memset( &kstroke, 0, sizeof( kstroke ) );

	kstroke.code = code;
	kstroke.state = 0;
	interception_send( GlobalStore.context, GlobalStore.KeyboardDevice, (InterceptionStroke *)&kstroke, 1);
	if( PressDownDelay == 0 )
		Sleep( SLEEP_BETWEEN_KEYPRESS );
	else
		Sleep( PressDownDelay );
	kstroke.code = code;
	kstroke.state = 1;	
	interception_send( GlobalStore.context, GlobalStore.KeyboardDevice, (InterceptionStroke *)&kstroke, 1);
	if( ReleaseDelay != 0 )
		Sleep( ReleaseDelay );
}

void SendMouseChange( int state, int flags, int Xchange, int Ychange, int PressDownDelay, int ReleaseDelay )
{
	InterceptionMouseStroke mstroke;
	memset( &mstroke, 0, sizeof( mstroke ) );

//printf( "sending mouse state change %d x %d y %d down %d up %d \n", state, Xchange, Ychange, PressDownDelay, ReleaseDelay );
	mstroke.state = state;
	mstroke.x = Xchange;
	mstroke.y = Ychange;
	if( flags == 0 )
		mstroke.flags = INTERCEPTION_MOUSE_MOVE_RELATIVE;
	else
		mstroke.flags = flags;
	interception_send( GlobalStore.context, GlobalStore.MouseDevice, (InterceptionStroke *)&mstroke, 1);
	if( PressDownDelay == 0 )
		Sleep( SLEEP_BETWEEN_KEYPRESS );
	else
		Sleep( PressDownDelay );
	if( state == INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN || state == INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN )
	{
		mstroke.state = 2 * state;
		mstroke.x = 0;
		mstroke.y = 0;
		interception_send( GlobalStore.context, GlobalStore.MouseDevice, (InterceptionStroke *)&mstroke, 1);
		if( ReleaseDelay != 0 )
			Sleep( ReleaseDelay );
	}
}

DWORD WINAPI KeyScannerThread( LPVOID lpParam )
{
	GlobalStore.context = interception_create_context();

	InterceptionDevice		LastDevice;
	InterceptionStroke		LastStroke;
	int		PrevKeyEventTick = GetTickCount();
	int		PrevMouseEventTick = GetTickCount();

	bool	HasKeyboardHandlers = false;
	bool	HasMouseHandlers = false;

	std::list<IrcGameKeyStore*>::iterator itr;
	for( itr=GlobalStore.MonitoredKeys.begin(); itr!=GlobalStore.MonitoredKeys.end(); itr++ )
	{
		if( (*itr)->StrokeCode != 0 )
			HasKeyboardHandlers = true;
		if( (*itr)->MouseX != 0 || (*itr)->MouseY != 0 || (*itr)->MouseKey != 0 )
			HasMouseHandlers = true;
	}

	if( GlobalStore.PrintKeysPressed == true )
	{
		HasKeyboardHandlers = true;
		HasMouseHandlers = true;
	}

	if( HasKeyboardHandlers )
		interception_set_filter( GlobalStore.context, interception_is_keyboard, INTERCEPTION_FILTER_KEY_DOWN | INTERCEPTION_FILTER_KEY_UP);
	if( HasMouseHandlers )
		interception_set_filter( GlobalStore.context, interception_is_mouse, INTERCEPTION_FILTER_MOUSE_MOVE | INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN | INTERCEPTION_MOUSE_LEFT_BUTTON_UP | INTERCEPTION_MOUSE_RIGHT_BUTTON_DOWN | INTERCEPTION_MOUSE_RIGHT_BUTTON_UP );
	while( interception_receive( GlobalStore.context, LastDevice = interception_wait( GlobalStore.context ), (InterceptionStroke *)&LastStroke, 1) > 0
		&& GlobalStore.WorkerThreadAlive == 1
//		&& 0
		)
	{
		if( interception_is_mouse( LastDevice ) )
		{
			GlobalStore.MouseDevice = LastDevice;
            InterceptionMouseStroke &mstroke = *(InterceptionMouseStroke *) &LastStroke;
			
			memcpy( &GlobalStore.MouseStroke, &LastStroke, sizeof( InterceptionMouseStroke ) );

			//try to track mouse coordinate
            if( (mstroke.flags & INTERCEPTION_MOUSE_MOVE_ABSOLUTE) == 0 )
			{
				//this can go off screen
				GlobalStore.TrackedMouseX += mstroke.x;
				if( GlobalStore.TrackedMouseX > GlobalStore.DosBoxWidth )
					GlobalStore.TrackedMouseX = GlobalStore.DosBoxWidth;
				if( GlobalStore.TrackedMouseX < 0 )
					GlobalStore.TrackedMouseX = 0;
				GlobalStore.TrackedMouseY += mstroke.y;
				if( GlobalStore.TrackedMouseY > GlobalStore.DosBoxHeight )
					GlobalStore.TrackedMouseY = GlobalStore.DosBoxHeight;
				if( GlobalStore.TrackedMouseY < 0 )
					GlobalStore.TrackedMouseY = 0;
			}

            interception_send( GlobalStore.context, GlobalStore.MouseDevice, (InterceptionStroke *)&mstroke, 1);

			if( GlobalStore.PrintKeysPressed )
			{
				printf( "Mouse %d: state %d, x %d y %d, flags %d, rolling %d, info %d, tracked %d %d\n", GetTickCount() - PrevKeyEventTick, mstroke.state, mstroke.x, mstroke.y, mstroke.flags, mstroke.rolling, mstroke.information, GlobalStore.TrackedMouseX, GlobalStore.TrackedMouseY );
				PrevKeyEventTick = GetTickCount();
			}
		}
		if( interception_is_keyboard( LastDevice ) )
		{
			GlobalStore.KeyboardDevice = LastDevice;
            InterceptionKeyStroke &kstroke = *(InterceptionKeyStroke *) &LastStroke;

			memcpy( &GlobalStore.KeyboardStroke, &LastStroke, sizeof( InterceptionKeyStroke ) );

			interception_send( GlobalStore.context, GlobalStore.KeyboardDevice, (InterceptionStroke *)&LastStroke, 1);

			if( GlobalStore.PrintKeysPressed )
			{
				printf( "Keyboard %d: scan code %d %d %d\n", GetTickCount() - PrevMouseEventTick, kstroke.code, kstroke.state, kstroke.information );
				PrevMouseEventTick = GetTickCount();
			}

			if( kstroke.code == SCANCODE_CONSOLE )
			{
				GlobalStore.WorkerThreadAlive = 0;
				printf( "Esc pressed. Shutting down\n" );
				break;
			}
		}
	}/**/

	//this happens in case laptop mouse goes idle. It might come back later = Never give up hope !
	if( GlobalStore.WorkerThreadAlive == 1 )
	{
		printf("Worker thread keymonitor device failure. Trying to work with remaining devices...\n");
		while( GlobalStore.WorkerThreadAlive == 1 )
			Sleep( 1000 );
	}
	interception_destroy_context( GlobalStore.context );
	printf("Worker thread keymonitor exited\n");

	return 0;
}

int CanClickWithMouse( int ClickKey )
{
//	printf("Testick canclick %d\n", ClickKey );
	//this is not a click. We can move mouse anywhere
	if( ClickKey == 0 )
		return 1;
	//are we inside a box that denies clicks ?
	POINT p;
	if( GetCursorPos( &p ) )
	{
		std::list<NoMouseClickBox*>::iterator itr;
		for( itr=GlobalStore.DenyMouseActionBoxes.begin(); itr!=GlobalStore.DenyMouseActionBoxes.end(); itr++ )
		{
//			printf("Testing Mouse click %d %d %d -> %d %d %d %d %d\n", ClickKey, p.x, p.y, (*itr)->MouseKey, (*itr)->XStart, (*itr)->YStart, (*itr)->XEnd, (*itr)->YEnd );
			if( (*itr)->XStart <= p.x && p.x <= (*itr)->XEnd
				&& (*itr)->YStart <= p.y && p.y <= (*itr)->YEnd
				&& ( (*itr)->MouseKey & ClickKey ) )
			{
//				printf("Deny clicking with mouse\n");
				return 0;
			}
		}
	}
//	printf("Allow clicking with mouse\n");
	return 1;
}