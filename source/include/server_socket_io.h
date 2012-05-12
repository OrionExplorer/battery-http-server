/*******************************************************************

Projekt battery_Server

Plik: server_socket_io.h

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#ifndef SERVER_SOCKET_IO_H
#define SERVER_SOCKET_IO_H

#include "server_shared.h"

void serverStart( void );

void			SOCKET_main( void );
void			SOCKET_run( void );
void			SOCKET_stop( void );

int				SOCKET_send( HTTP_SESSION *http_session, const char *buf, int http_content_size );
void			SOCKET_disconnect_client( HTTP_SESSION *http_session );
void			SOCKET_release( HTTP_SESSION *http_session );
char*			SOCKET_get_remote_ip( HTTP_SESSION *http_session );
void			SOCKET_process( void *socket_fd );

#endif
