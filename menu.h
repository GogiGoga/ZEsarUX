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

#ifndef MENU_H
#define MENU_H

#include "cpu.h"

//Valor para ninguna tecla pulsada
//Tener en cuenta que en spectrum y zx80/81 se usan solo 5 bits pero en Z88 se usan 8 bits
//en casos de spectrum y zx80/81 se agregan los 3 bits faltantes
#define MENU_PUERTO_TECLADO_NINGUNA 255

extern int menu_overlay_activo;
extern void (*menu_overlay_function)(void);

extern char *esc_key_message;
extern char *openmenu_key_message;

extern z80_bit menu_desactivado;

extern void set_menu_overlay_function(void (*funcion)(void) );
extern void reset_menu_overlay_function(void);
extern void pruebas_texto_menu(void);
extern void cls_menu_overlay(void);
extern void menu_escribe_texto(z80_byte x,z80_byte y,z80_byte tinta,z80_byte papel,char *texto);
extern void normal_overlay_texto_menu(void);
//extern int menu_second_layer;
extern int menu_footer;
//extern int menu_second_layer_counter;
//extern void enable_second_layer(void);
//extern void disable_second_layer(void);
extern void enable_footer(void);
extern void disable_footer(void);
extern void menu_init_footer(void);
extern void menu_footer_z88(void);

struct s_overlay_screen {
	z80_byte tinta,papel;
	z80_byte caracter;
};

typedef struct s_overlay_screen overlay_screen;

#define MAX_F_FUNCTIONS 15

enum defined_f_function_ids {
	//reset, hard-reset, nmi, open menu, ocr, smartload, osd keyboard, exitemulator.
	F_FUNCION_DEFAULT,   //1
	F_FUNCION_NOTHING,
	F_FUNCION_RESET,
	F_FUNCION_HARDRESET,
	F_FUNCION_NMI,  //5
	F_FUNCION_OPENMENU,
	F_FUNCION_OCR,
	F_FUNCION_SMARTLOAD,
	F_FUNCION_LOADBINARY,
	F_FUNCION_SAVEBINARY, //10
	F_FUNCION_OSDKEYBOARD,
	F_FUNCION_RELOADMMC,
	F_FUNCION_DEBUGCPU,
	F_FUNCION_PAUSE,
 	F_FUNCION_EXITEMULATOR  //15
};

//Define teclas F que se pueden mapear a acciones
struct s_defined_f_function {
	char texto_funcion[20];
	enum defined_f_function_ids id_funcion;
};

typedef struct s_defined_f_function defined_f_function;


extern defined_f_function defined_f_functions_array[];


#define MAX_F_FUNCTIONS_KEYS 15

//Array de teclas F mapeadas
extern enum defined_f_function_ids defined_f_functions_keys_array[];

extern int menu_define_key_function(int tecla,char *funcion);


extern overlay_screen overlay_screen_array[];
//extern overlay_screen second_overlay_screen_array[];

//definiciones para funcion menu_generic_message
#define MAX_LINEAS_VENTANA_GENERIC_MESSAGE 18

//#define MAX_LINEAS_TOTAL_GENERIC_MESSAGE 1000

//archivo LICENSE ocupa 1519 lineas ya parseado
#define MAX_LINEAS_TOTAL_GENERIC_MESSAGE 2000

#define MAX_ANCHO_LINEAS_GENERIC_MESSAGE 32
#define MAX_TEXTO_GENERIC_MESSAGE (MAX_LINEAS_TOTAL_GENERIC_MESSAGE*MAX_ANCHO_LINEAS_GENERIC_MESSAGE)

extern int menu_generic_message_aux_wordwrap(char *texto,int inicio, int final);
extern void menu_generic_message_aux_copia(char *origen,char *destino, int longitud);
extern int menu_generic_message_aux_filter(char *texto,int inicio, int final);

