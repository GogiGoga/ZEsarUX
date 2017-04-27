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

/*

This is a template for a video driver. It can be used as an example for a graphical video driver,
like xwindows or fbdev. Some other drivers, like curses or cacalib works in a different way

Replace:
VIDEONAME_CAP with the video driver name in capital letters, like "XWINDOWS"
videoname with the video driver name in lowercase letters, like "xwindows"

*/



#include "scrvideoname.h"

#include "compileoptions.h"
#include "cpu.h"
#include "screen.h"
#include "charset.h"
#include "debug.h"
#include "menu.h"
#include "cpc.h"

#include <string.h>
#include <stdio.h>


void scrvideoname_putpixel(int x,int y,unsigned int color)
{
        //Putpixel Call (x,y,spectrum_colortable[color]);
}


void scrvideoname_putchar_zx8081(int x,int y, z80_byte caracter)
{
        scr_putchar_zx8081_comun(x,y, caracter);
}

void scrvideoname_debug_registers(void)
{

	char buffer[2048];
	print_registers(buffer);

	printf ("%s\r",buffer);
}

void scrvideoname_messages_debug(char *s)
{
        printf ("%s\n",s);
        //flush de salida standard. normalmente no hace falta esto, pero si ha finalizado el driver curses, deja la salida que no hace flush
        fflush(stdout);

}

//Rutina de putchar para menu
void scrvideoname_putchar_menu(int x,int y, z80_byte caracter,z80_byte tinta,z80_byte papel)
{

        z80_bit inverse,f;

        inverse.v=0;
        f.v=0;
        //128 y 129 corresponden a franja de menu y a letra enye minuscula
        if (caracter<32 || caracter>MAX_CHARSET_GRAPHIC) caracter='?';
        scr_putsprite_comun(&char_set[(caracter-32)*8],x,y,inverse,tinta,papel,f);

}

//Rutina de putchar para footer window
void scrvideoname_putchar_footer(int x,int y, z80_byte caracter,z80_byte tinta,z80_byte papel) {


        int yorigen;


	yorigen=screen_get_emulated_display_height_no_zoom_bottomborder_en()/8;

/*
        if (MACHINE_IS_Z88) yorigen=24;

	else if (MACHINE_IS_CPC) {
                yorigen=(CPC_DISPLAY_HEIGHT/8);
                if (border_enabled.v) yorigen+=CPC_TOP_BORDER_NO_ZOOM/8;
        }

	else if (MACHINE_IS_PRISM) {
                yorigen=(PRISM_DISPLAY_HEIGHT/8);
                if (border_enabled.v) yorigen+=PRISM_TOP_BORDER_NO_ZOOM/8;
        }

        else {
                //Spectrum o ZX80/81
                if (border_enabled.v) yorigen=31;
                else yorigen=24;
        }

*/

        scr_putchar_menu(x,yorigen+y,caracter,tinta,papel);
}


void scrvideoname_set_fullscreen(void)
{

}


void scrvideoname_reset_fullscreen(void)
{

}


void scrvideoname_refresca_pantalla_zx81(void)
{

        scr_refresca_pantalla_y_border_zx8081();

}


void scrvideoname_refresca_border(void)
{
        scr_refresca_border();

}


//Common routine for a graphical driver
void scrvideoname_refresca_pantalla(void)
{

        if (scr_si_color_oscuro() ) {
                //printf ("color oscuro\n");
                spectrum_colortable=spectrum_colortable_oscuro;

                //esto invalida la cache y por tanto ralentizando el refresco de pantalla
                //clear_putpixel_cache();
        }



        if (MACHINE_IS_ZX8081) {


                //scr_refresca_pantalla_rainbow_comun();
                scrvideoname_refresca_pantalla_zx81();
        }

        else if (MACHINE_IS_SPECTRUM) {


                //modo clasico. sin rainbow
                if (rainbow_enabled.v==0) {
                        if (border_enabled.v) {
                                //ver si hay que refrescar border
                                if (modificado_border.v)
                                {
                                        scrvideoname_refresca_border();
                                        modificado_border.v=0;
                                }

                        }

                        scr_refresca_pantalla_comun();
                }

                else {
                //modo rainbow - real video
                        scr_refresca_pantalla_rainbow_comun();
                }
        }

        else if (MACHINE_IS_Z88) {
                screen_z88_refresca_pantalla();
        }


        else if (MACHINE_IS_ACE) {
                scr_refresca_pantalla_y_border_ace();
        }


	else if (MACHINE_IS_CPC) {
                scr_refresca_pantalla_y_border_cpc();
        }


        //printf ("%d\n",spectrum_colortable[1]);

        if (menu_overlay_activo) {
                //printf ("color claro\n");
                spectrum_colortable=spectrum_colortable_normal;
                //clear_putpixel_cache();
                menu_overlay_function();
        }


        //Escribir footer
        draw_footer();

}


void scrvideoname_end(void)
{
}

z80_byte scrvideoname_lee_puerto(z80_byte puerto_h,z80_byte puerto_l)
{
	return 255;
}

void scrvideoname_actualiza_tablas_teclado(void)
{

	//Si se pulsa tecla y en modo z88, notificar interrupcion
	//if (pressrelease) notificar_tecla_interrupcion_si_z88();

}


void scrvideoname_z88_cpc_load_keymap(void)
{
	debug_printf (VERBOSE_INFO,"Loading keymap");
}

void scrvideoname_detectedchar_print(z80_byte caracter)
{
        printf ("%c",caracter);
        //flush de salida standard
        fflush(stdout);

}

int scrvideoname_init (void) {

	debug_printf (VERBOSE_INFO,"Init VIDEONAME_CAP Video Driver");


        //Inicializaciones necesarias
        scr_putpixel=scrvideoname_putpixel;
        scr_putchar_zx8081=scrvideoname_putchar_zx8081;
        scr_debug_registers=scrvideoname_debug_registers;
        scr_messages_debug=scrvideoname_messages_debug;
        scr_putchar_menu=scrvideoname_putchar_menu;
	scr_putchar_footer=scrvideoname_putchar_footer;
        scr_set_fullscreen=scrvideoname_set_fullscreen;
        scr_reset_fullscreen=scrvideoname_reset_fullscreen;
	scr_z88_cpc_load_keymap=scrvideoname_z88_cpc_load_keymap;
	scr_detectedchar_print=scrvideoname_detectedchar_print;
        scr_tiene_colores=1;
        screen_refresh_menu=1;


        //Otra inicializacion necesaria
        //Esto debe estar al final, para que funcione correctamente desde menu, cuando se selecciona un driver, y no va, que pueda volver al anterior
        scr_driver_name="videoname";

	//importante: modificar funcion int si_complete_video_driver(void) de screen.c si este driver es completo, agregarlo ahi
	//importante: definicion de f_functions en menu.c, si driver permite teclas F


	scr_z88_cpc_load_keymap();

        return 0;

}


