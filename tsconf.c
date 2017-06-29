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
#include "tsconf.h"
#include "mem128.h"
#include "debug.h"
#include "contend.h"


z80_byte tsconf_last_port_eff7;
z80_byte tsconf_last_port_dff7;
z80_byte tsconf_nvram[256];

z80_byte tsconf_af_ports[256];


//Direcciones donde estan cada pagina de rom. 32 paginas de 16 kb
z80_byte *tsconf_rom_mem_table[32];

//Direcciones donde estan cada pagina de ram, en paginas de 16 kb
z80_byte *tsconf_ram_mem_table[256];


//Direcciones actuales mapeadas, bloques de 16 kb
z80_byte *tsconf_memory_paged[4];

//En teoria esto se activa cuando hay traps tr-dos
z80_bit tsconf_dos_signal={0};


void tsconf_write_af_port(z80_byte puerto_h,z80_byte value)
{
  tsconf_af_ports[puerto_h]=value;
}

z80_byte tsconf_get_af_port(z80_byte index)
{
  return tsconf_af_ports[index];
}



void tsconf_init_memory_tables(void)
{
	debug_printf (VERBOSE_DEBUG,"Initializing TSConf memory pages");

	z80_byte *puntero;
	puntero=memoria_spectrum;

	int i;
	for (i=0;i<32;i++) {
		tsconf_rom_mem_table[i]=puntero;
		puntero +=16384;
	}

	for (i=0;i<256;i++) {
		tsconf_ram_mem_table[i]=puntero;
		puntero +=16384;
	}

	//Tablas contend
	contend_pages_actual[0]=0;
	contend_pages_actual[1]=contend_pages_tsconf[5];
	contend_pages_actual[2]=contend_pages_tsconf[2];
	contend_pages_actual[3]=contend_pages_tsconf[0];


}




z80_byte tsconf_get_rom_bank(void)
{


        z80_byte banco;

        banco=((puerto_32765>>4)&1);

	      return banco;
}

z80_byte tsconf_get_ram_bank_c0(void)
{
        z80_byte banco;

      	banco=puerto_32765&7;

        return banco;
}




void tsconf_set_memory_pages(void)
{
	z80_byte rom_page=tsconf_get_rom_bank();
	z80_byte ram_page_c0=tsconf_get_ram_bank_c0();


	/*
	Port 1FFDh (read/write)
	Bit 0 If 1 maps banks 8 or 9 at 0000h (switch off rom).
	Bit 1 High bit of ROM selection and bank 8 (0) or 9 (1) if bit0 = 1.
	*/

	tsconf_memory_paged[0]=tsconf_rom_mem_table[rom_page];



	tsconf_memory_paged[1]=tsconf_ram_mem_table[5];
	tsconf_memory_paged[2]=tsconf_ram_mem_table[2];
	tsconf_memory_paged[3]=tsconf_ram_mem_table[ram_page_c0];

  debug_paginas_memoria_mapeadas[0]=128+rom_page;
	debug_paginas_memoria_mapeadas[1]=5;
	debug_paginas_memoria_mapeadas[2]=2;
	debug_paginas_memoria_mapeadas[3]=ram_page_c0;

}
