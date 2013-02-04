/*******************************************************************

Projekt battery_Server

Plik: server_htaccess_manager.c

Przeznaczenie:
Wczytanie ustawie� dotycz�cych dost�p�w do danych zasob�w

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_shared.h"
#include "include/server_htaccess_manager.h"
#include "include/server_log.h"
#include "include/server_base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int	ht_access_count = 0;

/*
HTACCESS_load_configuration( const char *filename )
@filename - nazwa pliku konfiguracyjnego
- zwraca int, gdzie 1 = konfiguracja wczytana poprawnie. */
short HTACCESS_load_configuration( FILE *cfg_file ) {
	char *buf;			/* Zmienna pomocnicza */
	char *option;		/* Wczytany typ danych */
	char *res_filename;	/* Przechowuje nazw� zasobu */
	char *res_login;	/* Przechowuje login do zasobu */
	char *res_pwd;		/* Przechowuje has�o do zasobu */
	char *res_auth_u;	/* Przechowuje niezaszyfrowane, sformatowane dane */
	char *res_auth_f;	/* Przechowuje zaszyfrowane dane */

	if( !cfg_file ) {
		return 0;
	}

	rewind( cfg_file );

	/* Alokacja pami�ci */
	buf = malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( buf, 21 );

	res_filename = malloc( MAX_PATH_LENGTH_CHAR );
	mem_allocated( res_filename, 22 );

	res_login = malloc( SMALL_BUFF_SIZE_CHAR );
	mem_allocated( res_login, 23 );

	res_pwd = malloc( SMALL_BUFF_SIZE_CHAR );
	mem_allocated( res_pwd, 24 );

	res_auth_u = malloc( STD_BUFF_SIZE_CHAR+1 );
	mem_allocated( res_auth_u, 25 );

	res_auth_f = malloc( STD_BUFF_SIZE_CHAR+1 );
	mem_allocated( res_auth_f, 26 );

	option = malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( option, 27 );

	while( fgets( buf, STD_BUFF_SIZE, cfg_file ) ) {
		if( sscanf( buf, "%s %s %s %s", option, res_filename, res_login, res_pwd ) == 4 ) {
			if( strncmp( option, "global_ht_access", STD_BUFF_SIZE ) == 0 ) {
				/* Wczytanie danych z pliku */
				strncpy( ht_access[ ht_access_count ].res_filename, res_filename, MAX_PATH_LENGTH );
				strncpy( ht_access[ ht_access_count ].res_login, res_login, SMALL_BUFF_SIZE );
				strncpy( ht_access[ ht_access_count ].res_pwd, res_pwd, SMALL_BUFF_SIZE );

				/* Format loginu i has�a: "login:has�o" */
				strncpy( res_auth_u, res_login, STD_BUFF_SIZE );
				strncat( res_auth_u, ":", STD_BUFF_SIZE );
				strncat( res_auth_u, res_pwd, STD_BUFF_SIZE );

				/* Kodowanie sformatowanych danych algorytmem base64 */
				base64_encode( ( unsigned char* )res_auth_u, strlen( res_auth_u ), res_auth_f, STD_BUFF_SIZE );

				/* Zapisanie do elementu struktury ht_access informacji o spodziewanym ha�le */
				strncpy( ht_access[ ht_access_count ].res_auth, res_auth_f, STD_BUFF_SIZE );

				LOG_print( "\t- access to \"%s\" with login \"%s\", password \"%s\" ( %s ).\n", res_filename, res_login, res_pwd, res_auth_f );
				ht_access_count++;
			}
		}
	}

	/* Zwolnienie pami�ci */
	free( buf );
	buf = NULL;

	free( option );
	option = NULL;

	free( res_filename );
	res_filename = NULL;

	free( res_login );
	res_login = NULL;

	free( res_pwd );
	res_pwd = NULL;

	free( res_auth_u );
	res_auth_u = NULL;

	free( res_auth_f );
	res_auth_f = NULL;

	/* Zamkni�cie pliku konfiguracyjnego */
	//fclose( cfg_file );

	return 1;
}
