/*******************************************************************

Projekt battery_Server

Plik: server_mime_types.c

Przeznaczenie:
Wczytanie typ�w mime obs�ugiwanych przez serwer

Autor: Marcin Kelar (marcin.kelar@holicon.pl)
*******************************************************************/
#include "include/server_shared.h"
#include "include/server_log.h"
#include "include/server_mime_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Przechowuje informacj� o ilo�ci wczytanych typ�w mime */
int			mime_types_count;

/*
mime_types_load_configuration(const char *filename)
@filename - nazwa pliku konfiguracyjnego 
- zwraca int, gdzie 1 = uda�o si� wczyta� konfiguracj�, a 0 = nie uda�o si�. */
int mime_types_load_configuration(const char *filename)
{
	FILE *cfg_file;
	char *buf;				/* Wczytana linia z pliku */
	char *ext;				/* Wczytane rozszerzenie pliku z buf */
	char *mime_type;		/* Wczytany typ MIME z pliku */
	int lines_count = 1;

	print_log("Loading MIME-TYPES...");
	cfg_file = fopen(filename, "rt");
	
	if(!cfg_file)
	{
		print_log("error.\n");
		return 0;
	}
	else print_log("ok.\n");

	/* Alokacja pami�ci */
	buf = (char*)malloc(STD_BUFF_SIZE_CHAR);
	mem_allocated(buf, 250);

	ext = (char*)malloc(EXT_LEN_CHAR);
	mem_allocated(ext, 251);

	mime_type = (char*)malloc(SMALL_BUFF_SIZE_CHAR);
	mem_allocated(mime_type, 252);
	
	/* Ilo�� wczytanych typ�w MIME */
	mime_types_count = 0;

	while(fgets(buf, STD_BUFF_SIZE, cfg_file))
	{
		if((sscanf(buf, "%s %s", ext, mime_type) == 2) && (mime_types_count < STD_BUFF_SIZE))
		{
			/* Wczytanie rozszerzenia pliku */
			strncpy(mime_types[mime_types_count].ext, ext, EXT_LEN);
			/* Wczytanie pliku MIME */
			strncpy(mime_types[mime_types_count].mime_type, mime_type, SMALL_BUFF_SIZE);
			print_log("\t- %d \"%s\" assigned to \"%s\".\n", mime_types_count, mime_types[mime_types_count].ext, mime_types[mime_types_count].mime_type);
			mime_types_count++;
		}
		else print_log("\t- unknown command at line %d.\n", lines_count);

		lines_count++;
	}

	/* Zamkni�cie pliku konfiguracyjnego */
	fclose(cfg_file);

	/* Zwolnienie pami�ci */
	free(buf);
	buf = NULL;
	
	free(ext);
	ext = NULL;
	
	free(mime_type);
	mime_type = NULL;

	return 1;
}
