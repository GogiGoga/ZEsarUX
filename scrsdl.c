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

#include <string.h>
#include <stdio.h>

#if defined(__APPLE__)
        #include <SDL.h>
#else
	#include <SDL/SDL.h>
#endif



#include "scrsdl.h"
#include "common_sdl.h"

#include "compileoptions.h"
#include "cpu.h"
#include "screen.h"
#include "charset.h"
#include "debug.h"
#include "menu.h"
#include "utils.h"
#include "joystick.h"
#include "cpc.h"
#include "prism.h"
#include "sam.h"
#include "ql.h"



SDL_Surface *sdl_screen;


//Indica que ha habido un evento de resize y queda pendiente redimensionar la ventana
//lo que sucede es que cuando se empieza a redimensionar una ventana, usando el raton,
//al recrear la ventana con el nuevo tamanyo, realmente lo que hace es cambiar el buffer de video interno de la ventana
//con el nuevo tamanyo, pero realmente el tamanyo de la ventana NO lo cambia, sino que queda siempre forzado con el tamanyo
//que estamos indicando con el raton
//Por tanto, lo que hacemos es que, activamos este indicador de redimensionar, y cuando hay un siguiente evento diferente de resize,
//la ventana se redimensiona
int scrsdl_debe_redimensionar=0;

int scrsdl_crea_ventana(void)
{

        Uint32 flags;

        flags=SDL_SWSURFACE | SDL_RESIZABLE;

        if (ventana_fullscreen) {
		flags |=SDL_FULLSCREEN;
	}


	debug_printf (VERBOSE_DEBUG,"Creating window %d X %d",screen_get_window_size_width_zoom_border_en(),screen_get_window_size_height_zoom_border_en() );

        sdl_screen = SDL_SetVideoMode(screen_get_window_size_width_zoom_border_en(),
                                      screen_get_window_size_height_zoom_border_en(),
                                        32, flags);
        if ( sdl_screen == NULL ) {
                return 1;
        }


        //establecer titulo ventana
        SDL_WM_SetCaption("ZEsarUX " EMULATOR_VERSION , "ZEsarUX");

	if (mouse_pointer_shown.v==0) SDL_ShowCursor(0);

	modificado_border.v=1;

	screen_z88_draw_lower_screen();

	menu_init_footer();



        return 0;

}



void scrsdl_putpixel(int x,int y,unsigned int color)
{

		Uint8 *p = (Uint8 *)sdl_screen->pixels + y * sdl_screen->pitch + x * 4;


                //escribir de golpe los 32 bits.
                unsigned int color32=spectrum_colortable[color];
                //agregar alpha
                color32 |=0xFF000000;
                //y escribir

		*(Uint32 *)p = color32;
}


void scrsdl_putchar_zx8081(int x,int y, z80_byte caracter)
{
        scr_putchar_zx8081_comun(x,y, caracter);
}

void scrsdl_debug_registers(void)
{

	char buffer[2048];
	print_registers(buffer);

	printf ("%s\r",buffer);
}

void scrsdl_messages_debug(char *s)
{
        printf ("%s\n",s);
        //flush de salida standard. normalmente no hace falta esto, pero si ha finalizado el driver curses, deja la salida que no hace flush
        fflush(stdout);

}

//Rutina de putchar para menu
void scrsdl_putchar_menu(int x,int y, z80_byte caracter,z80_byte tinta,z80_byte papel)
{

        z80_bit inverse,f;

        inverse.v=0;
        f.v=0;
        //128 y 129 corresponden a franja de menu y a letra enye minuscula
        if (caracter<32 || caracter>MAX_CHARSET_GRAPHIC) caracter='?';
        scr_putsprite_comun(&char_set[(caracter-32)*8],x,y,inverse,tinta,papel,f);

}

void scrsdl_putchar_footer(int x,int y, z80_byte caracter,z80_byte tinta,z80_byte papel) {


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

        else if (MACHINE_IS_SAM) {
                yorigen=(SAM_DISPLAY_HEIGHT/8);
                if (border_enabled.v) yorigen+=SAM_TOP_BORDER_NO_ZOOM/8;
        }

        else if (MACHINE_IS_QL) {
                yorigen=(QL_DISPLAY_HEIGHT/8);
                if (border_enabled.v) yorigen+=QL_TOP_BORDER_NO_ZOOM/8;
        }


        else {
                //Spectrum o ZX80/81
                if (border_enabled.v) yorigen=31;
                else yorigen=24;
        }

*/

        scr_putchar_menu(x,yorigen+y,caracter,tinta,papel);
}



