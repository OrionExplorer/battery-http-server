/*******************************************************************

Projekt battery-http-server

Plik: strings_utils.c

Przeznaczenie:
Zbiór funkcji rozszerzających działania na ciągach znaków

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/string_utils.h"
#include "include/shared.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
strdelbslash( char *s1 )
@s1 - ciąg znaków, z którego będą usunięte podwójne backslashe
- usuwa z ciągu znaków s1 podwójne backslashe*/
void strdelbslash( char *s1 ) {
    char *c1;

#ifdef _WIN32
    while( ( c1 = strstr( s1, "\\\\" ) ) ) {
#else
    while( ( c1 = strstr( s1, "//" ) ) ) {
#endif
        strcpy( c1 + 1, c1 + 2 );
    }
}

/*
strdelstr( const char *s1, const char s2 )
@s1 - ciąg znaków, z którego będą usuwane znaki s2
@s2 - ciąg znaków, który ma zostać usunięty z ciągu s1
- funkcja usuwa ciąg znaków s2 z ciągu s1*/
void strdelstr( char *s1, const char* s2 )
{
    int t_len = strlen( s2 );

    /* Jeżeli ciąg s2 nie występuje w ciągu s1 */
    if( !strstr( s1, s2 ) ) {
        return;
    }

    while( ( s1 = strstr( s1, s2 ) ) ) {
        memmove( s1, s1 + t_len, 1 + strlen( s1 + t_len ) );
    }
}

/*
strrepchar( char *s1, char c1, char c2 )
@s1 - ciąg znaków, w którym ma nastąpić zmiana
@c1 - znak, który ma zostać w ciągu s1 zastąpiony
@c2 - znak, którym ma zostać zastąpiony znak c1 w ciągu s1
- funkcja zamienia pojedyncze znaki bezpośrednio w obiekcie, na który wskazuje s1. */
void strrepchar( char *s1, char c1, char c2 ) {
    int len = strlen( s1 );

    if( len <= 1 ) {
        return;
    }

    while( len > -1 ) {
        if( s1[ len ] == c1 ) {
            s1[ len ] = c2;
        }
        len--;
    }
}

/*
strpos( char *s1, char *s2 )
@s1 - ciąg znaków, na którym będzie przeprowadzona operacja wyszukiwania ciągu s2
@s2 - ciąg znaków, który będzie wyszukany w ciągu s1
- zwraca int, który wskazuje na pozycję ciągu s2 w ciągu s1. Zwraca -1, gdy ciągu nie znaleziono */
int strpos( const char *s1, const char *s2 ) {
    char *p = ( char* )strstr( s1, s2 );
    if( p ) {
        return p - s1;
    }
    return -1;
}

/* Na platformie WIN32+Visual C++ brak funkcji strncasecmp */
#ifdef _MSC_VER
/*
strncasecmp( const char *s1, const char *s2 )
@s1 - ciąg znaków do porównania
@s2 - ciąg znaków do porównania
@n - maksymalna liczba znaków do porównania
- zwraca int, który wskazuje na różnicę pomiędzy ciągiem s1 a s2 */
int strncasecmp( const char *s1, const char *s2, int n ) {
{
    if ( !s1 && !s2 )
        return 0;
    if ( !s1 )
        return 1;
    if ( !s2 )
        return -1;
    if ( n < 0 )
        return 0;
    while ( n && *s1 && *s2 && tolower( ( unsigned char )*s1 ) == tolower( ( unsigned char )*s2 ) ) {
        s1++;
        s2++;
        n--;
    }

    return n == 0 ? 0 : tolower( ( unsigned char )*s1 ) - tolower( ( unsigned char )*s2 );
}
#endif
