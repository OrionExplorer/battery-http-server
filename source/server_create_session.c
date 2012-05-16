/*******************************************************************

Projekt battery_Server

Plik: server_create_session.c

Przeznaczenie:
Interpretacja danych otrzymanych od klienta
Przekazanie danych do funkcji wykonuj�cej ��danie

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_create_session.h"
#include "include/server_socket_io.h"
#include "include/server_http_protocol.h"
#include "include/server_files_io.h"
#include "include/server_log.h"
#include "include/server_base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void		SESSION_get_ip_addr( HTTP_SESSION *http_session );
static void		SESSION_reset( HTTP_SESSION *http_session );
static void		SESSION_get_connection( HTTP_SESSION *http_session );
static void		SESSION_get_if_modified_since( HTTP_SESSION *http_session );
static void		SESSION_get_if_unmodified_since( HTTP_SESSION *http_session );
static void		SESSION_get_res_range( HTTP_SESSION *http_session );
static void		SESSION_get_content_type( HTTP_SESSION *http_session );
static void		SESSION_check_auth( HTTP_SESSION *http_session );
static void		SESSION_get_http_header( HTTP_SESSION *http_session );
short			SESSION_local_path_is_valid( HTTP_SESSION *http_session );
short			SESSION_http_protocol_is_valid( HTTP_SESSION *http_session );
short			SESSION_check_connections_limit( HTTP_SESSION *http_session );

HTTP_SESSION			*sessions[ MAX_CLIENTS ];
int						send_d_count;

/*
SESSION_http_protocol_is_valid( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- weryfikuje protok�, kt�rego u�ywa pod��czony klient. */
short SESSION_http_protocol_is_valid( HTTP_SESSION *http_session ) {
	if( strncmp( http_session->http_info.protocol_ver, HTTP_VER, PROTO_BUFF_SIZE ) == 0 ) {
		/* HTTP/1.1: Sprawdzenie, czy jest nag��wek "Host:" */
		if( strstr( http_session->http_info.header, HEADER_HOST ) == 0 ) {
			/* brak... */
			RESPONSE_error( http_session, HTTP_400_BAD_REQUEST, HTTP_ERR_400_MSG, NULL );
			SOCKET_disconnect_client( http_session );
			SESSION_release( http_session );
			return 0;
		}
	} else if( ( strcmp( http_session->http_info.protocol_ver, HTTP_VER ) != 0 ) && /* Sprawdzenie, czy protok� jest obs�ugiwany */
			( strcmp( http_session->http_info.protocol_ver, HTTP_VER_1_0 ) != 0 ) ) {

		/* Protok� w wersji innej ni� HTTP/1.0 i HTTP/1.1 - b��d 505 */
		RESPONSE_error( http_session, HTTP_505_HTTP_VERSION_NOT_SUPPORTED, HTTP_ERR_505_MSG, NULL );
		SOCKET_disconnect_client( http_session );
		SESSION_release( http_session );
		return 0;
	}

	return 1;
}

/*
SESSION_local_path_is_valid( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- weryfikuje poprawnos� ��danej scie�ki do zasobu. */
short SESSION_local_path_is_valid( HTTP_SESSION *http_session ) {
	char *tmp_local_file_path;		/* Przechowuje rzeczywist� �cie�k� do pliku na dysku */

	/* Weryfikacja d�ugo�ci ��danej �cie�ki */
	if( strlen( http_session->http_info.http_local_path ) > MAX_URI_LENGTH ) {
		LOG_print( "URI too long.\n" );
		RESPONSE_error( http_session, HTTP_414_REQUEST_URI_TOO_LONG, HTTP_ERR_414_MSG, NULL );
		SOCKET_disconnect_client( http_session );
		SESSION_release( http_session );
		return 0;
	}

	/* Je�eli podano sam� nazw� katalogu to automatycznie dodajemy plik indeksu */
	if( strncmp( file_get_name( http_session->http_info.http_local_path ), "", 1 ) == 0 ) {
		tmp_local_file_path = ( char* )malloc( MAX_PATH_LENGTH_CHAR+1 );
		mem_allocated( tmp_local_file_path, 9003 );

		strncpy( tmp_local_file_path, app_path, MAX_PATH_LENGTH );
		strncat( tmp_local_file_path, http_session->http_info.http_local_path, MAX_PATH_LENGTH );
		strrepchar( tmp_local_file_path, '/', C_SLASH );
		strdelbslash( tmp_local_file_path );
		strncat( http_session->http_info.http_local_path, REQUEST_get_index( tmp_local_file_path ), MAX_PATH_LENGTH );

		free( tmp_local_file_path );
		tmp_local_file_path = NULL;
	}

	/* Sprawdzenie, czy w ��danej �cie�ce zawieraj� si� znaki ".." lub "/." */
	if( ( strstr( http_session->http_info.http_local_path, ".." ) ) || ( strstr( http_session->http_info.http_local_path, "/." ) ) ) {
		RESPONSE_error( http_session, HTTP_400_BAD_REQUEST, HTTP_ERR_400_MSG, NULL );
		SOCKET_disconnect_client( http_session );
		SESSION_release( http_session );
		return 0;
	}

	return 1;
}

