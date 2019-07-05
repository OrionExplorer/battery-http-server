/*******************************************************************

Projekt battery-http-server

Plik: ssl.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef SSL_H
#define SSL_H

#include "shared.h"

void	SSL_init( void );
void	SSL_destroy( void );
void	SSL_close( void );
SSL_CTX	*SSL_create_context( void );
void	SSL_configure_context( SSL_CTX *ctx, const char* cert_file, const char* key_file );

#endif
