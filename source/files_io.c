/*******************************************************************

Projekt battery-http-server

Plik: files_io.c
Przeznaczenie:
Zbiór funkcji przeznaczonych do obsługi plików i katalogów

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/files_io.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#ifndef _MSC_VER
    #include <unistd.h>
#endif

OPENED_FILE opened_files[ FOPEN_MAX ];

/*
get_app_path( void )
- zwraca ciąg znaków - folder startowy aplikacji */
char* get_app_path( void ) {
    static char buf[ MAX_PATH_LENGTH ];
    if( getcwd( buf, MAX_PATH_LENGTH ) ) {
        return strncat( buf, SLASH, MAX_PATH_LENGTH );
    } else {
        return "";
    }
}

/*
directory_exists( const char *path )
@path - ścieżka, która ma zostać sprawdzona
- funkcja próbuje ustawić katalog roboczy na ścieżkę podaną w zmiennej path
- zwraca int, gdzie 0 = ścieżka nie istnieje, 1 = ścieżka istnieje */
short directory_exists( const char *path ) {
    if( chdir( path ) == 0 ) {
        return 1;
    } else {
        return 0;
    }
}

/*
file_get_name( const char *full_filename )
@full_filename - pełna nazwa pliku ( +ścieżka )
- pobiera nazwę pliku z pełnej nazwy ( +ścieżka )
- zwraca char *z nazwą pliku */
char* file_get_name( const char *full_filename ) {
    char *filename;

    if( strstr( full_filename, SLASH ) ) {
        filename = ( char* )strrchr( full_filename, C_SLASH );
    } else {
        filename = ( char* )strrchr( full_filename, '/' );
    }

    /*Usunięcie znaku "\" z początku filename */
    if( filename ) {
        return ++filename;
    }

    return "";
}

/*
file_get_ext( const char *filename )
@filename - nazwa pliku lub pełna nazwa pliku ( +ścieżka )
- pobiera rozszerzenie z podanej nazwy pliku
- zwraca char *z rozszerzeniem */
char* file_get_ext( const char *filename ) {
    char* file_ext = strrchr( file_get_name( filename ), '.' );

    if( !file_ext ) {
        return "";
    }
    return file_ext;
}

/*
file_params( const char *filename )
@filename - nazwa pliku ( +ścieżka )
- sprawdza, czy podany w zmiennej filename plik istnieje
- próbuje otworzyć plik
- zwraca int, gdzie:
+ 0 = nie istnieje
+ 1 = istnieje, jest do odczytu, nie wymaga autentykacji
+ 2 = istnieje, brak uprawnień do odczytu
+ 3 = istnieje, wymagana autentykacja */
short file_params( HTTP_SESSION *http_session, const char *filename, char *ht_access_pwd ) {
    FILE *resource;
    struct stat file_stat;
    int tmp_socket;
    int i = 0;

    /* Weryfikacja, czy podany parametr jest prawidłową nazwę pliku */
    stat( filename, &file_stat );
    if( file_stat.st_mode & S_IFREG );
    else {/* Nie jest... */
        return 0;
    }

    tmp_socket = ( http_session ? http_session->socket_fd : -133 );
    /* Sprawdza, czy udało się otworzyć plik */
    resource = battery_fopen( filename, READ_BINARY, 0, tmp_socket, STD_FILE );

    if( !resource ) {
        return 0;
    } else {
        if( http_session == NULL ) {
            return 1;
        }

        /* Pobranie informacji o ostatniej dacie modyfikacji zasobu */
        if( !http_session->local_info.date_res_last_modified ) {
            http_session->local_info.date_res_last_modified = malloc( TIME_BUFF_SIZE_CHAR );
            mem_allocated( http_session->local_info.date_res_last_modified, 20 );
        }

        strftime( http_session->local_info.date_res_last_modified, TIME_BUFF_SIZE, RFC1123FMT, gmtime( &file_stat.st_mtime ) );

        /* Sprawdza, czy plik ma uprawnienia do odczytu */
        if( file_stat.st_mode & S_IREAD ) {
            if( ht_access_pwd && ht_access_count > 0 ) {
                for( i = 0; i < ht_access_count; i++ ) {
                    if( strncmp( ht_access[ i ].res_filename, filename, MAX_PATH_LENGTH ) == 0 ) {
                        /* Zasób wymaga autoryzacji */
                        strncpy( ht_access_pwd, ht_access[ i ].res_auth, STD_BUFF_SIZE );

                        return 3;
                    }
                }
            }
        } else {
            /* Nie ma */
            return 2;
        }
    }

    return 1;
}

