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
#include "timex.h"
#include "debug.h"
#include "contend.h"
#include "mem128.h"
#include "menu.h"
#include "tape.h"



z80_byte timex_port_f4;
//Para Timex Video modes
z80_bit timex_video_emulation={0};
z80_byte timex_port_ff=0;





//Direcciones donde estan cada pagina de rom. 1 pagina de 16 kb
z80_byte *timex_rom_mem_table[1];

//Direcciones donde estan cada pagina de ram home. 3 paginas de 16 kb cada una
z80_byte *timex_home_ram_mem_table[3];

//Direcciones donde estan cada pagina de ram ex
z80_byte *timex_ex_ram_mem_table[8];

//Direcciones donde estan cada pagina de ram dock
z80_byte *timex_dock_ram_mem_table[8];

//Direcciones actuales mapeadas, bloques de 8 kb
z80_byte *timex_memory_paged[8];


//Si real, es 512x192 sin escalado, pero no permite efectos de linea
//Si no, es 512x192 escalado a 256x192 y permite efectos (cambios de modo) en el mismo frame
z80_bit timex_mode_512192_real={1};


//Tipos de memoria mapeadas
//0=rom
//1=home
//2=dock
//3=ex
//Constantes definidas en TIMEX_MEMORY_TYPE_ROM, _HOME, _DOCK, _EX
z80_byte timex_type_memory_paged[8];


//Paginas mapeadas en cada zona de RAM. Se solamente usa en menu debug y breakpoints, no para el core de emulacion
z80_byte debug_timex_paginas_memoria_mapeadas[8];


void timex_set_memory_pages(void)
{
	//timex_port_f4;
	//Cada bit determina si se mapea home(0) o dock/ex
	//timex_port_ff
	//Bit 7:    Selects which bank the horizontal MMU should use. 0=DOCK, 1=EX-ROM.

        z80_byte *puntero_rom=timex_rom_mem_table[0];

        timex_memory_paged[0]=puntero_rom;
        timex_memory_paged[1]=&puntero_rom[8192];

        timex_type_memory_paged[0]=TIMEX_MEMORY_TYPE_ROM;
        timex_type_memory_paged[1]=TIMEX_MEMORY_TYPE_ROM;

        debug_timex_paginas_memoria_mapeadas[0]=0;
        debug_timex_paginas_memoria_mapeadas[1]=0;


	//Por defecto suponemos mapeo:
	//16 kb rom, ram 0, ram 1, ram 2

	int pagina,bloque_8k;

	bloque_8k=2;

	for (pagina=0;pagina<3;pagina++) {

        	z80_byte *puntero_ram=timex_home_ram_mem_table[pagina];     

	        timex_memory_paged[bloque_8k]=puntero_ram;
        	timex_type_memory_paged[bloque_8k]=TIMEX_MEMORY_TYPE_HOME;
	        debug_timex_paginas_memoria_mapeadas[bloque_8k]=pagina;
		bloque_8k++;

	        timex_memory_paged[bloque_8k]=&puntero_ram[8192];
        	timex_type_memory_paged[bloque_8k]=TIMEX_MEMORY_TYPE_HOME;
	        debug_timex_paginas_memoria_mapeadas[bloque_8k]=pagina;
		bloque_8k++;

	}


	//Aplicamos segun paginacion TIMEX 280SE
		int bloque_ram;
		z80_byte mascara_puerto_f4=1;

		for (bloque_ram=0;bloque_ram<8;bloque_ram++) {
			//Ver cada bit del puerto f4
			if (timex_port_f4 & mascara_puerto_f4) {
				//Se pagina ex o dock
			        //timex_port_ff
			        //Bit 7:    Selects which bank the horizontal MMU should use. 0=DOCK, 1=EX-ROM.
				z80_byte *puntero_memoria;
				if (timex_port_ff&128) {
					//EX
					puntero_memoria=timex_ex_ram_mem_table[bloque_ram];
					timex_type_memory_paged[bloque_ram]=TIMEX_MEMORY_TYPE_EX;

					//temp
					//printf ("forzamos menu\n");
					//menu_abierto=1;


				}
				else {
					//DOCK
					puntero_memoria=timex_dock_ram_mem_table[bloque_ram];
					timex_type_memory_paged[bloque_ram]=TIMEX_MEMORY_TYPE_DOCK;
				}
				timex_memory_paged[bloque_ram]=puntero_memoria;
				debug_timex_paginas_memoria_mapeadas[bloque_ram]=bloque_ram;
			}

			mascara_puerto_f4=mascara_puerto_f4<<1;
		}

	//printf ("fin timex_set_memory_pages\n");

}


