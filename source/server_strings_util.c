/*******************************************************************

Projekt battery_Server

Plik: server_strings_util.c

Przeznaczenie:
Zbiór funkcji rozszerzaj¹cych dzia³ania na ci¹gach znaków

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_strings_util.h"
#include "include/server_shared.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*
strdelbslash( char *s1 )
@s1 - ci¹g znaków, z którego bêd¹ usuniête podwójne backslashe
- usuwa z ci¹gu znaków s1 podwójne backslashe*/
void strdelbslash( char *s1 ) {
	char *c1;

#ifdef _WIN32
	while( ( c1 = strstr(  s1, "\\\\"  ) ) ) {
#else
	while( ( c1 = strstr(  s1, "//"  ) ) ) {
#endif
		strcpy( c1 + 1, c1 + 2 );
	}
}

/*
strdelstr( const char *s1, const char s2 )
@s1 - ci¹g znaków, z którego bêd¹ usuwane znaki s2
@s2 - ci¹g znaków, który ma zostaæ usuniêty z ci¹gu s1
- funkcja usuwa ci¹g znaków s2 z ci¹gu s1*/
void strdelstr( char *s1, const char* s2 )
{
	int t_len = strlen( s2 );

	/* Je¿eli ci¹g s2 nie wystêpuje w ci¹gu s1 */
	if( !strstr( s1, s2 ) ) {
		return;
	}

	while( ( s1 = strstr( s1, s2 ) ) ) {
		memmove( s1, s1 + t_len, 1 + strlen( s1 + t_len ) );
	}
}

/*
strrepchar( char *s1, char c1, char c2 )
@s1 - ci¹g znaków, w którym ma nast¹piæ zmiana
@c1 - znak, który ma zostaæ w ci¹gu s1 zast¹piony
@c2 - znak, którym ma zostaæ zast¹piony znak c1 w ci¹gu s1
- funkcja zamienia pojedyncze znaki bezpoœrednio w obiekcie, na który wskazuje s1. */
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
strpos( char *where_, char *what )
@where_ - ci¹g znaków, na którym bêdzie przeprowadzona operacja wyszukiwania ci¹gu what
@what - ci¹g znaków, który bêdzie wyszukany w ci¹gu where_
- zwraca int, który wskazuje na pozycjê ci¹gu what w ci¹gu where_. Zwraca -1, gdy ci¹gu nie znaleziono */
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
@s1 - ci¹g znaków do porównania
@s2 - ci¹g znaków do porównania
@n - maksymalna iloœæ znaków do porównania
- zwraca int, który wskazuje na ró¿nicê pomiêdzy ci¹giem s1 a s2 */
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

/*#include "external\md5\md5.h"
text_md5( const char *sz_Text )
@sz_Text - treœæ, z której ma zostaæ wyliczony MD5
- zwraca char *z wyliczonego MD5 zmiennej sz_Text
char *text_md5( const char *sz_Text )
{
md5_state_t state;
md5_byte_t digest[ 16[ ;
char hex_output[ 16*2 + 1[ ;
static char sz_md5Result[ 128[ ;
int di;

md5_init( &state );
md5_append( &state, ( const md5_byte_t * )sz_Text, strlen( sz_Text ) );
md5_finish( &state, digest );

for ( di = 0; di < 16; ++di )
sprintf( hex_output + di  *2, "%02x", digest[ di[  );

strncpy( sz_md5Result, hex_output, STD_BUFF_SIZE );

return ( ( char* )&sz_md5Result );
}*/
