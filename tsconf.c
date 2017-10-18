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
#include "menu.h"


z80_byte tsconf_last_port_eff7;
z80_byte tsconf_last_port_dff7;
z80_byte tsconf_nvram[256];

z80_byte tsconf_af_ports[256];


z80_byte tsconf_fmaps[TSCONF_FMAPS_SIZE];
/*
Offset  A[11:8] RAM	  Description
#000    000x  CRAM    Color Palette RAM, 256 cells 16 bit wide, 16 bit access
#200    001x  SFILE   Sprite Descriptors, 85 cells 48 bit wide, 16 bit access
#400    0100  REGS    TS-Conf Registers, 8 bit access, adressing is the same as by #nnAF port
*/

//Retorna valor entre 0...32767, segun color de entrada entre 0..255
z80_int tsconf_return_cram_color(z80_byte color)
{
  int offset=color*2;
  z80_byte color_l=tsconf_fmaps[offset];
  z80_byte color_h=tsconf_fmaps[offset+1];

  z80_int color_retorno=(color_h<<8)|color_l;

  return color_retorno;
}


z80_byte tsconf_return_cram_palette_offset(void)
{
 return (tsconf_af_ports[7]&0xF)*16;
}

//Direcciones donde estan cada pagina de rom. 32 paginas de 16 kb
z80_byte *tsconf_rom_mem_table[32];

//Direcciones donde estan cada pagina de ram, en paginas de 16 kb
z80_byte *tsconf_ram_mem_table[256];


//Direcciones actuales mapeadas, bloques de 16 kb
z80_byte *tsconf_memory_paged[4];

//En teoria esto se activa cuando hay traps tr-dos
z80_bit tsconf_dos_signal={0};


//Almacena tamanyo pantalla y border para modo actual
//Los inicializamos con algo por si acaso
//Border indica la mitad del total, o sea, si dice 40, es margen izquierdo 40 y margen derecho 40
int tsconf_current_pixel_width=256;
int tsconf_current_pixel_height=192;
int tsconf_current_border_width=0;
int tsconf_current_border_height=0;

char *tsconf_video_modes_array[]={
  "ZX",
  "16c",
  "256c",
  "Text"
};

char *tsconf_video_sizes_array[]={
  "256x192",
  "320x200",
  "320x240",
  "360x288"
};

z80_byte tsconf_get_video_mode_display(void)
{
  /*
  Modos de video:
0 ZX

1 16c.
Bits are index to CRAM, where PalSel.GPAL is 4 MSBs and the pixel are 4 LSBs of the index.
Pixels are bits7..4 - left, bits3..0 - right.
Each line address is aligned to 256.
GXOffs and GYOffs add offset to X and Y start address in pixels.

2  256c.
Bits 7..0 are index to CRAM.
Each line address is aligned to 512.
GXOffs and GYOffs add offset to X and Y start address in pixels.

3 text

  */
  return (tsconf_af_ports[0]&3);
}

z80_byte tsconf_get_video_size_display(void)
{
  return ((tsconf_af_ports[0]>>6)&3);
}

//Actualiza valores de variables de tamanyo pantalla segun modo de video actual
//Modo de video ZX Spectrum solo tiene sentido con resolucion 256x192
void tsconf_set_sizes_display(void)
{
  z80_byte videosize=tsconf_get_video_size_display();

/*
  00 - 256x192: border size: 104x96
01 - 320x200: border size: 40x88
10 - 320x240: border size: 40x48
11 - 360x288: border size: 0x0
*/

  switch (videosize) {
    case 0:
      tsconf_current_pixel_width=256;
      tsconf_current_pixel_height=192;
    break;

    case 1:
      tsconf_current_pixel_width=320;
      tsconf_current_pixel_height=200;
    break;

    case 2:
      tsconf_current_pixel_width=320;
      tsconf_current_pixel_height=240;
    break;

    case 3:
      tsconf_current_pixel_width=360;
      tsconf_current_pixel_height=288;
    break;

  }

  tsconf_current_border_width=(360-tsconf_current_pixel_width)/2;
  tsconf_current_border_height=(288-tsconf_current_pixel_height)/2;


  //printf ("Current video size. Pixel size: %dX%d Border size: %dX%d\n",
  //    tsconf_current_pixel_width,tsconf_current_pixel_height,tsconf_current_border_width*2,tsconf_current_border_height*2);

}



