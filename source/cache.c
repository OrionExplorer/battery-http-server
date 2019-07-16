/*******************************************************************

Projekt battery-http-server

Plik: cache.c

Przeznaczenie:
Funkcje ułatwiające zarządzanie pamięcią

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/shared.h"
#include "include/cache.h"
#include "include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE_CACHE           cached_files[ MAX_OPEN_FILES ];

/*
int CACHE_add( FILE *file, const char *filename )
@file - deskryptor otwartego pliku
@filename - nazwa otwartego pliku
@size - rozmiar pliku
- funkcja wczytuje treść podanego pliku do pamięci. */
int CACHE_add( FILE *file, const char *filename, size_t size ) {
    int i = MAX_OPEN_FILES;

    if( !file || !filename || size <= 0 ) {
        LOG_print("[CACHE_add] Error: input parameters are invalid!\n");
        return -1;
    }

    /* Weryfikacja, czy plik nie został już dodany do cache. */
    for( i = MAX_OPEN_FILES-1; i >= 0; i-- ) {
        if( strncmp( cached_files[ i ].filename, filename, FILENAME_MAX ) == 0 ) {
            return 1;
        }
    }

    /* Dodanie pliku do cache. */
    for( i = MAX_OPEN_FILES-1; i >= 0; i-- ) {
        if( cached_files[ i ].file == NULL ) {
            cached_files[ i ].file = file;
            strncpy( cached_files[ i ].filename, filename, FILENAME_MAX );
            cached_files[ i ].content = calloc( size+1, sizeof( char ) );
            if( cached_files[ i ].content == NULL ) {
                LOG_print( "Error: unable to create cache for file %s.\n", filename );
            } else {
                rewind( file );
                if( fread( cached_files[ i ].content, size, 1, file ) == 1 ) {
                    cached_files[ i ].content[ size ] = '\0';
                    LOG_print("[CACHE] File %s loaded to memory.\n", filename );
                } else {
                    LOG_print( "Error: unable load file %s to memory.\n", filename );
                }
            }
            return 1;
        }
    }

    return -1;
}

/*
int CACHE_delete( const char *filename )
@file- deskryptor otwartego pliku
- funkcja usuwa z pamięci treść podanego pliku. */
int CACHE_delete( FILE *file ) {
    int i = MAX_OPEN_FILES;

    if( !file ) {
        return -1;
    }

    for( i = MAX_OPEN_FILES-1; i >= 0; i-- ) {
        if( cached_files[ i ].file == file ) {
            cached_files[ i ].file = NULL;
            if( cached_files[ i ].content ) {
                free( cached_files[ i ].content );
                cached_files[ i ].content = NULL;
            }
            LOG_print( "[CACHE] File %s removed.\n", cached_files[ i ].filename );
            memset( cached_files[ i ].filename, '\0', FILENAME_MAX );
            return 1;
        }
    }

    return -1;
}
