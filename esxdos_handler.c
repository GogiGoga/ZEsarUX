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

void esxdos_handler_no_error_uncarry(void)
{
	Z80_FLAGS=(Z80_FLAGS & (255-FLAG_C));
}

void esxdos_handler_error_carry(void)
{
	Z80_FLAGS |=FLAG_C;
}

void esxdos_handler_return_call(void)
{
	reg_pc=pop_valor();
}

//Temporales para leer 1 solo archivo
FILE *temp_esxdos_last_open_file_handler_unix=NULL;
z80_byte temp_esxdos_last_open_file_handler;

void esxdos_handler_call_f_open(void)
{

	char nombre_archivo[PATH_MAX];
	char nombre_archivo_final[PATH_MAX];
	temp_debug_rst8_copy_hl_to_string(nombre_archivo);

	//Nombre de archivo final sera root_dir+cwd+nombre_archivo
	sprintf (nombre_archivo_final,"%s/%s/%s",esxdos_handler_root_dir,esxdos_handler_cwd,nombre_archivo);

	//Abrir el archivo
	temp_esxdos_last_open_file_handler_unix=fopen(nombre_archivo_final,"rb");
	if (temp_esxdos_last_open_file_handler_unix==NULL) {
		esxdos_handler_error_carry();
	}
	else {
		temp_esxdos_last_open_file_handler=1;

		reg_a=temp_esxdos_last_open_file_handler;
		esxdos_handler_no_error_uncarry();
	}

	esxdos_handler_return_call();

	/*
;                                                                       // Open file. A=drive. HL=Pointer to null-
;                                                                       // terminated string containg path and/or
;                                                                       // filename. B=file access mode. DE=Pointer
;                                                                       // to BASIC header data/buffer to be filled
;                                                                       // with 8 byte PLUS3DOS BASIC header. If you
;                                                                       // open a headerless file, the BASIC type is
;                                                                       // $ff. Only used when specified in B.
;                                                                       // On return without error, A=file handle.
}
*/
}

void esxdos_handler_call_f_read(void)
{
	if (temp_esxdos_last_open_file_handler_unix==NULL) {
		esxdos_handler_error_carry();
	}
	else {
		/*
		f_read                  equ fsys_base + 5;      // $9d  sbc a,l
;                                                                       // Read BC bytes at HL from file handle A.
;                                                                       // On return BC=number of bytes successfully
;                                                                       // read. File pointer is updated.
*/
		z80_int total_leidos=0;
		z80_int bytes_a_leer=reg_bc;
		int leidos=1;

		while (bytes_a_leer && leidos) {
			z80_byte byte_read;
			leidos=fread(&byte_read,1,1,temp_esxdos_last_open_file_handler_unix);
			if (leidos) {
					poke_byte_no_time(reg_hl+total_leidos,byte_read);
					total_leidos++;
					bytes_a_leer--;
			}
		}

		reg_bc=total_leidos;
		esxdos_handler_no_error_uncarry();

	}

	esxdos_handler_return_call();
}

void esxdos_handler_call_f_close(void)
{

	if (temp_esxdos_last_open_file_handler_unix==NULL) {
		esxdos_handler_error_carry();
	}
	else {
		fclose(temp_esxdos_last_open_file_handler_unix);
		temp_esxdos_last_open_file_handler_unix=NULL;
		esxdos_handler_no_error_uncarry();
	}

	esxdos_handler_return_call();
}


void esxdos_handler_call_f_chdir(void)
{

	char ruta[PATH_MAX];
	temp_debug_rst8_copy_hl_to_string(ruta);

	//TODO . comprobar si ruta final (root+cwd) valido


	strcpy(esxdos_handler_cwd,ruta);

	esxdos_handler_no_error_uncarry();
	esxdos_handler_return_call();
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
			esxdos_handler_call_f_open();

		break;

		case ESXDOS_RST8_F_CLOSE:
			printf ("ESXDOS_RST8_F_CLOSE\n");
			esxdos_handler_call_f_close();

		break;

		case ESXDOS_RST8_F_READ:
		//Read BC bytes at HL from file handle A.
			printf ("ESXDOS_RST8_F_READ. Read %d bytes at %04XH from file handle %d\n",reg_bc,reg_hl,reg_a);
			esxdos_handler_call_f_read();
		break;

		case ESXDOS_RST8_F_GETCWD:
			printf ("ESXDOS_RST8_F_GETCWD\n");
			rst(8);
		break;

		case ESXDOS_RST8_F_CHDIR:
			temp_debug_rst8_copy_hl_to_string(buffer_fichero);
			printf ("ESXDOS_RST8_F_CHDIR: %s\n",buffer_fichero);
			esxdos_handler_call_f_chdir();
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

void esxdos_handler_enable(void)
{
	//root dir se pone directorio actual si esta vacio
if (esxdos_handler_root_dir[0]==0) getcwd(esxdos_handler_root_dir,PATH_MAX);

	esxdos_handler_enabled.v=1;
}



void esxdos_handler_disable(void)
{
	esxdos_handler_enabled.v=0;
}
