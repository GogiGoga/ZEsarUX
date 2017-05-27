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

#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "chrome.h"
#include "mem128.h"
#include "debug.h"
#include "contend.h"



//Direcciones donde estan cada pagina de rom. 4 paginas de 16 kb
z80_byte *chrome_rom_mem_table[4];

//Direcciones donde estan cada pagina de ram home, en paginas de 16 kb
z80_byte *chrome_ram_mem_table[10];


//Direcciones actuales mapeadas, bloques de 16 kb
z80_byte *chrome_memory_paged[4];


void chrome_set_memory_pages(void)
{
}


void chrome_init_memory_tables(void)
{
	debug_printf (VERBOSE_DEBUG,"Initializing Chrome memory pages");

	//memoria_spectrum
	//64 kb rom
	//160 kb ram

	z80_byte *puntero;
	puntero=memoria_spectrum;

	int i;
	for (i=0;i<4;i++) {
		chrome_rom_mem_table[i]=puntero;
		puntero +=16384;
	}

	for (i=0;i<10;i++) {
		chrome_ram_mem_table[i]=puntero;
		puntero +=16384;
	}

	//Tablas contend
	contend_pages_actual[0]=0;
	contend_pages_actual[1]=contend_pages_chrome[5];
	contend_pages_actual[2]=contend_pages_chrome[2];
	contend_pages_actual[3]=contend_pages_chrome[0];


}
