/*******************************************************************

Projekt battery_Server

Plik:
server_cgi_manager.c

Przeznaczenie:
Uruchamia skrypty CGI i odczytuje wyniki ich dzia�a�

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_shared.h"
#include "include/server_cgi_manager.h"
#include "include/server_files_io.h"
#include "include/server_http_protocol.h"
#include "include/server_create_session.h"
#include "include/server_portable.h"
#include "include/server_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <string.h>

static void		cgi_set_env_variables( HTTP_SESSION *http_session, const char *filename );
static void		cgi_set_env( const char *var, const char *val );

/*
CGI_execute( HTTP_SESSION *http_session, const char *filename )
@http_session - wska�nik do pod��czonego klienta
@filename - nazwa pliku skryptu CGI pobranego z ��dania
- funkcja wykonuje ��dany skrypt i wysy�a odpowied� do pod��czonego klienta. */
void CGI_execute( HTTP_SESSION *http_session, const char *filename ) {
	FILE *cgi_script_file;
	long result_size = 0;		/* Ilo�� wczytanych bajt�w z wyniku dzia�ania CGI */
	unsigned char *cgi_result;	/* Wynik dzia�ania CGI */
	unsigned char *cgi_body;	/* Wynik dzia�ania CGI bez nag��wk�w */
	char buf[MAX_BUFFER];		/* Wczytany wynik dzia�ania CGI */
	char *cgi_script_exec;		/* Nazwa pliku wykonywalnego CGI ( g��wny ) */
	char *exec_filename;		/* Nazwa pliku wykonywalnego CGI */
	char *cgi_param;			/* Parametr, z kt�rym ma zosta� uruchomiony plik wykonywalny CGI */
	char *add_hdr;				/* Uzyskane nag��wki z dzia�ania skryptu CGI */
	int valid_res = -1;			/* Przechowuje rezultat dzia�ania funkcji CGI_valid */
	long hdr_len = 0;			/* Rozmiar wczytanych nag��wk�w */
	int i = 0, j = 0;			/* Zmienne pomocnicze dla p�tli */

	/* Alokacja pami�ci na obiekty, na kt�rych b�dzie grzeba� funkcja CGI_valid */
	exec_filename = ( char* )malloc( MAX_PATH_LENGTH_CHAR );
	mem_allocated( exec_filename, 39 );

	cgi_param = ( char* )malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( cgi_param, 40 );

	add_hdr = ( char* )calloc( BIG_BUFF_SIZE+1, sizeof( char ) );
	mem_allocated( add_hdr, 49 );

	/* Sprawdzenie typu pliku CGI i pobranie ew. nazwy programu zewn�trznego i parametru
	( patrz definicja funkcji CGI_valid() ) */
	CGI_valid( filename, &valid_res, exec_filename, cgi_param );

	/* Plik nie jest skryptem CGI */
	if( valid_res == 0 ) {
		free( exec_filename );
		exec_filename = NULL;

		free( cgi_param );
		cgi_param = NULL;

		free( add_hdr );
		add_hdr = NULL;

		RESPONSE_error( http_session, HTTP_403_FORBIDDEN, HTTP_ERR_403_MSG, NULL );
		return;
	}

	/* Alokacja pami�ci na obiekt, kt�ry b�dzie zawiera� nazw� zewn�trznego pliku
	wykonywalnego i ewentualnie parametr, z kt�rym ma zosta� uruchomiony */
	cgi_script_exec = ( char* )malloc( MAX_PATH_LENGTH_CHAR+1 );
	mem_allocated( cgi_script_exec, 41 );

	/* Weryfikacja, czy ��dany plik ( filename ) jest wykonywalny sam z siebie, czy
	trzeba u�y� zewn�trznej aplikacji */
	if( valid_res == 1 ) {
		/* Plik jest wykonywalny sam z siebie */
		strncpy( cgi_script_exec, filename, MAX_PATH_LENGTH );
	} else if( valid_res == 2 ) {
		/* Plik wymaga zewn�trznej aplikacji */
		strncpy( cgi_script_exec, exec_filename, MAX_PATH_LENGTH );
		strncat( cgi_script_exec, " ", MAX_PATH_LENGTH );
		strncat( cgi_script_exec, filename, MAX_PATH_LENGTH );
	} else if( valid_res == 3 ) {
		/* Plik wymaga zewn�trznej aplikacji z parametrem */
		strncpy( cgi_script_exec, exec_filename, MAX_PATH_LENGTH );
		strncat( cgi_script_exec, " ", MAX_PATH_LENGTH );
		strncat( cgi_script_exec, filename, MAX_PATH_LENGTH );
		strncat( cgi_script_exec, " ", MAX_PATH_LENGTH );
		strncat( cgi_script_exec, cgi_param, MAX_PATH_LENGTH );
	}

	/* Ustawienie zmiennych systemowych */
	cgi_set_env_variables( http_session, filename );

	/* Stworzenie potoku ze skryptem CGI. Weryfikacja poprawno�ci nazwy nast�pi�a w funkcji
	CGI_valid. Odczyt danych w trybie binarnym. */
	cgi_script_file = popen( cgi_script_exec, READ_BINARY );

	if( !cgi_script_file ) {
		/* Zwolnienie pami�ci */
		free( cgi_script_exec );
		cgi_script_exec = NULL;

		free( exec_filename );
		exec_filename = NULL;

		free( cgi_param );
		cgi_param = NULL;

		/* Problem - plik zosta� wcze�niej odnaleziony w REQUEST_process, ale teraz si� go nie da otworzy� */
		RESPONSE_error( http_session, HTTP_500_SERVER_ERROR, HTTP_ERR_500_MSG, NULL );
		return;
	}

	/* Rezerwacja pami�ci */
	cgi_result = ( unsigned char* )calloc( MAX_BUFFER, sizeof( char ) );
	mem_allocated( ( char* )cgi_result, 17 );

	cgi_body = ( unsigned char* )calloc( MAX_BUFFER, sizeof( char ) );
	mem_allocated( ( char* )cgi_body, 220 );

	/* Wczytanie wyniku dzia�ania skryptu CGI i pobranie rozmiaru danych */
	result_size = fread( buf, sizeof( char ), MAX_BUFFER, cgi_script_file );

	/* Zamkni�cie potoku */
	pclose( cgi_script_file );
	cgi_script_file = NULL;

	/* Skopiowanie wczytanego wyniku z buf do cgi_result */
	for( i = 0, j = 0; i < result_size; ++i, ++j ) {
		cgi_result[j] = buf[i];
	}

	strncpy( add_hdr, REQUEST_get_message_header( buf, result_size ), BIG_BUFF_SIZE );

	hdr_len = strlen( add_hdr );

	/* Wczytanie contentu */
	/* U�yty modyfikator 2 ( poni�ej i w RESPONSE_header ) = "\r\n" = pusta linia mi�dzy nag��wkami a contentem */
	for( i = hdr_len+2, j = 0; i < result_size-1; i++, j++ ) {
		cgi_body[j] = cgi_result[i];
	}

	/* Usuni�cie ostatniego znaku */
	cgi_body[result_size-hdr_len-1] = '\0';

	/* Pusta tre�� dzia�ania skryptu */
	if( strlen( ( char* )cgi_result ) == 0 ) {
		RESPONSE_error( http_session, HTTP_204_NO_CONTENT, HTTP_ERR_204_MSG, NULL );
	} else {
		/*Wysy�ka */
		RESPONSE_header( http_session, HTTP_200_OK, NULL, result_size-hdr_len-2, ( char* )cgi_body, add_hdr );
	}

	/* Zwalnianie pami�ci */
	free( cgi_result );
	cgi_result = NULL;

	free( cgi_body );
	cgi_body = NULL;

	free( exec_filename );
	exec_filename = NULL;

	free( cgi_param );
	cgi_param = NULL;

	free( add_hdr );
	add_hdr = NULL;

	free( cgi_script_exec );
	cgi_script_exec = NULL;
}