void tsconf_splash_video_size_mode_change(void)
{
  char buffer_mensaje[64];

  sprintf (buffer_mensaje,"Setting video mode %s, size %s",
    tsconf_video_modes_array[tsconf_get_video_mode_display()],tsconf_video_sizes_array[tsconf_get_video_size_display()]);

  screen_print_splash_text(10,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,buffer_mensaje);
}


void tsconf_set_emulator_setting_turbo(void)
{
        /*
        Register 32
        bits 1-0:
        00: 3.5 mhz
        01: 7
        10: 14
        11: reserved

                                */
        z80_byte t=tsconf_af_ports[32] & 3;
        if (t==0) cpu_turbo_speed=1;
        else if (t==1) cpu_turbo_speed=2;
        else cpu_turbo_speed=4;


        //printf ("Setting turbo: %d\n",cpu_turbo_speed);

        cpu_set_turbo_speed();
}

void tsconf_write_af_port(z80_byte puerto_h,z80_byte value)
{

  tsconf_af_ports[puerto_h]=value;

  if (puerto_h==0) {
    tsconf_splash_video_size_mode_change();


    //Cambio vconfig
    tsconf_set_sizes_display();
    modificado_border.v=1;
  }

  //temp debug vpage
  if (puerto_h==1) {
    //printf ("---VPAGE: %02XH\n",puerto_h);
  }

  if (puerto_h==7) {
    //printf ("Palette: %02XH\n",value);
  }

  //Bit 4 de 32765 es bit 0 de #21AF
  if (puerto_h==0x21) {
    puerto_32765 &=(255-16); //Reset bit 4
    //Si bit 0 de 21af, poner bit 4
    if (value&1) puerto_32765|=16;
  }

  //Port 0x7FFD is an alias of Page3, page3=tsconf_af_ports[0x13];
  if (puerto_h==0x13) {
    puerto_32765=value;
  }

  //Gestion fmaps
  /*
  Vamos a empezar por la carga de la paleta. Para ello, debe habilitar la memoria RAM paleta de mapeo,
  incluyendo el puerto FMAddr cuarto bit. En este caso, los bits 0-3 especifica la asignación de dirección
   (por ejemplo, 0 - # 0000 8-8000 # 15 - # F000, etc.). Después de encender el mapeo de las direcciones seleccionadas
   estarán disponibles para la ventana para cargar paleta. A continuación, utilizando LDIR convencional nos movemos paleta de 512 bytes.
   Después de enviar los datos, debe cerrar la ventana de asignación, dejando caer el puerto FMAddr cuarto bit. Considere este ejemplo:

   FMAddr     EQU #15AF
LOAD_PAL   LD  A,%00010000   ; Включаем маппинг по адресу #0000
           LD  BC,FMAddr
           OUT (C),A
           LD  HL,ZXPAL      ; Перебрасываем данные (32 байта)
           LD  DE,#0000
           LD  BC,#0020
           LDIR
           XOR  A            ; Отключаем маппинг
           LD  BC,FMAddr
           OUT (C),A
           RET

ZXPAL      dw  #0000,#0010,#4000,#4010,#0200,#0210,#4200,#4210
           dw  #0000,#0018,#6000,#6018,#0300,#0318,#6300,#6318
  */
  if (puerto_h==0x15) {
    //printf ("Registro fmaps 0x15 valor: %02XH\n",value);
  }

  if (puerto_h==32) {
    tsconf_set_emulator_setting_turbo();
  }

  //Si cambia registro #21AF (memconfig) o page0-3
  if (puerto_h==0x21 || puerto_h==0x10 || puerto_h==0x11 || puerto_h==0x12 || puerto_h==0x13 ) tsconf_set_memory_pages();
}

