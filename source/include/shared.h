/*******************************************************************

Projekt battery-http-server

Plik: shared.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef SHARED_H
#define SHARED_H

#include "portable.h"
#include "string_utils.h"
#include "time_utils.h"
#include "mem_manager.h"
#include <stdio.h>
#include <time.h>

#define MAX_OPEN_FILES                      1024

#ifndef FD_SETSIZE
    #define FD_SETSIZE                      1024
#endif

/* Poniższe definicje są konieczne z powodu "FIXME" w implementacji biblioteki Ws2tcpip.h w MinGW */
#ifdef _WIN32

    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT                0x0501
    #endif

    #include <WinSock2.h>
    #include <WS2tcpip.h>
#endif

#ifdef _MSC_VER
    #pragma comment( lib, "WS2_32.lib" )
#endif

#ifndef _MSC_VER
    #include <sys/types.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

#ifdef __linux__
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <errno.h>
#endif

#ifdef __linux__
    #define APP_PLATFORM                    "linux"
#elif _WIN32
    #define APP_PLATFORM                    "windows"
#endif

#define APP_NAME                            "battery"
#define APP_VER                             APP_PLATFORM"-1.0.0"
#define SERVER_NAME                         APP_NAME"/"APP_VER

#define LOGS_PATH                           "logs"SLASH
#define CONFIG_PATH                         "configuration"SLASH
#define NETWORK_CFG_PATH                    CONFIG_PATH"battery.conf"

#define DEFAULT_PORT                        80

#define MAX_BUFFER                          65535
#define MAX_BUFFER_CHAR                     65535*sizeof( char )
#define UPLOAD_BUFFER                       16384
#define UPLOAD_BUFFER_CHAR                  16384*sizeof( char )
#define LOG_BUFFER                          10240
#define LOG_BUFFER_CHAR                     10240*sizeof( char )
#define BIG_BUFF_SIZE                       2048
#define BIG_BUFF_SIZE_CHAR                  2048*sizeof( char )
#define MEDIUM_BUFF_SIZE                    1024
#define MEDIUM_BUFF_SIZE_CHAR               1024*sizeof( char )
#define STD_BUFF_SIZE                       256
#define STD_BUFF_SIZE_CHAR                  256*sizeof( char )
#define TIME_BUFF_SIZE                      30
#define TIME_BUFF_SIZE_CHAR                 30*sizeof( char )
#define SMALL_BUFF_SIZE                     32
#define SMALL_BUFF_SIZE_CHAR                32*sizeof( char )
#define TINY_BUFF_SIZE                      16
#define TINY_BUFF_SIZE_CHAR                 16*sizeof( char )
#define PROTO_BUFF_SIZE                     10
#define PROTO_BUFF_SIZE_CHAR                10*sizeof( char )
#define MICRO_BUFF_SIZE                     8
#define MICRO_BUFF_SIZE_CHAR                8*sizeof( char )
#define EXT_LEN                             16
#define EXT_LEN_CHAR                        16*sizeof( char )

#define MAX_EVENTS                          10240
#define MAX_CLIENTS                         10240

#define MAX_PATH_LENGTH                     1024
#define MAX_PATH_LENGTH_CHAR                1024*sizeof( char )

#define RFC1123FMT                          "%a, %d %b %Y %H:%M:%S GMT"

/* Przechowuje ścieżkę, w której znajduje się plik wykonywalny */
char                            app_path[ MAX_PATH_LENGTH ];

/* Przechowuje ścieżkę, z której będą serwowane zasoby */
extern char*                    document_root;

/* Zaimplementowane metody dostępu do zasobów serwera */
extern const char               *http_method_list[ ];


/* Obsługiwane metody protokołu HTTP */
typedef enum http_m {
    UNKNOWN_HTTP_METHOD,
    GET,
    HEAD,
    POST
} http_m;

/* Typ żądanego zasobu */
typedef enum RESOURCE_TYPE {
    NONE,
    STD_FILE,
    SCRIPT
} RESOURCE_TYPE;

/* Struktura przechowująca informacje o prawach dostę do danych zasobów */
typedef struct HT_ACCESS {
    char res_filename[ MAX_PATH_LENGTH ];           /* Nazwa zasobu */
    char res_login[ SMALL_BUFF_SIZE ];              /* Wymagany login */
    char res_pwd[ SMALL_BUFF_SIZE ];                /* Wymagane hasło */
    char res_auth[ STD_BUFF_SIZE ];
} HT_ACCESS;

/* Struktura przechowująca informacje o mime-type dla danego rozszerzenia pliku */
typedef struct MIME_TYPE {
    char ext[ EXT_LEN ];                            /* Rozszerzenie */
    char mime_type[ SMALL_BUFF_SIZE ];              /* MIME */
} MIME_TYPE;

