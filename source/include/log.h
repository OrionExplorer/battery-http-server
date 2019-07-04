/*******************************************************************

Projekt battery-http-server

Plik: log.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef LOG_H
#define LOG_H

extern char LOG_filename[ MAX_PATH_LENGTH ];
void        LOG_print( char *fmt, ... );
void        LOG_save( void );

#endif
