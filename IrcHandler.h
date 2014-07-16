#ifndef _IRC_HANDLER_H_
#define _IRC_HANDLER_H_

#define IRC_RECONNECT_WAIT		10000
//if you spam messages, twitch will ban you from chat for quite a while
#define IRC_CHAT_COOLDOWN_MIN	2000

enum IrcWriteMessageTypes
{
	IRC_WMSG_SEND_TXT_TO_CHANNEL		= 1,
	IRC_WMSG_SEND_TXT_TO_CHANNEL_ONCE	= 2,
};

struct IrcServerMessage;

void raw( SOCKET conn, char *fmt, ...);
void ExplodeIrcReadBuffer( char *ReadBuff, int MaxInputLen, IrcServerMessage &out );
DWORD WINAPI SocketReaderThread( LPVOID lpParam );
DWORD WINAPI SocketWriterThread( LPVOID lpParam );
void	ShutdownIrcConnection();
int		ReConnectToIrc( bool FirstConnect );
void	QueueBotMessage( char *msg );
void	QueueBotMessage( char *msg1, char *msg2 );
void	SendChatToChannel( char *msg );
void	SendChatToChannel( char *msg1, char *msg2 );

#endif