//Posiciones de texto mostrado en second overlay
#define WINDOW_FOOTER_ELEMENT_X_JOYSTICK 0
#define WINDOW_FOOTER_ELEMENT_X_FPS 0
#define WINDOW_FOOTER_ELEMENT_X_PRINTING 10
#define WINDOW_FOOTER_ELEMENT_X_FLASH 10
#define WINDOW_FOOTER_ELEMENT_X_MMC 11
#define WINDOW_FOOTER_ELEMENT_X_IDE 11
#define WINDOW_FOOTER_ELEMENT_X_ESX 11
#define WINDOW_FOOTER_ELEMENT_X_DISK 11
#define WINDOW_FOOTER_ELEMENT_X_TEXT_FILTER 10
#define WINDOW_FOOTER_ELEMENT_X_ZXPAND 10
#define WINDOW_FOOTER_ELEMENT_X_TAPE 11
#define WINDOW_FOOTER_ELEMENT_X_FLAP 11
#define WINDOW_FOOTER_ELEMENT_X_CPUSTEP 11
#define WINDOW_FOOTER_ELEMENT_X_CPU_TEMP 19
#define WINDOW_FOOTER_ELEMENT_X_CPU_USE 25
#define WINDOW_FOOTER_ELEMENT_X_BATERIA 30

#define WINDOW_FOOTER_ELEMENT_Y_CPU_USE 1
#define WINDOW_FOOTER_ELEMENT_Y_CPU_TEMP 1
#define WINDOW_FOOTER_ELEMENT_Y_FPS 1

#define WINDOW_FOOTER_ELEMENT_Y_F5MENU 2
#define WINDOW_FOOTER_ELEMENT_Y_ZESARUX_EMULATOR 2

/*

Como quedan los textos:

01234567890123456789012345678901
50 FPS    -PRINT-  29.3C 100% BAT
           -TAPE-
          -FLASH-
          -SPEECH-
	   MMC
          -ZXPAND-
*/



extern void putchar_menu_overlay(int x,int y,z80_byte caracter,z80_byte tinta,z80_byte papel);
//extern void putchar_menu_second_overlay(int x,int y,z80_byte caracter,z80_byte tinta,z80_byte papel);
extern void menu_putchar_footer(int x,int y,z80_byte caracter,z80_byte tinta,z80_byte papel);
extern void menu_putstring_footer(int x,int y,char *texto,z80_byte tinta,z80_byte papel);
extern void cls_menu_overlay(void);
extern int menu_multitarea;
extern int menu_abierto;

extern int if_pending_error_message;
extern char pending_error_message[];

extern void menu_footer_bottom_line(void);


extern void menu_inicio(void);

extern void set_splash_text(void);
extern void reset_splash_text(void);
extern z80_bit menu_splash_text_active;
extern int menu_splash_segundos;
extern z80_byte menu_da_todas_teclas(void);
extern void menu_espera_tecla_o_joystick(void);
extern void menu_espera_no_tecla(void);
extern void menu_get_dir(char *ruta,char *directorio);

extern int menu_tooltip_counter;

extern z80_bit tooltip_enabled;

extern z80_bit mouse_menu_disabled;

extern char *quickfile;
extern char quickload_file[];

extern z80_bit menu_button_quickload;
extern z80_bit menu_button_osdkeyboard;
extern z80_bit menu_button_osdkeyboard_return;
extern z80_bit menu_button_exit_emulator;
extern z80_bit menu_event_drag_drop;
//extern char menu_event_drag_drop_file[PATH_MAX];
extern z80_bit menu_event_remote_protocol_enterstep;
extern z80_bit menu_button_f_function;
extern int menu_button_f_function_index;

extern int menu_contador_teclas_repeticion;
extern int menu_segundo_contador_teclas_repeticion;

extern int menu_speech_tecla_pulsada;

extern char binary_file_load[];

extern char menu_buffer_textspeech_filter_program[];

extern char menu_buffer_textspeech_stop_filter_program[];

extern void screen_print_splash_text(z80_byte y,z80_byte tinta,z80_byte papel,char *texto);

