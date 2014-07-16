#include "StdAfx.h"

// Get the horizontal and vertical screen sizes in pixel
void GetDesktopResolution(int& horizontal, int& vertical)
{
   RECT desktop;
   // Get a handle to the desktop window
   const HWND hDesktop = GetDesktopWindow();
   // Get the size of screen to the variable desktop
   GetWindowRect(hDesktop, &desktop);
   // The top left corner will have coordinates (0,0)
   // and the bottom right corner will have coordinates
   // (horizontal, vertical)
   horizontal = desktop.right;
   vertical = desktop.bottom;
}

void BarbaricTCharToChar( TCHAR *In, char *Out, int MaxLen )
{
	for( int i=0; i<MaxLen && *In != 0; i++ )
	{
		*Out = (char)*In;
		Out++;
		In++;
	}
	*Out = '\0';
}

HWND IsWindowOurFocusWindow( HWND ActiveWindow, int Loop )
{
	if( Loop == 0 && ActiveWindow == 0 )
		ActiveWindow = GetForegroundWindow();
	if( Loop > 10 )
		return 0; 
	if( ActiveWindow )
	{
		char ActiveWindowTitle[DEFAULT_BUFLEN];
		GetWindowText( ActiveWindow, ActiveWindowTitle, DEFAULT_BUFLEN );
		if( strstr( ActiveWindowTitle, GlobalStore.ActiveWindowName ) == NULL )
		{
//for( int i=0;i<Loop;i++)printf("\t");
//printf("Current window title is %s\n", ActiveWindowTitle);
			return IsWindowOurFocusWindow( GetParent( ActiveWindow ), Loop + 1 );
		}
		else 
			return ActiveWindow; //we found it !
	}
	return 0;
}

BOOL CALLBACK EnumWindowsProcSetActive( HWND hwnd, LPARAM lParam )
{
//	char ClassName[ DEFAULT_BUFLEN ];
	char WindowTitle[ DEFAULT_BUFLEN ];
//	GetClassName( hwnd, ClassName, sizeof( ClassName ) );
	GetWindowText( hwnd, WindowTitle, sizeof( WindowTitle ) );

//	printf( "Class name %s\n", ClassName );
	if( GlobalStore.PrintKeysPressed == true && strlen( WindowTitle ) > 1 )
		printf( "window name %s\n", WindowTitle );

	if( strstr( WindowTitle, GlobalStore.ActiveWindowName ) != NULL )
	{
		SetActiveWindow( hwnd );
		SetForegroundWindow( hwnd );
	}
	return TRUE;
}

void GetMouseNormalizedCordsForPixel( int &x, int &y )
{
	int Width, Height;
	GetDesktopResolution( Width, Height );

	if( x < 0 || y < 0 )
	{
		POINT p;
		if( GetCursorPos( &p ) )
		{
			if( x < 0 )
				x = p.x;
			if( y < 0 )
				y = p.y;
		}
	}
	x = 65535 * x / Width ;
	y = 65535 * y / Height ;
}

void MoveMouseAsDemo()
{
	int MouseDemoCounter = 0;
	while( GlobalStore.WorkerThreadAlive == 1 )
	{
		int Phase = (MouseDemoCounter / 50) % 4;
		MouseDemoCounter++;
		//up
		if( Phase == 0 )
			SendMouseChange( 0, 0, 0, -1, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
		//right
		else if( Phase == 1 )
			SendMouseChange( 0, 0, 1, 0, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
		//down
		else if( Phase == 2 )
			SendMouseChange( 0, 0, 0, 1, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
		//left
		else if( Phase == 3 )
			SendMouseChange( 0, 0, -1, 0, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
		//click test
		if( MouseDemoCounter % 50 == 0 )
			SendMouseChange( INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN, 0, 0, 0, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
		Sleep( 100 );
	}
}