/*******************************************************************

Projekt battery_Server

Plik: batteryServer.c

Przeznaczenie:
Uruchomenie funkcji CORE_initialize()

Kompilacja na Win32: -lws2_32

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include <stdio.h>
#include <signal.h>
#include "include/server_core.h"
#include "include/server_socket_io.h"
#include "include/server_log.h"

void app_terminate( void );

int main( void ) {
	signal( SIGABRT, ( sighandler )&app_terminate );
	signal( SIGTERM, ( sighandler )&app_terminate );
	signal( SIGINT, ( sighandler )&app_terminate );

	printf( "%s\n", SERVER_NAME );
	CORE_initialize();
	return 0;
}

void app_terminate( void ) {
	LOG_print( "Server is being closed...\n" );
	printf( "Server is being closed..." );
	SOCKET_stop();
	LOG_print( "Server closed.\n" );
	printf( "ok.\n" );
	LOG_save();
}
