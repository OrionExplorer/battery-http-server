/*******************************************************************

Projekt battery_Server

Plik: server_create_session.h

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#ifndef SERVER_CREATE_SESSION_H
#define SERVER_CREATE_SESSION_H

#include "server_shared.h"

void			SESSION_prepare( HTTP_SESSION *http_session );
void			SESSION_release( HTTP_SESSION *http_session );
int				SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size );

#endif
