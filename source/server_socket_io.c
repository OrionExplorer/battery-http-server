/*******************************************************************

Projekt battery_Server

Plik: server_socket_io.c

Przeznaczenie:
Inicjalizacja socketï¿½w
Konfiguracja socketï¿½w
Odbieranie danych z sieci i przekazanie do interpretacji

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#include "include/server_socket_io.h"
#include "include/server_create_session.h"
#include "include/server_http_protocol.h"
#include "include/server_shared.h"
#include "include/server_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

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
int					i_sac;
fd_set				master;
fd_set				read_fds;
int					fdmax;
int					newfd;
struct hostent		*host;
struct in_addr		addr;

int					http_conn_count = 0;
char				*tmp_buf;

static void		socket_initialization(void);
static void		socket_prepare(void);
void			socket_free(void);
/*
socket_initialization(void)
- wersja dla systemu Windows
- inicjalizacja WinSock
- inicjalizacja socketa */
static void socket_initialization(void)
{
	print_log("Waiting for Socket server initialization...");

#ifdef _WIN32
	/*Inicjalizacja WinSock */
	if (WSAStartup(MAKEWORD(2, 2), &wsk) != 0)
	{
		print_log("\nError creating Winsock.\n");
		printf("Error creating Winsock.\n");
		system("pause");
		exit(EXIT_FAILURE);
	}
#endif
	/*Utworzenie socketa nasï¿½uchujï¿½cego */
	socket_server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_server == SOCKET_ERROR)
	{
		print_log("Error creating socket.\n");
		printf("Error creating socket.\n");
		socket_free();
		exit(EXIT_FAILURE);
	}

	if(active_port < 0 || active_port > 65535)
		active_port = DEFAULT_PORT;

	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons((u_short)active_port);
}

/*
socket_prepare(void)
- wersja dla systemu Windows
- nasï¿½uchiwanie w celu odbioru danych od klienta */
static void socket_prepare(void)
{
	unsigned long b = 0;
	int i = 1;
	int wsa_result = 0;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

#ifndef _WIN32
	setuid(0);
	setgid(0);
#endif

	if (bind(socket_server, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR)
	{
		wsa_result = WSAGetLastError();
		print_log("bind() error: %d.\n", wsa_result);
		printf("bind() error: %d.\n", wsa_result);
		socket_free();
		exit(EXIT_FAILURE);
	}

	if(setsockopt(socket_server, IPPROTO_TCP, TCP_NODELAY, (char *)&i, sizeof(int)) == SOCKET_ERROR)
	{
		wsa_result = WSAGetLastError();
		print_log("setsockopt(TCP_NODELAY) error: %d.\n", wsa_result);
		printf("setsockopt(TCP_NODELAY) error: %d.\n", wsa_result);
	}

	if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, (char *)&i, sizeof(int)) == SOCKET_ERROR)
	{
		wsa_result = WSAGetLastError();
		print_log("setsockopt(SO_REUSEADDR) error: %d.\n", wsa_result);
		printf("setsockopt(SO_REUSEADDR) error: %d.\n", wsa_result);
	}

	/* Ustawienie na non-blocking socket */
	if(fcntl(socket_server, F_SETFL, &b) == SOCKET_ERROR)
	{
		wsa_result = WSAGetLastError();
		print_log("ioctlsocket(): error: %d.\n", wsa_result);
		printf("ioctlsocket(): error: %d.\n", wsa_result);
		socket_free();
		exit(EXIT_FAILURE);
	}

	/* Rozpoczï¿½cie nasï¿½uchiwania */
	if(listen(socket_server, MAX_CLIENTS) == SOCKET_ERROR)
	{
		wsa_result = WSAGetLastError();
		print_log("listen() error: %d.\n", wsa_result);
		printf("listen() error: %d.\n", wsa_result);
		socket_free();
		exit(EXIT_FAILURE);
	}

	print_log("ok.\nSocket server is running:\n");
	print_log("- Port: %d.\n", active_port);
	print_log("Lock and load...\n");
	/* Teraz czekamy na poï¿½ï¿½czenia i dane */
}

