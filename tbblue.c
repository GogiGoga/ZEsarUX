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

#include "timex.h"
#include "ula.h"

//Punteros a los 64 bloques de 8kb de ram de spectrum
z80_byte *tbblue_ram_memory_pages[64];

//Punteros a los 8 bloques de 8kb de rom de spectrum
z80_byte *tbblue_rom_memory_pages[8];

z80_byte *tbblue_fpga_rom;

//Memoria mapeada, en 8 bloques de 8 kb
z80_byte *tbblue_memory_paged[8];


//Sprites

//Paleta de 256 colores formato RGB9 RRRGGGBBB
//Valores son de 9 bits por tanto lo definimos con z80_int que es de 16 bits
//z80_int tbsprite_palette[256];


//Diferentes paletas
//Total:
//     000 = ULA first palette
//     100 = ULA secondary palette
//     001 = Layer 2 first palette
//    101 = Layer 2 secondary palette
//     010 = Sprites first palette 
//     110 = Sprites secondary palette
//Paletas de 256 colores formato RGB9 RRRGGGBBB
//Valores son de 9 bits por tanto lo definimos con z80_int que es de 16 bits
z80_int tbblue_palette_ula_first[256];
z80_int tbblue_palette_ula_second[256];
z80_int tbblue_palette_layer2_first[256];
z80_int tbblue_palette_layer2_second[256];
z80_int tbblue_palette_sprite_first[256];
z80_int tbblue_palette_sprite_second[256];


//Diferentes layers a componer la imagen final
/*
(R/W) 0x15 (21) => Sprite and Layers system
  bit 7 - LoRes mode, 128 x 96 x 256 colours (1 = enabled)
  bits 6-5 = Reserved, must be 0
  bits 4-2 = set layers priorities:
     Reset default is 000, sprites over the Layer 2, over the ULA graphics
     000 - S L U
     001 - L S U
     010 - S U L
     011 - L U S
     100 - U S L
     101 - U L S
 */

//Si en zona pantalla y todo es transparente, se pone un 0
//Layers con el indice al olor final en la paleta RGB9 (0..511)
z80_int tbblue_layer_ula[512];
z80_int tbblue_layer_layer2[512];
z80_int tbblue_layer_sprites[512];


//Damos la paleta que se esta leyendo o escribiendo en una operacion de I/O
//Para ello mirar bits 6-4  de reg 0x43
z80_int *tbblue_get_palette_rw(void)
{
/*
(R/W) 0x43 (67) => Palette Control
  bit 7 = Reserved, must be 0
  bits 6-4 = Select palette for reading or writing:
     000 = ULA first palette
     100 = ULA secondary palette
     001 = Layer 2 first palette
     101 = Layer 2 secondary palette
     010 = Sprites first palette 
     110 = Sprites secondary palette
  bit 3 = Select Sprites palette (0 = first palette, 1 = secondary palette)
  bit 2 = Select Layer 2 palette (0 = first palette, 1 = secondary palette)
  bit 1 = Select ULA palette (0 = first palette, 1 = secondary palette)
*/
	z80_byte active_palette=(tbblue_registers[0x43]>>4)&7;

	switch (active_palette) {
		case 0:
			return tbblue_palette_ula_first;
		break;

		case 4:
			return tbblue_palette_ula_second;
		break;

		case 1:
			return tbblue_palette_layer2_first;
		break;

		case 5:
			return tbblue_palette_layer2_second;
		break;

		case 2:
			return tbblue_palette_sprite_first;
		break;

		case 6:
			return tbblue_palette_sprite_second;
		break;

		//por defecto retornar siempre ULA first palette
		default:
			return tbblue_palette_ula_first;
		break;
	}
}

//Damos el valor del color de la paleta que se esta leyendo o escribiendo en una operacion de I/O
z80_int tbblue_get_value_palette_rw(z80_byte index)
{
	z80_int *paleta;

	paleta=tbblue_get_palette_rw();

	return paleta[index];
}


//Modificamos el valor del color de la paleta que se esta leyendo o escribiendo en una operacion de I/O
void tbblue_set_value_palette_rw(z80_byte index,z80_int valor)
{
	z80_int *paleta;

	paleta=tbblue_get_palette_rw();

	paleta[index]=valor;
}

//Damos el valor del color de la paleta que se esta mostrando en pantalla para sprites
//Para ello mirar bit 3 de reg 0x43
z80_int tbblue_get_palette_active_sprite(z80_byte index)
{
/*
(R/W) 0x43 (67) => Palette Control

  bit 3 = Select Sprites palette (0 = first palette, 1 = secondary palette)
  bit 2 = Select Layer 2 palette (0 = first palette, 1 = secondary palette)
  bit 1 = Select ULA palette (0 = first palette, 1 = secondary palette)
*/
	if (tbblue_registers[0x43]&8) return tbblue_palette_sprite_second[index];
	else return tbblue_palette_sprite_first[index];

}

//Damos el valor del color de la paleta que se esta mostrando en pantalla para layer2
//Para ello mirar bit 2 de reg 0x43
z80_int tbblue_get_palette_active_layer2(z80_byte index)
{
/*
(R/W) 0x43 (67) => Palette Control

  bit 3 = Select Sprites palette (0 = first palette, 1 = secondary palette)
  bit 2 = Select Layer 2 palette (0 = first palette, 1 = secondary palette)
  bit 1 = Select ULA palette (0 = first palette, 1 = secondary palette)
*/
	if (tbblue_registers[0x43]&4) return tbblue_palette_layer2_second[index];
	else return tbblue_palette_layer2_first[index];

}

//Damos el valor del color de la paleta que se esta mostrando en pantalla para ula
//Para ello mirar bit 1 de reg 0x43
z80_int tbblue_get_palette_active_ula(z80_byte index)
{
/*
(R/W) 0x43 (67) => Palette Control

  bit 3 = Select Sprites palette (0 = first palette, 1 = secondary palette)
  bit 2 = Select Layer 2 palette (0 = first palette, 1 = secondary palette)
  bit 1 = Select ULA palette (0 = first palette, 1 = secondary palette)
*/
	if (tbblue_registers[0x43]&2) return tbblue_palette_ula_second[index];
	else return tbblue_palette_ula_first[index];

}

//64 patterns de Sprites
/*
In the palette each byte represents the colors in the RRRGGGBB format, and the pink color, defined by standard 1110011, is reserved for the transparent color.
*/
z80_byte tbsprite_patterns[TBBLUE_MAX_PATTERNS][256];
//64 sprites
/*
[0] 1st: X position (bits 7-0).
[1] 2nd: Y position (0-255).
[2] 3rd: bits 7-4 is palette offset, bit 3 is X MSB, bit 2 is X mirror, bit 1 is Y mirror and bit 0 is visible flag.
[3] 4th: bits 7-6 is reserved, bits 5-0 is Name (pattern index, 0-63).
*/
z80_byte tbsprite_sprites[TBBLUE_MAX_SPRITES][4];

//Indices al indicar paleta, pattern, sprites. Subindex indica dentro de cada pattern o sprite a que posicion (0..3 en sprites o 0..255 en pattern ) apunta
z80_byte tbsprite_index_palette;
z80_byte tbsprite_index_pattern,tbsprite_index_pattern_subindex;
z80_byte tbsprite_index_sprite,tbsprite_index_sprite_subindex;

/*
Port 0x303B, if read, returns some information:

Bits 7-2: Reserved, must be 0.
Bit 1: max sprites per line flag.
Bit 0: Collision flag.
Port 0x303B, if written, defines the sprite slot to be configured by ports 0x55 and 0x57, and also initializes the address of the palette.

*/

z80_byte tbblue_port_303b;


/* Informacion relacionada con Layer2. Puede cambiar en el futuro, hay que ir revisando info en web de Next

Registros internos implicados:


(R/W) 18 => Layer 2 RAM page
 bits 7-6 = Reserved, must be 0
 bits 5-0 = SRAM page

(R/W) 19 => Layer 2 RAM shadow page
 bits 7-6 = Reserved, must be 0
 bits 5-0 = SRAM page


(R/W) 20 => Layer 2 transparency color
  bits 7-4 = Reserved, must be 0
  bits 3-0 = ULA transparency color (IGRB)(Reset to "1000" black with bright, after a reset)



(R/W) 22 => Layer2 Offset X
  bits 7-0 = X Offset (0-255)(Reset to 0 after a reset)

(R/W) 23 => Layer2 Offset Y
  bist 7-0 = Y Offset (0-255)(Reset to 0 after a reset)


Posiblemente registro 20 aplica a cuando el layer2 esta por detras de pantalla de spectrum, y dice el color de pantalla de spectrum
que actua como transparente
Cuando layer2 esta encima de pantalla spectrum, el color transparente parece que es el mismo que sprites: TBBLUE_TRANSPARENT_COLOR 0xE3

Formato layer2: 256x192, linear, 8bpp, RRRGGGBB (mismos colores que sprites), ocupa 48kb

Se accede en modo escritura en 0000-3fffh mediante puerto:

Banking in Layer2 is out 4667 ($123B)
bit 0 = write enable, which changes writes from 0-3fff to write to layer2,
bit 1 = Layer2 ON or OFF set=ON,
bit 2 = ????
bit 3 = Use register 19 instead of 18 to tell sram page
bit 4 puts layer 2 behind the normal spectrum screen
bit 6 and 7 are to say which 16K section is paged in,
$03 = 00000011b Layer2 on and writable and top third paged in at $0000,
$43 = 01000011b Layer2 on and writable and middle third paged in at $0000,
$C3 = 11000011b Layer2 on and writable and bottom third paged in at $0000,  ?? sera 100000011b??? TODO
$02 = 00000010b Layer2 on and nothing paged in. etc

Parece que se mapea la pagina de sram indicada en registro 19

*/


/*

IMPORTANT!!

Trying some old layer2 demos that doesn't set register 19 is dangerous.
To avoid problems, first do:
out 9275, 19
out 9531,32
To set layer2 to the extra ram:
0x080000 – 0x0FFFFF (512K) => Extra RAM

Then load the demo program and will work

*/

z80_byte tbblue_port_123b;

int tbblue_write_on_layer2(void)
{
	if (tbblue_port_123b &1) return 1;
	return 0;
}

int tbblue_is_active_layer2(void)
{
	if (tbblue_port_123b & 2) return 1;
	return 0;
}


int tbblue_get_offset_start_layer2(void)
{
	int offset=tbblue_registers[18]&63;

	if (tbblue_port_123b & 8 ) offset=tbblue_registers[19]&63;

	//offset=tbblue_registers[18]&63;
	offset*=16384;

	return offset;
}

