/*******************************************************************

Projekt battery-http-server

Plik: http_protocol.c

Przeznaczenie:
Przechowuje zmienne, stałe oraz funkcje używane przy pracy serwera w trybie HTTP

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/http_protocol.h"
#include "include/session.h"
#include "include/files_io.h"
#include "include/log.h"
#include "include/socket_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const char          *http_method_list[] = { "", "GET", "HEAD", "POST" };
SEND_INFO           send_d[ MAX_CLIENTS ];

/*
REQUEST_get_message_body( const char *content_data )
@http_session - wskaźnik do podłączonego klienta
- zwraca ciąg znaków będący message body. */
char* REQUEST_get_message_body( HTTP_SESSION *http_session ) {
    char *p1 = malloc( MAX_BUFFER_CHAR );
    static char body_result[ MAX_BUFFER ];

    memset( body_result, '\0', MAX_BUFFER );
    strncpy( p1, http_session->http_info.content_data, MAX_BUFFER );
    strncpy( body_result, p1+strspn( http_session->http_info.header, http_session->http_info.content_data ), MAX_BUFFER );

    free( p1 );
    p1 = NULL;

    return ( ( char* )&body_result );
}

/*
REQUEST_get_message_header( const char *content_data, long content_length )
@content_data - tresć całego żądania
@content_length - rozmiar żądania
- zwraca ciąg znaków będący nagłówkami żądania. */
char* REQUEST_get_message_header( const char *content_data, long content_length ) {
    char *hdr_line;
    char *temp_hdr_result;
    static char hdr_result[ BIG_BUFF_SIZE ];
    int i = 0;
    int j = 0;

    if( strstr( content_data, "\r\n\r\n" ) == NULL ) {
        /*sprintf( hdr_result, "Content-Type:application/x-httpd-php\r\n\r\n" );
        return ( char* )&hdr_result;*/
        return "";
    }
    /* Rezerwacja pamięci */
    hdr_line = ( char* )calloc( STD_BUFF_SIZE, sizeof( char ) );
    temp_hdr_result = ( char* )calloc( BIG_BUFF_SIZE+1, sizeof( char ) );

    for( i = 0; i < content_length; i++ ) {
        /* Skopiowanie znaków z wyniku do zmiennej przechowującej nagłówki */
        if( content_data[ i ]!= '\n' ) {
            hdr_line[ j ]= content_data[ i ];
            j++;
        } else {
            /* Napotkano koniec linii */
            if( strlen( hdr_line ) == 1 ) {
                /* Jeżeli długość linii = 1 to jest to pusta linia - po niej następuje content */
                break;
            }

            /* Jeżeli długość add_hdr = 0 to znaczy, że jest to pierwsza linia i trzeba ją przepisać...*/
            if( strlen( temp_hdr_result ) == 0 ) {
                strncpy( temp_hdr_result, hdr_line, BIG_BUFF_SIZE );
            } else {
                /* ...a tu dopisać */
                strncat( temp_hdr_result, hdr_line, BIG_BUFF_SIZE );
            }

            strncat( temp_hdr_result, "\n", BIG_BUFF_SIZE );
            memset( hdr_line, '\0', STD_BUFF_SIZE );
            j = 0;
        }
    }

    free( hdr_line );
    hdr_line = NULL;

    memset( hdr_result, '\0', BIG_BUFF_SIZE );
    strncpy( hdr_result, temp_hdr_result, BIG_BUFF_SIZE );

    free( temp_hdr_result );
    temp_hdr_result = NULL;

    return ( ( char* )&hdr_result );
}


/*
REQUEST_get_query( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- zwraca:
+ przy metodzie GET ciąg znaków znajdujący się za znakiem "?" w URI
+ przy metodzie POST zawartość żądania */
char* REQUEST_get_query( HTTP_SESSION *http_session ) {
    char *result = NULL;

    /* Metoda GET - query string zaczyna się od znaku "?" */
    if( http_session->http_info.method_name == GET ) {
        result = strchr( http_session->http_info.http_local_path, '?' )+1;
    } else if( http_session->http_info.method_name == POST ) {
        /* Metoda POST - query string to message body */
        result = REQUEST_get_message_body( http_session );
    }

    if( !result ) {
        return "";
    } else {
        return result;
    }
}

