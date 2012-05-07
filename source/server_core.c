/*******************************************************************

Projekt battery_Server

Plik: server_core.c

Przeznaczenie:
Konfiguracja aplikacji
Ustawienie nas�uchiwania socket�w

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#include "include/server_core.h"
#include "include/server_shared.h"
#include "include/server_files_io.h"
#include "include/server_socket_io.h"
#include "include/server_base64.h"
#include "include/server_log.h"
#include "include/server_mime_types.h"
#include "include/server_cgi_manager.h"
#include "include/server_htaccess_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


/* �cie�ka startowa aplikacji */
char	app_path[];

/* Katalog roboczy - udost�pnione klientom zasoby */
char	*working_dir;

/*Pe�na nazwa pliku (+�cie�ka dost�pu) "log.txt" */
char	log_filename[MAX_PATH_LENGTH];

/*Przechowuje informacj� o typach adres�w IP: IPv4 lub IPv6 */
int		ip_proto_ver = -1;

/* Przechowuje informacj� o ilo�ci wczytanych rozszerze� dla innych typ�w plik�w */
int		cgi_other_count = 0;

/* Przechowuje informacj� o ilo�ci wczytanych nazw plik�w index */
int		index_file_count = 0;

static void		server_log_prepare(void);
static void		server_validate_paths(void);

/*
server_log_prepare()
- sprawdza, czy istnieje folder, w kt�rym przechowywany b�dzie log z dzia�ania aplikacji
- tworzy plik "log.txt" w katalogu, kt�rego nazwa jest aktualn� dat� */
static void server_log_prepare(void)
{
	char *tmp_path = (char*)malloc(MAX_PATH_LENGTH_CHAR+1);

	/*Utworzenie �cie�ki do pliku "log.txt" */
	strncpy(tmp_path, app_path, MAX_PATH_LENGTH);
	strncat(tmp_path, LOGS_PATH, MAX_PATH_LENGTH);

	/*Weryfikacja istnienia �cie�ki do pliku "log.txt" */
	if(directory_exists(tmp_path) == 0)
	{
		print_log("Creating path: %s...\n", tmp_path);
		if(mkdir(tmp_path, 0777) != 0) /*Utworzenie �cie�ki */
			print_log("\t- Error creating path!\n");
		if(chdir(app_path) != 0)
		{
			print_log("Error: chdir().\n");
			return;
		}
	}

	print_log("\t- verified: %s.\n", tmp_path);

	/*Dodanie do utworzonej �cie�ki nazwy pliku "log.txt" */
	strncpy(log_filename, tmp_path, MAX_PATH_LENGTH);
	strncat(log_filename, "log.txt", MAX_PATH_LENGTH);

	print_log("NEW SERVER SESSION.\n");
	print_log("log_filename: %s.\n", log_filename);

	free(tmp_path);
	tmp_path = NULL;
}

/*
server_validate_paths()
- sprawdza, czy istniej� wszystie foldery niezb�dne do poprawnego dzia�ania aplikacji */
static void server_validate_paths(void)
{
	char *tmp_path = (char*)malloc(MAX_PATH_LENGTH_CHAR+1);
	int res = -1;

	print_log("Starting server_validate_paths()...\n");

	/*Przypisanie �cie�ki, z kt�rej uruchamiana jest aplikacja */
	strncpy(tmp_path, app_path, MAX_PATH_LENGTH);

	/*Dopisanie �cie�ki do pliku "log.txt", bez nazwy pliku */
	strncat(tmp_path, LOGS_PATH, MAX_PATH_LENGTH);
	if(directory_exists(tmp_path) == 0)
	{
		print_log("Creating path: %s...\n", tmp_path);
		if(mkdir(tmp_path, 0777) != 0) /*Utworzenie �cie�ki */
			print_log("\t- Error (%d) creating path!\n", res);
		if(chdir(app_path) != 0)
		{
			print_log("Error: chdir().\n");
			return;
		}
	}
	print_log("\t- verified: %s\n", tmp_path);

	/*Patrz opis funkcji server_log_prepare() */
	server_log_prepare();

	print_log("server_validate_paths() done.\n");

	free(tmp_path);
	tmp_path = NULL;
}

