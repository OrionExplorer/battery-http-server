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



/* Ścieżki do plików/folderów na systemach LINUX są oddzielone "/" */
#define SLASH               "/"
#define C_SLASH             '/'
/* Dla zachowania kompatybilności z WinSock ( ! )*/
#define SOCKET_ERROR        ( -1 )
#define EXIT_FAILURE        ( 1 )
#define READ_BINARY         "re"
#define sighandler          __sighandler_t
#define Sleep(x)            usleep(x*1000)

#endif
