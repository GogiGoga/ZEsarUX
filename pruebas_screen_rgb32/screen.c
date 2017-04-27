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
#include <unistd.h>
#include <string.h>

#include "screen.h"
#include "debug.h"
#include "mem128.h"
#include "operaciones.h"
#include "zx8081.h"
#include "charset.h"
#include "menu.h"
#include "audio.h"
#include "contend.h"
#include "ula.h"
#include "tape_smp.h"
#include "z88.h"
#include "ulaplus.h"


//Incluimos estos dos para la funcion de fade out
#ifdef COMPILE_XWINDOWS
	#include "scrxwindows.h"
#endif

#ifdef COMPILE_AA
	#include "scraa.h"
#endif

#ifdef COMPILE_CURSES
	#include "scrcurses.h"
#endif

z80_int *sprite_table;

//ultima posicion y para funcion screen_print
int screen_print_y=0;

//si se muestran determinados mensajes en splash, como los de cambio de modo de video
//no confundir con el mensaje de bienvenida
z80_bit screen_show_splash_texts={1};

//Rutinas de pantalla
void (*scr_refresca_pantalla) (void);
void (*scr_set_fullscreen) (void);
void (*scr_reset_fullscreen) (void);
int ventana_fullscreen=0;
int (*scr_init_pantalla) (void);
void (*scr_end_pantalla) (void);
z80_byte (*scr_lee_puerto) (z80_byte puerto_h,z80_byte puerto_l);
void (*scr_actualiza_tablas_teclado) (void);

void (*scr_putpixel_zoom) (int x,int y,unsigned int color);
void (*scr_putpixel_zoom_rainbow)(int x,int y,unsigned int color);
void (*scr_putpixel) (int x,int y,unsigned int color);

void (*scr_z88_load_keymap) (void);


int scr_ver_si_refrescar_por_menu_activo(int x,int fila);

//Rutina que muestra los mensajes de registros de cpu, propio para cada driver
void (*scr_debug_registers)(void);

//Rutina que muestra los mensajes de "debug_printf", propio para cada driver
void (*scr_messages_debug)(char *mensaje);

//Rutina para imprimir un caracter del menu
void (*scr_putchar_menu) (int x,int y, z80_byte caracter,z80_byte tinta,z80_byte papel);

void (*scr_putchar_zx8081) (int x,int y, z80_byte caracter);

//indica que el driver de pantalla tiene colores reales. valido para xwindows y curses
int scr_tiene_colores=0;

//nombre del driver: aa, null, xwindows, etc. inicializado por cada driver en init
char *scr_driver_name;


//esto se usa para curses y stdout, tambien afecta 
//a que los caracteres no imprimibles de zx8081 se muestren como ? o como caracteres simulados
z80_bit texto_artistico;
int umbral_arttext=4;


//Para frameskip manual
//valor indicado en menu para frameskip
int frameskip=0;
//conteo actual para ver si toca refrescar frame (cuando vale 0)
int frameskip_counter=0;


//Intento de hacer las rutinas de refresco de pantalla mas rapidas
//Se guarda minima y dibujada y maxima y, para solo refrescar esa zona que se haya tocado con putpixel
//probado de momento con X11 y consume mas cpu aun
int putpixel_max_y=-1;
int putpixel_min_y=99999;


//simular modo video zx80/81 en spectrum
z80_bit simulate_screen_zx8081;

//umbral de pixeles para dibujar o no un punto
z80_byte umbral_simulate_screen_zx8081=4;


//colores usados para el fondo cuando hay menu/overlay activo
int spectrum_colortable_oscuro[EMULATOR_TOTAL_PALETTE_COLOURS];

//colores usados para grises, red, green, etc
//int spectrum_colortable_grises[EMULATOR_TOTAL_PALETTE_COLOURS];

//y puntero que indica una tabla o la otra
int *spectrum_colortable;


//Colores normales (los primeros sin oscuro), sean ya colores o gama de grises
//los 16 colores (de 16..31) son mas oscuros usados en modos interlaced
//los 256 colores siguientes son los usados en gigascreen
//los 4 colores siguientes son los usados en Z88
//int *spectrum_colortable_normal;
int spectrum_colortable_normal[EMULATOR_TOTAL_PALETTE_COLOURS];

//Tabla con los colores reales del Spectrum. Formato RGB
const int spectrum_colortable_original[16] =
{

0x000000,
0x0000CD,  //azul
0xCD0000,  //rojo
0xCD00CD,
0x00CD00,  //verde
0x00CDCD,
0xCDCD00,
0xCDCDCD,

0x000000,
0x0000FF,
0xFF0000,
0xFF00FF,
0x00FF00,
0x00FFFF,
0xFFFF00,
0xFFFFFF

};

//Tabla con los colores reales del Z88. Formato RGB
const int z88_colortable_original[4]={
0x461B7D, //Enabled pixel
0x90B0A7, //Grey enabled pixel
0xD2E0B9, //Empty pixel when screen is switched on
0xE0E0E0 //Empty pixel when screen is switched off
};

//ubicacion en el array de colores de los de Z88
//ver screen.h, Z88_PXCOLON, etc


//Modo de grises activo
//0: colores normales
//1: componente Blue
//2: componente Green
//4: componente Red
//Se pueden sumar para diferentes valores
int screen_gray_mode=0;

//Indica que esta el driver de pantalla stdout
//y por tanto, el bucle de cpu debe interceptar llamadas a RST16
int screen_stdout_driver=0;

z80_bit inverse_video;



//Indica que el driver de video (por el momento, solo xwindows y fbdev) debe repintar la pantalla
//teniendo en cuenta si hay menu activo, y por tanto evitar pintar zonas donde hay texto del menu
//esta variable la pone a 1 el driver de xwindows y fbdev
int screen_refresh_menu=0;


//si esta activado real video
z80_bit rainbow_enabled;

//Si hay autodetecccion de modo rainbow
z80_bit autodetect_rainbow;


//Valores usados en real video
//normalmente a 16
int screen_invisible_borde_superior;
//normalmente a 48. 
int screen_borde_superior;

//estos dos anteriores se suman aqui. es 64 en 48k, y 63 en 128k. por tanto, uno de los dos valores vale 1 menos
int screen_indice_inicio_pant;

//suma del anterior+192
int screen_indice_fin_pant;

//normalmente a 56
int screen_total_borde_inferior;

//normalmente a 48
int screen_total_borde_izquierdo;
//lo mismo en t_estados
int screen_testados_total_borde_izquierdo;

//normalmente a 48
int screen_total_borde_derecho;

//normalmente a 96
int screen_invisible_borde_derecho;

//lo mismo pero en testados
//int screen_invisible_testados_borde_derecho;


//Total de scanlines. usualmente 312 o 311
int screen_scanlines;

//Total de t_estados por linea
int screen_testados_linea;

//Total de t_estados de un frame entero
int screen_testados_total;


//donde empieza borde derecho, en testados
int screen_testados_indice_borde_derecho;

//Estos valores se inicializaran la primera vez en set_machine
int get_total_ancho_rainbow_cached;
int get_total_alto_rainbow_cached;


//buffer border de linea actual
//#define BORDER_ARRAY_LENGTH 228+24 -> en screen.h
//24 de screen_total_borde_izquierdo/2
//Un scanline empieza con display, border derecho, retrace horizontal y borde izquierdo. y ahi se inicia una nueva scanline
//A la hora de dibujar en nuestra rutina consideramos: borde izq-display-borde derecho-retrace
//Pero el border en cambio si que lo guardamos teniendo en cuenta esto,
//sentencias out guardan valor de border comenzando en posicion 24
z80_byte buffer_border[BORDER_ARRAY_LENGTH];

z80_byte buffer_atributos[ATRIBUTOS_ARRAY_LENGTH];

//ultima posicion leida en buffer_atributos
int last_x_atributo;



//frames que hemos saltado
int framedrop_total=0;
//frames que se iban a dibujar, saltados o no... cuando se llega a 50, se resetea, y se muestra por pantalla los drop
int frames_total=0;

//ha llegado la interrupcion de final de frame antes de redibujar la pantalla. Normalmente no dibujar ese frame para ganar velocidad
int framescreen_saltar;

//Ultimo FPS leido. Para mostrar en consola o debug menu
int ultimo_fps=0;

//frame entrelazado par
//z80_bit interlaced_frame_par;

//numero de frame actual. para interlaced
z80_byte interlaced_numero_frame=0;

//si esta activo el modo entrelazado
z80_bit video_interlaced_mode;

//si esta activo el scanlines mode. requiere interlaced
z80_bit video_interlaced_scanlines={0};

//si esta activo gigascreen
z80_bit gigascreen_enabled={0};

//ajuste para que algunos programas de gigascreen se vean bien
//esto pasa porque seguramente hay algun error con los t_estados, pero no se donde...
//esas demos gigascreen, por ejemplo, Animeeshon, utilizan paginacion de ram 5/7 de pantalla, y ademas,
//usan metodo rainbow para cambiar el color en varias lineas de un cuadro de 8x8
int gigascreen_ajuste_t_estados=0;

/*

paralaktica- ajuste 100 para hada inicio frame. o 5 para puerto 32765
animeshoon - ajuste 54 inicio frame. o 14 para puerto 32765
mescaline - ajuste 84 para pantalla inicio inicio frame. 4 o 5 para puerto 32756

*/


//vofile
FILE *ptr_vofile;
char *vofilename;

z80_bit vofile_inserted;

//fps del archivo final=50/vofile_fps
int vofile_fps=10;

int vofile_frame_actual;


//devuelve 1 si hay que dibujar la linea, de acorde al entrelazado
/*
int if_store_scanline_interlace(int y)
{
         //si linea no coincide con entrelazado, volvemos
         if (video_interlaced_mode.v==0) return 1;
         if ((y&1) == interlaced_frame_par.v ) return 1;

        return 0;
}
*/

int if_store_scanline_interlace(int y)
{

	//para que no se queje el compilador
	y++;

	return 1;
}

//Retorna 1 si el driver grafico es completo
int si_complete_video_driver(void)
{
        if (!strcmp(scr_driver_name,"xwindows")) return 1;
        if (!strcmp(scr_driver_name,"sdl")) return 1;
        if (!strcmp(scr_driver_name,"fbdev")) return 1;
        if (!strcmp(scr_driver_name,"cocoa")) return 1;
        return 0;
}



//establece valor de screen_indice_inicio_pant y screen_indice_fin_pant
void screen_set_video_params_indices(void)
{
	screen_indice_inicio_pant=screen_invisible_borde_superior+screen_borde_superior;
	screen_indice_fin_pant=screen_indice_inicio_pant+192;
	screen_scanlines=screen_indice_fin_pant+screen_total_borde_inferior;
	//screen_testados_linea=(screen_invisible_borde_izquierdo+screen_total_borde_izquierdo+256+screen_total_borde_derecho)/2;

	screen_testados_total=screen_testados_linea*screen_scanlines;

	//TODO. hacer esto de manera mas elegante

	//timer para Z88 es cada 5 ms y ejecutamos los mismos ciclos que un spectrum para cada 5 ms. Por tanto, va 4 veces mas rapido que spectrum

	 
	if (MACHINE_IS_Z88) {
		//dado que se genera interrupcion cada 5 ms (y no cada 20ms) esto equivale a 4 veces menos

		//Desactivado. 
		//Si hago esto, saltando a interrupciones solo con EI: no funciona el teclado, quiza porque entonces el core del Z88 es demasiado lento...
		//si salta interrupciones aunque haya un DI, entonces se llaman interrupciones como parte del proceso inicial del boot y no arranca
		screen_testados_total /=4;
		//screen_testados_total /=2;
	}
	


	screen_testados_total_borde_izquierdo=screen_total_borde_izquierdo/2;

	screen_testados_indice_borde_derecho=screen_testados_total_borde_izquierdo+128;

	//printf ("t_estados_linea: %d\n",screen_testados_linea);
	//printf ("scanlines: %d\n",screen_scanlines);
	//printf ("t_estados_total: %d\n",screen_testados_total);

}


//el formato del buffer del video rainbow es:
//1 byte por pixel, cada pixel tiene el valor de color 0..15. 
//Valores 16..255 no tienen sentido, de momento
//Valores mas alla de 255 son usados en ulaplus. ver tablas exactas

/*
z80_int *rainbow_buffer=NULL;


//Para gigascreen
z80_int *rainbow_buffer_one=NULL;
z80_int *rainbow_buffer_two=NULL;

*/

z80_long_int *rainbow_buffer=NULL;


//Para gigascreen
z80_long_int *rainbow_buffer_one=NULL;
z80_long_int *rainbow_buffer_two=NULL;


//cache de putpixel
z80_int *putpixel_cache=NULL;


//funcion con debug. usada en el macro con debug

void store_value_rainbow_debug(z80_int **p, z80_byte valor)
{
	z80_int *puntero_buf_rainbow;
	puntero_buf_rainbow=*p;

        int ancho,alto,tamanyo;

	ancho=screen_get_window_width_no_zoom();
	alto=screen_get_window_height_no_zoom();


        tamanyo=ancho*alto;

        //Asignamos mas bytes dado que la ultima linea de pantalla genera datos (de borde izquierdo) mas alla de donde corresponde
        tamanyo=tamanyo+ancho;




	if (puntero_buf_rainbow==NULL) {
		printf ("puntero_buf_rainbow NULL\n");
		return ;
	}

	if (puntero_buf_rainbow>(rainbow_buffer+tamanyo)) {
		printf ("puntero_buf_rainbow mayor limite final\n");
		return;
	}

	if (puntero_buf_rainbow<rainbow_buffer) {
                printf ("puntero_buf_rainbow menor que inicial\n");
                return;
        }


	*puntero_buf_rainbow=valor;
	(*p)++;
	
}


//Funcion normal
#define store_value_rainbow(p,x) *p++=x;

//Funcion con debug
//#define store_value_rainbow(p,x) store_value_rainbow_debug(&p,x);


void recalcular_get_total_ancho_rainbow(void)
{
	debug_printf (VERBOSE_INFO,"Recalculate get_total_ancho_rainbow");
	if (MACHINE_IS_Z88) {
		get_total_ancho_rainbow_cached=SCREEN_Z88_WIDTH;
	}

	else {
		get_total_ancho_rainbow_cached=(screen_total_borde_izquierdo+screen_total_borde_derecho)*border_enabled.v+256;
	}
}

//sin contar la parte invisible
void recalcular_get_total_alto_rainbow(void)
{
        debug_printf (VERBOSE_INFO,"Recalculate get_total_alto_rainbow");
	if (MACHINE_IS_Z88) {
		//get_total_alto_rainbow_cached=(screen_borde_superior+screen_total_borde_inferior)+192;
		get_total_alto_rainbow_cached=SCREEN_Z88_HEIGHT;
        }
	
	else {
	        get_total_alto_rainbow_cached=(screen_borde_superior+screen_total_borde_inferior)*border_enabled.v+192;
	}
}

//esas dos funciones, get_total_ancho_rainbow y get_total_alto_rainbow ahora son macros definidos en screen.h


void init_rainbow(void)
{

        if (rainbow_buffer_one!=NULL) {
                debug_printf (VERBOSE_INFO,"Freeing previous rainbow video buffer");
                free(rainbow_buffer_one);
		free(rainbow_buffer_two);
        }


	int ancho,alto,tamanyo;



        ancho=screen_get_window_width_no_zoom();
        alto=screen_get_window_height_no_zoom();


	tamanyo=ancho*alto*2; //buffer de 16 bits (*2 bytes)



        //Multiplicar por 2, para tener R,G,B,alpha
        tamanyo *=2;


	//Asignamos mas bytes dado que la ultima linea de pantalla genera datos (de borde izquierdo) mas alla de donde corresponde
	tamanyo=tamanyo+ancho;

	debug_printf (VERBOSE_INFO,"Initializing two rainbow video buffer of size: %d x %d , %d bytes each",ancho,alto,tamanyo);

	rainbow_buffer_one=malloc(tamanyo);
	if (rainbow_buffer_one==NULL) {
		cpu_panic("Error allocating rainbow video buffer");
	}


        rainbow_buffer_two=malloc(tamanyo);
        if (rainbow_buffer_two==NULL) {
                cpu_panic("Error allocating rainbow video buffer");
        }    

	


	rainbow_buffer=rainbow_buffer_one;


}

