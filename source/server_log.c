/*******************************************************************

Projekt battery_Server

Plik: server_log.c

Przeznaczenie:
Obs�uga logowania zdarze� do pliku logs/log.txt

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/*Przechowuje logi */
static char	log_object[LOG_BUFFER+STD_BUFF_SIZE];

/*Pe�na nazwa pliku ( +�cie�ka dost�pu ) "log.txt" */
char		LOG_filename[MAX_PATH_LENGTH];

extern int errno;

/*
LOG_print( char *fmt, ... )
@fmt - tekst do parsowania
- funkcjonalno�� funkcji printf
- zmiana polega na zapisaniu przekazanego tekstu wraz z warto�ciami do pliku "log.txt" co dany rozmiar bloku */
void LOG_print( char *fmt, ... ) {
	char *output_text;	/* Pe�na tre�� po uwzgl�dnieniu formatowania */
	char *tmp_buf;		/* Je�eli rozmiar log_object jest wi�kszy od LOG_BUFFER, to tutaj b�dzie reszta */
	FILE *f_LOG;
	va_list args;
	int buf_len = 0;
	int len = 0;

	/* Rezerwacja pami�ci */
	output_text = ( char* )malloc( STD_BUFF_SIZE_CHAR );
	mem_allocated( output_text, 150 );

	/* Przetworzenie tekstu na form� wyj�ciow� */
	va_start( args,fmt );
	vsnprintf( output_text, STD_BUFF_SIZE, fmt, args );
	va_end( args );

	/* Dopisanie aktualnej linii do globalnej zawarto�ci loga */
	strncat( log_object, output_text, LOG_BUFFER );

	/* Zwolnienie pami�ci */
	free( output_text );
	output_text = NULL;

	buf_len = strlen( log_object );
	if( buf_len >= LOG_BUFFER ) {
		/*Nie chcemy utraci� niczego z log_object */
		if( buf_len > LOG_BUFFER ) {
			/* Sprawdzenie, o ile wi�kszy jest rozmiar log_object */
			len = buf_len - LOG_BUFFER;
			tmp_buf = ( char* )malloc( len );
			mem_allocated( tmp_buf, 160 );
			strncpy( tmp_buf, log_object+LOG_BUFFER, len );
			strncat( log_object, tmp_buf, len );
			free( tmp_buf );
			tmp_buf = NULL;
		}

		/*Zapis tre�ci ( log_object ) do pliku "log.txt" */
		f_LOG = fopen( LOG_filename, "a+" );
		if( f_LOG ) {
			fseek( f_LOG, 0, SEEK_END );
			fwrite( log_object, LOG_BUFFER + len, 1, f_LOG );
			if( fclose( f_LOG ) == EOF ) {
				LOG_print( "Error: unable to save log object!\n" );
			}
		}
		/* Wyczyszczenie tre�ci loga */
		memset( log_object, 0, LOG_BUFFER );
		if( chdir( app_path ) != 0 ) {
			LOG_print( "Error: unable to perform chdir( %s ).\n", app_path );
			printf( "Error: unable to perform chdir( %s ).\n", app_path );
		}
	}
}

/*
LOG_save( void )
- zapisuje bie��c� zawarto�� log_object do pliku nie zwaracaj�c uwagi na poziom zape�nienia bufora */
void LOG_save( void ) {
	FILE *f_LOG;

	/*Zapis tre�ci ( log_object ) do pliku "log.txt" */
	f_LOG = fopen( LOG_filename, "a+" );
	if( f_LOG ) {
		fseek( f_LOG, 0, SEEK_END );
		fwrite( log_object, strlen( log_object ), 1, f_LOG );
		if( fclose( f_LOG ) == EOF ) {
			printf( "Error: unable to save log object!\n" );
		}
	} else {
		LOG_print( "Error: unable to open log file.\n" );
		printf( "Error: unable to open log file (%d).\n", errno );
		printf("%d\n", errno);
		exit( EXIT_FAILURE );
	}

	/* Wyczyszczenie tre�ci loga */
	memset( log_object, 0, LOG_BUFFER );
	if( chdir( app_path ) != 0 ) {
		LOG_print( "Error: unable to perform chdir( %s ).\n", app_path );
		printf( "Error: unable to perform chdir( %s ).\n", app_path );
	}
}
