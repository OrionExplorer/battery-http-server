/*******************************************************************

Projekt battery-http-server

Plik: http_protocol.h

Autor: Marcin Kelar ( marcin.kelar@gmail.com )
*******************************************************************/
#ifndef HTTP_PROTOCOL_H
#define HTTP_PROTOCOL_H

#include "shared.h"

#define MAX_URI_LENGTH                              1024

#define SITE_INDEX                                  "index.html"

#define HTTP_VER                                    "HTTP/1.1"
#define HTTP_VER_1_0                                "HTTP/1.0"

#define HTTP_200_OK                                 "200 OK"
#define HTTP_204_NO_CONTENT                         "204 NO CONTENT"
#define HTTP_206_PARTIAL_CONTENT                    "206 PARTIAL CONTENT"
#define HTTP_302_FOUND                              "302 FOUND"
#define HTTP_304_NOT_MODIFIED                       "304 NOT MODIFIED"
#define HTTP_400_BAD_REQUEST                        "400 BAD REQUEST"
#define HTTP_401_AUTHORIZATION_REQUIRED             "401 AUTHORIZATION REQUIRED"
#define HTTP_403_FORBIDDEN                          "403 FORBIDDEN"
#define HTTP_404_NOT_FOUND                          "404 NOT FOUND"
#define HTTP_411_LENGTH_REQUIRED                    "411 LENGTH REQUIRED"
#define HTTP_412_PRECONDITION_FAILED                "412 PRECONDITION FAILED"
#define HTTP_413_REQUEST_ENTITY_TOO_LARGE           "413 REQUEST ENTITY TOO LARGE"
#define HTTP_414_REQUEST_URI_TOO_LONG               "414 REQUEST URI TOO LONG"
#define HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE    "416 REQUESTED RANGE NOT SATISFIABLE"
#define HTTP_500_SERVER_ERROR                       "500 SERVER ERROR"
#define HTTP_501_NOT_IMPLEMENTED                    "501 NOT IMPLEMENTED"
#define HTTP_503_SERVICE_UNAVAILABLE                "503 SERVICE UNAVAILABLE"
#define HTTP_505_HTTP_VERSION_NOT_SUPPORTED         "505 HTTP VERSION NOT SUPPORTED"

#define HEADER_DATE                         "Date: "
#define HEADER_CONTENT_TYPE                 "Content-Type: "
#define HEADER_CONTENT_LENGTH               "Content-Length: "
#define HEADER_CONNECTION                   "Connection: "
#define HEADER_HOST                         "Host: "
#define HEADER_SERVER                       "Server: "SERVER_NAME
#define HEADER_LAST_MODIFIED                "Last-Modified: "
#define HEADER_IF_MODIFIED_SINCE            "If-Modified-Since: "
#define HEADER_IF_UNMODIFIED_SINCE          "If-Unmodified-Since: "
#define HEADER_LOCATION                     "Location: "
#define HEADER_RANGE                        "Range: "
#define HEADER_CONTENT_RANGE                "Content-Range: "
#define HEADER_ACCEPT_RANGES                "Accept-Ranges: bytes\r\n"
#define HEADER_AUTHORIZATION                "Authorization: "
#define HEADER_AUTHENTICATION               "WWW-Authenticate: Basic realm=\"Secure Area\"\r\n"

#define HEADER_KEEP_ALIVE_STR               "Keep-Alive"
#define KEEP_ALIVE_PREF                     "timeout=9, max=99\r\n"

