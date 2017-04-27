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
#include "tbblue.h"
#include "mem128.h"
#include "debug.h"
#include "contend.h"
#include "utils.h"
#include "menu.h"
#include "divmmc.h"
#include "diviface.h"
#include "ulaplus.h"
#include "screen.h"

//Punteros a los 8 bloques de 16kb de ram de spectrum
z80_byte *tbblue_ram_memory_pages[8];

//Punteros a los 4 bloques de 16kb de rom de spectrum
z80_byte *tbblue_rom_memory_pages[4];

z80_byte *tbblue_fpga_rom;

//Memoria mapedada
z80_byte *tbblue_memory_paged[4];



//'bootrom' takes '1' on hard-reset and takes '0' if there is any writing on the i/o port 'config1'. It can not be read.
z80_bit tbblue_bootrom={1};

//Puerto tbblue de maquina/mapeo
/*
- Write in port 0x24DB (config1)

                case cpu_do(7 downto 6) is
                    when "01"    => maquina <= s_speccy48;
                    when "10"    => maquina <= s_speccy128;
                    when "11"    => maquina <= s_speccy3e;
                    when others    => maquina <= s_config;   ---config mode
                end case;
                romram_page <= cpu_do(4 downto 0);                -- rom or ram page
*/

z80_byte tbblue_config1=0;

/*
Para habilitar las interfaces y las opciones que utilizo el puerto 0x24DD (config2). Para iniciar la ejecución de la máquina elegida escribo la máquina de la puerta 0x24DB y luego escribir 0x01 en puerta 0x24D9 (hardsoftreset), que activa el "SoftReset" y hace PC = 0.

config2:

Si el bit 7 es '1', los demás bits se refiere a:

6 => Frecuencia vertical (50 o 60 Hz);
5-4 => PSG modo;
3 => "ULAplus" habilitado o no;
2 => "DivMMC" habilitado o no;
1 => "Scanlines" habilitadas o no;
0 => "Scandoubler" habilitado o no;

Si el bit 7 es "0":

6 => "Lightpen" activado;
5 => "Multiface" activado;
4 => "PS/2" teclado o el ratón;
3-2 => el modo de joystick1;
1-0 => el modo de joystick2;

Puerta 0x24D9:

bit 1 => realizar un "hard reset" ( "maquina"=0,  y "bootrom" recibe '1' )
bit 0 => realizar un "soft reset" ( PC = 0x0000 )
*/

z80_byte tbblue_config2=0;
z80_byte tbblue_hardsoftreset=0;


//Si segmento bajo (0-16383) es escribible
z80_bit tbblue_low_segment_writable={0};


//Indice a lectura del puerto
z80_byte tbblue_read_port_24d5_index=0;

//port 0x24DF bit 2 = 1 turbo mode (7Mhz)
z80_byte tbblue_port_24df;



void tbblue_init_memory_tables(void)
{
/*
0x000000 - 0x03FFFF (256K) => DivMMC RAM
0x040000 - 0x05FFFF (128K) => Speecy RAM
0x060000 - 0x063FFF (16K) => DivMMC ROM (ESXDOS)
0x064000 - 0x067FFF (16K) => Multiface ROM
0x068000 - 0x06BFFF (16K) => Multiface ROM
0x06C000 - 0x06FFFF (16K) => Multiface RAM
0x070000 to 0x07FFFF (64K) => Speccy ROM
*/


	int i,indice;

	//Los 8 KB de la fpga ROM estan al final
	tbblue_fpga_rom=&memoria_spectrum[512*1024];

	for (i=0;i<8;i++) {
		indice=0x040000+16384*i;
		tbblue_ram_memory_pages[i]=&memoria_spectrum[indice];
	}

	for (i=0;i<4;i++) {
		indice=0x070000+16384*i;
		tbblue_rom_memory_pages[i]=&memoria_spectrum[indice];
	}

}

