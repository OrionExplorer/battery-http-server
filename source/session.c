/*******************************************************************

Projekt battery-http-server

Plik: session.c

Przeznaczenie:
Interpretacja danych otrzymanych od klienta
Przekazanie danych do funkcji wykonującej żądanie

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/session.h"
#include "include/socket_io.h"
#include "include/http_protocol.h"
#include "include/files_io.h"
#include "include/log.h"
#include "include/base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void     SESSION_get_ip_addr( HTTP_SESSION *http_session );
static void     SESSION_reset( HTTP_SESSION *http_session );
static void     SESSION_get_connection( HTTP_SESSION *http_session );
static void     SESSION_get_if_modified_since( HTTP_SESSION *http_session );
static void     SESSION_get_if_unmodified_since( HTTP_SESSION *http_session );
static void     SESSION_get_res_range( HTTP_SESSION *http_session );
static void     SESSION_get_content_type( HTTP_SESSION *http_session );
static void     SESSION_check_auth( HTTP_SESSION *http_session );
static void     SESSION_get_http_header( HTTP_SESSION *http_session );
short           SESSION_local_path_is_valid( HTTP_SESSION *http_session );
short           SESSION_http_protocol_is_valid( HTTP_SESSION *http_session );
short           SESSION_check_connections_limit( HTTP_SESSION *http_session );

/*
SESSION_http_protocol_is_valid( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- weryfikuje protokół, którego używa podłączony klient. */
short SESSION_http_protocol_is_valid( HTTP_SESSION *http_session ) {
    if( strncmp( http_session->http_info.protocol_ver, HTTP_VER, PROTO_BUFF_SIZE ) == 0 ) {
        /* HTTP/1.1: Sprawdzenie, czy jest nagłówek "Host:" */
        if( strstr( http_session->http_info.header, HEADER_HOST ) == 0 ) {
            /* brak... */
            RESPONSE_error( http_session, HTTP_400_BAD_REQUEST, HTTP_ERR_400_MSG, NULL );
            SOCKET_disconnect_client( http_session );
            SESSION_release( http_session );
            return 0;
        }
    } else if( ( strcmp( http_session->http_info.protocol_ver, HTTP_VER ) != 0 ) && /* Sprawdzenie, czy protokół jest obsługiwany */
            ( strcmp( http_session->http_info.protocol_ver, HTTP_VER_1_0 ) != 0 ) ) {

        /* Protokół w wersji innej niż HTTP/1.0 i HTTP/1.1 - błąd 505 */
        RESPONSE_error( http_session, HTTP_505_HTTP_VERSION_NOT_SUPPORTED, HTTP_ERR_505_MSG, NULL );
        SOCKET_disconnect_client( http_session );
        SESSION_release( http_session );
        return 0;
    }

    return 1;
}

