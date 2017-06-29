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

  //Bit 4 de 32765 es bit 0 de #21AF
  if (puerto_h==0x21) {
    puerto_32765 &=(255-16); //Reset bit 4
    //Si bit 0 de 21af, poner bit 4
    if (value&1) puerto_32765|=16;
  }

  //Si cambia registro #21AF (memconfig) o #10af (page0)
  if (puerto_h==0x21 || puerto_h==0x10) tsconf_set_memory_pages();
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

int temp_tsconf_in_system_rom_flag=1;


int tsconf_in_system_rom(void)
{
  if (temp_tsconf_in_system_rom_flag) return 1;
  return 0;
  //TODO. No entiendo bien cuando entra aqui: 00 - after reset, only in "no map" mode, System ROM, suponemos que solo al encender la maquina,
            //cosa que no es cierta
}

z80_byte tsconf_get_rom_bank(void)
{
/*
Mapeo ROM
The following ports/signals are in play:
- #7FFD bit4,
- DOS signal,
- #21AF (Memconfig),
- #10AF (Page0).

Memconfig:
- bit0 is an alias of bit4 #7FFD,
- bit2 selects mapping mode (0 - map, 1 - no map),
- bit3 selects what is in #0000..#3FFF (0 - ROM, 1 - RAM).

In "no map" the page in the win0 (#0000..#3FFF) is set in Page0 directly. 8 bits are used if RAM, 5 lower bits - if ROM (ROM has 32 pages i.e. 512kB).
In "map" mode Page0 selects ROM page bits 4..2 (which are set to 0 at reset) and bits 1..0 are taken from other sources, selecting one of four ROM pages, see table.

bit1/0
00 - after reset, only in "no map" mode, System ROM,
01 - when DOS signal active, TR-DOS,
10 - when no DOS and #7FFD.4 = 0 (or Memconfig.0 = 0), Basic 128,
11 - when no DOS and #7FFD.4 = 1 (or Memconfig.0 = 1), Basic 48.
*/



        z80_byte memconfig=tsconf_af_ports[0x21];
        if (memconfig & 4) {
          //Modo no map
          cpu_panic("No map mode not emulated yet");
          //solo para que no se queje el compilador
          return 0;
        }
        else {
          z80_byte banco;
          //Modo map
          z80_byte page0=tsconf_af_ports[0x10];
          page0 = page0 << 2;  //In "map" mode Page0 selects ROM page bits 4..2

          //TODO. No entiendo bien cuando entra aqui: 00 - after reset, only in "no map" mode, System ROM, suponemos que solo al encender la maquina,
          //cosa que no es cierta
          if (tsconf_in_system_rom() ) banco=0;


          else {
            if (tsconf_dos_signal.v) banco=1;
            else banco=((puerto_32765>>4)&1) | 2;
          }

          return page0 | banco;

        }




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
