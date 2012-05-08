==================
battery-server 0.7
- simple, fast and portable HTTP server.

Marcin Kelar (marcin.kelar@gmail.com)
==================

Source is written in ANSI C, so you can simply compile and run it on Windows/Linux/Mac OS X.
I'm sure it's not perfect, not even finish, but it works pretty well right now.


What you have to do to run this program:
1. In file Release/configuration/network.conf you must configure 3 lines:
	0: IP protocol version (4 or 6) - this is under construction
	1: port
	2: your "var/www" catalog

2. In file Release/configuration/scripts.conf you can configure your scripts (PHP and/or CGI), eg. ".php D:\php-cgi.exe <exec>" where:
	- ".ext" is your script file extension given by the HTTP request
	- "D:\php-cgi.exe" is path to your external script runner*
	- "<exec>" is extra parameter for your external script runner
* if your script is already stand-alone CGI 1.0/1.1 then just give it "<exec>"

3. HTTP authorization is under construction and should not be used for production development, but in file Release/configuration/ht_access.conf you can authorize access to any content, eg:
	"D:\battery-server\index.php my_login my_password"

4. Compiling on Win32: link WinSock2 library (-lws2_32).

5. Compiling on Linux: do not link WinSock2 library.


FOR COPYRIGHT INFO PLEASE READ COPYING.txt