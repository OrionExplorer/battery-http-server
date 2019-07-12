/*******************************************************************

Projekt battery-http-server

Plik: cache.c

Przeznaczenie:
Funkcje ułatwiające zarządzanie pamięcią

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/shared.h"
#include "include/cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE_CACHE           cached_files[ FOPEN_MAX ];

/*
int CACHE_add( FILE *file, const char *filename )
@file - deskryptor otwartego pliku
@filename - nazwa otwartego pliku
@size - rozmiar pliku
- funkcja wczytuje treść podanego pliku do pamięci. */
int CACHE_add( FILE *file, const char *filename, size_t size ) {
    int i = FOPEN_MAX;

    if( !file || !filename || size <= 0 ) {
        printf("[CACHE_add] Error: input parameters are invalid!\n");
        return -1;
    }

    printf("[CACHE_add]: filename = %s\n", filename);
    /* Weryfikacja, czy plik nie został już dodany do cache. */
    for( i = FOPEN_MAX-1; i >= 0; i-- ) {
        if( strncmp( cached_files[ i ].filename, filename, FILENAME_MAX ) == 0 ) {
            printf("[CACHE] File %s already cached.\n", filename );
            return 1;
        }
    }

    /* Dodanie pliku do cache. */
    for( i = FOPEN_MAX-1; i >= 0; i-- ) {
        if( cached_files[ i ].file == NULL ) {
            cached_files[ i ].file = file;
            strncpy( cached_files[ i ].filename, filename, FILENAME_MAX );
            cached_files[ i ].content = calloc( size+1, sizeof( char ) );
            if( cached_files[ i ].content == NULL ) {
                printf( "Error: unable to create cache for file %s.\n", filename );
            } else {
                rewind( file );
                if( fread( cached_files[ i ].content, size, 1, file ) == 1 ) {
                    cached_files[ i ].content[ size ] = '\0';
                    printf("[CACHE] File %s loaded to memory.\n", filename );
                } else {
                    printf( "Error: unable load file %s to memory.\n", filename );
                }
            }
            return 1;
        }
    }

    return -1;
}

/*
int CACHE_delete( const char *filename )
@filename - nazwa otwartego pliku
- funkcja usuwa z pamięci treść podanego pliku. */
int CACHE_delete( const char *filename ) {
    int i = FOPEN_MAX;

    if( !filename ) {
        return -1;
    }

    for( i = FOPEN_MAX; i > 0; i-- ) {
        if( strncmp( cached_files[ i ].filename, filename, FILENAME_MAX ) == 0 ) {
            cached_files[ i ].file = NULL;
            if( cached_files[ i ].content ) {
                free( cached_files[ i ].content );
                cached_files[ i ].content = NULL;
            }
            printf( "[CACHE] File %s removed.\n", filename );
            return 1;
        }
    }

    return -1;
}
