/*******************************************************************

Projekt battery_Server

Plik: server_cgi_manager.h

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#ifndef SERVER_CGI_MANAGER_H
#define SERVER_CGI_MANAGER_H

#include "server_shared.h"

#define CGI_VER				"CGI/1.1"		/* Wersja protoko³u CGI */

void			cgi_execute_script(HTTP_SESSION *http_session, const char *filename);
int				cgi_load_script_configuration(const char *cfg_file);

#endif
