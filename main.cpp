#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0500

#include "StdAfx.h"

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "interception.lib")

#define MAX_THREADS				4

GlobalStateStore GlobalStore;

DWORD WINAPI DemocracyKeypressThread( LPVOID lpParam )
{
	do{
		if( GlobalStore.Sync2Stamp != 0 )
		{
			//check if we are restricted taking actions only in 1 window
			if( GlobalStore.ActiveWindowName )
			{
				HWND ActiveWindow = IsWindowOurFocusWindow( 0 , 0 );
				if( ActiveWindow == 0 )
				{
					HWND ActiveWindow = GetForegroundWindow();
					char ActiveWindowTitle[DEFAULT_BUFLEN];
					GetWindowText( ActiveWindow, ActiveWindowTitle, DEFAULT_BUFLEN );
					if( GlobalStore.RefocusWindow != 0 )
					{
						printf("Game window is not active. Trying to focus on it. We need / have : \n\t%s \n\t%s\n", GlobalStore.ActiveWindowName, ActiveWindowTitle );
						EnumWindows( EnumWindowsProcSetActive, NULL );
					}
					else
					{
						printf("Game window is not active. Skipping actions. We need / have : \n\t%s \n\t%s\n", GlobalStore.ActiveWindowName, ActiveWindowTitle );
					}
					Sleep( GlobalStore.DemoctraticVoteWait );
					continue;
				}
			}
			//check if mouse is withing boundary
			if( GlobalStore.MouseXLimitMin != 0 || GlobalStore.MouseYLimitMin != 0 || GlobalStore.MouseXLimitMax != 0 || GlobalStore.MouseYLimitMax != 0)
			{
				POINT p;
//				printf("Mouse is limited to %d %d\n", GlobalStore.MouseXLimit, GlobalStore.MouseYLimit );
				if( GetCursorPos( &p ) )
				{
//					printf("Mouse position now %d %d\n", p.x, p.y );
					if( p.x > GlobalStore.MouseXLimitMax || p.y > GlobalStore.MouseYLimitMax || p.x < GlobalStore.MouseXLimitMin || p.y < GlobalStore.MouseYLimitMin )
					{
						int InPixelX;
						if( p.x > GlobalStore.MouseXLimitMax )
							InPixelX = GlobalStore.MouseXLimitMax;
						else if( p.x < GlobalStore.MouseXLimitMin )
							InPixelX = GlobalStore.MouseXLimitMin;
						else
							InPixelX = p.x;
						int InPixelY = GlobalStore.MouseYLimitMin + ( GlobalStore.MouseYLimitMax - GlobalStore.MouseYLimitMin ) / 2;
						if( p.y > GlobalStore.MouseYLimitMax )
							InPixelY = GlobalStore.MouseYLimitMax;
						else if( p.y < GlobalStore.MouseYLimitMin )
							InPixelY = GlobalStore.MouseYLimitMin;
						else
							InPixelY = p.y;
						int NormalizedMouseX = InPixelX;
						int NormalizedMouseY = InPixelY;
						GetMouseNormalizedCordsForPixel( NormalizedMouseX, NormalizedMouseY );
//						printf("Mouse is out of limits %d %d. Readjusting to %d %d - %d %d \n", p.x, p.y, InPixelX, InPixelY, NormalizedMouseX, NormalizedMouseY );
						SendMouseChange( 0, INTERCEPTION_MOUSE_MOVE_ABSOLUTE, NormalizedMouseX, NormalizedMouseY, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
					}
				}
			}

			int PrevPlayerGroupUpdated = -1;
			int PlayerGroupNotYetUpdated;
			bool ActionStillCanBeTaken = true;
			std::list<IrcGameKeyStore*>::iterator itr;

			do{
				//check if we still have a player group that can be updated with keypresses
				PlayerGroupNotYetUpdated = PrevPlayerGroupUpdated;
				for( itr=GlobalStore.MonitoredKeys.begin(); itr!=GlobalStore.MonitoredKeys.end(); itr++ )
					if( (*itr)->PlayerGroup > PlayerGroupNotYetUpdated 
						&& (*itr)->PushesSinceLastUpdate != 0
						)
					{
						PlayerGroupNotYetUpdated = (*itr)->PlayerGroup;
						break;
					}

				//we found a player group
				if( PlayerGroupNotYetUpdated != PrevPlayerGroupUpdated )
				{
					ActionStillCanBeTaken = true;

					//get most voted action
					int BestActionCount = 0;
					IrcGameKeyStore *BestAction = NULL;
					for( itr=GlobalStore.MonitoredKeys.begin(); itr!=GlobalStore.MonitoredKeys.end(); itr++ )
						if( (*itr)->PlayerGroup == PlayerGroupNotYetUpdated 
							&& (*itr)->PushesSinceLastUpdate > BestActionCount
							)
						{
							BestActionCount = (*itr)->PushesSinceLastUpdate;
							BestAction = (*itr);
						}

					//perform the action
					if( BestAction != NULL )
					{
						if( BestAction->StrokeCode > 0 )
						{
							SendKeyPress3( BestAction->StrokeCode, BestAction->StrokePushdownDelay, BestAction->StrokeReleaseDelay );
							printf("Taking voted keyboard action : %s\n", BestAction->IrcText );
						}
						if( BestAction->MouseKey != 0 || BestAction->MouseX != 0 || BestAction->MouseY != 0 )
						{
							if( BestAction->MouseMoveType == MOUSE_MOVE_TYPE_ABSOLUTE_NORMALIZED )
							{
								//remember position
								int NormalizedMouseX = -1;
								int NormalizedMouseY = -1;
								GetMouseNormalizedCordsForPixel( NormalizedMouseX, NormalizedMouseY );
								//move to absolute normalized cord location
								SendMouseChange( 0, INTERCEPTION_MOUSE_MOVE_ABSOLUTE, BestAction->MouseX, BestAction->MouseY, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
								//do a left mouse click
								SendMouseChange( INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN, 0, 0, 0, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
								//move back to old location
								SendMouseChange( 0, INTERCEPTION_MOUSE_MOVE_ABSOLUTE, NormalizedMouseX, NormalizedMouseY, BestAction->StrokePushdownDelay, BestAction->StrokeReleaseDelay );
							}
							//
							else if( BestAction->MouseFlags == MOUSE_MOVE_TYPE_ABSOLUTE_PIXEL )
							{
								//remember position
								int NormalizedMouseX = -1;
								int NormalizedMouseY = -1;
								GetMouseNormalizedCordsForPixel( NormalizedMouseX, NormalizedMouseY );
								//bring mouse to 0 0
								SendMouseChange( 0, 0, -10000, -10000, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
								//now bring it to specific pixel coords
								SendMouseChange( 0, 0, BestAction->MouseX, BestAction->MouseY, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
								//do a left mouse click
								SendMouseChange( INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN, 0, 0, 0, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
								//move back to old location
								SendMouseChange( 0, INTERCEPTION_MOUSE_MOVE_ABSOLUTE, NormalizedMouseX, NormalizedMouseY, BestAction->StrokePushdownDelay, BestAction->StrokeReleaseDelay );
							}
							//dosbox only handles relative mouse coordinate system !
							else if( BestAction->MouseFlags == MOUSE_MOVE_TYPE_ABSOLUTE_PIXEL_DOSBOX )
							{
								//bring mouse to 0 0
								SendMouseChange( 0, 0, -10000, -10000, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
								//now bring it to specific coords
								SendMouseChange( 0, 0, BestAction->MouseX, BestAction->MouseY, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
								//do a left mouse click
								SendMouseChange( INTERCEPTION_MOUSE_LEFT_BUTTON_DOWN, 0, 0, 0, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
							}
							else if( CanClickWithMouse( BestAction->MouseKey ) )
								SendMouseChange( BestAction->MouseKey, BestAction->MouseFlags, BestAction->MouseX, BestAction->MouseY, BestAction->StrokePushdownDelay, BestAction->StrokeReleaseDelay );
							printf("Taking voted mouse action : %s\n", BestAction->IrcText );
//							SendChatToChannel( "Taking voted mouse action : ",  BestAction->IrcText );
							QueueBotMessage( "Taking voted mouse action : ",  BestAction->IrcText );
						}
					}
				}
				else
					ActionStillCanBeTaken = false;
				PrevPlayerGroupUpdated = PlayerGroupNotYetUpdated;

			}while( PlayerGroupNotYetUpdated != PrevPlayerGroupUpdated 
				&& GlobalStore.WorkerThreadAlive == 1
				);

			//reset all actions
			for( itr=GlobalStore.MonitoredKeys.begin(); itr!=GlobalStore.MonitoredKeys.end(); itr++ )
				(*itr)->PushesSinceLastUpdate = 0;

			//sleep to give time to the next democratic vote
			Sleep( GlobalStore.DemoctraticVoteWait );
		}
		else
			Sleep( GlobalStore.DemoctraticVoteWait );
	}while( GlobalStore.WorkerThreadAlive == 1 );

	printf("Worker thread democracy voter exited\n");\

	return 0;
}

int main( int argc, char **argv ) 
{   
	//let's not trust random
	GlobalStore.Nick = GlobalStore.Channel = GlobalStore.Host = GlobalStore.Port = GlobalStore.oAuth = GlobalStore.ActiveWindowName = NULL;
	GlobalStore.NextIrcMsgStamp = GlobalStore.WorkerThreadAlive = GlobalStore.Sync1Stamp = GlobalStore.Sync2Stamp = GlobalStore.DemoctraticVoteWait = 0;
	GlobalStore.MouseX = GlobalStore.MouseY = GlobalStore.RefocusWindow = GlobalStore.MouseXLimitMin = GlobalStore.MouseYLimitMin = GlobalStore.MouseXLimitMax = GlobalStore.MouseYLimitMax = 0;
	GlobalStore.TrackedMouseX = GlobalStore.TrackedMouseY = GlobalStore.DosBoxWidth = GlobalStore.DosBoxHeight = 0;
	GlobalStore.PrintKeysPressed = GlobalStore.IrcConnected = false;
	GlobalStore.ConnectSocket = INVALID_SOCKET;
	GlobalStore.fMyPMs = NULL;

	printf("Loading config file...\n");
	LoadSettingsFromFile( );

	if( argc > 1 )
	{
		if( atoi( argv[1] ) == 1 )
		{
			printf("scanning for keyboard and mouse codes until '~' is pressed\n");
			GlobalStore.WorkerThreadAlive = 1;
			GlobalStore.PrintKeysPressed = true;
			KeyScannerThread( NULL );
		}
		else if( atoi( argv[1] ) == 2 )
		{
			//check if the game window is active
			printf("Printing currently active window titles for copy paste purpuse\n");
			GlobalStore.PrintKeysPressed = true;
			EnumWindows( EnumWindowsProcSetActive, NULL );
		}
		else if( atoi( argv[1] ) == 3 )
		{
			//check if the game window is active
			printf("Printing mouse cords until force close:\n");
			while( 1 )
			{
				POINT p;
				if( GetCursorPos( &p ) )
				{
					int Width, Height;
					GetDesktopResolution( Width, Height );
					int NormalizedMouseX = 65535 * p.x / Width;
					int NormalizedMouseY = 65535 * p.y / Height;
					printf("Mouse position now : %d %d : Normalized %d %d : Tracked %d %d\n", p.x, p.y, NormalizedMouseX, NormalizedMouseY, GlobalStore.TrackedMouseX, GlobalStore.TrackedMouseY );
				}
				Sleep( 500 );
			}
		}
		return 0;
	}

    WSADATA wsaData;
    int iResult;
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != NO_ERROR)
	{
      printf("WSAStartup failed: %d\n", iResult);
      return 1;
    }

	//connect to the irc server
	printf("Connecting to irc server...\n");
	if( ReConnectToIrc( true ) != 0 )
		return 1;

	//init PMLogger
	printf("Initializeing PM logger\n");
	PMLoggerInit();

	//init threads to run the logic
	printf("Starting worker threads...\n");
	DWORD   dwThreadIdArray[MAX_THREADS];
	HANDLE  hThreadArray[MAX_THREADS]; 
	GlobalStore.WorkerThreadAlive = 1;
	{
		printf("\t Starting democracy voter thread...\n");
		hThreadArray[0] = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			DemocracyKeypressThread,			// thread function name
			0,						// argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArray[0]);   // returns the thread identifier 
		if (hThreadArray[0] == NULL) 
		   ExitProcess(3);

		printf("\t Starting keyboard driver thread...\n");
		hThreadArray[1] = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			KeyScannerThread,			// thread function name
			0,						// argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArray[1]);   // returns the thread identifier 
		if (hThreadArray[1] == NULL) 
		   ExitProcess(3);

		printf("\t Starting IRC client read thread...\n");
		hThreadArray[2] = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			SocketReaderThread,		// thread function name
			0,						// argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArray[2]);   // returns the thread identifier 
		if (hThreadArray[2] == NULL) 
		   ExitProcess(3);

		printf("\t Starting IRC client write thread...\n");
		hThreadArray[3] = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			SocketWriterThread,		// thread function name
			0,						// argument to thread function 
			0,                      // use default creation flags 
			&dwThreadIdArray[3]);   // returns the thread identifier 
		if (hThreadArray[3] == NULL) 
		   ExitProcess(3);
	}

	//don't really need this, we could use the waitmultiplethreads, but then irc client might block
	printf("\tStarting console listener thread...\n");
	LoopListenConsole();
//	MoveMouseAsDemo();

	printf("Main Thread : Started shutdown \n");

    // shutdown the connection since no more data will be sent
	{
		printf("Started Irc shutdown\n" );
		ShutdownIrcConnection();
	}

	WSACleanup();

	{
		printf("Waiting for worker threads to exit\n");
		GlobalStore.WorkerThreadAlive = 0;
		WaitForMultipleObjects(MAX_THREADS, hThreadArray, TRUE, INFINITE);

		// Close all thread handles and free memory allocations.
		for(int i=0; i<MAX_THREADS; i++)
			CloseHandle(hThreadArray[i]);
	}

	printf("Shutting down PM logger\n");
	PMLoggerShutdown();

    return 0;   
}