/*******************************************************************

Projekt battery-http-server

Plik: log.c

Przeznaczenie:
Obsługa logowania zdarzeń do pliku logs/log.txt

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/*Przechowuje logi */
static char log_object[ LOG_BUFFER+STD_BUFF_SIZE ];

/*Pełna nazwa pliku ( +ścieżka dostępu ) "log.txt" */
char        LOG_filename[ MAX_PATH_LENGTH ];

/*
LOG_print( char *fmt, ... )
@fmt - tekst do parsowania
- funkcjonalność funkcji printf
- zmiana polega na zapisaniu przekazanego tekstu wraz z wartościami do pliku "log.txt" co dany rozmiar bloku */
void LOG_print( char *fmt, ... ) {
    char *output_text;  /* Pełna treść po uwzględnieniu formatowania */
    char *tmp_buf;      /* Jeżeli rozmiar log_object jest większy od LOG_BUFFER, to tutaj będzie reszta */
    FILE *f_LOG;
    va_list args;
    int buf_len = 0;
    int len = 0;

    /* Rezerwacja pamięci */
    output_text = malloc( STD_BUFF_SIZE_CHAR );
    mem_allocated( output_text, 38 );

    /* Przetworzenie tekstu na formę wyjściową */
    va_start( args,fmt );
    vsnprintf( output_text, STD_BUFF_SIZE, fmt, args );
    va_end( args );

    /* Dopisanie aktualnej linii do globalnej zawartości loga */
    strncat( log_object, output_text, LOG_BUFFER );

    /* Zwolnienie pamięci */
    free( output_text );
    output_text = NULL;

    buf_len = strlen( log_object );
    if( buf_len >= LOG_BUFFER ) {
        /*Nie chcemy utracić niczego z log_object */
        if( buf_len > LOG_BUFFER ) {
            /* Sprawdzenie, o ile większy jest rozmiar log_object */
            len = buf_len - LOG_BUFFER;
            tmp_buf = malloc( len );
            mem_allocated( tmp_buf, 39 );
            strncpy( tmp_buf, log_object+LOG_BUFFER, len );
            strncat( log_object, tmp_buf, len );
            free( tmp_buf );
            tmp_buf = NULL;
        }

        /*Zapis treści ( log_object ) do pliku "log.txt" */
        f_LOG = fopen( LOG_filename, "a+" );
        if( f_LOG ) {
            fseek( f_LOG, 0, SEEK_END );
            fwrite( log_object, LOG_BUFFER + len, 1, f_LOG );
            if( fclose( f_LOG ) == EOF ) {
                LOG_print( "Error: unable to save log object!\n" );
            }
        }
        /* Wyczyszczenie treści loga */
        memset( log_object, 0, LOG_BUFFER );
        if( chdir( app_path ) != 0 ) {
            LOG_print( "Error: unable to perform chdir( %s ).\n", app_path );
            printf( "Error: unable to perform chdir( %s ).\n", app_path );
        }
    }
}

/*
LOG_save( void )
- zapisuje bieżącą zawartość log_object do pliku nie zwaracając uwagi na poziom zapełnienia bufora */
void LOG_save( void ) {
    FILE *f_LOG;

    /*Zapis treści ( log_object ) do pliku "log.txt" */
    f_LOG = fopen( LOG_filename, "a+" );
    if( f_LOG ) {
        fseek( f_LOG, 0, SEEK_END );
        fwrite( log_object, strlen( log_object ), 1, f_LOG );
        if( fclose( f_LOG ) == EOF ) {
            printf( "Error: unable to save log object!\n" );
        }
    } else {
        LOG_print( "Error: unable to open log file.\n" );
        exit( EXIT_FAILURE );
    }

    /* Wyczyszczenie treści loga */
    memset( log_object, 0, LOG_BUFFER );
    if( chdir( app_path ) != 0 ) {
        LOG_print( "Error: unable to perform chdir( %s ).\n", app_path );
        printf( "Error: unable to perform chdir( %s ).\n", app_path );
    }
}