/*
SESSION_local_path_is_valid( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- weryfikuje poprawność żądanej ścieżki do zasobu. */
short SESSION_local_path_is_valid( HTTP_SESSION *http_session ) {
    char *tmp_local_file_path;      /* Przechowuje rzeczywistą ścieżkę do pliku na dysku */

    /* Weryfikacja długości żądanej ścieżki */
    if( strlen( http_session->http_info.http_local_path ) > MAX_URI_LENGTH ) {
        LOG_print( "URI too long.\n" );
        RESPONSE_error( http_session, HTTP_414_REQUEST_URI_TOO_LONG, HTTP_ERR_414_MSG, NULL );
        SOCKET_disconnect_client( http_session );
        SESSION_release( http_session );
        return 0;
    }

    /* Jeżeli podano samą nazwę katalogu to automatycznie dodajemy plik indeksu */
    if( strncmp( file_get_name( http_session->http_info.http_local_path ), "", 1 ) == 0 ) {
        tmp_local_file_path = malloc( MAX_PATH_LENGTH_CHAR+1 );
        mem_allocated( tmp_local_file_path, 10 );

        strncpy( tmp_local_file_path, app_path, MAX_PATH_LENGTH );
        strncat( tmp_local_file_path, http_session->http_info.http_local_path, MAX_PATH_LENGTH );
        strrepchar( tmp_local_file_path, '/', C_SLASH );
        strdelbslash( tmp_local_file_path );
        strncat( http_session->http_info.http_local_path, REQUEST_get_index( tmp_local_file_path ), MAX_PATH_LENGTH );

        free( tmp_local_file_path );
        tmp_local_file_path = NULL;
    }

    /* Sprawdzenie, czy w żądanej ścieżce zawierają się znaki ".." lub "/." */
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
@http_session - wskaźnik do podłączonego klienta
- pobiera nagłówek wiadomości HTTP. */
static void SESSION_get_http_header( HTTP_SESSION *http_session ) {
    http_session->http_info.header = malloc( BIG_BUFF_SIZE_CHAR );
    mem_allocated( http_session->http_info.header, 11 );
    strncpy( http_session->http_info.header, REQUEST_get_message_header( http_session->http_info.content_data, http_session->recv_data_len ), BIG_BUFF_SIZE );
}

/*
SESSION_reset( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- zeruje zmienne liczbowe ze struktury HTTP_SESSION. */
static void SESSION_reset( HTTP_SESSION *http_session ) {
    /* Wyzerowanie zmiennych odpowiedzialnych za zakres wysyï¿½anych danych z pliku */
    http_session->http_info.range_st = -1;
    http_session->http_info.range_en = -1;
    /* Wyzerowanie zmiennej odpowiedzialnej za informowanie, czy zapytanie jest żądaniem zasobu, czy wykonaniem skryptu CGI */
}

/*
 SESSION_check_auth( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- pobiera dane autoryzacyjne do zasobu od klienta. */
static void SESSION_check_auth( HTTP_SESSION *http_session ) {
    char *user_auth_enc;            /* Przechowuje odszyfrowane login i hasło */

    if( strstr( http_session->http_info.header, HEADER_AUTHORIZATION ) ) {
        http_session->http_info.authorization = malloc( STD_BUFF_SIZE_CHAR );
        strncpy( http_session->http_info.authorization, REQUEST_get_header_value( http_session->http_info.header, HEADER_AUTHORIZATION ), STD_BUFF_SIZE );
        /* Kodowanie metodą Digest jest nieobsługiwane */
        if( strstr( " Digest ", http_session->http_info.authorization ) ) {
            RESPONSE_error( http_session, HTTP_400_BAD_REQUEST, HTTP_ERR_400_MSG, NULL );
            SESSION_release( http_session );
            return;
        } else {
            strncpy( http_session->http_info.authorization, strchr( http_session->http_info.authorization, ' ' )+1, STD_BUFF_SIZE );
            /* Odszyfrowanie danych */
            user_auth_enc = ( char* )calloc( STD_BUFF_SIZE, sizeof( char ) );
            base64_decode( http_session->http_info.authorization, ( unsigned char* )user_auth_enc, STD_BUFF_SIZE );
            /* Pobranie nazwy użytkownika */
            http_session->http_info.user_login = malloc( SMALL_BUFF_SIZE_CHAR );
            strncpy( http_session->http_info.user_login, strtok( user_auth_enc, ":" ), SMALL_BUFF_SIZE );
            /* Pobranie hasła */
            http_session->http_info.user_pwd = malloc( SMALL_BUFF_SIZE_CHAR );
            strncpy( http_session->http_info.user_pwd, strtok( NULL, ":" ), SMALL_BUFF_SIZE );

            free( user_auth_enc );
            user_auth_enc = NULL;
        }
    }
}

/*
SESSION_get_content_type( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- pobiera informacjê o nagłówku "Content-Type". */
static void SESSION_get_content_type( HTTP_SESSION *http_session ) {
    if( strstr( http_session->http_info.header, HEADER_CONTENT_TYPE ) ) {
        http_session->http_info.content_type = malloc( STD_BUFF_SIZE_CHAR );
        strncpy( http_session->http_info.content_type, REQUEST_get_header_value( http_session->http_info.header, HEADER_CONTENT_TYPE ), STD_BUFF_SIZE );
    }
}

/*
SESSION_get_res_range( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- pobiera informacjê o ¿¹danym przez klienta fragmencie zasobu. */
static void SESSION_get_res_range( HTTP_SESSION *http_session ) {
    if( strstr( http_session->http_info.header, HEADER_RANGE ) ) {
        http_session->http_info.range_st = REQUEST_get_range( http_session, 0 );
        http_session->http_info.range_en = REQUEST_get_range( http_session, 1 );
    }
}

/*
SESSION_get_if_unmodified_since( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- pobiera informacje o nagłówku "If-Unmodified-Since". */
static void SESSION_get_if_unmodified_since( HTTP_SESSION *http_session ) {
    if( strstr( http_session->http_info.header, HEADER_IF_UNMODIFIED_SINCE ) ) {
        http_session->http_info.date_if_unmodified_since = malloc( TIME_BUFF_SIZE_CHAR );
        strncpy( http_session->http_info.date_if_unmodified_since, REQUEST_get_header_value( http_session->http_info.header, HEADER_IF_UNMODIFIED_SINCE ), TIME_BUFF_SIZE );
    }
}

/*
SESSION_get_if_modified_since( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- pobiera informacje o nagłówku "If-Modified-Since". */
static void SESSION_get_if_modified_since( HTTP_SESSION *http_session ) {
    if( strstr( http_session->http_info.header, HEADER_IF_MODIFIED_SINCE ) ) {
        http_session->http_info.date_if_modified_since = malloc( TIME_BUFF_SIZE_CHAR );
        strncpy( http_session->http_info.date_if_modified_since, REQUEST_get_header_value( http_session->http_info.header, HEADER_IF_MODIFIED_SINCE ), TIME_BUFF_SIZE );
    }
}

/*
SESSION_get_connection( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- pobiera informacjê o nagłówku "Connection". */
static void SESSION_get_connection( HTTP_SESSION *http_session ) {
    char *temp_conn_type_handle;    /* Do wczytania informacji o rodzaju połączenia */
    SEND_INFO *send_struct;

    if( strstr( http_session->http_info.header, HEADER_CONNECTION ) ) {
        /* Pobranie informacji o połączeniu ( "Connection: Keep-Alive/Close" ) - przypisanie do zmiennej tymczasowej */
        temp_conn_type_handle = malloc( SMALL_BUFF_SIZE_CHAR );
        mem_allocated( temp_conn_type_handle, 12 );
        strncpy( temp_conn_type_handle, REQUEST_get_header_value( http_session->http_info.header, HEADER_CONNECTION ), SMALL_BUFF_SIZE );
        /* Ustawienie zmiennej keep-alive w zależności od wartości zmiennej temp_conn_type_handle */
        if( strncasecmp( temp_conn_type_handle, HEADER_KEEP_ALIVE_STR, SMALL_BUFF_SIZE ) == 0 ) {
            http_session->http_info.keep_alive = 1;
        } else {
            http_session->http_info.keep_alive = 0;
        }

        free( temp_conn_type_handle );
        temp_conn_type_handle = NULL;

    } else {
        /* Brak - połączenie zamkniï¿½te */
        http_session->http_info.keep_alive = 0;
    }

    send_struct = SESSION_find_response_struct_by_id( http_session->socket_fd );
    if( send_struct ) {
        send_struct->keep_alive = http_session->http_info.keep_alive;
    }
}

/*
SESSION_check_connections_limit( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- weryfikuje liczbę podłączonych klientów */
short SESSION_check_connections_limit( HTTP_SESSION *http_session ) {
    /* Sprawdzenie liczby podłączonych klientów. Jeżeli jest max = błąd 503 */
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
@http_session - wskaźnik do podłączonego klienta
- pobiera adres IP podłączonego klienta. */
static void SESSION_get_ip_addr( HTTP_SESSION *http_session ) {
    http_session->http_info.remote_addr = malloc( TINY_BUFF_SIZE_CHAR );
    mem_allocated( http_session->http_info.remote_addr, 13 );
    strncpy( http_session->http_info.remote_addr, SOCKET_get_remote_ip( http_session ), TINY_BUFF_SIZE );
}

/*
SESSION_prepare( HTTP_SESSION *http_session, const char *content_data )
@http_session - wskaźnik do danych podłączonego klienta
- przeprowadza weryfikację przesłanych danych:
+ metoda ( obsługiwane: GET, HEAD, POST )
+ długość adresu URI
+ obecność nagłówka "Host"
+ typ połączenia ( close/keep-alive )
+ obecność nagłówka "If-Modified-Since"
+ obecność nagłówka "If-Unmodified-Since"
+ obecność nagłówka "Content-Length"
+ obecność nagłówka "Content-Type"
+ obecność nagłówka "Range"
+ po pomyślnej weryfikacji struktury przekazuje dane do wykonania przez funkcję REQUEST_process */
void SESSION_prepare( HTTP_SESSION *http_session ) {
    char *temp_http_method_name;    /* Do wczytania żądanej metody */
    char *temp_entire_msg;          /* Do przechowania całej wiadomości. Treść będzie traktowana funkcjï¿½ strtok, a oryginalną zawartością zmiennej http_info.content_data chcemy zatrzymać */
    char *tmp_post_data;            /* Do przechowania POST data */

    /* Tutaj jest zabezpieczenie przed otrzymaniem metody POST w częściach.
    Zmienna received_all jest ustawiana na 0 przy sprawdzaniu "Content-Length",
    kiedy pobrana zawartość ( content ) ma rozmiar mniejszy niż wartość tego nagłówka.
    Wtedy podczas odbioru danych z socketa otrzymane dane zostają sklejone z http_info.content_data
    i dopiero komunikat jest procesowany. received_all ma wartość -1, jeżeli metodą jest GET lub HEAD.*/
    if( http_session->http_info.received_all == 1 ) {
        /* Reset zmiennej odpowiedzialnej za odbiór kolejnej części komunikatu */
        http_session->http_info.received_all = -1;

        /* Przejście do wywołania funkcji procesującej żądanie i zarządzanie rozłączeniem klienta */
        REQUEST_process( http_session );

        return;
    }

    /* Wyzerowanie zmiennych liczbowych */
    SESSION_reset( http_session );

    /*Pobranie adresu IP */
    SESSION_get_ip_addr( http_session );

    /*Pobranie nagłówka wiadomoï¿½ci */
    SESSION_get_http_header( http_session );

    /* Pobranie pierwszej linijki żądania */
    http_session->http_info.request_line = malloc( BIG_BUFF_SIZE_CHAR );
    mem_allocated( http_session->http_info.request_line, 14 );
    /* Zmienna temp_entire_msg będzie przechowywać zawartość http_info.content_data na potrzeby funkcji strtok */
    temp_entire_msg = malloc( MAX_PATH_LENGTH+TINY_BUFF_SIZE );
    strncpy( temp_entire_msg, http_session->http_info.content_data, MAX_PATH_LENGTH+TINY_BUFF_SIZE );
    strncpy( http_session->http_info.request_line, strtok( temp_entire_msg, "\015\012" ), MAX_PATH_LENGTH+TINY_BUFF_SIZE );

    /* Pobranie żądanej metody z request_line */
    temp_http_method_name = malloc( MICRO_BUFF_SIZE_CHAR );
    mem_allocated( temp_http_method_name, 15 );
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
        /* Metoda nieobsługiwana - papa... */
        RESPONSE_error( http_session, HTTP_501_NOT_IMPLEMENTED, HTTP_ERR_501_MSG, NULL );
        SOCKET_disconnect_client( http_session );
        SESSION_release( http_session );
        if( temp_http_method_name ) {
            free( temp_http_method_name );
            temp_http_method_name = NULL;
        }
        return;
    }

    free( temp_http_method_name );
    temp_http_method_name = NULL;

    /* Pamięć na ścieżkę lokalną */
    http_session->http_info.http_local_path = malloc( MAX_PATH_LENGTH_CHAR );
    mem_allocated( http_session->http_info.http_local_path, 16 );
    /* Pobranie ścieżki do pliku */
    strncpy( http_session->http_info.http_local_path, strtok( NULL, " " ), MAX_PATH_LENGTH );

    /* Weryfikacja ścieżki do żądanego zasobu */
    if( SESSION_local_path_is_valid( http_session ) == 0 ) {
        return;
    }

    /* Pamięć na wersję protokołu HTTP */
    http_session->http_info.protocol_ver = malloc( PROTO_BUFF_SIZE_CHAR );
    mem_allocated( http_session->http_info.protocol_ver, 17 );
    /* Pobranie wersji protokołu HTTP */
    strncpy( http_session->http_info.protocol_ver, strtok( NULL, "\015" ), PROTO_BUFF_SIZE );

    free( temp_entire_msg );
    temp_entire_msg = NULL;

    /* Sprawdzenie wersji protokołu przeglądarki */
    if( SESSION_http_protocol_is_valid( http_session ) == 0 ) {
        return;
    }

    /* Sprawdzenie liczby podłączonych klientów. W razie osiągnięcia maksymalnej wartości przestaje procesować żądanie i wysyłaa informację do klienta. */
    if( SESSION_check_connections_limit( http_session ) == 0 ) {
        return;
    }

    /* Sprawdzenie, czy jest nagłówek "Connection:" */
    SESSION_get_connection( http_session );

    /* Pobranie informacji o parametrze "If-Modified-Since" */
    SESSION_get_if_modified_since( http_session );

    /* Pobranie informacji o parametrze "If-Unmodified-Since" */
    SESSION_get_if_unmodified_since( http_session );

    /* Pobranie informacji o parametrze "Range:" */
    SESSION_get_res_range( http_session );

    /* Pobranie informacji o parametrze "Content-Type" */
    SESSION_get_content_type( http_session );

    /* Pobranie informacji o nagłówku "Authorization" */
    SESSION_check_auth( http_session );

    /* Pobranie informacji o parametrze "Content-Length" */
    if( strstr( http_session->http_info.header, HEADER_CONTENT_LENGTH ) ) {
        /* Przypisanie zmiennej http_info.content_length zawartości nagłówka "Content-Length" */
        http_session->http_info.content_length = atoi( REQUEST_get_header_value( http_session->http_info.header, HEADER_CONTENT_LENGTH ) );
        /* Próba pobrania zawartości żądania do zmiennej tymczasowej */
        tmp_post_data = malloc( MAX_BUFFER_CHAR );
        mem_allocated( tmp_post_data, 18 );
        strncpy( tmp_post_data, REQUEST_get_message_body( http_session ), MAX_BUFFER );
        /* Sprawdzenie, czy rozmiar pobranej zawartości jest zgodny z danymi z nagłówka "Content-Length" */
        /* TODO: BINARY DATA!!! */
        if( strlen( tmp_post_data ) < http_session->http_info.content_length ) {
            /* Nie odebrano całego żądania, przekazanie informacji do funkcji odbierającej dane
            o tym, że kolejna porcja danych z socketa nie będzie nową sesją, tylko dokończeniem starej */
            http_session->http_info.received_all = 0;

            free( tmp_post_data );
            tmp_post_data = NULL;
            return;
        } else {
            http_session->http_info.received_all = -1;
        }

        /* Odebrano wszystko za jednym razem;
        przypisanie do zmiennej http_info.query_string zawartości komunikatu */
        if( !http_session->http_info.query_string ) {
            http_session->http_info.query_string = malloc( MAX_BUFFER_CHAR );
            mem_allocated( http_session->http_info.query_string, 19 );
        }
        strncpy( http_session->http_info.query_string, tmp_post_data, MAX_BUFFER );

        if( strlen( http_session->http_info.query_string ) >= MAX_BUFFER ) {
            RESPONSE_error( http_session, HTTP_413_REQUEST_ENTITY_TOO_LARGE, HTTP_ERR_413_MSG, NULL );
            SOCKET_disconnect_client( http_session );
            SESSION_release( http_session );

            free( tmp_post_data );
            tmp_post_data = NULL;
            return;
        }

        free( tmp_post_data );
        tmp_post_data = NULL;
    } else {    /* Brak nagłówka "Content-Length", a żądana metoda to POST = błąd 411 */
        if( http_session->http_info.method_name == POST ) {
            RESPONSE_error( http_session, HTTP_411_LENGTH_REQUIRED, HTTP_ERR_411_MSG, NULL );
            SOCKET_disconnect_client( http_session );
            SESSION_release( http_session );
            return;
        }
    }

    /* Przejście do wywołania funkcji procesującej żądanie i zarządzanie rozłączeniem klienta */
    REQUEST_process( http_session );
}

/*
SESSION_init( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- zeruje elementy struktury */
void SESSION_init( HTTP_SESSION *http_session ) {

    http_session->http_info.content_data = NULL;
    http_session->http_info.http_local_path = NULL;
    http_session->http_info.protocol_ver = NULL;
    http_session->http_info.remote_addr = NULL;
    http_session->http_info.remote_host = NULL;
    http_session->http_info.header = NULL;
    http_session->http_info.request_line = NULL;
    http_session->http_info.date_if_modified_since = NULL;
    http_session->http_info.date_if_unmodified_since = NULL;
    http_session->http_info.query_string = NULL;
    http_session->http_info.content_type = NULL;
    http_session->local_info.date_res_last_modified = NULL;
    http_session->http_info.authorization = NULL;
    http_session->http_info.user_login = NULL;
    http_session->http_info.user_pwd = NULL;
    /* Zerowanie zmiennych liczbowych */
    http_session->http_info.range_st = -1;
    http_session->http_info.range_en = -1;
    http_session->http_info.content_length = -1;
    /* Ustawienie metody na nieprzypisaną */
    http_session->http_info.method_name = UNKNOWN_HTTP_METHOD;
}

/*
SESSION_release( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- sprawdza, czy elementy struktury HTTP_SESSION zajmują pamięć i w razie potrzeby zwalnia ją */
void SESSION_release( HTTP_SESSION *http_session ) {

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
    http_session->http_info.content_length = -1;
    /* Ustawienie metody na nieprzypisanï¿½ */
    http_session->http_info.method_name = UNKNOWN_HTTP_METHOD;
}

/*
SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size )
@http_session - wskaźnik do podłączonego klienta
@content_data - dane, które mają zostać wysłane
@http_content_size - rozmiar danych do wysłania
- wysyła pakiet danych content_data do aktualnie podłączonego klienta */
int SESSION_send_response( HTTP_SESSION *http_session, const char *content_data, int http_content_size ) {
    int result = 0;

    if( ( http_content_size <= 0 ) || ( http_session->socket_fd == SOCKET_ERROR ) ) {
        return 0;
    }

    /* Wysyłka przez socket */
    SOCKET_send( http_session, content_data, http_content_size, &result );

    return result;
}

/*
SESSION_find_response_struct_by_id( int socket )
@socket - identyfikator gniazda
- funkcja na podstawie identyfikatora gniazda odnajduje strukturę z danymi do wysyłki */
SEND_INFO* SESSION_find_response_struct_by_id( int socket ) {
    int i;

    for( i = 0; i <= MAX_CLIENTS; i++ ) {
        if( send_d[ i ].socket_fd == socket ) {
            return &send_d[ i ];
        }
    }

    return NULL;
}

/*
SESSION_add_new_send_struct( int socket_fd )
@socket_fd - identyfikator gniazda
- funkcja znajduje pustą strukturę w tablicy SEND_INFO i przydziela jej identyfikator gniazda w celu umożliwienia późniejszej wysyłki danych */
void SESSION_add_new_send_struct( int socket_fd ) {
    int i;

    for( i = 0; i <= MAX_CLIENTS-1; i++ ){
        if( send_d[ i ].socket_fd == 0 ) {
            send_d[ i ].socket_fd = socket_fd;
            send_d[ i ].sent_size = 0;
            send_d[ i ].http_content_size = 0;
            send_d[ i ].total_size = 0;
            send_d[ i ].keep_alive = 0;
            return;
        }
    }
}

/*
SESSION_add_new_send_struct( int socket_fd )
@socket_fd - identyfikator gniazda
- funkcja usuwa strukturę SEND_INFO, ponieważ wysłanie contentu zakończyło się lub klient się rozłączy  */
void SESSION_delete_send_struct( int socket_fd ) {
    int i;

    for( i = 0; i <= MAX_CLIENTS-1; i++ ){
        if( send_d[ i ].socket_fd == socket_fd ) {
            battery_fclose( send_d[ i ].file, socket_fd );

            /*if( send_d[ i ].keep_alive <= 0 ) {
                SOCKET_close_fd( send_d[ i ].socket_fd );
            }*/

            send_d[ i ].socket_fd = 0;
            send_d[ i ].sent_size = 0;
            send_d[ i ].http_content_size = 0;
            send_d[ i ].keep_alive = 0;
            send_d[ i ].total_size = 0;

            return;
        }
    }
}