void init_cache_putpixel(void)
{

#ifdef PUTPIXELCACHE	
	if (putpixel_cache!=NULL) {
		debug_printf (VERBOSE_INFO,"Freeing previous putpixel_cache");
		free(putpixel_cache);
	}

        int ancho,alto,tamanyo;


        ancho=screen_get_window_width_no_zoom();
        alto=screen_get_window_height_no_zoom();


        tamanyo=ancho*alto;
	//para poder hacer putpixel cache con zoom y*2 y interlaced, doble de tamanyo
	tamanyo *=2;

	
	putpixel_cache=malloc(tamanyo*2); //*2 porque es z80_int
	

	debug_printf (VERBOSE_INFO,"Initializing putpixel_cache of size: %d bytes",tamanyo);

	if (putpixel_cache==NULL) {
		cpu_panic("Error allocating putpixel_cache video buffer");
	}	

	clear_putpixel_cache();
#else
	debug_printf (VERBOSE_INFO,"Putpixel cache disabled on compilation time");
#endif

}


//#define put_putpixel_cache(x,y) putpixel_cache[x]=y
	

//rutina para comparar un caracter
//entrada:
//p: direccion de pantalla en sp


//salida:
//caracter que coincide
//0 si no hay coincidencia

//inverse si o no

//usado en scrcurses y en simular video de zx80/81
//sprite origen a intervalos de "step"
z80_byte compare_char_tabla_step(z80_byte *origen,z80_byte *inverse,z80_byte *tabla_leemos,int step) {

        z80_byte *copia_origen;
	z80_byte *tabla_comparar;


        z80_byte caracter=32;

        for (;caracter<128;caracter++) {
                //printf ("%d\n",caracter);
                //tabla_leemos apunta siempre al primer byte de la tabla del caracter que leemos
                tabla_comparar=tabla_leemos;
                copia_origen=origen;

                //tabla_comparar : puntero sobre la tabla de caracteres
                //copia_origen: puntero sobre la pantalla

                //
                int numero_byte=0;
                for (numero_byte=0; (numero_byte<8) && (*copia_origen == *tabla_comparar) ;numero_byte++,copia_origen+=step,tabla_comparar++) {
                }

                if (numero_byte == 8) {
                        *inverse=0;
                        return caracter;
                }


                //probar con texto inverso

                numero_byte=0;
                for (numero_byte=0; (numero_byte<8) && (*copia_origen == (*tabla_comparar ^ 255 )) ;numero_byte++,copia_origen+=step,tabla_comparar++) {
                }

                if (numero_byte == 8) {
                        *inverse=1;
                        return caracter;
                }


                tabla_leemos +=8;
        }



        return 0;
}

//comparar sprite de origen (con direccionamiento de spectrum, cada linea a intervalo 256) con la tabla de caracteres de la ROM
z80_byte compare_char_tabla(z80_byte *origen,z80_byte *inverse,z80_byte *tabla_leemos) {
	return compare_char_tabla_step(origen,inverse,tabla_leemos,256);
}



//usado en scrcurses con rainbow en zx8081
//devuelve 255 si no coincide
z80_byte compare_char_tabla_rainbow(z80_byte *origen,z80_byte *inverse,z80_byte *tabla_leemos) {

        z80_byte *tabla_comparar;


        z80_byte caracter=0;



        for (;caracter<64;caracter++) {
                //printf ("%d\n",caracter);
                //tabla_leemos apunta siempre al primer byte de la tabla del caracter que leemos
                tabla_comparar=tabla_leemos;
                //copia_origen=origen;

                //tabla_comparar : puntero sobre la tabla de caracteres
                //copia_origen: puntero sobre la pantalla

                //
                int numero_byte=0;
                for (numero_byte=0; (numero_byte<8) && (origen[numero_byte] == *tabla_comparar) ;numero_byte++,tabla_comparar++) {
                }

                if (numero_byte == 8) {
                        *inverse=0;
                        return caracter;
                }


                //probar con texto inverso
               	numero_byte=0;
                for (numero_byte=0; (numero_byte<8) && (origen[numero_byte] == (*tabla_comparar ^ 255 )) ;numero_byte++,tabla_comparar++) {
       	        }

               	if (numero_byte == 8) {
			*inverse=1;
                        return caracter;
       	        }


                tabla_leemos +=8;
        }



        return 255;
}



z80_byte compare_char_step(z80_byte *origen,z80_byte *inverse,int step)
{
        z80_byte *tabla_leemos;
	z80_byte caracter;
        
	//Tenemos que buscar en toda la tabla de caracteres. Primero en tabla conocida y luego en la que apunta a 23606/7
	tabla_leemos=char_set;

	caracter=compare_char_tabla_step(origen,inverse,tabla_leemos,step);
	if (caracter!=0) return caracter;

	z80_int puntero_tabla_caracteres;
	if (MACHINE_IS_SPECTRUM_16_48) {
		puntero_tabla_caracteres=value_8_to_16(memoria_spectrum[23607],memoria_spectrum[23606])+256;
		
		caracter=compare_char_tabla_step(origen,inverse,&memoria_spectrum[puntero_tabla_caracteres],step);

		return caracter;
	}

	if (MACHINE_IS_SPECTRUM_128_P2_P2A) {
		//modelos 128k y +2a
		z80_byte *offset_ram_5=ram_mem_table[5];

		//buscamos el puntero
		offset_ram_5 +=(23606-16384);

		puntero_tabla_caracteres=value_8_to_16(*(offset_ram_5+1),*offset_ram_5)+256;
		//printf ("puntero: %d\n",puntero_tabla_caracteres);
		//convertimos ese puntero de spectrum en puntero de ram


		z80_int dir=puntero_tabla_caracteres;
		//obtenido tal cual de peek byte

		z80_byte *puntero;


		//apunta a rom?
		if (dir<16384) {

			//hemos de suponer que siempre apunta a rom3(+2a)/rom1(128k)

			//modelos 128k
			if (MACHINE_IS_SPECTRUM_128_P2) {
				//ROM1
				puntero=&memoria_spectrum[16384]+dir;
			}

			//modelos +2a
			//else if (MACHINE_IS_SPECTRUM_P2A) {
			else {
				//ROM 3
				puntero=&memoria_spectrum[49152]+dir;
			}

		}	
		else {

			int segmento;
	                segmento=dir / 16384;
        	        dir = dir & 16383;
                	puntero=memory_paged[segmento]+dir;
		}

		caracter=compare_char_tabla_step(origen,inverse,puntero,step);
		return caracter;

	}

	return caracter;
}

z80_byte compare_char(z80_byte *origen,z80_byte *inverse)
{
	return compare_char_step(origen,inverse,256);
}

z80_int devuelve_direccion_pantalla_no_table(z80_byte x,z80_byte y)
{
        
        z80_byte linea,high,low;
        
        linea=y/8;
        
        low=x+ ((linea & 7 )<< 5);
        high= (linea  & 24 )+ (y%8);
        


        return low+high*256;
}

void init_sprite_table(void)
{

	int x,y;
	int index=0;
	z80_int direccion;

	sprite_table=malloc(6144*2);
	if (sprite_table==NULL) {
		cpu_panic ("Error allocating sprite table");
	}


	for (y=0;y<192;y++) {
                for (x=0;x<32;x++) {
                direccion=devuelve_direccion_pantalla_no_table(x,y);
		sprite_table[index++]=direccion;
		}
	}

}



int scr_si_color_oscuro(void)
{
	if (menu_overlay_activo) {

		//si esta modo ulaplus activo, no gris
		if (ulaplus_presente.v && ulaplus_enabled.v) return 0;

                //pero si no hay menu y esta la segunda capa de overlay, no poner en gris


                //pero si hay texto splash, si hay que poner en gris
		//si hay texto de guessing loading, hay que poner gris

                if (menu_abierto==1) {
			return 1;
                }

		else {
			//si no estamos en menu, hacerlo solo cuando este splash   //o guessing tape
			if (menu_splash_text_active.v) return 1;	
			if (tape_guessing_parameters) return 1;
		}
	}

	return 0;
}


//Rutina comun de refresco de border de zx80,81 y spectrum
void scr_refresca_border_comun_spectrumzx8081(z80_byte color)
{
//      printf ("Refresco border\n");

        int x,y;

	//simular modo fast
        if (MACHINE_IS_ZX8081 && video_fast_mode_emulation.v==1 && video_fast_mode_next_frame_black==LIMIT_FAST_FRAME_BLACK) {

		//printf ("color border 0\n");
		color=0;
	}




        //parte superior
        for (y=0;y<TOP_BORDER;y++) {
                for (x=0;x<ANCHO_PANTALLA*zoom_x+LEFT_BORDER*2;x++) {
                                scr_putpixel(x,y,color);


                }
        }

        //parte inferior
        for (y=0;y<BOTTOM_BORDER;y++) {
                for (x=0;x<ANCHO_PANTALLA*zoom_x+LEFT_BORDER*2;x++) {
                                scr_putpixel(x,TOP_BORDER+y+ALTO_PANTALLA*zoom_y,color);


                }
        }


        //laterales
        for (y=0;y<ALTO_PANTALLA*zoom_y;y++) {
                for (x=0;x<LEFT_BORDER;x++) {
                        scr_putpixel(x,TOP_BORDER+y,color);
                        scr_putpixel(LEFT_BORDER+ANCHO_PANTALLA*zoom_x+x,TOP_BORDER+y,color);
                }

        }


}

void scr_refresca_border(void)
{
	int color;

	if (simulate_screen_zx8081.v==1) color=15;
	else color=out_254 & 7;
	scr_refresca_border_comun_spectrumzx8081(color);
}	

void scr_refresca_border_zx8081(void)
{

	scr_refresca_border_comun_spectrumzx8081(15);
}



void clear_putpixel_cache(void)
{

#ifdef PUTPIXELCACHE
	int x,y;
	int indice=0;

	int tamanyo_y;

	tamanyo_y=get_total_alto_rainbow();

	if (video_interlaced_mode.v) tamanyo_y *=2;

	for (y=0;y<tamanyo_y;y++) {
		for (x=0;x<get_total_ancho_rainbow();x++) {
			//cambiar toda la cache
			
			
			//put_putpixel_cache(indice,putpixel_cache[indice] ^1);
			//put_putpixel_cache(indice,254);
			
			//ponemos cualquier valor que no pueda existir, para invalidarla
			putpixel_cache[indice]=65535;
			
			indice++;
		}
	}
#endif

}

//putpixel escalandolo al zoom necesario y teniendo en cuenta toda la pantalla entera (rainbow)
//y con cache
//por tanto, (0,0) = arriba izquierda del border
void scr_putpixel_zoom_rainbow_mas_de_uno(int x,int y,unsigned int color)
{

#ifdef PUTPIXELCACHE
	int indice_cache;

	indice_cache=(get_total_ancho_rainbow()*y)+x;

	if (putpixel_cache[indice_cache]==color) return;

	//printf ("not in cache: x %d y %d\n",x,y);
	//put_putpixel_cache(indice_cache,color);
	putpixel_cache[indice_cache]=color;
#endif

        int zx,zy;
        int xzoom=x*zoom_x;
        int yzoom=y*zoom_y;

	
        //Escalado a zoom indicado
        for (zx=0;zx<zoom_x;zx++) {
        	for (zy=0;zy<zoom_y;zy++) {
                        scr_putpixel(xzoom+zx,yzoom+zy,color);
                }
        }

}


//putpixel con zoom y multiple de 2 y teniendo en cuenta el interlaced
void scr_putpixel_zoom_rainbow_interlaced_zoom_two(int x,int y,unsigned int color)
{
	int zyinicial=( (interlaced_numero_frame & 1)==1 ? 1 : 0);

	//interlaced mode, linea impar mas oscura
	if (zyinicial && video_interlaced_scanlines.v) color +=16;

	//printf ("%d\n",zyinicial);

	y=y*2;

#ifdef PUTPIXELCACHE
        int indice_cache;


	//putpixel cache en caso de interlaced zoom y*2 tiene doble de alto
        indice_cache=(get_total_ancho_rainbow()*(y+zyinicial) )+x;


        if (putpixel_cache[indice_cache]==color) return;

        //printf ("not in cache: x %d y %d\n",x,y);
        //put_putpixel_cache(indice_cache,color);
        putpixel_cache[indice_cache]=color;
#endif

        int zx,zy;
        int xzoom=x*zoom_x;

	int zoom_y_result=zoom_y/2;
        int yzoom=(y+zyinicial)*zoom_y_result;



        //Escalado a zoom indicado
        for (zx=0;zx<zoom_x;zx++) {
                for (zy=0;zy<zoom_y_result;zy++) {
                        scr_putpixel(xzoom+zx,yzoom+zy,color);
                }
		//scr_putpixel(xzoom+zx,y,color);
        }

}

//putpixel escalandolo con zoom 1 - sin escalado
//y con cache
//por tanto, (0,0) = arriba izquierda del border
void scr_putpixel_zoom_rainbow_uno(int x,int y,unsigned int color)
{

#ifdef PUTPIXELCACHE
        int indice_cache;
        
        indice_cache=(get_total_ancho_rainbow()*y)+x;
        
        if (putpixel_cache[indice_cache]==color) return;
        
        //printf ("not in cache: x %d y %d\n",x,y);
        //put_putpixel_cache(indice_cache,color);
        putpixel_cache[indice_cache]=color;
#endif
        
	scr_putpixel(x,y,color);
}


//putpixel escalandolo al zoom necesario y teniendo en cuenta el border
//por tanto, (0,0) = dentro de pantalla
void scr_putpixel_zoom_mas_de_uno(int x,int y,unsigned int color)
{

#ifdef PUTPIXELCACHE
	int indice_cache;

	if (MACHINE_IS_Z88) {
		indice_cache=(get_total_ancho_rainbow()*(y)) + x;
	}
	else {	
		indice_cache=(get_total_ancho_rainbow()*(screen_borde_superior*border_enabled.v+y)) + screen_total_borde_izquierdo*border_enabled.v+x;
	}
	
	if (putpixel_cache[indice_cache]==color) return;

	//printf ("scr_putpixel_zoom not in cache: x %d y %d indice_cache=%d \n",x,y,indice_cache);
	//put_putpixel_cache(indice_cache,color);
	putpixel_cache[indice_cache]=color;	
#endif

        int zx,zy;
	int offsetx,offsety;

	if (MACHINE_IS_Z88) {
		offsetx=0;
		offsety=0;
	}
	else {
	        offsetx=LEFT_BORDER*border_enabled.v;
        	offsety=TOP_BORDER*border_enabled.v;
	}
        int xzoom=x*zoom_x;
        int yzoom=y*zoom_y;

	

	//Escalado a zoom indicado
        for (zx=0;zx<zoom_x;zx++) {
        	for (zy=0;zy<zoom_y;zy++) {
                	scr_putpixel(offsetx+xzoom+zx,offsety+yzoom+zy,color);
		}
	}
}

//putpixel escalandolo a zoom 1 -> no zoom
//por tanto, (0,0) = dentro de pantalla
void scr_putpixel_zoom_uno(int x,int y,unsigned int color)
{

#ifdef PUTPIXELCACHE
        int indice_cache;

	if (MACHINE_IS_Z88) {
		indice_cache=(get_total_ancho_rainbow()*(y)) + x;
	}

	else {
        	indice_cache=(get_total_ancho_rainbow()*(screen_borde_superior*border_enabled.v+y)) + screen_total_borde_izquierdo*border_enabled.v+x;
	}

        if (putpixel_cache[indice_cache]==color) return;
        
        //printf ("scr_putpixel_zoom not in cache: x %d y %d indice_cache=%d \n",x,y,indice_cache);
        //put_putpixel_cache(indice_cache,color);
        putpixel_cache[indice_cache]=color;     
#endif  

	        int offsetx,offsety;


	if (MACHINE_IS_Z88) {
		offsetx=0;
		offsety=0;
	}
	else {
        offsetx=LEFT_BORDER*border_enabled.v;
        offsety=TOP_BORDER*border_enabled.v;
	}

	scr_putpixel(offsetx+x,offsety+y,color);
}               