/*
REQUEST_get_mime_type( const char *filename )
@filename - nazwa pliku, po rozszerzeniu której będzie przydzielany mime-type
- zwraca char* z typ MIME na podstawie tablicy mime_types ( server_shared.h ) wczytanej z pliku mime_type.cfg. */
char* REQUEST_get_mime_type( const char *filename ) {
    int i = 0;
    char *ext = file_get_ext( filename );

    for( i = 0; i < mime_types_count; ++i ) {
        if( strncasecmp( ext, mime_types[ i ].ext, EXT_LEN ) == 0 ) {
            return mime_types[ i ].mime_type;
        }
    }

    return NULL;
}

/*
REQUEST_get_header_value( const char *header, const char *requested_value_name )
@header - cały nagłówek żądania, w którym będzie wyszukiwana żądana wartość
@requested_value_name - żądana wartość, która będzie wyszukana w nagłówku
- zwraca ciąg znaków z wartością, która odpowiada nagłówkowi requested_value_name. */
char* REQUEST_get_header_value( const char *header, const char *requested_value_name ) {
    char *result_handler;                   /* Przechowuje całą linię z żądanym nagłówkiem */
    static char result[ STD_BUFF_SIZE ];    /* Zwracana przez funkcję wartość */
    char *dst;
    char *tmp_header_val;                   /* wskaźnik do początku żądanego nagłówka */
    int count = 0;

    /* Skopiowanie do result_handler zawartości hdr_handler od momentu, gdzie zaczyna się requested_value_name */
    tmp_header_val = strstr( ( char* )header, requested_value_name );
    if( !tmp_header_val ) {
        /* Brak żądanego nagłówka - zwracamy standardowy text/html */
        return "";
    }

    /* Alokacja pamięci */
    result_handler = malloc( BIG_BUFF_SIZE );
    dst = ( char* )calloc( STD_BUFF_SIZE, sizeof( char ) );

    strncpy( result_handler, tmp_header_val, BIG_BUFF_SIZE );
    tmp_header_val = NULL;

    /* Skopiowanie do dst pierwszej linii z result_handler */
    while( ( dst[ count ] = result_handler[ count ] ) != '\015' ) {
        if( result_handler[ count ] != '\015' ) {
             count++;
        }
    }
    dst[ count ]= '\0';

    free( result_handler );
    result_handler = NULL;

    /* Pobranie zawartości dst od momentu wystąpienia ": ", czyli rzeczywistej wartości nagłówka */
    memset( result, '\0', STD_BUFF_SIZE );
    strncpy( result, strstr( dst, ": " )+2, STD_BUFF_SIZE );

    free( dst );
    dst = NULL;

    return ( ( char* )&result );
}