void scrsdl_set_fullscreen(void)
{

	ventana_fullscreen=1;
	scrsdl_crea_ventana();

}


void scrsdl_reset_fullscreen(void)
{

	ventana_fullscreen=0;
	scrsdl_crea_ventana();

}


void scrsdl_refresca_pantalla_zx81(void)
{

        scr_refresca_pantalla_y_border_zx8081();

}


void scrsdl_refresca_border(void)
{
        scr_refresca_border();

}

void scrsdl_refresca_pantalla_solo_driver(void)
{
        SDL_UpdateRect(sdl_screen, 0, 0, screen_get_window_size_width_zoom_border_en(), screen_get_window_size_height_zoom_border_en() );


        /* UnLock the screen for direct access to the pixels */
        if ( SDL_MUSTLOCK(sdl_screen) ) {
                SDL_UnlockSurface(sdl_screen);
        }

}


//Common routine for a graphical driver
void scrsdl_refresca_pantalla(void)
{

        /* Lock the screen for direct access to the pixels */
	//Aunque en mis pruebas esta funcion dice que la pantalla no necesita hacer lock
        if ( SDL_MUSTLOCK(sdl_screen) ) {
		//printf ("locking screen\n");
                if ( SDL_LockSurface(sdl_screen) < 0 ) {
                        cpu_panic ("scr sdl can't lock screen");
                }
        }


        if (scr_si_color_oscuro() ) {
                //printf ("color oscuro\n");
                spectrum_colortable=spectrum_colortable_oscuro;

                //esto invalida la cache y por tanto ralentizando el refresco de pantalla
                //clear_putpixel_cache();
        }



        if (MACHINE_IS_ZX8081) {


                //scr_refresca_pantalla_rainbow_comun();
                scrsdl_refresca_pantalla_zx81();
        }

        else if (MACHINE_IS_PRISM) {
                screen_prism_refresca_pantalla();
        }


        else if (MACHINE_IS_SPECTRUM) {


                //modo clasico. sin rainbow
                if (rainbow_enabled.v==0) {
                        if (border_enabled.v) {
                                //ver si hay que refrescar border
                                if (modificado_border.v)
                                {
                                        scrsdl_refresca_border();
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

        else if (MACHINE_IS_SAM) {
                scr_refresca_pantalla_y_border_sam();
        }

        else if (MACHINE_IS_QL) {
                scr_refresca_pantalla_y_border_ql();
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

	scrsdl_refresca_pantalla_solo_driver();

}


void scrsdl_end(void)
{
	debug_printf (VERBOSE_INFO,"Closing SDL video driver");
	scrsdl_inicializado.v=0;
	commonsdl_end();
}

z80_byte scrsdl_lee_puerto(z80_byte puerto_h,z80_byte puerto_l)
{

        //Para evitar warnings al compilar de "unused parameter"
        puerto_h=puerto_l;
        puerto_l=puerto_h;

	return 255;
}

//Teclas de Z88 asociadas a cada tecla del teclado fisico
int scrsdl_keymap_z88_cpc_minus;		//Tecla a la derecha del 0
int scrsdl_keymap_z88_cpc_equal;		//Tecla a la derecha de la anterior
int scrsdl_keymap_z88_cpc_backslash;		//Tecla a la derecha de la anterior
int scrsdl_keymap_z88_cpc_bracket_left;		//Tecla a la derecha de la P
int scrsdl_keymap_z88_cpc_bracket_right;	//Tecla a la derecha de la anterior
int scrsdl_keymap_z88_cpc_semicolon;		//Tecla a la derecha de la L
int scrsdl_keymap_z88_cpc_apostrophe;		//Tecla a la derecha de la anterior
int scrsdl_keymap_z88_cpc_pound;		//Tecla a la derecha de la anterior
int scrsdl_keymap_z88_cpc_comma;		//Tecla a la derecha de la M
int scrsdl_keymap_z88_cpc_period;		//Tecla a la derecha de la anterior
int scrsdl_keymap_z88_cpc_slash;		//Tecla a la derecha de la anterior

int scrsdl_keymap_z88_cpc_circunflejo;		//Solo en CPC. Tecla a la derecha del 0. Misma que scrsdl_keymap_z88_cpc_equal
int scrsdl_keymap_z88_cpc_colon;		//Solo en CPC. Tecla a la derecha de la L. Misma que scrsdl_keymap_z88_cpc_semicolon
int scrsdl_keymap_z88_cpc_arroba;		//Solo en CPC. Tecla a la derecha de la P. Misma que scrsdl_keymap_z88_cpc_bracket_right

int scrsdl_keymap_z88_cpc_leftz;		//Tecla a la izquierda de la Z

//valores para estas teclas que vienen con compose, valores que me invento
#define SDLK_COMPOSE_BACKQUOTE (16384+34)
#define SDLK_COMPOSE_APOSTROPHE (16384+48)


void scrsdl_z88_cpc_load_keymap(void)
{
	debug_printf (VERBOSE_INFO,"Loading keymap");

        //Teclas se ubican en misma disposicion fisica del Z88, excepto:
        //libra~ -> spanish: cedilla (misma ubicacion fisica del z88). english: acento grave (supuestamente a la izquierda del 1)
        //backslash: en english esta en fila inferior del z88. en spanish, lo ubicamos a la izquierda del 1 (ºª\)
        switch (z88_cpc_keymap_type) {

                case 1:
			if (MACHINE_IS_Z88 || MACHINE_IS_SAM || MACHINE_IS_QL) {
	                        scrsdl_keymap_z88_cpc_minus=SDLK_QUOTE;
        	                scrsdl_keymap_z88_cpc_equal=SDLK_WORLD_1;
                	        scrsdl_keymap_z88_cpc_backslash=SDLK_WORLD_26;

	                        scrsdl_keymap_z88_cpc_bracket_left=SDLK_COMPOSE_BACKQUOTE;
        	                scrsdl_keymap_z88_cpc_bracket_right=SDLK_PLUS;
                	        scrsdl_keymap_z88_cpc_semicolon=SDLK_WORLD_81;
	                        scrsdl_keymap_z88_cpc_apostrophe=SDLK_COMPOSE_APOSTROPHE;
        	                scrsdl_keymap_z88_cpc_pound=SDLK_WORLD_71;
                	        scrsdl_keymap_z88_cpc_comma=SDLK_COMMA;
                        	scrsdl_keymap_z88_cpc_period=SDLK_PERIOD;
	                        scrsdl_keymap_z88_cpc_slash=SDLK_MINUS;
	                        scrsdl_keymap_z88_cpc_leftz=SDLK_LESS;
			}

			else if (MACHINE_IS_CPC) {
				scrsdl_keymap_z88_cpc_minus=SDLK_QUOTE;
	                        scrsdl_keymap_z88_cpc_circunflejo=SDLK_WORLD_1;

        	                scrsdl_keymap_z88_cpc_arroba=SDLK_COMPOSE_BACKQUOTE;
                	        scrsdl_keymap_z88_cpc_bracket_left=SDLK_PLUS;
	                        scrsdl_keymap_z88_cpc_colon=SDLK_WORLD_81;
        	                scrsdl_keymap_z88_cpc_semicolon=SDLK_COMPOSE_APOSTROPHE;
                	        scrsdl_keymap_z88_cpc_bracket_right=SDLK_WORLD_71;
                        	scrsdl_keymap_z88_cpc_comma=SDLK_COMMA;
	                        scrsdl_keymap_z88_cpc_period=SDLK_PERIOD;
        	                scrsdl_keymap_z88_cpc_slash=SDLK_MINUS;

                	        scrsdl_keymap_z88_cpc_backslash=SDLK_WORLD_26;
	                        scrsdl_keymap_z88_cpc_leftz=SDLK_LESS;

			}

			else if (MACHINE_IS_SPECTRUM && chloe_keyboard.v) {
                                scrsdl_keymap_z88_cpc_minus=SDLK_QUOTE;
                                scrsdl_keymap_z88_cpc_equal=SDLK_WORLD_1;
                                scrsdl_keymap_z88_cpc_backslash=SDLK_WORLD_26;

                                scrsdl_keymap_z88_cpc_bracket_left=SDLK_COMPOSE_BACKQUOTE;
                                scrsdl_keymap_z88_cpc_bracket_right=SDLK_PLUS;
                                scrsdl_keymap_z88_cpc_semicolon=SDLK_WORLD_81;
                                scrsdl_keymap_z88_cpc_apostrophe=SDLK_COMPOSE_APOSTROPHE;
                                scrsdl_keymap_z88_cpc_pound=SDLK_WORLD_71;
                                scrsdl_keymap_z88_cpc_comma=SDLK_COMMA;
                                scrsdl_keymap_z88_cpc_period=SDLK_PERIOD;
                                scrsdl_keymap_z88_cpc_slash=SDLK_MINUS;
	                        scrsdl_keymap_z88_cpc_leftz=SDLK_LESS;
                        }

                break;


                default:
                        //0 o default
			if (MACHINE_IS_Z88 || MACHINE_IS_SAM) {
	                        scrsdl_keymap_z88_cpc_minus=SDLK_MINUS;
        	                scrsdl_keymap_z88_cpc_equal=SDLK_EQUALS;
                	        scrsdl_keymap_z88_cpc_backslash=SDLK_BACKSLASH;

                        	scrsdl_keymap_z88_cpc_bracket_left=SDLK_LEFTBRACKET;
	                        scrsdl_keymap_z88_cpc_bracket_right=SDLK_RIGHTBRACKET;
        	                scrsdl_keymap_z88_cpc_semicolon=SDLK_SEMICOLON;
                	        scrsdl_keymap_z88_cpc_apostrophe=SDLK_QUOTE;
                        	scrsdl_keymap_z88_cpc_pound=SDLK_BACKQUOTE;
	                        scrsdl_keymap_z88_cpc_comma=SDLK_COMMA;
        	                scrsdl_keymap_z88_cpc_period=SDLK_PERIOD;
                	        scrsdl_keymap_z88_cpc_slash=SDLK_SLASH;
	                        scrsdl_keymap_z88_cpc_leftz=SDLK_LESS;
			}

			else if (MACHINE_IS_CPC) {
				scrsdl_keymap_z88_cpc_minus=SDLK_MINUS;
                                scrsdl_keymap_z88_cpc_circunflejo=SDLK_EQUALS;

                                scrsdl_keymap_z88_cpc_arroba=SDLK_LEFTBRACKET;
                                scrsdl_keymap_z88_cpc_bracket_left=SDLK_RIGHTBRACKET;
                                scrsdl_keymap_z88_cpc_colon=SDLK_SEMICOLON;
                                scrsdl_keymap_z88_cpc_semicolon=SDLK_QUOTE;
                                scrsdl_keymap_z88_cpc_bracket_right=SDLK_BACKQUOTE;
                                scrsdl_keymap_z88_cpc_comma=SDLK_COMMA;
                                scrsdl_keymap_z88_cpc_period=SDLK_PERIOD;
                                scrsdl_keymap_z88_cpc_slash=SDLK_SLASH;

                                scrsdl_keymap_z88_cpc_backslash=SDLK_BACKSLASH;
	                        scrsdl_keymap_z88_cpc_leftz=SDLK_LESS;
			}

			else if (MACHINE_IS_SPECTRUM && chloe_keyboard.v) {
                                scrsdl_keymap_z88_cpc_minus=SDLK_MINUS;
                                scrsdl_keymap_z88_cpc_equal=SDLK_EQUALS;
                                scrsdl_keymap_z88_cpc_backslash=SDLK_BACKSLASH;

                                scrsdl_keymap_z88_cpc_bracket_left=SDLK_LEFTBRACKET;
                                scrsdl_keymap_z88_cpc_bracket_right=SDLK_RIGHTBRACKET;
                                scrsdl_keymap_z88_cpc_semicolon=SDLK_SEMICOLON;
                                scrsdl_keymap_z88_cpc_apostrophe=SDLK_QUOTE;
                                scrsdl_keymap_z88_cpc_pound=SDLK_BACKQUOTE;
                                scrsdl_keymap_z88_cpc_comma=SDLK_COMMA;
                                scrsdl_keymap_z88_cpc_period=SDLK_PERIOD;
                                scrsdl_keymap_z88_cpc_slash=SDLK_SLASH;
	                        scrsdl_keymap_z88_cpc_leftz=SDLK_LESS;
                        }

                break;
        }
}




void scrsdl_deal_keys(int pressrelease,int tecla)
{

	//printf ("scrsdl_deal_keys tecla: %d 0x%X pressrelease: %d\n",tecla,tecla,pressrelease);
	//printf ("tecla: 0x%X\n",tecla);

        //Teclas que necesitan conversion de teclado para Chloe
        int tecla_gestionada_chloe=0;
        if (MACHINE_IS_SPECTRUM && chloe_keyboard.v) {
                        tecla_gestionada_chloe=1;

                        if (tecla==scrsdl_keymap_z88_cpc_minus) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_MINUS,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_equal) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_EQUAL,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_backslash) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_BACKSLASH,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_bracket_left) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_BRACKET_LEFT,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_bracket_right) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_BRACKET_RIGHT,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_semicolon) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_SEMICOLON,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_apostrophe) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_APOSTROPHE,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_pound) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_POUND,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_comma) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_COMMA,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_period) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_PERIOD,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_slash) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_SLASH,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_leftz) util_set_reset_key_chloe_keymap(UTIL_KEY_CHLOE_LEFTZ,pressrelease);

                        else tecla_gestionada_chloe=0;
        }


        if (tecla_gestionada_chloe) return;



        int tecla_gestionada_sam_ql=0;
        if (MACHINE_IS_SAM || MACHINE_IS_QL) {
                tecla_gestionada_sam_ql=1;

                        if (tecla==scrsdl_keymap_z88_cpc_minus) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_MINUS,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_equal) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_EQUAL,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_backslash) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_BACKSLASH,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_bracket_left) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_BRACKET_LEFT,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_bracket_right) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_BRACKET_RIGHT,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_semicolon) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_SEMICOLON,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_apostrophe) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_APOSTROPHE,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_pound) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_POUND,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_comma) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_COMMA,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_period) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_PERIOD,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_slash) util_set_reset_key_common_keymap(UTIL_KEY_COMMON_KEYMAP_SLASH,pressrelease);


                else tecla_gestionada_sam_ql=0;
        }

        if (tecla_gestionada_sam_ql) return;



        switch (tecla) {

                        case 32:
                                util_set_reset_key(UTIL_KEY_SPACE,pressrelease);
                        break;

                        case SDLK_RETURN:
                                util_set_reset_key(UTIL_KEY_ENTER,pressrelease);
                        break;

                        case SDLK_LSHIFT:
                                util_set_reset_key(UTIL_KEY_SHIFT_L,pressrelease);
                        break;

                        case SDLK_RSHIFT:
                                util_set_reset_key(UTIL_KEY_SHIFT_R,pressrelease);
                        break;

                        case SDLK_LALT:
                                util_set_reset_key(UTIL_KEY_ALT_L,pressrelease);
                        break;
                        case SDLK_RALT:
                                util_set_reset_key(UTIL_KEY_ALT_R,pressrelease);
                        break;


                        case SDLK_LCTRL:
                                util_set_reset_key(UTIL_KEY_CONTROL_L,pressrelease);
                        break;

                        case SDLK_RCTRL:
                                util_set_reset_key(UTIL_KEY_CONTROL_R,pressrelease);
                        break;

                        case SDLK_LSUPER:
                        case SDLK_RSUPER:
                                util_set_reset_key(UTIL_KEY_WINKEY,pressrelease);
                        break;



                        //Teclas que generan doble pulsacion
                        case SDLK_BACKSPACE:
                                util_set_reset_key(UTIL_KEY_BACKSPACE,pressrelease);
                        break;

                        case SDLK_HOME:
                                util_set_reset_key(UTIL_KEY_HOME,pressrelease);
                        break;

                        case SDLK_LEFT:
                                util_set_reset_key(UTIL_KEY_LEFT,pressrelease);
                        break;

                        case SDLK_RIGHT:
                                util_set_reset_key(UTIL_KEY_RIGHT,pressrelease);
                        break;

                        case SDLK_DOWN:
                                util_set_reset_key(UTIL_KEY_DOWN,pressrelease);
                        break;

                        case SDLK_UP:
                                util_set_reset_key(UTIL_KEY_UP,pressrelease);
                        break;

                        case SDLK_TAB:
                                util_set_reset_key(UTIL_KEY_TAB,pressrelease);
                        break;

                        case SDLK_CAPSLOCK:
                                util_set_reset_key(UTIL_KEY_CAPS_LOCK,pressrelease);
                        break;

                        case SDLK_COMMA:
                                util_set_reset_key(UTIL_KEY_COMMA,pressrelease);
                        break;

                        case SDLK_PERIOD:
                                util_set_reset_key(UTIL_KEY_PERIOD,pressrelease);
                        break;

                        case SDLK_MINUS:
                                util_set_reset_key(UTIL_KEY_MINUS,pressrelease);
                        break;

                        case SDLK_PLUS:
                                util_set_reset_key(UTIL_KEY_PLUS,pressrelease);
                        break;

                        case SDLK_SLASH:
                                util_set_reset_key(UTIL_KEY_SLASH,pressrelease);
                        break;

                        case SDLK_ASTERISK:
                                util_set_reset_key(UTIL_KEY_ASTERISK,pressrelease);
                        break;



                        //F1 pulsado
                        case SDLK_F1:
                                util_set_reset_key(UTIL_KEY_F1,pressrelease);
                        break;

                        //F2 pulsado
                        case SDLK_F2:
                                util_set_reset_key(UTIL_KEY_F2,pressrelease);
                        break;

                        //F3 pulsado
                        case SDLK_F3:
                                util_set_reset_key(UTIL_KEY_F3,pressrelease);
                        break;

                        //F4 pulsado
                        case SDLK_F4:
                                util_set_reset_key(UTIL_KEY_F4,pressrelease);
                        break;

                        //F5 pulsado
                        case SDLK_F5:
                                util_set_reset_key(UTIL_KEY_F5,pressrelease);
                        break;




                        //F8 pulsado. osdkeyboard
                        case SDLK_F8:
                                util_set_reset_key(UTIL_KEY_F8,pressrelease);
                        break;

                        //F9 pulsado. quickload
                        case SDLK_F9:
                                util_set_reset_key(UTIL_KEY_F9,pressrelease);
                        break;


                        //F10 pulsado
                        case SDLK_F10:
                                util_set_reset_key(UTIL_KEY_F10,pressrelease);
                        break;


                        //ESC pulsado
                        case SDLK_ESCAPE:
                                util_set_reset_key(UTIL_KEY_ESC,pressrelease);
                        break;

                        //PgUp
                        case SDLK_PAGEUP:
                                util_set_reset_key(UTIL_KEY_PAGE_UP,pressrelease);
                        break;

                        //PgDn
                        case SDLK_PAGEDOWN:
                                util_set_reset_key(UTIL_KEY_PAGE_DOWN,pressrelease);
                        break;

			//Teclas del keypad
			case SDLK_KP0:
                                util_set_reset_key(UTIL_KEY_KP0,pressrelease);
                        break;

                        case SDLK_KP1:
                                util_set_reset_key(UTIL_KEY_KP1,pressrelease);
                        break;

                        case SDLK_KP2:
                                util_set_reset_key(UTIL_KEY_KP2,pressrelease);
                        break;

                        case SDLK_KP3:
                                util_set_reset_key(UTIL_KEY_KP3,pressrelease);
                        break;

                        case SDLK_KP4:
                                util_set_reset_key(UTIL_KEY_KP4,pressrelease);
                        break;

                        case SDLK_KP5:
                                util_set_reset_key(UTIL_KEY_KP5,pressrelease);
                        break;

                        case SDLK_KP6:
                                util_set_reset_key(UTIL_KEY_KP6,pressrelease);
                        break;

                        case SDLK_KP7:
                                util_set_reset_key(UTIL_KEY_KP7,pressrelease);
                        break;

                        case SDLK_KP8:
                                util_set_reset_key(UTIL_KEY_KP8,pressrelease);
                        break;

                        case SDLK_KP9:
                                util_set_reset_key(UTIL_KEY_KP9,pressrelease);
                        break;

                        case SDLK_KP_PERIOD:
                                util_set_reset_key(UTIL_KEY_KP_COMMA,pressrelease);
                        break;

                        case SDLK_KP_ENTER:
                                util_set_reset_key(UTIL_KEY_KP_ENTER,pressrelease);
                        break;



                        default:
                                if (tecla<256) convert_numeros_letras_puerto_teclado(tecla,pressrelease);
                        break;

                }


        //Fuera del switch
