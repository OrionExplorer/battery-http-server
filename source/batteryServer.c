/*******************************************************************

Projekt battery_Server

Plik: batteryServer.c

Przeznaczenie:
Uruchomenie funkcji server_initialize()

Kompilacja na Win32: -lws2_32

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#include "include/server_core.h"
#include "include/server_socket_io.h"
#include "include/server_log.h"
#include <signal.h>

void app_terminate(void);

int main(void)
{
	signal(SIGABRT, (sighandler)&app_terminate);
	signal(SIGTERM, (sighandler)&app_terminate);
	signal(SIGINT, (sighandler)&app_terminate);
	server_initialize();
	return 0;
}

void app_terminate(void)
{
	print_log("Server is being closed...\n");
	socket_free();
	print_log("Server closed.\n");
	log_save();
}
