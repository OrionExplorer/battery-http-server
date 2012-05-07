/*******************************************************************

Projekt battery_Server

Plik: server_files_io.h

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#ifndef SERVER_FILES_IO_H
#define SERVER_FILES_IO_H

#include "server_shared.h"

char*	get_app_path(void);
char*	file_get_ext(const char *filename);
char*	file_get_name(const char *full_filename);
void	file_extract_path(char *full_filename, char delim);
int		directory_exists(const char *path);
int		file_exists(const char *filename);
int		file_params(HTTP_SESSION *http_session, const char *filename, char *ht_access_pwd);

#endif
