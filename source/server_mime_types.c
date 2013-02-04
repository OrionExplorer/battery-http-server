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
	char *buf;				/* Wczytana linia z pliku */
	char *ext;				/* Wczytane rozszerzenie pliku z buf */
	char *option;			/* Wczytany typ danych */
	char *mime_type;		/* Wczytany typ MIME z pliku */

	if( !cfg_file ) {
		return 0;
	}

	rewind( cfg_file );

	/* Alokacja pami�ci */
	buf = malloc( STD_BUFF_SIZE_CHAR );

	ext = malloc( EXT_LEN_CHAR );

	mime_type = malloc( SMALL_BUFF_SIZE_CHAR );

	option = malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( option, 2521 );

	/* Ilo�� wczytanych typ�w MIME */
	mime_types_count = 0;

	while( fgets( buf, STD_BUFF_SIZE, cfg_file ) ) {
		if( ( sscanf( buf, "%s %s %s", option, ext, mime_type ) == 3 ) && ( mime_types_count < STD_BUFF_SIZE ) ) {
			if( strncmp( option, "mime_type", STD_BUFF_SIZE ) == 0 ) {
				/* Wczytanie rozszerzenia pliku */
				strncpy( mime_types[ mime_types_count ].ext, ext, EXT_LEN );
				/* Wczytanie pliku MIME */
				strncpy( mime_types[ mime_types_count ].mime_type, mime_type, SMALL_BUFF_SIZE );
				LOG_print( "\t- mime type \"%s\" assigned to \"%s\".\n", mime_types[ mime_types_count ].ext, mime_types[ mime_types_count ].mime_type );
				mime_types_count++;
			}
		}
	}

	/* Zamkni�cie pliku konfiguracyjnego */
	//fclose( cfg_file );

	/* Zwolnienie pami�ci */
	free( buf );
	buf = NULL;

	free( ext );
	ext = NULL;

	free( mime_type );
	mime_type = NULL;

	free( option );
	option = NULL;

	return 1;
}