//Teclas que necesitan conversion de teclado para CPC
        if (MACHINE_IS_CPC) {

                        if (tecla==scrsdl_keymap_z88_cpc_minus) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_MINUS,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_circunflejo) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_CIRCUNFLEJO,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_arroba) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_ARROBA,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_bracket_left) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_BRACKET_LEFT,pressrelease);



                        else if (tecla==scrsdl_keymap_z88_cpc_colon) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_COLON,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_semicolon) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_SEMICOLON,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_bracket_right) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_BRACKET_RIGHT,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_comma) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_COMMA,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_period) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_PERIOD,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_slash) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_SLASH,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_backslash) util_set_reset_key_cpc_keymap(UTIL_KEY_CPC_BACKSLASH,pressrelease);


        }


//Teclas que necesitan conversion de teclado para Z88
        if (!MACHINE_IS_Z88) return;

                        if (tecla==scrsdl_keymap_z88_cpc_minus) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_MINUS,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_equal) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_EQUAL,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_backslash) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_BACKSLASH,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_bracket_left) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_BRACKET_LEFT,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_bracket_right) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_BRACKET_RIGHT,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_semicolon) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_SEMICOLON,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_apostrophe) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_APOSTROPHE,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_pound) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_POUND,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_comma) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_COMMA,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_period) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_PERIOD,pressrelease);

                        else if (tecla==scrsdl_keymap_z88_cpc_slash) util_set_reset_key_z88_keymap(UTIL_KEY_Z88_SLASH,pressrelease);





}