z80_byte tsconf_get_af_port(z80_byte index)
{
  return tsconf_af_ports[index];
}

void tsconf_reset_cpu(void)
{
  tsconf_af_ports[0]=0;
}

void tsconf_init_memory_tables(void)
{
	debug_printf (VERBOSE_DEBUG,"Initializing TSConf memory pages");

	z80_byte *puntero;
	puntero=memoria_spectrum;

	int i;
	for (i=0;i<TSCONF_ROM_PAGES;i++) {
		tsconf_rom_mem_table[i]=puntero;
		puntero +=16384;
	}

	for (i=0;i<TSCONF_RAM_PAGES;i++) {
		tsconf_ram_mem_table[i]=puntero;
		puntero +=16384;
	}

	//Tablas contend
	contend_pages_actual[0]=0;
	contend_pages_actual[1]=contend_pages_tsconf[5];
	contend_pages_actual[2]=contend_pages_tsconf[2];
	contend_pages_actual[3]=contend_pages_tsconf[0];


}



/*

Memory paging:

af_registers[16] page at 0000 at reset: 0
af_registers[17] page at 4000 at reset: 5
af_registers[18] page at 8000 at reset: 2
af_registers[19] page at c000 at reset: 0

Segmentos 4000 y 8000 siempre determinados por esos 17 y 18


En espacio c000 interviene:
af_registers[19] (page3)
Memconfig=af_register[33]
bits 6,7=lck128
si lck128:

00 512 kb  7ffd[7:6]=page3[4:3]
01 128 kb  7ffd[7:6]=00
10 auto    !a[13] ???????
11 1024kb   #7FFD[7:6] = Page3[4:3], #7FFD[5] = Page3[5]


*/

z80_byte tsconf_get_memconfig(void)
{
  return tsconf_af_ports[0x21];
}

