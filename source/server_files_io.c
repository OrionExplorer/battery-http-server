/*******************************************************************

Projekt battery_Server

Plik: server_files_io.c
Przeznaczenie:
Zbi�r funkcji przeznaczonych do obs�ugi plik�w i katalog�w

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_files_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

OPENED_FILE opened_files[ FOPEN_MAX ];

/*
get_app_path( void )
- zwraca ci�g znak�w - folder startowy aplikacji */
char* get_app_path( void ) {
	static char buf[MAX_PATH_LENGTH];
	if( getcwd( buf, MAX_PATH_LENGTH ) ) {
		return strncat( buf, SLASH, MAX_PATH_LENGTH );
	} else {
		return "";
	}
}

/*
directory_exists( const char *path )
@path - �cie�ka, kt�ra ma zosta� sprawdzona
- funkcja pr�buje ustawi� katalog roboczy na �cie�k� podan� w zmiennej path
- zwraca int, gdzie 0 = �cie�ka nie istnieje, 1 = �cie�ka istnieje */
short directory_exists( const char *path ) {
	if( chdir( path ) == 0 ) {
		return 1;
	} else {
		return 0;
	}
}

/*
file_get_name( const char *full_filename )
@full_filename - pe�na nazwa pliku ( +�cie�ka )
- pobiera nazw� pliku z pe�nej nazwy ( +�cie�ka )
- zwraca char *z nazw� pliku */
char* file_get_name( const char *full_filename ) {
	char *filename;

	if( strstr( full_filename, SLASH ) ) {
		filename = ( char* )strrchr( full_filename, C_SLASH );
	} else {
		filename = ( char* )strrchr( full_filename, '/' );
	}

	/*Usuni�cie znaku "\" z pocz�tku filename */
	if( filename ) {
		return ++filename;
	}

	return "";
}

/*
file_get_ext( const char *filename )
@filename - nazwa pliku lub pe�na nazwa pliku ( +�cie�ka )
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
@filename - nazwa pliku ( +�cie�ka )
- sprawdza, czy podany w zmiennej filename plik istnieje
- pr�buje otworzy� plik
- zwraca int, gdzie:
+ 0 = nie istnieje
+ 1 = istnieje, jest do odczytu, nie wymaga autentykacji
+ 2 = istnieje, brak uprawnie� do odczytu
+ 3 = istnieje, wymagana autentykacja */
short file_params( HTTP_SESSION *http_session, const char *filename, char *ht_access_pwd ) {
	FILE *resource;
	struct stat file_stat;
	int i = 0;

	/* Weryfikacja, czy podany parametr jest prawid�ow� nazw� pliku */
	stat( filename, &file_stat );
	if( file_stat.st_mode & S_IFREG );
	else /* Nie jest... */
		return 0;

	/*Sprawdza, czy uda�o si� otworzy� plik */
	resource = fopen( filename, READ_BINARY );

	if( !resource ) {
		return 0;
	} else {
		//printf("!!Otwarty plik\n");
		if( http_session == NULL ) {
			//printf("!!Zamkniety plik 1\n");
			fclose( resource );
			return 1;
		}

		/* Pobranie informacji o ostatniej dacie modyfikacji zasobu */
		if( !http_session->local_info.date_res_last_modified ) {
			http_session->local_info.date_res_last_modified = ( char* )malloc( TIME_BUFF_SIZE_CHAR );
			mem_allocated( http_session->local_info.date_res_last_modified, 1111 );
		}

		strftime( http_session->local_info.date_res_last_modified, TIME_BUFF_SIZE, RFC1123FMT, gmtime( &file_stat.st_mtime ) );

		/* Sprawdza, czy plik ma uprawnienia do odczytu */
		if( file_stat.st_mode & S_IREAD ) {
			if( ht_access_pwd ) {
				for( i = 0; i < ht_access_count; i++ ) {
					if( strncmp( ht_access[i].res, filename, MAX_PATH_LENGTH ) == 0 ) {
						/* Zas�b wymaga autoryzacji */
						strncpy( ht_access_pwd, ht_access[i].res_auth, STD_BUFF_SIZE );
						//printf("!!Zamkniety plik 2\n");
						fclose( resource );
						return 3;
					}
				}
			}
		} else {
			/* Nie ma */
			//printf("!!Zamkniety plik 3\n");
			fclose( resource );
			return 2;
		}
	}

	/* Zamkni�cie pliku */
	if( resource ) {
		//printf("!!Zamkniety plik 4\n");
		fclose( resource );
	}

	return 1;
}

/*
file_exists( const char *filename )
@filename - nazwa szukanego pliku
- zwraca int, gdzie 1 = znaleziono plik. */
short file_exists( const char *filename ) {
	FILE *resource;	/* Uchwyt do pliku */

	if( ( resource = battery_fopen( filename, "r", 0 ) ) ) {
		//printf("!!Otwarty plik\n");
		/* Uda�o si� otworzy� plik = istnieje */
		fclose( resource );
		//printf("!!Zamkniety plik 5\n");
		return 1;
	} else {
		/* Nie istnieje */
		return 0;
	}
}

/*
file_extract_path( const char *full_filename, char delim )
@full_filename - �cie�ka dost�pu do pliku, z kt�rej b�dzie pobrana sama �cie�ka
@delim - znak, od kt�rego ma zosta� "obci�ta" �cie�ka
- zwraca ci�g znak�w, kt�ry jest wyci�t� �cie�k� z pe�nej �cie�ki.
Rezultat nale�y p�niej zwolni� poprzez funkcj� free(). */
void file_extract_path(char *full_filename, char delim)
{
	int i = strlen(full_filename);

	if(i > 0) {
		while(--i) {
			if(full_filename[i] == delim) {
				full_filename[i+1] = '\0';
			}
		}
	}
}

FILE *battery_fopen( const char *filename, const char *mode, short add_to_list ) {
	int i = FOPEN_MAX;
	FILE *tmp;

	while( --i && i >= 0 ) {
		if( strcmp( opened_files[ i ].filename, filename ) == 0 ) {
			return opened_files[ i ].file;
		}
	}

	tmp = fopen( filename, mode );

	if( tmp ) {
		if( add_to_list == 0) {
			return tmp;
		} else {
			fseek( tmp, 0, SEEK_END );
			i = FOPEN_MAX;

			while( --i && i >= 0 ) {
				if( opened_files[ i ].file == NULL ) {
					opened_files[ i ].file = tmp;
					strcpy( opened_files[ i ].filename, filename );
					opened_files[ i ].size = ftell( tmp );
					break;
				}
			}

			return tmp;
		}
	}

	return NULL;
}

long battery_ftell( FILE *file ) {
	int i = FOPEN_MAX;

	while( --i && i >= 0 ) {
		if( opened_files[ i ].file == file ) {
			opened_files[ i ].size = ftell( file );
			return opened_files[ i ].size;
		}
	}

	return 0;
}

char*	battery_get_filename( FILE *file ) {
	int i = FOPEN_MAX;

	while( --i && i >= 0 ) {
		if( opened_files[ i ].file == file ) {
			return opened_files[ i ].filename;
		}
	}

	return NULL;
}

void battery_fclose( FILE *file ) {
	int i = FOPEN_MAX;

	while( --i && i >= 0 ) {
		if( opened_files[ i ].file == file ) {
			fclose( file );
			opened_files[ i ].file = NULL;
			memset( opened_files[ i ].filename, '\0', FILENAME_MAX );
		}
	}
}
