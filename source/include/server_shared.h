/*******************************************************************

Projekt battery_Server

Plik: server_shared.h

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#ifndef SERVER_SHARED_H
#define SERVER_SHARED_H

#include "server_portable.h"
#include "server_strings_util.h"
#include "server_time_util.h"
#include "server_mem_manager.h"
#include <time.h>

#define FD_SETSIZE	1024

/* Poniï¿½sze definicje sï¿½ konieczne z powodu "FIXME" w implementacji biblioteki Ws2tcpip.h w MinGW */
#ifdef _WIN32
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif
/* Obsï¿½uga socketï¿½w na systemach WIN32 jest w bibliotece WinSock */
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

#ifdef _MSC_VER
#pragma comment(lib, "WS2_32.lib")
#endif

#ifndef _WIN32
/* Obsï¿½uga socketï¿½w na systemach LINUX */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#define APP_VER								"0.7"
#define SERVER_NAME							APP_NAME"/"APP_VER

#define LOGS_PATH							"logs"SLASH
#define CONFIG_PATH							"configuration"SLASH
#define NETWORK_CFG_PATH					CONFIG_PATH"network.conf"
#define SCRIPTS_CFG_PATH					CONFIG_PATH"scripts.conf"
#define MIME_TYPES_CFG_PATH					CONFIG_PATH"mime_type.conf"
#define HT_ACCESS_CFG_PATH					CONFIG_PATH"ht_access.conf"
#define INDEX_FILE_CFG_PATH					CONFIG_PATH"index.conf"

#define DEFAULT_PORT						80

#define MAX_BUFFER							65535
#define MAX_BUFFER_CHAR						65535*sizeof(char)
#define UPLOAD_BUFFER						16384
#define UPLOAD_BUFFER_CHAR					16384*sizeof(char)
#define LOG_BUFFER							10240
#define LOG_BUFFER_CHAR						10240*sizeof(char)
#define BIG_BUFF_SIZE						2048
#define BIG_BUFF_SIZE_CHAR					2048*sizeof(char)
#define MEDIUM_BUFF_SIZE					1024
#define MEDIUM_BUFF_SIZE_CHAR				1024*sizeof(char)
#define STD_BUFF_SIZE						256
#define STD_BUFF_SIZE_CHAR					256*sizeof(char)
#define TIME_BUFF_SIZE						30
#define TIME_BUFF_SIZE_CHAR					30*sizeof(char)
#define SMALL_BUFF_SIZE						32
#define SMALL_BUFF_SIZE_CHAR				32*sizeof(char)
#define TINY_BUFF_SIZE						16
#define TINY_BUFF_SIZE_CHAR					16*sizeof(char)
#define PROTO_BUFF_SIZE						10
#define PROTO_BUFF_SIZE_CHAR				10*sizeof(char)
#define MICRO_BUFF_SIZE						8
#define MICRO_BUFF_SIZE_CHAR				8*sizeof(char)
#define EXT_LEN								8
#define EXT_LEN_CHAR						8*sizeof(char)

#define MAX_PATH_LENGTH						1024
#define MAX_PATH_LENGTH_CHAR				1024*sizeof(char)
#define MAX_CLIENTS							FD_SETSIZE

#define RFC1123FMT							"%a, %d %b %Y %H:%M:%S GMT"

char				app_path[MAX_PATH_LENGTH];
extern char*		working_dir;

typedef enum {
	UNKNOWN_HTTP_METHOD,
	GET,
	HEAD,
	POST
} http_m;

/* Zaimplementowane metody dostï¿½pu do zasobï¿½w serwera */
extern const char *http_method_list[];


/* Struktura przechowujï¿½ca informacje o prawach dostï¿½pu do danych zasobï¿½w */
typedef struct
{
	char res[MAX_PATH_LENGTH];
	char res_login[SMALL_BUFF_SIZE];
	char res_pwd[SMALL_BUFF_SIZE];
	char res_auth[STD_BUFF_SIZE];
} HT_ACCESS;

/* Struktura przechowujï¿½ca informacje o mime-type dla danego rozszerzenia pliku */
typedef struct
{
	char ext[EXT_LEN];
	char mime_type[SMALL_BUFF_SIZE];
} MIME_TYPE;