void tbblue_mem_page_ram_rom(void)
{
	z80_byte page_type;
	
	page_type=(puerto_8189 >>1) & 3;
	
	switch (page_type) {
		case 0:
			debug_printf (VERBOSE_DEBUG,"Pages 0,1,2,3");
			tbblue_memory_paged[0]=tbblue_ram_memory_pages[0];
			tbblue_memory_paged[1]=tbblue_ram_memory_pages[1];
			tbblue_memory_paged[2]=tbblue_ram_memory_pages[2];
			tbblue_memory_paged[3]=tbblue_ram_memory_pages[3];
			
			contend_pages_actual[0]=contend_pages_128k_p2a[0];
			contend_pages_actual[1]=contend_pages_128k_p2a[1];
			contend_pages_actual[2]=contend_pages_128k_p2a[2];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];
			
			debug_paginas_memoria_mapeadas[0]=0;
			debug_paginas_memoria_mapeadas[1]=1;
			debug_paginas_memoria_mapeadas[2]=2;
			debug_paginas_memoria_mapeadas[3]=3;
			
			break;
			
		case 1:
			debug_printf (VERBOSE_DEBUG,"Pages 4,5,6,7");
			tbblue_memory_paged[0]=tbblue_ram_memory_pages[4];
			tbblue_memory_paged[1]=tbblue_ram_memory_pages[5];
			tbblue_memory_paged[2]=tbblue_ram_memory_pages[6];
			tbblue_memory_paged[3]=tbblue_ram_memory_pages[7];
			
			
			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[5];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[7];
			
			debug_paginas_memoria_mapeadas[0]=4;
			debug_paginas_memoria_mapeadas[1]=5;
			debug_paginas_memoria_mapeadas[2]=6;
			debug_paginas_memoria_mapeadas[3]=7;
			
			
			
			break;
			
		case 2:
			debug_printf (VERBOSE_DEBUG,"Pages 4,5,6,3");
			tbblue_memory_paged[0]=tbblue_ram_memory_pages[4];
			tbblue_memory_paged[1]=tbblue_ram_memory_pages[5];
			tbblue_memory_paged[2]=tbblue_ram_memory_pages[6];
			tbblue_memory_paged[3]=tbblue_ram_memory_pages[3];
			
			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[5];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];
			
			debug_paginas_memoria_mapeadas[0]=4;
			debug_paginas_memoria_mapeadas[1]=5;
			debug_paginas_memoria_mapeadas[2]=6;
			debug_paginas_memoria_mapeadas[3]=3;
			
			
			break;
			
		case 3:
			debug_printf (VERBOSE_DEBUG,"Pages 4,7,6,3");
			tbblue_memory_paged[0]=tbblue_ram_memory_pages[4];
			tbblue_memory_paged[1]=tbblue_ram_memory_pages[7];
			tbblue_memory_paged[2]=tbblue_ram_memory_pages[6];
			tbblue_memory_paged[3]=tbblue_ram_memory_pages[3];
			
			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[7];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];
			
			debug_paginas_memoria_mapeadas[0]=4;
			debug_paginas_memoria_mapeadas[1]=7;
			debug_paginas_memoria_mapeadas[2]=6;
			debug_paginas_memoria_mapeadas[3]=3;
			
			
			break;
			
	}
}



