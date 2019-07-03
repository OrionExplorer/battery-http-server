/*******************************************************************

Projekt battery-http-server

Plik: server_mem_manager.c

Przeznaczenie:
Funkcje ułatwiające zarządzanie pamięcią

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_shared.h"
#include "include/server_mem_manager.h"
#include "include/server_log.h"
#include <stdio.h>
#include <stdlib.h>

/*
mem_allocated( char* ptr, int n )
@ptr - wskaźnik do przydzielonej pamięci łańcucha
@n - numer porządkowy, służący do póniejszej analizy problemu
- funkcja sprawdza, czy udało się przydzielić pamięć dla danego wskaźnika, kończy program, jeżeli wystąpił problem */
void mem_allocated( char *ptr, int n ) {
	/* Błąd alokacji pamięci, ptr = NULL */
	if( !ptr ) {
		LOG_print( "Error: malloc( %d ).\n", n );
		printf( "Error: malloc( %d ).\n", n );
		exit( EXIT_FAILURE );
	}
	/* Wszystko ok, kontynuuje działanie programu */
}

