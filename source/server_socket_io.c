/*******************************************************************

Projekt battery_Server

Plik: server_socket_io.c

Przeznaczenie:
Inicjalizacja socketï¿½w
Konfiguracja socketï¿½w
Odbieranie danych z sieci i przekazanie do interpretacji

Autor: Marcin Kelar ( marcin.kelar@holicon.pl )
*******************************************************************/
#include "include/server_socket_io.h"
#include "include/server_create_session.h"
#include "include/server_http_protocol.h"
#include "include/server_shared.h"
#include "include/server_log.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

/*Sockety */
#ifdef _WIN32
/*Inicjalizacja WinSock */
WSADATA				wsk;
SOCKET				socket_server;
#else
int					socket_server;
#endif

int					addr_size;
int					active_port;
struct sockaddr_in	server_address;
HTTP_SESSION		http_session_;
HTTP_SESSION		*sessions[ MAX_CLIENTS ];
int					i_sac;
fd_set				master;
fd_set				read_fds;
int					fdmax;
int					newfd;
struct hostent		*host;
struct in_addr		addr;

int					http_conn_count = 0;
SEND_INFO			send_d[ MAX_CLIENTS ];

static void		socket_initialization( void );
static void		socket_prepare( void );
void			SOCKET_stop( void );

/*
socket_initialization( void )
- wersja dla systemu Windows
- inicjalizacja WinSock
- inicjalizacja socketa */
static void socket_initialization( void ) {
	LOG_print( "Waiting for Socket server initialization..." );

#ifdef _WIN32
	/*Inicjalizacja WinSock */
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsk ) != 0 ) {
		LOG_print( "\nError creating Winsock.\n" );
		printf( "Error creating Winsock.\n" );
		system( "pause" );
		exit( EXIT_FAILURE );
	}
#endif
	/*Utworzenie socketa nasï¿½uchujï¿½cego */
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
socket_prepare( void )
- wersja dla systemu Windows
- nasï¿½uchiwanie w celu odbioru danych od klienta */
static void socket_prepare( void ) {
	unsigned long b = 0;
	int i = 1;
	int wsa_result = 0;

	FD_ZERO( &master );
	FD_ZERO( &read_fds );

#ifndef _WIN32
	setuid( 0 );
	setgid( 0 );
#endif

	if ( bind( socket_server, ( struct sockaddr* )&server_address, sizeof( server_address ) ) == SOCKET_ERROR ) {
		wsa_result = WSAGetLastError();
		LOG_print( "bind() error: %d.\n", wsa_result );
		printf( "bind() error: %d.\n", wsa_result );
		SOCKET_stop();
		exit( EXIT_FAILURE );
	}

	if( setsockopt( socket_server, IPPROTO_TCP, TCP_NODELAY, ( char * )&i, sizeof( int ) ) == SOCKET_ERROR ) {
		wsa_result = WSAGetLastError();
		LOG_print( "setsockopt( TCP_NODELAY ) error: %d.\n", wsa_result );
		printf( "setsockopt( TCP_NODELAY ) error: %d.\n", wsa_result );
	}

	if( setsockopt( socket_server, SOL_SOCKET, SO_REUSEADDR, ( char * )&i, sizeof( int ) ) == SOCKET_ERROR ) {
		wsa_result = WSAGetLastError();
		LOG_print( "setsockopt( SO_REUSEADDR ) error: %d.\n", wsa_result );
		printf( "setsockopt( SO_REUSEADDR ) error: %d.\n", wsa_result );
	}

	/* Ustawienie na non-blocking socket */
	if( fcntl( socket_server, F_SETFL, &b ) == SOCKET_ERROR ) {
		wsa_result = WSAGetLastError();
		LOG_print( "ioctlsocket(): error: %d.\n", wsa_result );
		printf( "ioctlsocket(): error: %d.\n", wsa_result );
		SOCKET_stop();
		exit( EXIT_FAILURE );
	}

	/* Rozpoczï¿½cie nasï¿½uchiwania */
	if( listen( socket_server, MAX_CLIENTS ) == SOCKET_ERROR ) {
		wsa_result = WSAGetLastError();
		LOG_print( "listen() error: %d.\n", wsa_result );
		printf( "listen() error: %d.\n", wsa_result );
		SOCKET_stop();
		exit( EXIT_FAILURE );
	}

	LOG_print( "ok.\nSocket server is running:\n" );
	LOG_print( "- Port: %d.\n", active_port );
	LOG_print( "Lock and load...\n" );
	printf( "Lock and load...\n" );
	/* Teraz czekamy na poï¿½ï¿½czenia i dane */
}