extern char menu_realtape_name[];

extern z80_bit menu_force_writing_inverse_color;

extern void menu_filesel_chdir(char *dir);

extern z80_bit force_confirm_yes;

extern void draw_footer(void);

struct s_estilos_gui {
        char nombre_estilo[20];
        int papel_normal;
        int tinta_normal;

        int muestra_cursor; //si se muestra cursor > en seleccion de opcion
                                        //Esto es asi en ZXSpectr
                                        //el cursor entonces se mostrara con los colores indicados a continuacion

        int muestra_recuadro; //si se muestra recuadro en ventana

        int muestra_rainbow;  //si se muestra rainbow en titulo

        int solo_mayusculas; //para ZX80/81


        int papel_seleccionado;
        int tinta_seleccionado;

        int papel_no_disponible; //Colores cuando una opción no esta disponible
        int tinta_no_disponible;

        int papel_seleccionado_no_disponible;    //Colores cuando una opcion esta seleccionada pero no disponible
        int tinta_seleccionado_no_disponible;

        int papel_titulo, tinta_titulo;

        int color_waveform;  //Color para forma de onda en view waveform
        int color_unused_visualmem; //Color para zona no usada en visualmem
};

typedef struct s_estilos_gui estilos_gui;


#define ESTILOS_GUI 8

#define ESTILO_GUI_QL 7
#define ESTILO_GUI_MANSOFTWARE 6
#define ESTILO_GUI_SAM 5
#define ESTILO_GUI_CPC 4
#define ESTILO_GUI_Z88 3

extern void estilo_gui_retorna_nombres(void);

extern int estilo_gui_activo;

extern estilos_gui definiciones_estilos_gui[];

extern void set_charset(void);


#define ESTILO_GUI_PAPEL_NORMAL (definiciones_estilos_gui[estilo_gui_activo].papel_normal)
#define ESTILO_GUI_TINTA_NORMAL (definiciones_estilos_gui[estilo_gui_activo].tinta_normal)
#define ESTILO_GUI_PAPEL_SELECCIONADO (definiciones_estilos_gui[estilo_gui_activo].papel_seleccionado)
#define ESTILO_GUI_TINTA_SELECCIONADO (definiciones_estilos_gui[estilo_gui_activo].tinta_seleccionado)


#define ESTILO_GUI_PAPEL_NO_DISPONIBLE (definiciones_estilos_gui[estilo_gui_activo].papel_no_disponible);
#define ESTILO_GUI_TINTA_NO_DISPONIBLE (definiciones_estilos_gui[estilo_gui_activo].tinta_no_disponible);
#define ESTILO_GUI_PAPEL_SEL_NO_DISPONIBLE (definiciones_estilos_gui[estilo_gui_activo].papel_seleccionado_no_disponible);
#define ESTILO_GUI_TINTA_SEL_NO_DISPONIBLE (definiciones_estilos_gui[estilo_gui_activo].tinta_seleccionado_no_disponible);

#define ESTILO_GUI_PAPEL_TITULO (definiciones_estilos_gui[estilo_gui_activo].papel_titulo)
#define ESTILO_GUI_TINTA_TITULO (definiciones_estilos_gui[estilo_gui_activo].tinta_titulo)

#define ESTILO_GUI_COLOR_WAVEFORM (definiciones_estilos_gui[estilo_gui_activo].color_waveform)
#define ESTILO_GUI_COLOR_UNUSED_VISUALMEM (definiciones_estilos_gui[estilo_gui_activo].color_unused_visualmem)

#define ESTILO_GUI_MUESTRA_CURSOR (definiciones_estilos_gui[estilo_gui_activo].muestra_cursor)
#define ESTILO_GUI_MUESTRA_RECUADRO (definiciones_estilos_gui[estilo_gui_activo].muestra_recuadro)
#define ESTILO_GUI_MUESTRA_RAINBOW (definiciones_estilos_gui[estilo_gui_activo].muestra_rainbow)
#define ESTILO_GUI_SOLO_MAYUSCULAS (definiciones_estilos_gui[estilo_gui_activo].solo_mayusculas)

