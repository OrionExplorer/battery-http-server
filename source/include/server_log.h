/*******************************************************************

Projekt battery_Server

Plik: server_log.h

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#ifndef SERVER_LOG_H
#define SERVER_LOG_H

extern char	log_filename[MAX_PATH_LENGTH];
void		print_log(char *fmt, ...);
void		log_save(void);

#endif