/*******************************************************************

Projekt battery-http-server

Plik: cache.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef CACHE_H
#define CACHE_H

int     CACHE_add( FILE *file, const char* filename, size_t size );
int     CACHE_delete( const char* filename );

#endif
