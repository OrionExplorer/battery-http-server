CC=gcc 
CFLAGS=-Wall
CLIBS=-lssl -lcrypto
INSTALL_DIR=/opt/battery

all: battery

battery: battery.o session.o mem_manager.o files_io.o mime_types.o base64.o htaccess_manager.o socket_io.o http_protocol.o string_utils.o core.o log.o time_utils.o ssl.o
	@ mkdir build/configuration -p
	@ cp source/configuration/battery.conf build/configuration/battery.conf
	@ cp certificate.pem build/configuration/certificate.pem
	@ cp private.key build/configuration/private.key
	gcc battery.o session.o mem_manager.o files_io.o mime_types.o base64.o htaccess_manager.o socket_io.o http_protocol.o string_utils.o core.o log.o time_utils.o ssl.o -o build/battery $(CLIBS)
	@ rm *.o
	@ rm build/logs -rf

battery.o: source/battery.c
	gcc -c source/battery.c

session.o: source/session.c
	gcc -c source/session.c

mem_manager.o: source/mem_manager.c
	gcc -c source/mem_manager.c

files_io.o: source/files_io.c
	gcc -c source/files_io.c

mime_types.o: source/mime_types.c
	gcc -c source/mime_types.c

base64.o: source/base64.c
	gcc -c source/base64.c

htaccess_manager.o: source/htaccess_manager.c
	gcc -c source/htaccess_manager.c

socket_io.o: source/socket_io.c
	gcc -c source/socket_io.c

http_protocol.o: source/http_protocol.c
	gcc -c source/http_protocol.c

string_utils.o: source/string_utils.c
	gcc -c source/string_utils.c

core.o: source/core.c
	gcc -c source/core.c

log.o: source/log.c
	gcc -c source/log.c

time_utils.o: source/time_utils.c
	gcc -c source/time_utils.c

ssl.o: source/ssl.c
	gcc -c source/ssl.c

clean:
	rm *.o

install:
	@ rm -rf $(INSTALL_DIR)/battery
	@ mkdir $(INSTALL_DIR) -p
	@ mkdir $(INSTALL_DIR)/configuration -p
	@ cp build/battery $(INSTALL_DIR)/battery
	@ cp source/scripts/run.sh $(INSTALL_DIR)/run.sh
	@ cp source/configuration/battery.conf $(INSTALL_DIR)/configuration/battery.conf
	@ chmod +x $(INSTALL_DIR)/run.sh
	@ chown pi:pi $(INSTALL_DIR) -R || echo "Unable to chown!"