/* Struktura przechowująca informacje o żądaniu HTTP */
typedef struct HTTP_INFO {
    long                range_st;                   /* Dla nagłówka "Range" */
    long                range_en;                   /* Dla nagłówka "Range" */
    size_t              content_length;             /* Dla nagłówka "Content-Length" */
    http_m              method_name;                /* Typ żądanej metody ( GET/HEAD/POST ) */
    char*               content_data;               /* Całkowita zawartość żądania */
    char*               header;                     /* Nagłówki z żądania */
    char*               request_line;               /* Pierwsza linia z żądania */
    char*               http_local_path;            /* Do przechowania żądanej ścieżki do zasobu */
    char*               protocol_ver;               /* Wersja protokołu klienta */
    char*               date_if_modified_since;     /* Dla nagłówka "If-Modified-Since" */
    char*               date_if_unmodified_since;   /* Dla nagłówka "If-Unmodified-Since" */
    char*               remote_addr;                /* Adres IP, z którego łączy się klient */
    char*               remote_host;                /* Nazwa hosta, z którego łączy się klient */
    char*               content_type;               /* Dla nagłówka "Content-Type" */
    char*               query_string;               /* Dla skryptów CGI */
    char*               authorization;              /* Login i hasło do żądanego zasobu */
    char*               user_login;                 /* Odszyfrowany login użytkownika */
    char*               user_pwd;                   /* Odszyfrowane hasło użytkownika */
    short               keep_alive;                 /* Przechowuje informację o typie połączenia ( Close/Keep-Alive ) */
    short               received_all;               /* Dla żądań typu POST - informuje, czy odebrano całą wiadomość */
} HTTP_INFO;

/* Struktura przechowuje informacje szczegółowe o lokalnym zasobie */
typedef struct LOCAL_INFO {
    char*               date_res_last_modified;     /* Przechowuje informację kiedy zasób został ostatnio zmodyfikowany */
} LOCAL_INFO;

/* Struktura przechowuje informacje o statusie wysyłki lokalnego zasobu */
typedef struct SEND_INFO {
    FILE*               file;                       /* Deskryptor otwartego pliku */
    long                http_content_size;          /* Rozmiar pliku */
    size_t              sent_size;                  /* Ilość danych wysłana do tej pory */
    int                 socket_fd;                  /* Deksryptor podłączonego klienta, który wysłał żądanie */
    size_t              total_size;                 /* Przechowuje całkowity rozmiar pliku */
    short               keep_alive;                 /* Informuje, czy utrzymać połączenie po zrealizowaniu żądania */
} SEND_INFO;

/* Struktura przechowuje informacje o otwartym pliku */
typedef struct OPENED_FILE {
    char                filename[ FILENAME_MAX ];   /* Nazwa */
    FILE*               file;                       /* Deskryptor otwartego pliku */
    size_t              size;                       /* Rozmiar */
    int                 socket_fd;                  /* Deskryptor podłączonego klienta, który wysłał żądanie */
    RESOURCE_TYPE       type;                       /* Rodzaj zasobu */
} OPENED_FILE;

/* Struktura przechowuje treść otwartego pliku */
typedef struct FILE_CACHE {
    char                filename [ FILENAME_MAX ];  /* Nazwa */
    FILE*               file;                       /* Deskryptor otwartego pliku */
    char*               content;                    /* Zawartość pliku - cache */
} FILE_CACHE;

/* Główna struktura, która będzie przechowywała wszystkie informacje o połączonym kliencie */
typedef struct HTTP_SESSION {
    struct sockaddr_in      address;                /* Zmienna pomocnicza */
    ssize_t                 recv_data_len;          /* Przechowuje długość otrzymanego ciągu znaków*/
    int                     socket_fd;              /* Deskryptor podłączoego klienta */
    HTTP_INFO               http_info;              /* Informacje o żądaniu HTTP */
    LOCAL_INFO              local_info;             /* Informacje o pliku */
} HTTP_SESSION;


#ifdef _WIN32
    extern WSADATA          wsk;                                /* [Win32] Inicjalizacja Winsock */
    extern SOCKET           socket_server;                      /* [Win32] Główny deskryptor gniazda serwera */
#elif __linux__
    extern int              socket_server;                      /* Główny deskryptor gniazda serwera*/
#endif

/* Przechowuje informację o porcie, na ktorym serwer ma działać */
extern int                  active_port;

/* Przechowuje informacje o danych gotowych do wysyłki dla poszczególnych klientów */
extern SEND_INFO            send_d[ MAX_CLIENTS ];

/* Przechowuje informacje pozwalające do jednorazowego otwarcia pliku żądanego przez więcej niż jednego klienta */
extern OPENED_FILE          opened_files[ MAX_OPEN_FILES ];

/* Przechowuje treści plików żądanych aktualnie przez klientów */
extern FILE_CACHE           cached_files[ MAX_OPEN_FILES ];

/* Przechowuje liczbę aktualnie podłączonych klientów - w celu obsługi błędu 501 */
extern int                  http_conn_count;

/* Przechowuje informacje o dostępach do zasobów */
HT_ACCESS                   ht_access[ 256 ];
extern int                  ht_access_count;

/* Przechowuje wczytane rozszerzenia plików wraz z ich typami MIME */
MIME_TYPE                   mime_types[ STD_BUFF_SIZE ];
extern int                  mime_types_count;

/* Wersja protokołu */
typedef enum IP_VER {
    IPv4 = 4,
    IPv6 = 6
} IP_VER;

/* Opcjonalne wykorzystanie funkcji sendfile dla osiągnięcia "zero-copy". */
extern int                  use_sendfile;

/* Metoda przetwarzania połączeń */
typedef enum CONN_PROC {
    CP_EPOLL,               /* epoll() */
    CP_SELECT               /* select() */
} CONN_PROC;
extern CONN_PROC            connection_processor;

/* Lista możliwych plików typu index */
char                        *index_file_list[ MICRO_BUFF_SIZE ];
extern int                  index_file_count;

#endif
