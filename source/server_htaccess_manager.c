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
short HTACCESS_load_configuration( const char *filename ) {
	FILE *cfg_file;
	char *buf;			/* Zmienna pomocnicza */
	char *res;			/* Przechowuje nazw� zasobu */
	char *res_login;	/* Przechowuje login do zasobu */
	char *res_pwd;		/* Przechowuje has�o do zasobu */
	char *res_auth_u;	/* Przechowuje niezaszyfrowane, sformatowane dane */
	char *res_auth_f;	/* Przechowuje zaszyfrowane dane */

	LOG_print( "Loading access configuration..." );

	cfg_file = fopen( filename, "rt" );

	if( !cfg_file ) {
		LOG_print( "error.\n" );
		return 0;
	} else {
		LOG_print( "ok.\n" );
	}

	/* Alokacja pami�ci */
	buf = ( char* )malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( buf, 1050 );

	res = ( char* )malloc( MAX_PATH_LENGTH_CHAR );
	mem_allocated( res, 1051 );

	res_login = ( char* )malloc( SMALL_BUFF_SIZE_CHAR );
	mem_allocated( res_login, 1052 );

	res_pwd = ( char* )malloc( SMALL_BUFF_SIZE_CHAR );
	mem_allocated( res_pwd, 1053 );

	res_auth_u = ( char* )malloc( STD_BUFF_SIZE_CHAR+1 );
	mem_allocated( res_auth_u, 1054 );

	res_auth_f = ( char* )malloc( STD_BUFF_SIZE_CHAR+1 );
	mem_allocated( res_auth_f, 1055 );

	while( fgets( buf, STD_BUFF_SIZE, cfg_file ) ) {
		if( sscanf( buf, "%s %s %s", res, res_login, res_pwd ) == 3 ) {
			/* Wczytanie danych z pliku */
			strncpy( ht_access[ht_access_count].res, res, MAX_PATH_LENGTH );
			strncpy( ht_access[ht_access_count].res_login, res_login, SMALL_BUFF_SIZE );
			strncpy( ht_access[ht_access_count].res_pwd, res_pwd, SMALL_BUFF_SIZE );

			/* Format loginu i has�a: "login:has�o" */
			strncpy( res_auth_u, res_login, STD_BUFF_SIZE );
			strncat( res_auth_u, ":", STD_BUFF_SIZE );
			strncat( res_auth_u, res_pwd, STD_BUFF_SIZE );

			/* Kodowanie sformatowanych danych algorytmem base64 */
			base64_encode( ( unsigned char* )res_auth_u, strlen( res_auth_u ), res_auth_f, STD_BUFF_SIZE );

			/* Zapisanie do elementu struktury ht_access informacji o spodziewanym ha�le */
			strncpy( ht_access[ht_access_count].res_auth, res_auth_f, STD_BUFF_SIZE );

			LOG_print( "\t- %d access to \"%s\" with login \"%s\", password \"%s\" ( %s ).\n", ht_access_count, res, res_login, res_pwd, res_auth_f );
			ht_access_count++;
		}
	}

	/* Zwolnienie pami�ci */
	free( buf );
	buf = NULL;

	free( res );
	res = NULL;

	free( res_login );
	res_login = NULL;

	free( res_pwd );
	res_pwd = NULL;

	free( res_auth_u );
	res_auth_u = NULL;

	free( res_auth_f );
	res_auth_f = NULL;

	/* Zamkni�cie pliku konfiguracyjnego */
	fclose( cfg_file );

	return 1;
}
