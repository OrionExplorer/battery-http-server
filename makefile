CC=gcc 
CFLAGS=-Wall

all: battery

battery: batteryServer.o server_create_session.o server_mem_manager.o server_files_io.o server_mime_types.o server_base64.o server_htaccess_manager.o server_socket_io.o server_http_protocol.o server_strings_util.o server_core.o server_log.o server_time_util.o 
	@ mkdir build/configuration -p
	cp source/configuration/battery.conf build/configuration/battery.conf
	gcc batteryServer.o server_create_session.o server_mem_manager.o server_files_io.o server_mime_types.o server_base64.o server_htaccess_manager.o server_socket_io.o server_http_protocol.o server_strings_util.o server_core.o server_log.o server_time_util.o  -o build/battery
	@ rm *.o
	@ rm build/logs -rf

batteryServer.o: source/batteryServer.c
	gcc -c source/batteryServer.c

server_create_session.o: source/server_create_session.c
	gcc -c source/server_create_session.c

server_mem_manager.o: source/server_mem_manager.c
	gcc -c source/server_mem_manager.c

server_files_io.o: source/server_files_io.c
	gcc -c source/server_files_io.c

server_mime_types.o: source/server_mime_types.c
	gcc -c source/server_mime_types.c

server_base64.o: source/server_base64.c
	gcc -c source/server_base64.c

server_htaccess_manager.o: source/server_htaccess_manager.c
	gcc -c source/server_htaccess_manager.c

server_socket_io.o: source/server_socket_io.c
	gcc -c source/server_socket_io.c

server_http_protocol.o: source/server_http_protocol.c
	gcc -c source/server_http_protocol.c

server_strings_util.o: source/server_strings_util.c
	gcc -c source/server_strings_util.c

server_core.o: source/server_core.c
	gcc -c source/server_core.c

server_log.o: source/server_log.c
	gcc -c source/server_log.c

server_time_util.o: source/server_time_util.c
	gcc -c source/server_time_util.c

clean:
	rm *.o