/*
SESSION_get_http_header( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- pobiera nag��wek wiadomosci HTTP. */
static void SESSION_get_http_header( HTTP_SESSION *http_session ) {
	http_session->http_info.header = ( char* )malloc( BIG_BUFF_SIZE_CHAR );
	mem_allocated( http_session->http_info.header, 0 );
	strncpy( http_session->http_info.header, REQUEST_get_message_header( http_session->http_info.content_data, http_session->address_length ), BIG_BUFF_SIZE );
}

/*
SESSION_reset( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- zeruje zmienne liczbowe ze struktury HTTP_SESSION. */
static void SESSION_reset( HTTP_SESSION *http_session ) {
	/* Wyzerowanie zmiennych odpowiedzialnych za zakres wysy�anych danych z pliku */
	http_session->http_info.range_st = -1;
	http_session->http_info.range_en = -1;
	/* Wyzerowanie zmiennej odpowiedzialnej za informowanie, czy zapytanie jest ��daniem zasobu, czy wykonaniem skryptu CGI */
	http_session->http_info.is_cgi = 0;
}

/*
 SESSION_check_auth( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- pobiera dane autoryzacyjne do zasobu od klienta. */
static void SESSION_check_auth( HTTP_SESSION *http_session ) {
	char *user_auth_enc;			/* Przechowuje odszyfrowane login i has�o */

	if( strstr( http_session->http_info.header, HEADER_AUTHORIZATION ) ) {
		http_session->http_info.authorization = ( char* )malloc( STD_BUFF_SIZE_CHAR );
		strncpy( http_session->http_info.authorization, REQUEST_get_header_value( http_session->http_info.header, HEADER_AUTHORIZATION ), STD_BUFF_SIZE );
		/* Kodowanie metod� Digest jest nieobs�ugiwane */
		if( strstr( " Digest ", http_session->http_info.authorization ) ) {
			RESPONSE_error( http_session, HTTP_400_BAD_REQUEST, HTTP_ERR_400_MSG, NULL );
			SESSION_release( http_session );
			return;
		} else {
			strncpy( http_session->http_info.authorization, strchr( http_session->http_info.authorization, ' ' )+1, STD_BUFF_SIZE );
			/* Odszyfrowanie danych */
			user_auth_enc = ( char* )calloc( STD_BUFF_SIZE, sizeof( char ) );
			base64_decode( http_session->http_info.authorization, ( unsigned char* )user_auth_enc, STD_BUFF_SIZE );
			/* Pobranie nazwy u�ytkownika */
			http_session->http_info.user_login = ( char* )malloc( SMALL_BUFF_SIZE_CHAR );
			strncpy( http_session->http_info.user_login, strtok( user_auth_enc, ":" ), SMALL_BUFF_SIZE );
			/* Pobranie has�a */
			http_session->http_info.user_pwd = ( char* )malloc( SMALL_BUFF_SIZE_CHAR );
			strncpy( http_session->http_info.user_pwd, strtok( NULL, ":" ), SMALL_BUFF_SIZE );

			free( user_auth_enc );
			user_auth_enc = NULL;
		}
	}
}