#define HEADER_STD_CONTENT_TYPE             "text/html"
#define HTTP_ERR_101_MSG                    "<HTML><HEAD><TITLE>"HTTP_101_SWITCHING_PROTOCOL"</TITLE></HEAD><BODY><H1>"HTTP_101_SWITCHING_PROTOCOL"</H1>"HTTP_101_SWITCHING_PROTOCOL"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_204_MSG                    "<HTML><HEAD><TITLE>"HTTP_204_NO_CONTENT"</TITLE></HEAD><BODY><H1>"HTTP_204_NO_CONTENT"</H1>"HTTP_204_NO_CONTENT"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_302_MSG                    "<HTML><HEAD><TITLE>"HTTP_302_FOUND"</TITLE></HEAD><BODY><H1>"HTTP_302_FOUND"</H1>"HTTP_302_FOUND"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_400_MSG                    "<HTML><HEAD><TITLE>"HTTP_400_BAD_REQUEST"</TITLE></HEAD><BODY><H1>"HTTP_400_BAD_REQUEST"</H1>"HTTP_400_BAD_REQUEST"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_401_MSG                    "<HTML><HEAD><TITLE>"HTTP_401_AUTHORIZATION_REQUIRED"</TITLE></HEAD><BODY><H1>"HTTP_401_AUTHORIZATION_REQUIRED"</H1>"HTTP_401_AUTHORIZATION_REQUIRED"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_403_MSG                    "<HTML><HEAD><TITLE>"HTTP_403_FORBIDDEN"</TITLE></HEAD><BODY><H1>"HTTP_403_FORBIDDEN"</H1>"HTTP_403_FORBIDDEN"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_404_MSG                    "<HTML><HEAD><TITLE>"HTTP_404_NOT_FOUND"</TITLE></HEAD><BODY><H1>"HTTP_404_NOT_FOUND"</H1>"HTTP_404_NOT_FOUND"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_411_MSG                    "<HTML><HEAD><TITLE>"HTTP_411_LENGTH_REQUIRED"</TITLE></HEAD><BODY><H1>"HTTP_411_LENGTH_REQUIRED"</H1>"HTTP_411_LENGTH_REQUIRED"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_413_MSG                    "<HTML><HEAD><TITLE>"HTTP_413_REQUEST_ENTITY_TOO_LARGE"</TITLE></HEAD><BODY><H1>"HTTP_413_REQUEST_ENTITY_TOO_LARGE"</H1>"HTTP_413_REQUEST_ENTITY_TOO_LARGE"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_414_MSG                    "<HTML><HEAD><TITLE>"HTTP_414_REQUEST_URI_TOO_LONG"</TITLE></HEAD><BODY><H1>"HTTP_414_REQUEST_URI_TOO_LONG"</H1>"HTTP_414_REQUEST_URI_TOO_LONG"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_416_MSG                    "<HTML><HEAD><TITLE>"HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE"</TITLE></HEAD><BODY><H1>"HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE"</H1>"HTTP_416_REQUESTED_RANGE_NOT_SATISFIABLE"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_500_MSG                    "<HTML><HEAD><TITLE>"HTTP_500_SERVER_ERROR"</TITLE></HEAD><BODY><H1>"HTTP_500_SERVER_ERROR"</H1>"HTTP_500_SERVER_ERROR"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_501_MSG                    "<HTML><HEAD><TITLE>"HTTP_501_NOT_IMPLEMENTED"</TITLE></HEAD><BODY><H1>"HTTP_501_NOT_IMPLEMENTED"</H1>"HTTP_501_NOT_IMPLEMENTED"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_503_MSG                    "<HTML><HEAD><TITLE>"HTTP_503_SERVICE_UNAVAILABLE"</TITLE></HEAD><BODY><H1>"HTTP_503_SERVICE_UNAVAILABLE"</H1>"HTTP_503_SERVICE_UNAVAILABLE"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"
#define HTTP_ERR_505_MSG                    "<HTML><HEAD><TITLE>"HTTP_505_HTTP_VERSION_NOT_SUPPORTED"</TITLE></HEAD><BODY><H1>"HTTP_505_HTTP_VERSION_NOT_SUPPORTED"</H1>"HTTP_505_HTTP_VERSION_NOT_SUPPORTED"<HR><ADDRES><I>"APP_NAME"/"APP_VER"</I></ADDRES></BODY></HTML>"

void            REQUEST_process( HTTP_SESSION *http_session );

long            REQUEST_get_range( HTTP_SESSION *http_session, int type );
char*           REQUEST_get_mime_type( const char *filename );
char*           REQUEST_get_message_body( HTTP_SESSION *http_session );
char*           REQUEST_get_message_header( const char *content_data, long content_length );
char*           REQUEST_get_cgi_name( HTTP_SESSION *http_session );
char*           REQUEST_get_query( HTTP_SESSION *http_session );
char*           REQUEST_get_header_value( const char *header, const char *requested_value_name );

void            RESPONSE_header( HTTP_SESSION *http_session, const char *http_status_code, const char *http_mime_type, size_t http_content_length, const char *content_data, const char* add_headers );
void            RESPONSE_file( HTTP_SESSION *http_session, const char *filename );
void            RESPONSE_error( HTTP_SESSION *http_session, const char *http_status_code, const char *http_error_message, const char* add_headers );

char*           REQUEST_get_index( const char *path );

#endif