void set_putpixel_zoom(void)
{
	if (zoom_x==1 && zoom_y==1) {
		scr_putpixel_zoom=scr_putpixel_zoom_uno;
		scr_putpixel_zoom_rainbow=scr_putpixel_zoom_rainbow_uno;
		debug_printf (VERBOSE_INFO,"Setting putpixel functions to zoom 1");
	}		

	//zoom_y multiple de dos (valor par) y interlaced
	else if (zoom_y>=2 && (zoom_y&1)==0 && video_interlaced_mode.v) {
		scr_putpixel_zoom=scr_putpixel_zoom_mas_de_uno;
                scr_putpixel_zoom_rainbow=scr_putpixel_zoom_rainbow_interlaced_zoom_two;
		debug_printf (VERBOSE_INFO,"Setting putpixel functions to interlaced zoom multiple of two");
	}

	else {
		scr_putpixel_zoom=scr_putpixel_zoom_mas_de_uno;
		scr_putpixel_zoom_rainbow=scr_putpixel_zoom_rainbow_mas_de_uno;
		debug_printf (VERBOSE_INFO,"Setting putpixel functions to variable zoom");
	}
}


//Muestra un caracter en pantalla, al estilo del spectrum o zx80/81
//entrada: puntero=direccion a tabla del caracter
//x,y: coordenadas en x-0..31 e y 0..23 del zx81
//inverse si o no
//ink, paper
//si emula fast mode o no
void scr_putsprite_comun(z80_byte *puntero,int x,int y,z80_bit inverse,z80_byte tinta,z80_byte papel,z80_bit fast_mode)
{
        z80_byte color;
        z80_byte bit;
        z80_byte line;
        z80_byte byte_leido;

        //margenes de zona interior de pantalla. Para modo rainbow
        int margenx_izq=screen_total_borde_izquierdo*border_enabled.v;
        int margeny_arr=screen_borde_superior*border_enabled.v;


        y=y*8;

        for (line=0;line<8;line++,y++) {
          byte_leido=*puntero++;
          if (inverse.v==1) byte_leido = byte_leido ^255;
          for (bit=0;bit<8;bit++) {
                if (byte_leido & 128 ) color=tinta;
                else color=papel;

                //simular modo fast para zx81
                if (fast_mode.v==1 && video_fast_mode_emulation.v==1 && video_fast_mode_next_frame_black==LIMIT_FAST_FRAME_BLACK) color=0;

                byte_leido=(byte_leido&127)<<1;

		//este scr_putpixel_zoom_rainbow tiene en cuenta los timings de la maquina (borde superior, por ejemplo)
		if (rainbow_enabled.v==1) scr_putpixel_zoom_rainbow((x*8)+bit+margenx_izq,y+margeny_arr,color);

                else scr_putpixel_zoom((x*8)+bit,y,color);

           }
        }
}


//Muestra un caracter en pantalla, al estilo del zx80/81
//entrada: puntero=direccion a tabla del caracter
//x,y: coordenadas en x-0..31 e y 0..23 del zx81
//inverse si o no
void scr_putsprite(z80_byte *puntero,int x,int y,z80_bit inverse)
{

        z80_bit f;
        f.v=0;

	scr_putsprite_comun(puntero,x,y,inverse,0,15,f);
}




//Muestra un caracter de zx80/zx81 en pantalla
//entrada: direccion=tabla del caracter en direccion de memoria_spectrum
//x,y: coordenadas en x-0..31 e y 0..23 del zx81
//inverse si o no
void scr_putsprite_zx8081(z80_int direccion,int x,int y,z80_bit inverse)
{
	z80_bit f;

	f.v=1;

        scr_putsprite_comun(&memoria_spectrum[direccion],x,y,inverse,0,15,f);
        return;


}

//Devuelve bit pixel, en coordenadas 0..255,0..191. En pantalla rainbow para zx8081
int scr_get_pixel_rainbow(int x,int y)
{

	z80_byte byte_leido;

	z80_int *puntero_buf_rainbow;

        puntero_buf_rainbow=&rainbow_buffer[ y*get_total_ancho_rainbow()+x ];

	byte_leido=(*puntero_buf_rainbow)&15;
	if (byte_leido==0) return 1;
	else return 0;

}



//Devuelve pixel a 1 o 0, en coordenadas 0..255,0..191. En pantalla de spectrum
int scr_get_pixel(int x,int y)
{

	z80_int direccion;
	z80_byte byte_leido;
	z80_byte bit;
	z80_byte mascara;

       z80_byte *screen=get_base_mem_pantalla();
       direccion=sprite_table[(y<<5)]+x/8;
       byte_leido=screen[direccion];


	bit=x%8;
	mascara=128;
	if (bit) mascara=mascara>>bit;
	if ((byte_leido & mascara)==0) return 0;
	else return 1;

}


//Devuelve suma de pixeles a 1 en un cuadrado de 4x4, en coordenadas 0..255,0..191. En pantalla de spectrum
int scr_get_4pixel(int x,int y)
{

	int result=0;
	int dx,dy;

        for (dx=0;dx<4;dx++) {
                for (dy=0;dy<4;dy++) {
			result +=scr_get_pixel(x+dx,y+dy);
		}
	}

	return result;

}


//Devuelve suma de pixeles de colores en un cuadrado de 4x4, en coordenadas 0..255,0..191. En rainbow para zx8081
int scr_get_4pixel_rainbow(int x,int y)
{

	int result=0;
	int dx,dy;

        for (dx=0;dx<4;dx++) {
                for (dy=0;dy<4;dy++) {
                        result +=scr_get_pixel_rainbow(x+dx,y+dy);
                }
        }
        return result;


}


void scr_simular_video_zx8081_put4pixel(int x,int y,int color)
{

	int dx,dy;
	//int zx,zy;

	for (dx=0;dx<4;dx++) {
		for (dy=0;dy<4;dy++) {
				scr_putpixel_zoom(x+dx,y+dy,color);


		}
	}


}

int calcula_offset_screen (int x,int y)
{

        unsigned char high,low;

        low=x+ ((y & 7 )<< 5);
        high= y  & 24;



        return low+high*256;



}




//Simular pantalla del zx80/81 en spectrum
//Se busca para cada bloque de 8x8 coincidencias con tablas de caracter
//sino, se divide en 4 bloques de 4x4 y para cada uno, si los pixeles a 1 es mayor o igual que el umbral, se pone pixel(color 0). Si no, se quita (color 15)
void scr_simular_video_zx8081(void)
{

int x,y;
z80_byte caracter;
z80_byte *screen;


screen=get_base_mem_pantalla();
unsigned char inv;
z80_bit inversebit;

for (y=0;y<192;y+=8) {
	for (x=0;x<256;x+=8) {

                //Ver en casos en que puede que haya menu activo y hay que hacer overlay
               if (scr_ver_si_refrescar_por_menu_activo(x/8,y/8)) {

			caracter=compare_char(&screen[  calcula_offset_screen(x/8,y/8)  ] , &inv);
		
			if (caracter) {
				if (inv) inversebit.v=1;
				else inversebit.v=0;
				//printf ("caracter: %d\n",caracter);
				scr_putsprite(&char_set[(caracter-32)*8],x/8,y/8,inversebit);

			}


			else {
				//Pixel izquierda arriba
				if (scr_get_4pixel(x,y)>=umbral_simulate_screen_zx8081) scr_simular_video_zx8081_put4pixel(x,y,0);
				else scr_simular_video_zx8081_put4pixel(x,y,15);

				//Pixel derecha arriba
				if (scr_get_4pixel(x+4,y)>=umbral_simulate_screen_zx8081) scr_simular_video_zx8081_put4pixel(x+4,y,0);
				else scr_simular_video_zx8081_put4pixel(x+4,y,15);

				//Pixel derecha abajo
				if (scr_get_4pixel(x+4,y+4)>=umbral_simulate_screen_zx8081) scr_simular_video_zx8081_put4pixel(x+4,y+4,0);
				else scr_simular_video_zx8081_put4pixel(x+4,y+4,15);

				//Pixel izquierda abajo
				if (scr_get_4pixel(x,y+4)>=umbral_simulate_screen_zx8081) scr_simular_video_zx8081_put4pixel(x,y+4,0);
				else scr_simular_video_zx8081_put4pixel(x,y+4,15);
			}
		}

	}
}

}


//Retorna 0 si no hay que refrescar esa zona
//Pese a que en cada driver de video, cuando refresca pantalla, luego llama a overlay menu
//Pero en xwindows, se suele producir un refresco por parte del servidor X que provoc
//parpadeo entre la pantalla de spectrum y el menu
//por tanto, es preferible que si esa zona de pantalla de spectrum esta ocupada por algun texto del menu, no repintar para no borrar texto del menu
//Esto incluye tambien el texto de splash del inicio 
//No incluiria cualquier otra funcion de overlay diferente del menu o el splash
int scr_ver_si_refrescar_por_menu_activo(int x,int fila)
{


                        //Ver en casos en que puede que haya menu activo y hay que hacer overlay
                        if (screen_refresh_menu==1) {
                                if (menu_overlay_activo==1) {
                                        //hay menu activo. no refrescar esa coordenada si hay texto del menu
					int pos=fila*32+x;
                                        if (overlay_screen_array[pos].caracter!=0) {
                                                //no hay que repintar en esa zona
						return 0;
                                        }

					//segunda capa overlay. esto no hace falta. lo que haremos es que cada vez que se escribe en second_overlay,
					//se copia al primero. cuando se hace cls tambien se copia al primero
					//esto hace que sea mas rapido la llamada a esta funcion, porque asi solo hay que comprobar una capa
					/*
					if (menu_second_layer) {
						if (second_overlay_screen_array[pos].caracter!=0) {
                                            	    	//no hay que repintar en esa zona
	                                                return 0;
	                                        }

					}
					*/

                                }
                        }
	return 1;

}


void scr_refresca_pantalla_rainbow_comun_gigascreen(void)
{
	if ((interlaced_numero_frame&1)==0) {

		//printf ("refresco con gigascreen\n");

        //aqui no tiene sentido (o si?) el modo simular video zx80/81 en spectrum
        int ancho,alto; 
        
        ancho=get_total_ancho_rainbow();
        alto=get_total_alto_rainbow();
        
        int x,y,bit;
        
        //margenes de zona interior de pantalla. Para overlay menu
        int margenx_izq=screen_total_borde_izquierdo*border_enabled.v;
        int margenx_der=screen_total_borde_izquierdo*border_enabled.v+256;
        int margeny_arr=screen_borde_superior*border_enabled.v;
        int margeny_aba=screen_borde_superior*border_enabled.v+192;
        //para overlay menu tambien
        //int fila;
        //int columna;

	//Para gigascreen, valores que se encontraran en el buffer rainbow seran entre 0 y 15
        z80_byte color_pixel_one,color_pixel_two;

	int color_pixel_final;
        z80_int *puntero_one,*puntero_two;

        puntero_one=rainbow_buffer_one;
        puntero_two=rainbow_buffer_two;

        int dibujar;

        for (y=0;y<alto;y++) {
                for (x=0;x<ancho;x+=8) {
                        dibujar=1;

                        //Ver si esa zona esta ocupada por texto de menu u overlay

                        if (y>=margeny_arr && y<margeny_aba && x>=margenx_izq && x<margenx_der) {


                                //normalmente a 48
                                //int screen_total_borde_izquierdo;

                                if (!scr_ver_si_refrescar_por_menu_activo( (x-margenx_izq)/8, (y-margeny_arr)/8) )
                                        dibujar=0;

                        }
                        if (dibujar==1) {
                        
                                        for (bit=0;bit<8;bit++) {
                                        
                                        
                                                //printf ("x: %d y: %d\n",x,y);
                                                
                                                color_pixel_one=*puntero_one++;
						color_pixel_two=*puntero_two++;


						color_pixel_final=get_gigascreen_color(color_pixel_one,color_pixel_two);

	                                                                                                
                                                scr_putpixel_zoom_rainbow(x+bit,y,color_pixel_final);
                                        }       
                        }               
                        else {
				puntero_one+=8;
				puntero_two+=8;
			}
                        
                }       
        }       
        
        


	}

	screen_switch_rainbow_buffer();
}

//Refresco pantalla con rainbow


void scr_refresca_pantalla_rainbow_comun(void)
{

	if (gigascreen_enabled.v) {
		scr_refresca_pantalla_rainbow_comun_gigascreen();
		return;
	}

	
	//aqui no tiene sentido (o si?) el modo simular video zx80/81 en spectrum
	int ancho,alto;

	ancho=get_total_ancho_rainbow();
	alto=get_total_alto_rainbow();

	int x,y,bit;
	
	//margenes de zona interior de pantalla. Para overlay menu
	int margenx_izq=screen_total_borde_izquierdo*border_enabled.v;
	int margenx_der=screen_total_borde_izquierdo*border_enabled.v+256;
	int margeny_arr=screen_borde_superior*border_enabled.v;
	int margeny_aba=screen_borde_superior*border_enabled.v+192;
	//para overlay menu tambien
	//int fila;
	//int columna;
	
	z80_int color_pixel;
	//z80_int *puntero;
	z80_long_int *puntero;

	puntero=rainbow_buffer;
	int dibujar;

	for (y=0;y<alto;y++) {
		for (x=0;x<ancho;x+=8) {
			dibujar=1;
			
			//Ver si esa zona esta ocupada por texto de menu u overlay
			
			if (y>=margeny_arr && y<margeny_aba && x>=margenx_izq && x<margenx_der) {
				
				
				//normalmente a 48
				//int screen_total_borde_izquierdo;
			
				if (!scr_ver_si_refrescar_por_menu_activo( (x-margenx_izq)/8, (y-margeny_arr)/8) ) 
					dibujar=0;
				
			}
					
					
			if (dibujar==1) {
			
					for (bit=0;bit<8;bit++) {
			

						//printf ("x: %d y: %d\n",x,y);

						//color_pixel=*puntero++;

						//scr_putpixel_zoom_rainbow(x+bit,y,color_pixel);
						unsigned int colorrgb=*puntero;
						puntero ++;
void scrsdl_putpixel_rgb(int x,int y,unsigned int rgb);
						scrsdl_putpixel_rgb(x+bit,y,colorrgb);
					}
			}
			else puntero+=8;
			
		}
	}


}



//Refresco pantalla sin rainbow
void scr_refresca_pantalla_comun(void)
{
	int x,y,bit;
        z80_int direccion,dir_atributo;
        z80_byte byte_leido;
        int color=0;
        int fila;
        //int zx,zy;

        z80_byte attribute,ink,paper,bright,flash,aux;


	if (simulate_screen_zx8081.v==1) {
		//simular modo video zx80/81
		scr_simular_video_zx8081();
		return;
	}


       z80_byte *screen=get_base_mem_pantalla();

        //printf ("dpy=%x ventana=%x gc=%x image=%x\n",dpy,ventana,gc,image);
	z80_byte x_hi;

        for (y=0;y<192;y++) {
                //direccion=16384 | devuelve_direccion_pantalla(0,y);

                //direccion=16384 | sprite_table[(y<<5)];
                direccion=sprite_table[(y<<5)];


                fila=y/8;
                dir_atributo=6144+(fila*32);
                for (x=0,x_hi=0;x<32;x++,x_hi +=8) {


			//Ver en casos en que puede que haya menu activo y hay que hacer overlay
			if (scr_ver_si_refrescar_por_menu_activo(x,fila)) {

                	        byte_leido=screen[direccion];
	                        attribute=screen[dir_atributo];


        	                ink=attribute &7;
                	        paper=(attribute>>3) &7;
	                        bright=(attribute) &64;
        	                flash=(attribute)&128;
                	        if (flash) {
                        	        //intercambiar si conviene
	                                if (estado_parpadeo.v) {
        	                                aux=paper;
                	                        paper=ink;
	                                        ink=aux;
        	                        }
                	        }

				if (bright) {
					ink +=8;
					paper +=8;
				}
	
                        	for (bit=0;bit<8;bit++) {

					color= ( byte_leido & 128 ? ink : paper );
					scr_putpixel_zoom(x_hi+bit,y,color);

	                                byte_leido=byte_leido<<1;
        	                }
			}

			//temp
			//else {
			//	printf ("no refrescamos zona x %d fila %d\n",x,fila);
			//}


                        direccion++;
			dir_atributo++;
                }

        }

}


