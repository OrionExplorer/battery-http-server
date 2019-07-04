/*******************************************************************

Projekt battery-http-server

Plik: session.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef SESSION_H
#define SESSION_H

#include "shared.h"

void            SESSION_prepare( HTTP_SESSION *http_session );
void            SESSION_release( HTTP_SESSION *http_session );
int             SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size );
SEND_INFO*      SESSION_find_response_struct_by_id( int socket );
HTTP_SESSION*   SESSION_find_by_id( int socket );
void            SESSION_add_new_send_struct( int socket_descriptor );
void            SESSION_delete_send_struct( int socket_descriptor );
void            SESSION_init( HTTP_SESSION *http_session );

#endif