/*
SESSION_get_content_type( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- pobiera informacj� o nag��wku "Content-Type". */
static void SESSION_get_content_type( HTTP_SESSION *http_session ) {
	if( strstr( http_session->http_info.header, HEADER_CONTENT_TYPE ) ) {
		http_session->http_info.content_type = ( char* )malloc( STD_BUFF_SIZE_CHAR );
		strncpy( http_session->http_info.content_type, REQUEST_get_header_value( http_session->http_info.header, HEADER_CONTENT_TYPE ), STD_BUFF_SIZE );
	}
}

/*
SESSION_get_res_range( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- pobiera informacj� o ��danym przez klienta fragmencie zasobu. */
static void SESSION_get_res_range( HTTP_SESSION *http_session ) {
	if( strstr( http_session->http_info.header, HEADER_RANGE ) ) {
		http_session->http_info.range_st = REQUEST_get_range( http_session, 0 );
		http_session->http_info.range_en = REQUEST_get_range( http_session, 1 );
	}
}

/*
SESSION_get_if_unmodified_since( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- pobiera informacje o nag��wku "If-Unmodified-Since". */
static void SESSION_get_if_unmodified_since( HTTP_SESSION *http_session ) {
	if( strstr( http_session->http_info.header, HEADER_IF_UNMODIFIED_SINCE ) ) {
		http_session->http_info.date_if_unmodified_since = ( char* )malloc( TIME_BUFF_SIZE_CHAR );
		strncpy( http_session->http_info.date_if_unmodified_since, REQUEST_get_header_value( http_session->http_info.header, HEADER_IF_UNMODIFIED_SINCE ), TIME_BUFF_SIZE );
	}
}

/*
SESSION_get_if_modified_since( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- pobiera informacje o nag��wku "If-Modified-Since". */
static void SESSION_get_if_modified_since( HTTP_SESSION *http_session ) {
	if( strstr( http_session->http_info.header, HEADER_IF_MODIFIED_SINCE ) ) {
		http_session->http_info.date_if_modified_since = ( char* )malloc( TIME_BUFF_SIZE_CHAR );
		strncpy( http_session->http_info.date_if_modified_since, REQUEST_get_header_value( http_session->http_info.header, HEADER_IF_MODIFIED_SINCE ), TIME_BUFF_SIZE );
	}
}

/*
SESSION_get_connection( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- pobiera informacj� o nag��wku "Connection". */
static void SESSION_get_connection( HTTP_SESSION *http_session ) {
	char *temp_conn_type_handle;	/* Do wczytania informacji o rodzaju po��czenia */

	if( strstr( http_session->http_info.header, HEADER_CONNECTION ) ) {
		/* Pobranie informacji o po��czeniu ( "Connection: Keep-Alive/Close" ) - przypisanie do zmiennej tymczasowej */
		temp_conn_type_handle = ( char* )malloc( SMALL_BUFF_SIZE_CHAR );
		mem_allocated( temp_conn_type_handle, 9002 );
		strncpy( temp_conn_type_handle, REQUEST_get_header_value( http_session->http_info.header, HEADER_CONNECTION ), SMALL_BUFF_SIZE );
		/* Ustawienie zmiennej keep-alive w zale�no�ci od warto�ci zmiennej temp_conn_type_handle */
		if( strncasecmp( temp_conn_type_handle, HEADER_KEEP_ALIVE_STR, SMALL_BUFF_SIZE ) == 0 ) {
			http_session->http_info.keep_alive = 1;
		} else {
			http_session->http_info.keep_alive = 0;
		}

		free( temp_conn_type_handle );
		temp_conn_type_handle = NULL;

	} else {
		/* Brak - po��czenie zamkni�te */
		http_session->http_info.keep_alive = 0;
	}
}

/*
SESSION_check_connections_limit( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- weryfikuje ilos� pod��czonych klient�w */
short SESSION_check_connections_limit( HTTP_SESSION *http_session ) {
	/* Sprawdzenie ilo�ci pod��czonych klient�w. Je�eli jest max = b��d 503 */
	if( http_conn_count == MAX_CLIENTS ) {
		RESPONSE_error( http_session, HTTP_503_SERVICE_UNAVAILABLE, HTTP_ERR_503_MSG, NULL );
		SOCKET_disconnect_client( http_session );
		SESSION_release( http_session );
		return 0;
	}

	return 1;
}

