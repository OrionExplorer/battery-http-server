==================
battery-http-server
- fast and portable HTTP server.

Marcin Kelar (marcin.kelar@gmail.com)
==================

Source is written in ANSI C, so you can simply compile and run it under Windows/Linux/Mac OS X.
I'm sure it's not perfect, not even finish, but it works pretty well right now.

• CGI 1.0/1.1 and PHP support

Featured HTTP versions:
• HTTP/1.0
• HTTP/1.1

Featured HTTP methods:
• POST
• GET
• HEAD

Featured HTTP response codes:
• 200 OK
• 204 No Content
• 206 Partial Content
• 302 Found
• 304 Not Modified
• 400 Bad Request
• 401 Authorization Required
• 403 Forbidden
• 404 Not Found
• 411 Length Required
• 412 Predondition Failed
• 413 Request Entity Too Large
• 414 Request Uri Too Long
• 416 Request Range Not Satisfiable
• 500 Server Error
• 501 Not Implemented
• 503 Service Unavailable
• 504 Not Supported

Additional informations:
• Single-threaded
• Multiplatform
• Event log
• Simple configuration based on text files
• High performance (tested with Apache Benchmark)

TODO:
• Rebuild CGI/PHP algorithm
• Rebuild range-based data send
• Rebuild HTTP authorization mechanism
• Add HTTP Proxy support
• Add method PUT
• Add method DELETE
• Add IPv6 support
• Add HTTPS support
• Higher performance!


What you have to do to run this program:
1. (REQUIRED) In file Release/configuration/network.conf you must configure 3 lines:
	0: IP protocol version (4 or 6) - this is under construction [number]
	1: port [number]
	2: your "var/www" catalog [string]

2. (OPTIONAL) In file Release/configuration/scripts.conf you can configure your scripts (PHP and/or CGI), eg. ".php D:\php-cgi.exe extra_param" where:
	- ".php" is your script file extension given by the HTTP request
	- "D:\php-cgi.exe" is path to your external script runner
	- "extra_param" is extra parameter for your external script runner
If your script is already stand-alone CGI 1.0/1.1 application and/or there's no need to pass any extra_param to run it, then just give them "<exec>" value, eg ".cgi D:\my-script.cgi <exec> <exec>".

3. (OPTIONAL) HTTP authorization is under construction and should not be used for production environment, but in file Release/configuration/ht_access.conf you can authorize access to any content, eg:
	"D:\battery-server\index.php my_login my_password"
where "my_login" and "my_password" must be passed by user before access to the requested resource.

4. Compiling on Win32: link WinSock2 library (-lws2_32).

5. Compiling on Linux: do not link WinSock2 library.


FOR COPYRIGHT INFO PLEASE READ COPYING.txt