/*
socket_run(void)
- funkcja zarzï¿½dza poï¿½ï¿½czeniami przychodzï¿½cymi do gniazda. */
void socket_run(void)
{
	register int i = 0;
	struct timeval tv;

	/* Reset zmiennej informujï¿½cej o czï¿½ciowym odbiorze przychodzï¿½cej treï¿½ci */
	http_session_.http_info.received_all = -1;

	FD_SET(socket_server, &master);

	tv.tv_sec = 1;
	tv.tv_usec = 500000;

	fdmax = socket_server;

	for(;"elvis presley lives";)
	{
		read_fds = master;
		if(select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1)
		{
			print_log("Socket error: select().\n");
			printf("Socket error: select().\n");
			socket_free();
			exit(EXIT_FAILURE);
		}

		i = fdmax+1;
		while(--i)
		{
			if(FD_ISSET(i, &read_fds)) /* Coï¿½ siï¿½ dzieje na sockecie... */
			{
				if(i == socket_server)	/* Podï¿½ï¿½czyï¿½ siï¿½ nowy klient */
				{
					/* Kolejny klient - do obsï¿½ugi bï¿½ï¿½du 503 */
					http_conn_count++;
					http_session_.address_length = sizeof(struct sockaddr);
					newfd = accept(socket_server, (struct sockaddr*)&http_session_.address, &http_session_.address_length);

					if(newfd == -1)
						print_log("Socket error: accept().\n");
					else
					{
						FD_SET(newfd, &master);
						if(newfd > fdmax)
							fdmax = newfd;
					}
				}
				else /* Podï¿½ï¿½czony klient przesï¿½aï¿½ dane... */
				{
					tmp_buf = (char*)malloc(MAX_BUFFER_CHAR);

					if ((http_session_.address_length = recv(i, tmp_buf, MAX_BUFFER, 0)) <= 0)
					{
						/* ...ale to jednak byï¿½o rozï¿½ï¿½czenie */
						FD_CLR(i, &master);
						shutdown(i, SHUT_RDWR);
						close(i);
						/* Zmniejszensie licznika podï¿½ï¿½czonych klientï¿½w */
						http_conn_count--;
					}
					else
					{
						/* Nie zostaï¿½y wczeï¿½niej odebrane wszystkie dane - metoda POST.
						Teraz trzeba je dokleiï¿½ do http_info.content_data */
						if(http_session_.http_info.received_all == 0)
						{
							/* Obiekt jest juï¿½ stworzony, nie trzeba przydzielaï¿½ pamiï¿½ci */
							http_session_.http_info.content_data = (char*)realloc(http_session_.http_info.content_data, strlen(http_session_.http_info.content_data)+http_session_.address_length+1);
							strncat(http_session_.http_info.content_data, tmp_buf, http_session_.address_length);
							http_session_.http_info.received_all = 1;
						}
						else if(http_session_.http_info.received_all == -1)
						{
							/* Dla metod GET i HEAD */
							http_session_.http_info.content_data = (char*)malloc(http_session_.address_length+1);
							mem_allocated(http_session_.http_info.content_data, 25);
							strncpy(http_session_.http_info.content_data, tmp_buf, http_session_.address_length);
						}

						/* Przypisanie deskryptora */
						http_session_.socket_descriptor = i;
						
						/* "Przerobienie" zapytania */
						data_prepare_http_session(&http_session_);
					}

					if(tmp_buf)
					{
						free(tmp_buf);
						tmp_buf = NULL;						
					}
				}
			} /*nowe poï¿½ï¿½czenie */
		} /*pï¿½tla deskryptorï¿½w while(--i)*/
	} /*for(;;) */
}

/*
socket_free(void)
- zwolnienie WinSock
- zwolnienie socketa */
void socket_free(void)
{
	print_log("socket_free(%d):\n", fdmax);
	print_log("\t- shutdown(%d)...", socket_server);
	shutdown(socket_server, SHUT_RDWR);
	print_log("ok.\n");
	print_log("\t- close(%d)...", http_session_.socket);
	close(http_session_.socket);
	print_log("ok.\n");
	print_log("\t- close(%d)...", socket_server);
	close(socket_server);
	print_log("ok.\n");

#ifdef _WIN32
	print_log("\t- WSACleanup()...");
	WSACleanup();
	print_log("ok.\n");
#endif
}

/*
server_release_socket(HTTP_SESSION *http_session)
@http_session - wskaï¿½nik do podï¿½ï¿½czonego klienta
- funkcja resetuje zmienne informujï¿½ce o podï¿½ï¿½czonym sockecie. */
void server_release_socket(HTTP_SESSION *http_session)
{
	http_session->socket_descriptor = -1;
	http_session->address_length = -1;
	http_session->http_info.keep_alive = -1;
}

/*
server_disconnect_client(HTTP_SESSION *http_session)
- rozï¿½ï¿½cza klienta podanego jako struktura http_session */
void server_disconnect_client(HTTP_SESSION *http_session)
{
	if(http_session->socket_descriptor != SOCKET_ERROR)
	{
		FD_CLR(http_session->socket_descriptor, &master);
		shutdown(http_session->socket, SHUT_RDWR);
		close(http_session->socket_descriptor);
	}
	else server_release_socket(http_session);

	/* Zmniejszenie licznika podï¿½ï¿½czonych klientï¿½w */
	http_conn_count--;
}

/*
server_send_data_to_client(HTTP_SESSION *http_session, char *buf, int http_content_size)
- wysyï¿½a pakiet danych (buf) do danego klienta (http_session) */
int server_send_data_to_client(HTTP_SESSION *http_session, const char *buf, int http_content_size)
{
	if((http_session->address_length = send(http_session->socket_descriptor, buf, http_content_size, 0)) <= 0)
	{
		server_disconnect_client(http_session);
		return 0;
	}
	else return 1;
}

/*
server_get_remote_hostname(const char *remote_addr)
@http_session - wskaŸnik do pod³¹czonego klienta
- zwraca ciï¿½g znakï¿½w bï¿½dï¿½cy nazwï¿½ hosta. */
char* server_get_remote_hostname(HTTP_SESSION *http_session)
{
	static char remote_name[TINY_BUFF_SIZE];
	memset(remote_name, '\0', TINY_BUFF_SIZE);
	getnameinfo((struct sockaddr *)&http_session->address, sizeof(http_session->address), remote_name, sizeof(remote_name), NULL, 0, NI_NAMEREQD);
	return ((char*)&remote_name);
}

/*
server_get_remote_ip_address(HTTP_SESSION *http_session)
@http_session - wskaŸnik do pod³¹czonego klienta
- zwraca ciï¿½g znakï¿½w bï¿½dï¿½cy adresem IP. */
char* server_get_remote_ip_address(HTTP_SESSION *http_session)
{
	static char ip_addr[TINY_BUFF_SIZE];
	memset(ip_addr, '\0', TINY_BUFF_SIZE);
	getnameinfo((struct sockaddr *)&http_session->address, sizeof(http_session->address), ip_addr, sizeof(ip_addr), NULL, 0, NI_NUMERICHOST);
	return ((char*)&ip_addr);
}

/*
server_start_socket(void)
- obsï¿½uga funkcji socketï¿½w */
void server_start_socket(void)
{
	(void)socket_initialization();
	(void)socket_prepare();
	(void)socket_run();
	(void)socket_free();
}
