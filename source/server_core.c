/*******************************************************************

Projekt battery_Server

Plik: server_core.c

Przeznaczenie:
Konfiguracja aplikacji
Ustawienie nas�uchiwania socket�w

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_core.h"
#include "include/server_shared.h"
#include "include/server_files_io.h"
#include "include/server_socket_io.h"
#include "include/server_base64.h"
#include "include/server_log.h"
#include "include/server_mime_types.h"
#include "include/server_htaccess_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


/* �cie�ka startowa aplikacji */
extern char	app_path[];

/* Katalog roboczy - udost�pnione klientom zasoby */
char	*document_root;

/*Pe�na nazwa pliku ( +�cie�ka dost�pu ) "log.txt" */
char	LOG_filename[ MAX_PATH_LENGTH ];

/*Przechowuje informacj� o typach adres�w IP: IPv4 lub IPv6 */
int		ip_proto_ver = -1;

/* Przechowuje informacj� o ilo�ci wczytanych rozszerze� dla innych typ�w plik�w */
int		cgi_other_count = 0;

/* Przechowuje informacj� o ilo�ci wczytanych nazw plik�w index */
int		index_file_count = 0;

static void		server_log_prepare( void );
static void		server_validate_paths( void );

/*
server_log_prepare()
- sprawdza, czy istnieje folder, w kt�rym przechowywany b�dzie log z dzia�ania aplikacji
- tworzy plik "log.txt" w katalogu, kt�rego nazwa jest aktualn� dat� */
static void server_log_prepare( void ) {
	char *tmp_path = malloc( MAX_PATH_LENGTH_CHAR+1 );

	/*Utworzenie �cie�ki do pliku "log.txt" */
	strncpy( tmp_path, app_path, MAX_PATH_LENGTH );
	strncat( tmp_path, LOGS_PATH, MAX_PATH_LENGTH );

	/*Weryfikacja istnienia �cie�ki do pliku "log.txt" */
	if( directory_exists( tmp_path ) == 0 ) {
		LOG_print( "Creating path: %s...\n", tmp_path );
		if( mkdir( tmp_path, 0777 ) != 0 ) {/*Utworzenie �cie�ki */
			LOG_print( "\t- Error creating path!\n" );
		}

		if( chdir( app_path ) != 0 ) {
			LOG_print( "Error: chdir().\n" );
			return;
		}
	}

	LOG_print( "\t- verified: %s.\n", tmp_path );

	/*Dodanie do utworzonej �cie�ki nazwy pliku "log.txt" */
	strncpy( LOG_filename, tmp_path, MAX_PATH_LENGTH );
	strncat( LOG_filename, "log.txt", MAX_PATH_LENGTH );

	LOG_print( "NEW SERVER SESSION.\n" );
	LOG_print( "LOG_filename: %s.\n", LOG_filename );

	free( tmp_path );
	tmp_path = NULL;
}

