/*******************************************************************

Projekt battery-http-server

Plik: socket_io.c

Przeznaczenie:
Inicjalizacja socketów
Konfiguracja socketów
Odbieranie danych z sieci i przekazanie do interpretacji

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#include "include/socket_io.h"
#include "include/session.h"
#include "include/http_protocol.h"
#include "include/shared.h"
#include "include/log.h"
#include "include/files_io.h"
#include <sys/sendfile.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

/*Sockety */
#ifdef _WIN32
/*Inicjalizacja WinSock */
WSADATA             wsk;
SOCKET              socket_server;
#else
int                 socket_server;
#endif

int                 addr_size;
int                 active_port;
struct sockaddr_in  server_address;
HTTP_SESSION        http_session_;
int                 i_sac;
fd_set              master;
fd_set              read_fds;
int                 fdmax;
int                 newfd;
struct hostent      *host;
struct in_addr      addr;

int                 http_conn_count = 0;
SEND_INFO           send_d[ MAX_CLIENTS ];

static void     SOCKET_initialization( void );
static void     SOCKET_prepare( void );
static void     SOCKET_process( int socket_fd );
static void     SOCKET_send_all_data( void );
void            SOCKET_stop( void );

/*
SOCKET_initialization( void )
- wersja dla systemu Windows
- inicjalizacja WinSock
- inicjalizacja socketa */
static void SOCKET_initialization( void ) {
    LOG_print( "Waiting for socket server initialization..." );

#ifdef _WIN32
    /*Inicjalizacja WinSock */
    if ( WSAStartup( MAKEWORD( 2, 2 ), &wsk ) != 0 ) {
        LOG_print( "\nError creating Winsock.\n" );
        printf( "Error creating Winsock.\n" );
        system( "pause" );
        exit( EXIT_FAILURE );
    }
#endif
    /*Utworzenie socketa nasłuchującego */
    socket_server = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( socket_server == SOCKET_ERROR ) {
        LOG_print( "Error creating socket.\n" );
        printf( "Error creating socket.\n" );
        SOCKET_stop();
        exit( EXIT_FAILURE );
    }

    if( active_port < 0 || active_port > 65535 ) {
        active_port = DEFAULT_PORT;
    }

    memset( &server_address, 0, sizeof( server_address ) );
    server_address.sin_addr.s_addr = htonl( INADDR_ANY );
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons( ( u_short )active_port );
}

/*
SOCKET_send_all_data( void )
- funkcja weryfikuje, czy są do wysłania dane z któregokolwiek elementu tablicy SEND_INFO. Jeżeli tak, to następuje wysyłka kolejnego fragmentu pliku. */
static void SOCKET_send_all_data( void ) {
    int j;
    static char m_buf[ UPLOAD_BUFFER ];
    size_t nwrite;
    size_t nread;
    size_t tmp_now1, tmp_now2;
    off_t offset = 0;

    for( j = 0; j < MAX_CLIENTS; j++ ) {
        if( send_d[ j ].http_content_size > 0 && send_d[ j ].socket_descriptor > 0 ) {
            //fseek( send_d[ j ].file, send_d[ j ].sent_size, SEEK_SET );
            //nread = fread( m_buf, sizeof( char ), UPLOAD_BUFFER, send_d[ j ].file );
            //nwrite = send( send_d[ j ].socket_descriptor, m_buf, nread, 0 );
            nwrite = sendfile( send_d[ j ].socket_descriptor, send_d[ j ].file->_fileno, &send_d[ j ].sent_size, BUFSIZ );

            send_d[ j ].http_content_size -= nwrite;
            
            if( (nwrite < 0 && GetLastError() != EWOULDBLOCK ) || (send_d[ j ].http_content_size <= 0 && send_d[ j ].keep_alive == 0)) {
                SESSION_delete_send_struct( send_d[ j ].socket_descriptor );
            }
        }
    }
}

