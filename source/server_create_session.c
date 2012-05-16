/*******************************************************************

Projekt battery_Server

Plik: server_create_session.c

Przeznaczenie:
Interpretacja danych otrzymanych od klienta
Przekazanie danych do funkcji wykonujï¿½cej ï¿½ï¿½danie

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
@http_session - wskaŸnik do pod³¹czonego klienta
- weryfikuje protokó³, którego u¿ywa pod³¹czony klient. */
short SESSION_http_protocol_is_valid( HTTP_SESSION *http_session ) {
	if( strncmp( http_session->http_info.protocol_ver, HTTP_VER, PROTO_BUFF_SIZE ) == 0 ) {
		/* HTTP/1.1: Sprawdzenie, czy jest nagï¿½ï¿½wek "Host:" */
		if( strstr( http_session->http_info.header, HEADER_HOST ) == 0 ) {
			/* brak... */
			RESPONSE_error( http_session, HTTP_400_BAD_REQUEST, HTTP_ERR_400_MSG, NULL );
			SOCKET_disconnect_client( http_session );
			SESSION_release( http_session );
			return 0;
		}
	} else if( ( strcmp( http_session->http_info.protocol_ver, HTTP_VER ) != 0 ) && /* Sprawdzenie, czy protokï¿½ jest obsï¿½ugiwany */
			( strcmp( http_session->http_info.protocol_ver, HTTP_VER_1_0 ) != 0 ) ) {

		/* Protokï¿½ w wersji innej niï¿½ HTTP/1.0 i HTTP/1.1 - bï¿½ï¿½d 505 */
		RESPONSE_error( http_session, HTTP_505_HTTP_VERSION_NOT_SUPPORTED, HTTP_ERR_505_MSG, NULL );
		SOCKET_disconnect_client( http_session );
		SESSION_release( http_session );
		return 0;
	}

	return 1;
}

