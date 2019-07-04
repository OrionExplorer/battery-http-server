/*******************************************************************

Projekt battery-http-server

Plik: server_htaccess_manager.c

Przeznaczenie:
Wczytanie ustawień dotyczących dostępów do danych zasobów

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_shared.h"
#include "include/server_htaccess_manager.h"
#include "include/server_log.h"
#include "include/server_base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ht_access_count = 0;

/*
HTACCESS_load_configuration( const char *filename )
@filename - nazwa pliku konfiguracyjnego
- zwraca int, gdzie 1 = konfiguracja wczytana poprawnie. */
short HTACCESS_load_configuration( FILE *cfg_file ) {
    char buf[ STD_BUFF_SIZE ];              /* Zmienna pomocnicza */
    char option[ STD_BUFF_SIZE ];           /* Wczytany typ danych */
    char res_filename[ MAX_PATH_LENGTH ];   /* Przechowuje nazwę zasobu */
    char res_login[ SMALL_BUFF_SIZE ];      /* Przechowuje login do zasobu */
    char res_pwd[ SMALL_BUFF_SIZE ];        /* Przechowuje hasło do zasobu */
    char res_auth_u[ STD_BUFF_SIZE ];       /* Przechowuje niezaszyfrowane, sformatowane dane */
    char res_auth_f[ STD_BUFF_SIZE ];       /* Przechowuje zaszyfrowane dane */

    if( !cfg_file ) {
        return 0;
    }

    rewind( cfg_file );

    while( fgets( buf, STD_BUFF_SIZE, cfg_file ) ) {
        if( sscanf( buf, "%256s %1024s %32s %32s", option, res_filename, res_login, res_pwd ) == 4 ) {
            if( strncmp( option, "global_ht_access", STD_BUFF_SIZE ) == 0 ) {
                /* Wczytanie danych z pliku */
                strncpy( ht_access[ ht_access_count ].res_filename, res_filename, MAX_PATH_LENGTH );
                strncpy( ht_access[ ht_access_count ].res_login, res_login, SMALL_BUFF_SIZE );
                strncpy( ht_access[ ht_access_count ].res_pwd, res_pwd, SMALL_BUFF_SIZE );

                /* Format loginu i hasła: "login:hasło" */
                strncpy( res_auth_u, res_login, STD_BUFF_SIZE );
                strncat( res_auth_u, ":", STD_BUFF_SIZE );
                strncat( res_auth_u, res_pwd, STD_BUFF_SIZE );

                /* Kodowanie sformatowanych danych algorytmem base64 */
                base64_encode( ( unsigned char* )res_auth_u, strlen( res_auth_u ), res_auth_f, STD_BUFF_SIZE );

                /* Zapisanie do elementu struktury ht_access informacji o spodziewanym haśle */
                strncpy( ht_access[ ht_access_count ].res_auth, res_auth_f, STD_BUFF_SIZE );

                LOG_print( "\t- access to \"%s\" with login \"%s\", password \"%s\" ( %s ).\n", res_filename, res_login, res_pwd, res_auth_f );
                ht_access_count++;
            }
        }
    }

    return 1;
}