void tbblue_reset_sprites(void)
{

	int i;

	

	//Resetear patterns todos a transparente
	for (i=0;i<TBBLUE_MAX_PATTERNS;i++) {
		int j;
		for (j=0;j<256;j++) {
			tbsprite_patterns[i][j]=TBBLUE_DEFAULT_TRANSPARENT;
		}
	}

	//Poner toda info de sprites a 0. Seria quiza suficiente con poner bit de visible a 0
	for (i=0;i<TBBLUE_MAX_SPRITES;i++) {
		tbsprite_sprites[i][0]=0;
		tbsprite_sprites[i][1]=0;
		tbsprite_sprites[i][2]=0;
		tbsprite_sprites[i][3]=0;
	}


	tbsprite_index_palette=tbsprite_index_pattern=tbsprite_index_sprite=0;

	tbblue_port_303b=0;

	tbblue_registers[22]=0;
	tbblue_registers[23]=0;


}


void tbblue_reset_palettes(void)
{
	//Inicializar Paleta
	int i;

	for (i=0;i<256;i++) {
		//Metemos colores RRRGGGBB0
		tbblue_palette_ula_first[i]=i*2;
 		tbblue_palette_ula_second[i]=i*2;
 		tbblue_palette_layer2_first[i]=i*2;
 		tbblue_palette_layer2_second[i]=i*2;
 		tbblue_palette_sprite_first[i]=i*2;
 		tbblue_palette_sprite_second[i]=i*2;
	}

	//Pero el ultimo color quiero que apunte a 511 y no 510, porque sino, el 510 es:
	//FFFFDBH amarillento
	//y 511 es:
	//FFFFFFH blanco


	tbblue_palette_ula_first[255]=511;
 	tbblue_palette_ula_second[255]=511;
 	tbblue_palette_layer2_first[255]=511;
 	tbblue_palette_layer2_second[255]=511;
 	tbblue_palette_sprite_first[255]=511;
 	tbblue_palette_sprite_second[255]=511;
	

	//Y de momento la paleta ula first tiene los colores lo mas parecido a spectrum:
	//CD->DB
	/*
	0x000000,  //negro 0
0x0000CD,  //azul 1
0xCD0000,  //rojo 2
0xCD00CD,  //magenta 3
0x00CD00,  //verde 4
0x00CDCD,  //cyan 5
0xCDCD00,  //amarillo 6
0xCDCDCD,  //blanco 7

0x000000, 8
0x0000FF, 9 
0xFF0000, 10
0xFF00FF, 11
0x00FF00, 12
0x00FFFF, 13
0xFFFF00, 14
0xFFFFFF  15
*/
	tbblue_palette_ula_first[1]=6; 
	tbblue_palette_ula_first[2]=384;
	tbblue_palette_ula_first[3]=390;
	tbblue_palette_ula_first[4]=48;
	tbblue_palette_ula_first[5]=54;
	tbblue_palette_ula_first[6]=432; 
	tbblue_palette_ula_first[7]=438;
	tbblue_palette_ula_first[8]=0;
	tbblue_palette_ula_first[9]=7;
	tbblue_palette_ula_first[10]=448;
	tbblue_palette_ula_first[11]=455; 

	 //tanto 455 como 454 (1C7H y 1C6H) son colores transparentes por defecto (1C7H y 1C6H  / 2 = E3H)
	tbblue_palette_ula_first[12]=56;
	tbblue_palette_ula_first[13]=63;
	tbblue_palette_ula_first[14]=504;
	tbblue_palette_ula_first[15]=511;


}


void tbblue_out_port_sprite_index(z80_byte value)
{
	//printf ("Out tbblue_out_port_sprite_index %02XH\n",value);
	tbsprite_index_palette=tbsprite_index_pattern=tbsprite_index_sprite=value;

	tbsprite_index_pattern_subindex=tbsprite_index_sprite_subindex=0;
}

/*void tbblue_out_sprite_palette(z80_byte value)
{
	//printf ("Out tbblue_out_sprite_palette %02XH\n",value);

	tbsprite_palette[tbsprite_index_palette]=value;
	if (tbsprite_index_palette==255) tbsprite_index_palette=0;
	else tbsprite_index_palette++;
}*/

//Escribe valor de 8 bits superiores (de total de 9) para indice de color de paleta e incrementa indice
void tbblue_write_palette_value_high8(z80_byte valor)
{
/*
(R/W) 0x40 (64) => Palette Index
  bits 7-0 = Select the palette index to change the default colour. 
  0 to 127 indexes are to ink colours and 128 to 255 indexes are to papers.
  (Except full ink colour mode, that all values 0 to 255 are inks)
  Border colours are the same as paper 0 to 7, positions 128 to 135,
  even at full ink mode. 
  (inks and papers concept only applies to ULANext palette. 
  Layer 2 and Sprite palettes works as "full ink" mode)

  (R/W) 0x41 (65) => Palette Value
  bits 7-0 = Colour for the palette index selected by the register 0x40. Format is RRRGGGBB
  After the write, the palette index is auto-incremented to the next index. 
  The changed palette remains until a Hard Reset.

  (R/W) 0x44 (68) => Palette Lower bit
  bits 7-1 = Reserved, must be 0
  bit 0 = Set the lower blue bit colour for the current palette value
*/
	z80_byte indice=tbblue_registers[0x40];

	//Obtenemos valor actual y alteramos los 8 bits altos del total de 9
	z80_int color_actual=tbblue_get_value_palette_rw(indice);

	//Conservamos bit bajo
	color_actual &=1;

	z80_int valor16=valor;

	//rotamos a la izquierda para que sean los 8 bits altos
	valor16=valor16<<1;
	//y or del valor de 1 bit de B
	valor16 |=color_actual;

	tbblue_set_value_palette_rw(indice,valor16);

	//Y sumamos contador
	indice++;
	tbblue_registers[0x40]=indice;

}


//Escribe valor de 1 bit inferior (de total de 9) para indice de color de paleta 
void tbblue_write_palette_value_low1(z80_byte valor)
{
/*
(R/W) 0x40 (64) => Palette Index
  bits 7-0 = Select the palette index to change the default colour. 
  0 to 127 indexes are to ink colours and 128 to 255 indexes are to papers.
  (Except full ink colour mode, that all values 0 to 255 are inks)
  Border colours are the same as paper 0 to 7, positions 128 to 135,
  even at full ink mode. 
  (inks and papers concept only applies to ULANext palette. 
  Layer 2 and Sprite palettes works as "full ink" mode)

  (R/W) 0x41 (65) => Palette Value
  bits 7-0 = Colour for the palette index selected by the register 0x40. Format is RRRGGGBB
  After the write, the palette index is auto-incremented to the next index. 
  The changed palette remains until a Hard Reset.

  (R/W) 0x44 (68) => Palette Lower bit
  bits 7-1 = Reserved, must be 0
  bit 0 = Set the lower blue bit colour for the current palette value
*/
	z80_byte indice=tbblue_registers[0x40];

	//Obtenemos valor actual y conservamos los 8 bits altos del total de 9
	z80_int color_actual=tbblue_get_value_palette_rw(indice);

	//Conservamos 8 bits altos
	color_actual &=0x1FE;

	//Y valor indicado, solo conservar 1 bit
	valor &= 1;

	color_actual |=valor;

	tbblue_set_value_palette_rw(indice,color_actual);


}

void tbblue_out_sprite_pattern(z80_byte value)
{


	//z80_byte tbsprite_index_pattern,tbsprite_index_pattern_subindex;
	//z80_byte tbsprite_patterns[TBBLUE_MAX_PATTERNS][256];


	tbsprite_patterns[tbsprite_index_pattern][tbsprite_index_pattern_subindex]=value;

//printf ("Out tbblue_out_sprite_pattern. Index: %d subindex: %d %02XH\n",tbsprite_index_pattern,tbsprite_index_pattern_subindex,
//tbsprite_patterns[tbsprite_index_pattern][tbsprite_index_pattern_subindex]);

	if (tbsprite_index_pattern_subindex==255) {
		tbsprite_index_pattern_subindex=0;
		tbsprite_index_pattern++;
		if (tbsprite_index_pattern>=TBBLUE_MAX_PATTERNS) tbsprite_index_pattern=0;
	}
	else tbsprite_index_pattern_subindex++;

}

void tbblue_out_sprite_sprite(z80_byte value)
{
	//printf ("Out tbblue_out_sprite_sprite. Index: %d subindex: %d %02XH\n",tbsprite_index_sprite,tbsprite_index_sprite_subindex,value);



	//Indices al indicar paleta, pattern, sprites. Subindex indica dentro de cada pattern o sprite a que posicion (0..3 en sprites o 0..255 en pattern ) apunta
	//z80_byte tbsprite_index_sprite,tbsprite_index_sprite_subindex;

	tbsprite_sprites[tbsprite_index_sprite][tbsprite_index_sprite_subindex]=value;
	if (tbsprite_index_sprite_subindex==3) {
		//printf ("sprite %d [3] pattern: %d\n",tbsprite_index_sprite,tbsprite_sprites[tbsprite_index_sprite][3]&63);
		tbsprite_index_sprite_subindex=0;
		tbsprite_index_sprite++;
		if (tbsprite_index_sprite>=TBBLUE_MAX_SPRITES) tbsprite_index_sprite=0;
	}

	else tbsprite_index_sprite_subindex++;
}


//Guarda scanline actual y el pattern (los indices a colores) sobre la paleta activa de sprites
//z80_byte sprite_line[MAX_X_SPRITE_LINE];


//Dice si un color de la paleta rbg9 es transparente
int tbblue_si_transparent(z80_int color)
{
	//if ( (color&0x1FE)==TBBLUE_TRANSPARENT_COLOR) return 1;
	color=(color>>1)&0xFF;
	if (color==TBBLUE_TRANSPARENT_REGISTER) return 1;
	return 0;
}