/*
SESSION_get_ip_addr( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- pobiera adres IP pod��czonego klienta. */
static void SESSION_get_ip_addr( HTTP_SESSION *http_session ) {
	http_session->http_info.remote_addr = ( char* )malloc( TINY_BUFF_SIZE_CHAR );
	mem_allocated( http_session->http_info.remote_addr, 6 );
	strncpy( http_session->http_info.remote_addr, SOCKET_get_remote_ip( http_session ), TINY_BUFF_SIZE );
}

/*
SESSION_prepare( HTTP_SESSION *http_session, const char *content_data )
@http_session - wska�nik do danych pod��czonego klienta
- przeprowadza weryfikacj� przes�anych danych:
+ metoda ( obs�ugiwane: GET, HEAD, POST )
+ d�ugo�� adresu URI
+ obecno�� nag��wka "Host"
+ typ po��czenia ( close/keep-alive )
+ obecno�� nag��wka "If-Modified-Since"
+ obecno�� nag��wka "If-Unmodified-Since"
+ obecno�� nag��wka "Content-Length"
+ obecno�� nag��wka "Content-Type"
+ obecno�� nag��wka "Range"
+ po pomy�lnej weryfikacji struktury przekazuje dane do wykonania przez funkcj� REQUEST_process */
void SESSION_prepare( HTTP_SESSION *http_session ) {
	char *temp_http_method_name;	/* Do wczytania ��danej metody */
	char *temp_entire_msg;			/* Do przechowania ca�ej wiadomo�ci. Tre�� b�dzie traktowana funkcj� strtok, a oryginaln� zawarto�� zmiennej http_info.content_data chcemy zatrzyma� */
	char *tmp_post_data;			/* Do przechowania POST data */

	/* Tutaj jest zabezpieczenie przed otrzymaniem metody POST w cz�ciach.
	Zmienna received_all jest ustawiana na 0 przy sprawdzaniu "Content-Length",
	kiedy pobrana zawarto�� ( content ) ma rozmiar mniejszy ni� warto�� tego nag��wka.
	Wtedy podczas odbioru danych z socketa otrzymane dane zostaj� sklejone z http_info.content_data
	i dopiero komunikat jest procesowany. received_all ma warto�� -1, je�eli metod� jest GET lub HEAD.*/
	if( http_session->http_info.received_all == 1 ) {
		/* Reset zmiennej odpowiedzialnej za odbi�r kolejnej cz�ci komunikatu */
		http_session->http_info.received_all = -1;

		/* Przej�cie do wywo�ania funkcji procesuj�cej ��danie i zarz�dzanie roz��czeniem klienta */
		REQUEST_process( http_session );

		return;
	}

	/* Wyzerowanie zmiennych liczbowych */
	SESSION_reset( http_session );

	/*Pobranie adresu IP */
	SESSION_get_ip_addr( http_session );

	/*Pobranie nag��wka wiadomo�ci */
	SESSION_get_http_header( http_session );

	/* Pobranie pierwszej linijki ��dania */
	http_session->http_info.request_line = ( char* )malloc( BIG_BUFF_SIZE_CHAR );
	mem_allocated( http_session->http_info.request_line, 1 );
	/* Zmienna temp_entire_msg b�dzie przechowywa� zawarto�� http_info.content_data na potrzeby funkcji strtok */
	temp_entire_msg = ( char* )malloc( MAX_PATH_LENGTH+TINY_BUFF_SIZE );
	strncpy( temp_entire_msg, http_session->http_info.content_data, MAX_PATH_LENGTH+TINY_BUFF_SIZE );
	strncpy( http_session->http_info.request_line, strtok( temp_entire_msg, "\015\012" ), MAX_PATH_LENGTH+TINY_BUFF_SIZE );

	/* Pobranie ��danej metody z request_line */
	temp_http_method_name = ( char* )malloc( MICRO_BUFF_SIZE_CHAR );
	mem_allocated( temp_http_method_name, 9001 );
	strncpy( temp_entire_msg, http_session->http_info.request_line, MAX_PATH_LENGTH+TINY_BUFF_SIZE );
	strncpy( temp_http_method_name, strtok( temp_entire_msg, " " ), MICRO_BUFF_SIZE );

	/*Przypisanie numeru metody na podstawie tablicy http_method_list i http_methods */
	if( strncmp( temp_http_method_name, "GET", MICRO_BUFF_SIZE ) == 0 ) {
		http_session->http_info.method_name = GET;
	} else if( strncmp( temp_http_method_name, "HEAD", MICRO_BUFF_SIZE ) == 0 ) {
		http_session->http_info.method_name = HEAD;
	} else if( strncmp( temp_http_method_name, "POST", MICRO_BUFF_SIZE ) == 0 ) {
		http_session->http_info.method_name = POST;
	} else {
		/* Metoda nieobs�ugiwana - papa... */
		RESPONSE_error( http_session, HTTP_501_NOT_SUPPORTED, HTTP_ERR_501_MSG, NULL );
		SESSION_release( http_session );
		free( temp_http_method_name );
		temp_http_method_name = NULL;
		return;
	}

	free( temp_http_method_name );
	temp_http_method_name = NULL;

	/* Pami�� na �cie�k� lokaln� */
	http_session->http_info.http_local_path = ( char* )malloc( MAX_PATH_LENGTH_CHAR );
	mem_allocated( http_session->http_info.http_local_path, 2 );
	/* Pobranie �cie�ki do pliku */
	strncpy( http_session->http_info.http_local_path, strtok( NULL, " " ), MAX_PATH_LENGTH );

	/* Weryfikacja scie�ki do ��danego zasobu */
	if( SESSION_local_path_is_valid( http_session ) == 0 ) {
		return;
	}

	/* Pami�� na wersj� protoko�u HTTP */
	http_session->http_info.protocol_ver = ( char* )malloc( PROTO_BUFF_SIZE_CHAR );
	mem_allocated( http_session->http_info.protocol_ver, 3 );
	/* Pobranie wersji protoko�u HTTP */
	strncpy( http_session->http_info.protocol_ver, strtok( NULL, "\015" ), PROTO_BUFF_SIZE );

	free( temp_entire_msg );
	temp_entire_msg = NULL;

	/* Sprawdzenie wersji protoko�u przegl�darki */
	if( SESSION_http_protocol_is_valid( http_session ) == 0 ) {
		return;
	}

	/* Sprawdzenie ilosci pod��czonych klient�w. W razie maksymalnej iloci przestaje procesowa� ��danie i wysy�a informacj� do klienta */
	if( SESSION_check_connections_limit( http_session ) == 0 ) {
		return;
	}

	/* Sprawdzenie, czy jest nag��wek "Connection:" */
	SESSION_get_connection( http_session );

	/* Pobranie informacji o parametrze "If-Modified-Since" */
	SESSION_get_if_modified_since( http_session );

	/* Pobranie informacji o parametrze "If-Unmodified-Since" */
	SESSION_get_if_unmodified_since( http_session );

	/* Pobranie informacji o parametrze "Range:" */
	SESSION_get_res_range( http_session );

	/* Pobranie informacji o parametrze "Content-Type" */
	SESSION_get_content_type( http_session );

	/* Pobranie informacji o nag��wku "Authorization" */
	SESSION_check_auth( http_session );

	/* Pobranie informacji o parametrze "Content-Length" */
	if( strstr( http_session->http_info.header, HEADER_CONTENT_LENGTH ) ) {
		/* Przypisanie zmiennej http_info.content_length zawarto�ci nag��wka "Content-Length" */
		http_session->http_info.content_length = atoi( REQUEST_get_header_value( http_session->http_info.header, HEADER_CONTENT_LENGTH ) );
		/* Pr�ba pobrania zawarto�ci ��dania do zmiennej tymczasowej */
		tmp_post_data = ( char* )malloc( MAX_BUFFER_CHAR );
		mem_allocated( tmp_post_data, 9002 );
		strncpy( tmp_post_data, REQUEST_get_message_body( http_session ), MAX_BUFFER );
		/* Sprawdzenie, czy rozmiar pobranej zawarto�ci jest zgodny z danymi z nag��wka "Content-Length" */
		if( strlen( tmp_post_data ) < http_session->http_info.content_length ) {
			/* Nie odebrano ca�ego ��dania, przekazanie informacji do funkcji odbieraj�cej dane
			o tym, �e kolejna porcja danych z socketa nie b�dzie now� sesj�, tylko doko�czeniem starej */
			http_session->http_info.received_all = 0;

			free( tmp_post_data );
			tmp_post_data = NULL;
			return;
		} else {
			http_session->http_info.received_all = -1;
		}

		/* Odebrano wszystko za jednym razem;
		przypisanie do zmiennej http_info.query_string zawarto�ci komunikatu */
		if( !http_session->http_info.query_string ) {
			http_session->http_info.query_string = ( char* )malloc( MAX_BUFFER_CHAR );
			mem_allocated( http_session->http_info.query_string, 15 );
		}
		strncpy( http_session->http_info.query_string, tmp_post_data, MAX_BUFFER );

		if( strlen( http_session->http_info.query_string ) == MAX_BUFFER ) {
			RESPONSE_error( http_session, HTTP_413_REQUEST_ENTITY_TOO_LARGE, HTTP_ERR_413_MSG, NULL );
			SOCKET_disconnect_client( http_session );
			SESSION_release( http_session );

			free( tmp_post_data );
			tmp_post_data = NULL;
			return;
		}

		free( tmp_post_data );
		tmp_post_data = NULL;
	} else {	/* Brak nag��wka "Content-Length", a ��dana metoda to POST = b��d 411 */
		if( http_session->http_info.method_name == POST ) {
			RESPONSE_error( http_session, HTTP_411_LENGTH_REQUIRED, HTTP_ERR_411_MSG, NULL );
			SOCKET_disconnect_client( http_session );
			SESSION_release( http_session );
			return;
		}
	}

	/* Przej�cie do wywo�ania funkcji procesuj�cej ��danie i zarz�dzanie roz��czeniem klienta */
	REQUEST_process( http_session );
}

