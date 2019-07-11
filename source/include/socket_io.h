/*******************************************************************

Projekt battery-http-server

Plik: socket_io.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef SOCKET_IO_H
#define SOCKET_IO_H

#include "shared.h"

void    SOCKET_main( void );
void    SOCKET_free( void );

void    SOCKET_send( HTTP_SESSION *http_session, const char *buf, int http_content_size, int *res );
void    SOCKET_disconnect_client( HTTP_SESSION *http_session );
char*   SOCKET_get_remote_ip( HTTP_SESSION *http_session );
void    SOCKET_close_fd( int socket_fd );
void    SOCKET_modify_clients_count( int mod );

#endif
