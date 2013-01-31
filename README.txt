==================
battery-http-server
- fast and portable HTTP server.

Marcin Kelar (marcin.kelar@gmail.com)
==================

Source is written in ANSI C, so you can simply compile and run it under Windows/Linux/Mac OS X.
I'm sure it's not perfect, not even finish, but it works pretty well right now.

For performance test please visit http://pastebin.com/zBG7nHbL

Featured HTTP versions:
• HTTP/1.0
• HTTP/1.1

Featured HTTP methods:
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

Additional information:
• Multiplatform
• Event log
• Simple configuration based on text files
• High performance (tested with Apache Benchmark - http://pastebin.com/zBG7nHbL)

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

2. (OPTIONAL) HTTP authorization is under construction and should not be used for production environment, but in file Release/configuration/ht_access.conf you can authorize access to any content, eg:
	"D:\battery-server\index.php my_login my_password"
where "my_login" and "my_password" must be passed by user before access to the requested resource.

3. Compilation requirements.
	• Win32: -lws2_32 -lpthread
	• Linux: -lpthread

FOR COPYRIGHT INFO PLEASE READ COPYING.txt