/*
SESSION_release( HTTP_SESSION *http_session, int release_content )
@http_session - wska�nik do pod��czonego klienta
- sprawdza, czy elementy struktury HTTP_SESSION zajmuj� pami�� i w razie potrzeby zwalnia j� */
void SESSION_release( HTTP_SESSION *http_session )
{
	/* Zwalnianie ci�g�w znak�w */
	if( http_session->http_info.content_data != NULL ) {
		free( http_session->http_info.content_data );
		http_session->http_info.content_data = NULL;
	}

	if( http_session->http_info.http_local_path != NULL ) {
		free( http_session->http_info.http_local_path );
		http_session->http_info.http_local_path = NULL;
	}

	if( http_session->http_info.protocol_ver != NULL ) {
		free( http_session->http_info.protocol_ver );
		http_session->http_info.protocol_ver = NULL;
	}

	if( http_session->http_info.remote_addr != NULL ) {
		free( http_session->http_info.remote_addr );
		http_session->http_info.remote_addr = NULL;
	}

	if( http_session->http_info.remote_host != NULL ) {
		free( http_session->http_info.remote_host );
		http_session->http_info.remote_host = NULL;
	}

	if( http_session->http_info.header != NULL ) {
		free( http_session->http_info.header );
		http_session->http_info.header = NULL;
	}

	if( http_session->http_info.request_line != NULL ) {
		free( http_session->http_info.request_line );
		http_session->http_info.request_line = NULL;
	}

	if( http_session->http_info.date_if_modified_since != NULL ) {
		free( http_session->http_info.date_if_modified_since );
		http_session->http_info.date_if_modified_since = NULL;
	}

	if( http_session->http_info.date_if_unmodified_since != NULL ) {
		free( http_session->http_info.date_if_unmodified_since );
		http_session->http_info.date_if_unmodified_since = NULL;
	}

	if( http_session->http_info.query_string != NULL ) {
		free( http_session->http_info.query_string );
		http_session->http_info.query_string = NULL;
	}

	if( http_session->http_info.content_type != NULL ) {
		free( http_session->http_info.content_type );
		http_session->http_info.content_type = NULL;
	}

	if( http_session->local_info.date_res_last_modified != NULL ) {
		free( http_session->local_info.date_res_last_modified );
		http_session->local_info.date_res_last_modified = NULL;
	}

	if( http_session->http_info.authorization != NULL ) {
		free( http_session->http_info.authorization );
		http_session->http_info.authorization = NULL;
	}

	if( http_session->http_info.user_login != NULL ) {
		free( http_session->http_info.user_login );
		http_session->http_info.user_login = NULL;
	}

	if( http_session->http_info.user_pwd != NULL ) {
		free( http_session->http_info.user_pwd );
		http_session->http_info.user_pwd = NULL;
	}

	/* Zerowanie zmiennych liczbowych */
	http_session->http_info.range_st = -1;
	http_session->http_info.range_en = -1;
	http_session->http_info.is_cgi = -1;
	http_session->http_info.content_length = -1;
	/* Ustawienie metody na nieprzypisan� */
	http_session->http_info.method_name = UNKNOWN_HTTP_METHOD;
}

