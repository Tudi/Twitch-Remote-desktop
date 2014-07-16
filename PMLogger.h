#ifndef _PM_LOGGER_H_
#define _PM_LOGGER_H_

void PMLoggerInit();
void PMLoggerEventChatMessageReceived( char *User, char *Msg );
void PMLoggerShutdown();

#endif