/*
SOCKET_prepare( void )
- nasłuchiwanie w celu odbioru danych od klienta */
static void SOCKET_prepare( void ) {
    unsigned long b = 0;
    int i = 1;
    int wsa_result = 0;
    struct timeval tv = {0, 0};

    tv.tv_sec = 0;
    tv.tv_usec = 20000;

    FD_ZERO( &master );
    FD_ZERO( &read_fds );

#ifndef _WIN32
    setuid( 0 );
    setgid( 0 );
#endif

    if( setsockopt( socket_server, SOL_SOCKET, SO_REUSEADDR, ( char * )&i, sizeof( i ) ) == SOCKET_ERROR ) {
        wsa_result = WSAGetLastError();
        LOG_print( "setsockopt( SO_REUSEADDR ) error: %d.\n", wsa_result );
        printf( "setsockopt( SO_REUSEADDR ) error: %d.\n", wsa_result );
    }

    if( setsockopt( socket_server, SOL_SOCKET, SO_RCVTIMEO, ( char* )&tv, sizeof( struct timeval ) ) == SOCKET_ERROR ) {
        wsa_result = WSAGetLastError();
        LOG_print( "setsockopt( SO_RCVTIMEO ) error: %d.\n", wsa_result );
        printf( "setsockopt( SO_RCVTIMEO ) error: %d.\n", wsa_result );
    }

    if( setsockopt( socket_server, SOL_SOCKET, SO_SNDTIMEO, ( char* )&tv, sizeof( struct timeval ) ) == SOCKET_ERROR ) {
        wsa_result = WSAGetLastError();
        LOG_print( "setsockopt( SO_SNDTIMEO ) error: %d.\n", wsa_result );
        printf( "setsockopt( SO_SNDTIMEO ) error: %d.\n", wsa_result );
    }

    if( setsockopt( socket_server, IPPROTO_TCP, TCP_NODELAY, ( char * )&i, sizeof( i ) ) == SOCKET_ERROR ) {
        wsa_result = WSAGetLastError();
        LOG_print( "setsockopt( TCP_NODELAY ) error: %d.\n", wsa_result );
        printf( "setsockopt( TCP_NODELAY ) error: %d.\n", wsa_result );
    }

    if( setsockopt( socket_server, IPPROTO_TCP, TCP_CORK, ( char * )&i, sizeof( i ) ) == SOCKET_ERROR ) {
        wsa_result = WSAGetLastError();
        LOG_print( "setsockopt( TCP_CORK ) error: %d.\n", wsa_result );
        printf( "setsockopt( TCP_CORK ) error: %d.\n", wsa_result );
    }

    /* Ustawienie na non-blocking socket */
    if( fcntl( socket_server, F_SETFL, &b ) == SOCKET_ERROR ) {
        wsa_result = WSAGetLastError();
        LOG_print( "ioctlsocket() error: %d.\n", wsa_result );
        printf( "ioctlsocket() error: %d.\n", wsa_result );
        SOCKET_stop();
        exit( EXIT_FAILURE );
    }

    if ( bind( socket_server, ( struct sockaddr* )&server_address, sizeof( server_address ) ) == SOCKET_ERROR ) {
        wsa_result = WSAGetLastError();
        LOG_print( "bind() error: %d.\n", wsa_result );
        printf( "bind() error: %d.\n", wsa_result );
        SOCKET_stop();
        exit( EXIT_FAILURE );
    }

    /* Rozpoczęcie nasłuchiwania */
    if( listen( socket_server, MAX_CLIENTS ) == SOCKET_ERROR ) {
        wsa_result = WSAGetLastError();
        LOG_print( "listen() error: %d.\n", wsa_result );
        printf( "listen() error: %d.\n", wsa_result );
        SOCKET_stop();
        exit( EXIT_FAILURE );
    }

    LOG_print( "ok.\nSocket server is running:\n" );
    LOG_print( "- Port: %d.\n", active_port );
    LOG_print( "Communication Interface ready...\n" );
    /* Teraz czekamy na połączenia i dane */
}

