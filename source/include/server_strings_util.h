/*******************************************************************

Projekt battery-http-server

Plik: sever_strings_util.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef SERVER_STRINGS_UTIL
#define SERVER_STRINGS_UTIL

void            strdelbslash( char *s1 );
int             strpos( const char *s1, const char *s2 );
void            strrepchar( char *s1, char c1, char c2 );
void            strdelstr( char *s1, const char* s2 );

#ifdef _MSC_VER
short           strncasecmp( const char *s1, const char *s2, int n );
#endif

#endif