/*
file_exists( const char *filename )
@filename - nazwa szukanego pliku
- zwraca int, gdzie 1 = znaleziono plik. */
short file_exists( const char *filename ) {
    FILE *resource; /* Uchwyt do pliku */

    if( ( resource = battery_fopen( filename, READ_BINARY, 1, -133, STD_FILE ) ) ) {
        /* Udało się otworzyć plik = istnieje */
        return 1;
    } else {
        /* Nie istnieje */
        return 0;
    }
}

/*
file_extract_path( const char *full_filename, char delim )
@full_filename - ścieżka dostępu do pliku, z której będzie pobrana sama ścieżka
@delim - znak, od którego ma zostać "obcięta" ścieżka
- zwraca ciąg znaków, który jest wyciętą ścieżką z pełnej ścieżki.
Rezultat należy później zwolnić poprzez funkcję free(). */
void file_extract_path(char *full_filename, char delim)
{
    int i = strlen(full_filename);

    if(i > 0) {
        while(--i) {
            if(full_filename[ i ] == delim) {
                full_filename[ i+1 ] = '\0';
            }
        }
    }
}

/*
battery_fopen( const char *filename, const char *mode, short add_to_list, int socket_fd )
@filename - nazwa pliku do otwarcia
@mode - tryb czytania pliku
@add_to_list - definiuje, czy plik ma zostać dodany do listy otwartych przez serwer plików
@socket_fd - powiązanie otwieranego pliku (tworzonej struktury) z podłączonym klientem
- funkcja weryfikuje, czy żądany plik jest już otwarty przez serwer */
FILE *battery_fopen( const char *filename, const char *mode, short add_to_list, int socket_fd, RESOURCE_TYPE type ) {
    int i;
    FILE *tmp = NULL;

    /* Weryfikacja, czy plik jest już otwarty przez serwer */
    for( i = 0; i < FOPEN_MAX; i++ ) {
        if( strlen( opened_files[ i ].filename ) ) {
            if( strcmp( opened_files[ i ].filename, filename ) == 0 && type == opened_files[ i ].type ) {
                tmp = opened_files[ i ].file;
                break;
            }
        }
    }

    /* Jeżeli nie jest - otwórz */
    if( tmp == NULL ) {

        tmp = ( type == STD_FILE ? fopen( filename, mode ) : popen( filename, mode ) );
    }

    if( tmp ) {
        if( add_to_list == 0) {
            return tmp;
        } else {
            fseek( tmp, 0, SEEK_END );

            for(i = 0; i < FOPEN_MAX; i++ ) {
                /* Dodanie informacji o otwartym pliku w pierwszym wolnym elemencie */
                if( opened_files[ i ].file == NULL ) {
                    opened_files[ i ].file = tmp;
                    opened_files[ i ].socket_fd = socket_fd;
                    strncpy( opened_files[ i ].filename, filename, FILENAME_MAX );
                    opened_files[ i ].size = ftell( tmp );
                    opened_files[ i ].type = type;
                    opened_files[ i ].content = NULL;

                    opened_files[ i ].content = calloc( opened_files[ i ].size+1, sizeof( char ) );
                    if( opened_files[ i ].content == NULL ) {
                        printf( "Error: unable to create cache for file %s.\n", filename );
                    } else {
                        rewind( tmp );
                        if( fread( opened_files[ i ].content, opened_files[ i ].size, 1, tmp ) == 1 ) {
                            opened_files[ i ].content[ opened_files[ i ].size ] = '\0';
                            printf("[CACHE] File %s loaded to memory.\n", filename );
                        } else {
                            printf( "Error: unable load file %s to memory.\n", filename );
                        }
                    }
                    break;
                }
            }

            return tmp;
        }
    }

    return NULL;
}

