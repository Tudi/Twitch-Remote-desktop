#ifndef _STDAFX_H_
#define _STDAFX_H_

#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <set>
#include <list>
#include <conio.h>
#include <stdlib.h>
#include <time.h>

#define DEFAULT_BUFLEN			1024 * 65
#define EMPTY_LINES_FOR_CLS		120

#include "KeypressHandler.h"
#include "IniReader.h"
//http://oblita.com/interception.html
#include "interception.h"
#include "IrcHandler.h"
#include "ConsoleListener.h"
#include "Tools.h"
#include "PMLogger.h"

struct DemocracyVoteStore
{
	int	MonitoredKeycount;
	int	*MonitoredKeyCountPressedInterval;
};

enum MouseMoveTypesEnum
{
	MOUSE_MOVE_TYPE_SIMPLE_MOVE				= 0,	// move as defined in config
	MOUSE_MOVE_TYPE_ABSOLUTE_NORMALIZED		= 1,	// the mouse cords are from 0 to 65k and do not depend on desktop resolution
	MOUSE_MOVE_TYPE_ABSOLUTE_PIXEL			= 2,	// move mouse to x,y than click, then move back to old position
	MOUSE_MOVE_TYPE_RELATIVE_NORMALIZED		= 3,	// can not even exist
	MOUSE_MOVE_TYPE_ABSOLUTE_PIXEL_DOSBOX	= 4,	// move back based on tracked coordinates
};

struct IrcGameKeyStore
{
	int		PlayerGroup;
	char	*IrcText;
	int		IrcTextLen;
	int		StrokeCode;
	int		StrokePushdownDelay;
	int		StrokeReleaseDelay;
	int		PushesSinceLastUpdate;
	int		MouseX,MouseY,MouseKey,MouseFlags,MouseMoveType;
};

struct AutoSendMessage
{
	char	*Message;
	int		Cooldown;
	int		MessageType;	//in case you wish to script messages. Ex : anpounce best player
};

struct NoMouseClickBox
{
	int		XStart,YStart,XEnd,YEnd;
	int		MouseKey;	//left click or right click ? Maybe other flags later
};

struct GlobalStateStore
{
    char					*Nick;
    char					*Channel;
    char					*Host;
    char					*Port;
	char					*oAuth;
	bool					IrcConnected;
	unsigned int			NextIrcMsgStamp;
	InterceptionContext		context;
	InterceptionDevice		KeyboardDevice;
	InterceptionDevice		MouseDevice;
	InterceptionKeyStroke	KeyboardStroke;
	InterceptionMouseStroke	MouseStroke;
	int						WorkerThreadAlive;
	int						Sync1Stamp,Sync2Stamp;
	SOCKET					ConnectSocket;
	IrcGameKeyStore			TempKeyStore;
	std::list<IrcGameKeyStore*>	MonitoredKeys;
	int						DemoctraticVoteWait;
	bool					PrintKeysPressed;
	int						MouseX,MouseY;	//this can become bad in case window looses focus or user moves the mouse !
	int						MouseXLimitMin,MouseYLimitMin,MouseXLimitMax,MouseYLimitMax;	//this is only checked periodically !
	int						TrackedMouseX,TrackedMouseY,DosBoxWidth,DosBoxHeight;
	AutoSendMessage			TempAutoSendMessage;
	std::list<AutoSendMessage*>	AutoSendMessages;	//should use message queue to avoid irc ban
	std::list<AutoSendMessage*>	AutoSendMessageQueue;	//should use message queue to avoid irc ban
	char					*ActiveWindowName;
	int						RefocusWindow;
	NoMouseClickBox			TempDenyBox;
	std::list<NoMouseClickBox*>	DenyMouseActionBoxes;
	FILE					*fMyPMs;
	char					*cMyPMNick;
};

struct IrcServerMessage
{
	char *User;
	char *UserFull;
	char *ServerCommand;
	char *Channel;
	char *UserCommand;
	char *UserCommandParamOrMsg;
	char *NextLine;
};

extern GlobalStateStore GlobalStore;

#endif