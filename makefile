CC=gcc
CFLAGS=-Wall -O2 -pedantic
CLIBS=#-lws2_32
INSTALL_DIR=/opt/battery

# ifeq ($(OS),Windows_NT)
# CLIBS += -lws2_32
# endif

all: battery

battery: battery.o session.o mem_manager.o files_io.o mime_types.o base64.o htaccess_manager.o socket_io.o http_protocol.o string_utils.o core.o log.o time_utils.o cache.o
	@ rm build -rf
	@ mkdir build/configuration -p
	@ cp source/configuration/battery.conf build/configuration/battery.conf
	$(CC) $(CFLAGS) battery.o session.o mem_manager.o files_io.o mime_types.o base64.o htaccess_manager.o socket_io.o http_protocol.o string_utils.o core.o log.o time_utils.o cache.o -o build/battery $(CLIBS)
	@ rm *.o

battery.o: source/battery.c
	$(CC) $(CFLAGS) -c source/battery.c

session.o: source/session.c
	$(CC) $(CFLAGS) -c source/session.c

mem_manager.o: source/mem_manager.c
	$(CC) $(CFLAGS) -c source/mem_manager.c

files_io.o: source/files_io.c
	$(CC) $(CFLAGS) -c source/files_io.c

mime_types.o: source/mime_types.c
	$(CC) $(CFLAGS) -c source/mime_types.c

base64.o: source/base64.c
	$(CC) $(CFLAGS) -c source/base64.c

htaccess_manager.o: source/htaccess_manager.c
	$(CC) $(CFLAGS) -c source/htaccess_manager.c

socket_io.o: source/socket_io.c
	$(CC) $(CFLAGS) -c source/socket_io.c

http_protocol.o: source/http_protocol.c
	$(CC) $(CFLAGS) -c source/http_protocol.c

string_utils.o: source/string_utils.c
	$(CC) $(CFLAGS) -c source/string_utils.c

core.o: source/core.c
	$(CC) $(CFLAGS) -c source/core.c

log.o: source/log.c
	$(CC) $(CFLAGS) -c source/log.c

time_utils.o: source/time_utils.c
	$(CC) $(CFLAGS) -c source/time_utils.c

cache.o: source/cache.c
	$(CC) $(CFLAGS) -c source/cache.c

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
