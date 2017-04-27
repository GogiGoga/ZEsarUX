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

#ifndef SCREEN_H
#define SCREEN_H

#include "cpu.h"
#include "z88.h"

extern z80_int devuelve_direccion_pantalla(z80_byte x,z80_byte y);
extern void init_sprite_table(void);
extern z80_int *sprite_table;
extern int spectrum_colortable_oscuro[];
//extern int spectrum_colortable_grises[];

extern int *spectrum_colortable;
extern int spectrum_colortable_normal[];
extern const int spectrum_colortable_original[16];

extern int screen_gray_mode;

extern z80_bit inverse_video;


extern void scr_refresca_border(void);


extern void (*scr_refresca_pantalla) (void);
extern void (*scr_set_fullscreen) (void);
extern void (*scr_reset_fullscreen) (void);
extern int ventana_fullscreen;
extern int (*scr_init_pantalla) (void);
extern void (*scr_end_pantalla) (void);
extern z80_byte (*scr_lee_puerto) (z80_byte puerto_h,z80_byte puerto_l);
extern void (*scr_actualiza_tablas_teclado) (void);
extern void (*scr_debug_registers)(void);
extern void (*scr_messages_debug)(char *mensaje);

extern void (*scr_putpixel) (int x,int y,unsigned int color);

extern void (*scr_putpixel_zoom) (int x,int y,unsigned color);
extern void (*scr_putpixel_zoom_rainbow)(int x,int y,unsigned int color);

extern void (*scr_z88_load_keymap) (void);


extern void scr_refresca_pantalla_comun(void);
extern void scr_refresca_border(void);

extern z80_int *putpixel_cache;

extern void scr_refresca_pantalla_rainbow_comun(void);

extern void (*scr_putchar_zx8081) (int x,int y, z80_byte caracter);
extern void scr_refresca_pantalla_zx8081(void);
extern void scr_refresca_pantalla_y_border_zx8081(void);
extern void scr_putsprite_zx8081(z80_int direccion,int x,int y,z80_bit inverse);
extern void scr_putchar_zx8081_comun(int x,int y, z80_byte caracter);
extern void scr_refresca_border_zx8081(void);

extern z80_bit simulate_screen_zx8081;
extern z80_byte umbral_simulate_screen_zx8081;
extern int scr_get_4pixel(int x,int y);
extern int scr_get_4pixel_rainbow(int x,int y);

extern int if_store_scanline_interlace(int y);

extern z80_byte compare_char(unsigned char *origen,unsigned char *inverse);
extern z80_byte compare_char_step(z80_byte *origen,z80_byte *inverse,int step);

extern z80_byte compare_char_tabla_rainbow(z80_byte *origen,z80_byte *inverse,z80_byte *tabla_leemos);
extern int calcula_offset_screen (int x,int y);

extern void (*scr_putchar_menu) (int x,int y, z80_byte caracter,z80_byte tinta,z80_byte papel);
extern void scr_putsprite_comun(z80_byte *puntero,int x,int y,z80_bit inverse,z80_byte tinta,z80_byte papel,z80_bit fast_mode);
extern int scr_tiene_colores;

extern char *scr_driver_name;

extern z80_bit texto_artistico;
extern int umbral_arttext;

extern int screen_refresh_menu;

extern int screen_stdout_driver;

extern void load_screen(char *scrfile);
extern void save_screen(char *scrfile);

//extern z80_bit interlaced_frame_par;
extern z80_byte interlaced_numero_frame;

extern z80_bit video_interlaced_mode;

extern z80_bit video_interlaced_scanlines;

extern z80_bit gigascreen_enabled;

extern int gigascreen_ajuste_t_estados;


//Tamanyos pantalla para zxspectrum. Usados en funciones de repintado
#define LEFT_BORDER_NO_ZOOM 48
#define RIGHT_BORDER_NO_ZOOM 48

//hay 16 pixeles arriba que no son visibles
#define TOP_BORDER_NO_ZOOM 48

#define BOTTOM_BORDER_NO_ZOOM 56

/*


<-96->  <-----48----->       <-256->           <-----48----->  ^
                                                               | 16
        -----------------------------------------------------  v
        -                                                   -  ^ 48                             
        -                                                   -  |
        -            --------------------------             -  v 
        -            -                        -             -
        -            -                        -             -
        -            -                        -             -
        -            -                        -             -
        -            -                        -             -
        -            -                        -             -
        -            -                        -             -
        -            -                        -             -
        -            -                        -             -
        -            --------------------------             -  ^
        -                                                   -  |
        -                                                   -  |  56
        -----------------------------------------------------  v


*/


