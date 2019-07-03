/*******************************************************************

Projekt battery_Server

Plik: server_time_util.c

Przeznaczenie:
Zbiór funkcji przeznaczonych do zarządzania datą i czasem

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_shared.h"
#include <stdio.h>
#include <string.h>

/*
get_actual_time_gmt()
- pobiera aktualny czas
- zwraca char *z aktualnym czasem w formacie GMT */
char* get_actual_time_gmt( void ) {
	static char s[ TIME_BUFF_SIZE ];
	struct tm tim;
	time_t now;

	now = time( NULL );
	tim = *( localtime( &now ) );
	strftime( s, TIME_BUFF_SIZE, RFC1123FMT, &tim );

	return ( ( char* )&s );
}