/*
SESSION_local_path_is_valid( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- weryfikuje poprawnosæ ¿¹danej scie¿ki do zasobu. */
short SESSION_local_path_is_valid( HTTP_SESSION *http_session ) {
	char *tmp_local_file_path;		/* Przechowuje rzeczywistï¿½ ï¿½cieï¿½kï¿½ do pliku na dysku */

	/* Weryfikacja dï¿½ugoï¿½ci ï¿½ï¿½danej ï¿½cieï¿½ki */
	if( strlen( http_session->http_info.http_local_path ) > MAX_URI_LENGTH ) {
		LOG_print( "URI too long.\n" );
		RESPONSE_error( http_session, HTTP_414_REQUEST_URI_TOO_LONG, HTTP_ERR_414_MSG, NULL );
		SOCKET_disconnect_client( http_session );
		SESSION_release( http_session );
		return 0;
	}

	/* Jeï¿½eli podano samï¿½ nazwï¿½ katalogu to automatycznie dodajemy plik indeksu */
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

	/* Sprawdzenie, czy w ï¿½ï¿½danej ï¿½cieï¿½ce zawierajï¿½ siï¿½ znaki ".." lub "/." */
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
@http_session - wskaŸnik do pod³¹czonego klienta
- pobiera nag³ówek wiadomosci HTTP. */
static void SESSION_get_http_header( HTTP_SESSION *http_session ) {
	http_session->http_info.header = ( char* )malloc( BIG_BUFF_SIZE_CHAR );
	mem_allocated( http_session->http_info.header, 0 );
	strncpy( http_session->http_info.header, REQUEST_get_message_header( http_session->http_info.content_data, http_session->address_length ), BIG_BUFF_SIZE );
}

/*
SESSION_reset( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- zeruje zmienne liczbowe ze struktury HTTP_SESSION. */
static void SESSION_reset( HTTP_SESSION *http_session ) {
	/* Wyzerowanie zmiennych odpowiedzialnych za zakres wysyï¿½anych danych z pliku */
	http_session->http_info.range_st = -1;
	http_session->http_info.range_en = -1;
	/* Wyzerowanie zmiennej odpowiedzialnej za informowanie, czy zapytanie jest ï¿½ï¿½daniem zasobu, czy wykonaniem skryptu CGI */
	http_session->http_info.is_cgi = 0;
}

/*
 SESSION_check_auth( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- pobiera dane autoryzacyjne do zasobu od klienta. */
static void SESSION_check_auth( HTTP_SESSION *http_session ) {
	char *user_auth_enc;			/* Przechowuje odszyfrowane login i hasï¿½o */

	if( strstr( http_session->http_info.header, HEADER_AUTHORIZATION ) ) {
		http_session->http_info.authorization = ( char* )malloc( STD_BUFF_SIZE_CHAR );
		strncpy( http_session->http_info.authorization, REQUEST_get_header_value( http_session->http_info.header, HEADER_AUTHORIZATION ), STD_BUFF_SIZE );
		/* Kodowanie metodï¿½ Digest jest nieobsï¿½ugiwane */
		if( strstr( " Digest ", http_session->http_info.authorization ) ) {
			RESPONSE_error( http_session, HTTP_400_BAD_REQUEST, HTTP_ERR_400_MSG, NULL );
			SESSION_release( http_session );
			return;
		} else {
			strncpy( http_session->http_info.authorization, strchr( http_session->http_info.authorization, ' ' )+1, STD_BUFF_SIZE );
			/* Odszyfrowanie danych */
			user_auth_enc = ( char* )calloc( STD_BUFF_SIZE, sizeof( char ) );
			base64_decode( http_session->http_info.authorization, ( unsigned char* )user_auth_enc, STD_BUFF_SIZE );
			/* Pobranie nazwy uï¿½ytkownika */
			http_session->http_info.user_login = ( char* )malloc( SMALL_BUFF_SIZE_CHAR );
			strncpy( http_session->http_info.user_login, strtok( user_auth_enc, ":" ), SMALL_BUFF_SIZE );
			/* Pobranie hasï¿½a */
			http_session->http_info.user_pwd = ( char* )malloc( SMALL_BUFF_SIZE_CHAR );
			strncpy( http_session->http_info.user_pwd, strtok( NULL, ":" ), SMALL_BUFF_SIZE );

			free( user_auth_enc );
			user_auth_enc = NULL;
		}
	}
}

/*
SESSION_get_content_type( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- pobiera informacjê o nag³ówku "Content-Type". */
static void SESSION_get_content_type( HTTP_SESSION *http_session ) {
	if( strstr( http_session->http_info.header, HEADER_CONTENT_TYPE ) ) {
		http_session->http_info.content_type = ( char* )malloc( STD_BUFF_SIZE_CHAR );
		strncpy( http_session->http_info.content_type, REQUEST_get_header_value( http_session->http_info.header, HEADER_CONTENT_TYPE ), STD_BUFF_SIZE );
	}
}

/*
SESSION_get_res_range( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- pobiera informacjê o ¿¹danym przez klienta fragmencie zasobu. */
static void SESSION_get_res_range( HTTP_SESSION *http_session ) {
	if( strstr( http_session->http_info.header, HEADER_RANGE ) ) {
		http_session->http_info.range_st = REQUEST_get_range( http_session, 0 );
		http_session->http_info.range_en = REQUEST_get_range( http_session, 1 );
	}
}

/*
SESSION_get_if_unmodified_since( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- pobiera informacje o nag³ówku "If-Unmodified-Since". */
static void SESSION_get_if_unmodified_since( HTTP_SESSION *http_session ) {
	if( strstr( http_session->http_info.header, HEADER_IF_UNMODIFIED_SINCE ) ) {
		http_session->http_info.date_if_unmodified_since = ( char* )malloc( TIME_BUFF_SIZE_CHAR );
		strncpy( http_session->http_info.date_if_unmodified_since, REQUEST_get_header_value( http_session->http_info.header, HEADER_IF_UNMODIFIED_SINCE ), TIME_BUFF_SIZE );
	}
}

/*
SESSION_get_if_modified_since( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- pobiera informacje o nag³ówku "If-Modified-Since". */
static void SESSION_get_if_modified_since( HTTP_SESSION *http_session ) {
	if( strstr( http_session->http_info.header, HEADER_IF_MODIFIED_SINCE ) ) {
		http_session->http_info.date_if_modified_since = ( char* )malloc( TIME_BUFF_SIZE_CHAR );
		strncpy( http_session->http_info.date_if_modified_since, REQUEST_get_header_value( http_session->http_info.header, HEADER_IF_MODIFIED_SINCE ), TIME_BUFF_SIZE );
	}
}

/*
SESSION_get_connection( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- pobiera informacjê o nag³ówku "Connection". */
static void SESSION_get_connection( HTTP_SESSION *http_session ) {
	char *temp_conn_type_handle;	/* Do wczytania informacji o rodzaju poï¿½ï¿½czenia */

	if( strstr( http_session->http_info.header, HEADER_CONNECTION ) ) {
		/* Pobranie informacji o poï¿½ï¿½czeniu ( "Connection: Keep-Alive/Close" ) - przypisanie do zmiennej tymczasowej */
		temp_conn_type_handle = ( char* )malloc( SMALL_BUFF_SIZE_CHAR );
		mem_allocated( temp_conn_type_handle, 9002 );
		strncpy( temp_conn_type_handle, REQUEST_get_header_value( http_session->http_info.header, HEADER_CONNECTION ), SMALL_BUFF_SIZE );
		/* Ustawienie zmiennej keep-alive w zaleï¿½noï¿½ci od wartoï¿½ci zmiennej temp_conn_type_handle */
		if( strncasecmp( temp_conn_type_handle, HEADER_KEEP_ALIVE_STR, SMALL_BUFF_SIZE ) == 0 ) {
			http_session->http_info.keep_alive = 1;
		} else {
			http_session->http_info.keep_alive = 0;
		}

		free( temp_conn_type_handle );
		temp_conn_type_handle = NULL;

	} else {
		/* Brak - poï¿½ï¿½czenie zamkniï¿½te */
		http_session->http_info.keep_alive = 0;
	}
}

/*
SESSION_check_connections_limit( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- weryfikuje ilosæ pod³¹czonych klientów */
short SESSION_check_connections_limit( HTTP_SESSION *http_session ) {
	/* Sprawdzenie iloï¿½ci podï¿½ï¿½czonych klientï¿½w. Jeï¿½eli jest max = bï¿½ï¿½d 503 */
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
@http_session - wskaŸnik do pod³¹czonego klienta
- pobiera adres IP pod³¹czonego klienta. */
static void SESSION_get_ip_addr( HTTP_SESSION *http_session ) {
	http_session->http_info.remote_addr = ( char* )malloc( TINY_BUFF_SIZE_CHAR );
	mem_allocated( http_session->http_info.remote_addr, 6 );
	strncpy( http_session->http_info.remote_addr, SOCKET_get_remote_ip( http_session ), TINY_BUFF_SIZE );
}

/*
SESSION_prepare( HTTP_SESSION *http_session, const char *content_data )
@http_session - wskaï¿½nik do danych podï¿½ï¿½czonego klienta
- przeprowadza weryfikacjï¿½ przesï¿½anych danych:
+ metoda ( obsï¿½ugiwane: GET, HEAD, POST )
+ dï¿½ugoï¿½ï¿½ adresu URI
+ obecnoï¿½ï¿½ nagï¿½ï¿½wka "Host"
+ typ poï¿½ï¿½czenia ( close/keep-alive )
+ obecnoï¿½ï¿½ nagï¿½ï¿½wka "If-Modified-Since"
+ obecnoï¿½ï¿½ nagï¿½ï¿½wka "If-Unmodified-Since"
+ obecnoï¿½ï¿½ nagï¿½ï¿½wka "Content-Length"
+ obecnoï¿½ï¿½ nagï¿½ï¿½wka "Content-Type"
+ obecnoï¿½ï¿½ nagï¿½ï¿½wka "Range"
+ po pomyï¿½lnej weryfikacji struktury przekazuje dane do wykonania przez funkcjï¿½ REQUEST_process */
void SESSION_prepare( HTTP_SESSION *http_session ) {
	char *temp_http_method_name;	/* Do wczytania ï¿½ï¿½danej metody */
	char *temp_entire_msg;			/* Do przechowania caï¿½ej wiadomoï¿½ci. Treï¿½ï¿½ bï¿½dzie traktowana funkcjï¿½ strtok, a oryginalnï¿½ zawartoï¿½ï¿½ zmiennej http_info.content_data chcemy zatrzymaï¿½ */
	char *tmp_post_data;			/* Do przechowania POST data */

	/* Tutaj jest zabezpieczenie przed otrzymaniem metody POST w czï¿½ciach.
	Zmienna received_all jest ustawiana na 0 przy sprawdzaniu "Content-Length",
	kiedy pobrana zawartoï¿½ï¿½ ( content ) ma rozmiar mniejszy niï¿½ wartoï¿½ï¿½ tego nagï¿½ï¿½wka.
	Wtedy podczas odbioru danych z socketa otrzymane dane zostajï¿½ sklejone z http_info.content_data
	i dopiero komunikat jest procesowany. received_all ma wartoï¿½ï¿½ -1, jeï¿½eli metodï¿½ jest GET lub HEAD.*/
	if( http_session->http_info.received_all == 1 ) {
		/* Reset zmiennej odpowiedzialnej za odbiï¿½r kolejnej czï¿½ci komunikatu */
		http_session->http_info.received_all = -1;

		/* Przejï¿½cie do wywoï¿½ania funkcji procesujï¿½cej ï¿½ï¿½danie i zarzï¿½dzanie rozï¿½ï¿½czeniem klienta */
		REQUEST_process( http_session );

		return;
	}

	/* Wyzerowanie zmiennych liczbowych */
	SESSION_reset( http_session );

	/*Pobranie adresu IP */
	SESSION_get_ip_addr( http_session );

	/*Pobranie nagï¿½ï¿½wka wiadomoï¿½ci */
	SESSION_get_http_header( http_session );

	/* Pobranie pierwszej linijki ï¿½ï¿½dania */
	http_session->http_info.request_line = ( char* )malloc( BIG_BUFF_SIZE_CHAR );
	mem_allocated( http_session->http_info.request_line, 1 );
	/* Zmienna temp_entire_msg bï¿½dzie przechowywaï¿½ zawartoï¿½ï¿½ http_info.content_data na potrzeby funkcji strtok */
	temp_entire_msg = ( char* )malloc( MAX_PATH_LENGTH+TINY_BUFF_SIZE );
	strncpy( temp_entire_msg, http_session->http_info.content_data, MAX_PATH_LENGTH+TINY_BUFF_SIZE );
	strncpy( http_session->http_info.request_line, strtok( temp_entire_msg, "\015\012" ), MAX_PATH_LENGTH+TINY_BUFF_SIZE );

	/* Pobranie ï¿½ï¿½danej metody z request_line */
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
		/* Metoda nieobsï¿½ugiwana - papa... */
		RESPONSE_error( http_session, HTTP_501_NOT_SUPPORTED, HTTP_ERR_501_MSG, NULL );
		SESSION_release( http_session );
		free( temp_http_method_name );
		temp_http_method_name = NULL;
		return;
	}

	free( temp_http_method_name );
	temp_http_method_name = NULL;

	/* Pamiï¿½ï¿½ na ï¿½cieï¿½kï¿½ lokalnï¿½ */
	http_session->http_info.http_local_path = ( char* )malloc( MAX_PATH_LENGTH_CHAR );
	mem_allocated( http_session->http_info.http_local_path, 2 );
	/* Pobranie ï¿½cieï¿½ki do pliku */
	strncpy( http_session->http_info.http_local_path, strtok( NULL, " " ), MAX_PATH_LENGTH );

	/* Weryfikacja scie¿ki do ¿¹danego zasobu */
	if( SESSION_local_path_is_valid( http_session ) == 0 ) {
		return;
	}

	/* Pamiï¿½ï¿½ na wersjï¿½ protokoï¿½u HTTP */
	http_session->http_info.protocol_ver = ( char* )malloc( PROTO_BUFF_SIZE_CHAR );
	mem_allocated( http_session->http_info.protocol_ver, 3 );
	/* Pobranie wersji protokoï¿½u HTTP */
	strncpy( http_session->http_info.protocol_ver, strtok( NULL, "\015" ), PROTO_BUFF_SIZE );

	free( temp_entire_msg );
	temp_entire_msg = NULL;

	/* Sprawdzenie wersji protokoï¿½u przeglï¿½darki */
	if( SESSION_http_protocol_is_valid( http_session ) == 0 ) {
		return;
	}

	/* Sprawdzenie ilosci pod³¹czonych klientów. W razie maksymalnej iloci przestaje procesowaæ ¿¹danie i wysy³a informacjê do klienta */
	if( SESSION_check_connections_limit( http_session ) == 0 ) {
		return;
	}

	/* Sprawdzenie, czy jest nagï¿½ï¿½wek "Connection:" */
	SESSION_get_connection( http_session );

	/* Pobranie informacji o parametrze "If-Modified-Since" */
	SESSION_get_if_modified_since( http_session );

	/* Pobranie informacji o parametrze "If-Unmodified-Since" */
	SESSION_get_if_unmodified_since( http_session );

	/* Pobranie informacji o parametrze "Range:" */
	SESSION_get_res_range( http_session );

	/* Pobranie informacji o parametrze "Content-Type" */
	SESSION_get_content_type( http_session );

	/* Pobranie informacji o nagï¿½ï¿½wku "Authorization" */
	SESSION_check_auth( http_session );

	/* Pobranie informacji o parametrze "Content-Length" */
	if( strstr( http_session->http_info.header, HEADER_CONTENT_LENGTH ) ) {
		/* Przypisanie zmiennej http_info.content_length zawartoï¿½ci nagï¿½ï¿½wka "Content-Length" */
		http_session->http_info.content_length = atoi( REQUEST_get_header_value( http_session->http_info.header, HEADER_CONTENT_LENGTH ) );
		/* Prï¿½ba pobrania zawartoï¿½ci ï¿½ï¿½dania do zmiennej tymczasowej */
		tmp_post_data = ( char* )malloc( MAX_BUFFER_CHAR );
		mem_allocated( tmp_post_data, 9002 );
		strncpy( tmp_post_data, REQUEST_get_message_body( http_session ), MAX_BUFFER );
		/* Sprawdzenie, czy rozmiar pobranej zawartoï¿½ci jest zgodny z danymi z nagï¿½ï¿½wka "Content-Length" */
		if( strlen( tmp_post_data ) < http_session->http_info.content_length ) {
			/* Nie odebrano caï¿½ego ï¿½ï¿½dania, przekazanie informacji do funkcji odbierajï¿½cej dane
			o tym, ï¿½e kolejna porcja danych z socketa nie bï¿½dzie nowï¿½ sesjï¿½, tylko dokoï¿½czeniem starej */
			http_session->http_info.received_all = 0;

			free( tmp_post_data );
			tmp_post_data = NULL;
			return;
		} else {
			http_session->http_info.received_all = -1;
		}

		/* Odebrano wszystko za jednym razem;
		przypisanie do zmiennej http_info.query_string zawartoï¿½ci komunikatu */
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
	} else {	/* Brak nagï¿½ï¿½wka "Content-Length", a ï¿½ï¿½dana metoda to POST = bï¿½ï¿½d 411 */
		if( http_session->http_info.method_name == POST ) {
			RESPONSE_error( http_session, HTTP_411_LENGTH_REQUIRED, HTTP_ERR_411_MSG, NULL );
			SOCKET_disconnect_client( http_session );
			SESSION_release( http_session );
			return;
		}
	}

	/* Przejï¿½cie do wywoï¿½ania funkcji procesujï¿½cej ï¿½ï¿½danie i zarzï¿½dzanie rozï¿½ï¿½czeniem klienta */
	REQUEST_process( http_session );
}

/*
SESSION_release( HTTP_SESSION *http_session, int release_content )
@http_session - wskaï¿½nik do podï¿½ï¿½czonego klienta
- sprawdza, czy elementy struktury HTTP_SESSION zajmujï¿½ pamiï¿½ï¿½ i w razie potrzeby zwalnia jï¿½ */
void SESSION_release( HTTP_SESSION *http_session )
{
	/* Zwalnianie ciï¿½gï¿½w znakï¿½w */
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
	/* Ustawienie metody na nieprzypisanï¿½ */
	http_session->http_info.method_name = UNKNOWN_HTTP_METHOD;
}

/*
SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size )
@http_session - wskaï¿½nik do podï¿½ï¿½czonego klienta
@content_data - dane, ktï¿½re majï¿½ zostaï¿½ wysï¿½ane
@http_content_size - rozmiar danych do wysï¿½ania
- wysyï¿½a pakiet danych content_data do aktualnie podï¿½ï¿½czonego klienta */
short SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size ) {
	int result = 0;

	if( ( http_content_size <= 0 ) || ( http_session->socket_descriptor == SOCKET_ERROR ) ) {
		return 0;
	}

	/*Wysyï¿½ka przez socket */
	SOCKET_send( http_session, content_data, http_content_size, &result );

	return result;
}

/*
SESSION_add_new_ptr( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- funkcja przypisuje pustemu elementowi tablicy sessions wskaŸnik do http_session */
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
@http_session - wskaŸnik do pod³¹czonego klienta
- funkcja usuwa z tablicy sessions wskaŸnik do http_session */
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
- funkcja na podstawie identyfikatora gniazda odnajduje strukturê z danymi do wysy³ki */
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
- funkcja znajduje pust¹ strukturê w tablicy SEND_INFO i przydziela jej identyfikator gniazda w celu umo¿liwienia póŸniejszej wysy³ki danych */
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
