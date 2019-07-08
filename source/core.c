/*******************************************************************

Projekt battery-http-server

Plik: core.c

Przeznaczenie:
Konfiguracja aplikacji
Ustawienie nasłuchiwania socketów

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/core.h"
#include "include/shared.h"
#include "include/files_io.h"
#include "include/socket_io.h"
#include "include/base64.h"
#include "include/log.h"
#include "include/mime_types.h"
#include "include/htaccess_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


/* Ścieżka startowa aplikacji */
extern char app_path[];

/* Katalog roboczy - udostępnione klientom zasoby */
char    *document_root;

/* Informuje, czy obsługa SSL jest aktywna */
int     ssl_on;

/* Plik certyfikatu SSL */
char    *ssl_cert_file;

/* Plik klucza prywatnego SSL */
char    *ssl_key_file;

/*Pełna nazwa pliku (ścieżka dostępu ) "log.txt" */
char    LOG_filename[ MAX_PATH_LENGTH ];

/*Przechowuje informację o typach adresów IP: IPv4 lub IPv6 */
int     ip_proto_ver = -1;

/* Przechowuje informację o liczbie wczytanych rozszerzeń dla innych typów plików */
int     cgi_other_count = 0;

/* Przechowuje informację o liczbie wczytanych nazw plikóww index */
int     index_file_count = 0;

static void     server_log_prepare( void );
static void     server_validate_paths( void );

/*
server_log_prepare()
- sprawdza, czy istnieje folder, w którym przechowywany będzie log z działania aplikacji
- tworzy plik "log.txt" w katalogu logs */
static void server_log_prepare( void ) {
    char *tmp_path = malloc( MAX_PATH_LENGTH_CHAR+1 );

    /* Utworzenie ścieżki do pliku "log.txt" */
    strncpy( tmp_path, app_path, MAX_PATH_LENGTH );
    strncat( tmp_path, LOGS_PATH, MAX_PATH_LENGTH );

    /* Weryfikacja istnienia ścieżki do pliku "log.txt" */
    if( directory_exists( tmp_path ) == 0 ) {
        LOG_print( "Creating path: %s...\n", tmp_path );
        if( mkdir( tmp_path, 0777 ) != 0 ) { /* Utworzenie ścieżki */
            LOG_print( "\t- Error creating path!\n" );
        }

        if( chdir( app_path ) != 0 ) {
            LOG_print( "Error: chdir().\n" );
            return;
        }
    }

    LOG_print( "\t- verified: %s.\n", tmp_path );

    /* Dodanie do utworzonej ścieżki nazwy pliku "log.txt" */
    strncpy( LOG_filename, tmp_path, MAX_PATH_LENGTH );
    strncat( LOG_filename, "log.txt", MAX_PATH_LENGTH );

    LOG_print( "NEW SERVER SESSION.\n" );
    LOG_print( "LOG_filename: %s.\n", LOG_filename );

    free( tmp_path );
    tmp_path = NULL;
}

/*
server_validate_paths()
- sprawdza, czy istnieją wszystie foldery niezbędne do poprawnego działania aplikacji */
static void server_validate_paths( void ) {
    char *tmp_path = malloc( MAX_PATH_LENGTH_CHAR+1 );
    int res = -1;

    LOG_print( "Starting server_validate_paths()...\n" );

    /* Przypisanie ścieżki, z której uruchamiana jest aplikacja */
    strncpy( tmp_path, app_path, MAX_PATH_LENGTH );

    /* Dopisanie ścieżki do pliku "log.txt", bez nazwy pliku */
    strncat( tmp_path, LOGS_PATH, MAX_PATH_LENGTH );
    if( directory_exists( tmp_path ) == 0 ) {
        LOG_print( "Creating path: %s...\n", tmp_path );
        if( mkdir( tmp_path, 0777 ) != 0 ) {/*Utworzenie ścieżki */
            LOG_print( "\t- Error ( %d ) creating path!\n", res );
        }
        if( chdir( app_path ) != 0 ) {
            LOG_print( "Error: chdir().\n" );
            return;
        }
    }
    LOG_print( "\t- verified: %s\n", tmp_path );

    /* Patrz opis funkcji server_log_prepare() */
    server_log_prepare();

    LOG_print( "server_validate_paths() done.\n" );

    free( tmp_path );
    tmp_path = NULL;
}