void timex_init_memory_tables(void)
{
	debug_printf (VERBOSE_DEBUG,"Initializing Timex memory pages");

	/*
//Direcciones donde estan cada pagina de rom. 2 paginas de 16 kb
//z80_byte *timex_rom_mem_table[2];

//Direcciones donde estan cada pagina de ram home
//z80_byte *timex_home_ram_mem_table[8];

//Direcciones donde estan cada pagina de ram ex
//z80_byte *timex_ex_ram_mem_table[8];

//Direcciones donde estan cada pagina de ram dock
//z80_byte *timex_dock_ram_mem_table[8];

//Direcciones actuales mapeadas, bloques de 8 kb
	*/

	//memoria_spectrum
	//16 kb rom
	//48 kb home
	//64 kb ex
	//64 kb dock

	z80_byte *puntero;
	puntero=memoria_spectrum;

	timex_rom_mem_table[0]=puntero;
	puntero +=16384;

	int i;
	for (i=0;i<3;i++) {
		timex_home_ram_mem_table[i]=puntero;
		puntero +=16384;
	}

	for (i=0;i<8;i++) {
		timex_ex_ram_mem_table[i]=puntero;
		puntero +=8192;
	}

	for (i=0;i<8;i++) {
		timex_dock_ram_mem_table[i]=puntero;
		puntero +=8192;
	}

	//Parece que los 8 kb de rom que se cargan en ex rom[0] tambien estan presentes en rom[1]
	timex_ex_ram_mem_table[1]=timex_ex_ram_mem_table[0];
	//temp
	//timex_ex_ram_mem_table[2]=timex_ex_ram_mem_table[0];
	//timex_ex_ram_mem_table[3]=timex_ex_ram_mem_table[0];
	//timex_ex_ram_mem_table[4]=timex_ex_ram_mem_table[0];
	//timex_ex_ram_mem_table[5]=timex_ex_ram_mem_table[0];
	//timex_ex_ram_mem_table[6]=timex_ex_ram_mem_table[0];
	//timex_ex_ram_mem_table[7]=timex_ex_ram_mem_table[0];

	//prueba
	//timex_dock_ram_mem_table[0]=timex_ex_ram_mem_table[0];


}


//Inicializar cartucho con 255
void timex_empty_dock_space(void)
{

	debug_printf(VERBOSE_INFO,"Emptying timex dock memory");

	int i;

	z80_byte *puntero;

	puntero=timex_dock_ram_mem_table[0];

	for (i=0;i<65536;i++) {
		*puntero=255;
		puntero++;
	}
}


