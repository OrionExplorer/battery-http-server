/*******************************************************************

Projekt battery-http-server

Plik: cgi_manager.c

Przeznaczenie:
Uruchamia skrypty CGI i odczytuje wyniki ich działania

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/shared.h"
#include "include/cgi_manager.h"
#include "include/files_io.h"
#include "include/http_protocol.h"
#include "include/session.h"
#include "include/portable.h"
#include "include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <string.h>

static void     CGI_set_env_variables( HTTP_SESSION *http_session, const char *filename );
static void     CGI_set_env( const char *var, const char *val );

/*
CGI_execute( HTTP_SESSION *http_session, const char *filename )
@http_session - wskaźnik do podłączonego klienta
@filename - nazwa pliku skryptu CGI pobranego z żądania
- funkcja wykonuje żąday skrypt i wysyła odpowiedź do podłączonego klienta. */
void CGI_execute( HTTP_SESSION *http_session, const char *filename ) {
    FILE *cgi_script_file;
    size_t result_size = 0;     /* Liczba wczytanych bajtóww z wyniku działania CGI */
    unsigned char *cgi_result;  /* Wynik działania CGI */
    unsigned char *cgi_body;    /* Wynik działania CGI bez nagłówków */
    char buf[ MAX_BUFFER ];     /* Wczytany wynik działania CGI */
    char *cgi_script_exec;      /* Nazwa pliku wykonywalnego CGI ( główny ) */
    char *exec_filename;        /* Nazwa pliku wykonywalnego CGI */
    char *cgi_param;            /* Parametr, z którym ma zostać uruchomiony plik wykonywalny CGI */
    char *add_hdr;              /* Uzyskane nagłówki z działania skryptu CGI */
    int valid_res = -1;         /* Przechowuje rezultat działania funkcji CGI_valid */
    long hdr_len = 0;           /* Rozmiar wczytanych nagłówków */
    int i = 0, j = 0;           /* Zmienne pomocnicze dla pętli */

    /* Alokacja pamięci na obiekty, w których będzie grzebać funkcja CGI_valid */
    exec_filename = malloc( MAX_PATH_LENGTH_CHAR );
    mem_allocated( exec_filename, 39 );

    cgi_param = malloc( STD_BUFF_SIZE_CHAR );
    mem_allocated( cgi_param, 40 );

    add_hdr = ( char* )calloc( BIG_BUFF_SIZE+1, sizeof( char ) );
    mem_allocated( add_hdr, 49 );

    /* Sprawdzenie typu pliku CGI i pobranie ew. nazwy programu zewnętrznego i parametru
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

    /* Alokacja pamięci na obiekt, który będzie zawierał nazwę zewnętrznego pliku
    wykonywalnego i ewentualnie parametr, z którym ma zostać uruchomiony */
    cgi_script_exec = malloc( MAX_PATH_LENGTH_CHAR+1 );
    mem_allocated( cgi_script_exec, 41 );

    /* Weryfikacja, czy żądany plik ( filename ) jest wykonywalny sam z siebie, czy
    trzeba użyć zewnętrznej aplikacji */
    if( valid_res == 1 ) {
        /* Plik jest wykonywalny sam z siebie */
        strncpy( cgi_script_exec, filename, MAX_PATH_LENGTH );
    } else if( valid_res == 2 ) {
        /* Plik wymaga zewnętrznej aplikacji */
        strncpy( cgi_script_exec, exec_filename, MAX_PATH_LENGTH );
        strncat( cgi_script_exec, " ", MAX_PATH_LENGTH );
        strncat( cgi_script_exec, filename, MAX_PATH_LENGTH );
    } else if( valid_res == 3 ) {
        /* Plik wymaga zewnętrznej aplikacji z parametrem */
        strncpy( cgi_script_exec, exec_filename, MAX_PATH_LENGTH );
        strncat( cgi_script_exec, " ", MAX_PATH_LENGTH );
        strncat( cgi_script_exec, filename, MAX_PATH_LENGTH );
        strncat( cgi_script_exec, " ", MAX_PATH_LENGTH );
        strncat( cgi_script_exec, cgi_param, MAX_PATH_LENGTH );
    }

    /* Ustawienie zmiennych systemowych */
    CGI_set_env_variables( http_session, filename );

    /* Stworzenie potoku ze skryptem CGI. Weryfikacja poprawności nazwy nastąpiła w funkcji
    CGI_valid. Odczyt danych w trybie binarnym. */
    cgi_script_file = popen( cgi_script_exec, READ_BINARY );

    //cgi_script_file = battery_fopen( cgi_script_exec, READ_BINARY, 1, http_session->socket_descriptor, SCRIPT );

    if( !cgi_script_file ) {
        /* Zwolnienie pamięci */
        if( cgi_script_exec ) {
            free( cgi_script_exec );
            cgi_script_exec = NULL;
        }

        if( exec_filename ) {
            free( exec_filename );
            exec_filename = NULL;
        }

        if( cgi_param ) {
            free( cgi_param );
            cgi_param = NULL;
        }

        /* Problem - plik został wcześniej odnaleziony w REQUEST_process, ale teraz się go nie da otworzyć */
        RESPONSE_error( http_session, HTTP_500_SERVER_ERROR, HTTP_ERR_500_MSG, NULL );
        return;
    }

    /* Wczytanie wyniku działania skryptu CGI i pobranie rozmiaru danych */
    result_size = fread( buf, sizeof( char ), MAX_BUFFER, cgi_script_file );

    /* Zamknięcie potoku */
    if( cgi_script_file) {
        pclose( cgi_script_file );
        cgi_script_file = NULL;
    }

    /* Weryfikacja rozmiaru outputu */
    if( result_size <= 0 ) {
        RESPONSE_error( http_session, HTTP_204_NO_CONTENT, HTTP_ERR_204_MSG, NULL );
        return;
    }

    /* Rezerwacja pamięci */
    cgi_result = ( unsigned char* )calloc( MAX_BUFFER, sizeof( char ) );
    mem_allocated( ( char* )cgi_result, 17 );

    cgi_body = ( unsigned char* )calloc( MAX_BUFFER, sizeof( char ) );
    mem_allocated( ( char* )cgi_body, 220 );

    /* Skopiowanie wczytanego wyniku z buf do cgi_result */
    for( i = 0, j = 0; i < result_size-1; i++, j++ ) {
        cgi_result[ j ] = buf[ i ];
    }

    strncpy( add_hdr, REQUEST_get_message_header( buf, result_size ), BIG_BUFF_SIZE );

    hdr_len = strlen( add_hdr );

    /* Wczytanie contentu */
    /* Użyty modyfikator 2 ( poniżej i w RESPONSE_header ) = "\r\n" = pusta linia między nagłówkami a contentem */
    for( i = hdr_len+2, j = 0; i < result_size-1; i++, j++ ) {
        cgi_body[ j ] = cgi_result[ i ];
    }

    /* Usunięcie ostatniego znaku */
    cgi_body[ result_size-hdr_len-1 ] = '\0';

    /* Pusta treść działania skryptu */
    if( strlen( ( char* )cgi_result ) == 0 ) {
        RESPONSE_error( http_session, HTTP_204_NO_CONTENT, HTTP_ERR_204_MSG, NULL );
    } else {
        /*Wysyłka */
        RESPONSE_header( http_session, HTTP_200_OK, NULL, result_size-hdr_len-2, ( char* )cgi_body, add_hdr );
        /*RESPONSE_header( http_session, HTTP_200_OK, NULL, result_size-hdr_len-2, NULL, add_hdr );

        send_struct = SESSION_find_response_struct_by_id( http_session->socket_descriptor );

        if( send_struct ) {
            send_struct->file = cgi_script_file;
            send_struct->http_content_size = result_size-hdr_len;
            send_struct->sent_size = 0;
        }*/

    }

    /* Zwalnianie pamięci */
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

static void CGI_set_env( const char *var, const char *val ) {
#ifdef _WIN32
    char env_var[ MAX_BUFFER ];
    sprintf( env_var, "%s=%s", var, val );
    putenv( env_var );
#else
    ( val == NULL ) ?   unsetenv( var ) : setenv( var, val, 1 );
#endif
}

/*
CGI_set_env_variables( HTTP_SESSION *http_session, const char *filename )
@http_session - wskaźnik do podłączonego klienta
@filename - pełna nazwa pliku skryptu
- funkcja ustawia zmienne środowiskowe, zgodnie ze specyfikacją CGI/1.1 ( RFC3875 )*/
static void CGI_set_env_variables( HTTP_SESSION *http_session, const char *filename ) {
    char str_res[ SMALL_BUFF_SIZE ];

    /* "Content-Length" i "Content-Type" są nieobecne w przypadku metody GET i HEAD */
    if( http_session->http_info.method_name == POST ) {
        sprintf( str_res, "%ld", http_session->http_info.content_length );
        CGI_set_env( "CONTENT_LENGTH", str_res );
        CGI_set_env( "CONTENT_TYPE", http_session->http_info.content_type );
    } else {
        /* Zerowanie zmiennych systemowych */
        CGI_set_env( "CONTENT_LENGTH", NULL );
        CGI_set_env( "CONTENT_TYPE", NULL );
    }

    CGI_set_env( "GATEWAY_INTERFACE", CGI_VER );

    /*sprintf( env_var,"PATH_INFO=%s", path_in );
    putenv( env_var );*/

    CGI_set_env( "SCRIPT_FILENAME", filename );

    CGI_set_env( "QUERY_STRING", http_session->http_info.query_string );

    CGI_set_env( "REMOTE_ADDR", http_session->http_info.remote_addr );

    CGI_set_env( "AUTH_TYPE", http_session->http_info.authorization == NULL ? NULL:"Basic" );

    CGI_set_env( "REMOTE_USER", http_session->http_info.user_login == NULL ? NULL:http_session->http_info.user_login );

    CGI_set_env( "REMOTE_IDENT", http_session->http_info.user_pwd == NULL ? NULL:http_session->http_info.user_pwd );

    CGI_set_env( "REMOTE_HOST", server_get_remote_hostname( http_session ) );

    CGI_set_env( "REQUEST_METHOD", http_method_list[ http_session->http_info.method_name ] );

    CGI_set_env( "SCRIPT_NAME", http_session->http_info.http_local_path );

    CGI_set_env( "SERVER_NAME", APP_NAME );

    sprintf( str_res, "%d", active_port );
    CGI_set_env( "SERVER_PORT", str_res );

    CGI_set_env( "SERVER_PROTOCOL", http_session->http_info.protocol_ver );

    CGI_set_env( "SERVER_SOFTWARE", SERVER_NAME );

    CGI_set_env( "REDIRECT_STATUS", "200" );

}

/*
CGI_load_configuration( void )*/
short CGI_load_configuration( const char *filename ) {
    FILE *cfg_file;
    char *buf;              /* Wczytana linia z pliku konfiguracyjnego */
    char *ext;              /* Wczytane rozszerzenie pliku z buf */
    char *exec_name;        /* Wczytana nazwa pliku wykonywalnego z buf */
    char *param;            /* Wczytany parametr dla pliku wykonywalnego z buf */
    struct stat file_stat;  /* Dla sprawdzenia właściwości pliku */
    int lines_count = 1;

    LOG_print( "Loading CGI script configuration..." );
    cfg_file = fopen( filename, "rt" );

    if( !cfg_file ) {
        LOG_print( "error.\n" );
        return 0;
    } else {
        LOG_print( "ok.\n" );
    }

    /* Alokacja pamięci */
    buf = malloc( STD_BUFF_SIZE_CHAR );
    mem_allocated( buf, 450 );

    ext = malloc( EXT_LEN_CHAR );
    mem_allocated( ext, 451 );

    exec_name = malloc( STD_BUFF_SIZE_CHAR );
    mem_allocated( exec_name, 452 );

    param = malloc( STD_BUFF_SIZE_CHAR );
    mem_allocated( param, 453 );

    while( fgets( buf, STD_BUFF_SIZE, cfg_file ) ) {
        if( ( sscanf( buf, "%s %s %s", ext, exec_name, param ) == 3 ) && ( cgi_other_count < 8 ) ) {
            /* Żądana opcja jest w pliku pod numerem 2 */
            strncpy( other_script_type[ cgi_other_count ].ext, ext, EXT_LEN );
            strncpy( other_script_type[ cgi_other_count ].external_app, exec_name, STD_BUFF_SIZE );
            strncpy( other_script_type[ cgi_other_count ].param, param, STD_BUFF_SIZE );

            /* Weryfikacja, czy plik pod zmienną exec_name jest wykonywalny */
            /* Jeżeli exec_name = <exec> plik sam z siebie jest plikiem wykonywalnym */
            if( strncmp( exec_name, STD_EXEC, STD_BUFF_SIZE ) != 0 ) {
                /* Sprawdzenie, czy plik istnieje */
                if( file_params( NULL, exec_name, NULL ) == 0 ) {
                    LOG_print( "Error: file not found ( %s ).\n", file_get_name( exec_name ) );
                    continue;
                }

                /* Pobranie właściwości pliku */
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

            /* Zwiększenie licznika wczytanych rozszerzeń */
            cgi_other_count++;
        } else {
            LOG_print( "\t- unknown command at line: %d\n", lines_count );
        }
        lines_count++;
    }

    /* Zamknięcie pliku konfiguracyjnego*/
    fclose( cfg_file );

    /* Zwalnianie pamięci */
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
@filename - nazwa pliku, który ma być zweryfikowany jako skrypt CGI
@valid_res - wskaźnik do int z funkcji wywołującej. Zwraca:
+ 0 - plik nie jest skryptem CGI
+ 1 - plik jest skryptem CGI i sam w sobie jest wykonywalny
+ 2 - plik jest skryptem CGI, ale potrzebuje zewnętrznej aplikacji
+ 3 - plik jest skryptem CGI, ale potrzebuje zewnętrznej aplikacji uruchamianej z parametrem
@exec_name - wskaźnik do char z funkcji wywołującej. Zwraca nazwę zewnętrznego programu wywołującego skrypt.
@param - wskaźnik do char z funkcji wywołującej. Zwraca parametr, z którym ma zostać wywołany zewnętrzny program. */
void CGI_valid( const char *filename, int *valid_res, char *exec_name, char *param ) {
    char *ext;
    int i = 0;

    *valid_res = 0;

    ext = malloc( EXT_LEN_CHAR );
    mem_allocated( ext, 37 );

    strncpy( ext, file_get_ext( filename ), EXT_LEN );

    /* Szukanie rozszerzenia w tablicy other_script_type */
    for( i = 0; i < cgi_other_count; ++i ) {
        /* Znaleziono pasujące rozszerzenie */
        if( strncasecmp( ext, other_script_type[ i ].ext, EXT_LEN ) == 0 ) {
            free( ext );
            ext = NULL;

            /* Przypisanie wartości do wywołanych argumentów:
                - nazwa zewnętrznej aplikacji */
            if( exec_name ) {
                strncpy( exec_name, other_script_type[ i ].external_app, MAX_PATH_LENGTH );
            }

            if( strncmp( other_script_type[ i ].external_app, STD_EXEC, STD_BUFF_SIZE ) == 0 ) {
                *valid_res = 1; /* Skrypt jest wykonywalny sam w sobie ( np. plik exe ) */
            } else {
                if( strncmp( other_script_type[ i ].param, STD_EXEC, STD_BUFF_SIZE ) == 0 ) {
                    *valid_res = 2; /* Potrzeba uruchomienia zewnętrznego programu */
                } else {
                    *valid_res = 3; /* Potrzeba uruchomienia zewnętrznego programu z parametrem */
                    /*  - parametr */
                    strncpy( param, other_script_type[ i ].param, STD_BUFF_SIZE );
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
