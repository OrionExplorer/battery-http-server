/*******************************************************************

Projekt battery_Server

Plik: server_log.h

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#ifndef SERVER_LOG_H
#define SERVER_LOG_H

extern char				LOG_filename[MAX_PATH_LENGTH];
void					LOG_print( char *fmt, ... );
void					LOG_save( void );

#endif