static void cgi_set_env( const char *var, const char *val ) {
#ifdef _WIN32
	char env_var[MAX_BUFFER];
	sprintf( env_var, "%s=%s", var, val );
	putenv( env_var );
#else
	( val == NULL ) ?	unsetenv( var ) : setenv( var, val, 1 );
#endif
}

/*
cgi_set_env_variables( HTTP_SESSION *http_session, const char *filename )
@http_session - wska�nik do pod��czonego klienta
@filename - pe�na nazwa pliku skryptu
- funkcja ustawia zmienne �rodowiskowe, zgodnie ze specyfikacj� CGI/1.1 ( RFC3875 )*/
static void cgi_set_env_variables( HTTP_SESSION *http_session, const char *filename ) {
	char str_res[SMALL_BUFF_SIZE];

	/* "Content-Length" i "Content-Type" s� nieobecne w przypadku metody GET i HEAD */
	if( http_session->http_info.method_name == POST ) {
		sprintf( str_res, "%ld", http_session->http_info.content_length );
		cgi_set_env( "CONTENT_LENGTH", str_res );
		cgi_set_env( "CONTENT_TYPE", http_session->http_info.content_type );
	} else {
		/* Zerowanie zmiennych systemowych */
		cgi_set_env( "CONTENT_LENGTH", NULL );
		cgi_set_env( "CONTENT_TYPE", NULL );
	}

	cgi_set_env( "GATEWAY_INTERFACE", CGI_VER );

	/*sprintf( env_var,"PATH_INFO=%s", path_in );
	putenv( env_var );*/

	cgi_set_env( "SCRIPT_FILENAME", filename );

	cgi_set_env( "QUERY_STRING", http_session->http_info.query_string );

	cgi_set_env( "REMOTE_ADDR", http_session->http_info.remote_addr );

	cgi_set_env( "AUTH_TYPE", http_session->http_info.authorization == NULL ? NULL:"Basic" );

	cgi_set_env( "REMOTE_USER", http_session->http_info.user_login == NULL ? NULL:http_session->http_info.user_login );

	cgi_set_env( "REMOTE_IDENT", http_session->http_info.user_pwd == NULL ? NULL:http_session->http_info.user_pwd );

	cgi_set_env( "REMOTE_HOST", server_get_remote_hostname( http_session ) );

	cgi_set_env( "REQUEST_METHOD", http_method_list[http_session->http_info.method_name] );

	cgi_set_env( "SCRIPT_NAME", http_session->http_info.http_local_path );

	cgi_set_env( "SERVER_NAME", APP_NAME );

	sprintf( str_res, "%d", active_port );
	cgi_set_env( "SERVER_PORT", str_res );

	cgi_set_env( "SERVER_PROTOCOL", http_session->http_info.protocol_ver );

	cgi_set_env( "SERVER_SOFTWARE", SERVER_NAME );

	cgi_set_env( "REDIRECT_STATUS", "200" );

}