//Rutina usada por todos los drivers para escribir caracteres en pantalla en zx8081, en rutina de refresco que
//lee directament de DFILE
void scr_putchar_zx8081_comun(int x,int y, z80_byte caracter)
{


	z80_bit inverse;
	z80_int direccion;


	if (caracter>127) {
        	inverse.v=1;
	        caracter-=128;
	}

	else inverse.v=0;

	//con los caracteres fuera de rango, devolvemos '?'
	if (caracter>63) caracter=15;

        if (MACHINE_IS_ZX80) direccion=0x0E00;
        else direccion=0x1E00;

        scr_putsprite_zx8081(direccion+caracter*8,x,y,inverse);

}




void scr_refresca_pantalla_y_border_zx8081(void)
{


        //modo caracteres alta resolucion- rainbow - metodo nuevo
        if (rainbow_enabled.v==1) {
		scr_refresca_pantalla_rainbow_comun();
		return;
	}




        //simulacion pantalla negra fast
        if (hsync_generator_active.v==0) {

                if (video_fast_mode_next_frame_black!=LIMIT_FAST_FRAME_BLACK) {
			video_fast_mode_next_frame_black++;
		}

		if (video_fast_mode_next_frame_black==LIMIT_FAST_FRAME_BLACK) {
                        debug_printf(VERBOSE_DEBUG,"Detected fast mode");
			//forzar refresco de border
                        modificado_border.v=1;

                }
        }




                if (border_enabled.v) {
                        //ver si hay que refrescar border
                        if (modificado_border.v)
                        {
				//printf ("refrescamos border\n");
                                scr_refresca_border_zx8081();
                                modificado_border.v=0;
				//sleep (1);
                        }

                }

	//modo caracteres normal
        if (rainbow_enabled.v==0) scr_refresca_pantalla_zx8081();


}



void scr_refresca_pantalla_zx8081(void)
{
        int x,y;
        z80_byte caracter;

        //zx81
        z80_int video_pointer;

	//puntero pantalla en DFILE
        video_pointer=peek_word_no_time(0x400C);


	//Pruebas alterando video pointer para ver si funcionan los juegos flicker free
	//este medio funciona: Space\ Invaders\ 1K\ \(Macronics\ 1981\)\ Reconstruction.o  --zx8081mem 3

	while (video_pointer>ramtop_zx8081) {
		debug_printf (VERBOSE_DEBUG,"invalid video_pointer: %d",video_pointer);
		video_pointer -=0x4000;
                debug_printf (VERBOSE_DEBUG,"new video_pointer: %d",video_pointer);

	}

		

        //se supone que el primer byte es 118 . saltarlo
        video_pointer++;
        y=0;
        x=0;
        while (y<24) {

                caracter=memoria_spectrum[video_pointer++];
                if (caracter==118) {
                        //rellenar con espacios hasta final de linea
				//if (x<32) printf ("compressed line %d \n",y);
                                for (;x<32;x++) {
					if (scr_ver_si_refrescar_por_menu_activo(x,y)) scr_putchar_zx8081(x,y,0);
                                }
                                y++;

                                x=0;
                }

                else {
			if (scr_ver_si_refrescar_por_menu_activo(x,y)) scr_putchar_zx8081(x,y,caracter);

                        x++;

                        if (x==32) {
                                if (memoria_spectrum[video_pointer]!=118) 
					debug_printf (VERBOSE_DEBUG,"End of line %d is not 118 opcode. Is: 0x%x",y,memoria_spectrum[video_pointer]);
                                //saltamos el HALT que debe haber en el caso de linea con 32 caracteres
                                video_pointer++;
                                x=0;
                                y++;
                        }

                }


    }

}

void load_screen(char *scrfile)
{

	if (MACHINE_IS_SPECTRUM) {
		debug_printf (VERBOSE_INFO,"Loading Screen File");
		FILE *ptr_scrfile;
		ptr_scrfile=fopen(scrfile,"rb");
                if (!ptr_scrfile) {
			debug_printf (VERBOSE_ERR,"Unable to open Screen file");
		}

		else {
        
			if (MACHINE_IS_SPECTRUM_16_48) fread(memoria_spectrum+16384,1,6912,ptr_scrfile);

			else {

				//modo 128k. cargar en pagina 5 o 7
				int pagina=5;
				if ( (puerto_32765&8) ) pagina=7;

				fread(ram_mem_table[pagina],1,6912,ptr_scrfile);
			}



			fclose(ptr_scrfile);

		}

	}

	else {
		debug_printf (VERBOSE_ERR,"Screen loading only allowed on Spectrum models");
	}

}

void save_screen(char *scrfile)
{

                 if (MACHINE_IS_SPECTRUM) {
                                debug_printf (VERBOSE_INFO,"Saving Screen File");

FILE *ptr_scrfile;
                                  ptr_scrfile=fopen(scrfile,"wb");
                                  if (!ptr_scrfile)
                                {
                                      debug_printf (VERBOSE_ERR,"Unable to open Screen file");
                                  }
                                else {

                                   if (MACHINE_IS_SPECTRUM_16_48)
						fwrite(memoria_spectrum+16384, 1, 6912, ptr_scrfile);

				else {

                                        //modo 128k. grabar pagina 5 o 7
                                        int pagina=5;
                                        if ( (puerto_32765&8) ) pagina=7;
					fwrite(ram_mem_table[pagina], 1, 6912, ptr_scrfile);
                                  }


                                  fclose(ptr_scrfile);

                                }
                        }

                        else {
                                debug_printf (VERBOSE_ERR,"Screen saving only allowed on Spectrum models");
                        }


}


//Guardar en buffer rainbow una linea del caracter de zx8081. usado en modo de video real
void screen_store_scanline_char_zx8081(int x,int y,z80_byte byte_leido,z80_byte caracter,int inverse)
{
	int bit;
        z80_byte color;

	z80_byte colortinta=0;
	z80_byte colorpapel=15;

	//Si modo chroma81 y lo ha activado

	if (color_es_chroma() ) {
		//ver modo
		if ((chroma81_port_7FEF&16)!=0) {
			//1 attribute file
			chroma81_return_mode1_colour(reg_pc,&colortinta,&colorpapel);
			//printf ("modo 1\n");
		}
		else {
			//0 character code
			//tablas colores van primero para los 64 normales y luego para los 64 inversos
			if (inverse) {
				caracter +=64;	
			}

			z80_int d=caracter*8+0xc000;
			d=d+(y&7);
			z80_byte leido=peek_byte_no_time(d);
			colortinta=leido&15;
			colorpapel=(leido>>4)&15;

			//printf ("modo 0\n");

		}
	}



        for (bit=0;bit<8;bit++) {
		if (byte_leido & 128 ) color=colortinta;
		else color=colorpapel;

		rainbow_buffer[y*get_total_ancho_rainbow()+x+bit]=color;

		byte_leido=(byte_leido&127)<<1;

        }

}

/*
void screen_store_scanline_char_zx8081_normal(int x,int y,z80_byte byte_leido,z80_int direccion_sprite)
{
        int bit;
        z80_byte color;

        z80_byte colortinta=0;
        z80_byte colorpapel=15;


        for (bit=0;bit<8;bit++) {
                if (byte_leido & 128 ) color=colortinta;
                else color=colorpapel;

                rainbow_buffer[y*get_total_ancho_rainbow()+x+bit]=color;

                byte_leido=(byte_leido&127)<<1;

        }

}
*/

void screen_store_scanline_char_zx8081_border_scanline(int x,int y,z80_byte byte_leido)
{
        int bit;
        z80_byte color;

        z80_byte colortinta=0;
        z80_byte colorpapel=15;

        //Si modo chroma81 y lo ha activado
        if (color_es_chroma() ) {
		//color border
		colorpapel=chroma81_port_7FEF&15;
        }



        for (bit=0;bit<8;bit++) {
                if (byte_leido & 128 ) color=colortinta;
                else color=colorpapel;

                rainbow_buffer[y*get_total_ancho_rainbow()+x+bit]=color;

                byte_leido=(byte_leido&127)<<1;

        }

}


void screen_store_scanline_rainbow_border_comun(z80_int *puntero_buf_rainbow,int xinicial)
{

	return;
        
	int inicio_retrace_horiz=(256+screen_total_borde_derecho)/2;
	int final_retrace_horiz=inicio_retrace_horiz+screen_invisible_borde_derecho/2;

	//X inicial de nuestro bucle. Siempre empieza en la zona de display-> al acabar borde izquierdo
	int x=screen_total_borde_izquierdo;

	z80_byte border_leido;

	//Primer color siempre se define en el array posicion 0
	z80_int last_color;

	//Para modo interlace
	int y=t_scanline_draw;

	int indice_border=0;

	//Hay que recorrer todo el array del border
	for (;indice_border<screen_testados_linea;indice_border++) {
                //obtenemos si hay cambio de border
                border_leido=buffer_border[indice_border];
                if (border_leido!=255) {
                        last_color=border_leido;
                        //if (indice_border!=0) printf ("cambio color en indice_border=%d color=%d\n",indice_border,last_color);


			/*
			if (ulaplus_presente.v && ulaplus_enabled.v) {
				//Si es ulaplus, modos <3, color border es el del BRIGHT 0/FLASH 0 PAPER 0 colour
				if (ulaplus_mode<3) last_color=ULAPLUS_INDEX_FIRST_COLOR+8;

				//modos >=3, color sale del puerto pero indexado a ulaplus 0..7
				else {
					last_color=last_color+ULAPLUS_INDEX_FIRST_COLOR;
				}
			}
			*/

			//Modos ulaplus (cualquiera) el color del border es del puerto 254, indexado a la tabla de paper
			//TODO: Ver si esto es asi realmente
			if (ulaplus_presente.v && ulaplus_enabled.v) {
				last_color=last_color+ULAPLUS_INDEX_FIRST_COLOR+8;
			}

                }


		//Si estamos en x a partir del parametro inicial y Si no estamos en zona de retrace horizontal, dibujar border e incrementar posicion
		if (x>=xinicial) {

			//si nos pasamos de border izquierdo

			if ( (indice_border<inicio_retrace_horiz || indice_border>=final_retrace_horiz) ) {
				//Por cada t_estado van 2 pixeles
				if (if_store_scanline_interlace(y)) {
					store_value_rainbow(puntero_buf_rainbow,last_color);
					store_value_rainbow(puntero_buf_rainbow,last_color);
				}
				else {
					//incrementamos puntero sin tocar contenido
					puntero_buf_rainbow +=2;
				}
			}

			//Para modo interlace
			if (indice_border==inicio_retrace_horiz) y++;
		}

		//Por cada t_estado van 2 pixeles
		x+=2;

	}
}

//Guardar en buffer rainbow linea actual de borde superior o inferior
void screen_store_scanline_rainbow_border_comun_supinf(void)
{

	return;

	int scanline_copia=t_scanline_draw-screen_invisible_borde_superior;

	z80_int *puntero_buf_rainbow;

	int x=screen_total_borde_izquierdo;
	
	//printf ("%d\n",scanline_copia*get_total_ancho_rainbow());
	//esto podria ser un contador y no hace falta que lo recalculemos cada vez. TODO
	puntero_buf_rainbow=&rainbow_buffer[scanline_copia*get_total_ancho_rainbow()+x];	
	
	//Empezamos desde x en zona display
	screen_store_scanline_rainbow_border_comun(puntero_buf_rainbow,x );
	
		
}	


/*
Macro de obtencion de color de un pixel
Nota: Quiza el if (ulaplus_presente.v && ulaplus_enabled.v) consume algunos ciclos de cpu,
y se podria haber evitado, haciendo rutina de screen_store_scanline_rainbow_solo_display solo para ulaplus,
con lo cual el if se evitaria y ahorrariamos ciclos de cpu...
Esto a la practica no ahorra cpu apreciable, y ademas, se tendria
otra rutina de screen_store_scanline_rainbow_solo_display diferente de la que no tiene ulaplus,
agregando duplicidad de funciones sin verdadera necesidad...
*/
#define GET_PIXEL_COLOR \
			if (ulaplus_presente.v && ulaplus_enabled.v) {  \
				GET_PIXEL_ULAPLUS_COLOR \
			} \
			else { \
                        ink=attribute &7; \
                        paper=(attribute>>3) &7; \
                        bright=(attribute)&64; \
                        flash=(attribute)&128; \
                        if (flash) { \
                                if (estado_parpadeo.v) { \
                                        aux=paper; \
                                        paper=ink; \
                                        ink=aux; \
                                } \
                        } \
			\
			if (bright) {	\
				paper+=8; \
				ink+=8; \
			} \
			} \


//Macro de obtencion de color de un pixel con ulaplus
#define GET_PIXEL_ULAPLUS_COLOR \
                        ink=attribute &7; \
                        paper=(attribute>>3) &7; \
                        bright=( (attribute)&64 ? 1 : 0 )  ; \
                        flash=( (attribute)&128 ? 1 : 0 ) ; \
			\
			z80_int temp_color=(flash * 2 + bright) * 16 + ULAPLUS_INDEX_FIRST_COLOR ; \
			ink=temp_color+ink; \
			paper=temp_color+paper+8;  \


//para snow effect
z80_byte byte_leido_antes;
int snow_effect_counter=0;


//cada cuantos bytes hacemos snow
//#define MIN_SNOW_EFFECT_COUNTER 14
#define MIN_SNOW_EFFECT_COUNTER 45

int snow_effect_counter_hang=0;
//contador que dice cuando se reseteara
#define MIN_SNOW_EFFECT_RESET_COUNTER 40000


//Devuelve 1 si reg_i apunta a memoria contended
//Si no apunta a memoria contended, reseteamos a 0 contador de snow_effect_counter_hang
//asi en juegos como VECTRON, que hacen el efecto intencionadamente,
//como no lo hace durante mucho tiempo, la maquina no se resetea (PC no va a 0)
int snow_effect_si_contend(void)
{
	//En modos 48k
	if (MACHINE_IS_SPECTRUM_16_48) {
		if (reg_i>=64 && reg_i<=127) return 1;
		else {
			snow_effect_counter_hang=0;
			return 0;
		}
	}

	//Otros casos, suponemos 128k
	z80_int segmento;
	segmento=reg_i / 64;
	if (contend_pages_actual[segmento]) return 1;
	else {
		snow_effect_counter_hang=0;
		return 0;
	}
}

//Devuelve 1 si hay que hacer snow effect
int si_toca_snow_effect(void)
{


	//Maquinas +2A, +3 no tienen efecto snow
	if (MACHINE_IS_SPECTRUM_P2A) return 0;



	if (snow_effect_si_contend () ) {
		snow_effect_counter++;

                                                //cada X bytes, perdemos uno
                                                if (snow_effect_counter==MIN_SNOW_EFFECT_COUNTER) {
                                                        snow_effect_counter=0;

							snow_effect_counter_hang++;

							//printf ("snow_effect_counter_hang: %d\n",snow_effect_counter_hang);

							if (snow_effect_counter_hang==MIN_SNOW_EFFECT_RESET_COUNTER) {
								snow_effect_counter_hang=0;
								//Si maquina 48k, reseteamos cuando el contador llega al limite
								if (MACHINE_IS_SPECTRUM_16_48) {
									reg_pc=0;
									//registro i ya se resetea desde la rom
									//reg_i=0;
								}
							}

							return 1;

                                                }	
	}

	return 0;

}