void tbblue_set_memory_pages(void)
{
	//Mapeamos paginas de RAM segun config1
	z80_byte maquina=(tbblue_config1>>6)&3;

	int romram_page;
	int ram_page,rom_page;
	int indice;

	//printf ("tbblue set memory pages. maquina=%d\n",maquina);

	switch (maquina) {
		case 1:
                    //when "01"    => maquina <= s_speccy48;
			tbblue_memory_paged[0]=tbblue_rom_memory_pages[0];
			tbblue_memory_paged[1]=tbblue_ram_memory_pages[5];
			tbblue_memory_paged[2]=tbblue_ram_memory_pages[2];
			tbblue_memory_paged[3]=tbblue_ram_memory_pages[0];

			debug_paginas_memoria_mapeadas[0]=0+128;
			debug_paginas_memoria_mapeadas[1]=5;
			debug_paginas_memoria_mapeadas[2]=2;
			debug_paginas_memoria_mapeadas[3]=0;

		        contend_pages_actual[0]=0;
		        contend_pages_actual[1]=contend_pages_128k_p2a[5];
		        contend_pages_actual[2]=contend_pages_128k_p2a[2];
		        contend_pages_actual[3]=contend_pages_128k_p2a[0];


			tbblue_low_segment_writable.v=0;
		break;

		case 2:
                    //when "10"    => maquina <= s_speccy128;
			rom_page=(puerto_32765>>4)&1;
                        tbblue_memory_paged[0]=tbblue_rom_memory_pages[rom_page];

                        tbblue_memory_paged[1]=tbblue_ram_memory_pages[5];
                        tbblue_memory_paged[2]=tbblue_ram_memory_pages[2];

			ram_page=puerto_32765&7;
                        tbblue_memory_paged[3]=tbblue_ram_memory_pages[ram_page];

			debug_paginas_memoria_mapeadas[0]=rom_page+128;
			debug_paginas_memoria_mapeadas[1]=5;
			debug_paginas_memoria_mapeadas[2]=2;
			debug_paginas_memoria_mapeadas[3]=ram_page;

			tbblue_low_segment_writable.v=0;
		        contend_pages_actual[0]=0;
		        contend_pages_actual[1]=contend_pages_128k_p2a[5];
		        contend_pages_actual[2]=contend_pages_128k_p2a[2];
		        contend_pages_actual[3]=contend_pages_128k_p2a[ram_page];


		break;

		case 3:

			//Si RAM en ROM
			if (puerto_8189&1) {
				
				tbblue_mem_page_ram_rom();
				tbblue_low_segment_writable.v=1;
			}

			else {

                    //when "11"    => maquina <= s_speccy3e;
                        rom_page=(puerto_32765>>4)&1;

		        z80_byte rom1f=(puerto_8189>>1)&2;
		        z80_byte rom7f=(puerto_32765>>4)&1;

			z80_byte rom_page=rom1f | rom7f;


                        tbblue_memory_paged[0]=tbblue_rom_memory_pages[rom_page];

                        tbblue_memory_paged[1]=tbblue_ram_memory_pages[5];
                        tbblue_memory_paged[2]=tbblue_ram_memory_pages[2];

                        ram_page=puerto_32765&7;
                        tbblue_memory_paged[3]=tbblue_ram_memory_pages[ram_page];

			debug_paginas_memoria_mapeadas[0]=rom_page+128;
			debug_paginas_memoria_mapeadas[1]=5;
			debug_paginas_memoria_mapeadas[2]=2;
			debug_paginas_memoria_mapeadas[3]=ram_page;

		        contend_pages_actual[0]=0;
		        contend_pages_actual[1]=contend_pages_128k_p2a[5];
		        contend_pages_actual[2]=contend_pages_128k_p2a[2];
		        contend_pages_actual[3]=contend_pages_128k_p2a[ram_page];


			}
		break;

		default:

			//printf ("tbblue_bootrom.v=%d\n",tbblue_bootrom.v);

			if (tbblue_bootrom.v==0) {
				/*
When the variable 'bootrom' takes '0', page 0 (0-16383) is mapped to the RAM 512K, and the page mapping is configured by bits 4-0 of the I/O port 'config1'. These 5 bits maps 32 16K pages the start of 512K SRAM space 0-16363 the Speccy, which allows you access to all SRAM.
*/
				romram_page=(tbblue_config1&31);
				indice=romram_page*16384;
				//printf ("page on 0-16383: %d offset: %X\n",romram_page,indice);
				tbblue_memory_paged[0]=&memoria_spectrum[indice];
				tbblue_low_segment_writable.v=1;
			}
			else {
				//In this setting state, the page 0 repeats the content of the ROM 'loader', ie 0-8191 appear memory contents, and repeats 8192-16383
				//La rom es de 8 kb pero la hemos cargado dos veces
				tbblue_memory_paged[0]=tbblue_fpga_rom;
				tbblue_low_segment_writable.v=0;
			}

			tbblue_memory_paged[1]=tbblue_ram_memory_pages[5];
			tbblue_memory_paged[2]=tbblue_ram_memory_pages[2];
			tbblue_memory_paged[3]=tbblue_ram_memory_pages[7];

			debug_paginas_memoria_mapeadas[0]=0+128;
			debug_paginas_memoria_mapeadas[1]=5;
			debug_paginas_memoria_mapeadas[2]=2;
			debug_paginas_memoria_mapeadas[3]=7;

		        contend_pages_actual[0]=0; //Suponemos que esa pagina no tiene contienda 
		        contend_pages_actual[1]=contend_pages_128k_p2a[5];
		        contend_pages_actual[2]=contend_pages_128k_p2a[2];
		        contend_pages_actual[3]=contend_pages_128k_p2a[7];

		break;
	}

}

