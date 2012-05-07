/*******************************************************************

Projekt battery_Server

Plik: server_socket_io.h

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#ifndef SERVER_SOCKET_IO_H
#define SERVER_SOCKET_IO_H

#include "server_shared.h"

void serverStart(void);

void			server_start_socket(void);
void			socket_run(void);
void			socket_free(void);

int				server_send_data_to_client(HTTP_SESSION *http_session, const char *buf, int http_content_size);
void			server_disconnect_client(HTTP_SESSION *http_session);
void			server_release_socket(HTTP_SESSION *http_session);
char*			server_get_remote_ip_address(HTTP_SESSION *http_session);

#endif