/*
SOCKET_run( void )
- funkcja zarzï¿½dza poï¿½ï¿½czeniami przychodzï¿½cymi do gniazda. */
void SOCKET_run( void ) {
	register int i = 0;
	register int j = 0;
	struct timeval tv;
	char m_buf[ UPLOAD_BUFFER_CHAR ];
	int nwrite;

	/* Reset zmiennej informujï¿½cej o czï¿½ciowym odbiorze przychodzï¿½cej treï¿½ci */
	http_session_.http_info.received_all = -1;

	FD_SET( socket_server, &master );

	tv.tv_sec = 1;
	tv.tv_usec = 500000;

	fdmax = socket_server;

	for( ;"elvis presley lives"; ) {
		read_fds = master;
		if( select( fdmax+1, &read_fds, NULL, NULL, &tv ) == -1 ) {
			SOCKET_stop();
			exit( EXIT_FAILURE );
		}

		for(j = 0; j <= http_conn_count; j++) {
			if( send_d[ j ].http_content_size > 0 ) {
				if( send_d[ j ].m_buf_used == send_d[ j ].m_buf_len) {
					send_d[ j ].m_buf_len = fread( m_buf, sizeof( char ), UPLOAD_BUFFER, send_d[ j ].file );
					if( send_d[ j ].m_buf_len == 0) {
						printf("Closing file!\n");
						fclose( send_d[ j ].file );
						continue;
					}
					printf("Readed buf len: %d\n", send_d[ j ].m_buf_len);
					send_d[ j ].m_buf_used = 0;
				}

				assert( send_d[ j ].m_buf_len > send_d[ j ].m_buf_used );
				printf("m_buf_used: %d\n", send_d[ j ].m_buf_used);
				printf("m_buf_len: %d\n", send_d[ j ].m_buf_len);
				nwrite = send( send_d[ j ].socket_descriptor, m_buf + send_d[ j ].m_buf_used, send_d[ j ].m_buf_len - send_d[ j ].m_buf_used, 0 );
				printf("Sent: %d (%ld)\n", nwrite, send_d[ j ].http_content_size);
				send_d[ j ].http_content_size -= nwrite;
			}
		}
		i = fdmax+1;
		while( --i ) {
			if( FD_ISSET( i, &read_fds ) ) { /* Coï¿½ siï¿½ dzieje na sockecie... */
				if( i == socket_server ) {
					/* Podï¿½ï¿½czyï¿½ siï¿½ nowy klient */
					__sync_fetch_and_add( &http_conn_count, 1); /* Kolejny klient - do obsï¿½ugi bï¿½ï¿½du 503 */
					http_session_.address_length = sizeof( struct sockaddr );
					newfd = accept( socket_server, ( struct sockaddr* )&http_session_.address, &http_session_.address_length );

					if( newfd == -1 ) {
						LOG_print( "Socket error: accept().\n" );
					} else {
						send_d[ http_conn_count ].socket_descriptor = newfd;
						send_d[ http_conn_count ].http_content_size = 0;
						printf("Client with desc=%d\n", newfd);
						FD_SET( newfd, &master );
						if( newfd > fdmax ) {
							fdmax = newfd;
						}
					}
				} else {
					/* Podï¿½ï¿½czony klient przesï¿½aï¿½ dane... */
					SOCKET_process( ( void * )i );
				}
			} /*nowe poï¿½ï¿½czenie */
		} /*pï¿½tla deskryptorï¿½w while( --i )*/
	} /*for( ;; ) */
}

HTTP_SESSION* SOCKET_find_session_by_id( int socket ) {
	int i;

	printf("Looking for socket %d...", socket );
	for( i = 0; i <= http_conn_count; i++ ) {
		if( sessions[i]->socket_descriptor == socket ) {
			printf("ok.\n");
			return sessions[i];
		}
	}
	printf("not found.\n");
	return NULL;
}

SEND_INFO* SOCKET_find_response_struct_by_id( int socket ) {
	int i;

	printf("Looking for socket %d...", socket );
	for( i = 0; i <= http_conn_count; i++ ) {
		if( send_d[i].socket_descriptor == socket ) {
			printf("ok.\n");
			return &send_d[ i ];
		}
	}
	printf("not found.\n");
	return NULL;
}

/*
SOCKET_process_thread( void *socket_fd )
- funkcja uruchamia jest w w¹tku w celu wywo³ania SOCKET_process */
void SOCKET_process_thread( void *socket_fd ) {

	SOCKET_process( socket_fd );
}

