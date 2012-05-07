/*******************************************************************

Projekt battery_Server

Plik: server_core.h

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#ifndef SERVER_CORE_H
#define SERVER_CORE_H

void			server_initialize(void);
void			server_run(void);

int				server_load_configuration(void);
int				server_load_index_configuration(const char* filename);

int				server_load_access_configuration(const char *cfg_file);
#endif
