/*******************************************************************

Projekt battery_Server

Plik: server_mime_types.c

Przeznaczenie:
Wczytanie typ�w mime obs�ugiwanych przez serwer

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_shared.h"
#include "include/server_log.h"
#include "include/server_mime_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Przechowuje informacj� o ilo�ci wczytanych typ�w mime */
int			mime_types_count;

/*
MIME_load_configuration( const char *filename )
@filename - nazwa pliku konfiguracyjnego
- zwraca int, gdzie 1 = uda�o si� wczyta� konfiguracj�, a 0 = nie uda�o si�. */
short MIME_load_configuration( FILE *cfg_file) {
	char buf[ STD_BUFF_SIZE ];				/* Wczytana linia z pliku */
	char mime_ext[ EXT_LEN ];				/* Wczytane rozszerzenie pliku z buf */
	char option[ STD_BUFF_SIZE ];			/* Wczytany typ danych */
	char mime_type[ SMALL_BUFF_SIZE ];		/* Wczytany typ MIME z pliku */

	if( !cfg_file ) {
		return 0;
	}

	rewind( cfg_file );

	/* Ilo�� wczytanych typ�w MIME */
	mime_types_count = 0;

	while( fgets( buf, STD_BUFF_SIZE, cfg_file ) ) {
		if( ( sscanf( buf, "%256s %16s %32s", option, mime_ext, mime_type ) == 3 ) && ( mime_types_count < STD_BUFF_SIZE ) ) {
			if( strncmp( option, "mime_type", STD_BUFF_SIZE ) == 0 ) {
				/* Wczytanie rozszerzenia pliku */
				strncpy( mime_types[ mime_types_count ].ext, mime_ext, EXT_LEN );
				/* Wczytanie pliku MIME */
				strncpy( mime_types[ mime_types_count ].mime_type, mime_type, SMALL_BUFF_SIZE );
				LOG_print( "\t- mime type \"%s\" assigned to \"%s\".\n", mime_types[ mime_types_count ].ext, mime_types[ mime_types_count ].mime_type );
				mime_types_count++;
			}
		}
	}

	return 1;
}