#define LEFT_BORDER LEFT_BORDER_NO_ZOOM*zoom_x
#define TOP_BORDER TOP_BORDER_NO_ZOOM*zoom_y
#define BOTTOM_BORDER BOTTOM_BORDER_NO_ZOOM*zoom_y




#define ANCHO_PANTALLA 256
#define ALTO_PANTALLA 192
#define DISPLAY_SCREEN_HEIGHT ALTO_PANTALLA


#define SCREEN_Z88_WIDTH (640)
//#define SCREEN_Z88_HEIGHT (ALTO_PANTALLA+TOP_BORDER_NO_ZOOM+BOTTOM_BORDER_NO_ZOOM)
#define SCREEN_Z88_HEIGHT (192)
//#define SCREEN_SPECTRUM_WIDTH (ANCHO_PANTALLA+LEFT_BORDER_NO_ZOOM*2*border_enabled.v)
//#define SCREEN_SPECTRUM_HEIGHT (ALTO_PANTALLA+TOP_BORDER_NO_ZOOM*border_enabled.v+BOTTOM_BORDER_NO_ZOOM*border_enabled.v)

#define SCREEN_SPECTRUM_WIDTH (ANCHO_PANTALLA+LEFT_BORDER_NO_ZOOM+RIGHT_BORDER_NO_ZOOM)
#define SCREEN_SPECTRUM_HEIGHT (ALTO_PANTALLA+TOP_BORDER_NO_ZOOM+BOTTOM_BORDER_NO_ZOOM)

//Valores usados en real video

extern z80_bit rainbow_enabled;
extern z80_bit autodetect_rainbow;


//normalmente a 16
extern int screen_invisible_borde_superior;
//normalmente a 48. 
extern int screen_borde_superior;

//estos dos anteriores se suman aqui. es 64 en 48k, y 63 en 128k. por tanto, uno de los dos valores vale 1 menos
extern int screen_total_borde_superior;

//normalmente a 56
extern int screen_total_borde_inferior;

//normalmente a 48
extern int screen_total_borde_izquierdo;

extern int screen_testados_total_borde_izquierdo;

//normalmente a 96
extern int screen_invisible_borde_derecho;

//normalmente a 48
extern int screen_total_borde_derecho;

//estos dos anteriores se suman aqui. es 64 en 48k, y 63 en 128k. por tanto, uno de los dos valores vale 1 menos
extern int screen_indice_inicio_pant;

//suma del anterior+192
extern int screen_indice_fin_pant;

extern int screen_testados_indice_borde_derecho;

extern void screen_set_video_params_indices(void);

extern void init_rainbow(void);

extern int get_total_ancho_rainbow(void);
extern int get_total_alto_rainbow(void);

//extern z80_int *rainbow_buffer;
extern z80_long_int *rainbow_buffer;

extern int screen_scanlines;

//Total de t_estados por linea
extern int screen_testados_linea;

extern int screen_testados_total;

//extern void screen_store_scanline_rainbow(void);
extern void screen_store_scanline_rainbow_solo_display(void);
extern void screen_store_scanline_rainbow_solo_border(void);

extern z80_byte buffer_border[];
#define BORDER_ARRAY_LENGTH 228

#define ATRIBUTOS_ARRAY_LENGTH 64
extern z80_byte buffer_atributos[];

extern int last_x_atributo;

extern void clear_putpixel_cache(void);

extern void init_cache_putpixel(void);

//extern void screen_store_scanline_char_zx8081(int x,int y,z80_byte byte_leido);
extern void screen_store_scanline_char_zx8081(int x,int y,z80_byte byte_leido,z80_byte caracter,int inverse);
extern void screen_store_scanline_char_zx8081_border_scanline(int x,int y,z80_byte byte_leido);

extern void siguiente_frame_pantalla(void);


extern int framedrop_total;
extern int frames_total;
extern int framescreen_saltar;
extern int ultimo_fps;

//vofile
extern char *vofilename;
extern void init_vofile(void);
extern void close_vofile(void);
extern void vofile_send_frame(z80_int *buffer);
extern z80_bit vofile_inserted;

extern int vofile_fps;
extern int vofile_frame_actual;
extern void print_helper_aofile_vofile(void);

extern void screen_init_colour_table(void);

extern void scr_fadeout(void);

