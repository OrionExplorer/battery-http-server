/*******************************************************************

Projekt battery-http-server

Plik: files_io.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef FILES_IO_H
#define FILES_IO_H

#include "shared.h"

char*   get_app_path( void );
char*   file_get_ext( const char *filename );
char*   file_get_name( const char *full_filename );
void    file_extract_path( char *full_filename, char delim );
short   directory_exists( const char *path );
short   file_exists( const char *filename );
short   file_params( HTTP_SESSION *http_session, const char *filename, char *ht_access_pwd );

FILE    *battery_fopen( const char *filename, const char *mode, short add_to_list, int socket_fd, RESOURCE_TYPE type );
void    battery_fclose( FILE *file, int socket_fd );
long    battery_ftell( FILE *file );
size_t	battery_fread( FILE *file, int socket_fd, char *dst, size_t s_pos, size_t size );
char*   battery_get_filename( FILE *file );

#endif