int server_load_index_configuration(const char* filename)
{
	FILE *cfg_file;
	char *buf;
	char *index_filename;
	int len = 0;

	/* Alokacja pami�ci */
	buf = (char*)malloc(STD_BUFF_SIZE_CHAR);
	mem_allocated(buf, 1060);

	index_filename = (char*)malloc(STD_BUFF_SIZE);
	mem_allocated(buf, 1061);

	print_log("Loading index list...");

	cfg_file = fopen(filename, "rt");
	if(!cfg_file)
	{
		print_log("error.\n");
		printf("Error loading index file list.\n");
		return 0;
	}
	else print_log("ok.\n");

	index_file_count = 0;

	while(fgets(buf, STD_BUFF_SIZE, cfg_file))
	{
		if(sscanf(buf, "%s", index_filename) == 1)
		{
			/* Wczytano maksymaln� ilo�� plik�w */
			if(index_file_count == MICRO_BUFF_SIZE)
			{
				print_log("Reached maximum list index count.\n");
				break;
			}

			/* Pobranie d�ugo�ci wczytanej nazwy pliku */
			len = strlen(buf);

			/* Stworzenie nowego obiektu */
			index_file_list[index_file_count] = (char*)calloc(len, sizeof(char));
			mem_allocated(index_file_list[index_file_count], 1062);

			strncpy(index_file_list[index_file_count], index_filename, len);
			print_log("\t- %d new index file: %s.\n", index_file_count, index_file_list[index_file_count]);
			index_file_count++;
		}
	}

	/* Zwolnienie pami�ci */
	free(buf);
	buf = NULL;

	free(index_filename);
	index_filename = NULL;

	return 1;
}

