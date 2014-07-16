#include "StdAfx.h"

void PMLoggerInit()
{
	if( GlobalStore.fMyPMs == NULL )
	{
		errno_t ret = fopen_s( &GlobalStore.fMyPMs, "MyPMs.txt", "at" );
	}
}

void PMLoggerEventChatMessageReceived( char *User, char *Msg )
{
	if( Msg == NULL )
		return;
	if( GlobalStore.fMyPMs == NULL )
		return;
	char Buff[ DEFAULT_BUFLEN ];
	strcpy_s( Buff, DEFAULT_BUFLEN, Msg );
	errno_t ret = _strlwr_s( Buff, DEFAULT_BUFLEN );
	if( strstr( Buff, GlobalStore.cMyPMNick ) != NULL )
	{
		time_t t = time(NULL);
		tm aTm;
		errno_t ret = localtime_s( &aTm, &t);
		fprintf( GlobalStore.fMyPMs, "[%-4d-%02d-%02d %02d:%02d:%02d] %s %s\n",aTm.tm_year+1900,aTm.tm_mon+1,aTm.tm_mday,aTm.tm_hour,aTm.tm_min,aTm.tm_sec, User, Msg );
	}
}


void PMLoggerShutdown()
{
	if( GlobalStore.fMyPMs != NULL )
	{
		fclose( GlobalStore.fMyPMs );
		GlobalStore.fMyPMs = NULL;
	}
}