/*
SOCKET_process( int socket_fd )
- funkcja odczytuje dane z socketu */
void SOCKET_process( void *socket_fd ) {
	HTTP_SESSION *session = ( HTTP_SESSION* )malloc( sizeof( HTTP_SESSION ) );
	char* tmp_buf = ( char* )malloc( MAX_BUFFER_CHAR );

	session->http_info.received_all = http_session_.http_info.received_all;
	session->address_length = http_session_.address_length;
	session->address = http_session_.address;
	session->socket_descriptor = ( int )socket_fd;

	sessions[ http_conn_count ] = session;

	if ( ( session->address_length = recv( ( int )socket_fd, tmp_buf, MAX_BUFFER, 0 ) ) <= 0 ) {
		/* ...ale to jednak byï¿½o rozï¿½ï¿½czenie */
		FD_CLR( ( int )socket_fd, &master );
		shutdown( ( int )socket_fd, SHUT_RDWR );
		close( ( int ) socket_fd );
		/* Zmniejszensie licznika podï¿½ï¿½czonych klientï¿½w */
		__sync_fetch_and_sub( &http_conn_count, 1);
	} else {
		/* Nie zostaï¿½y wczeï¿½niej odebrane wszystkie dane - metoda POST.
		Teraz trzeba je dokleiï¿½ do http_info.content_data */
		if( session->http_info.received_all == 0 ) {
			/* Obiekt jest juï¿½ stworzony, nie trzeba przydzielaï¿½ pamiï¿½ci */
			session->http_info.content_data = ( char* )realloc( session->http_info.content_data, strlen( session->http_info.content_data )+session->address_length+1 );
			strncat( session->http_info.content_data, tmp_buf, session->address_length );
			session->http_info.received_all = 1;
		} else if( session->http_info.received_all == -1 ) {
			/* Dla metod GET i HEAD */
			session->http_info.content_data = ( char* )malloc( session->address_length+1 );
			mem_allocated( session->http_info.content_data, 25 );
			strncpy( session->http_info.content_data, tmp_buf, session->address_length );
		}
		/* "Przerobienie" zapytania */
		SESSION_prepare( session );
	}

	if( tmp_buf ) {
		free( tmp_buf );
		tmp_buf = NULL;
	}

	if( session ) {
		free( session );
		session = NULL;
	}

//	pthread_mutex_lock( &mutexsum );
//	threads_count--;
//	pthread_mutex_unlock( &mutexsum );
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
@http_session - wskaï¿½nik do podï¿½ï¿½czonego klienta
- funkcja resetuje zmienne informujï¿½ce o podï¿½ï¿½czonym sockecie. */
void SOCKET_release( HTTP_SESSION *http_session ) {
	http_session->socket_descriptor = -1;
	http_session->address_length = -1;
	http_session->http_info.keep_alive = -1;
}

/*
SOCKET_disconnect_client( HTTP_SESSION *http_session )
- rozï¿½ï¿½cza klienta podanego jako struktura http_session */
void SOCKET_disconnect_client( HTTP_SESSION *http_session ) {
	if( http_session->socket_descriptor != SOCKET_ERROR ) {
		FD_CLR( http_session->socket_descriptor, &master );
		shutdown( http_session->socket, SHUT_RDWR );
		close( http_session->socket_descriptor );
	} else {
		SOCKET_release( http_session );
	}

	/* Zmniejszenie licznika podï¿½ï¿½czonych klientï¿½w */
	__sync_fetch_and_sub(&http_conn_count, 1);
}

/*
SOCKET_send( HTTP_SESSION *http_session, char *buf, int http_content_size )
- wysyï¿½a pakiet danych ( buf ) do danego klienta ( http_session ) */
void SOCKET_send( HTTP_SESSION *http_session, const char *buf, int http_content_size, int *res ) {
	//write(http_session->socket_descriptor, buf, http_content_size );
	if( ( http_session->address_length = send( http_session->socket_descriptor, buf, http_content_size, 0 ) ) <= 0 ) {
		SOCKET_disconnect_client( http_session );
		if( res ) {
			*res = http_session->address_length;
		}
	} else {
		if( res ) {
			*res = http_session->address_length;
		}
	}
}

/*
server_get_remote_hostname( const char *remote_addr )
@http_session - wskaŸnik do pod³¹czonego klienta
- zwraca ciï¿½g znakï¿½w bï¿½dï¿½cy nazwï¿½ hosta. */
char* server_get_remote_hostname( HTTP_SESSION *http_session ) {
	static char remote_name[TINY_BUFF_SIZE];
	memset( remote_name, '\0', TINY_BUFF_SIZE );
	getnameinfo( ( struct sockaddr * )&http_session->address, sizeof( http_session->address ), remote_name, sizeof( remote_name ), NULL, 0, NI_NAMEREQD );
	return ( ( char* )&remote_name );
}

/*
SOCKET_get_remote_ip( HTTP_SESSION *http_session )
@http_session - wskaŸnik do pod³¹czonego klienta
- zwraca ciï¿½g znakï¿½w bï¿½dï¿½cy adresem IP. */
char* SOCKET_get_remote_ip( HTTP_SESSION *http_session ) {
	static char ip_addr[TINY_BUFF_SIZE];
	memset( ip_addr, '\0', TINY_BUFF_SIZE );
	getnameinfo( ( struct sockaddr * )&http_session->address, sizeof( http_session->address ), ip_addr, sizeof( ip_addr ), NULL, 0, NI_NUMERICHOST );
	return ( ( char* )&ip_addr );
}

/*
SOCKET_main( void )
- obsï¿½uga funkcji socketï¿½w */
void SOCKET_main( void ) {
	( void )socket_initialization();
	( void )socket_prepare();
	( void )SOCKET_run();
	( void )SOCKET_stop();
}