short CORE_load_index_names( FILE *cfg_file ) {
    char buf[ STD_BUFF_SIZE ];
    char option[ STD_BUFF_SIZE ];
    char index_filename[ STD_BUFF_SIZE ];
    int len = 0;

    if( !cfg_file ) {
        return 0;
    }

    rewind( cfg_file );

    index_file_count = 0;

    while( fgets( buf, STD_BUFF_SIZE, cfg_file ) ) {
        if( sscanf( buf, "%256s %256s", option, index_filename ) == 2 ) {
            if( strncmp( option, "site_index", STD_BUFF_SIZE ) == 0 ) {
                /* Wczytano maksymalną liczbę nazw plików */
                if( index_file_count == MICRO_BUFF_SIZE ) {
                    LOG_print( "Reached maximum list index count.\n" );
                    break;
                }

                /* Pobranie długości wczytanej nazwy pliku */
                len = strlen( buf );

                /* Stworzenie nowego obiektu */
                index_file_list[ index_file_count ] = ( char* )calloc( len, sizeof( char ) );
                mem_allocated( index_file_list[ index_file_count ], 4 );

                strncpy( index_file_list[ index_file_count ], index_filename, len );
                LOG_print( "\t- new site index: %s.\n", index_file_list[ index_file_count ] );
                index_file_count++;
            }
        }
    }

    return 1;
}