extern void screen_reset_scr_driver_params(void);

extern int scr_si_color_oscuro(void);

extern int frameskip;
extern int frameskip_counter;

extern void cpu_loop_refresca_pantalla(void);

extern int si_complete_video_driver(void);

extern void screen_show_panic_screen(void);
extern void screen_print(int x,int y,z80_byte tinta,z80_byte papel,char *mensaje);

extern int screen_print_y;

extern void set_putpixel_zoom(void);

extern void enable_rainbow(void);
extern void disable_rainbow(void);

extern void recalcular_get_total_ancho_rainbow(void);
extern void recalcular_get_total_alto_rainbow(void);
extern void enable_border(void);
extern void disable_border(void);


extern int get_total_ancho_rainbow_cached;
extern int get_total_alto_rainbow_cached;

//Variables cacheadas para que sea mas rapido
#define get_total_ancho_rainbow() get_total_ancho_rainbow_cached
#define get_total_alto_rainbow() get_total_alto_rainbow_cached


extern void set_t_scanline_draw_zero(void);
extern void t_scanline_next_line(void);

extern void screen_z88_refresca_pantalla(void);

extern void screen_z88_draw_lower_screen(void);

//extern void z88_return_character_atributes(z80_byte *sbr,int *ascii_caracter,int *ancho,int *inverse,int *subrallado,int *parpadeo,int *gris,int *null_caracter);


//Estructura usada para funcion de conversion de caracter z88, y para funcion de bucle de refresco de pantalla de z88 en ascii
struct s_z88_return_character_atributes
{
	//z80_byte *sbr;
	z88_dir sbr;
	int ascii_caracter;
	int ancho;
	int inverse;
	int subrallado;
	int parpadeo;
	int gris;
	int null_caracter;

	//Estos siguientes solo usados en funcion de bucle de refresco de pantalla
	int x,y;
	void (*f_print_char)(struct s_z88_return_character_atributes *s);
	void (*f_new_line)(struct s_z88_return_character_atributes *s);
};

extern void z88_return_character_atributes(struct s_z88_return_character_atributes *z88_caracter);

extern void screen_repinta_pantalla_z88(struct s_z88_return_character_atributes *z88_caracter);

extern int screen_get_window_width_no_zoom(void);
extern int screen_get_window_height_no_zoom(void);
extern int screen_get_window_width_zoom(void);
extern int screen_get_window_height_zoom(void);

extern int screen_get_window_width_no_zoom_border_en(void);
extern int screen_get_window_height_no_zoom_border_en(void);
extern int screen_get_window_width_zoom_border_en(void);
extern int screen_get_window_height_zoom_border_en(void);

extern void disable_interlace(void);
extern void enable_interlace(void);

extern void all_interlace_scr_refresca_pantalla(void);

extern int get_gigascreen_color(int c0,int c1);

extern int get_gigascreen_rgb_value(int c0,int c1,int i0,int i1);

extern void disable_gigascreen(void);
extern void enable_gigascreen(void);

extern void screen_switch_rainbow_buffer(void);

#define Z88_PXCOLON (32+256+0)
#define Z88_PXCOLGREY (32+256+1)
#define Z88_PXCOLOFF (32+256+2)
#define Z88_PXCOLSCROFF (32+256+3)

#define ULAPLUS_INDEX_FIRST_COLOR (Z88_PXCOLSCROFF+1)

//16 colores normales spectrum, 16 grises de modo scanline, 256 de gigascreen, 4 de z88, 64 de ulaplus
#define EMULATOR_TOTAL_PALETTE_COLOURS (32+256+4+64)

//macro usado en paletas de colores (opciones --red, --green, --blue)
#define VALOR_GRIS_A_R_G_B \
                                if (screen_gray_mode & 1) b=valorgris; \
                                else b=0; \
				\
                                if (screen_gray_mode & 2) g=valorgris; \
                                else g=0; \
				\
                                if (screen_gray_mode & 4) r=valorgris; \
                                else r=0; \

extern void screen_get_sprite_char(int x,int y,z80_byte *caracter);
extern void screen_set_colour_normal(int index, int colour);

extern void screen_reset_putpixel_maxmin_y(void);
extern int putpixel_max_y, putpixel_min_y;

extern void screen_print_splash_text(z80_byte y,z80_byte tinta,z80_byte papel,char *texto);
extern z80_bit screen_show_splash_texts;
extern void screen_set_frameskip_slow_machines(void);

#endif
