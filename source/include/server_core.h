/*******************************************************************

Projekt battery_Server

Plik: server_core.h

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#ifndef SERVER_CORE_H
#define SERVER_CORE_H

void			CORE_initialize( void );
void			CORE_start( void );
short			CORE_load_configuration( void );
short			CORE_load_index_names( const char* filename );

#endif
