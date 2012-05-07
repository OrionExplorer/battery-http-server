/*******************************************************************

Projekt battery_Server

Plik: server_create_session.h

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#ifndef SERVER_CREATE_SESSION_H
#define SERVER_CREATE_SESSION_H

#include "server_shared.h"

void			data_prepare_http_session(HTTP_SESSION *http_session);
void			data_release_http_session(HTTP_SESSION *http_session);
int				data_send_http_response(HTTP_SESSION *http_session, const char *content_data, int http_content_size);

#endif