/*
SOCKET_run( void )
- funkcja zarządza połączeniami przychodzącymi do gniazda. */
void SOCKET_run( void ) {
    register int i = 0;
    struct timeval tv;

    /* Reset zmiennej informującej o częściowym odbiorze przychodzącej treści */
    http_session_.http_info.received_all = -1;

    FD_SET( socket_server, &master );

    tv.tv_sec = 5;
    tv.tv_usec = 500000;

    fdmax = socket_server;

    for( ;"elvis presley lives"; ) {
        read_fds = master;
        if( select( fdmax+1, &read_fds, NULL, NULL, &tv ) == -1 ) {
            SOCKET_stop();
            exit( EXIT_FAILURE );
        }

        i = fdmax+1;
        while( --i ) {
            if( FD_ISSET( i, &read_fds ) ) { /* Coś się dzieje na sockecie... */
                if( i == socket_server ) {
                    /* Podłączył się nowy klient */
                    SOCKET_modify_clients_count( 1 ); /* Kolejny klient - zliczanie do obsługi błędu 503 */
                    http_session_.address_length = sizeof( struct sockaddr );
                    newfd = accept( socket_server, ( struct sockaddr* )&http_session_.address, &http_session_.address_length );

                    if( newfd == -1 ) {
                        LOG_print( "Socket error: accept().\n" );
                    } else {
                        SESSION_add_new_send_struct( newfd );
                        FD_SET( newfd, &master );
                        if( newfd > fdmax ) {
                            fdmax = newfd;
                        }
                    }
                } else {
                    /* Podłączony klient przesłał dane... */
                    SOCKET_process( i );
                }
            } /* nowe połączenie */
            SOCKET_send_all_data();
        } /* pętla deskryptorów while( --i )*/
        SOCKET_send_all_data();
        Sleep(1);
    } /* for( ;; ) */
}

/*
SOCKET_process( int socket_fd )
@socket_fd - identyfikator gniazda
- funkcja odczytuje dane z gniazda */
static void SOCKET_process( int socket_fd ) {
    HTTP_SESSION *session = ( HTTP_SESSION* )malloc( sizeof( HTTP_SESSION ) );
    char tmp_buf[ MAX_BUFFER ];
    extern int errno;

    SESSION_init( session );

    errno = 0;
    session->http_info.received_all = http_session_.http_info.received_all;
    session->address = http_session_.address;
    session->socket_descriptor = socket_fd;
    session->address_length = recv( ( int )socket_fd, tmp_buf, MAX_BUFFER, 0 );

    if( session->address_length < MAX_URI_LENGTH ) {
        if( errno > 1) {
            SESSION_delete_send_struct( socket_fd );
            SOCKET_close( socket_fd );
        } else {
            if ( session->address_length <= 0 ) {
                /* ...ale to jednak było rozłączenie */
                SESSION_delete_send_struct( socket_fd );
                SOCKET_close( socket_fd );
            } else if (session->address_length > 0 ) {
                /* Nie zostały wcześniej odebrane wszystkie dane - metoda POST.
                Teraz trzeba je dokleić do http_info.content_data */
                if( session->http_info.received_all == 0 ) {
                    /* Obiekt jest już stworzony, nie trzeba przydzielać pamięci */
                    session->http_info.content_data = ( char* )realloc( session->http_info.content_data, strlen( session->http_info.content_data )+session->address_length+1 );
                    /* TODO: BINARY DATA!!! */
                    strncat( session->http_info.content_data, tmp_buf, session->address_length );
                    session->http_info.received_all = 1;
                } else if( session->http_info.received_all == -1 ) {
                    /* Dla metod GET i HEAD */
                    session->http_info.content_data = malloc( (session->address_length+1)*sizeof( char ) );
                    mem_allocated( session->http_info.content_data, 25 );
                    strncpy( session->http_info.content_data, tmp_buf, session->address_length );
                }
                /* "Przerobienie" zapytania */
                SESSION_prepare( session );
            }
        }
    } else {
        RESPONSE_error( session, HTTP_414_REQUEST_URI_TOO_LONG, HTTP_ERR_414_MSG, NULL );
        SOCKET_disconnect_client( session );
        SESSION_release( session );
    }

    if( session ) {
        free( session );
        session = NULL;
    }
}