void scrsdl_resize(int width,int height)
{
	clear_putpixel_cache();

        int zoom_x_calculado,zoom_y_calculado;

        debug_printf (VERBOSE_INFO,"width: %d get_window_width: %d height: %d get_window_height: %d",width,screen_get_window_size_width_no_zoom_border_en(),height,screen_get_window_size_height_no_zoom_border_en());


	zoom_x_calculado=width/screen_get_window_size_width_no_zoom_border_en();
	zoom_y_calculado=height/screen_get_window_size_height_no_zoom_border_en();


        if (!zoom_x_calculado) zoom_x_calculado=1;
        if (!zoom_y_calculado) zoom_y_calculado=1;

        debug_printf (VERBOSE_INFO,"zoom_x: %d zoom_y: %d zoom_x_calculated: %d zoom_y_calculated: %d",zoom_x,zoom_y,zoom_x_calculado,zoom_y_calculado);

        if (zoom_x_calculado!=zoom_x || zoom_y_calculado!=zoom_y) {
                //resize
		debug_printf (VERBOSE_INFO,"Resizing window");

                zoom_x=zoom_x_calculado;
                zoom_y=zoom_y_calculado;
                set_putpixel_zoom();

                //width=screen_get_window_size_width_zoom_border_en();
                //height=screen_get_window_size_height_zoom_border_en();


        }

	scrsdl_debe_redimensionar=1;
	scrsdl_crea_ventana();


}


