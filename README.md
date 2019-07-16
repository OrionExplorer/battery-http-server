## battery-http-server [![Build Status](https://travis-ci.org/OrionExplorer/battery-http-server.png?branch=master)](https://travis-ci.org/OrionExplorer/battery-http-server)
###### Copyright (C) 2012 - 2019
###### Marcin Kelar (marcin.kelar@gmail.com)

Fast and portable HTTP server.

### Supported HTTP versions
* HTTP/1.0
* HTTP/1.1

### Supported HTTP methods
* GET
* HEAD

### Supported HTTP response codes
* 200 OK
* 204 No Content
* 206 Partial Content
* 302 Found
* 304 Not Modified
* 400 Bad Request
* 401 Authorization Required
* 403 Forbidden
* 404 Not Found
* 411 Length Required
* 412 Precondition Failed
* 413 Request Entity Too Large
* 414 Request Uri Too Long
* 416 Request Range Not Satisfiable
* 500 Server Error
* 501 Not Implemented
* 503 Service Unavailable
* 504 Not Implemented

### Other features
* High performance (tested with Apache Benchmark - http://pastebin.com/zBG7nHbL)
  * Make use of [epoll()](http://man7.org/linux/man-pages/man7/epoll.7.html) or [select()](http://man7.org/linux/man-pages/man2/select.2.html)
  * Optional: make use of [sendfile()](http://man7.org/linux/man-pages/man2/sendfile.2.html)
  * Advanced control over opened files
  * Simple internal cache mechanism
* Multiplatform (Windows/Linux)
* Basic Access Authentication
* Simple configuration
* Single-threaded
* Buffered event log

### TODO
- [ ] Higher performance
  - [x] epoll
  - [x] cache 1.0 - based on current requests
  - [ ] cache 2.0 - time-based
- [ ] Rebuild event log
  - [ ] Mark standard access and error entries
  - [ ] Move LOG_BUFFER definition to battery.conf
- [ ] Add Accept-Encoding/Content-Encoding header support (gzip, deflate?)
- [ ] Add HTTPS support
- [ ] New CGI algorithm and effort to restore POST method
- [ ] Extend Basic Access Authentication configuration to work with .htaccess files
- [ ] Rebuild range-based data send
- [ ] Add method PUT
- [ ] Add method DELETE
- [ ] Add HTTP Proxy support
- [ ] Add IPv6 support



Battery HTTP Server configuration file is "build/configuration/battery.conf".
For performance test please visit http://pastebin.com/zBG7nHbL

### Compilation and installation
#### Linux
* make
* sudo make install

#### Windows 
* -lws2_32


Special thanks to ZoczuS, the bug hunter from http://zoczus.blogspot.com!
