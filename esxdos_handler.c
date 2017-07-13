/*
    ZEsarUX  ZX Second-Emulator And Released for UniX
    Copyright (C) 2013 Cesar Hernandez Bano

    This file is part of ZEsarUX.

    ZEsarUX is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "cpu.h"
#include "esxdos_handler.h"
#include "operaciones.h"
#include "debug.h"

#if defined(__APPLE__)
	#include <sys/syslimits.h>
#endif


//Al leer directorio se usa esxdos_handler_root_dir y esxdos_handler_cwd
char esxdos_handler_root_dir[PATH_MAX]="";
char esxdos_handler_cwd[PATH_MAX]="";

z80_bit esxdos_handler_enabled={0};

void temp_debug_rst8_copy_hl_to_string(char *buffer_fichero)
{

	int i;

	for (i=0;peek_byte_no_time(reg_hl+i);i++) {
		buffer_fichero[i]=peek_byte_no_time(reg_hl+i);
	}

	buffer_fichero[i]=0;
}

void debug_rst8_esxdos(void)
{
	z80_byte funcion=peek_byte_no_time(reg_pc);

	char buffer_fichero[256];

	switch (funcion)
	{

		case ESXDOS_RST8_DISK_READ:
			printf ("ESXDOS_RST8_DISK_READ\n");
			rst(8);
		break;

		case ESXDOS_RST8_M_GETSETDRV:
			printf ("ESXDOS_RST8_M_GETSETDRV\n");
			rst(8);
	  break;

		case ESXDOS_RST8_F_OPEN:

			temp_debug_rst8_copy_hl_to_string(buffer_fichero);
			printf ("ESXDOS_RST8_F_OPEN. File: %s\n",buffer_fichero);
			rst(8);

		break;

		case ESXDOS_RST8_F_CLOSE:
			printf ("ESXDOS_RST8_F_CLOSE\n");
			rst(8);
		break;

		case ESXDOS_RST8_F_READ:
		//Read BC bytes at HL from file handle A.
			printf ("ESXDOS_RST8_F_READ. Read %d bytes at %04XH from file handle %d\n",reg_bc,reg_hl,reg_a);
			rst(8);
		break;

		case ESXDOS_RST8_F_GETCWD:
			printf ("ESXDOS_RST8_F_GETCWD\n");
			rst(8);
		break;

		case ESXDOS_RST8_F_CHDIR:
			temp_debug_rst8_copy_hl_to_string(buffer_fichero);
			printf ("ESXDOS_RST8_F_CHDIR: %s\n",buffer_fichero);
			rst(8);
		break;

		case ESXDOS_RST8_F_OPENDIR:
			printf ("ESXDOS_RST8_F_OPENDIR\n");
			rst(8);
		break;

		case ESXDOS_RST8_F_READDIR:
			printf ("ESXDOS_RST8_F_READDIR\n");
			rst(8);
		break;



		default:
			if (funcion>=0x80) {
				printf ("Unknown ESXDOS_RST8: %02XH !! \n",funcion);
			}
			rst(8);
		break;
	}
}


void esxdos_handler_run(void)
{
	debug_rst8_esxdos();

}
