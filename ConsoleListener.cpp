#include "StdAfx.h"

void HandleConsoleLine( char *Line )
{
	if( strncmp( Line, "shutdown\0", strlen( "shutdown\0" ) ) == 0 ) 
	{
		GlobalStore.WorkerThreadAlive = 0;
		printf("Console Listener : Shutdown initiated\n");
	}
	else if( strncmp( Line, "start\0", strlen( "start\0" ) ) == 0 ) 
	{
		GlobalStore.Sync1Stamp = GetTickCount();
		GlobalStore.Sync2Stamp = GlobalStore.Sync1Stamp + 1;
		printf("Console Listener : force democracy thread start\n");
	}
	else if( strncmp( Line, "resetmouse\0", strlen( "resetmouse\0" ) ) == 0 ) 
	{
		GlobalStore.MouseX = 0;
		GlobalStore.MouseY = 0;
		printf("Console Listener : mouse cords reseted to 0 0\n");
		SendMouseChange( 0, INTERCEPTION_MOUSE_MOVE_ABSOLUTE, 0, 0, SLEEP_BETWEEN_KEYPRESS, SLEEP_BETWEEN_KEYPRESS );
	}
	else if( strncmp( Line, "cls\0", strlen( "cls\0" ) ) == 0 ) 
	{
		printf("Console Listener : adding empty lines to clear screen\n");
		for( int i = 0; i<EMPTY_LINES_FOR_CLS; i++ )
			printf("\n");
	}
	else if( strncmp( Line, "queuemsg ", strlen( "queuemsg " ) ) == 0 ) 
	{
		printf("Console Listener : queue bot message\n");
		QueueBotMessage( Line + strlen( "queuemsg " ) );
	}
	else if( strncmp( Line, "pause\0", strlen( "pause\0" ) ) == 0 ) 
	{
		printf("Console Listener : pausing actiontaker thread\n");
		GlobalStore.Sync1Stamp = GlobalStore.Sync2Stamp = 0;
	}
	else if( strncmp( Line, "autofocus\0", strlen( "autofocus\0" ) ) == 0 )
	{
		printf("Console Listener : autofocus is toggled\n" );
		GlobalStore.RefocusWindow = 1 - GlobalStore.RefocusWindow;
	}
	else if( strncmp( Line, "voteduration ", strlen( "voteduration " ) ) == 0 ) 
	{
		int NewDuration = atoi( Line + strlen( "voteduration " ) );
		printf("Console Listener : vote duration %d -> %d\n", GlobalStore.DemoctraticVoteWait, NewDuration );
		GlobalStore.DemoctraticVoteWait = NewDuration;
	}
}

void LoopListenConsole()
{
	char LineBuffer[ DEFAULT_BUFLEN ];
	int WriteIndex = 0;
	while( GlobalStore.WorkerThreadAlive == 1 )
	{
		//this will not let us auto exit program until keypressed
		char c = _getch();
		if( c == '\r' || c == '\n' )
		{
			LineBuffer[ WriteIndex ] = '\0';
			if( WriteIndex > 1 )
			{
				printf("\n");
				HandleConsoleLine( LineBuffer );
			}
			WriteIndex = 0;
		}
		else if( c == '`' )
			GlobalStore.WorkerThreadAlive = 0;
		else if( WriteIndex < DEFAULT_BUFLEN - 1 )
		{
			LineBuffer[ WriteIndex ] = c;
			WriteIndex++;
			printf("%c",c);
		}
	}
}