void scrsdl_actualiza_tablas_teclado(void)
{

	SDL_Event event;
	int pressrelease;




	while( SDL_PollEvent( &event ) ){


		//Si se ha dejado de redimensionar la ventana
		if (event.type!=SDL_VIDEORESIZE) {
                        if (scrsdl_debe_redimensionar) {

				//printf ("evento: %d\n",event.type);

                                debug_printf (VERBOSE_DEBUG,"Resizing windows due to a previous pending resize");

                                clear_putpixel_cache();
                                scrsdl_debe_redimensionar=0;
                                scrsdl_crea_ventana();
                        }
		}



		if (event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) {
			if (event.type==SDL_KEYDOWN) pressrelease=1;
			if (event.type==SDL_KEYUP) pressrelease=0;



			SDL_keysym keysym=event.key.keysym;

			//SDLKey sym=keysym.sym;
			int tecla=keysym.sym;

			if (tecla==SDLK_COMPOSE) {
				if (keysym.scancode==34) tecla=SDLK_COMPOSE_BACKQUOTE;
				else if (keysym.scancode==48) tecla=SDLK_COMPOSE_APOSTROPHE;
			}

			//printf ("tecla: %d scancode: %d\n",tecla,keysym.scancode);

			if (pressrelease) notificar_tecla_interrupcion_si_z88();

			scrsdl_deal_keys(pressrelease,tecla);

		}

		//resize
		if (event.type==SDL_VIDEORESIZE) {
			scrsdl_resize(event.resize.w, event.resize.h);
		}


		//mouse motion
		if (event.type==SDL_MOUSEMOTION) {
                    mouse_x=event.motion.x;
                    mouse_y=event.motion.y;



                    kempston_mouse_x=mouse_x/zoom_x;
                    kempston_mouse_y=255-mouse_y/zoom_y;
                    //printf("Mouse is at (%d,%d)\n", kempston_mouse_x, kempston_mouse_y);

                        debug_printf (VERBOSE_PARANOID,"Mouse motion. X: %d Y:%d kempston x: %d y: %d",mouse_x,mouse_y,kempston_mouse_x,kempston_mouse_y);
		}


		//mouse button
		if (event.type==SDL_MOUSEBUTTONDOWN) {

			debug_printf (VERBOSE_DEBUG,"Mouse Button Press. x=%d y=%d", event.button.x, event.button.y);

                        if ( event.button.button == SDL_BUTTON_LEFT ) {
				//mouse_left=1;
				util_set_reset_mouse(UTIL_MOUSE_LEFT_BUTTON,1);
			}
                        if ( event.button.button == SDL_BUTTON_RIGHT ) {
                                //mouse_right=1;
				util_set_reset_mouse(UTIL_MOUSE_RIGHT_BUTTON,1);
                        }

                        gunstick_x=event.button.x;
                        gunstick_y=event.button.y;
                        gunstick_x=gunstick_x/zoom_x;
                        gunstick_y=gunstick_y/zoom_y;

                        debug_printf (VERBOSE_DEBUG,"Mouse Button press. x=%d y=%d. gunstick: x: %d y: %d", event.button.x, event.button.y,gunstick_x
,gunstick_y);

		}

		if (event.type==SDL_MOUSEBUTTONUP) {
                        debug_printf (VERBOSE_DEBUG,"Mouse Button release. x=%d y=%d", event.button.x, event.button.y);
                        if ( event.button.button == SDL_BUTTON_LEFT ) {
				util_set_reset_mouse(UTIL_MOUSE_LEFT_BUTTON,0);
				//mouse_left=0;
			}
                        if ( event.button.button == SDL_BUTTON_RIGHT ) {
				util_set_reset_mouse(UTIL_MOUSE_RIGHT_BUTTON,0);
				//mouse_right=0;
			}

		}

		if (event.type==SDL_QUIT) {
	                debug_printf (VERBOSE_INFO,"Received window close event");
        	        //Hacemos que aparezca el menu con opcion de salir del emulador
                	menu_abierto=1;
	                menu_button_exit_emulator.v=1;
		}





	}




}

