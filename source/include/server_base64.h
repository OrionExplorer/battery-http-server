/*******************************************************************

Projekt battery-http-server

Plik: server_base64.h

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#ifndef SERVER_BASE64_H
#define SERVER_BASE64_H

size_t	base64_decode( char *source, unsigned char *target, size_t targetlen );
int		base64_encode( unsigned char *source, size_t sourcelen, char *target, size_t targetlen );

#endif
