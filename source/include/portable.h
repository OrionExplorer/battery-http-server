/*******************************************************************

Projekt battery-http-server

Plik: portable.h

Przeznaczenie:
Dzięki definicjom w tym pliku możliwe jest ujednolicenie kodu dla rónych platform.
Definicje, które nie są uniwersalne swoje miejsce znalazły w adekwatnych plikach źródłowychh.

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef PORTABLE_H
#define PORTABLE_H


#ifdef _WIN32
    #include <direct.h>
/* Ścieżki do plików/folderów na systemach WIN32 są oddzielone "\" */
    #define SLASH               "\\"
    #define C_SLASH             '\\'
/* Dla zachowania kompatybilności z Linux */
/* Potoki - dla uruchamiania skryptów CGI */
    #define READ_BINARY         "rb"
    #define popen               _popen
    #define pclose              _pclose
/* Zmienne systemowe */
    #define putenv              _putenv
/* Operacje na plikach i folderach */
    #define chdir               _chdir
    #define mkdir( a,b )        mkdir( a )
    #define S_IREAD             _S_IREAD
/* Właściwości socketów */
    #define fcntl               ioctlsocket
    #define close               closesocket
    #define SHUT_RDWR           SD_BOTH
    #define F_SETFL             FIONBIO
    #define S_IXOTH             S_IEXEC
    #define sighandler          __p_sig_fn_t
    #define MSG_NOSIGNAL        0
    #define EWOULDBLOCK         WSAEWOULDBLOCK
    #define sleep               Sleep
#else
/* Ścieżki do plików/folderów na systemach LINUX są oddzielone "/" */
    #define SLASH               "/"
    #define C_SLASH             '/'
/* Dla zachowania kompatybilności z WinSock ( ! )*/
    #define SOCKET_ERROR        ( -1 )
    #define EXIT_FAILURE        ( 1 )
    #define READ_BINARY         "re"
    #define sighandler          __sighandler_t
    #define Sleep(x)            usleep(x*1000)
    #define GetLastError()      errno
    #define WSAGetLastError()   errno
#endif

#endif
