/*******************************************************************

Projekt battery-http-server

Plik: battery.c

Przeznaczenie:
Uruchomenie funkcji CORE_initialize()

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "include/core.h"
#include "include/socket_io.h"
#include "include/log.h"

static void app_terminate( void );

int main( void ) {

    signal( SIGINT, ( sighandler )&app_terminate );
    #ifdef __linux__ 
        signal( SIGPIPE, SIG_IGN );
        signal( SA_RESTART, ( sighandler )&app_terminate );
    #endif
    signal( SIGABRT, ( sighandler )&app_terminate );
    signal( SIGTERM, ( sighandler )&app_terminate );

    printf( "%s\n", SERVER_NAME );
    CORE_initialize();
    return 0;
}

static void app_terminate( void ) {
    LOG_print( "Server is being closed...\n" );
    SOCKET_free();
    LOG_print( "Server closed.\n" );
    LOG_save();
    exit( EXIT_SUCCESS );
}
