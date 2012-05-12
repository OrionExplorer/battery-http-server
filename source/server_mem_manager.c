/*******************************************************************

Projekt battery_Server

Plik: server_mem_manager.c

Przeznaczenie:
Funkcje u�atwiaj�ce zarz�dzanie pami�ci�

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_shared.h"
#include "include/server_mem_manager.h"
#include "include/server_log.h"
#include <stdio.h>
#include <stdlib.h>

/*
mem_allocated( char* ptr, int n )
@ptr - wska�nik do przydzielonej pami�ci �a�cucha
@n - numer porz�dkowy, s�u��cy do p�niejszej analizy problemu
- funkcja sprawdza, czy uda�o si� przydzieli� pami�� dla danego wska�nika, ko�czy program, je�eli wyst�pi� problem */
void mem_allocated( char *ptr, int n ) {
	/* B��d alokacji pami�ci, ptr = NULL */
	if( !ptr ) {
		LOG_print( "Error: ( char* )malloc( %d ).\n", n );
		printf( "Error: ( char* )malloc( %d ).\n", n );
		exit( EXIT_FAILURE );
	}
	/* Wszystko ok, kontynuuje dzia�anie programu */
}