/*
CGI_load_configuration( void )*/
int CGI_load_configuration( const char *filename ) {
	FILE *cfg_file;
	char *buf;				/* Wczytana linia z pliku konfiguracyjnego */
	char *ext;				/* Wczytane rozszerzenie pliku z buf */
	char *exec_name;		/* Wczytana nazwa pliku wykonywalnego z buf */
	char *param;			/* Wczytany parametr dla pliku wykonywalnego z buf */
	struct stat file_stat;	/* Dla sprawdzenia w�a�ciwo�ci pliku */
	int lines_count = 1;

	LOG_print( "Loading CGI script configuration..." );
	cfg_file = fopen( filename, "rt" );

	if( !cfg_file ) {
		LOG_print( "error.\n" );
		return 0;
	} else {
		LOG_print( "ok.\n" );
	}

	/* Alokacja pami�ci */
	buf = ( char* )malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( buf, 450 );

	ext = ( char* )malloc( EXT_LEN_CHAR );
	mem_allocated( ext, 451 );

	exec_name = ( char* )malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( exec_name, 452 );

	param = ( char* )malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( param, 453 );

	while( fgets( buf, STD_BUFF_SIZE, cfg_file ) ) {
		if( ( sscanf( buf, "%s %s %s", ext, exec_name, param ) == 3 ) && ( cgi_other_count < 8 ) ) {
			/* ��dana opcja jest w pliku pod numerem 2 */
			strncpy( other_script_type[cgi_other_count].ext, ext, EXT_LEN );
			strncpy( other_script_type[cgi_other_count].external_app, exec_name, STD_BUFF_SIZE );
			strncpy( other_script_type[cgi_other_count].param, param, STD_BUFF_SIZE );

			/* Weryfikacja, czy plik pod zmienn� exec_name jest wykonywalny */
			/* Je�eli exec_name = <exec> plik sam z siebie jest plikiem wykonywalnym */
			if( strncmp( exec_name, STD_EXEC, STD_BUFF_SIZE ) != 0 ) {
				/* Sprawdzenie, czy plik istnieje */
				if( file_params( NULL, exec_name, NULL ) == 0 ) {
					LOG_print( "Error: file not found ( %s ).\n", file_get_name( exec_name ) );
					continue;
				}

				/* Pobranie w�a�ciwo�ci pliku */
				stat( exec_name, &file_stat );
				if( file_stat.st_mode & S_IXOTH ) {
					LOG_print( "\t- %d: \"%s\" assigned to \"%s\" with param \"%s\".\n", cgi_other_count, ext, exec_name, param );
				} else {
					LOG_print( "%s is not executable.\n", exec_name );
					continue; /* Nie jest... */
				}

			}
			else {
				LOG_print( "\t- %d: \"%s\" assigned to \"%s\" with param \"%s\".\n", cgi_other_count, ext, exec_name, param );
			}

			/* Zwi�kszenie licznika wczytanych rozszerze� */
			cgi_other_count++;
		} else {
			LOG_print( "\t- unknown command at line: %d\n", lines_count );
		}
		lines_count++;
	}

	/* Zamkni�cie pliku konfiguracyjnego*/
	fclose( cfg_file );

	/* Zwalnianie pami�ci */
	free( buf );
	buf = NULL;

	free( ext );
	ext = NULL;

	free( exec_name );
	exec_name = NULL;

	free( param );
	param = NULL;

	return 1;
}

