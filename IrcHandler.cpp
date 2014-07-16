#include "StdAfx.h"

void	ShutdownIrcConnection()
{
	shutdown( GlobalStore.ConnectSocket, SD_BOTH );
	closesocket( GlobalStore.ConnectSocket );
	GlobalStore.ConnectSocket = INVALID_SOCKET;
}

int ReConnectToIrc( bool FirstConnect )
{
	if( FirstConnect == false )
		Sleep( IRC_RECONNECT_WAIT );

	GlobalStore.IrcConnected = false;
	int iResult;
    struct addrinfo hints, *res;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo( GlobalStore.Host, GlobalStore.Port, &hints, &res );

	GlobalStore.ConnectSocket = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
	if( GlobalStore.ConnectSocket == INVALID_SOCKET ) 
	{
		printf("Error at socket(): %ld\n", WSAGetLastError() );
		return 1;
	}
    
	iResult = connect( GlobalStore.ConnectSocket, res->ai_addr, res->ai_addrlen );
	if ( iResult == SOCKET_ERROR) 
	{
		closesocket( GlobalStore.ConnectSocket );
		printf("Unable to connect to server: %ld\n", WSAGetLastError() );
		GlobalStore.ConnectSocket = INVALID_SOCKET;
		return 1;
	}

	//send user / auth / connect to a channel
	{
		raw( GlobalStore.ConnectSocket, "PASS %s\r\n", GlobalStore.oAuth );
		raw( GlobalStore.ConnectSocket, "NICK %s\r\n", GlobalStore.Nick );
		raw( GlobalStore.ConnectSocket, "JOIN :%s\r\n", GlobalStore.Channel );
//		raw( ConnectSocket, "USER %s 0 0 :%s\r\n", nick, nick );
	}
	GlobalStore.IrcConnected = true;

	return 0;
}