/*
Port 0x243B is write-only and is used to set the registry number.

Port 0x253B is used to access the registry value.

Register:
(R/W) 21 => Sprite system
 bits 7-2 = Reserved, must be 0
 bit 1 = Over border (1 = yes)
 bit 0 = Sprites visible (1 = visible)
*/
void tbsprite_put_color_line(int x,z80_byte color,int rangoxmin,int rangoxmax)
{

	//Si coordenadas invalidas, volver
	//if (x<0 || x>=MAX_X_SPRITE_LINE) return;

	//Si coordenadas fuera de la parte visible (border si o no), volver
	if (x<rangoxmin || x>rangoxmax) return;

	z80_int color_final=tbblue_get_palette_active_sprite(color);

	//Si color transparente, no hacer nada
	if (tbblue_si_transparent(color_final)) return;

	int xfinal=x;

	xfinal +=screen_total_borde_izquierdo*border_enabled.v;
	xfinal -=TBBLUE_SPRITE_BORDER;


	//Ver si habia un color y activar bit colision
	z80_int color_antes=tbblue_layer_sprites[xfinal];

	if (!tbblue_si_transparent(color_antes)) {
		//colision
		tbblue_port_303b |=1;
		//printf ("set colision flag. result value: %d\n",tbblue_port_303b);
	}
	

	//sprite_line[x]=color;
	tbblue_layer_sprites[xfinal]=color_final;

}

z80_byte tbsprite_do_overlay_get_pattern_xy(z80_byte index_pattern,z80_byte sx,z80_byte sy)
{

	return tbsprite_patterns[index_pattern][sy*TBBLUE_SPRITE_WIDTH+sx];
}

z80_int tbsprite_return_color_index(z80_byte index)
{
	//z80_int color_final=tbsprite_palette[index];

	z80_int color_final=tbblue_get_palette_active_sprite(index);
	//return RGB9_INDEX_FIRST_COLOR+color_final;
	return color_final;
}

void tbsprite_do_overlay(void)
{

        //spritechip activo o no?
        if ( (tbblue_registers[21]&1)==0) return;

				//printf ("tbblue sprite chip activo\n");


        //int scanline_copia=t_scanline_draw-screen_indice_inicio_pant;
        int y=t_scanline_draw; //0..63 es border (8 no visibles)

				int border_no_visible=screen_indice_inicio_pant-TBBLUE_SPRITE_BORDER;

				y -=border_no_visible;

				//Ejemplo: scanline_draw=32 (justo donde se ve sprites). border_no_visible=64-32 =32
				//y=y-32 -> y=0


				//Situamos el 0 32 pixeles por encima de dentro de pantalla, tal cual como funcionan las cordenadas de sprite de tbblue


        //if (border_enabled.v==0) y=y-screen_borde_superior;
        //z80_int *puntero_buf_rainbow;
        //puntero_buf_rainbow=&rainbow_buffer[ y*get_total_ancho_rainbow() ];


				//Calculos exclusivos para puntero buffer rainbow
		    int rainbowy=t_scanline_draw-screen_invisible_borde_superior;
		    if (border_enabled.v==0) rainbowy=rainbowy-screen_borde_superior;
		    //z80_int *puntero_buf_rainbow;
		    //puntero_buf_rainbow=&rainbow_buffer[ rainbowy*get_total_ancho_rainbow() ];

		    int puntero_array_layer=0;




        //puntero_buf_rainbow +=screen_total_borde_izquierdo*border_enabled.v;

				//printf ("overlay t_scanline_draw: %d y: %d\n",t_scanline_draw,y);

				//Aqui tenemos el y=0 arriba del todo del border

        //Bucle para cada sprite
        int conta_sprites;
				z80_byte index_pattern;

		int i;
		//int offset_pattern;

		z80_byte sprites_over_border=tbblue_registers[21]&2;


				//Inicializar linea a transparente
				//for (i=0;i<MAX_X_SPRITE_LINE;i++) {
				//	sprite_line[i]=TBBLUE_TRANSPARENT_COLOR;
				//}

				int rangoxmin, rangoxmax;

				if (sprites_over_border) {
					rangoxmin=0;
					rangoxmax=TBBLUE_SPRITE_BORDER+256+TBBLUE_SPRITE_BORDER-1;
				}

				else {
					rangoxmin=TBBLUE_SPRITE_BORDER;
					rangoxmax=TBBLUE_SPRITE_BORDER+255;
				}


				int total_sprites=0;


        for (conta_sprites=0;conta_sprites<TBBLUE_MAX_SPRITES && total_sprites<MAX_SPRITES_PER_LINE;conta_sprites++) {
					int sprite_x;
					int sprite_y;



					/*

					OLD
					[0] 1st: X position (bits 7-0).
					[1] 2nd: Y position (0-255).
					[2] 3rd: bits 7-4 is palette offset, bit 3 is X mirror, bit 2 is Y mirror, bit 1 is visible flag and bit 0 is X MSB.
					[3] 4th: bits 7-6 is reserved, bits 5-0 is Name (pattern index, 0-63).

					NEW
					[0] 1st: X position (bits 7-0).
					[1] 2nd: Y position (0-255).
					[2] 3rd: bits 7-4 is palette offset, bit 3 is X mirror, bit 2 is Y mirror, bit 1 is rotate flag and bit 0 is X MSB.
					[3] 4th: bit 7 is visible flag, bit 6 is reserved, bits 5-0 is Name (pattern index, 0-63).


					*/
					/*
					Because sprites can be displayed on top of the ZX Spectrum border, the coordinates of each sprite can range
					from 0 to 319 for the X axis and 0 to 255 for the Y axis. For both axes, values from 0 to 31 are reserved
					for the Left or top border, for the X axis the values 288 to 319 is reserved for the right border and for
					the Y axis values 224 to 255 for the lower border.

If the display of the sprites on the border is disabled, the coordinates of the sprites range from (32,32) to (287,223).
*/

					//Si sprite visible
					if (tbsprite_sprites[conta_sprites][3]&128) {
						sprite_x=tbsprite_sprites[conta_sprites][0] | ((tbsprite_sprites[conta_sprites][2]&1)<<8);

						//printf ("sprite %d x: %d \n",conta_sprites,sprite_x);

						sprite_y=tbsprite_sprites[conta_sprites][1];

						//Posicionamos esa y teniendo en cuenta que nosotros contamos 0 arriba del todo del border en cambio sprites aqui
						//Considera y=32 dentro de pantalla y y=0..31 en el border
						//sprite_y +=screen_borde_superior-32;

						//Si y==32-> y=32+48-32=32+16=48
						//Si y==0 -> y=48-32=16

						z80_byte mirror_x=tbsprite_sprites[conta_sprites][2]&8;
						//[2] 3rd: bits 7-4 is palette offset, bit 3 is X mirror, bit 2 is Y mirror, bit 1 is rotate flag and bit 0 is X MSB.
						z80_byte mirror_y=tbsprite_sprites[conta_sprites][2]&4;

						//3rd: bits 7-4 is palette offset, bit 3 is X mirror, bit 2 is Y mirror, bit 1 is rotate flag and bit 0 is X MSB.
						//Offset paleta se lee tal cual sin rotar valor
						z80_byte palette_offset=(tbsprite_sprites[conta_sprites][2]) & 0xF0;

						index_pattern=tbsprite_sprites[conta_sprites][3]&63;
						//Si coordenada y esta en margen y sprite activo

						int diferencia=y-sprite_y;


						int rangoymin, rangoymax;

						if (sprites_over_border) {
							rangoymin=0;
							rangoymax=TBBLUE_SPRITE_BORDER+192+TBBLUE_SPRITE_BORDER-1;
						}

						else {
							rangoymin=TBBLUE_SPRITE_BORDER;
							rangoymax=TBBLUE_SPRITE_BORDER+191;
						}


						//Pintar el sprite si esta en rango de coordenada y
						if (diferencia>=0 && diferencia<TBBLUE_SPRITE_HEIGHT && y>=rangoymin && y<=rangoymax) {

							//printf ("y: %d t_scanline_draw: %d rainbowy:%d sprite_y: %d\n",y,t_scanline_draw,rainbowy,sprite_y);
							z80_byte sx=0,sy=0; //Coordenadas x,y dentro del pattern
							//offset_pattern=0;

							//Incrementos de x e y
							int incx=+1;
							int incy=0;

							//Aplicar mirror si conviene y situarnos en la ultima linea
							if (mirror_y) {
								//offset_pattern=offset_pattern+TBBLUE_SPRITE_WIDTH*(TBBLUE_SPRITE_HEIGHT-1);
								sy=TBBLUE_SPRITE_HEIGHT-1-diferencia;
								//offset_pattern -=TBBLUE_SPRITE_WIDTH*diferencia;
							}
							else {
								//offset_pattern +=TBBLUE_SPRITE_WIDTH*diferencia;
								sy=diferencia;
							}


							//index_pattern ya apunta a pattern a pintar en pantalla
							//z80_int *puntero_buf_rainbow_sprite;
							//puntero_buf_rainbow_sprite=puntero_buf_rainbow+sprite_x;

							//Dibujar linea x

							//Cambiar offset si mirror x, ubicarlo a la derecha del todo
							if (mirror_x) {
								//offset_pattern=offset_pattern+TBBLUE_SPRITE_WIDTH-1;
								sx=TBBLUE_SPRITE_WIDTH-1;
								incx=-1;
							}

							z80_byte sprite_rotate;

							sprite_rotate=tbsprite_sprites[conta_sprites][2]&2;

							/*
							Comparar bits rotacion con ejemplo en media/spectrum/tbblue/sprites/rotate_example.png
							*/
							/*
							Basicamente sin rotar un sprite, se tiene (reduzco el tamaño a la mitad aqui para que ocupe menos)


							El sentido normal de dibujado viene por ->, aumentando coordenada X


					->  ---X----
							---XX---
							---XXX--
							---XXXX-
							---X----
							---X----
							---X----
							---X----

							Luego cuando se rota 90 grados, en vez de empezar de arriba a la izquierda, se empieza desde abajo y reduciendo coordenada Y:

							    ---X----
									---XX---
									---XXX--
									---XXXX-
									---X----
									---X----
							^ 	---X----
							|		---X----

							Entonces, al dibujar empezando asi, la imagen queda rotada:

							--------
							--------
							XXXXXXXX
							----XXX-
							----XX--
							----X---
							--------

							De ahi que el incremento y sea -incremento x , incremento x sera 0

							Aplicando tambien el comportamiento para mirror, se tiene el resto de combinaciones

							*/


							if (sprite_rotate) {
								z80_byte sy_old=sy;
								sy=(TBBLUE_SPRITE_HEIGHT-1)-sx;
								sx=sy_old;

								incy=-incx;
								incx=0;
							}


							for (i=0;i<TBBLUE_SPRITE_WIDTH;i++) {
								//z80_byte index_color=tbsprite_patterns[index_pattern][offset_pattern];
								z80_byte index_color=tbsprite_do_overlay_get_pattern_xy(index_pattern,sx,sy);

								//Sumar palette offset. Logicamente si es >256 el resultado, dará la vuelta el contador
								index_color +=palette_offset;

								//printf ("index color: %d\n",index_color);
								

								sx=sx+incx;
								sy=sy+incy;

								/*
								if (mirror_x) {
									//offset_pattern--;
									sx--;
								}
								else {
									//offset_pattern++;
									sx++;
								}*/
								//z80_byte color=tbsprite_palette[index_color];
								//tbsprite_put_color_line(sprite_x++,color,rangoxmin,rangoxmax);
								tbsprite_put_color_line(sprite_x++,index_color,rangoxmin,rangoxmax);


							}

							total_sprites++;
							//printf ("total sprites in this line: %d\n",total_sprites);
							if (total_sprites==MAX_SPRITES_PER_LINE) {
								//max sprites per line flag
								tbblue_port_303b |=2;
								//printf ("set max sprites per line flag\n");
							}

						}

				}
			}

			//Dibujar linea de sprites en pantalla ignorando color transparente

			//Tener en cuenta que de 0..31 en x es el border
			//Posicionar puntero rainbow en zona interior pantalla-32 pixels border

			//puntero_buf_rainbow +=screen_total_borde_izquierdo*border_enabled.v;

			//puntero_buf_rainbow -=TBBLUE_SPRITE_BORDER;



			//Inicializar linea a transparente

/*
Port 0x243B is write-only and is used to set the registry number.

Port 0x253B is used to access the registry value.

Register:
(R/W) 21 => Sprite system
 bits 7-2 = Reserved, must be 0
 bit 1 = Over border (1 = yes)
 bit 0 = Sprites visible (1 = visible)
 */



			//Si no se permite en el border, rango entre 0..31 y 288..319 no permitido
			/*
			#define MAX_X_SPRITE_LINE 320
			z80_byte sprite_line[MAX_X_SPRITE_LINE];

			#define MAX_SPRITES_PER_LINE 12

			#define TBBLUE_SPRITE_BORDER 32
			*/



			//Dibujar en buffer rainbow
			/*
			for (i=0;i<MAX_X_SPRITE_LINE;i++) {
				z80_byte color=sprite_line[i];

				if (i>=rangoxmin && i<=rangoxmax) {

					//color transparente hace referencia al valor indice que hay en pattern. y NO al color final
					//if (color!=TBBLUE_TRANSPARENT_COLOR) {

						//Pasamos de RGB a GRB
						//z80_byte r,g,b;
						//r=(color>>5)&7;
						//g=(color>>2)&7;
						//b=(color&3);

						z80_byte colorulaplus=(g<<5)|(r<<2)|b;


						//TODO conversion rgb. esto no es ulaplus. usamos tabla ulaplus solo para probar


						//color_final=colorulaplus+ULAPLUS_INDEX_FIRST_COLOR;
						z80_int color_final;
						//color_final=RGB8_INDEX_FIRST_COLOR+color;
						color_final=tbsprite_return_color_index(color);
						//color_final=ulaplus_rgb_table[color_final];

						// *puntero_buf_rainbow=color_final;
						tbblue_layer_sprites[puntero_array_layer]=color_fnal;

					//}
				}

				//puntero_buf_rainbow++;
				puntero_array_layer++;

			}
			*/

}