z80_byte tsconf_get_memconfig_lck(void)
{
  return (tsconf_get_memconfig()>>6)&3;
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



        z80_byte memconfig=tsconf_get_memconfig();
        if (memconfig & 4) {
          //Modo no map
          //cpu_panic("No map mode not emulated yet");
          //solo para que no se queje el compilador
          z80_byte banco=tsconf_af_ports[0x10]&31;
          return banco;
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
    //De momento mapear como un 128k
    z80_byte tsconf_lck=tsconf_get_memconfig_lck();

    /*
    00 512 kb  7ffd[7:6]=page3[4:3]
    01 128 kb  7ffd[7:6]=00
    10 auto    !a[13] ???????
    11 1024kb   #7FFD[7:6] = Page3[4:3], #7FFD[5] = Page3[5]


    Port 0x7FFD is an alias of Page3, thus selects page in 0xC000-0xFFFF window. Its behaviour depends on MemConfig LCK128 bits.
    00 - 512k: #7FFD[7:6] -> Page3[4:3], #7FFD[2:0] -> Page3[2:0], Page3[7:5] = 0,
    01 - 128k: #7FFD[2:0] -> Page3[2:0], Page3[7:3] = 0,
    10 - Auto: OUT (#FD) works as mode 01, OUT (C), r works as mode 00,
    11 - 1024k: #7FFD[7:6] -> Page3[4:3], #7FFD[2:0] -> Page3[2:0], #7FFD[5] -> Page3[5], Page3[7:6] = 0.


    Port 0x7FFD is an alias of Page3, page3=tsconf_af_ports[0x13];
    */

    z80_byte banco=0;

    switch (tsconf_lck) {

        case 0:
          banco=(puerto_32765&7)| ((puerto_32765>>3)&24);
        break;

        case 1:
      	 banco=puerto_32765&7;
        break;

        default:
          cpu_panic("TODO invalid value for tsconf_lck");
        break;
    }

    return banco;

}


z80_byte tsconf_get_text_font_page(void)
{
  //En teoria es la misma pagina que registro af vpage(01) xor 1 pero algo falla, nos guiamos por pagina mapeada en segmento c0
  //z80_byte pagina=tsconf_get_ram_bank_c0() ^ 1;

  z80_byte pagina=tsconf_af_ports[1] ^ 1;
  return pagina;
}

z80_byte tsconf_get_vram_page(void)
{
  return tsconf_af_ports[1];
}


void tsconf_set_memory_pages(void)
{
	z80_byte rom_page=tsconf_get_rom_bank();

  //z80_byte ram_page_c0=tsconf_get_ram_bank_c0();
  //temp
  z80_byte ram_page_c0=tsconf_af_ports[19];

  z80_byte ram_page_40=tsconf_af_ports[17];
  z80_byte ram_page_80=tsconf_af_ports[18];


	/*
	Port 1FFDh (read/write)
	Bit 0 If 1 maps banks 8 or 9 at 0000h (switch off rom).
	Bit 1 High bit of ROM selection and bank 8 (0) or 9 (1) if bit0 = 1.
	*/

	//memconfig
	//bit3 selects what is in #0000..#3FFF (0 - ROM, 1 - RAM).

	if (tsconf_get_memconfig()&8) {
    debug_paginas_memoria_mapeadas[0]=rom_page;
    tsconf_memory_paged[0]=tsconf_ram_mem_table[rom_page];
  }

	else {
    debug_paginas_memoria_mapeadas[0]=128+rom_page;
    tsconf_memory_paged[0]=tsconf_rom_mem_table[rom_page];
  }



	tsconf_memory_paged[1]=tsconf_ram_mem_table[ram_page_40];
	tsconf_memory_paged[2]=tsconf_ram_mem_table[ram_page_80];
	tsconf_memory_paged[3]=tsconf_ram_mem_table[ram_page_c0];


	debug_paginas_memoria_mapeadas[1]=ram_page_40;
	debug_paginas_memoria_mapeadas[2]=ram_page_80;
	debug_paginas_memoria_mapeadas[3]=ram_page_c0;

  //printf ("32765: %02XH rom %d ram1 %d ram2 %d ram3 %d\n",puerto_32765,rom_page,ram_page_40,ram_page_80,ram_page_c0);

}


void tsconf_hard_reset(void)
{

  reset_cpu();
  temp_tsconf_in_system_rom_flag=1;
  tsconf_af_ports[0x21]=4;

	int i;
	for (i=0;i<TSCONF_FMAPS_SIZE;i++) tsconf_fmaps[i]=0;

       //Borrar toda memoria ram
        int d;
        z80_byte *puntero;
        
        for (i=0;i<TSCONF_RAM_PAGES;i++) {
                puntero=tsconf_ram_mem_table[i];
                for (d=0;d<16384;d++,puntero++) {
                        *puntero=0;
                }
        }


  //Valores por defecto
  tsconf_af_ports[0]=0;
  tsconf_af_ports[1]=5;
  tsconf_af_ports[2]=0;

  tsconf_af_ports[3] &=(255-1);
  tsconf_af_ports[4]=0;
  tsconf_af_ports[5] &=(255-1);
  tsconf_af_ports[6] &=3;
  tsconf_af_ports[7]=15;

  //Paginas RAM
  tsconf_af_ports[0x10]=0;
  tsconf_af_ports[0x11]=5;
  tsconf_af_ports[0x12]=2;
  tsconf_af_ports[0x13]=0;

  tsconf_af_ports[0x15] &=(255-16);
  tsconf_af_ports[0x20]=1;

  tsconf_af_ports[0x22]=1;
  tsconf_af_ports[0x23]=0;
  tsconf_af_ports[0x24] &=(2+4+8);

  tsconf_af_ports[0x29] &=(255-1-2-4-8);

  tsconf_af_ports[0x2A] &=(255-1-2-4);
  tsconf_af_ports[0x2A] |=1;

  tsconf_af_ports[0x2B] &=(255-1-2-4-8);

  tsconf_set_memory_pages();
  tsconf_set_sizes_display();
  tsconf_set_emulator_setting_turbo();
}