//Guardar en buffer rainbow la linea actual. Modos ulaplus lineales, incluido radastan. Para Spectrum. solo display
//Tener en cuenta que si border esta desactivado, la primera linea del buffer sera de display,
//en cambio, si border esta activado, la primera linea del buffer sera de border
void screen_store_scanline_rainbow_solo_display_ulaplus_lineal(void)
{

        //printf ("scan line de pantalla fisica (no border): %d\n",t_scanline_draw);

        //linea que se debe leer
        int scanline_copia=t_scanline_draw-screen_indice_inicio_pant;

        //la copiamos a buffer rainbow
        //z80_int *puntero_buf_rainbow;
        z80_long_int *puntero_buf_rainbow;
        //esto podria ser un contador y no hace falta que lo recalculemos cada vez. TODO
        int y;

        y=t_scanline_draw-screen_invisible_borde_superior;
        if (border_enabled.v==0) y=y-screen_borde_superior;

        puntero_buf_rainbow=&rainbow_buffer[ y*get_total_ancho_rainbow() ];

        puntero_buf_rainbow +=screen_total_borde_izquierdo*border_enabled.v;


        int x;
        z80_int direccion=0;
        z80_byte byte_leido;

        int color_rada;
        z80_byte *screen;



	if (ulaplus_mode==9) {
		direccion=16384;
	}

	else {
	        screen=get_base_mem_pantalla();
	}

	/*
                                modo 3 es radastan 128x96, aunque va en contra de la ultima especificacion
                                
                                modo 5: 256x96

                                modo 7: 128x192

				modo 9: 256x192. empezando siempre en 16384
	*/

	//modos 3 y 5 duplican cada pixel por alto, por tanto:
	//pixel y=0->direccion de pantalla linea 0
	//pixel y=1->direccion de pantalla linea 0
	//pixel y=2->direccion de pantalla linea 1
        //pixel y=3->direccion de pantalla linea 1
        //pixel y=4->direccion de pantalla linea 2
	//etc...

	int bytes_por_linea=0;
	int incremento_x=0;

	switch (ulaplus_mode) {
		case 3:
			//dividimos y/2
			scanline_copia/=2;
			bytes_por_linea=64;
			incremento_x=2;
		break;

                case 5:
                        //dividimos y/2
                        scanline_copia/=2;
                        bytes_por_linea=128;
			incremento_x=1;
                break;

                case 7:
                        bytes_por_linea=64;
			incremento_x=2;
                break;

		case 9:
			bytes_por_linea=128;
			incremento_x=1;
                break;


		
	}



	direccion=direccion+bytes_por_linea*scanline_copia;

        for (x=0;x<128;x+=incremento_x) {

			//temp controlar esto
			//if (direccion>22527) printf ("direccion: %d scanline_copia: %d\n",direccion,scanline_copia);

			//Cada byte tiene dos pixeles de color
			//Cada pixel duplicado en ancho en modos 3 y 7

			if (ulaplus_mode==9) {
				byte_leido=peek_byte_no_time(direccion);
			}
			else {
	                        byte_leido=screen[direccion];
			}
                        color_rada=(byte_leido>>4)+ULAPLUS_INDEX_FIRST_COLOR;

			//if (color_rada>ULAPLUS_INDEX_FIRST_COLOR) printf ("color rada: %d\n",color_rada);

			store_value_rainbow(puntero_buf_rainbow,color_rada);
			if (incremento_x==2) store_value_rainbow(puntero_buf_rainbow,color_rada);



                        color_rada=(byte_leido&15)+ULAPLUS_INDEX_FIRST_COLOR;
	
			//if (color_rada>ULAPLUS_INDEX_FIRST_COLOR) printf ("color rada: %d\n",color_rada);

			store_value_rainbow(puntero_buf_rainbow,color_rada);
			if (incremento_x==2) store_value_rainbow(puntero_buf_rainbow,color_rada);


                        direccion++;

        }


        

}



//Guardar en buffer rainbow la linea actual. Para Spectrum. solo display
//Tener en cuenta que si border esta desactivado, la primera linea del buffer sera de display,
//en cambio, si border esta activado, la primera linea del buffer sera de border
void screen_store_scanline_rainbow_solo_display(void)
{
	//si linea no coincide con entrelazado, volvemos
	if (if_store_scanline_interlace(t_scanline_draw)==0) return;

  if (t_scanline_draw>=screen_indice_inicio_pant && t_scanline_draw<screen_indice_fin_pant) {


	if (ulaplus_presente.v && ulaplus_enabled.v && ulaplus_mode>=3) {
		screen_store_scanline_rainbow_solo_display_ulaplus_lineal();
		return;
	}

        //printf ("scan line de pantalla fisica (no border): %d\n",t_scanline_draw);

        //linea que se debe leer
        int scanline_copia=t_scanline_draw-screen_indice_inicio_pant;

        //la copiamos a buffer rainbow
        //z80_int *puntero_buf_rainbow;
        z80_long_int *puntero_buf_rainbow;
        //esto podria ser un contador y no hace falta que lo recalculemos cada vez. TODO
        int y;

        y=t_scanline_draw-screen_invisible_borde_superior;
        if (border_enabled.v==0) y=y-screen_borde_superior;

        puntero_buf_rainbow=&rainbow_buffer[ y*get_total_ancho_rainbow() ];

        puntero_buf_rainbow +=screen_total_borde_izquierdo*border_enabled.v;


        int x,bit;
        z80_int direccion,dir_atributo;
        z80_byte byte_leido;


        int color=0;
        int fila;

        z80_byte attribute,bright,flash;
	z80_int ink,paper,aux;


        z80_byte *screen=get_base_mem_pantalla();

        direccion=sprite_table[(scanline_copia<<5)];


        fila=scanline_copia/8;
        dir_atributo=6144+(fila*32);

	
        for (x=0;x<32;x++) {


                        byte_leido=screen[direccion];

                        attribute=buffer_atributos[x];


                                //snow effect
				//TODO: ver exactamente el comportamiento real del snow effect
                                if (snow_effect_enabled.v==1) {

					if (si_toca_snow_effect() ) {
					
							//Byte leido es byte anterior
							//byte leido es (DIR & FF00) | reg_r;
							z80_int puntero_snow;
							puntero_snow=direccion & 0xFF00;
							
							z80_byte calculado_reg_r=(reg_r&127) | (reg_r_bit7&128);
			
							//y sumamos a reg_r columna*2 (esto simula incremento de registro R)	
							//1 linea=224 estados. 1 instruccion=3 estados=1 incremento de R
							//en una linea, 74 instrucciones simples=74 incrementos de R
							//32 columnas * 2 = 64 = casi 74
					 		calculado_reg_r +=x*2;


							puntero_snow |=calculado_reg_r;

							byte_leido=screen[puntero_snow];

							//atributo debe ser el de misma columna que el byte que lee la ula
                        				attribute=buffer_atributos[puntero_snow&31];

        	                        }
				}


			GET_PIXEL_COLOR

                        for (bit=0;bit<8;bit++) {

				//ula color delay para Inves. No para todos las posiciones de x
				//TODO: manera de desactivarlo??
				if (machine_type==2 && x!=0 && (x%inves_ula_delay_factor)==0 ) {
					//Cuando hay parpadeo no sucede esto
					if (bit==0 && !flash) {	
						//if ((screen_atributo[dir_atributo-1] & 128)==0) {
						if ((buffer_atributos[x-1] & 128)==0) {
		                	        	//attribute=screen_atributo[dir_atributo-1];
                        				attribute=buffer_atributos[x-1];
		        	                        GET_PIXEL_COLOR
						}
					}

					//resto de bits color normal

					else if (bit==1) {                   
                       				attribute=buffer_atributos[x];
                                                GET_PIXEL_COLOR
                                        }

				}


				color= ( byte_leido & 128 ? ink : paper ) ;	

                                //store_value_rainbow(puntero_buf_rainbow,color);
				unsigned int color32=spectrum_colortable[color];
				*puntero_buf_rainbow=color32;
				puntero_buf_rainbow ++;

                                byte_leido=byte_leido<<1;
                        }
			direccion++;
                	dir_atributo++;

        	}


	}

}


//Guardar en buffer rainbow la linea actual-solo border. Para Spectrum
void screen_store_scanline_rainbow_solo_border(void)
{

	if (border_enabled.v==0) return;

        //si linea no coincide con entrelazado, volvemos
        //if (if_store_scanline_interlace(t_scanline_draw)==0) return;


        //zona de border superior o inferior
        if ( (t_scanline_draw>=screen_invisible_borde_superior && t_scanline_draw<screen_indice_inicio_pant) ||
             (t_scanline_draw>=screen_indice_fin_pant && t_scanline_draw<screen_indice_fin_pant+screen_total_borde_inferior) 	
	   ) {
           
		screen_store_scanline_rainbow_border_comun_supinf();
        }

        //zona de border + pantalla + border
        else if (t_scanline_draw>=screen_indice_inicio_pant && t_scanline_draw<screen_indice_fin_pant) {

	        //linea que se debe leer
	        //int scanline_copia=t_scanline_draw-screen_indice_inicio_pant;

        	z80_int *puntero_buf_rainbow;
	        //esto podria ser un contador y no hace falta que lo recalculemos cada vez. TODO
        	int y;

	        y=t_scanline_draw-screen_invisible_borde_superior;

		//nos situamos en borde derecho
	        puntero_buf_rainbow=&rainbow_buffer[ y*get_total_ancho_rainbow()+screen_total_borde_izquierdo+256 ];


	        screen_store_scanline_rainbow_border_comun(puntero_buf_rainbow,screen_total_borde_izquierdo+256);

        }

	//primera linea de border. Realmente empieza una linea atras y acaba la primera linea de borde 
	//con el borde izquierdo de la primera linea visibble
	
	else if ( t_scanline_draw==screen_invisible_borde_superior-1 ) {
		z80_int *puntero_buf_rainbow;

		puntero_buf_rainbow=&rainbow_buffer[0];

		int xinicial=screen_total_borde_izquierdo+256+screen_total_borde_derecho+screen_invisible_borde_derecho;
		//printf ("primera linea de borde: %d empezamos en xinicial: %d \n",t_scanline_draw,xinicial);

		screen_store_scanline_rainbow_border_comun(puntero_buf_rainbow,xinicial);

	}
	
	


}




void siguiente_frame_pantalla(void)
{

	frames_total++;
                        if (frames_total==50) {

                              //contador framedrop
                                if (framedrop_total!=0) {
					//si no hay frameskip forzado
                                        if (!frameskip && ultimo_fps!=50) debug_printf(VERBOSE_INFO,"FPS: %d",ultimo_fps);
                                }


				ultimo_fps=50-framedrop_total;

                                framedrop_total=0;
                                frames_total=0;
                        }
}


z80_byte *vofile_buffer;

void print_helper_aofile_vofile(void)
{

         int ancho,alto;


        //ancho=LEFT_BORDER_NO_ZOOM+ANCHO_PANTALLA+RIGHT_BORDER_NO_ZOOM;
        //alto=TOP_BORDER_NO_ZOOM+ALTO_PANTALLA+BOTTOM_BORDER_NO_ZOOM;

        ancho=screen_get_window_width_no_zoom();
        alto=screen_get_window_height_no_zoom();


#define AOFILE_TYPE_RAW 0
#define AOFILE_TYPE_WAV 1
extern int aofile_type;

        char buffer_texto_video[500];
        char buffer_texto_audio[500];
        sprintf(buffer_texto_video,"-demuxer rawvideo -rawvideo fps=%d:w=%d:h=%d:format=bgr24",50/vofile_fps,ancho,alto);

	if (aofile_type==AOFILE_TYPE_RAW) {
        	sprintf(buffer_texto_audio,"-audiofile %s -audio-demuxer rawaudio -rawaudio channels=1:rate=%d:samplesize=1",aofilename,FRECUENCIA_SONIDO);
	}

	if (aofile_type==AOFILE_TYPE_WAV) {
		sprintf(buffer_texto_audio,"-audiofile %s",aofilename);
	}




	if (aofile_inserted.v==1 && vofile_inserted.v==0) {

		if (aofile_type==AOFILE_TYPE_RAW) {
			debug_printf(VERBOSE_INFO,"You can convert with: sox  -t .raw -r %d -b 8 -e unsigned -c 1 %s outputfile.wav",FRECUENCIA_SONIDO,aofilename);
		}


		//debug_printf(VERBOSE_INFO,"You can play it with: mplayer %s %s",buffer_texto_audio,aofilename);
	}

	if (aofile_inserted.v==0 && vofile_inserted.v==1) {
		debug_printf(VERBOSE_INFO,"You can play it with : mplayer %s %s",buffer_texto_video,vofilename);
	}

	if (aofile_inserted.v==1 && vofile_inserted.v==1) {
		debug_printf(VERBOSE_INFO,"You can play it with : mplayer %s %s %s",buffer_texto_video,buffer_texto_audio,vofilename);
	}

}

void init_vofile(void)
{

                //debug_printf (VERBOSE_INFO,"Initializing Audio Output File");

                ptr_vofile=fopen(vofilename,"wb");
                //printf ("ptr_vofile: %p\n",ptr_vofile);

                if (!ptr_vofile)
                {
                        debug_printf(VERBOSE_ERR,"Unable to create vofile %s",vofilename);
                        vofilename=NULL;
                        vofile_inserted.v=0;
                        return;
                }

         int ancho,alto,tamanyo;
        //ancho=LEFT_BORDER_NO_ZOOM+ANCHO_PANTALLA+RIGHT_BORDER_NO_ZOOM;
        //alto=TOP_BORDER_NO_ZOOM+ALTO_PANTALLA+BOTTOM_BORDER_NO_ZOOM;

        ancho=screen_get_window_width_no_zoom();
        alto=screen_get_window_height_no_zoom();


        tamanyo=ancho*alto;

        vofile_buffer=malloc(tamanyo*3);
        if (vofile_buffer==NULL) {
                cpu_panic("Error allocating video output buffer");
        }

	//Hay que activar realvideo dado que el video se genera en base a esto
	enable_rainbow();


	vofile_frame_actual=0;

        vofile_inserted.v=1;

        debug_printf(VERBOSE_INFO,"Writing video output file, format raw, %d FPS, %d X %d, bgr24",50/vofile_fps,ancho,alto);
	print_helper_aofile_vofile();
}




unsigned char buffer_rgb[3];


/* 

Paleta antigua para vofile no usada ya. Usamos misma paleta activa de color

// Paletas VGA en 6 bit, Paleta archivo raw 8 bit, multiplicar por 4
#define BRI0      (42+5)*4
#define BRI1      (16)*4

// Tabla para los colores reales 

unsigned char tabla_colores[]={
//      RED       GREEN     BLUE                 G R B 
    	0,	  0,        0,			// 0 En SP: 0 0 0 Black 
    	0,        0,        BRI0,	      	// 1        0 0 1 Blue 
	BRI0,     0,	    0,         		// 2        0 1 0 Red 
	BRI0,	  0,	    BRI0,      		// 3        0 1 1 Magenta 
	0,	  BRI0,	    0,			// 4        1 0 0 Green 
	0,	  BRI0,	    BRI0,		// 5        1 0 1 Cyan 
	BRI0,	  BRI0,	    0,			// 6        1 1 0 Yellow 
	BRI0,	  BRI0,	    BRI0,		// 7        1 1 1 White 


//With brightness 

	0,	  0,        0,			// 0        0 0 0 Black 
    	0,        0,        BRI0+BRI1, 		// 1        0 0 1 Blue 
	BRI0+BRI1,0,	    0,         		// 2        0 1 0 Red 
	BRI0+BRI1,0,	    BRI0+BRI1, 		// 3        0 1 1 Magenta 
	0,	  BRI0+BRI1,0,			// 4        1 0 0 Green 
	0,	  BRI0+BRI1,BRI0+BRI1,		// 5        1 0 1 Cyan 
	BRI0+BRI1,BRI0+BRI1,0,			// 6        1 1 0 Yellow 
	BRI0+BRI1,BRI0+BRI1,BRI0+BRI1,		// 7        1 1 1 White 

};

*/

void convertir_paleta(z80_int valor)
{

	unsigned char valor_r,valor_g,valor_b;

	//colores originales
	//int color=spectrum_colortable_original[valor];

	//colores de tabla activa
	int color=spectrum_colortable[valor];


	valor_r=(color & 0xFF0000) >> 16;
	valor_g=(color & 0x00FF00) >> 8;
	valor_b= color & 0x0000FF;


	buffer_rgb[0]=valor_b;
	buffer_rgb[1]=valor_g;
	buffer_rgb[2]=valor_r;

}

/*
	convertir_paleta(valor);
   fwrite( &buffer_rgb, 1, 3, fichero_out);
*/	 

int vofile_add_watermark_aux_indice_xy(int x,int y)
{
	         int ancho;
        //ancho=LEFT_BORDER_NO_ZOOM+ANCHO_PANTALLA+RIGHT_BORDER_NO_ZOOM;
        ancho=screen_get_window_width_no_zoom();


	return ancho*y*3+x*3;
}