z80_byte tbblue_get_port_layer2_value(void)
{
	return tbblue_port_123b;
}

void tbblue_out_port_layer2_value(z80_byte value)
{
	tbblue_port_123b=value;
}


z80_byte tbblue_get_port_sprite_index(void)
{
	/*
	Port 0x303B, if read, returns some information:

Bits 7-2: Reserved, must be 0.
Bit 1: max sprites per line flag.
Bit 0: Collision flag.
*/
	z80_byte value=tbblue_port_303b;
	//Cuando se lee, se resetean bits 0 y 1
	//printf ("-----Reading port 303b. result value: %d\n",value);
	tbblue_port_303b &=(255-1-2);

	return value;

}



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

//z80_byte tbblue_config1=0;

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

//z80_byte tbblue_config2=0;
//z80_byte tbblue_hardsoftreset=0;


//Si segmento bajo (0-16383) es escribible
z80_bit tbblue_low_segment_writable={0};


//Indice a lectura del puerto
//z80_byte tbblue_read_port_24d5_index=0;

//port 0x24DF bit 2 = 1 turbo mode (7Mhz)
//z80_byte tbblue_port_24df;


//Asumimos 256 registros
z80_byte tbblue_registers[256];

//Ultimo registro seleccionado
z80_byte tbblue_last_register;


void tbblue_init_memory_tables(void)
{
/*

Primer bloque de ram: memoria interna de tbblue en principio no accesible por el spectrum:

Mapeo viejo

0x000000 – 0x01FFFF (128K) => DivMMC RAM
0x020000 – 0x03FFFF (128K) => Layer2 RAM
0x040000 – 0x05FFFF (128K) => ??????????
0x060000 – 0x06FFFF (64K) => ESXDOS and Multiface RAM
0x060000 – 0x063FFF (16K) => ESXDOS ROM
0x064000 – 0x067FFF (16K) => Multiface ROM
0x068000 – 0x06BFFF (16K) => Multiface extra ROM
0x06c000 – 0x06FFFF (16K) => Multiface RAM
0x070000 – 0x07FFFF (64K) => ZX Spectrum ROM

Segundo bloque de ram: 512K, todo accesible para Spectrum. Se mapean 256 o 512 mediante bit 6 y 7 de puerto 32765
0x080000 - 0x0FFFFF (512K) => Speccy RAM

Luego 8 KB de rom de la fpga
0x100000 - 0x101FFF

Nuevo:

0x000000 – 0x00FFFF (64K) => ZX Spectrum ROM
0x010000 – 0x013FFF (16K) => ESXDOS ROM
0x014000 – 0x017FFF (16K) => Multiface ROM
0x018000 – 0x01BFFF (16K) => Multiface extra ROM
0x01c000 – 0x01FFFF (16K) => Multiface RAM
0x020000 – 0x05FFFF (256K) => divMMC RAM
0x060000 – 0x07FFFF (128K) => ZX Spectrum RAM
0x080000 – 0x0FFFFF (512K) => Extra RAM

*/





	int i,indice;

	//Los 8 KB de la fpga ROM estan al final
	tbblue_fpga_rom=&memoria_spectrum[1024*1024];

	//64 Paginas RAM spectrum 512k. Hay 128kb mas despues de estos...
	for (i=0;i<64;i++) {
		indice=0x060000+8192*i;
		tbblue_ram_memory_pages[i]=&memoria_spectrum[indice];
	}

	//4 Paginas ROM
	for (i=0;i<8;i++) {
		indice=0+8192*i;
		tbblue_rom_memory_pages[i]=&memoria_spectrum[indice];
	}

}

void tbblue_set_ram_page(z80_byte segment)
{
	z80_byte tbblue_register=80+segment;
	z80_byte reg_value=tbblue_registers[tbblue_register];

	//tbblue_memory_paged[segment]=tbblue_ram_memory_pages[page];
	tbblue_memory_paged[segment]=tbblue_ram_memory_pages[reg_value];

	debug_paginas_memoria_mapeadas[segment]=reg_value;
}

void tbblue_set_rom_page(z80_byte segment,z80_byte page)
{
	z80_byte tbblue_register=80+segment;
	z80_byte reg_value=tbblue_registers[tbblue_register];

	if (reg_value==255) {
		tbblue_memory_paged[segment]=tbblue_rom_memory_pages[page];
		debug_paginas_memoria_mapeadas[segment]=page+128;
	}
	else {
	  tbblue_memory_paged[segment]=tbblue_ram_memory_pages[reg_value];
	  debug_paginas_memoria_mapeadas[segment]=reg_value;
	}
}


void tbblue_mem_page_ram_rom(void)
{
	z80_byte page_type;

	page_type=(puerto_8189 >>1) & 3;

	switch (page_type) {
		case 0:
			debug_printf (VERBOSE_DEBUG,"Pages 0,1,2,3");
			tbblue_registers[80]=0*2;
			tbblue_registers[81]=0*2+1;
			tbblue_registers[82]=1*2;
			tbblue_registers[83]=1*2+1;
			tbblue_registers[84]=2*2;
			tbblue_registers[85]=2*2+1;
			tbblue_registers[86]=3*2;
			tbblue_registers[87]=3*2+1;


			tbblue_set_ram_page(0*2);
			tbblue_set_ram_page(0*2+1);
			tbblue_set_ram_page(1*2);
			tbblue_set_ram_page(1*2+1);
			tbblue_set_ram_page(2*2);
			tbblue_set_ram_page(2*2+1);
			tbblue_set_ram_page(3*3);
			tbblue_set_ram_page(3*2+1);

			contend_pages_actual[0]=contend_pages_128k_p2a[0];
			contend_pages_actual[1]=contend_pages_128k_p2a[1];
			contend_pages_actual[2]=contend_pages_128k_p2a[2];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];



			break;

		case 1:
			debug_printf (VERBOSE_DEBUG,"Pages 4,5,6,7");

			tbblue_registers[80]=4*2;
			tbblue_registers[81]=4*2+1;
			tbblue_registers[82]=5*2;
			tbblue_registers[83]=5*2+1;
			tbblue_registers[84]=6*2;
			tbblue_registers[85]=6*2+1;
			tbblue_registers[86]=7*2;
			tbblue_registers[87]=7*2+1;

			tbblue_set_ram_page(0*2);
			tbblue_set_ram_page(0*2+1);
			tbblue_set_ram_page(1*2);
			tbblue_set_ram_page(1*2+1);
			tbblue_set_ram_page(2*2);
			tbblue_set_ram_page(2*2+1);
			tbblue_set_ram_page(3*2);
			tbblue_set_ram_page(3*2+1);


			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[5];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[7];





			break;

		case 2:
			debug_printf (VERBOSE_DEBUG,"Pages 4,5,6,3");

			tbblue_registers[80]=4*2;
			tbblue_registers[81]=4*2+1;
			tbblue_registers[82]=5*2;
			tbblue_registers[83]=5*2+1;
			tbblue_registers[84]=6*2;
			tbblue_registers[85]=6*2+1;
			tbblue_registers[86]=3*2;
			tbblue_registers[87]=3*2+1;

			tbblue_set_ram_page(0*2);
			tbblue_set_ram_page(0*2+1);
			tbblue_set_ram_page(1*2);
			tbblue_set_ram_page(1*2+1);
			tbblue_set_ram_page(2*2);
			tbblue_set_ram_page(2*2+1);
			tbblue_set_ram_page(3*2);
			tbblue_set_ram_page(3*2+1);

			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[5];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];




			break;

		case 3:
			debug_printf (VERBOSE_DEBUG,"Pages 4,7,6,3");

			tbblue_registers[80]=4*2;
			tbblue_registers[81]=4*2+1;
			tbblue_registers[82]=7*2;
			tbblue_registers[83]=7*2+1;
			tbblue_registers[84]=6*2;
			tbblue_registers[85]=6*2+1;
			tbblue_registers[86]=3*2;
			tbblue_registers[87]=3*2+1;

			tbblue_set_ram_page(0*2);
			tbblue_set_ram_page(0*2+1);
			tbblue_set_ram_page(1*2);
			tbblue_set_ram_page(1*2+1);
			tbblue_set_ram_page(2*2);
			tbblue_set_ram_page(2*2+1);
			tbblue_set_ram_page(3*2);
			tbblue_set_ram_page(3*2+1);

			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[7];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];




			break;

	}
}