/*
server_load_configuration()
- je�li istnieje, wczytuje plik "server.cfg" i z niego pobiera konfiguracj� dla zmiennych:
+ ip_proto_ver
+ active_port */
int server_load_configuration(void)
{
	FILE *cfg_file;
	char *network_configuration_filename;		/* Nazwa pliku konfiguracji sieci */
	char *script_configuration_filename;		/* Nazwa pliku konfiguracji skrypt�w CGI */
	char *mime_types_configuration_filename;	/* Nazwa pliku konfiguracji typ�w MIME */
	char *ht_access_configuration_filename;		/* Nazwa pliku konfiguracji dost�p�w */
	char *index_list_configuration_filename;	/* Nazwa pliku konfiguracji listy plik�w index */
	char *buf;									/* Wczytana linia z pliku konfiguracyjnego */
	char *value;								/* Wczytana warto�� z buf */
	int option = 0;								/* Wczytana warto�� z buf */
	int lines_count = 1;

	/* Alokacja pami�ci */
	buf = (char*)malloc(STD_BUFF_SIZE_CHAR);
	mem_allocated(buf, 350);

	network_configuration_filename = (char*)malloc(MAX_PATH_LENGTH_CHAR);
	mem_allocated(network_configuration_filename, 351);

	script_configuration_filename = (char*)malloc(MAX_PATH_LENGTH_CHAR);
	mem_allocated(script_configuration_filename, 352);

	mime_types_configuration_filename = (char*)malloc(MAX_PATH_LENGTH_CHAR);
	mem_allocated(mime_types_configuration_filename, 353);

	ht_access_configuration_filename = (char*)malloc(MAX_PATH_LENGTH_CHAR);
	mem_allocated(ht_access_configuration_filename, 354);

	index_list_configuration_filename = (char*)malloc(MAX_PATH_LENGTH_CHAR);
	mem_allocated(index_list_configuration_filename, 355);

	value = (char*)malloc(STD_BUFF_SIZE_CHAR);
	mem_allocated(value, 356);

	/* Reset zmiennych */
	ip_proto_ver = -1;
	active_port = -1;

	/*Przypisanie �cie�ki, z kt�rej uruchamiana jest aplikacja */
	strncpy(network_configuration_filename, app_path, MAX_PATH_LENGTH);
	strncpy(script_configuration_filename, app_path, MAX_PATH_LENGTH);
	strncpy(mime_types_configuration_filename, app_path, MAX_PATH_LENGTH);
	strncpy(ht_access_configuration_filename, app_path, MAX_PATH_LENGTH);
	strncpy(index_list_configuration_filename, app_path, MAX_PATH_LENGTH);
	/*Dopisanie nazw plik�w z konfiguracj� */
	strncat(network_configuration_filename, NETWORK_CFG_PATH, MAX_PATH_LENGTH);
	strncat(script_configuration_filename, SCRIPTS_CFG_PATH, MAX_PATH_LENGTH);
	strncat(mime_types_configuration_filename, MIME_TYPES_CFG_PATH, MAX_PATH_LENGTH);
	strncat(ht_access_configuration_filename, HT_ACCESS_CFG_PATH, MAX_PATH_LENGTH);
	strncat(index_list_configuration_filename, INDEX_FILE_CFG_PATH, MAX_PATH_LENGTH);

	print_log("Loading configuration file (%s)...", network_configuration_filename);
	cfg_file = fopen(network_configuration_filename, "rt");

	if(cfg_file)
	{
		print_log("\n\t- file opened successfully...\n");
		while(fgets(buf, STD_BUFF_SIZE, cfg_file) != NULL)
		{
			option = -1;
			if(sscanf(buf, "%d %s", &option, value) == 2)
			{
				if(option >= 0 && option <= 65535)
				{
					switch(option)
					{
					case 0: {	/*Wczytanie informacji o typie adresu IP */
						switch(atoi(value))
						{
						case IPv4:	ip_proto_ver = IPv4; break;
						case IPv6:	ip_proto_ver = IPv6; break;
						default: ip_proto_ver = IPv4; break;
						}
						print_log("\t- variable loaded: ip_proto_ver = %d.\n", ip_proto_ver);
						break;
							}

					case 1: {	/* Wczytanie informacji o numerze portu */
						active_port = atoi(value);
						print_log("\t- variable loaded: active_port = %d.\n", active_port);
						break;
							}

					case 2: {	/* Wczytanie informacji o udost�pnionym zasobie */
						/* Alokacja pami�ci */
						working_dir = (char*)malloc(MAX_PATH_LENGTH);
						mem_allocated(working_dir, 999);

						strncpy(working_dir, value, MAX_PATH_LENGTH);
						if(directory_exists(working_dir) == 0) /* Podany zas�b nie istnieje */
						{
							print_log("\t- Error: working dir is invalid.\n");
							printf("Error: working dir is invalid: \"%s\"\n", working_dir);
							exit(EXIT_FAILURE);
						}
						else print_log("\t- working dir: %s\n", working_dir);
						break;
							}
					}
				}
			}
			lines_count++;
		}

		/* Zamkni�cie pliku konfiguracji */
		fclose(cfg_file);

		/* Wczytanie konfiguracji skrypt�w CGI */
		if(!cgi_load_script_configuration(script_configuration_filename)) {
			log_save();
			return 0;
		}
		/* Wczytanie typ�w MIME */
		if(!mime_types_load_configuration(mime_types_configuration_filename)) {
			log_save();
			return 0;
		}
		/* Wczytanie praw dost�p�w do zasob�w */
		if(!ht_access_load_configuration(ht_access_configuration_filename)) {
			log_save();
			return 0;
		}
		/* Wczytanie listy plik�w index */
		if(!server_load_index_configuration(index_list_configuration_filename)) {
			log_save();
			return 0;
		}
		print_log("Configuration file loaded successfully.\n");

		/* Zwolnienie pami�ci */
		free(buf);
		buf = NULL;

		free(network_configuration_filename);
		network_configuration_filename = NULL;

		free(value);
		value = NULL;

		return 1;
	}

	print_log("error or file not found.\n", network_configuration_filename);
	print_log("Loading configuration file (%s)...error or file not found.\n", network_configuration_filename);

	free(buf);
	buf = NULL;

	free(network_configuration_filename);
	network_configuration_filename = NULL;

	free(value);
	value = NULL;

	return 0;
}

/*
server_initialize()
- zast�puje funkcj� main()
- inicjuje zmienne globalne:
+ app_path
- uruchamia procedury konfiguracyjne:
+ server_load_configuration()
- uruchamia obs�ug� danych:
+ server_run() */
void server_initialize()
{
	/* Pobranie �cie�ki startowej aplikacji */
	strncpy(app_path, get_app_path(), MAX_PATH_LENGTH);

	/*Patrz opis funkcji server_validate_paths() */
	(void)server_validate_paths();

	print_log("Application start path:\n%s\n", app_path);

	/*Patrz opisy poszczeg�lnych funkcji */
	if(server_load_configuration() == 0)
	{
		print_log("Error: unable to load configuration.\n");
		printf("Error: unable to load configuration.\n");
		log_save();
		exit(EXIT_FAILURE);
	}

	/* Zapisanie informacji o serwerze do log.txt */
	log_save();

	atexit(socket_free);

	/* Prze��czenie si� na folder, z kt�rego b�d� udost�pniane zasoby */
	strncpy(app_path, working_dir, MAX_PATH_LENGTH);

	/* Uruchomienie sieci */
	server_start_socket();
}