void SOCKET_modify_clients_count( int mod ) {
    if( mod > 0 ) {
        http_conn_count++;
    } else {
        if( (http_conn_count - mod) >= 0 ) {
            http_conn_count--;
        }
    }
}

void SOCKET_close( int socket_descriptor ) {
    FD_CLR( socket_descriptor, &master );
    shutdown( socket_descriptor, SHUT_RDWR );
    close( socket_descriptor );
    /* Zmniejszensie licznika podłączonych klientów */
    SOCKET_modify_clients_count( -1 );
}

/*
SOCKET_stop( void )
- zwolnienie WinSock
- zwolnienie socketa */
void SOCKET_stop( void ) {
    LOG_print( "SOCKET_stop( %d ):\n", fdmax );
    LOG_print( "\t- shutdown( %d )...", socket_server );
    shutdown( socket_server, SHUT_RDWR );
    LOG_print( "ok.\n" );

    LOG_print( "\t- close( %d )...", http_session_.socket );
    close( http_session_.socket );
    LOG_print( "ok.\n" );

    LOG_print( "\t- close( %d )...", socket_server );
    close( socket_server );
    LOG_print( "ok.\n" );

#ifdef _WIN32
    LOG_print( "\t- WSACleanup()..." );
    WSACleanup();
    LOG_print( "ok.\n" );
#endif
}

/*
SOCKET_release( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- funkcja resetuje zmienne informujące o podłączonym sockecie. */
void SOCKET_release( HTTP_SESSION *http_session ) {
    http_session->socket_descriptor = -1;
    http_session->address_length = -1;
    http_session->http_info.keep_alive = -1;
}

/*
SOCKET_disconnect_client( HTTP_SESSION *http_session )
- rozłącza klienta podanego jako struktura http_session */
void SOCKET_disconnect_client( HTTP_SESSION *http_session ) {
    if( http_session->socket_descriptor != SOCKET_ERROR ) {
        SOCKET_close( http_session->socket_descriptor );
    } else {
        SOCKET_release( http_session );
    }
}

/*
SOCKET_send( HTTP_SESSION *http_session, char *buf, int http_content_size )
- wysyła pakiet danych ( buf ) do danego klienta ( http_session ) */
void SOCKET_send( HTTP_SESSION *http_session, const char *buf, int http_content_size, int *res ) {
    if( ( http_session->address_length = send( http_session->socket_descriptor, buf, http_content_size, 0 ) ) <= 0 ) {
        SOCKET_disconnect_client( http_session );
    }
    *res = http_session->address_length;
}

/*
server_get_remote_hostname( const char *remote_addr )
@http_session - wskaźnik do podłączonego klienta
- zwraca ciąg znaków będący nazwą hosta. */
char* server_get_remote_hostname( HTTP_SESSION *http_session ) {
    static char remote_name[ TINY_BUFF_SIZE ];
    memset( remote_name, '\0', TINY_BUFF_SIZE );
    getnameinfo( ( struct sockaddr * )&http_session->address, sizeof( http_session->address ), remote_name, sizeof( remote_name ), NULL, 0, NI_NAMEREQD );
    return ( ( char* )&remote_name );
}

/*
SOCKET_get_remote_ip( HTTP_SESSION *http_session )
@http_session - wskaźnik do podłączonego klienta
- zwraca ciąg znaków będący adresem IP. */
char* SOCKET_get_remote_ip( HTTP_SESSION *http_session ) {
    static char ip_addr[ TINY_BUFF_SIZE ];
    memset( ip_addr, '\0', TINY_BUFF_SIZE );
    getnameinfo( ( struct sockaddr * )&http_session->address, sizeof( http_session->address ), ip_addr, sizeof( ip_addr ), NULL, 0, NI_NUMERICHOST );
    return ( ( char* )&ip_addr );
}

/*
SOCKET_main( void )
- obsługa funkcji socketów */
void SOCKET_main( void ) {
    ( void )SOCKET_initialization();
    ( void )SOCKET_prepare();
    ( void )SOCKET_run();
    ( void )SOCKET_stop();
}
