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

#include <stdlib.h>
#include <stdio.h>

#include "scrnull.h"
#include "debug.h"
#include "screen.h"
#include "cpu.h"


void scrnull_z88_cpc_load_keymap(void)
{
	debug_printf (VERBOSE_INFO,"Loading keymap");
}


//Rutina de putchar para menu
void scrnull_putchar_menu(int x,int y, z80_byte caracter,z80_byte tinta,z80_byte papel)
{

        //Para evitar warnings al compilar de "unused parameter"
        x=y;
        y=x;
        caracter=tinta=papel;
        tinta=papel=caracter;


}

void scrnull_putchar_footer(int x,int y, z80_byte caracter,z80_byte tinta,z80_byte papel)
{

        //Para evitar warnings al compilar de "unused parameter"
        x=y;
        y=x;
        caracter=tinta=papel;
        tinta=papel=caracter;


}




void scrnull_set_fullscreen(void)
{
	debug_printf (VERBOSE_ERR,"Full screen mode not supported on this video driver");
}

void scrnull_reset_fullscreen(void)
{
	debug_printf (VERBOSE_ERR,"Full screen mode not supported on this video driver");
}

void scrnull_detectedchar_print(z80_byte caracter)
{
	//para que no se queje el compilador de variable no usada
	caracter++;
}


//Null video drivers
int scrnull_init (void){ 

debug_printf (VERBOSE_INFO,"Init Null Video Driver");

	scr_debug_registers=scrnull_debug_registers;
        scr_messages_debug=scrnull_messages_debug;

	scr_putchar_menu=scrnull_putchar_menu;
	scr_putchar_footer=scrnull_putchar_footer;

	scr_driver_name="null";

	scr_set_fullscreen=scrnull_set_fullscreen;
	scr_reset_fullscreen=scrnull_reset_fullscreen;
	scr_z88_cpc_load_keymap=scrnull_z88_cpc_load_keymap;
	scr_detectedchar_print=scrnull_detectedchar_print;


return 0; 

}

void scrnull_end(void)
{
debug_printf (VERBOSE_INFO,"Closing video driver");

}

void scrnull_refresca_pantalla_solo_driver(void)
{
        //Como esto solo lo uso de momento para drivers graficos, de momento lo dejo vacio
}


void scrnull_refresca_pantalla(void){}
z80_byte scrnull_lee_puerto(z80_byte puerto_h,z80_byte puerto_l){

        //Para evitar warnings al compilar de "unused parameter"
        puerto_h=puerto_l;
        puerto_l=puerto_h;


        return 255;
}
void scrnull_actualiza_tablas_teclado(void){}
void scrnull_debug_registers(void){


char buffer[2048];

print_registers(buffer);

printf ("%s\r",buffer);

}



void scrnull_messages_debug(char *s)
{
        printf ("%s\n",s);
}