void scrsdl_detectedchar_print(z80_byte caracter)
{
        printf ("%c",caracter);
        //flush de salida standard
        fflush(stdout);

}



int scrsdl_init (void) {

	debug_printf (VERBOSE_INFO,"Init SDL Video Driver");


        //Inicializaciones necesarias
        scr_putpixel=scrsdl_putpixel;
        scr_putchar_zx8081=scrsdl_putchar_zx8081;
        scr_debug_registers=scrsdl_debug_registers;
        scr_messages_debug=scrsdl_messages_debug;
        scr_putchar_menu=scrsdl_putchar_menu;
        scr_putchar_footer=scrsdl_putchar_footer;
        scr_set_fullscreen=scrsdl_set_fullscreen;
        scr_reset_fullscreen=scrsdl_reset_fullscreen;
	scr_z88_cpc_load_keymap=scrsdl_z88_cpc_load_keymap;
	scr_detectedchar_print=scrsdl_detectedchar_print;
        scr_tiene_colores=1;
        screen_refresh_menu=1;



    	if (commonsdl_init() != 0 ) {
		debug_printf (VERBOSE_ERR,"scrsdl_init: Error initializing driver");
                return 1;
	}

	scrsdl_inicializado.v=1;

	if (scrsdl_crea_ventana()) {
		debug_printf (VERBOSE_ERR,"scrsdl_init: Could not set video mode");
		return 1;
	}



        //Otra inicializacion necesaria
        //Esto debe estar al final, para que funcione correctamente desde menu, cuando se selecciona un driver, y no va, que pueda volver al anterior
        scr_driver_name="sdl";


	scr_z88_cpc_load_keymap();

        return 0;

}
