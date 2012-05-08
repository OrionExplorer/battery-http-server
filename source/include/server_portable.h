/*******************************************************************

Projekt battery_Server

Plik: server_portable.h

Przeznaczenie:
Dzi�ki definicjom w tym pliku mo�liwe jest ujednolicenie kodu dla r�nych platform.
Definicje, kt�re nie s� uniwersalne swoje miejsce znalaz�y w adekwatnych plikach �r�d�owychh.

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#ifndef SERVER_PORTABLE_H
#define SERVER_PORTABLE_H

/* #undef _WIN32 */

#ifdef _WIN32
	#include <direct.h>
/* �cie�ki do plik�w/folder�w na systemach WIN32 s� oddzielone "\" */
	#define SLASH		"\\"
	#define C_SLASH		'\\'
/* Dla zachowania kompatybilno�ci z Linux */
/* Potoki - dla uruchamiania skrypt�w CGI */
	#define READ_BINARY	"rb"
	#define popen		_popen
	#define pclose		_pclose
/* Zmienne systemowe */
	#define putenv		_putenv
/* Operacje na plikach i folderach */
	#define chdir		_chdir
	#define mkdir(a,b)	mkdir(a)
	#define S_IREAD		_S_IREAD
/* W�a�ciwo�ci socket�w */
	#define fcntl		ioctlsocket
	#define close		closesocket
	#define SHUT_RDWR	SD_BOTH
	#define F_SETFL		FIONBIO
	#define S_IXOTH		S_IEXEC

	#define APP_NAME	"batteryServer (Win32)"
#else
	#define WSAGetLastError() -1
/* �cie�ki do plik�w/folder�w na systemach LINUX s� oddzielone "/" */
	#define SLASH		"/"
	#define C_SLASH		'/'
/* Dla zachowania kompatybilno�ci z WinSock (!)*/
	#define SOCKET_ERROR (-1)
	#define EXIT_FAILURE (1)
	#define READ_BINARY	"re"

	#define APP_NAME	"batteryServer (Linux)"
#endif

#endif