void vofile_add_watermark(void)
{


	int x,y;
	int pos;

	//offset respecto a la esquina superior
	int offset_x=8;
	int offset_y=8;

	//Tamanyo de la Z
	int z_size=24;

	//Parte de arriba de la Z. 2 lineas de grueso
	for (x=0;x<z_size;x++) {
		convertir_paleta(x&15);
		pos=vofile_add_watermark_aux_indice_xy(x+offset_x,0+offset_y);
		vofile_buffer[pos++]=buffer_rgb[0];
		vofile_buffer[pos++]=buffer_rgb[1];
		vofile_buffer[pos++]=buffer_rgb[2];

                pos=vofile_add_watermark_aux_indice_xy(x+offset_x,1+offset_y);
                vofile_buffer[pos++]=buffer_rgb[0];
                vofile_buffer[pos++]=buffer_rgb[1];
                vofile_buffer[pos++]=buffer_rgb[2];

	}

	//Diagonal de la z. 2 pixeles de ancho
        for (y=1,x=z_size-2;y<z_size-1;x--,y++) {
        	convertir_paleta(x&15);
                int pos=vofile_add_watermark_aux_indice_xy(x+offset_x,y+offset_y);
                vofile_buffer[pos++]=buffer_rgb[0];
                vofile_buffer[pos++]=buffer_rgb[1];
                vofile_buffer[pos++]=buffer_rgb[2];
		
                vofile_buffer[pos++]=buffer_rgb[0];
                vofile_buffer[pos++]=buffer_rgb[1];
                vofile_buffer[pos++]=buffer_rgb[2];

        }

        //Parte de abajo de la Z. 2 lineas de grueso
        for (x=0;x<z_size;x++) {
        	convertir_paleta(x&15);
                pos=vofile_add_watermark_aux_indice_xy(x+offset_x,z_size-2+offset_y);
                vofile_buffer[pos++]=buffer_rgb[0];
                vofile_buffer[pos++]=buffer_rgb[1];
                vofile_buffer[pos++]=buffer_rgb[2];

                pos=vofile_add_watermark_aux_indice_xy(x+offset_x,z_size-1+offset_y);
                vofile_buffer[pos++]=buffer_rgb[0];
                vofile_buffer[pos++]=buffer_rgb[1];
                vofile_buffer[pos++]=buffer_rgb[2];


        }



}

void vofile_send_frame(z80_int *buffer)
{

        if (vofile_inserted.v==0) return;

	vofile_frame_actual++;
	//printf ("actual %d tope %d\n",vofile_frame_actual,vofile_fps);
	if (vofile_frame_actual!=vofile_fps) return;
	vofile_frame_actual=0;

        int escritos;

         int ancho,alto,tamanyo;
        //ancho=LEFT_BORDER_NO_ZOOM+ANCHO_PANTALLA+RIGHT_BORDER_NO_ZOOM;
        //alto=TOP_BORDER_NO_ZOOM+ALTO_PANTALLA+BOTTOM_BORDER_NO_ZOOM;

        ancho=screen_get_window_width_no_zoom();
        alto=screen_get_window_height_no_zoom();


        tamanyo=ancho*alto;

	int origen_buffer=0;
	z80_byte *destino_buffer;
	destino_buffer=vofile_buffer;
	z80_byte byte_leido;

	for (;origen_buffer<tamanyo;origen_buffer++) {
		byte_leido=*buffer++;
		convertir_paleta(byte_leido);
	 	*destino_buffer++=buffer_rgb[0];
	 	*destino_buffer++=buffer_rgb[1];
	 	*destino_buffer++=buffer_rgb[2];
	}


        //printf ("buffer: %p ptr_vofile: %p\n",buffer,ptr_vofile);
        //escritos=fwrite(buffer, 1, tamanyo, ptr_vofile);


	//agregamos marca de agua
	vofile_add_watermark();

	escritos=fwrite(vofile_buffer,1,tamanyo*3, ptr_vofile);
        if (escritos!=tamanyo*3) {

                        debug_printf(VERBOSE_ERR,"Unable to write to vofile %s",vofilename);
                        vofilename=NULL;
                        vofile_inserted.v=0;

                //debug_printf(VERBOSE_ERR,"Bytes escritos: %d\n",escritos);
                //cpu_panic("Error writing vofile\n");
        }


}

void close_vofile(void)
{

        if (vofile_inserted.v==0) {
                debug_printf (VERBOSE_INFO,"Closing vofile. But already closed");
                return;
        }

        vofile_inserted.v=0;


	debug_printf (VERBOSE_INFO,"Closing vofile type RAW");
	fclose(ptr_vofile);
}

//Resetea algunos parametros de drivers de video, ya seteados a 0 al arrancar
//se llama aqui al cambiar el driver de video en caliente
void screen_reset_scr_driver_params(void)
{
	scr_tiene_colores=0;

	screen_stdout_driver=0;

	screen_refresh_menu=0;

	scr_messages_debug=NULL;
}

void screen_set_colour_normal(int index, int colour)
{

	spectrum_colortable_normal[index]=colour;


#ifdef COMPILE_AA
        //para aalib, tiene su propia paleta que hay que actualizar
        if (!strcmp(scr_driver_name,"aa")) {
                scraa_setpalette(index,(colour >> 16) & 0xFF, (colour >> 8) & 0xFF, (colour) & 0xFF );
        }
#endif


}

void screen_init_colour_table(void)
{

                	int i,j,r,g,b,r2,g2,b2,valorgris;






		if (screen_gray_mode!=0) {


//Modo de grises activo
//0: colores normales
//1: componente Blue
//2: componente Green
//4: componente Red
//Se pueden sumar para diferentes valores

#define GRAY_MODE_CONST 30
#define GRAY_MODE_CONST_BRILLO 20


	                for (i=0;i<16;i++) {
				valorgris=(i&7)*GRAY_MODE_CONST;
				
				if (i>=8) valorgris +=GRAY_MODE_CONST_BRILLO;

				VALOR_GRIS_A_R_G_B

				screen_set_colour_normal(i,(r<<16)|(g<<8)|b);

	                }

			//El color 8 es negro, con brillo 1. Pero negro igual
			screen_set_colour_normal(8,0);

			//spectrum_colortable_normal=spectrum_colortable_grises;


			//tramas de grises para Z88
			//en ese caso me baso en los mismos colores del spectrum con gris, es decir:
			//pixel on, color negro
			//pixel gris, color blanco sin brillo
			//pixel off o pantalla off: color blanco con brillo

			screen_set_colour_normal(Z88_PXCOLON,spectrum_colortable_normal[0]);
			screen_set_colour_normal(Z88_PXCOLGREY,spectrum_colortable_normal[7]);
			screen_set_colour_normal(Z88_PXCOLOFF,spectrum_colortable_normal[15]);
			screen_set_colour_normal(Z88_PXCOLSCROFF,spectrum_colortable_normal[15]);

			//trama de grises para ulaplus
			z80_byte color;
                        for (i=0;i<64;i++) {
				color=ulaplus_palette_table[i];
				ulaplus_set_final_palette_colour(i,color);
                        }
			


		}

		else {

			//si no gris
			//spectrum_colortable_normal=(int *)spectrum_colortable_original;
			int i;
			for (i=0;i<16;i++) {
				screen_set_colour_normal(i,spectrum_colortable_original[i]);
			}


			//colores para Z88
			screen_set_colour_normal(Z88_PXCOLON,z88_colortable_original[0]);
			screen_set_colour_normal(Z88_PXCOLGREY,z88_colortable_original[1]);
			screen_set_colour_normal(Z88_PXCOLOFF,z88_colortable_original[2]);
			screen_set_colour_normal(Z88_PXCOLSCROFF,z88_colortable_original[3]);

			//colores ulaplus
			//ulaplus_rgb_table
			//ULAPLUS_INDEX_FIRST_COLOR

			z80_byte color;
			for (i=0;i<64;i++) {
				color=ulaplus_palette_table[i];
				ulaplus_set_final_palette_colour(i,color);
			}

		}

		//Colores para interlaced scanlines. Linea impar mas oscura
		//copiamos del color generado del spectrum al color scanline (indice + 16)
                for (i=0;i<16;i++) {
                                b=spectrum_colortable_normal[i] & 0xFF;
                                g=(spectrum_colortable_normal[i] >> 8 ) & 0xFF;
                                r=(spectrum_colortable_normal[i] >> 16 ) & 0xFF;

                                //Valores mas oscuros para scanlines
                                r=r/2;
                                g=g/2;
                                b=b/2;

                                //printf ("%x %x %x\n",r,g,b);

                                screen_set_colour_normal(i+16,(r<<16)|(g<<8)|b);
                }


		//colores para gigascreen
		int bri0,bri1,index_giga=32;
		for (i=0;i<16;i++) {
	                for (j=0;j<16;j++) {
                                b=spectrum_colortable_normal[i&7] & 0xFF;
                                g=(spectrum_colortable_normal[i&7] >> 8 ) & 0xFF;
                                r=(spectrum_colortable_normal[i&7] >> 16 ) & 0xFF;

                                b2=spectrum_colortable_normal[j&7] & 0xFF;
                                g2=(spectrum_colortable_normal[j&7] >> 8 ) & 0xFF;
                                r2=(spectrum_colortable_normal[j&7] >> 16 ) & 0xFF;

				if (i>=8) bri0=1;
				else bri0=0;

				if (j>=8) bri1=1;
				else bri1=0;

				r=get_gigascreen_rgb_value(r,r2,bri0,bri1);
				g=get_gigascreen_rgb_value(g,g2,bri0,bri1);
				b=get_gigascreen_rgb_value(b,b2,bri0,bri1);

                                //printf ("index: %d %x %x %x\n",index_giga,r,g,b);

                                screen_set_colour_normal(index_giga++,(r<<16)|(g<<8)|b);
				
        	        }

		}




		//Si video inverso
		if (inverse_video.v==1) {
        	        for (i=0;i<EMULATOR_TOTAL_PALETTE_COLOURS;i++) {
                	        b=spectrum_colortable_normal[i] & 0xFF;
                        	g=(spectrum_colortable_normal[i] >> 8 ) & 0xFF;
	                        r=(spectrum_colortable_normal[i] >> 16 ) & 0xFF;

        	                r=r^255;
                	        g=g^255;
                        	b=b^255;

				screen_set_colour_normal(i,(r<<16)|(g<<8)|b);
			}
		}


                //inicializar tabla de colores oscuro
                for (i=0;i<EMULATOR_TOTAL_PALETTE_COLOURS;i++) {
                        b=spectrum_colortable_normal[i] & 0xFF;
                        g=(spectrum_colortable_normal[i] >> 8 ) & 0xFF;
                        r=(spectrum_colortable_normal[i] >> 16 ) & 0xFF;

                        r=r/2;
                        g=g/2;
                        b=b/2;

                        spectrum_colortable_oscuro[i]=(r<<16)|(g<<8)|b;
                }

		//Establecemos tabla actual
                spectrum_colortable=spectrum_colortable_normal;


#ifdef COMPILE_CURSES
		//Si driver curses, su paleta es diferente
		if (!strcmp(scr_driver_name,"curses")) scrcurses_inicializa_colores();
#endif


//#ifdef COMPILE_AA
//		//Si driver aa, reinicializar paleta
//		if (!strcmp(scr_driver_name,"aa")) scraa_inicializa_colores();
//#endif



}


void scr_fadeout(void)
{
        int color,i,r,g,b,j;

	scr_refresca_pantalla();


	//en curses,stdout y null no hacerlo
	if (!strcmp(scr_driver_name,"curses"))  return;
	if (!strcmp(scr_driver_name,"stdout"))  return;
	if (!strcmp(scr_driver_name,"null"))  return;

	debug_printf (VERBOSE_INFO,"Making fade out");


#ifdef COMPILE_XWINDOWS
	//parece que con shm activo, no hace fadeout en xwindows
	shm_used=0;
#endif

#define MAX_FADE 256
#define INC_FADE 10
#define TOTAL_SECONDS 1

#define SLEEPTIME (1000000*TOTAL_SECONDS/(MAX_FADE/INC_FADE))


	int spectrum_colortable_fade[EMULATOR_TOTAL_PALETTE_COLOURS];
	//int *spectrum_colortable_fade;
	
	for (j=0;j<MAX_FADE;j+=INC_FADE) {
		//spectrum_colortable_fade=malloc(sizeof(int)*16);
		spectrum_colortable=spectrum_colortable_fade;

		//printf ("%p\n",spectrum_colortable);

                for (i=0;i<EMULATOR_TOTAL_PALETTE_COLOURS;i++) {
			//printf ("i %d j %d\n",i,j);

			color=spectrum_colortable_normal[i];
                        b=color & 0xFF;
                        g=(color >> 8 ) & 0xFF;
                        r=(color >> 16 ) & 0xFF;

			r=r-j;
			g=g-j;
			b=b-j;

			if (r<0) r=0;
			if (g<0) g=0;
			if (b<0) b=0;

			

			color=(r<<16)|(g<<8)|b;

			//spectrum_colortable_normal[i]=color;
			spectrum_colortable_fade[i]=color;

			//printf ("fade. color=%06X\n",color);


			//en el caso de aalib usa una paleta diferente
#ifdef COMPILE_AA
                //Si driver aa, reinicializar paleta
                if (!strcmp(scr_driver_name,"aa")) scraa_inicializa_colores();
		//scraa_setpalette (i, r,g,b);
#endif




                }

		clear_putpixel_cache();
		modificado_border.v=1;
		all_interlace_scr_refresca_pantalla();
		screen_z88_draw_lower_screen();

		usleep(SLEEPTIME);

	}
}






void cpu_loop_refresca_pantalla(void)
{

		//printf ("saltar: %d counter %d\n",framescreen_saltar,frameskip_counter);

				//Si se ha llegado antes a final de frame, y no hay frameskip manual
                                if (framescreen_saltar==0 && frameskip_counter==0) {
                                        scr_refresca_pantalla();
                                        frameskip_counter=frameskip;
                                }


				//Si no se ha llegado a final de frame antes, o hay frameskip manual
                                else {
                                        if (frameskip_counter) frameskip_counter--;
                                        else debug_printf(VERBOSE_DEBUG,"Framedrop %d",framedrop_total);


                                        framedrop_total++;

                                }
}




//Escribe texto en pantalla empezando por en x,y, gestionando salto de linea
//de momento solo se usa en panic para xwindows y fbdev
//ultima posicion y queda guardada en screen_print_y
void screen_print(int x,int y,z80_byte tinta,z80_byte papel,char *mensaje)
{
	while (*mensaje) {
		scr_putchar_menu(x++,y,*mensaje++,tinta,papel);
		if (x==32) {
			x=0;
			y++;
		}
	}
	screen_print_y=y;
}


void screen_set_frameskip_slow_machines(void)
{

	//Parametros por defecto en Raspberry
#ifdef EMULATE_RASPBERRY


	//Frameskip 3 como minimo para realvideo
	if (rainbow_enabled.v==1) {
	        if (frameskip<3) {
        	        frameskip=3;
                	debug_printf (VERBOSE_INFO,"It is a raspberry system. With realvideo, setting frameskip to: %d",frameskip);
			return;
	        }
	}

	//Sin realvideo, frameskip 1 minimo
	if (rainbow_enabled.v==0) {
		if (frameskip<1) {
			frameskip=1;
			debug_printf (VERBOSE_INFO,"It is a raspberry system. Without realvideo, setting frameskip to: %d",frameskip);
			return;
		}
	}

	return;

#endif


	//Parametros por defecto para Cocoa
	if (!strcmp(scr_driver_name,"cocoa")) {
		//En Z88, frameskip 3
		if (MACHINE_IS_Z88) {
		        if (frameskip<3) {
                	        frameskip=3;
                        	debug_printf (VERBOSE_INFO,"It is a Mac OS X system with cocoa driver. With Z88, setting frameskip to: %d",frameskip);
				return;
			}
			return;
		}


		//Sin realvideo, forzar a frameskip 0, aunque este frameskip >0. Esto se hace porque si se entra en Z88, y se vuelve a ZX spectrum por ejemplo,
		//que vuelva el frameskip 0 y no deje frameskip 3 (el de z88)
		if (rainbow_enabled.v==0) {
		        if (frameskip>0) {
                	        frameskip=0;
                        	debug_printf (VERBOSE_INFO,"It is a Mac OS X system with cocoa driver. Without realvideo, setting frameskip to: %d",frameskip);
				return;
	                }
		}


		//Con realvideo, frameskip 1 minimo
		if (rainbow_enabled.v==1) {
		        if (frameskip<1) {
                	        frameskip=1;
                        	debug_printf (VERBOSE_INFO,"It is a Mac OS X system with cocoa driver. With realvideo, setting frameskip to: %d",frameskip);
	                }
		}


		return;
	}

}
	

