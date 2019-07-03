/*******************************************************************

Projekt battery-http-server

Plik: server_cgi_manager.h

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#ifndef SERVER_CGI_MANAGER_H
#define SERVER_CGI_MANAGER_H

#include "server_shared.h"

#define CGI_VER				"CGI/1.1"		/* Wersja protoko³u CGI */

void			CGI_execute( HTTP_SESSION *http_session, const char *filename );
short			CGI_load_configuration( const char *cfg_file );
void			CGI_valid( const char *filename, int *valid_res, char *exec_name, char *param );

#endif