/*
battery_ftell( FILE *file )
@file - wskaźnik do otwartego pliku
- funkcja zwraca rozmiar żądanego pliku */
long battery_ftell( FILE *file ) {
    int i = FOPEN_MAX;

    for( i = 0; i <= FOPEN_MAX-1; i++ ) {
        if( opened_files[ i ].file == file ) {
            return opened_files[ i ].size;
        }
    }

    return 0;
}

/*
battery_get_filename( FILE *file )
@file - wskaźnik do otwartego pliku
- funkcja zwraca nazwę pliku na podstawie jego deskryptora */
char* battery_get_filename( FILE *file ) {
    int i;

    for( i = 0; i <= FOPEN_MAX-1; i++ ) {
        if( opened_files[ i ].file == file ) {
            return opened_files[ i ].filename;
        }
    }

    return NULL;
}


size_t battery_fread( FILE *file, char *dst, size_t s_pos, size_t size ) {
    int i, j, k;

    if( file == NULL ) {
        return -1;
    }

    for( i = 0; i < FOPEN_MAX; i++ ) {
        if( file && opened_files[ i ].file == file ) {
            if( s_pos + size > opened_files[ i ].size ) {
                s_pos = opened_files[ i ].size - size;
            }
            for( k = 0, j = s_pos; j < s_pos + size; j++, k++ ) {
                *dst++ = opened_files[ i ].content[j];
            }
            return size;
        }
    }
    return -1;
}

/*
battery_fclose( FILE *file, int socket_fd )
@file - wskaźnik do otwartego pliku
@socket_fd - powiązanie otwieranego pliku (tworzonej struktury) z podłączonym klientem
- funkcja weryfikuje, czy żądany plik może zostać zamknięty na podstawie ilości korzystających z niego klientów */
void battery_fclose( FILE *file, int socket_fd ) {
    int i = 0;
    int clients_count = 0;
    short file_found = 0;
    RESOURCE_TYPE type = NONE;

    if( file == NULL ) {
        return;
    }

    for( i = 0; i <= FOPEN_MAX-1; i++ ) {
        /* Znaleziony element przechowujący informację o otwartym pliku */
        if( file && opened_files[ i ].file == file ) {
            //printf("battery_fclose\n");
            /* Usunięcie elementu przechowującego informacje dla żądanego klienta */
            if( opened_files[ i ].socket_fd == socket_fd ) {
                opened_files[ i ].socket_fd = 0;
                opened_files[ i ].file = NULL;
                opened_files[ i ].size = 0;
                type = opened_files[ i ].type;
                opened_files[ i ].type = NONE;
                if( opened_files[ i ].content ) {
                    printf("[CACHE] File %s removed.\n", opened_files[ i ].filename );
                    free( opened_files[ i ].content );
                    opened_files[ i ].content = NULL;
                }
                memset( opened_files[ i ].filename, '\0', FILENAME_MAX );
                file_found++;

            }
            /* Zliczenie ilości klientów korzystających z pliku */
            clients_count++;
        }
    }

    /* Z pliku korzystał jeden lub mniej klientów */
    if( clients_count == 2 && file_found > 0 ) {
        if( file ) {
            if( type == STD_FILE ) {
                fclose( file );
            } else if( type == SCRIPT ) {
                pclose( file );
            }
        }
    }
}