/*
CORE_load_configuration()
- jeśli istnieje, wczytuje plik "battery.conf" i z niego pobiera konfigurację dla zmiennych:
+ ip_proto_ver
+ active_port */
short CORE_load_configuration( void ) {
    FILE *cfg_file;
    char network_configuration_filename[ MAX_PATH_LENGTH ];     /* Nazwa pliku konfiguracji sieci */
    char buf[ STD_BUFF_SIZE ];                                  /* Wczytana linia z pliku konfiguracyjnego */
    char option[ STD_BUFF_SIZE ];                               /* Wczytana wartość z buf */
    char value[ STD_BUFF_SIZE ];                                /* Wczytana wartość z buf */

    /* Przypisanie ścieżki, z której uruchamiana jest aplikacja */
    strncpy( network_configuration_filename, app_path, MAX_PATH_LENGTH );
    /* Dopisanie nazw plików z konfiguracją */
    strncat( network_configuration_filename, NETWORK_CFG_PATH, MAX_PATH_LENGTH );

    LOG_print( "Loading configuration file ( %s )...", network_configuration_filename );
    cfg_file = fopen( network_configuration_filename, "rt" );

    if( cfg_file ) {
        /* Reset zmiennych */
        ip_proto_ver = -1;
        active_port = -1;
        ssl_cert_file = NULL,
        ssl_key_file = NULL;

        LOG_print( "\n\t- file opened successfully...\n" );

        while( fgets( buf, STD_BUFF_SIZE, cfg_file ) != NULL ) {
            if( sscanf( buf, "%256s %256s", option, value ) == 2 ) {

                if( strncmp( option, "ip_ver", STD_BUFF_SIZE) == 0 ) {
                    switch ( atoi( value ) ) {
                        case IPv4 : ip_proto_ver = IPv4; break;
                        case IPv6 : ip_proto_ver = IPv6; break;
                        default : ip_proto_ver = IPv4; break;
                    }
                    LOG_print( "\t- variable loaded: ip_proto_ver = %d.\n", ip_proto_ver );
                }

                else if( strncmp( option, "port_number", STD_BUFF_SIZE ) == 0 ) {
                    active_port = atoi( value );
                    LOG_print( "\t- variable loaded: active_port = %d.\n", active_port );
                }

                else if( strncmp( option, "document_root", STD_BUFF_SIZE ) == 0 ) {
                    /* Alokacja pamięci */
                    document_root = malloc( STD_BUFF_SIZE );
                    mem_allocated( document_root, 9 );

                    strncpy( document_root, value, STD_BUFF_SIZE );
                    if( directory_exists( document_root ) == 0 ) { /* Podany zasób nie istnieje */
                        LOG_print( "\t- Error: document root path is invalid.\n" );
                        printf( "Error: document root path is invalid: \"%s\"\n", document_root );
                        exit( EXIT_FAILURE );
                    } else {
                        LOG_print( "\t- document root path: %s\n", document_root );
                    }
                }

                else if( strncmp( option, "ssl_certificate_file", STD_BUFF_SIZE ) == 0 ) {
                    /* Weryfikacja czy plik istnieje */
                    if( file_exists( value ) == 1 ) {
                        LOG_print("\t- SSL certificate: %s.\n", value );
                        LOG_save();
                        /* Istnieje - alokacja pamięci */
                        ssl_cert_file = malloc( STD_BUFF_SIZE );
                        mem_allocated( ssl_cert_file, 91 );
                        strncpy( ssl_cert_file, value, STD_BUFF_SIZE );
                    } else {
                        /* Nie istnieje */
                        LOG_print("\t- Error: SSL certificate file does not exist: %s.\n", value );
                        LOG_save();
                        ssl_cert_file = NULL;
                    }
                }

                else if( strncmp( option, "ssl_key_file", STD_BUFF_SIZE ) == 0 ) {
                    /* Weryfikacja czy plik istnieje */
                    if( file_exists( value ) == 1 ) {
                        LOG_print("\t- SSL private key: %s.\n", value );
                        LOG_save();
                        /* Istnieje - alokacja pamięci */
                        ssl_key_file = malloc( MAX_PATH_LENGTH );
                        mem_allocated( ssl_key_file, 92 );
                        strncpy( ssl_key_file, value, MAX_PATH_LENGTH );
                    } else {
                        /* Nie istnieje */
                        LOG_print("\t- Error: SSL private key file does not exist: %s.\n", value );
                        LOG_save();
                        ssl_key_file = NULL;
                    }
                }
            }
        }

        /* Sprawdzenie, czy udało się wczytać pliki certyfikatu i klucza prywatnego SSL.
            Jeśli tak - obsługa SSL może być aktywna na serwerze, o ile wybrany port to 443.
         */
        ssl_on = active_port == 443 && ssl_cert_file && ssl_key_file;

        /* Wczytanie praw dostępów do zasobów */
        if( HTACCESS_load_configuration( cfg_file ) == 0 ) {
            LOG_save();
            return 0;
        }

        /* Wczytanie listy plików index */
        if( CORE_load_index_names( cfg_file ) == 0 ) {
            LOG_save();
            return 0;
        }

        /* Wczytanie typów MIME */
        if( MIME_load_configuration( cfg_file ) == 0 ) {
            LOG_save();
            return 0;
        }

        LOG_print( "Configuration file loaded successfully.\n" );

        /* Zamknięcie pliku konfiguracji */
        fclose( cfg_file );

        return 1;
    } else {
        return 0;
    }

    LOG_print( "error or file not found.\n" );

    return 0;
}

/*
CORE_initialize()
- zastępuje funkcję main()
- inicjuje zmienne globalne:
+ app_path
- uruchamia procedury konfiguracyjne:
+ CORE_load_configuration()
- uruchamia obsługę danych:
+ CORE_start() */
void CORE_initialize( void ) {
    /* Pobranie ścieżki startowej aplikacji */
    strncpy( app_path, get_app_path(), MAX_PATH_LENGTH );

    /*Patrz opis funkcji server_validate_paths() */
    ( void )server_validate_paths();

    LOG_print( "Application start path:\n%s\n", app_path );

    /*Patrz opisy poszczególnych funkcji */
    if( CORE_load_configuration() == 0 ) {
        LOG_print( "Error: unable to load configuration.\n" );
        printf( "Error: unable to load configuration.\n" );
        LOG_save();
        exit( EXIT_FAILURE );
    }

    /* Zapisanie informacji o serwerze do log.txt */
    LOG_save();

    atexit( SOCKET_free );

    /* Przełączenie się na folder, z którego będa udostępniane zasoby */
    strncpy( app_path, document_root, MAX_PATH_LENGTH );

    /* Uruchomienie sieci */
    SOCKET_main();
}

