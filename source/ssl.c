/*******************************************************************

Projekt battery-http-server

Plik: ssl.c

Przeznaczenie:
Umożliwienie pracy serwera w trybie bezpiecznym

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/shared.h"
#include "include/log.h"

/*
SSL_CTX *SSL_create_context( void )
- tworzy główny kontekst dla SSL
*/
SSL_CTX *SSL_create_context( void ) {
    const SSL_METHOD    *method;
    SSL_CTX             *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if( !ctx ) {
        LOG_print( "Error: unable to create SSL context.\n" );
        ERR_print_errors_fp( stderr );
        exit( EXIT_FAILURE );
    }

    return ctx;
}

void  SSL_configure_context( SSL_CTX *ctx, const char* cert_file, const char* key_file ) {
    SSL_CTX_set_ecdh_auto( ctx, 1 );

    if( SSL_CTX_use_certificate_file( ctx, cert_file, SSL_FILETYPE_PEM ) <= 0 ) {
        LOG_print( "Error: unable to load certificate file: %s.\n", cert_file );
        ERR_print_errors_fp( stderr );
        exit( EXIT_FAILURE );
    }

    if( SSL_CTX_use_PrivateKey_file( ctx, key_file, SSL_FILETYPE_PEM ) <= 0 ) {
        LOG_print( "Error: unable to load private key file: %s.\n", cert_file );
        ERR_print_errors_fp( stderr );
        exit( EXIT_FAILURE );   
    }
}

/*
SSL_init
- inicjalizacja biblioteki OpenSSL */
void SSL_init( void ) {
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

/*
SSL_destroy
- zwolnienie obiektu OpenSSL */
void SSL_destroy( void ) {
    ERR_free_strings();
    EVP_cleanup();
}