void timex_insert_dck_cartridge(char *filename)
{

	debug_printf(VERBOSE_INFO,"Inserting timex dock cartridge %s",filename);

        FILE *ptr_cartridge;
        ptr_cartridge=fopen(filename,"rb");

        if (!ptr_cartridge) {
		debug_printf (VERBOSE_ERR,"Unable to open cartridge file %s",filename);
                return;
        }

	//Leer primer byte identificador
	z80_byte dck_id;

        //int leidos=fread(mem,1,65535,ptr_cartridge);
        fread(&dck_id,1,1,ptr_cartridge);

	if (dck_id!=0) {
		debug_printf (VERBOSE_ERR,"DCK with id 0x%02X not supported",dck_id);
		return;
	}

	//flags para cada bloque de 8
	z80_byte block_flags[8];
	fread(block_flags,1,8,ptr_cartridge);

	//Leer cada bloque de 8 si conviene

	int bloque;

	for (bloque=0;bloque<8;bloque++) {
		if (block_flags[bloque]!=0) {
			if (block_flags[bloque]==2) {
				//Leer 8kb
				int segmento=bloque*8192;
				debug_printf (VERBOSE_DEBUG,"Loading 8kb block at Segment %04XH-%04XH",segmento,segmento+8191);
				fread(timex_dock_ram_mem_table[bloque],1,8192,ptr_cartridge);
			}

			else {
				debug_printf (VERBOSE_ERR,"8 KB block with id 0x%02X not supported",block_flags[bloque]);
				return;
			}
		}
	}


        fclose(ptr_cartridge);


        if (noautoload.v==0) {
                debug_printf (VERBOSE_INFO,"Reset cpu due to autoload");
                reset_cpu();
        }


}

/*
With the Timex hi res display, the border color is the same as the paper color in the second CLUT. Bits 3-5 of port FF set the ink, paper, and border values to the following ULAplus palette registers:
BITS INK PAPER BORDER
000 24 31 31 001 25 30 30
010 26 29 29
011 27 28 28
100 28 27 27
101 29 26 26
110 30 25 25
111 31 24 24

*/


//Retorna color tinta (sin brillo) para modo timex 6
int get_timex_ink_mode6_color(void)
{
        z80_byte col6=(timex_port_ff>>3)&7;
        return col6;
}


//Retorna color paper (sin brillo) para modo timex 6
int get_timex_paper_mode6_color(void)
{
        z80_byte col6=7-get_timex_ink_mode6_color();
        return col6;
}


//Retorna color border (con brillo)
int get_timex_border_mode6_color(void)
{

        return (7-get_timex_ink_mode6_color() )+8;
}



int timex_si_modo_512(void)
{
    //Si es modo timex 512x192, llamar a otra funcion
        if (MACHINE_IS_SPECTRUM) {
                z80_byte timex_video_mode=timex_port_ff&7;
                if (timex_video_emulation.v) {
                        if (timex_video_mode==4 || timex_video_mode==6) {
				return 1;
                        }
                }
        }

	return 0;
}



int timex_si_modo_512_y_zoom_par(void)
{
	if (timex_si_modo_512() ) {
		if ( (zoom_x&1)==0) return 1;
	}


	return 0;

}

int timex_si_modo_shadow(void)
{
     z80_byte timex_video_mode=timex_port_ff&7;

        if (timex_video_emulation.v) {
                //Modos de video Timex
                /*
000 - Video data at address 16384 and 8x8 color attributes at address 22528 (like on ordinary Spectrum);

001 - Video data at address 24576 and 8x8 color attributes at address 30720;

010 - Multicolor mode: video data at address 16384 and 8x1 color attributes at address 24576;

110 - Extended resolution: without color attributes, even columns of video data are taken from address 16384, and odd columns of video data are taken fr
om address 24576
                */

                if (timex_video_mode==1) return 1;

        }

        return 0;

}


int timex_si_modo_8x1(void)
{
     z80_byte timex_video_mode=timex_port_ff&7;

        if (timex_video_emulation.v) {
                //Modos de video Timex
                /*
000 - Video data at address 16384 and 8x8 color attributes at address 22528 (like on ordinary Spectrum);

001 - Video data at address 24576 and 8x8 color attributes at address 30720;

010 - Multicolor mode: video data at address 16384 and 8x1 color attributes at address 24576;

110 - Extended resolution: without color attributes, even columns of video data are taken from address 16384, and odd columns of video data are taken fr
om address 24576
                */

                if (timex_video_mode==2) return 1;

        }

        return 0;

}
