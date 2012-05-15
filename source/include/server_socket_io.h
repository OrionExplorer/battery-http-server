/*******************************************************************

Projekt battery_Server

Plik: server_socket_io.h

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#ifndef SERVER_SOCKET_IO_H
#define SERVER_SOCKET_IO_H

#include "server_shared.h"

void			SOCKET_main( void );
void			SOCKET_run( void );
void			SOCKET_stop( void );

void			SOCKET_send( HTTP_SESSION *http_session, const char *buf, int http_content_size, int *res );
void			SOCKET_disconnect_client( HTTP_SESSION *http_session );
void			SOCKET_release( HTTP_SESSION *http_session );
char*			SOCKET_get_remote_ip( HTTP_SESSION *http_session );

#endif