/*
RESPONSE_header( HTTP_SESSION *http_session, const char *http_status_code, const char *http_mime_type, size_t http_content_length, char *time_last_modified, const char *content_data, const char* add_headers )
@http_session - wskaźnik do połączonego klienta
@http_status_code - kod odpowiedzi
@http_mime_type - typ mime
@http_content_length - rozmiar wysyłanych danych
@time_last_modified - data ostatniej modyfikacji pliku ( 0 Jeżeli nagłówek Last-Modified ma nie zostać dołączony )
@content_data - treść wysyłanej wiadomości
@add_headers - dodatkowe nagłówki ( ustawione w funkcji wowołującej i przekazane przez ten parametr )
- główna funkcja wysyłająca odpowiedź do klienta: header + ewentualny content */
void RESPONSE_header( HTTP_SESSION *http_session, const char *http_status_code, const char *http_mime_type, size_t http_content_length, const char *content_data, const char* add_headers ) {
    char *http_header_to_send;      /* Przechowuje treść HEADER */
    char *http_single_header_line;  /* Pojedyncza linia nagłówka */

    /* Alokacja pamięci */
    http_header_to_send = malloc( MAX_BUFFER_CHAR+1 );
    mem_allocated( http_header_to_send, 28 );
    http_single_header_line = malloc( STD_BUFF_SIZE_CHAR );
    mem_allocated( http_single_header_line, 29 );

    /* HTTP/1.1 KOD OPIS */
    sprintf( http_single_header_line, "%s %s\r\n", HTTP_VER, http_status_code );
    strncpy( http_header_to_send, http_single_header_line, MAX_BUFFER );

    /* Date: */
    sprintf( http_single_header_line, "%s%s\r\n", HEADER_DATE, get_actual_time_gmt() );
    strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );

    /* Server: */
    sprintf( http_single_header_line, "%s\r\n", HEADER_SERVER );
    strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );

    /* Accept Ranges: bytes */
    strncat( http_header_to_send, HEADER_ACCEPT_RANGES, MAX_BUFFER );

    /* Content-Type: */
    if( http_mime_type ) {
        sprintf( http_single_header_line, "%s%s\r\n", HEADER_CONTENT_TYPE, http_mime_type );
        strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );
    }

    /* Connection: Keep-Alive */
    if( http_session->http_info.keep_alive ) {
        sprintf( http_single_header_line, "%s%s\r\n", HEADER_CONNECTION, HEADER_KEEP_ALIVE_STR );
        strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );
    }

    /* Content-Length: */
    if( http_content_length != 0 ) {
        sprintf( http_single_header_line,"%s%zu\r\n", HEADER_CONTENT_LENGTH, http_content_length );
        strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );
    }

    /* Last-Modified: */
    if( http_session->local_info.date_res_last_modified ) {
        sprintf( http_single_header_line, "%s%s\r\n", HEADER_LAST_MODIFIED, http_session->local_info.date_res_last_modified );
        strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );
    }

    /* Dodatkowe nagłówki? */
    if( add_headers ) {
        strncat( http_header_to_send, add_headers, MAX_BUFFER );
    }

    /* Pusta linia - po niej zaczyna się content */
    strncat( http_header_to_send, "\r\n", MAX_BUFFER );

    /* Wysyłka nagłówka HTTP */
    if( SESSION_send_response( http_session, http_header_to_send, strlen( http_header_to_send ) ) > 0 ) {
        /* Wysyłka contentu */
        if( content_data ) {
            SESSION_send_response( http_session, content_data, http_content_length );
        }
    }

    /* Zapis żądania do logu */
    LOG_print( "%s %s %s \"%s\" %s %.3s\n", get_actual_time_gmt(), http_session->http_info.remote_addr, http_method_list[ http_session->http_info.method_name ], http_session->http_info.http_local_path, http_session->http_info.protocol_ver, http_status_code );

    /* Zwolnienie pamięci */
    free( http_header_to_send );
    http_header_to_send = NULL;

    free( http_single_header_line );
    http_single_header_line = NULL;
}

/*
RESPONSE_error( HTTP_SESSION *http_session, const char *http_status_code, const char *http_error_message, const char* add_headers )
@http_session - wskaźnik do podłączonego klienta
@http_status_code - kod błędu
@http_error_message - treść błędu, która wyświetli się w przeglądarce
@add_headers - dodatkowe nagłówki ( ustawione w funkcji wowołującej i przekazane przez ten parametr )
- funkcja wysyła do przeglądarki ( klienta ) informację o błędzie, który wystąpił na skutek żądania */
void RESPONSE_error( HTTP_SESSION *http_session, const char *http_status_code, const char *http_error_message, const char* add_headers ) {
    char *http_header_to_send;
    char *http_single_header_line;
    int len = 0;

    /* Alokacja pamięci */
    http_header_to_send = malloc( MAX_BUFFER_CHAR+1 );
    mem_allocated( http_header_to_send, 30 );
    http_single_header_line = malloc( STD_BUFF_SIZE_CHAR );
    mem_allocated( http_single_header_line, 31 );

    /* HTTP/1.X XXX OPIS */
    sprintf( http_single_header_line, "%s %s\r\n", HTTP_VER, http_status_code );
    strncpy( http_header_to_send, http_single_header_line, MAX_BUFFER );

    /* Server: */
    sprintf( http_single_header_line, "%s\r\n", HEADER_SERVER );
    strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );

    /* Connection: Keep-Alive */
    if( http_session->http_info.keep_alive ) {
        sprintf( http_single_header_line, "%s%s\r\n", HEADER_CONNECTION, HEADER_KEEP_ALIVE_STR );
        strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );
    }

    /* Accept Ranges: bytes */
    strncat( http_header_to_send, HEADER_ACCEPT_RANGES, MAX_BUFFER );

    /* Dodatkowe nagłówki */
    if( add_headers ) {
        strncat( http_header_to_send, add_headers, MAX_BUFFER );
    }

    /* Content-Length */
    len = strlen( http_error_message );
    sprintf( http_single_header_line, "%s%d\r\n", HEADER_CONTENT_LENGTH, len );
    strncat( http_header_to_send, http_single_header_line, MAX_BUFFER );

    /* Pusta linia */
    strncat( http_header_to_send, "\r\n", MAX_BUFFER );

    /* Wysyłka nagłówka HTTP */
    SESSION_send_response( http_session, http_header_to_send, strlen( http_header_to_send ) );

    /* Wysyłka treści błędu, jeżeli istnieje */
    if( http_error_message ) {
        SESSION_send_response( http_session, http_error_message, len );
    }

    /* Zapis żądania do logu */
    LOG_print( "%s %s %s \"%s\" %s %.3s\n", get_actual_time_gmt(), http_session->http_info.remote_addr, http_method_list[ http_session->http_info.method_name ], http_session->http_info.http_local_path, http_session->http_info.protocol_ver, http_status_code );

    /* Zwolnienie pamięci */
    free( http_header_to_send );
    http_header_to_send = NULL;

    free( http_single_header_line );
    http_single_header_line = NULL;
}