/*
server_validate_paths()
- sprawdza, czy istniej� wszystie foldery niezb�dne do poprawnego dzia�ania aplikacji */
static void server_validate_paths( void ) {
	char *tmp_path = malloc( MAX_PATH_LENGTH_CHAR+1 );
	int res = -1;

	LOG_print( "Starting server_validate_paths()...\n" );

	/*Przypisanie �cie�ki, z kt�rej uruchamiana jest aplikacja */
	strncpy( tmp_path, app_path, MAX_PATH_LENGTH );

	/*Dopisanie �cie�ki do pliku "log.txt", bez nazwy pliku */
	strncat( tmp_path, LOGS_PATH, MAX_PATH_LENGTH );
	if( directory_exists( tmp_path ) == 0 ) {
		LOG_print( "Creating path: %s...\n", tmp_path );
		if( mkdir( tmp_path, 0777 ) != 0 ) {/*Utworzenie �cie�ki */
			LOG_print( "\t- Error ( %d ) creating path!\n", res );
		}
		if( chdir( app_path ) != 0 ) {
			LOG_print( "Error: chdir().\n" );
			return;
		}
	}
	LOG_print( "\t- verified: %s\n", tmp_path );

	/*Patrz opis funkcji server_log_prepare() */
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
		if( sscanf( buf, "%s %s", option, index_filename ) == 2 ) {
			if( strncmp( option, "site_index", STD_BUFF_SIZE ) == 0 ) {
				/* Wczytano maksymaln� ilo�� plik�w */
				if( index_file_count == MICRO_BUFF_SIZE ) {
					LOG_print( "Reached maximum list index count.\n" );
					break;
				}

				/* Pobranie d�ugo�ci wczytanej nazwy pliku */
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
- je�li istnieje, wczytuje plik "server.cfg" i z niego pobiera konfiguracj� dla zmiennych:
+ ip_proto_ver
+ active_port */
short CORE_load_configuration( void ) {
	FILE *cfg_file;
	char network_configuration_filename[ MAX_PATH_LENGTH ];		/* Nazwa pliku konfiguracji sieci */
	char buf[ STD_BUFF_SIZE ];									/* Wczytana linia z pliku konfiguracyjnego */
	char option[ STD_BUFF_SIZE ];								/* Wczytana warto�� z buf */
	char value[ STD_BUFF_SIZE ];								/* Wczytana warto�� z buf */

	/*Przypisanie �cie�ki, z kt�rej uruchamiana jest aplikacja */
	strncpy( network_configuration_filename, app_path, MAX_PATH_LENGTH );
	/*Dopisanie nazw plik�w z konfiguracj� */
	strncat( network_configuration_filename, NETWORK_CFG_PATH, MAX_PATH_LENGTH );

	LOG_print( "Loading configuration file ( %s )...", network_configuration_filename );
	cfg_file = fopen( network_configuration_filename, "rt" );

	if( cfg_file ) {
		/* Reset zmiennych */
		ip_proto_ver = -1;
		active_port = -1;

		LOG_print( "\n\t- file opened successfully...\n" );

		while( fgets( buf, STD_BUFF_SIZE, cfg_file ) != NULL ) {
			if( sscanf( buf, "%s %s", option, value ) == 2 ) {
				if( strncmp( option, "ip_ver", STD_BUFF_SIZE) == 0 ) {
					switch ( atoi( value ) ) {
						case IPv4 : ip_proto_ver = IPv4; break;
						case IPv6 : ip_proto_ver = IPv6; break;
						default : ip_proto_ver = IPv4; break;
					}
					LOG_print( "\t- variable loaded: ip_proto_ver = %d.\n", ip_proto_ver );
				} else if( strncmp( option, "port_number", STD_BUFF_SIZE ) == 0 ) {
					active_port = atoi( value );
					LOG_print( "\t- variable loaded: active_port = %d.\n", active_port );
				} else if( strncmp( option, "document_root", STD_BUFF_SIZE ) == 0 ) {
					/* Alokacja pami�ci */
					document_root = malloc( MAX_PATH_LENGTH );
					mem_allocated( document_root, 9 );

					strncpy( document_root, value, MAX_PATH_LENGTH );
					if( directory_exists( document_root ) == 0 ) {/* Podany zas�b nie istnieje */
						LOG_print( "\t- Error: working dir is invalid.\n" );
						printf( "Error: working dir is invalid: \"%s\"\n", document_root );
						exit( EXIT_FAILURE );
					} else {
						LOG_print( "\t- working dir: %s\n", document_root );
					}
				}
			}
		}

		/* Wczytanie praw dost�p�w do zasob�w */
		if( HTACCESS_load_configuration( cfg_file ) == 0 ) {
			LOG_save();
			return 0;
		}

		/* Wczytanie listy plik�w index */
		if( CORE_load_index_names( cfg_file ) == 0 ) {
			LOG_save();
			return 0;
		}

		/* Wczytanie typ�w MIME */
		if( MIME_load_configuration( cfg_file ) == 0 ) {
			LOG_save();
			return 0;
		}

		LOG_print( "Configuration file loaded successfully.\n" );

		/* Zamkni�cie pliku konfiguracji */
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
- zast�puje funkcj� main()
- inicjuje zmienne globalne:
+ app_path
- uruchamia procedury konfiguracyjne:
+ CORE_load_configuration()
- uruchamia obs�ug� danych:
+ CORE_start() */
void CORE_initialize( void ) {
	/* Pobranie �cie�ki startowej aplikacji */
	strncpy( app_path, get_app_path(), MAX_PATH_LENGTH );

	/*Patrz opis funkcji server_validate_paths() */
	( void )server_validate_paths();

	LOG_print( "Application start path:\n%s\n", app_path );

	/*Patrz opisy poszczeg�lnych funkcji */
	if( CORE_load_configuration() == 0 ) {
		LOG_print( "Error: unable to load configuration.\n" );
		printf( "Error: unable to load configuration.\n" );
		LOG_save();
		exit( EXIT_FAILURE );
	}

	/* Zapisanie informacji o serwerze do log.txt */
	LOG_save();

	atexit( SOCKET_stop );

	/* Prze��czenie si� na folder, z kt�rego b�d� udost�pniane zasoby */
	strncpy( app_path, document_root, MAX_PATH_LENGTH );

	/* Uruchomienie sieci */
	SOCKET_main();
}