void tbblue_set_emulator_setting_divmmc(void)
{
/*
Sincronizar divmmc de momento
Si el bit 7 es '1', los demás bits se refiere a:

6 => Frecuencia vertical (50 o 60 Hz);
5-4 => PSG modo;
3 => "ULAplus" habilitado o no;
2 => "DivMMC" habilitado o no;
1 => "Scanlines" habilitadas o no;
0 => "Scandoubler" habilitado o no;
*/
        z80_byte diven=tbblue_config2&4;
        debug_printf (VERBOSE_INFO,"Apply config2.divmmc change: %s",(diven ? "enabled" : "disabled") );
        //printf ("Apply config2.divmmc change: %s\n",(diven ? "enabled" : "disabled") );
        if (diven) divmmc_diviface_enable();
        else divmmc_diviface_disable();
}

void tbblue_set_emulator_setting_ulaplus(void)
{
/*
Sincronizar divmmc de momento
Si el bit 7 es '1', los demás bits se refiere a:

6 => Frecuencia vertical (50 o 60 Hz);
5-4 => PSG modo;
3 => "ULAplus" habilitado o no;
2 => "DivMMC" habilitado o no;
1 => "Scanlines" habilitadas o no;
0 => "Scandoubler" habilitado o no;
*/
        z80_byte ulaplusen=tbblue_config2&8;
        debug_printf (VERBOSE_INFO,"Apply config2.ulaplus change: %s",(ulaplusen ? "enabled" : "disabled") );
        //printf ("Apply config2.ulaplus change: %s\n",(ulaplusen ? "enabled" : "disabled") );
        if (ulaplusen) enable_ulaplus();
        else disable_ulaplus();
}

//port 0x24DF bit 2 = 1 turbo mode (7Mhz)
void tbblue_set_emulator_setting_turbo(void)
{
	z80_byte t=tbblue_port_24df & 4;
	if (t==0) cpu_turbo_speed=1;
	else cpu_turbo_speed=2;

	//printf ("Setting turbo: %d\n",cpu_turbo_speed);

	cpu_set_turbo_speed();
}

void tbblue_hard_reset(void)
{
	tbblue_config1=0;
	tbblue_config2=0;
	tbblue_port_24df=0;
	tbblue_hardsoftreset=0;

	tbblue_bootrom.v=1;
	tbblue_set_memory_pages();



	tbblue_set_emulator_setting_divmmc();
	tbblue_set_emulator_setting_ulaplus();

}



void tbblue_out_port(z80_int port,z80_byte value)
{
	//printf ("tbblue_out_port port=%04XH value=%02XH PC=%d\n",port,value,reg_pc);
	//debug_printf (VERBOSE_PARANOID,"tbblue_out_port port=%04XH value=%02XH",port,value);

	z80_byte last_value;

	switch (port) {
		case 0x24DB:
			last_value=tbblue_config1;
			tbblue_config1=value;
			tbblue_bootrom.v=0;
			tbblue_set_memory_pages();

/*
- Write in port 0x24DB (config1)

                case cpu_do(7 downto 6) is
                    when "01"    => maquina <= s_speccy48;
                    when "10"    => maquina <= s_speccy128;
                    when "11"    => maquina <= s_speccy3e;
                    when others    => maquina <= s_config;   ---config mode
                end case;

*/
			//Solo cuando hay cambio
                        if ( (last_value&(128+64)) != (value&(128+64)))  tbblue_set_emulator_setting_timing();
		break;

		case 0x24DD:
			last_value=tbblue_config2;
			tbblue_config2=value;

			//Solo cuando hay cambio
			if ( (last_value&4) != (value&4) ) tbblue_set_emulator_setting_divmmc();
			
			//Solo cuando hay cambio
			if ( (last_value&8) != (value&8) ) tbblue_set_emulator_setting_ulaplus();

			
			
		break;

		case 0x24DF:
			//printf ("\n\nEscribir %d en 24df\n\n",value);
			last_value=tbblue_port_24df;
			tbblue_port_24df=value;

			//Solo cuando hay cambio
                        if ( (last_value&4) != (value&4) ) tbblue_set_emulator_setting_turbo();
		break;


		case 0x24D9:
			tbblue_hardsoftreset=value;
			if (value&1) {
				//printf ("Doing soft reset due to writing to port 24D9H\n");
				reg_pc=0;
			}
			if (value&2) {
				//printf ("Doing hard reset due to writing to port 24D9H\n");
				tbblue_bootrom.v=1;
				tbblue_config1=0;
				tbblue_set_memory_pages();
				reg_pc=0;
			}
				
		break;
	}

}