z80_byte tbblue_mem_get_ram_page(void)
{

	//printf ("Valor 32765: %d\n",puerto_32765);

	z80_byte ram_entra=puerto_32765&7;

	z80_byte bit3=0;
	z80_byte bit4=0;

	//Forzamos a que lea siempre bit 6 y 7 del puerto 32765
	//Dejamos esto asi por si en un futuro hay manera de limitar la lectura de esos bits

	int multiplicador=4; //multiplicamos 128*4

	if (multiplicador==2 || multiplicador==4) {
		bit3=puerto_32765&64;  //Bit 6
		//Lo movemos a bit 3
		bit3=bit3>>3;
	}

  if (multiplicador==4) {
      bit4=puerto_32765&128;  //Bit 7
      //Lo movemos a bit 4
      bit4=bit4>>3;
  }


	ram_entra=ram_entra|bit3|bit4;

	//printf ("ram entra: %d\n",ram_entra);

	return ram_entra;
}


void tbblue_set_mmu_128k_default(void)
{
	//rom default, paginas ram 5,2,0
	tbblue_registers[80]=255;
	tbblue_registers[81]=255;
	tbblue_registers[82]=10;
	tbblue_registers[83]=11;
	tbblue_registers[84]=4;
	tbblue_registers[85]=5;
	tbblue_registers[86]=0;
	tbblue_registers[87]=1;

	debug_paginas_memoria_mapeadas[0]=0;
	debug_paginas_memoria_mapeadas[1]=1;
	debug_paginas_memoria_mapeadas[2]=10;
	debug_paginas_memoria_mapeadas[3]=11;
	debug_paginas_memoria_mapeadas[4]=4;
	debug_paginas_memoria_mapeadas[5]=5;
	debug_paginas_memoria_mapeadas[6]=0;
	debug_paginas_memoria_mapeadas[7]=1;

}

//Indica si estamos en modo ram in rom del +2a
z80_bit tbblue_was_in_p2a_ram_in_rom={0};