//Activar rainbow y el estabilizador de imagen de zx8081
void enable_rainbow(void) {

	debug_printf (VERBOSE_INFO,"Enabling RealVideo");

	//si hay un cambio
	if (rainbow_enabled.v==0) {
        	rainbow_enabled.v=1;
		screen_set_frameskip_slow_machines();
	}

        video_zx8081_estabilizador_imagen.v=1;


}

//Desactivar rainbow
void disable_rainbow(void) {
	debug_printf (VERBOSE_INFO,"Disabling RealVideo");

	//Vofile necesita de rainbow para funcionar. no dejar desactivarlo si esta esto activo
	if (vofile_inserted.v==1) {
		debug_printf (VERBOSE_ERR,"Video out to file needs realvideo to work. You can not disable realvideo with video out enabled");
		return;
	}

	//si hay un cambio
	if (rainbow_enabled.v==1) {
	        rainbow_enabled.v=0;
		screen_set_frameskip_slow_machines();
        }

        modificado_border.v=1;

	//Desactivar estos tres. Asi siempre que realvideo sea 0, ninguno de estos tres estara activo
	disable_interlace();
	disable_gigascreen();
	disable_ulaplus();
}



void enable_border(void)
{
	border_enabled.v=1;
	modificado_border.v=1;
        //Recalcular algunos valores cacheados
        recalcular_get_total_ancho_rainbow();
        recalcular_get_total_alto_rainbow();
}

void disable_border(void)
{
        border_enabled.v=0;
	modificado_border.v=1;
        //Recalcular algunos valores cacheados
        recalcular_get_total_ancho_rainbow();
        recalcular_get_total_alto_rainbow();
}



void set_t_scanline_draw_zero(void)
{
        t_scanline_draw=0;

}

void t_scanline_next_line(void)
{

        t_scanline_draw++;

        if (machine_type==2) {
                //Inves

                if (t_scanline_draw>=screen_scanlines) {
                        set_t_scanline_draw_zero();
                        //printf ("reset inves a 0\n");
                }
        }


        t_scanline++;


}

void screen_z88_return_sbr(z88_dir *dir)
{

        z80_byte bank; 
        z80_int direccion; 

        int extAddressBank = (blink_sbr << 5) & 0xFF00;
        int extAddressOffset = (blink_sbr << 3) & 0x0038;

        bank=extAddressBank>>8;
        direccion=extAddressOffset<<8;

	dir->bank=bank;
	dir->dir=direccion;

}

void screen_z88_return_pb0(z88_dir *dir)
{

        z80_byte bank;
        z80_int direccion;

        int extAddressBank = (blink_pixel_base[0] << 3) & 0xF700;
        int extAddressOffset = (blink_pixel_base[0] << 1) & 0x003F;


        bank=extAddressBank>>8;
        direccion=extAddressOffset<<8;

        dir->bank=bank;
        dir->dir=direccion;

}

void screen_z88_return_pb1(z88_dir *dir)
{

        z80_byte bank;
        z80_int direccion;

        int extAddressBank = (blink_pixel_base[1] << 6) & 0xFF00;
        int extAddressOffset = (blink_pixel_base[1] << 4) & 0x0030;

        bank=extAddressBank>>8;
        direccion=extAddressOffset<<8;

        dir->bank=bank;
        dir->dir=direccion;


}

void screen_z88_return_pb2(z88_dir *dir)
{

        z80_byte bank;
        z80_int direccion;
        
        int extAddressBank = (blink_pixel_base[2] << 7) & 0xFF00;
        int extAddressOffset = (blink_pixel_base[2] << 5) & 0x0020;


        bank=extAddressBank>>8;
        direccion=extAddressOffset<<8;

        dir->bank=bank;
        dir->dir=direccion;

}     

void screen_z88_return_pb3(z88_dir *dir)
{

        z80_byte bank;
        z80_int direccion;
        
        int extAddressBank = (blink_pixel_base[3] << 5) & 0xFF00;
        int extAddressOffset = (blink_pixel_base[3] << 3) & 0x0038;

        bank=extAddressBank>>8;
        direccion=extAddressOffset<<8;

        dir->bank=bank;
        dir->dir=direccion;

}       




void screen_z88_dibujar_udg(z88_dir *tabla_caracter,int x,int y,int ancho,int inverse,int subrallado,int parpadeo,int gris,int lorescursor)
{


	/*
	//TODO. temp. comprovacion puntero
	if (tabla_caracter==NULL) {
		debug_printf (VERBOSE_INFO,"screen_z88_dibujar_udg. tabla_caracter=NULL");
		return;
	}
	*/


	z80_int color;
	z80_byte caracter;

	z80_int colorblanco;
	z80_int colornegro;
	z80_int colorgris;

	int offsety;
	int offsetx;

	int xmenu;
	int ymenu;


	//printf ("sc refres %d menu_over: %d\n",screen_refresh_menu,menu_overlay_activo);


	//colorblanco=15;
	colorblanco=Z88_PXCOLOFF;

	//colornegro=0;
	colornegro=Z88_PXCOLON;

	//colorgris=7;	
	colorgris=Z88_PXCOLGREY;	

	//Ver si se sale el ancho
	if (x+ancho>640) {
		//printf ("limite ancho en x: %d y: %d\n",x,y);
		int xorig=x;

		//borramos esa zona
		for (offsety=0;offsety<8;offsety++) {
			for (x=xorig;x<640;x++) {
				scr_putpixel_zoom(x,y+offsety,colorblanco);
				//printf ("borrar zona %d %d\n",x,y+offsety);
			}
		}
		return;
	}

        //Caracter con parpadeo, no cursor
        if (parpadeo && estado_parpadeo.v && !lorescursor) {

                        //Parpadeo activo y no es cursor. borramos esa zona.
                        //El parpadeo funciona asi: no activo: dibujamos caracter. activo: borramos zona con color blanco
                        for (offsety=0;offsety<8;offsety++) {
                                for (offsetx=0;offsetx<ancho;offsetx++) {
                                        //Ver si esta zona esta ocupada por el menu
                                        xmenu=(x+offsetx)/8;
                                        ymenu=(y+offsety)/8;

                                        if (xmenu>=0 && ymenu>=0 && xmenu<=31 && ymenu<=23) {
                                                if (scr_ver_si_refrescar_por_menu_activo(xmenu,ymenu)) {
                                                        scr_putpixel_zoom(x+offsetx,y+offsety,colorblanco);
                                                }
                                        }
                                        else scr_putpixel_zoom(x+offsetx,y+offsety,colorblanco);


                                }
                        }


                        return;
        }

        //Cursor con parpadeo
        if (parpadeo && estado_parpadeo_cursor.v && lorescursor) {
                
                //Es cursor. Invertir colores
                        z80_int c; 
                        //Invertir colores
                        c=colorblanco;
                        colorblanco=colornegro;
                        colornegro=c;
        }



	for (offsety=0;offsety<8;offsety++) {


		//caracter=*tabla_caracter++;
		//printf ("tabla caracter: dir: %x bank: %x\n",tabla_caracter->dir,tabla_caracter->bank);

		caracter=peek_byte_no_time_z88_bank_no_check_low(tabla_caracter->dir,tabla_caracter->bank);
		tabla_caracter->dir++;

		if (inverse) caracter = caracter ^255;

		if (subrallado && offsety==7) caracter=255;

		//printf ("caracter: %x\n",caracter);
		if (ancho==6) caracter <<=2;
		for (offsetx=0;offsetx<ancho;offsetx++) {
			if (caracter&128) {
				color=colornegro;
				if (gris) color=colorgris;
			}

			else color=colorblanco;


                        //Ver si esta zona esta ocupada por el menu
                        xmenu=(x+offsetx)/8;
                        ymenu=(y+offsety)/8;
                        if (xmenu>=0 && ymenu>=0 && xmenu<=31 && ymenu<=23) {
                                if (scr_ver_si_refrescar_por_menu_activo(xmenu,ymenu)) {
                                        scr_putpixel_zoom(x+offsetx,y+offsety,color);
                                }
                        }

			else scr_putpixel_zoom(x+offsetx,y+offsety,color);



			caracter <<=1;
		}
	}


}

//Refrescar pantalla para drivers graficos
void screen_z88_refresca_pantalla(void)
{

	/*
	z80_byte *sbr=screen_z88_return_sbr();
	z80_byte *lores0=screen_z88_return_pb0();
	z80_byte *lores1=screen_z88_return_pb1();
	z80_byte *hires0=screen_z88_return_pb2();
	z80_byte *hires1=screen_z88_return_pb3();
	*/



        if ((blink_com&1)==0) {
                debug_printf (VERBOSE_DEBUG,"LCD is OFF");
                int x,y,xmenu,ymenu;

                for (y=0;y<64;y++) {
                        for (x=0;x<screen_get_window_width_no_zoom();x++) {

	                        //Ver si esta zona esta ocupada por el menu
        	                xmenu=x/8;
	                        ymenu=y/8;
	                        if (xmenu>=0 && ymenu>=0 && xmenu<=31 && ymenu<=23) {
        	                        if (scr_ver_si_refrescar_por_menu_activo(xmenu,ymenu)) {
						scr_putpixel_zoom(x,y,Z88_PXCOLSCROFF);
					}
				}
				else {
                                	scr_putpixel_zoom(x,y,Z88_PXCOLSCROFF);
				}
                        }
                }
                return;
        }


	z88_dir sbr,lores0,lores1,hires0,hires1;

	screen_z88_return_sbr(&sbr);
	screen_z88_return_pb0(&lores0);
	screen_z88_return_pb1(&lores1);
	screen_z88_return_pb2(&hires0);
	screen_z88_return_pb3(&hires1);

	//temp. hacer dir>16384

	/*	
	sbr.bank--;
	sbr.dir+=16384;

	lores0.bank--;
	lores0.dir+=16384;

        lores1.bank--;
        lores1.dir+=16384;

        hires0.bank--;
        hires0.dir+=16384;

        hires1.bank--;
        hires1.dir+=16384;
	*/


	/*
	printf ("sbr bank %x add %x\n",sbr.bank,sbr.dir);
	printf ("lores0 bank %x add %x\n",lores0.bank,lores0.dir);
	printf ("lores1 bank %x add %x\n",lores1.bank,lores1.dir);
	printf ("hires0 bank %x add %x\n",hires0.bank,hires0.dir);
	printf ("hires1 bank %x add %x\n",hires1.bank,hires1.dir);
	*/


	z80_byte caracter,atributo;


	int x;
	int y;
	int ancho;

	z88_dir copia_sbr;

	copia_sbr.bank=sbr.bank;
	copia_sbr.dir=sbr.dir;

	//z80_byte *tabla_caracteres;
	z88_dir tabla_caracteres;

	int null_caracter;
	int inverse,subrallado,parpadeo,gris;
	int lorescursor;

	int caracteres_linea;


	for (y=0;y<64;y+=8) {
		x=0;
		caracteres_linea=0;
		while (x<640 && caracteres_linea<108) { 


			//printf ("temp. x: %d y: %d\n",x,y);

			//caracter=*sbr++; 
                	caracter=peek_byte_no_time_z88_bank_no_check_low(sbr.dir,sbr.bank);
			sbr.dir++;


			//atributo=*sbr++;
                        atributo=peek_byte_no_time_z88_bank_no_check_low(sbr.dir,sbr.bank);
                        sbr.dir++;


			//printf ("sbr bank: %x dir: %x caracter: %x atributo: %x\n",sbr.bank,sbr.dir,caracter,atributo);
			//if (caracter>31 && caracter<128) printf ("%c",caracter); 

			caracteres_linea++;

			inverse=(atributo & 16 ? 1 : 0);
			subrallado=0;

		
			parpadeo=(atributo & 8 ? 1 : 0);
			gris=(atributo & 4 ? 1 : 0);
			/*

Attribute 2 (odd address):      Attribute 1 (even address): 
7   6   5   4   3   2   1   0   7   6   5   4   3   2   1   0  
---------------------------------------------------------------
sw1 sw2 lrs rev fls gry und ch8 ch7 ch6 ch5 ch4 ch3 ch2 ch1 ch0
---------------------------------------------------------------

sw1 : no hardware effect (used to store tiny flag)
sw2 : no hardware effect (used to store bold flag)
hrs : refer to Hires font (i.e. shift 8 bits in register), else Lores
rev : reverse (i.e. XOR)
fls : flash (1 second period)
gry : grey (probably a faster flash period)
und : underline (i.e. set the 8 bits when on 8th row), only valid for Lores
      it becomes ch9 when hrs is set.

The Lores fonts are addressed by 9 bits. It represents an amount of 512 characters ($000-$1FF).
- Lores1 ($000-$1BF) is the 6 * 8 OZ characters file in ROM
- Lores0 ($1C0-$1FF) is the 6 * 8 user defined characters file in RAM. Assignment starts from top address with the character for code '@'.

The Hires chars are addressed by 10 bits. It represents an amount of 1024 characters ($000-$3FF).
- Hires0 ($000-$2FF) is the 8 * 8 map file in RAM (only 256 are used)
- Hires1 ($300-$3FF) is the 8 * 8 characters for the OZ window (only 128 are used)


               5     4     3     2     1     0,  7-0
               hrs   rev   fls   gry   und   ch8-ch0
----------------------------------------------------
LORES          0     v     v     v     v     000-1FF
LORES CURSOR   1     1     1     v     v     000-1FF
NULL CHARACTER 1     1     0     1     xxx    -  xxx
HIRES          1     0     v     v     000    -  3FF



			*/
			z80_byte tipo_caracter=atributo&0x3E; //00111110
				null_caracter=0;
			//LORES o LORESCURSOR
			lorescursor=( (tipo_caracter & (32+16+8+4))==32+16+8 ? 1 : 0);

			if ( (tipo_caracter & 32)==0  || lorescursor ) {
				//printf ("(tipo_caracter & 32)==0  || lorescursor\n");
				ancho=6;
				//LORES
				if (atributo&2) subrallado=1;
				//Ver si 0 o 1
				z80_int caracter16=caracter | ((atributo&1)<<8);
				if (caracter16<=0x1BF) {
					//LORES1
					//tabla_caracteres=lores1+caracter16*8;
					tabla_caracteres.bank=lores1.bank;
					tabla_caracteres.dir=lores1.dir+caracter16*8;

					//if (parpadeo) printf ("en lores1 x: %d y: %d caracter16: %d lorescursor: %d\n",x,y,caracter16,lorescursor);
					//printf ("lores1 caracter16: %d\n",caracter16);


				}
				else {
					//LORES0
					//tabla_caracteres=lores0+(caracter16-0x1c0)*8;
                                        tabla_caracteres.bank=lores0.bank;
                                        tabla_caracteres.dir=lores0.dir+(caracter16-0x1c0)*8;

					//printf ("lores0 caracter16: %d\n",caracter16);

				}

			}

			else if ((tipo_caracter & 48)==32) {
				//printf ("((tipo_caracter & 48)==32)\n");
				//HIRES
				ancho=8;
				//Ver si 0 o 1
				z80_int caracter16=caracter | ((atributo&3)<<8);
				if (caracter16<=0x2ff) {
					//HIRES0
					//tabla_caracteres=hires0+caracter16*8;
                                        tabla_caracteres.bank=hires0.bank;
                                        tabla_caracteres.dir=hires0.dir+caracter16*8;

					//if (parpadeo) printf ("en hires0 x: %d y: %d caracter16: %d *8: %d\n",x,y,caracter16,caracter16*8);
					//printf ("hires0 caracter16: %d\n",caracter16);


				}
				else {
					//HIRES1
					//tabla_caracteres=hires1+(caracter16-0x300)*8;
                                        tabla_caracteres.bank=hires1.bank;
                                        tabla_caracteres.dir=hires1.dir+(caracter16-0x300)*8;
					//printf ("hires1 caracter16: %d\n",caracter16);


					//temp
					//tabla_caracteres=hires1+caracter*8;
					//if (parpadeo) printf ("en hires1 x: %d y: %d caracter16: %d *8: %d\n",x,y,caracter16,(caracter16-0x300)*8 );

					//temp	
					//if (caracter16==928) tabla_caracteres=hires1+(caracter16-0x300-128)*8;
					//if (parpadeo) parpadeo=0;

                                }

			}

			else if ((tipo_caracter & (32+16+8+4) )==32+16+4) {
				//NULL character
				null_caracter=1;
			}

			else {
				//Cualquier otro caso. por ejemplo, atributo: 51 (110011 ), no coincide con nada
				//Caracter corrupto
				//establecer ancho a algun valor. esto es importante, porque la variable ancho
				//no viene inicializada, y si el primer byte no coincide con ningun caracter normal, tendra valor indeterminado,
				//y en el siguiente codigo (mas abajo) donde hace x=x+ancho, x se sale de rango y provoca segmentation fault
				ancho=6;
				//printf ("ninguno de los anteriores caracter: %d atributo: %d\n",caracter,atributo);
			}

			//temp
			//if (parpadeo) printf ("parpadeo x: %d y: %d caracteres_linea: %d caracter: %d atributo: %d ancho: %d\n",x,y,caracteres_linea,caracter,atributo,ancho);
			
			if (null_caracter==0) {
				screen_z88_dibujar_udg(&tabla_caracteres,x,y,ancho,inverse,subrallado,parpadeo,gris,lorescursor);
				//if (ancho>20 || ancho<0) printf ("temp. x: %d y: %d ancho: %d\n",x,y,ancho);
				x=x+ancho;
				//if (ancho>20 || ancho<0)  printf ("temp. x: %d y: %d\n",x,y);
			}
			
		}

		//Restauramos valor inicial y sumamos 256. bank no se altera
		copia_sbr.dir +=256;
		sbr.dir=copia_sbr.dir;
	}

}


