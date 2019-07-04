/*******************************************************************

Projekt battery-http-server

Plik: cgi_manager.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef CGI_MANAGER_H
#define CGI_MANAGER_H

#include "shared.h"

#define CGI_VER "CGI/1.1"       /* Wersja protokołu CGI */

void            CGI_execute( HTTP_SESSION *http_session, const char *filename );
short           CGI_load_configuration( const char *cfg_file );
void            CGI_valid( const char *filename, int *valid_res, char *exec_name, char *param );

#endif