/*
RESPONSE_file( HTTP_SESSION *http_session, const char *filename )
@http_session - wskaźnik do podłączonego klienta
@filename - nazwa pliku, który ma został przesłany
- funkcja wysyła do przeglądarki żądany zasób lub jego fragment */
void RESPONSE_file( HTTP_SESSION *http_session, const char *filename ) {
    FILE *file;
    long filesize = 0;      /* Całkowity rozmiar żądanego pliku */
    long total = 0;         /* Całkowity rozmiar wysyłanych danych */
    char *buf;
    char *add_hdr;          /* Opcjonalne nagłówki */
    SEND_INFO *send_struct;

    /* Otwarcie pliku. Weryfikacja poprawności jego nazwy nastąpiła poprzez funkcję
    file_params w nadrzędnej funkcji REQUEST_process */
    file = battery_fopen( filename, READ_BINARY, 1, http_session->socket_descriptor, STD_FILE );

    /* Nie udało się otworzyć pliku, choć istnieje - problem z serwerem? */
    if( !file ) {
        RESPONSE_error( http_session, HTTP_500_SERVER_ERROR, HTTP_ERR_500_MSG, NULL );
    } else {
        /* Sprawdzenie, czy istnieje nagłówek "If-Modified-Since" */
        if( http_session->http_info.date_if_modified_since ) {
            /* Porównanie nagłówka "If-Modified-Since" z datą modyfikacji pliku */
            if( strcmp( http_session->local_info.date_res_last_modified, http_session->http_info.date_if_modified_since ) == 0 ) {
                battery_fclose( file, http_session->socket_descriptor );
                RESPONSE_header( http_session, HTTP_304_NOT_MODIFIED, HEADER_STD_CONTENT_TYPE, 0, NULL, NULL );
                return;
            }
        }

        /* Sprawdzenie, czy istnieje nagłówek "If-Unmodified-Since" */
        if( http_session->http_info.date_if_unmodified_since ) {
            /* Porównanie nagłówka "If-Unmodified-Since" z datą modyfikacji pliku */
            if( strcmp( http_session->local_info.date_res_last_modified, http_session->http_info.date_if_unmodified_since ) != 0 ) {
                battery_fclose( file, http_session->socket_descriptor );
                RESPONSE_header( http_session, HTTP_412_PRECONDITION_FAILED, HEADER_STD_CONTENT_TYPE, 0, NULL, NULL );
                return;
            }
        }

        /* Pobranie rozmiaru pliku */
        filesize = battery_ftell( file );

        /* Plik jest pusty = błąd 204 */
        if( filesize <= 0 ) {
            RESPONSE_error( http_session, HTTP_204_NO_CONTENT, HTTP_ERR_204_MSG, NULL );
        } else {
            /* Nie wybrano żadnego fragmentu pliku - brak nagłówka "Range" */
            if( ( http_session->http_info.range_st < 0 )&&( http_session->http_info.range_en < 0 ) ) {
                /* Wysyłka z kodem 200 - wszystko ok */
                RESPONSE_header( http_session, HTTP_200_OK, REQUEST_get_mime_type( filename ), filesize, NULL, NULL );

                send_struct = SESSION_find_response_struct_by_id( http_session->socket_descriptor );

                if( send_struct ) {
                    send_struct->file = file;
                    send_struct->http_content_size = filesize;
                    send_struct->total_size = filesize;
                    send_struct->sent_size = 0;
                }
            } else {
                /* Wysyłka wybranego fragmentu pliku */
                /* Jeżeli zakres końcowy jest mniejszy od 0 ( np. -1 ) to ustawiamy go jako rozmiar pliku */
                if( http_session->http_info.range_en <= 0 ) {
                    http_session->http_info.range_en = filesize;
                }

                /* Całkowity rozmiar fragmentu */
                http_session->http_info.range_en--; /* bajt zerowy! */
                total = http_session->http_info.range_en - http_session->http_info.range_st;

                /* Brak podanego zakresu w pliku lub błędny zakres = błąd 416 */
                if( ( fseek( file, http_session->http_info.range_st, SEEK_SET ) != 0 ) || ( total <= 0 ) ) {
                    RESPONSE_error( http_session, HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE, HTTP_ERR_416_MSG, NULL );
                } else {
                    /* Dodanie nagłówka "Content-Range" */
                    add_hdr = malloc( STD_BUFF_SIZE_CHAR );
                    mem_allocated( add_hdr, 32 );
                    sprintf( add_hdr, "%sbytes %ld-%ld/%ld\r\n", HEADER_CONTENT_RANGE, http_session->http_info.range_st, http_session->http_info.range_en, filesize );
                    /* Wysyłka z kodem 206 - wybrany fragment zasobu */
                    RESPONSE_header( http_session, HTTP_206_PARTIAL_CONTENT, REQUEST_get_mime_type( filename ), total+1, NULL, add_hdr );
                    free( add_hdr );
                    add_hdr = NULL;

                    /* Wczytanie fragmentu i wysyłka */
                    buf = malloc( total );
                    mem_allocated( buf, 33 );

                    if( fread( buf, sizeof( char ), total, file ) > 0 ) {
                        SESSION_send_response( http_session, buf, total+1 );
                    } else {
                        /* Napotkano błąd = 500 */
                        RESPONSE_error( http_session, HTTP_500_SERVER_ERROR, HTTP_ERR_500_MSG, NULL );
                    }

                    free( buf );
                    buf = NULL;
                }
            }
        }
    }
}