/*
CGI_valid( const char *filename )
@filename - nazwa pliku, kt�ry ma by� zweryfikowany jako skrypt CGI
@valid_res - wska�nik do int z funkcji wywo�uj�cej. Zwraca:
+ 0 - plik nie jest skryptem CGI
+ 1 - plik jest skryptem CGI i sam w sobie jest wykonywalny
+ 2 - plik jest skryptem CGI, ale potrzebuje zewn�trznej aplikacji
+ 3 - plik jest skryptem CGI, ale potrzebuje zewn�trznej aplikacji uruchamianej z parametrem
@exec_name - wska�nik do char z funkcji wywo�uj�cej. Zwraca nazw� zewn�trznego programu wywo�uj�cego skrypt.
@param - wska�nik do char z funkcji wywo�uj�cej. Zwraca parametr, z kt�rym ma zosta� wywo�any zewn�trzny program. */
void CGI_valid( const char *filename, int *valid_res, char *exec_name, char *param ) {
	char *ext;
	int i = 0;

	*valid_res = 0;

	ext = ( char* )malloc( EXT_LEN_CHAR );
	mem_allocated( ext, 37 );

	strncpy( ext, file_get_ext( filename ), EXT_LEN );

	/* Szukanie rozszerzenia w tablicy other_script_type */
	for( i = 0; i < cgi_other_count; ++i ) {
		/* Znaleziono pasuj�ce rozszerzenie */
		if( strncasecmp( ext, other_script_type[i].ext, EXT_LEN ) == 0 ) {
			free( ext );
			ext = NULL;

			/* Przypisanie warto�ci do wywo�anych argument�w:
				- nazwa zewn�trznej aplikacji */
			if( exec_name ) {
				strncpy( exec_name, other_script_type[i].external_app, MAX_PATH_LENGTH );
			}

			if( strncmp( other_script_type[i].external_app, STD_EXEC, STD_BUFF_SIZE ) == 0 ) {
				*valid_res = 1; /* Skrypt jest wykonywalny sam w sobie ( np. plik exe ) */
			} else {
				if( strncmp( other_script_type[i].param, STD_EXEC, STD_BUFF_SIZE ) == 0 ) {
					*valid_res = 2; /* Potrzeba uruchomienia zewn�trznego programu */
				} else {
					*valid_res = 3; /* Potrzeba uruchomienia zewn�trznego programu z parametrem */
					/*	- parametr */
					strncpy( param, other_script_type[i].param, STD_BUFF_SIZE );
				}
			}

			free( ext );
			ext = NULL;
			return;
		}
	}

	free( ext );
	ext = NULL;
}