void tbblue_set_memory_pages(void)
{
	//Mapeamos paginas de RAM segun config maquina
	z80_byte maquina=(tbblue_registers[3])&7;

	int romram_page;
	int ram_page,rom_page;
	int indice;

	//Por defecto
	tbblue_low_segment_writable.v=0;

	//printf ("tbblue set memory pages. maquina=%d\n",maquina);
	/*
	bits 1-0 = Machine type:
		00 = Config mode (bootrom)
		01 = ZX 48K
		10 = ZX 128K
		11 = ZX +2/+3e
	*/

	switch (maquina) {
		case 1:
                    //001 = ZX 48K
			tbblue_set_rom_page(0,0*2);
			tbblue_set_rom_page(1,0*2+1);
			tbblue_set_ram_page(2);
			tbblue_set_ram_page(3);
			tbblue_set_ram_page(4);
			tbblue_set_ram_page(5);
			tbblue_set_ram_page(6);
			tbblue_set_ram_page(7);


		        contend_pages_actual[0]=0;
		        contend_pages_actual[1]=contend_pages_128k_p2a[5];
		        contend_pages_actual[2]=contend_pages_128k_p2a[2];
		        contend_pages_actual[3]=contend_pages_128k_p2a[0];


			//tbblue_low_segment_writable.v=0;
		break;

		case 2:
                    //010 = ZX 128K
			rom_page=(puerto_32765>>4)&1;
                        tbblue_set_rom_page(0,rom_page*2);
			tbblue_set_rom_page(1,rom_page*2+1);

                        tbblue_set_ram_page(2);
			tbblue_set_ram_page(3);

                        tbblue_set_ram_page(4);
			tbblue_set_ram_page(5);

			ram_page=tbblue_mem_get_ram_page();
			tbblue_registers[80+6]=ram_page*2;
			tbblue_registers[80+7]=ram_page*2+1;


                        tbblue_set_ram_page(6);
			tbblue_set_ram_page(7);



			//tbblue_low_segment_writable.v=0;
		        contend_pages_actual[0]=0;
		        contend_pages_actual[1]=contend_pages_128k_p2a[5];
		        contend_pages_actual[2]=contend_pages_128k_p2a[2];
		        contend_pages_actual[3]=contend_pages_128k_p2a[ram_page];


		break;

		case 3:
			//011 = ZX +2/+3e
			//Si RAM en ROM
			if (puerto_8189&1) {

				tbblue_mem_page_ram_rom();
				//printf ("setting low segment writeable as port 8189 bit 1\n");
				tbblue_low_segment_writable.v=1;

				tbblue_was_in_p2a_ram_in_rom.v=1;
			}

			else {

				//printf ("NOT setting low segment writeable as port 8189 bit 1\n");

			 	//Si se cambiaba de modo ram in rom a normal
				if (tbblue_was_in_p2a_ram_in_rom.v) {
					debug_printf(VERBOSE_DEBUG,"Going from ram in rom mode to normal mode. Setting default ram pages");
					tbblue_set_mmu_128k_default();
					tbblue_was_in_p2a_ram_in_rom.v=0;
				}

                    //when "11"    => maquina <= s_speccy3e;
                        	rom_page=(puerto_32765>>4)&1;

		        	z80_byte rom1f=(puerto_8189>>1)&2;
		        	z80_byte rom7f=(puerto_32765>>4)&1;

				z80_byte rom_page=rom1f | rom7f;


                        	tbblue_set_rom_page(0,rom_page*2);
				tbblue_set_rom_page(1,rom_page*2+1);

                        	tbblue_set_ram_page(2);
				tbblue_set_ram_page(3);

                        	tbblue_set_ram_page(4);
				tbblue_set_ram_page(5);

                        	ram_page=tbblue_mem_get_ram_page();
				tbblue_registers[80+6]=ram_page*2;
				tbblue_registers[80+7]=ram_page*2+1;
                        	tbblue_set_ram_page(6);
				tbblue_set_ram_page(7);

		        	contend_pages_actual[0]=0;
		        	contend_pages_actual[1]=contend_pages_128k_p2a[5];
		        	contend_pages_actual[2]=contend_pages_128k_p2a[2];
		        	contend_pages_actual[3]=contend_pages_128k_p2a[ram_page];


			}
		break;

		case 4:
                    //100 = Pentagon 128K. TODO. de momento tal cual 128kb
			rom_page=(puerto_32765>>4)&1;
                        tbblue_set_rom_page(0,rom_page*2);
			tbblue_set_rom_page(1,rom_page*2+1);

                        tbblue_set_ram_page(2);
			tbblue_set_ram_page(3);

                        tbblue_set_ram_page(4);
			tbblue_set_ram_page(5);

			ram_page=tbblue_mem_get_ram_page();
			tbblue_registers[80+6]=ram_page*2;
			tbblue_registers[80+7]=ram_page*2+1;
                        tbblue_set_ram_page(6);
			tbblue_set_ram_page(7);


			//tbblue_low_segment_writable.v=0;
		        contend_pages_actual[0]=0;
		        contend_pages_actual[1]=contend_pages_128k_p2a[5];
		        contend_pages_actual[2]=contend_pages_128k_p2a[2];
		        contend_pages_actual[3]=contend_pages_128k_p2a[ram_page];


		break;

		default:

			//Caso maquina 0 u otros no contemplados
			//000 = Config mode

			//printf ("tbblue_bootrom.v=%d\n",tbblue_bootrom.v);

			if (tbblue_bootrom.v==0) {
				/*
When the variable 'bootrom' takes '0', page 0 (0-16383) is mapped to the RAM 1024K,
and the page mapping is configured by bits 5-0 of the I/O port 'config1'.
These 6 bits maps 64 16K pages the start of 1024K SRAM space 0-16363 the Speccy,
which allows you access to all SRAM.
*/
				romram_page=(tbblue_registers[4]&63);
				indice=romram_page*16384;
				//printf ("page on 0-16383: %d offset: %X\n",romram_page,indice);
				tbblue_memory_paged[0]=&memoria_spectrum[indice];
				tbblue_memory_paged[1]=&memoria_spectrum[indice+8192];
				tbblue_low_segment_writable.v=1;
				//printf ("low segment writable for machine default\n");

				debug_paginas_memoria_mapeadas[0]=romram_page;
				debug_paginas_memoria_mapeadas[1]=romram_page;
			}
			else {
				//In this setting state, the page 0 repeats the content of the ROM 'loader', ie 0-8191 appear memory contents, and repeats 8192-16383
				//La rom es de 8 kb pero la hemos cargado dos veces
				tbblue_memory_paged[0]=tbblue_fpga_rom;
				tbblue_memory_paged[1]=&tbblue_fpga_rom[8192];
				//tbblue_low_segment_writable.v=0;
				//printf ("low segment NON writable for machine default\n");
				debug_paginas_memoria_mapeadas[0]=0;
				debug_paginas_memoria_mapeadas[1]=0;
			}

			tbblue_set_ram_page(2);
			tbblue_set_ram_page(3);
			tbblue_set_ram_page(4);
			tbblue_set_ram_page(5);

			//En modo config, ram7 esta en segmento 3
			tbblue_registers[80+6]=7*2;
			tbblue_registers[80+7]=7*2+1;


			tbblue_set_ram_page(6);
			tbblue_set_ram_page(7);




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
(W)		06 => Peripheral 2 setting, only in bootrom or config mode:
			bit 7 = Enable turbo mode (0 = disabled, 1 = enabled)
			bit 6 = DAC chip mode (0 = I2S, 1 = JAP)
			bit 5 = Enable Lightpen  (1 = enabled)
			bit 4 = Enable DivMMC (1 = enabled) -> divmmc automatic paging. divmmc memory is supported using manual
		*/
        //z80_byte diven=tbblue_config2&4;
				z80_byte diven=tbblue_registers[6]&16;
        debug_printf (VERBOSE_INFO,"Apply config.divmmc change: %s",(diven ? "enabled" : "disabled") );
        //printf ("Apply config2.divmmc change: %s\n",(diven ? "enabled" : "disabled") );

				if (diven) {
					//printf ("Activando diviface automatic paging\n");
					divmmc_diviface_enable();
					diviface_allow_automatic_paging.v=1;
				}

        //else divmmc_diviface_disable();
				else {
					//printf ("Desactivando diviface automatic paging\n");
					diviface_allow_automatic_paging.v=0;
					//Y hacer un page-out si hay alguna pagina activa
					diviface_paginacion_automatica_activa.v=0;
				}

}

void tbblue_set_emulator_setting_ulaplus(void)
{

/*
(R/W)	05 => Peripheral 1 setting, only in bootrom or config mode:
			bits 7-6 = (W) joystick 1 mode (00 = Sinclair, 01 = Kempston, 10 = Cursor)
			bits 5-4 = (W) joystick 2 mode (same as joy1)
			bit 3 = (W) Enable ULAplus (1 = enabled)
			bit 2 = (R/W) 50/60 Hz mode (0 = 50Hz, 1 = 60Hz)
			bit 1 = (R/W) Enable Scanlines (1 = enabled)
			bit 0 = (R/W) Enable Scandoubler (1 = enabled)
			*/
        //z80_byte ulaplusen=tbblue_config2&8;
				z80_byte ulaplusen=tbblue_registers[5]&8;
        debug_printf (VERBOSE_INFO,"Apply config.ulaplus change: %s",(ulaplusen ? "enabled" : "disabled") );
        //printf ("Apply config2.ulaplus change: %s\n",(ulaplusen ? "enabled" : "disabled") );
        if (ulaplusen) enable_ulaplus();
        else disable_ulaplus();
}


void tbblue_set_emulator_setting_turbo(void)
{
	/*
	(R/W)	07 => Turbo mode
				bit 1-0 = Turbo (00 = 3.5MHz, 01 = 7MHz, 10 = 14MHz, 11 = 28MHz)(Reset to 00 after a PoR or Hard-reset)

				*/
	z80_byte t=tbblue_registers[7] & 3;
	if (t==0) cpu_turbo_speed=1;
	else if (t==1) cpu_turbo_speed=2;
	else if (t==2) cpu_turbo_speed=4;
	else cpu_turbo_speed=8;


	//printf ("Setting turbo: %d\n",cpu_turbo_speed);

	cpu_set_turbo_speed();
}

void tbblue_reset(void)
{

	//Los bits reservados los metemos a 0 también

	/*
	(R/W) 02 => Reset:
  bits 7-3 = Reserved, must be 0
  bit 2 = (R) Power-on reset (PoR)
  bit 1 = (R/W) Reading 1 indicates a Hard-reset. If written 1 causes a Hard Reset.
  bit 0 = (R/W) Reading 1 indicates a Soft-reset. If written 1 causes a Soft Reset.
	*/
	tbblue_registers[2]=1;
	/*
	(R/W) 0x14 (20) => Global transparency color
  bits 7-0 = Transparency color value (Reset to 0xE3, after a reset)

	*/
	tbblue_registers[20]=0xE3;

	tbblue_registers[21]=0;
	tbblue_registers[22]=0;
	tbblue_registers[23]=0;

	tbblue_registers[30]=0;
	tbblue_registers[31]=0;
	tbblue_registers[34]=0;
	tbblue_registers[35]=0;

	/*
	(R/W) 21 => Sprite system
  bits 7-2 = Reserved, must be 0
  bit 1 = Over border (1 = yes)(Reset to 0 after a reset)
  bit 0 = Sprites visible (1 = visible)(Reset to 0 after a reset)

(R/W) 22 => Layer2 Offset X
  bits 7-0 = X Offset (0-255)(Reset to 0 after a reset)

(R/W) 23 => Layer2 Offset Y
  bist 7-6 = Reserved, must be 0
  bits 5-0 = Y Offset (0-63)(Reset to 0 after a reset)

(R) 30 => Raster video line (MSB)
  bits 7-1 = Reserved, always 0
  bit 0 = Raster line MSB (Reset to 0 after a reset)

(R) 31 = Raster video line (LSB)
  bits 7-0 = Raster line LSB (0-255)(Reset to 0 after a reset)

(R/W) 34 => Raster line interrupt control
  bit 7 = (R) INT flag, 1=During INT (even if the processor has interrupt disabled)
  bits 6-3 = Reserved, must be 0
  bit 2 = If 1 disables original ULA interrupt (Reset to 0 after a reset)
  bit 1 = If 1 enables Raster line interrupt (Reset to 0 after a reset)
  bit 0 = MSB of Raster line interrupt value (Reset to 0 after a reset)
(R/W) 35 => Raster line interrupt value LSB
  bits 7-0 = Raster line value LSB (0-255)(Reset to 0 after a reset)
	*/


//(R/W) 0x50 (80) => MMU slot 0 (Reset to 255 after a reset)
	tbblue_set_mmu_128k_default();

	tbblue_was_in_p2a_ram_in_rom.v=0;


}

void tbblue_hard_reset(void)
{
	//tbblue_config1=0;
	//tbblue_config2=0;
	//tbblue_port_24df=0;
	//tbblue_hardsoftreset=0;

	/*
	(R/W) 02 => Reset:
	  bits 7-3 = Reserved, must be 0
	  bit 2 = (R) Power-on reset (PoR)
	  bit 1 = (R/W) Reading 1 indicates a Hard-reset. If written 1 causes a Hard Reset.
	  bit 0 = (R/W) Reading 1 indicates a Soft-reset. If written 1 causes a Soft Reset.
	*/

	//Aqui no estoy distinguiendo entre hard reset y power-on reset, dado que al iniciar maquina siempre llama a hard reset
	tbblue_registers[2]=4+2;


	tbblue_registers[3]=0;
	tbblue_registers[4]=0;
	tbblue_registers[5]=0;
	tbblue_registers[6]=0;
	tbblue_registers[7]=0;
	tbblue_registers[8]=0;

//TODO. Temporal . pagina sram para layer2 forzada a 32. 32*16384=0x80000
	//0x080000 – 0x0FFFFF (512K) => Extra RAM
	tbblue_registers[18]=32;
	tbblue_registers[19]=32;


	tbblue_registers[20]=TBBLUE_DEFAULT_TRANSPARENT;

	tbblue_registers[21]=0;
	tbblue_registers[22]=0;
	tbblue_registers[23]=0;

	tbblue_registers[30]=0;
	tbblue_registers[31]=0;
	tbblue_registers[34]=0;

	tbblue_port_123b=0;


	//(R/W) 0x50 (80) => MMU slot 0 (Reset to 255 after a reset)
	tbblue_set_mmu_128k_default();


	tbblue_bootrom.v=1;
	//printf ("----setting bootrom to 1\n");

	tbblue_was_in_p2a_ram_in_rom.v=0;

	tbblue_set_memory_pages();



	tbblue_set_emulator_setting_divmmc();
	tbblue_set_emulator_setting_ulaplus();

	tbblue_reset_sprites();
	tbblue_reset_palettes();

}





/*
z80_byte old_tbblue_read_port_24d5(void)
{

//- port 0x24D5: 1st read = return hardware number (below), 2nd read = return firmware version number (first nibble.second nibble) e.g. 0x12 = 1.02
//
//hardware numbers
//1 = DE-1 (old)
//2 = DE-2 (old)
//3 = DE-2 (new)
//4 = DE-1 (new)
//5 = FBLabs
//6 = VTrucco
//7 = WXEDA hardware (placa de desarrollo)
//8 = Emulators
//9 = ZX Spectrum Next


/

}

*/

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
	(W) 0x03 (03) => Set machine type, only in IPL or config mode:
	A write in this register disables the IPL
	(0x0000-0x3FFF are mapped to the RAM instead of the internal ROM)
	bit 7 = lock timing

	bits 6-4 = Timing:
	000 or 001 = ZX 48K
	010 = ZX 128K
	011 = ZX +2/+3e
	100 = Pentagon 128K

	bit 3 = Reserved, must be 0

	bits 2-0 = Machine type:
	000 = Config mode
	001 = ZX 48K
	010 = ZX 128K
	011 = ZX +2/+3e
	100 = Pentagon 128K
	*/


                //z80_byte t=(tbblue_config1 >> 6)&3;
		z80_byte t=(tbblue_registers[3]>>4)&7;

		//TODO: otros timings

                if (t<=1) {
		//48k
				debug_printf (VERBOSE_INFO,"Apply config.timing. change:48k");
				tbblue_change_timing(0);
		}
		else {
		//128k
				debug_printf (VERBOSE_INFO,"Apply config.timing. change:128k");
				tbblue_change_timing(1);
		}




}


void tbblue_set_register_port(z80_byte value)
{
	tbblue_last_register=value;
}