/*
REQUEST_process( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- po udanej weryfikacji danych przechodzi do próby przesłania żądanego zasobu */
void REQUEST_process( HTTP_SESSION *http_session ) {
    char *local_file_path;      /* Lokalna ścieżka do żądanego pliku */
    char *file_ext;             /* Rozszerzenie żądanego pliku */
    char *add_hdr;              /* Opcjonalne nagłówki */
    char *ht_access_pwd;        /* Hasło do żądanego zasobu z funkcji file_params*/
    int len_loc;                /* Długość żądanej ścieżki */
    int file_params_val = 0;    /* Przechowuje wynik działania funkcji file_params */

    len_loc = strlen( http_session->http_info.http_local_path );
    /* Przygotowanie pamięci na nazwę żądanego pliku */
    local_file_path = malloc( MAX_PATH_LENGTH_CHAR+1 );
    mem_allocated( local_file_path, 34 );

    /* Połączenie ścieżki z żądania z pełną ścieżką w systemie*/
    if( len_loc > 1 ) {
        /* Stworzenie lokalnej ścieżki dostępu do zasobu */
        strncpy( local_file_path, app_path, MAX_PATH_LENGTH );
        strncat( local_file_path, http_session->http_info.http_local_path, MAX_PATH_LENGTH );
    } else {
        /* Jesteśmy w głównym katalogu */
        strncpy( local_file_path, app_path, MAX_PATH_LENGTH );
    }

    /* Zamiana znaku "/" z request na "\" - tylko Win32*/
    strrepchar( local_file_path, '/', C_SLASH );

    /* Usunięcie podwójnych znaków slash */
    strdelbslash( local_file_path );

    /* Jeżeli podano samą nazwę katalogu to automatycznie dodajemy plik indeksu - tu juz po weryfikacji, czy zasób jest skryptem CGI */
    if( strncmp( file_get_name( local_file_path ), "", 1 ) == 0 ) {
        strncat( local_file_path, REQUEST_get_index( local_file_path ), MAX_PATH_LENGTH );
    }

    if( ht_access_count > 0 ) {
        /* Alokacja pamięci */
        ht_access_pwd = malloc( STD_BUFF_SIZE_CHAR );
        mem_allocated( ht_access_pwd, 35 );
    }

    /*Sprawdzamy, czy żądany plik istnieje... */
    file_params_val = file_params( http_session, local_file_path, (ht_access_count > 0 ? ht_access_pwd : NULL ) );
    if( file_params_val == 0 ) { /* Plik nie istnieje */
        file_ext = malloc( MICRO_BUFF_SIZE_CHAR );
        strncpy( file_ext, file_get_ext( local_file_path ), MICRO_BUFF_SIZE );
        /*...brak rozszerzenia = nie podano konkretnego pliku w URL... */
        if( strncmp( file_ext, "", MICRO_BUFF_SIZE ) == 0 ) {
            /*...sprawdzamy, czy ostatni znak to "/"... */
            if( local_file_path[ strlen( local_file_path )-1 ] != C_SLASH ) {
                /*...mimo wszystko sprawdzamy, czy ścieżka jest prawidłowa. Tak = błąd 302. Nie = błąd 400. */
                if( directory_exists( local_file_path ) ) {
                    /* Stworzenie dodatkowej informacji do nagłówka wysyłanego przez RESPONSE_error z prawidłową ścieżką */
                    add_hdr = malloc( MAX_PATH_LENGTH_CHAR );
                    sprintf( add_hdr, "%s%s/\r\n", HEADER_LOCATION, http_session->http_info.http_local_path );
                    RESPONSE_error( http_session, HTTP_302_FOUND, HTTP_ERR_302_MSG, add_hdr );
                    free( add_hdr );
                    add_hdr = NULL;
                } else {
                    RESPONSE_error( http_session, HTTP_400_BAD_REQUEST, HTTP_ERR_400_MSG, NULL );
                }
            } else {
                /*...plik nie istnieje, a index.html brak... */
                RESPONSE_error( http_session, HTTP_404_NOT_FOUND, HTTP_ERR_404_MSG, NULL );
            }
        } else {
            RESPONSE_error( http_session, HTTP_404_NOT_FOUND, HTTP_ERR_404_MSG, NULL );
        }

        /* Zwolnienie pamięci dla rozszerzenia żądanego pliku */
        free( file_ext );
        file_ext = NULL;
    } else {
        /* Plik istnieje, ale jakie ma właściwości ? */
        if( file_params_val == 1 ) {
            /* Plik istnieje i jest do odczytu */
            /* Jeżeli jest to skrypt CGI, to zostaje wykonany */
            /* Zwykłe żądanie zasobu */
            /* Zapytanie zweryfikowane - wysyłka zawartości zasobu */
            if( http_session->http_info.method_name != POST ) {
                RESPONSE_file( http_session, local_file_path ); /* Zapytanie HEAD jest weryfikowane w funkcji RESPONSE_header */
            }
        } else if( file_params_val == 2 ) {/* Plik istnieje, ale dost�p jest zabroniony */
            RESPONSE_error( http_session, HTTP_403_FORBIDDEN, HTTP_ERR_403_MSG, NULL );
        } else if( file_params_val == 3 ) {/* Plik istnieje, ale wymaga autoryzacji */
            if( http_session->http_info.authorization ) {
                if( strncmp( ht_access_pwd, http_session->http_info.authorization, STD_BUFF_SIZE ) == 0 ) {
                    /* Zwykłe żądanie zasobu */
                    /* Zapytanie zweryfikowane - wysyłka zawartości zasobu */
                    if( http_session->http_info.method_name != POST ) {
                        RESPONSE_file( http_session, local_file_path ); /* Zapytanie HEAD jest weryfikowane w funkcji RESPONSE_header */
                    } else {
                        RESPONSE_error( http_session, HTTP_401_AUTHORIZATION_REQUIRED, HTTP_ERR_401_MSG, HEADER_AUTHENTICATION );
                    }
                } else {
                    RESPONSE_error( http_session, HTTP_401_AUTHORIZATION_REQUIRED, HTTP_ERR_401_MSG, HEADER_AUTHENTICATION );
                }
            } else {
                RESPONSE_error( http_session, HTTP_401_AUTHORIZATION_REQUIRED, HTTP_ERR_401_MSG, HEADER_AUTHENTICATION );
            }
        }
    }

    if( ht_access_count > 0) {
        /* Zwolnienie pamięci dla hasła zasobu */
        if( ht_access_pwd ) {
            free( ht_access_pwd );
            ht_access_pwd = NULL;
        }
    }

    /* Zwolnienie pamięci dla nazwy żądanego pliku */
    free( local_file_path );
    local_file_path = NULL;

    /* Zwalniamy pamięć */
    SESSION_release( http_session );
}