/*
SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size )
@http_session - wska�nik do pod��czonego klienta
@content_data - dane, kt�re maj� zosta� wys�ane
@http_content_size - rozmiar danych do wys�ania
- wysy�a pakiet danych content_data do aktualnie pod��czonego klienta */
short SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size ) {
	int result = 0;

	if( ( http_content_size <= 0 ) || ( http_session->socket_descriptor == SOCKET_ERROR ) ) {
		return 0;
	}

	/*Wysy�ka przez socket */
	SOCKET_send( http_session, content_data, http_content_size, &result );

	return result;
}

/*
SESSION_add_new_ptr( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- funkcja przypisuje pustemu elementowi tablicy sessions wska�nik do http_session */
void SESSION_add_new_ptr( HTTP_SESSION *http_session ) {
	int i = http_conn_count;

	printf("Szukam4 (%d)...\n", http_conn_count );
	for( i = 0; i <= http_conn_count; i++ ) {
		if( sessions[ i ] == NULL ) {
			sessions[ i ] = http_session;
			printf("znalazlem.\n");
			return;
		}
	}
	printf("Nie znalazlem.\n");
}

/*
SESSION_delete_ptr( HTTP_SESSION *http_session )
@http_session - wska�nik do pod��czonego klienta
- funkcja usuwa z tablicy sessions wska�nik do http_session */
void SESSION_delete_ptr( HTTP_SESSION *http_session ) {
	int i;

	printf("Szukam3...\n");
	for( i = 0; i <= http_conn_count; i++ ) {
		if( sessions[ i ]) {
			if( sessions[ i ]->socket_descriptor == http_session->socket_descriptor ) {
				sessions[ i ] = NULL;
				printf("znalazlem.\n");
				return;
			}
		}
	}
	printf("Nie znalazlem.\n");
}