void tbblue_set_value_port(z80_byte value)
{

	z80_byte last_register_5=tbblue_registers[5];
	z80_byte last_register_6=tbblue_registers[6];
	z80_byte last_register_7=tbblue_registers[7];
	z80_byte last_register_21=tbblue_registers[21];
	

	if (tbblue_last_register==3) {
		//Controlar caso especial
		//(W) 0x03 (03) => Set machine type, only in IPL or config mode
		//   		bits 2-0 = Machine type:
		//      		000 = Config mode
		z80_byte machine_type=tbblue_registers[3]&7;

		if (!(machine_type==0 || tbblue_bootrom.v)) {
			debug_printf(VERBOSE_DEBUG,"Can not change machine type (to %02XH) while in non config mode or non IPL mode",value);
			return;
		}
	}

	tbblue_registers[tbblue_last_register]=value;

	switch(tbblue_last_register)
	{

		case 2:
		/*
		(R/W)	02 => Reset:
					bits 7-3 = Reserved, must be 0
					bit 2 = (R) Power-on reset
					bit 1 = (R/W) if 1 Hard Reset
					bit 0 = (R/W) if 1 Soft Reset
					*/

						//tbblue_hardsoftreset=value;
						if (value&1) {
							//printf ("Doing soft reset due to writing to port 24D9H\n");
							reg_pc=0;
						}
						if (value&2) {
							//printf ("Doing hard reset due to writing to port 24D9H\n");
							tbblue_bootrom.v=1;
							//printf ("----setting bootrom to 1. when writing register 2 and bit 1\n");
							tbblue_registers[3]=0;
							//tbblue_config1=0;
							tbblue_set_memory_pages();
							reg_pc=0;
						}

					break;


		break;

		case 3:
		/*
		(W) 0x03 (03) => Set machine type, only in IPL or config mode:
   		A write in this register disables the IPL
   		(0x0000-0x3FFF are mapped to the RAM instead of the internal ROM)
   		bit 7 = lock timing
   		bits 6-4 = Timing:
      		000 or 001 = ZX 48K
      		010 = ZX 128K
      		011 = ZX +2/+3e
      		100 = Pentagon 128K
   		bit 3 = Reserved, must be 0
   		bits 2-0 = Machine type:
      		000 = Config mode
      		001 = ZX 48K
      		010 = ZX 128K
      		011 = ZX +2/+3e
      		100 = Pentagon 128K
      		*/

		/*  OLD:
				(W)		03 => Set machine type, only in bootrom or config mode:
							A write in this register disables the bootrom mode (0000 to 3FFF are mapped to the RAM instead of the internal ROM)
							bits 7-5 = Reserved, must be 0
							bits 4-3 = Timing:
								00,
								01 = ZX 48K
								10 = ZX 128K
								11 = ZX +2/+3e
							bit 2 = Reserved, must be 0
							bits 1-0 = Machine type:
								00 = Config mode (bootrom)
								01 = ZX 48K
								10 = ZX 128K
								11 = ZX +2/+3e
								*/
			//Pentagon not supported yet. TODO
			//last_value=tbblue_config1;
			tbblue_bootrom.v=0;
			//printf ("----setting bootrom to 0\n");

			//printf ("Writing register 3 value %02XH\n",value);

			tbblue_set_memory_pages();


			//Solo cuando hay cambio
			//if ( last_register_3 != value )
			tbblue_set_emulator_setting_timing();
		break;


		break;

		case 4:

/*
		(W)		04 => Set page RAM, only in config mode (no bootrom):
					bits 7-5 = Reserved, must be 0
					bits 4-0 = RAM page mapped in 0000-3FFF (32 pages of 16K = 512K)
			*/

			tbblue_set_memory_pages();

		break;

		case 5:
			//Si hay cambio en ulaplus
					/*
			(R/W)	05 => Peripheral 1 setting, only in bootrom or config mode:

						bit 3 = (W) Enable ULAplus (1 = enabled)

						*/

			if ( (last_register_5&8) != (value&8)) tbblue_set_emulator_setting_ulaplus();
		break;

		case 6:
			//Si hay cambio en DivMMC
			/*
			(W)		06 => Peripheral 2 setting, only in bootrom or config mode:

						bit 4 = Enable DivMMC (1 = enabled)
					*/
			if ( (last_register_6&16) != (value&16)) tbblue_set_emulator_setting_divmmc();
		break;


		case 7:
		/*
		(R/W)	07 => Turbo mode
					bit 0 = Turbo (0 = 3.5MHz, 1 = 7MHz)
					*/
					if ( last_register_7 != value ) tbblue_set_emulator_setting_turbo();
		break;

		case 21:
			//modo lores
			if ( (last_register_21&128) != (value&128)) {
				if (value&128) screen_print_splash_text(10,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,"Enabling lores video mode. 128x96 256 colours");
				else screen_print_splash_text(10,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,"Disabling lores video mode");
			}
		break;


		//(R/W) 0x41 (65) => Palette Value
		case 65:
			tbblue_write_palette_value_high8(value);
		break;

		// (R/W) 0x44 (68) => Palette Lower bit
		case 68:
			tbblue_write_palette_value_low1(value);
		break;

	}


}

int tbblue_get_raster_line(void)
{
	/*
	Line 0 is first video line. In truth the line is the Y counter, Video is from 0 to 191, borders and hsync is >192
Same this page: http://www.zxdesign.info/vertcontrol.shtml


Row	Start	Row End	Length	Description
0		191	192	Video Display
192	247	56	Bottom Border
248	255	8	Vertical Sync
256	312	56	Top Border

*/
	if (t_scanline>=screen_indice_inicio_pant) return t_scanline-screen_indice_inicio_pant;
	else return t_scanline+192+screen_total_borde_inferior;


}


z80_byte tbblue_get_value_port_register(z80_byte registro)
{

	int linea_raster;

	//Casos especiales
	/*
	(R) 0x00 (00) => Machine ID

	(R) 0x01 (01) => Version (Nibble most significant = Major, Nibble less significant = Minor)
	*/
	switch(registro)
	{
		case 0:
			return 8;
		break;

		case 1:
			return 0x19;
		break;

		/*




		(R) 0x1E (30) => Active video line (MSB)
  bits 7-1 = Reserved, always 0
  bit 0 = Active line MSB (Reset to 0 after a reset)

(R) 0x1F (31) = Active video line (LSB)
  bits 7-0 = Active line LSB (0-255)(Reset to 0 after a reset)
		*/

		case 30:
			linea_raster=tbblue_get_raster_line();
			linea_raster=linea_raster >> 8;
			return (linea_raster&1);
		break;

		case 31:
			linea_raster=tbblue_get_raster_line();
			return (linea_raster&0xFF);
		break;




	}


	return tbblue_registers[registro];
}



z80_byte tbblue_get_value_port(void)
{
	return tbblue_get_value_port_register(tbblue_last_register);
}



//Devuelve puntero a direccion de memoria donde esta el scanline en modo lores para direccion y
z80_byte *get_lores_pointer(int y)
{
	z80_byte *base_pointer;

	//Siempre saldra de ram 5
	base_pointer=tbblue_ram_memory_pages[5*2];	

	//128x96 one byte per pixel in left to right, top to bottom order so that the 
	//top half of the screen is in the first timex display file at 0x4000 
	//and the bottom half is in the second timex display file at 0x6000
	
	z80_int offset=0;

	//int yorig=y;

	const int mitad_alto=96/2;

	//Segunda mitad
	if (y>=mitad_alto) {
		//printf ("segundo bloque. y=%d offset=%d\n",y,offset);
		offset +=0x2000;
		y=y-mitad_alto;
	}

	//Sumamos desplazamiento por y
	offset +=y*128;

	//printf ("y: %d offset: %d\n",yorig,offset);

	base_pointer +=offset;

	return base_pointer;
}



//Inicializa punteros a los 3 layers
z80_int *p_layer_first;
z80_int *p_layer_second;
z80_int *p_layer_third;

void tbblue_set_layer_priorities(void)
{
	//Por defecto
	//sprites over the Layer 2, over the ULA graphics
	p_layer_first=tbblue_layer_sprites;
	p_layer_second=tbblue_layer_layer2;
	p_layer_third=tbblue_layer_ula;

	/*
	(R/W) 0x15 (21) => Sprite and Layers system
  bit 7 - LoRes mode, 128 x 96 x 256 colours (1 = enabled)
  bits 6-5 = Reserved, must be 0
  bits 4-2 = set layers priorities:
     Reset default is 000, sprites over the Layer 2, over the ULA graphics
     000 - S L U
     001 - L S U
     010 - S U L
     011 - L U S
     100 - U S L
     101 - U L S
  bit 1 = Over border (1 = yes)(Back to 0 after a reset)
  bit 0 = Sprites visible (1 = visible)(Back to 0 after a reset)
  */
	z80_byte prio=(tbblue_registers[0x15] >> 2)&7;

	switch (prio) {
		case 0:
			p_layer_first=tbblue_layer_sprites;
			p_layer_second=tbblue_layer_layer2;
			p_layer_third=tbblue_layer_ula;
		break;

		case 1:
			p_layer_first=tbblue_layer_layer2;
			p_layer_second=tbblue_layer_sprites;
			p_layer_third=tbblue_layer_ula;
		break;


		case 2:
			p_layer_first=tbblue_layer_sprites;
			p_layer_second=tbblue_layer_ula;
			p_layer_third=tbblue_layer_layer2;
		break;

		case 3:
			p_layer_first=tbblue_layer_layer2;
			p_layer_second=tbblue_layer_ula;
			p_layer_third=tbblue_layer_sprites;
		break;

		case 4:
			p_layer_first=tbblue_layer_ula;
			p_layer_second=tbblue_layer_sprites;
			p_layer_third=tbblue_layer_layer2;
		break;

		case 5:
			p_layer_first=tbblue_layer_ula;
			p_layer_second=tbblue_layer_layer2;
			p_layer_third=tbblue_layer_sprites;
		break;
	}

}


