/*******************************************************************

Projekt battery-http-server

Plik: mem_manager.c

Przeznaczenie:
Funkcje ułatwiające zarządzanie pamięcią

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/shared.h"
#include "include/mem_manager.h"
#include "include/log.h"
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