void screen_z88_draw_lower_screen(void)
{
	if (!MACHINE_IS_Z88) return;

#ifdef COMPILE_CURSES
	if (!strcmp(scr_driver_name,"curses")) {
		scrcurses_z88_draw_lower_screen();
		return;
	}
#endif


	if (si_complete_video_driver() ) {

		int x,y;
	
		for (y=64;y<screen_get_window_height_no_zoom();y++) {
			for (x=0;x<screen_get_window_width_no_zoom();x++) {
				scr_putpixel_zoom(x,y,7);
			}
		}
	}
}

void z88_return_character_atributes(struct s_z88_return_character_atributes *z88_caracter) 
{

	z80_byte caracter,atributo;

	z88_dir sbr;
	sbr.dir=z88_caracter->sbr.dir;
	sbr.bank=z88_caracter->sbr.bank;

	//z80_byte *sbr=z88_caracter->sbr;

			//caracter=*sbr++;
                        caracter=peek_byte_no_time_z88_bank_no_check_low(sbr.dir,sbr.bank);
                        sbr.dir++;


                        //atributo=*sbr++;
                        atributo=peek_byte_no_time_z88_bank_no_check_low(sbr.dir,sbr.bank);
                        sbr.dir++;


			z88_caracter->inverse=(atributo & 16 ? 1 : 0);
			z88_caracter->subrallado=0;
			z88_caracter->parpadeo=(atributo & 8 ? 1 : 0);
			z88_caracter->gris=(atributo & 4 ? 1 : 0);
			/*

Attribute 2 (odd address):      Attribute 1 (even address): 
7   6   5   4   3   2   1   0   7   6   5   4   3   2   1   0  
---------------------------------------------------------------
sw1 sw2 lrs rev fls gry und ch8 ch7 ch6 ch5 ch4 ch3 ch2 ch1 ch0
---------------------------------------------------------------

sw1 : no hardware effect (used to store tiny flag)
sw2 : no hardware effect (used to store bold flag)
hrs : refer to Hires font (i.e. shift 8 bits in register), else Lores
rev : reverse (i.e. XOR)
fls : flash (1 second period)
gry : grey (probably a faster flash period)
und : underline (i.e. set the 8 bits when on 8th row), only valid for Lores
      it becomes ch9 when hrs is set.

The Lores fonts are addressed by 9 bits. It represents an amount of 512 characters ($000-$1FF).
- Lores1 ($000-$1BF) is the 6 * 8 OZ characters file in ROM
- Lores0 ($1C0-$1FF) is the 6 * 8 user defined characters file in RAM. Assignment starts from top address with the character for code '@'.

The Hires chars are addressed by 10 bits. It represents an amount of 1024 characters ($000-$3FF).
- Hires0 ($000-$2FF) is the 8 * 8 map file in RAM (only 256 are used)
- Hires1 ($300-$3FF) is the 8 * 8 characters for the OZ window (only 128 are used)


               5     4     3     2     1     0,  7-0
               hrs   rev   fls   gry   und   ch8-ch0
----------------------------------------------------
LORES          0     v     v     v     v     000-1FF
LORES CURSOR   1     1     1     v     v     000-1FF
NULL CHARACTER 1     1     0     1     xxx    -  xxx
HIRES          1     0     v     v     000    -  3FF



			*/

			z80_byte tipo_caracter=atributo&0x3E; //00111110
				z88_caracter->null_caracter=0;

			int caracter16=0; //lo inicializo a 0 para evitar warnings al compilar

                        //Cursor. no hace falta
                        /*
                        if ((tipo_caracter & (32+16+8+4) )==32+16+8 ) {
                                ascii_caracter=' ';
                        }
                        */




			//LORES o LORESCURSOR
			if ( (tipo_caracter & 32)==0  || (tipo_caracter & (32+16+8+4) )==32+16+8 ) {
				z88_caracter->ancho=6;
				//LORES
				if (atributo&2) z88_caracter->subrallado=1;
				//Ver si 0 o 1
				caracter16=caracter | ((atributo&1)<<8);
				if (caracter16<=0x1BF) {
					//LORES1

					//ENTER
                                        if (caracter16==259) caracter16='E';
                                        if (caracter16==260) caracter16='N';
                                        if (caracter16==261) caracter16='T';

					//ESC
                                        if (caracter16==268) caracter16='E';
                                        if (caracter16==269) caracter16='S';
                                        if (caracter16==270) caracter16='C';



					//MENU
					if (caracter16==274) caracter16='M';
					if (caracter16==275) caracter16='E';
					if (caracter16==276) caracter16='N';
				}
				else {
					//LORES0
					caracter16-= 0x1c0;


				}

			}

			else if ((tipo_caracter & 48)==32) {
				//HIRES
				z88_caracter->ancho=8;
				//Ver si 0 o 1
				caracter16=caracter | ((atributo&3)<<8);
				if (caracter16<=0x2ff) {
					//HIRES0
				}
				else {
					//HIRES1
					caracter16-= 0x300;

					//Caracteres especiales:
					//OZ
					if (caracter16==128) caracter16='O';
					if (caracter16==129) caracter16='Z';

					//CAPS
					if (caracter16==132) caracter16='C';
					if (caracter16==133) caracter16='L';


                                }

			}

			else if ((tipo_caracter & (32+16+8+4) )==32+16+4) {
				//NULL character
				z88_caracter->null_caracter=1;
			}



			//Adaptar caracter a conjunto ASCII
			if (z88_caracter->null_caracter==0) {
				if (caracter16<32) caracter16 +=32;

                                caracter16 &=127;

				if (caracter16<32 || caracter16>127)  caracter16='.';
			}


			z88_caracter->ascii_caracter=caracter16;
			
}




//Refrescar pantalla para drivers de texto
void screen_repinta_pantalla_z88(struct s_z88_return_character_atributes *z88_caracter)
{

	/*
        z80_byte bank; 
        z80_int direccion; 


        bank = (blink_sbr >>3) ;
        direccion = (blink_sbr << 3) & 0x38;
        direccion=direccion<<8;


        //Offset dentro del slot de memoria
        z80_long_int offset=bank*16384;

        offset+=(direccion&16383);
	*/

        z88_dir sbr;

        screen_z88_return_sbr(&sbr);


	z88_dir copiasbr;
	copiasbr.bank=sbr.bank;
	copiasbr.dir=sbr.dir;
	

        //z80_long_int offsetcopia=offset;
        //        offset=offsetcopia;


        //debug_printf (VERBOSE_DEBUG,"bank: 0x%x direccion: 0x%x (%d)-----",bank,direccion,direccion);

	if ((blink_com&1)==0) {
		debug_printf (VERBOSE_DEBUG,"LCD is OFF");
		return;
	}



        for (z88_caracter->y=0; z88_caracter->y<8 ; z88_caracter->y++) {
                int bytes_leidos;

                //Cada linea mientras se lean menos de 106 caracteres no nulos, y mientras se lean menos de 256 bytes
                for (z88_caracter->x=0 , bytes_leidos=0; z88_caracter->x<106 && bytes_leidos<256; z88_caracter->x++,bytes_leidos +=2) {

                        //z88_caracter->sbr=&z88_puntero_memoria[offset];
			z88_caracter->sbr.bank=sbr.bank;
			z88_caracter->sbr.dir=sbr.dir;

                        z88_return_character_atributes(z88_caracter);

                        //offset +=2;
			sbr.dir +=2;


                        //Si caracter no es nulo
                        if (z88_caracter->null_caracter==0) {
				z88_caracter->f_print_char(z88_caracter);

                        }

                        //Y si es nulo, no avanzamos caracter
                        else {
                                z88_caracter->x--;
                        }

                }

                //Linea siguiente
		z88_caracter->f_new_line(z88_caracter);

		copiasbr.dir +=256;
		sbr.bank=copiasbr.bank;
		sbr.dir=copiasbr.dir;

                //offsetcopia +=256;
                //offset=offsetcopia;
        }


}





int screen_get_window_width_no_zoom(void)
{

        if (MACHINE_IS_Z88) {
        return SCREEN_Z88_WIDTH;
        }

        else {
        return SCREEN_SPECTRUM_WIDTH;
        }
}

int screen_get_window_height_no_zoom(void)
{

        if (MACHINE_IS_Z88) {
        return SCREEN_Z88_HEIGHT;
        }

        else {
        return SCREEN_SPECTRUM_HEIGHT;
        }
}


int screen_get_window_width_no_zoom_border_en(void)
{

        if (MACHINE_IS_Z88) {
        return SCREEN_Z88_WIDTH;
        }

        else {
        return ANCHO_PANTALLA+(LEFT_BORDER_NO_ZOOM+RIGHT_BORDER_NO_ZOOM)*border_enabled.v;
	}
}

int screen_get_window_height_no_zoom_border_en(void)
{

        if (MACHINE_IS_Z88) {
        return SCREEN_Z88_HEIGHT;
        }

        else {
        return ALTO_PANTALLA+(TOP_BORDER_NO_ZOOM+BOTTOM_BORDER_NO_ZOOM)*border_enabled.v;
	}
}




int screen_get_window_width_zoom(void)
{
	return screen_get_window_width_no_zoom()*zoom_x;
}

int screen_get_window_height_zoom(void)
{
	return screen_get_window_height_no_zoom()*zoom_y;
}


int screen_get_window_width_zoom_border_en(void)
{
        return screen_get_window_width_no_zoom_border_en()*zoom_x;
}

int screen_get_window_height_zoom_border_en(void)
{
        return screen_get_window_height_no_zoom_border_en()*zoom_y;
}


void disable_gigascreen(void)
{
	debug_printf (VERBOSE_INFO,"Disable gigascreen");
	gigascreen_enabled.v=0;
}

void enable_gigascreen(void)
{
	debug_printf (VERBOSE_INFO,"Enable gigascreen");
	if (gigascreen_enabled.v==0) {
		screen_print_splash_text(10,0,7+8,"Enabling Gigascreen mode");
	}

	gigascreen_enabled.v=1;

	//son excluyentes
	disable_interlace();
	disable_ulaplus();

	//necesita real video
	enable_rainbow();
}

void disable_interlace(void)
{
	debug_printf (VERBOSE_INFO,"Disable interlace");
	video_interlaced_mode.v=0;
	set_putpixel_zoom();
}

void enable_interlace(void)
{

	debug_printf (VERBOSE_INFO,"Enable interlace");
	if (video_interlaced_mode.v==0) {
		screen_print_splash_text(10,0,7+8,"Enabling Interlace video mode");
	}

	//son excluyentes
	disable_gigascreen();
	disable_ulaplus();

	//necesita real video
	enable_rainbow();

	int reinicia_ventana=0;

	//Si zoom y no es multiple de 2
	if ((zoom_y&1)!=0) reinicia_ventana=1;


        if (reinicia_ventana) {
		scr_end_pantalla();
		zoom_y=2;
		zoom_x=2;
	}

        video_interlaced_mode.v=1;

        if (reinicia_ventana) scr_init_pantalla();

        set_putpixel_zoom();

	interlaced_numero_frame=0;


}

//refresco de pantalla, 2 veces, para que cuando haya modo interlaced o gigascreen y multitask on, se dibujen los dos frames, el par y el impar
void all_interlace_scr_refresca_pantalla(void)
{
        scr_refresca_pantalla();
        if (video_interlaced_mode.v || gigascreen_enabled.v) {
                interlaced_numero_frame++;
		screen_switch_rainbow_buffer();
                scr_refresca_pantalla();
        }
}



int get_gigascreen_color(int c0,int c1)
{
	//return c0 ^ c1;
	int index=32+(c0*16)+c1;


	//if (index>=287) {
	//	printf ("index: %d value: %06X\n",index,spectrum_colortable_normal[index]);
	//}

	return index;
}


int get_gigascreen_rgb_value(int c0,int c1,int i0,int i1)
{
/*
C = (C0 / 3 * 2 + C0 * I0 / 3 + C1 / 2 * 3 + C1 * I1 / 3) / 2, where
C0, C1 - corresponding color (R, G, B) of 0 and 1 ekranki taking the values 0 or 1,
I - Bright, 0 or 1.
C - with the intensity of the resulting color in the range of 0-1, wherein 0 - the zero level of the video, 1 - max.
*/
	int value=(c0 / 3 * 2 +  c0 * i0 / 3  +  c1 / 2 * 3  +  c1 * i1 / 3) / 2;

	//printf ("c0: %d c1: %d i0: %d i1:%d    value:%d\n",c0,c1,i0,i1,value);

	//valor llega a valer 289 en algunos casos
	if (value>255) value=255;
	//printf ("value corrected: %d\n",value);

	return value;
}


void screen_switch_rainbow_buffer(void)
{
        //conmutar para gigascreen
        if (rainbow_buffer==rainbow_buffer_one) rainbow_buffer=rainbow_buffer_two;
        else rainbow_buffer=rainbow_buffer_one;
}


//devuelve sprite caracter de posicion rainbow
//0,0 -> inicio rainbow
//El sprite de 8x8 en posicion x,y es guardado en direccion *caracter
//Usado por stdout y curses en modo zx81 con realvideo
void screen_get_sprite_char(int x,int y,z80_byte *caracter)
{
    z80_int *origen_rainbow;

        origen_rainbow=&rainbow_buffer[ y*get_total_ancho_rainbow()+x ];
        //origen_rainbow +=screen_total_borde_izquierdo*border_enabled.v;


        //construimos bytes de origen a comparar

        int bit,j;
        z80_byte acumulado;
        z80_int leido;
        for (j=0;j<8;j++) {

                acumulado=0;

                for (bit=0;bit<8;bit++) {
                        acumulado=acumulado*2;
                        leido=*origen_rainbow;
			//Si color 0, bit a 1. Sino, bit a 0
                        if ( leido==0 ) acumulado |=1;

                        origen_rainbow++;
                }

                *caracter=acumulado;
                caracter++;

                //siguiente byte origen
                origen_rainbow=origen_rainbow+get_total_ancho_rainbow()-8;


        }
}



void screen_reset_putpixel_maxmin_y(void)
{
	putpixel_max_y=-1;
	putpixel_min_y=99999;
}


void screen_print_splash_text(z80_byte y,z80_byte tinta,z80_byte papel,char *texto)
{

	z80_byte x;

	if (menu_abierto==0 && screen_show_splash_texts.v==1) {
        	cls_menu_overlay();

		//trocear maximo en 2 lineas
		int longitud=strlen(texto);
		if (longitud>32) {
			char buffer[33];
			strncpy(buffer,texto,32);
			buffer[32]=0;
			menu_escribe_texto(0,y++,tinta,papel,buffer);
			texto=&texto[32];
		}


		//centramos en x
		x=16-(strlen(texto))/2;

		//si se va a negativo
		if (x>=32) x=0;

	        menu_escribe_texto(x,y,tinta,papel,texto);
        
        	set_menu_overlay_function(normal_overlay_texto_menu);
	        menu_splash_text_active.v=1;
        	menu_splash_segundos=5;
	}

}

