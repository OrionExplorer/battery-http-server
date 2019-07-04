/*******************************************************************

Projekt battery-http-server

Plik: log.h

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#ifndef LOG_H
#define LOG_H

extern char LOG_filename[ MAX_PATH_LENGTH ];
void        LOG_print( char *fmt, ... );
void        LOG_save( void );

#endif