/*
SESSION_find_response_struct_by_id( int socket )
@socket - identyfikator gniazda
- funkcja na podstawie identyfikatora gniazda odnajduje struktur� z danymi do wysy�ki */
SEND_INFO* SESSION_find_response_struct_by_id( int socket ) {
	int i;

	printf("Szukam1...\n");
	for( i = 0; i <= send_d_count; i++ ) {
		if( send_d[i].socket_descriptor == socket ) {
			printf("znalazlem.\n");
			return &send_d[ i ];
		}
	}

	printf("Nie znalazlem.\n");

	return NULL;
}

/*
SESSION_add_new_send_struct( int socket_descriptor )
@socket_descriptor - identyfikator gniazda
- funkcja znajduje pust� struktur� w tablicy SEND_INFO i przydziela jej identyfikator gniazda w celu umo�liwienia p�niejszej wysy�ki danych */
void SESSION_add_new_send_struct( int socket_descriptor ) {
	int i;

	printf("Szukam2...\n");
	for( i = 0; i <= send_d_count; i++ ){
		if( send_d[ i ].socket_descriptor == 0 ) {
			send_d[ i ].socket_descriptor = socket_descriptor;
			send_d[ i ].sent_size = 0;
			printf("znalazlem.\n");
			send_d_count++;
			return;
		}
	}
	printf("Nie znalazlem.\n");
}

void SESSION_delete_send_struct( int socket_descriptor ) {
	int i;

	printf("Szukam5...\n");
	for( i = 0; i <= send_d_count; i++ ){
		if( send_d[ i ].socket_descriptor == socket_descriptor ) {
			send_d[ i ].socket_descriptor = 0;
			send_d[ i ].sent_size = 0;
			printf("znalazlem.\n");
			send_d_count--;
			return;
		}
	}
	printf("Nie znalazlem.\n");
}