/* Struktura przechowujï¿½ca informacje o ï¿½ï¿½daniu HTTP */
typedef struct
{
	long				range_st;					/* Dla nagï¿½ï¿½wka "Range" */
	long				range_en;					/* Dla nagï¿½ï¿½wka "Range" */
	long				content_length;				/* Dla nagï¿½ï¿½wka "Content-Length" */
	http_m				method_name;				/* Typ ï¿½ï¿½danej metody (GET/HEAD/POST) */
	char*				content_data;				/* Caï¿½kowita zawartoï¿½ï¿½ ï¿½ï¿½dania */
	char*				header;						/* Nagï¿½ï¿½wki z ï¿½ï¿½dania */
	char*				request_line;				/* Pierwsza linia z ï¿½ï¿½dania */
	char*				http_local_path;			/* Do przechowania ï¿½ï¿½danej ï¿½cieï¿½ki do zasobu */
	char*				protocol_ver;				/* Wersja protokoï¿½u klienta */
	char*				date_if_modified_since;		/* Dla nagï¿½ï¿½wka "If-Modified-Since" */
	char*				date_if_unmodified_since;	/* Dla nagï¿½ï¿½wka "If-Unmodified-Since" */
	char*				remote_addr;				/* Adres IP, z ktï¿½rego ï¿½ï¿½czy siï¿½ klient */
	char*				remote_host;				/* Nazwa hosta, z ktï¿½rego ï¿½ï¿½czy siï¿½ klient */
	char*				content_type;				/* Dla nagï¿½ï¿½wka "Content-Type" */
	char*				query_string;				/* Dla skryptï¿½w CGI */
	char*				authorization;				/* Login i hasï¿½o do ï¿½ï¿½danego zasobu */
	char*				user_login;					/* Odszyfrowany login uï¿½ytkownika */
	char*				user_pwd;					/* Odszyfrowane hasï¿½o uï¿½ytkownika */
	short				keep_alive;					/* Przechowuje informacjï¿½ o typie poï¿½ï¿½czenia (Close/Keep-Alive) */
	short				is_cgi;						/* 1 = ï¿½ï¿½danie jest operacjï¿½ na skrypcie, 0 = przesï¿½anie zawartoï¿½ci zasobu */
	short				received_all;				/* Dla ï¿½ï¿½daï¿½ typu POST - informuje, czy odebrano caï¿½ï¿½ wiadomoï¿½ï¿½ */
} HTTP_INFO;

typedef struct LOCAL_INFO
{
	char*				date_res_last_modified;		/* Przechowuje informacjï¿½ kiedy zasï¿½b zostaï¿½ ostatnio zmodyfikowany */
} LOCAL_INFO;

/* Gï¿½ï¿½wna struktura, ktï¿½ra bï¿½dzie przechowywaï¿½a wszystkie informacje o poï¿½ï¿½czonym kliencie */
typedef struct
{
	struct sockaddr_in		address;
#ifdef _WIN32
	SOCKET					socket;
#else
	int						socket;
#endif
	fd_set					socket_data;
	int						address_length;
	int						socket_descriptor;
	HTTP_INFO				http_info;
	LOCAL_INFO				local_info;
} HTTP_SESSION;

/* Struktura przechowujï¿½ca informacje o innych moï¿½liwych rozszerzeniach plikï¿½w,
ktï¿½re majï¿½ uprawnienia do wykonania jako skrypt CGI */
typedef struct									
{
	char ext[EXT_LEN];					/* Rozszerzenie*/
	char external_app[STD_BUFF_SIZE];	/* Zewnï¿½trzna aplikacja do uruchamiania skryptu */
	char param[STD_BUFF_SIZE];			/* Parametr, z ktï¿½rym ma zostaï¿½ uruchomiona aplikacja ze zmiennej external_app */
} OTHER_SCRIPTS;

#ifdef _WIN32
extern WSADATA				wsk;
extern SOCKET				socket_server;
#else
extern int					socket_server;
#endif
extern int					addr_size;
extern int					active_port;
extern struct sockaddr_in	server_address;
extern int					ip_proto_ver;
extern HTTP_SESSION			http_session_;
extern fd_set				master;
extern int					http_conn_count;
char*						server_get_remote_hostname(HTTP_SESSION *http_session);

/* Przechowuje wczytane rozszerzenia plikï¿½w uprawnionych do wykonania jako skrypt CGI */
OTHER_SCRIPTS				other_script_type[8];
extern int					cgi_other_count;
#define STD_EXEC			"<exec>"

/* Przechowuje informacje o dostï¿½pach do zasobï¿½w */
HT_ACCESS					ht_access[256];
extern int					ht_access_count;

/* Przechowuje wczytane rozszerzenia plików wraz z ich typami MIME */
MIME_TYPE					mime_types[STD_BUFF_SIZE];
extern int					mime_types_count;

/* Wersja protokoï¿½u */
enum IP_VER { IPv4 = 4, IPv6 = 6 };

/* Lista moï¿½liwych plikï¿½w typu index */
char		*index_file_list[MICRO_BUFF_SIZE];
extern int	index_file_count;

#endif