//Guardar en buffer rainbow la linea actual. Para Spectrum. solo display
//Tener en cuenta que si border esta desactivado, la primera linea del buffer sera de display,
//en cambio, si border esta activado, la primera linea del buffer sera de border
void screen_store_scanline_rainbow_solo_display_tbblue(void)
{

	//si linea no coincide con entrelazado, volvemos
	if (if_store_scanline_interlace(t_scanline_draw)==0) return;

	int i;
	for (i=0;i<512;i++) {
		tbblue_layer_ula[i]=TBBLUE_TRANSPARENT_REGISTER_9;
		tbblue_layer_layer2[i]=TBBLUE_TRANSPARENT_REGISTER_9;
		tbblue_layer_sprites[i]=TBBLUE_TRANSPARENT_REGISTER_9;
	}

	int bordesupinf=0;

  //En zona visible pantalla (no borde superior ni inferior)
  if (t_scanline_draw>=screen_indice_inicio_pant && t_scanline_draw<screen_indice_fin_pant) {







        //printf ("scan line de pantalla fisica (no border): %d\n",t_scanline_draw);

        //linea que se debe leer
        int scanline_copia=t_scanline_draw-screen_indice_inicio_pant;

        //la copiamos a buffer rainbow
        z80_int *puntero_buf_rainbow;
        //esto podria ser un contador y no hace falta que lo recalculemos cada vez. TODO
        int y;

        y=t_scanline_draw-screen_invisible_borde_superior;
        if (border_enabled.v==0) y=y-screen_borde_superior;

        puntero_buf_rainbow=&rainbow_buffer[ y*get_total_ancho_rainbow() ];

        puntero_buf_rainbow +=screen_total_borde_izquierdo*border_enabled.v;


        int x,bit;
        z80_int direccion;
	//z80_int dir_atributo;
        z80_byte byte_leido;


        int color=0;
        int fila;

        z80_byte attribute,bright,flash;
	z80_int ink,paper,aux;


        z80_byte *screen=get_base_mem_pantalla();

        direccion=screen_addr_table[(scanline_copia<<5)];

				//Inicializar puntero a layer2 de tbblue, irlo incrementando a medida que se ponen pixeles
				//Layer2 siempre se dibuja desde registro que indique pagina 18. Registro 19 es un backbuffer pero siempre se dibuja desde 18
				int tbblue_layer2_offset=tbblue_registers[18]&63;

				tbblue_layer2_offset*=16384;


				//Mantener el offset y en 0..255
				z80_byte tbblue_reg_23=tbblue_registers[23];
				tbblue_reg_23 +=scanline_copia;

				tbblue_layer2_offset +=tbblue_reg_23*256;

				z80_byte tbblue_reg_22=tbblue_registers[22];

/*
(R/W) 22 => Layer2 Offset X
  bits 7-0 = X Offset (0-255)(Reset to 0 after a reset)

(R/W) 23 => Layer2 Offset Y
  bist 7-0 = Y Offset (0-255)(Reset to 0 after a reset)
*/


        fila=scanline_copia/8;
        //dir_atributo=6144+(fila*32);


	z80_byte *puntero_buffer_atributos;


	//Si modo timex 512x192 pero se hace modo escalado
	//Si es modo timex 512x192, llamar a otra funcion
        if (timex_si_modo_512_y_zoom_par() ) {
                //Si zoom x par
                                        if (timex_mode_512192_real.v) {
                                                return;
                                        }
        }


	//temporal modo 6 timex 512x192 pero hacemos 256x192
	z80_byte temp_prueba_modo6[SCANLINEBUFFER_ONE_ARRAY_LENGTH];
	z80_byte col6;
	z80_byte tin6, pap6;

	z80_byte timex_video_mode=timex_port_ff&7;
	z80_byte timexhires_resultante;
	z80_int timexhires_origen;

	z80_bit si_timex_hires={0};

	//Por defecto
	puntero_buffer_atributos=scanline_buffer;

	/* modo lores
	(R/W) 0x15 (21) => Sprite and Layers system
  bit 7 - LoRes mode, 128 x 96 x 256 colours (1 = enabled)
  bits 6-5 = Reserved, must be 0
  	*/

  	int tbblue_lores=tbblue_registers[0x15] & 128;

  	z80_byte *lores_pointer;

  	if (tbblue_lores) {
  		lores_pointer=get_lores_pointer(scanline_copia/2);  //admite hasta y=95, dividimos entre 2 linea actual
  	}


	if (timex_video_emulation.v) {
		//Modos de video Timex
		/*
000 - Video data at address 16384 and 8x8 color attributes at address 22528 (like on ordinary Spectrum);

001 - Video data at address 24576 and 8x8 color attributes at address 30720;

010 - Multicolor mode: video data at address 16384 and 8x1 color attributes at address 24576;

110 - Extended resolution: without color attributes, even columns of video data are taken from address 16384, and odd columns of video data are taken from address 24576
		*/
		switch (timex_video_mode) {

			case 4:
			case 6:
				//512x192 monocromo. aunque hacemos 256x192
				//y color siempre fijo
				/*
bits D3-D5: Selection of ink and paper color in extended screen resolution mode (000=black/white, 001=blue/yellow, 010=red/cyan, 011=magenta/green, 100=green/magenta, 101=cyan/red, 110=yellow/blue, 111=white/black); these bits are ignored when D2=0

				black, blue, red, magenta, green, cyan, yellow, white
				*/

				//Si D2==0, these bits are ignored when D2=0?? Modo 4 que es??

				//col6=(timex_port_ff>>3)&7;
				tin6=get_timex_ink_mode6_color();


				//Obtenemos color
				//tin6=col6;
				pap6=get_timex_paper_mode6_color();

				//Y con brillo
				col6=((pap6*8)+tin6)+64;

				//Nos inventamos un array de colores, con mismo color siempre, correspondiente a lo que dice el registro timex
				//Saltamos de dos en dos
				//De manera similar al buffer scanlines_buffer, hay pixel, atributo, pixel, atributo, etc
				//por eso solo llenamos la parte que afecta al atributo

				puntero_buffer_atributos=temp_prueba_modo6;
				int i;
				for (i=1;i<SCANLINEBUFFER_ONE_ARRAY_LENGTH;i+=2) {
					temp_prueba_modo6[i]=col6;
				}
				si_timex_hires.v=1;
			break;


		}
	}

	int posicion_array_layer=0;

	posicion_array_layer +=screen_total_borde_izquierdo*border_enabled.v;


	int posicion_array_pixeles_atributos=0;
        for (x=0;x<32;x++) {


                        //byte_leido=screen[direccion];
                        byte_leido=puntero_buffer_atributos[posicion_array_pixeles_atributos++];

			//Timex. Reducir 512x192 a 256x192.
			//Obtenemos los dos bytes implicados, metemos en variable de 16 bits,
			//Y vamos comprimiendo cada 2 pixeles. De cada 2 pixeles, si los dos son 0, metemos 0. Si alguno o los dos son 1, metemos 1
			//Esto es muy lento

			if (si_timex_hires.v) {

					//comprimir bytes
					timexhires_resultante=0;
					//timexhires_origen=byte_leido*256+screen[direccion+8192];
					timexhires_origen=screen[direccion]*256+screen[direccion+8192];

					//comprimir pixeles
					int i;
					for (i=0;i<8;i++) {
						timexhires_resultante=timexhires_resultante<<1;
						if ( (timexhires_origen&(32768+16384))   ) timexhires_resultante |=1;
						timexhires_origen=timexhires_origen<<2;
					}

					byte_leido=timexhires_resultante;

			}



                        attribute=puntero_buffer_atributos[posicion_array_pixeles_atributos++];


                                


			GET_PIXEL_COLOR

			int cambiada_tinta,cambiada_paper;

			cambiada_tinta=cambiada_paper=0;

                        for (bit=0;bit<8;bit++) {

				
				color= ( byte_leido & 128 ? ink : paper ) ;

				if (tbblue_lores) {
					

					z80_byte lorescolor=*lores_pointer;
					//tenemos indice color de paleta
					//transformar a color final segun paleta ula activa
					color=tbblue_get_palette_active_ula(lorescolor);

					//x lo incremento cuando bit es impar, para tener doble de ancho
					if (bit&1) lores_pointer++; 
				}

				//Si layer2 tbblue
				if (tbblue_is_active_layer2() ) {


						//Si layer2 encima
						/*if ( (tbblue_port_123b & 16)==0) {

							//Pixel en Layer2 no transparente. Mostramos pixel de layer2
							if (color_layer2!=TBBLUE_TRANSPARENT_COLOR) {
								store_value_rainbow(puntero_buf_rainbow,RGB9_INDEX_FIRST_COLOR+final_color_layer2);
							}

							//Pixel en layer2 transparente. Mostramos pixel normal de pantalla speccy
							else {
								store_value_rainbow(puntero_buf_rainbow,color);
							}
						}

						//Layer2 debajo
						else {
							z80_byte tbblue_color_transparente=tbblue_registers[20];

							//Pixel en pantalla speccy no transparente. Mostramos pixel normal de pantalla speccy
							if (color!=tbblue_color_transparente) {
								store_value_rainbow(puntero_buf_rainbow,color);
							}

							else {
								//Pixel en pantalla speccy transparente. Mostramos pixel de layer2
								store_value_rainbow(puntero_buf_rainbow,RGB9_INDEX_FIRST_COLOR+final_color_layer2);
							}
						}*/
				}


				else {
                                	//store_value_rainbow(puntero_buf_rainbow,color);

                                	//temp
                                	//tbblue_layer_ula[posicion_array_layer++]=color;
				}

				//Capa ula
				tbblue_layer_ula[posicion_array_layer]=tbblue_get_palette_active_ula(color);

				//Capa layer2
				if (tbblue_is_active_layer2()) {
					z80_byte color_layer2=memoria_spectrum[tbblue_layer2_offset+tbblue_reg_22];
					z80_int final_color_layer2=tbblue_get_palette_active_layer2(color_layer2);
					tbblue_layer_layer2[posicion_array_layer]=final_color_layer2;
				}

				posicion_array_layer++;

                                byte_leido=byte_leido<<1;


					//tbblue_layer2_offset++;
				tbblue_reg_22++;
                        }
			direccion++;
                	//dir_atributo++;


        	}




	}

	else {
		bordesupinf=1;
	}

	//Aqui puede ser borde superior o inferior
	//capa sprites
	tbsprite_do_overlay();


	//int i;

	int scanline_copia=t_scanline_draw-screen_indice_inicio_pant;

        //la copiamos a buffer rainbow
        z80_int *puntero_buf_rainbow;
        //esto podria ser un contador y no hace falta que lo recalculemos cada vez. TODO
        int y;

        y=t_scanline_draw-screen_invisible_borde_superior;
        if (border_enabled.v==0) y=y-screen_borde_superior;

	//puntero_buf_rainbow +=screen_total_borde_izquierdo*border_enabled.v;
	z80_int *puntero_final_rainbow=&rainbow_buffer[ y*get_total_ancho_rainbow() ];

	//Por defecto
	//sprites over the Layer 2, over the ULA graphics

	//p_layer_first=tbblue_layer_sprites;
	//p_layer_second=tbblue_layer_layer2;
	//p_layer_third=tbblue_layer_ula;

	tbblue_set_layer_priorities();




	z80_int color;
	//z80_int color_final=;

	for (i=0;i<get_total_ancho_rainbow();i++) {

		//Primera capa
		color=p_layer_first[i];
		if (!tbblue_si_transparent(color)) {
			*puntero_final_rainbow=RGB9_INDEX_FIRST_COLOR+color;
		}

		else {
			color=p_layer_second[i];
			if (!tbblue_si_transparent(color)) {
				*puntero_final_rainbow=RGB9_INDEX_FIRST_COLOR+color;
			}

			else {
				color=p_layer_third[i];
				if (!tbblue_si_transparent(color)) {
					*puntero_final_rainbow=RGB9_INDEX_FIRST_COLOR+color;
				}
				//TODO: que pasa si las tres capas son transparentes
				else {
					if (bordesupinf) {
					//Si estamos en borde inferior o superior, no hacemos nada, dibujar color borde
					}

					else {
						//Borde izquierdo o derecho o pantalla. Ver si estamos en pantalla
						if (i>=screen_total_borde_izquierdo*border_enabled.v &&
							i<screen_total_borde_izquierdo*border_enabled.v+256) {
							//Poner color negro
							*puntero_final_rainbow=RGB9_INDEX_FIRST_COLOR+0;
						}

						else {
						//Es borde. dejar ese color
						}
					
					}
				}
			}

		}

		puntero_final_rainbow++;

		
	}

}


