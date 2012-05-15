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
short			SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size );
void			SESSION_add_new_ptr( HTTP_SESSION *http_session );
void			SESSION_delete_ptr( HTTP_SESSION *http_session );
SEND_INFO*		SESSION_find_response_struct_by_id( int socket );
HTTP_SESSION*	SESSION_find_by_id( int socket );
void			SESSION_add_new_send_struct( int socket_descriptor );

#endif
