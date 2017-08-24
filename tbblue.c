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

//Punteros a los 32 bloques de 16kb de ram de spectrum
z80_byte *tbblue_ram_memory_pages[32];

//Punteros a los 4 bloques de 16kb de rom de spectrum
z80_byte *tbblue_rom_memory_pages[4];

z80_byte *tbblue_fpga_rom;

//Memoria mapedada
z80_byte *tbblue_memory_paged[4];


//Sprites

//Paleta de 256 colores formato RRRGGGBB
z80_byte tbsprite_palette[256];
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
	offset=tbblue_registers[18]&63;
	offset*=16384;

	return offset;
}

void tbblue_reset_sprites(void)
{
	//Inicializar Paleta
	int i;

	for (i=0;i<256;i++) tbsprite_palette[i]=i;

	//Resetear patterns todos a transparente
	for (i=0;i<TBBLUE_MAX_PATTERNS;i++) {
		int j;
		for (j=0;j<256;j++) {
			tbsprite_patterns[i][j]=TBBLUE_TRANSPARENT_COLOR;
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


void tbblue_out_port_sprite_index(z80_byte value)
{
	//printf ("Out tbblue_out_port_sprite_index %02XH\n",value);
	tbsprite_index_palette=tbsprite_index_pattern=tbsprite_index_sprite=value;

	tbsprite_index_pattern_subindex=tbsprite_index_sprite_subindex=0;
}

void tbblue_out_sprite_palette(z80_byte value)
{
	//printf ("Out tbblue_out_sprite_palette %02XH\n",value);

	tbsprite_palette[tbsprite_index_palette]=value;
	if (tbsprite_index_palette==255) tbsprite_index_palette=0;
	else tbsprite_index_palette++;
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



z80_byte sprite_line[MAX_X_SPRITE_LINE];




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

	//Si color transparente, no hacer nada
	if (color==TBBLUE_TRANSPARENT_COLOR) return;


	//Ver si habia un color y activar bit colision
	z80_byte color_antes=sprite_line[x];

	if (color_antes!=TBBLUE_TRANSPARENT_COLOR) {
		//colision
		tbblue_port_303b |=1;
		//printf ("set colision flag. result value: %d\n",tbblue_port_303b);
	}

	sprite_line[x]=color;

}

z80_byte tbsprite_do_overlay_get_pattern_xy(z80_byte index_pattern,z80_byte sx,z80_byte sy)
{

	return tbsprite_patterns[index_pattern][sy*TBBLUE_SPRITE_WIDTH+sx];
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
		    z80_int *puntero_buf_rainbow;
		    puntero_buf_rainbow=&rainbow_buffer[ rainbowy*get_total_ancho_rainbow() ];




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
				for (i=0;i<MAX_X_SPRITE_LINE;i++) {
					sprite_line[i]=TBBLUE_TRANSPARENT_COLOR;
				}

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
								z80_byte color=tbsprite_palette[index_color];

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

								tbsprite_put_color_line(sprite_x++,color,rangoxmin,rangoxmax);


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

			puntero_buf_rainbow +=screen_total_borde_izquierdo*border_enabled.v;

			puntero_buf_rainbow -=TBBLUE_SPRITE_BORDER;

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



			for (i=0;i<MAX_X_SPRITE_LINE;i++) {
				z80_byte color=sprite_line[i];

				if (i>=rangoxmin && i<=rangoxmax) {

					if (color!=TBBLUE_TRANSPARENT_COLOR) {

						//Pasamos de RGB a GRB
						/*z80_byte r,g,b;
						r=(color>>5)&7;
						g=(color>>2)&7;
						b=(color&3);

						z80_byte colorulaplus=(g<<5)|(r<<2)|b;


						//TODO conversion rgb. esto no es ulaplus. usamos tabla ulaplus solo para probar


						color_final=colorulaplus+ULAPLUS_INDEX_FIRST_COLOR;*/
						z80_int color_final;
						color_final=RGB8_INDEX_FIRST_COLOR+color;
						//color_final=ulaplus_rgb_table[color_final];

						*puntero_buf_rainbow=color_final;

					}
				}

				puntero_buf_rainbow++;

			}

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

	//32 Paginas RAM spectrum 512k. Hay 128kb mas despues de estos...
	for (i=0;i<32;i++) {
		indice=0x060000+16384*i;
		tbblue_ram_memory_pages[i]=&memoria_spectrum[indice];
	}

	//4 Paginas ROM
	for (i=0;i<4;i++) {
		indice=0+16384*i;
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


void tbblue_set_memory_pages(void)
{
	//Mapeamos paginas de RAM segun config1
	z80_byte maquina=(tbblue_registers[3])&3;

	int romram_page;
	int ram_page,rom_page;
	int indice;

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

			ram_page=tbblue_mem_get_ram_page();
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

                        ram_page=tbblue_mem_get_ram_page();
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
When the variable 'bootrom' takes '0', page 0 (0-16383) is mapped to the RAM 1024K,
and the page mapping is configured by bits 5-0 of the I/O port 'config1'.
These 6 bits maps 64 16K pages the start of 1024K SRAM space 0-16363 the Speccy,
which allows you access to all SRAM.
*/
				romram_page=(tbblue_registers[4]&63);
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
(W)		06 => Peripheral 2 setting, only in bootrom or config mode:
			bit 7 = Enable turbo mode (0 = disabled, 1 = enabled)
			bit 6 = DAC chip mode (0 = I2S, 1 = JAP)
			bit 5 = Enable Lightpen  (1 = enabled)
			bit 4 = Enable DivMMC (1 = enabled) -> divmmc automatic paging. divmmc memory is supported
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
	(R/W) 20 => Layer 2 transparency color
  bits 7-4 = Reserved, must be 0
  bits 3-0 = ULA transparency color (IGRB)(Reset to "1000" black with bright, after a reset)

	*/
	tbblue_registers[20]=8;

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

	tbblue_port_123b=0;

	tbblue_bootrom.v=1;
	tbblue_set_memory_pages();



	tbblue_set_emulator_setting_divmmc();
	tbblue_set_emulator_setting_ulaplus();

	tbblue_reset_sprites();

}


/*
void old_tbblue_out_port(z80_int port,z80_byte value)
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


//- Write in port 0x24DB (config1)
//
  //              case cpu_do(7 downto 6) is
    //                when "01"    => maquina <= s_speccy48;
  //                  when "10"    => maquina <= s_speccy128;
//                    when "11"    => maquina <= s_speccy3e;
//                    when others    => maquina <= s_config;   ---config mode
//                end case;


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
*/


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
- Write in port 0x24DB (config1)

                case cpu_do(7 downto 6) is
                    when "01"    => maquina <= s_speccy48;
                    when "10"    => maquina <= s_speccy128;
                    when "11"    => maquina <= s_speccy3e;
                    when others    => maquina <= s_config;   ---config mode
                end case;

*/


                //z80_byte t=(tbblue_config1 >> 6)&3;
								z80_byte t=(tbblue_registers[3])&3;
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

	z80_byte last_register_3=tbblue_registers[3];
	z80_byte last_register_5=tbblue_registers[5];
	z80_byte last_register_6=tbblue_registers[6];
	z80_byte last_register_7=tbblue_registers[7];

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
							tbblue_registers[3]=0;
							//tbblue_config1=0;
							tbblue_set_memory_pages();
							reg_pc=0;
						}

					break;


		break;

		case 3:

		/*
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

			//last_value=tbblue_config1;
			tbblue_bootrom.v=0;
			tbblue_set_memory_pages();


//- Write in port 0x24DB (config1)
//
	//              case cpu_do(7 downto 6) is
		//                when "01"    => maquina <= s_speccy48;
	//                  when "10"    => maquina <= s_speccy128;
//                    when "11"    => maquina <= s_speccy3e;
//                    when others    => maquina <= s_config;   ---config mode
//                end case;


			//Solo cuando hay cambio
			if ( (last_register_3&3) != (value&3) ) tbblue_set_emulator_setting_timing();
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
	(R)		00 => Machine ID
	(R)		01 => Version (Nibble most significant = Major, Nibble less significant = Minor)
	*/
	switch(registro)
	{
		case 0:
			return 8;
		break;

		case 1:
			return 0x16;
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


	return tbblue_registers[tbblue_last_register];
}



z80_byte tbblue_get_value_port(void)
{
	return tbblue_get_value_port_register(tbblue_last_register);
}
