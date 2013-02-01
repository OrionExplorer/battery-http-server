==================
battery-http-server
- fast and portable HTTP server.

Marcin Kelar (marcin.kelar@gmail.com)
==================

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

Other features:
• High performance (tested with Apache Benchmark - http://pastebin.com/zBG7nHbL)
• Multiplatform (Windows/Linux/MacOS X)
• Basic Access Authentication
• Simple configuration
• Event log

TODO:
• Extend Basic Access Authentication configuration
• Rebuild range-based data send
• New CGI/PHP algorithm and restore POST method
• Add method PUT
• Add method DELETE
• Add HTTP Proxy support
• Add IPv6 support
• Add HTTPS support
• Higher performance!


#To run Battery HTTP Server:
1. File "configuration/network.conf" should contain following lines (without quotes):
	"0 [number]" - IP protocol version (4 or 6). This is under construction.
	"1 [number]" - Port number
	"2 [string]" - Document root
This step is required.

2. File "configuration/ht_access.conf" can contain resource access information in following format (without quotes):
	"resource login password" - Neither resource, nor login, nor password can contain spaces.
This step is optional.

#Compilation requirements:
	• Win32: -lws2_32 -lpthread
	• Linux: -lpthread

FOR COPYRIGHT INFO PLEASE READ COPYING.txt