z80_byte tbblue_read_port_24d5(void)
{
/*
- port 0x24D5: 1st read = return hardware number (below), 2nd read = return firmware version number (first nibble.second nibble) e.g. 0x12 = 1.02
 
hardware numbers
1 = DE-1 (old)
2 = DE-2 (old)
3 = DE-2 (new)
4 = DE-1 (new)
5 = FBLabs
6 = VTrucco
7 = WXEDA hardware (placa de desarrollo) 
8 = Emulators
9 = ZX Spectrum Next
*/

//Retornaremos 8.0 

	z80_byte value;


	if (tbblue_read_port_24d5_index==0) value=8;
	else if (tbblue_read_port_24d5_index==1) value=0;

	//Cualquier otra cosa, 255
	else value=255;

	//printf ("puerto 24d5. index=%d value=%d\n",tbblue_read_port_24d5_index,value);

	tbblue_read_port_24d5_index++;
	return value;

}



void tbblue_set_timing_128k(void)
{
        contend_read=contend_read_128k;
        contend_read_no_mreq=contend_read_no_mreq_128k;
        contend_write_no_mreq=contend_write_no_mreq_128k;
        
        ula_contend_port_early=ula_contend_port_early_128k;
        ula_contend_port_late=ula_contend_port_late_128k;
        
        
        screen_testados_linea=228;
        screen_invisible_borde_superior=7;
        screen_invisible_borde_derecho=104;
        
        port_from_ula=port_from_ula_p2a;
        contend_pages_128k_p2a=contend_pages_p2a;
        
}


void tbblue_set_timing_48k(void)
{
        contend_read=contend_read_48k;
        contend_read_no_mreq=contend_read_no_mreq_48k;
        contend_write_no_mreq=contend_write_no_mreq_48k;
        
        ula_contend_port_early=ula_contend_port_early_48k;
        ula_contend_port_late=ula_contend_port_late_48k;
        
        screen_testados_linea=224;
        screen_invisible_borde_superior=8;
        screen_invisible_borde_derecho=96;
        
        port_from_ula=port_from_ula_48k;
        
        //esto no se usara...
        contend_pages_128k_p2a=contend_pages_128k;
        
}

void tbblue_change_timing(int timing)
{
        if (timing==0) tbblue_set_timing_48k();
        else if (timing==1) tbblue_set_timing_128k();
        
        screen_set_video_params_indices();
        inicializa_tabla_contend();
        
}

/*
*/
void tbblue_set_emulator_setting_timing(void)
{
/*
- Write in port 0x24DB (config1)

                case cpu_do(7 downto 6) is
                    when "01"    => maquina <= s_speccy48;
                    when "10"    => maquina <= s_speccy128;
                    when "11"    => maquina <= s_speccy3e;
                    when others    => maquina <= s_config;   ---config mode
                end case;

*/
  
  
                z80_byte t=(tbblue_config1 >> 6)&3;
                if (t<=1) {
					//48k
							debug_printf (VERBOSE_INFO,"Apply config1.timing. change:48k");
							tbblue_change_timing(0);
					}
				else {
					//128k
							debug_printf (VERBOSE_INFO,"Apply config1.timing. change:128k");
							tbblue_change_timing(1);
					}
				

 
        
}