void raw( SOCKET conn, char *fmt, ...)
{
	char sbuf[DEFAULT_BUFLEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf_s( sbuf, DEFAULT_BUFLEN, DEFAULT_BUFLEN, fmt, ap);
    va_end(ap);
    printf("<< %s\n", sbuf);
    send( conn, sbuf, strlen(sbuf), 0);
}

void ExplodeIrcReadBuffer( char *ReadBuff, int MaxInputLen, IrcServerMessage &out )
{
	memset( &out, 0, sizeof( out ) );
	if( ReadBuff == NULL )
		return;
	//search for terminator
	char *ReadBarrier = ReadBuff;
	while( ReadBarrier < ReadBuff + MaxInputLen 
		&& *ReadBarrier != '\n'
		&& *ReadBarrier != '\0' 
		)
		ReadBarrier++;
	if( *ReadBarrier == '\r' )
		ReadBarrier--;

	//first char is generally ':'
	// :llamaru!llamaru@llamaru.tmi.twitch.tv PRIVMSG #twitchplayspokemon :democracy 
	if( *ReadBuff == ':' )
	{
		ReadBuff++;

		//try to read from who the command is
		if( ReadBuff < ReadBarrier )
		{
			out.User = ReadBuff;
			while( ReadBuff < ReadBarrier && *ReadBuff != '!' && *ReadBuff != ' ' && *ReadBuff != '\r' && *ReadBuff != '\n' && *ReadBuff != '\0' )
				ReadBuff++;
			if( *ReadBuff == '!' )
			{
				*ReadBuff = '\0';
				ReadBuff++;
				if( ReadBuff < ReadBarrier )
				{
					out.UserFull = ReadBuff;
					while( ReadBuff < ReadBarrier && *ReadBuff != ' ' && *ReadBuff != '\r' && *ReadBuff != '\n' && *ReadBuff != '\0' )
						ReadBuff++;
					if( *ReadBuff != ' ' || *ReadBuff != '\r' || *ReadBuff != '\n' || *ReadBuff != '\0' )
						*ReadBuff = '\0';
					ReadBuff++;
				}
			}
			else
			{
				*ReadBuff = '\0';
				ReadBuff++;
			}
		}

		if( ReadBuff < ReadBarrier )
		{
			out.ServerCommand = ReadBuff;
			while( ReadBuff < ReadBarrier && *ReadBuff != ' ' && *ReadBuff != '\r' && *ReadBuff != '\n' && *ReadBuff != '\0' )
				ReadBuff++;
			if( *ReadBuff != ' ' || *ReadBuff != '\r' || *ReadBuff != '\n' || *ReadBuff != '\0' )
				*ReadBuff = '\0';
			ReadBuff++;
		}

		//check if it is a command
		if( ReadBuff < ReadBarrier )
		{
			out.Channel = ReadBuff;
			while( ReadBuff < ReadBarrier && *ReadBuff != ' ' && *ReadBuff != '\r' && *ReadBuff != '\n' && *ReadBuff != '\0' )
				ReadBuff++;
			if( *ReadBuff != ' ' || *ReadBuff != '\r' || *ReadBuff != '\n' || *ReadBuff != '\0' )
				*ReadBuff = '\0';
			ReadBuff++;
		}
	}
	//second part
	if( *ReadBuff == ':' )
	{
		ReadBuff++;
		if( ReadBuff < ReadBarrier )
		{
			if( *ReadBuff == '!' )
			{
				out.UserCommand = ReadBuff;
				while( ReadBuff < ReadBarrier && *ReadBuff != ' ' && *ReadBuff != '\r' && *ReadBuff != '\n' && *ReadBuff != '\0' )
					ReadBuff++;
				if( *ReadBuff != ' ' || *ReadBuff != '\r' || *ReadBuff != '\n' || *ReadBuff != '\0' )
					*ReadBuff = '\0';
				ReadBuff++;
			}
		}
		if( ReadBuff < ReadBarrier )
		{
			out.UserCommandParamOrMsg = ReadBuff;
			while( ReadBuff < ReadBarrier && *ReadBuff != '\r' && *ReadBuff != '\n' && *ReadBuff != '\0' )
				ReadBuff++;
			if( *ReadBuff != '\r' || *ReadBuff != '\n' || *ReadBuff != '\0' )
				*ReadBuff = '\0';
			ReadBuff++;
		}
	}
	if( *ReadBuff == '\r' )
		ReadBuff++;
	if( *ReadBuff == '\n' )
		ReadBuff++;
	if( *ReadBuff == '0' )
		ReadBuff++;
	if( ReadBuff < ReadBarrier )
		out.NextLine = ReadBuff;
}

DWORD WINAPI SocketReaderThread( LPVOID lpParam )
{
	char SocketReadBuffer[DEFAULT_BUFLEN + 1];
	SocketReadBuffer[ DEFAULT_BUFLEN - 1 ] = 0;
	int StartStamp = GetTickCount();
	int ReceivedByteCount;
	while( GlobalStore.WorkerThreadAlive == 1 )
	{
		while( ( ReceivedByteCount = recv( GlobalStore.ConnectSocket, SocketReadBuffer, DEFAULT_BUFLEN, 0 ) ) //this can stall quite a lot if there is no data to be read !
			&& GlobalStore.WorkerThreadAlive == 1
			) 
		{
			SocketReadBuffer[ ReceivedByteCount ] = '\0';

	//		if( GetTickCount() - StartStamp > 20000 )
	//			break;

	//		printf("Received from server %d : %s\n\n", ReceivedByteCount, SocketReadBuffer );

			if( !strncmp(SocketReadBuffer, "PING", 4) )
			{
				raw( GlobalStore.ConnectSocket, "PONG" );
				continue;
			}

			IrcServerMessage msg;
			ExplodeIrcReadBuffer( SocketReadBuffer, DEFAULT_BUFLEN, msg );
			
	/*		if( msg.ServerCommand != NULL && ( strncmp(msg.ServerCommand, "PRIVMSG", 7) == 0 || strncmp(msg.ServerCommand, "NOTICE", 6) == 0 ) )
			{
				if( msg.User )
					printf("User : %s\n", msg.User );
				if( msg.UserFull )
					printf("User Full : %s\n", msg.UserFull );
				if( msg.ServerCommand )
					printf("ServerCommand : %s\n", msg.ServerCommand );
				if( msg.Channel )
					printf("ServerChannel : %s\n", msg.Channel );
				if( msg.UserCommand )
					printf("User command : %s\n", msg.UserCommand );
				if( msg.UserCommandParamOrMsg )
					printf("User message : %s\n", msg.UserCommandParamOrMsg );
				if( msg.NextLine )
					printf("Leftover : %s\n", msg.NextLine );
				printf( "\n" );
			}/**/

			if( msg.ServerCommand != NULL && ( strncmp(msg.ServerCommand, "PRIVMSG", 7) == 0 || strncmp(msg.ServerCommand, "NOTICE", 6) == 0 ) )
			{
				//Log messages sent to us
				PMLoggerEventChatMessageReceived( msg.User, msg.UserCommandParamOrMsg );

	//			if( msg.User )	printf("User : %s : %s\n", msg.User, msg.UserCommandParamOrMsg );
				if( strncmp( msg.User, GlobalStore.Nick, strlen( GlobalStore.Nick ) + 1 ) == 0 )
				{
	//				printf( "!Owner said something : '%s'\n", msg.UserCommandParamOrMsg );
					if( strncmp( msg.UserCommandParamOrMsg, "sync1\0", strlen( "sync1\0") ) == 0 )
					{
						GlobalStore.Sync1Stamp = GetTickCount();
						printf("IrcClient : Received Sync 1\n");
					}
					else if( strncmp( msg.UserCommandParamOrMsg, "sync2\0", strlen( "sync2\0") ) == 0 )
					{
						GlobalStore.Sync2Stamp = GetTickCount();
						printf("IrcClient : Received Sync 2. Desync duration %d\n", GlobalStore.Sync2Stamp - GlobalStore.Sync1Stamp );
					}
					else if( strncmp( msg.UserCommandParamOrMsg, "shutdown\0", strlen( "shutdown\0" ) ) == 0 )
					{
						printf("IrcClient : Shutdown received\n" );
						GlobalStore.WorkerThreadAlive = 0;
						break;
					}
					else if( strncmp( msg.UserCommandParamOrMsg, "clr\0", strlen( "clr\0") ) == 0 )
					{
						printf("IrcClient : adding empty lines to console\n" );
						for( int i = 0; i < EMPTY_LINES_FOR_CLS; i++ )
							printf("\n");
					}
					else if( strncmp( msg.UserCommandParamOrMsg, "pause\0", strlen( "pause\0" ) ) == 0 )
					{
						printf("IrcClient : pause actiontaker thread\n" );
						GlobalStore.Sync1Stamp = GlobalStore.Sync2Stamp = 0;
					}
					else if( strncmp( msg.UserCommandParamOrMsg, "autofocus\0", strlen( "autofocus\0" ) ) == 0 )
					{
						printf("IrcClient : autofocus is toggled\n" );
						GlobalStore.RefocusWindow = 1 - GlobalStore.RefocusWindow;
					}
					else if( strncmp( msg.UserCommandParamOrMsg, "voteduration ", strlen( "voteduration " ) ) == 0 ) 
					{
						int NewDuration = atoi( msg.UserCommandParamOrMsg + strlen( "voteduration " ) );
						printf("IrcClient : vote duration %d -> %d\n", GlobalStore.DemoctraticVoteWait, NewDuration );
						GlobalStore.DemoctraticVoteWait = NewDuration;
					}
				}

				std::list<IrcGameKeyStore*>::iterator itr;
				for( itr=GlobalStore.MonitoredKeys.begin(); itr!=GlobalStore.MonitoredKeys.end(); itr++ )
					if( strncmp( msg.UserCommandParamOrMsg, (*itr)->IrcText, (*itr)->IrcTextLen ) == 0 )
					{
						(*itr)->PushesSinceLastUpdate++;
						printf("User %s voted %s. Now at %d\n", msg.User, msg.UserCommandParamOrMsg, (*itr)->PushesSinceLastUpdate );
					}
			}
			
			if( msg.ServerCommand != NULL && ( strncmp(msg.ServerCommand, "JOIN", 4) == 0 ) )
			{
				//greet the newcommer
			}
			
			if( msg.ServerCommand != NULL && ( strncmp(msg.ServerCommand, "PART", 4) == 0 ) )
			{
				//jerk left
			}
		}
		if( GlobalStore.WorkerThreadAlive == 1 )
		{
			printf("Lost connection to IRC server. Trying to reconnect\n");
			ShutdownIrcConnection();
			ReConnectToIrc( false );
		}
	}
	printf("Worker thread irc read handler exited\n");

	return 0;
}

void SendChatToChannel( char *msg )
{
	char SocketWriteBuffer[ DEFAULT_BUFLEN ];
	sprintf_s( SocketWriteBuffer, DEFAULT_BUFLEN, "PRIVMSG %s : %s\n", GlobalStore.Channel, msg );
	raw( GlobalStore.ConnectSocket, SocketWriteBuffer );
}

void SendChatToChannel( char *msg1, char *msg2 )
{
	char SocketWriteBuffer[ DEFAULT_BUFLEN ];
	sprintf_s( SocketWriteBuffer, DEFAULT_BUFLEN, "PRIVMSG %s : %s%s\n", GlobalStore.Channel, msg1, msg2 );
	raw( GlobalStore.ConnectSocket, SocketWriteBuffer );
}

DWORD WINAPI SocketWriterThread( LPVOID lpParam )
{
	int StartStamp = GetTickCount();
	while( GlobalStore.WorkerThreadAlive == 1 )
	{
		//nothing to send atm. Maybe later we will have something to send
		if( GlobalStore.AutoSendMessages.empty() 
			|| GlobalStore.NextIrcMsgStamp > GetTickCount() 
			)
			Sleep( 1000 );

		if( GlobalStore.IrcConnected == false )
			continue;

		//console added messages evade default cooldowns
		if( GlobalStore.AutoSendMessageQueue.empty() == false )
		{
			std::list<AutoSendMessage*>::iterator itr = GlobalStore.AutoSendMessageQueue.begin();
			AutoSendMessage *Store = (*itr);

			char SocketWriteBuffer[ DEFAULT_BUFLEN ];
			sprintf_s( SocketWriteBuffer, DEFAULT_BUFLEN, "PRIVMSG %s : BOT : %s\n", GlobalStore.Channel, Store->Message );
			raw( GlobalStore.ConnectSocket, SocketWriteBuffer );

			free( Store->Message );
			Store->Message = NULL;
			free( Store );

			GlobalStore.AutoSendMessageQueue.pop_front();

			GlobalStore.NextIrcMsgStamp += IRC_CHAT_COOLDOWN_MIN;
		}

		//these are periodic messages
		if( GlobalStore.NextIrcMsgStamp < GetTickCount() )
		{
			//search for next message to send
			bool PickNext = false, PickedSomething = false;
			for( int i=0;i<2;i++ )
			{
				std::list<AutoSendMessage*>::iterator itr;
				for( itr=GlobalStore.AutoSendMessages.begin(); itr!=GlobalStore.AutoSendMessages.end(); itr++ )
				{
					if( PickNext == true )
					{
						memcpy( &GlobalStore.TempAutoSendMessage, (*itr), sizeof( AutoSendMessage ) );
						PickedSomething = true;
						break;
					}
					else if( GlobalStore.TempAutoSendMessage.Message == (*itr)->Message )
						PickNext = true;
				}
				if( PickedSomething == true )
					break;
				PickNext = true;
			}

			//send this message then wait for a bit
			if( GlobalStore.TempAutoSendMessage.MessageType == IRC_WMSG_SEND_TXT_TO_CHANNEL )
			{
				char SocketWriteBuffer[ DEFAULT_BUFLEN ];
				sprintf_s( SocketWriteBuffer, DEFAULT_BUFLEN, "PRIVMSG %s : BOT : %s\n", GlobalStore.Channel, GlobalStore.TempAutoSendMessage.Message );
				raw( GlobalStore.ConnectSocket, SocketWriteBuffer );
				GlobalStore.NextIrcMsgStamp = GetTickCount() + GlobalStore.TempAutoSendMessage.Cooldown;
			}
		}
	}
	printf("Worker thread irc write handler exited\n");

	return 0;
}

void QueueBotMessage( char *msg )
{
	AutoSendMessage *Store = (AutoSendMessage *)malloc( sizeof( AutoSendMessage ) );
	Store->Message = _strdup( msg );
	Store->Cooldown = IRC_CHAT_COOLDOWN_MIN;
	Store->MessageType = IRC_WMSG_SEND_TXT_TO_CHANNEL_ONCE;
	GlobalStore.AutoSendMessageQueue.push_back( Store );
}

void QueueBotMessage( char *msg1, char *msg2 )
{
	char WriteBuffer[ DEFAULT_BUFLEN ];
	sprintf_s( WriteBuffer, DEFAULT_BUFLEN, "%s%s", msg1, msg2 );
	AutoSendMessage *Store = (AutoSendMessage *)malloc( sizeof( AutoSendMessage ) );
	Store->Message = _strdup( WriteBuffer );
	Store->Cooldown = IRC_CHAT_COOLDOWN_MIN;
	Store->MessageType = IRC_WMSG_SEND_TXT_TO_CHANNEL_ONCE;
	GlobalStore.AutoSendMessageQueue.push_back( Store );
}