//extern z80_bit menu_espera_tecla_no_cpu_loop_flag_salir;
extern int salir_todos_menus;

extern int si_valid_char(z80_byte caracter);

extern int menu_debug_memory_zone;

extern menu_z80_moto_int menu_debug_memory_zone_size;

extern int menu_debug_show_memory_zones;


#define HELP_MESSAGE_CONDITION_BREAKPOINT \
"A condition breakpoint has the following format: \n" \
"[VARIABLE][CONDITION][VALUE] [OPERATOR] [VARIABLE][CONDITION][VALUE] [OPERATOR] .... where: \n" \
"[VARIABLE] can be a CPU register or some pseudo variables: A,B,C,D,E,F,H,L,I,R,BC,DE,HL,SP,PC,IX,IY\n" \
"FS,FZ,FP,FV,FH,FN,FC: Flags\n" \
"(BC),(DE),(HL),(SP),(PC),(IX),(IY), (NN), IFF1, IFF2, OPCODE,\n" \
"RAM: RAM mapped on 49152-65535 on Spectrum 128 or Prism,\n" \
"ROM: ROM mapped on 0-16383 on Spectrum 128,\n" \
"SEG0, SEG1, SEG2, SEG3: memory banks mapped on each 4 memory segments on Z88\n" \
"MRV: value returned on read memory operation\n" \
"MWV: value written on write memory operation\n" \
"MRA: address used on read memory operation\n" \
"MWA: address used on write memory operation\n" \
"PRV: value returned on read port operation\n" \
"PWV: value written on write port operation\n" \
"PRA: address used on read port operation\n" \
"PWA: address used on write port operation\n" \
"\n" \
"ENTERROM: returns 1 the first time PC register is on ROM space (0-16383)\n" \
"EXITROM: returns 1 the first time PC register is out ROM space (16384-65535)\n" \
"Note: The last two only return 1 the first time the breakpoint is fired, or a watch is shown, " \
"it will return 1 again only exiting required space address and entering again\n" \
"\n" \
"TSTATES: t-states total in a frame\n" \
"TSTATESL: t-states in a scanline\n" \
"TSTATESP: t-states partial\n" \
"SCANLINE: scanline counter\n" \
"\n" \
"[CONDITION] must be one of: <,>,=,/  (/ means not equal)\n" \
"[VALUE] must be a numeric value\n" \
"[OPERATOR] must be one of the following: and, or, xor\n" \
"Examples of conditions:\n" \
"SP<32768 : it will match when SP register is below 32768\n" \
"FS=1: it will match when flag S is set\n" \
"A=10 and BC<33 : it will match when A register is 10 and BC is below 33\n" \
"OPCODE=ED4AH : it will match when running opcode ADC HL,BC\n" \
"OPCODE=21H : it will match when running opcode LD HL,NN\n" \
"OPCODE=210040H : it will match when running opcode LD HL,4000H\n" \
"SEG2=40H : when memory bank 40H is mapped to memory segment 2 (49152-65535 range) on Z88\n" \
"MWA<16384 : it will match when attempting to write in ROM\n" \
"ENTERROM=1 : it will match when entering ROM space address\n" \
"TSTATESP>69888 : it will match when partial counter has executed a 48k full video frame (you should reset it before)\n" \
"\nNote: Any condition in the whole list can trigger a breakpoint" \


#define HELP_MESSAGE_BREAKPOINT_ACTION \
"Action can be one of the following: \n" \
"menu or break or empty string: Breaks current execution of program\n" \
"call address: Calls memory address\n" \
"printc c: Print character c to console\n" \
"printe expression: Print expression following the same syntax as watches and evaluate condition\n" \
"prints string: Prints string to console\n" \
"set-register string: Sets register indicated on string. Example: set-register PC=32768\n" \
"write address value: Write memory address with indicated value\n" \

#endif