/*
REQUEST_get_cgi_name( const char* http_local_path )
@http_local_path - URI przekazane w żądaniu
- zwraca ciąg znaków będący nazwą skryptu. */
char* REQUEST_get_cgi_name( HTTP_SESSION *http_session ) {
    static char result[ MAX_PATH_LENGTH ];
    int len = strlen( http_session->http_info.http_local_path );

    /* Metody GET i HEAD */
    if( http_session->http_info.method_name != POST ) {
        memset( result, '\0', MAX_PATH_LENGTH );
        strncpy( result, http_session->http_info.http_local_path, len );

        /* Usunięcie reszty znaków po znaku "?" */
        if( strstr( result, "?" ) ) {
            result[ strpos( result, "?" ) ] = '\0';
        }
        return ( ( char* )&result );
    } else {
        /* Metoda POST */
        return http_session->http_info.http_local_path;
    }
}

/*
REQUEST_get_range( HTTP_SESSION *http_session, int type )
@http_session - wskaźnik do podłączonego klienta
@type - określa, czy funkcja ma pobrać i zwrócić początkowy lub końcowy zakres z nagłówka "Range"
- zwraca liczbę bajtów ( początkowych lub końcowych ) dla żądania fragmentu zasobu */
long REQUEST_get_range( HTTP_SESSION *http_session, int type ) {
    char *range;
    char *temp_r;
    char *ptr;
    long range_s = -1;
    int i = 0;
    int len = 0;

    /* Pobranie wartości nagłówka "Range" do zmiennej tymczasowej */
    ptr = malloc( STD_BUFF_SIZE_CHAR );
    strncpy( ptr, REQUEST_get_header_value( http_session->http_info.header, HEADER_RANGE ), STD_BUFF_SIZE );
    if( strlen( ptr ) == 0 ) { /* Nagłówek nie istnieje */
        free( ptr );
        ptr = NULL;
        return ( -1 );
    }

    /* Rezerwacja pamięci */
    range = malloc( SMALL_BUFF_SIZE_CHAR );
    mem_allocated( range, 36 );

    temp_r = malloc( SMALL_BUFF_SIZE_CHAR );
    mem_allocated( temp_r, 37 );

    /* Przypisanie do zmiennej range wartości zmiennej tymczasowej */
    strncpy( range, ptr, SMALL_BUFF_SIZE );
    free( ptr );
    ptr = NULL;

    /* Funkcja wywołana w celu sprawdzenia zakresu początkowego */
    if( type == 0 ) {
        strncpy( temp_r, strstr( range, "=" ), SMALL_BUFF_SIZE );
        len = strlen( temp_r );
        temp_r[ len-1 ] = '\0';

        for( i = 0; i < len; ++i ) {
            if( i < len ) {
                temp_r[ i ] = temp_r[ i+1 ] ;
            }
        }
    } else if( type == 1 ) {
        /* Funkcja wywołana w celu sprawdzenia zakresu końcowego */
        strncpy( temp_r, strrchr( range, '-' ), SMALL_BUFF_SIZE );
        len = strlen( temp_r );

        /* Usunięcie znaku "-" pozostałego po strrchr() */
        for( i = 0; i < len; ++i ) {
            if( i < len ) {
                temp_r[ i ] = temp_r[ i+1 ];
            }
        }
    }

    /* Konwersja ciągu znaków na liczbę */
    range_s = atoi( temp_r );

    free( range );
    range = NULL;

    free( temp_r );
    temp_r = NULL;

    return range_s;
}

/*
REQUEST_get_index( const char *path )
@path - ścieżka, w której ma zostać odszukany index ( z listy index_file_list )
- zwraca ciąg znaków z plikiem index, który został odnaleziony w katalogu */
char* REQUEST_get_index( const char *path ) {
    char filename[ MAX_PATH_LENGTH+1 ];
    int i = 0;

    /* Sprawdzenie, czy w podanym katalogu znajduje się któryś z wczytanych plików index */
    for( i = 0; i < index_file_count; ++i ) {
        strncpy( filename, path, MAX_PATH_LENGTH );
        strncat( filename, index_file_list[ i ], MAX_PATH_LENGTH );
        if( file_exists( filename ) == 1) {
            return index_file_list[ i ];
        }
    }

    /* Jeżeli nie znaleziono żadnego z plików index_file_list to zwracamy SITE_INDEX */
    return SITE_INDEX;
}
