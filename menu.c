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
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>


#include "menu.h"
#include "screen.h"
#include "cpu.h"
#include "debug.h"
#include "zx8081.h"
#include "ay38912.h"
#include "tape.h"
#include "audio.h"
#include "timer.h"
#include "snap.h"
#include "operaciones.h"
#include "disassemble.h"
#include "utils.h"
#include "contend.h"
#include "joystick.h"
#include "ula.h"
#include "printers.h"
#include "realjoystick.h"
#include "scrstdout.h"
#include "z88.h"
#include "ulaplus.h"
#include "autoselectoptions.h"
#include "zxuno.h"
#include "charset.h"
#include "chardetect.h"
#include "textspeech.h"
#include "mmc.h"
#include "ide.h"
#include "divmmc.h"
#include "divide.h"
#include "diviface.h"
#include "zxpand.h"
#include "spectra.h"
#include "spritechip.h"
#include "jupiterace.h"
#include "timex.h"
#include "chloe.h"
#include "prism.h"
#include "cpc.h"
#include "sam.h"
#include "atomlite.h"
#include "if1.h"
#include "pd765.h"
#include "tbblue.h"
#include "dandanator.h"
#include "superupgrade.h"
#include "m68k.h"
#include "remote.h"
#include "snap_rzx.h"
#include "multiface.h"


#if defined(__APPLE__)
	#include <sys/syslimits.h>

	#include <sys/resource.h>

#endif

#include "compileoptions.h"

#ifdef COMPILE_CURSES
	#include "scrcurses.h"
#endif

#ifdef COMPILE_AA
	#include "scraa.h"
#endif

#ifdef COMPILE_STDOUT
	#include "scrstdout.h"
//macro llama a funcion real
	#define scrstdout_menu_print_speech_macro scrstdout_menu_print_speech
//funcion llama
#else
//funcion no llama a nada
	#define scrstdout_menu_print_speech_macro(x)
#endif

//si se pulsa tecla mientras se lee el menu
int menu_speech_tecla_pulsada=0;

#ifdef COMPILE_XWINDOWS
	#include "scrxwindows.h"
#endif

//indica que hay funcion activa de overlay o no
int menu_overlay_activo=0;



//funcion activa de overlay
void (*menu_overlay_function)(void);

//buffer de escritura por pantalla
overlay_screen overlay_screen_array[32*24];

//Indica que hay una segunda capa de texto por encima de menu y por encima del juego incluso
//util para mostrar indicadores de carga de cinta, por ejemplo
//int menu_second_layer=0;

//Footer activado por defecto
int menu_footer=1;

//se activa second layer solo para un tiempo limitado
//int menu_second_layer_counter=0;

//buffer de escritura de segunda capa
//overlay_screen second_overlay_screen_array[32*24];

//Si el menu esta desactivado completamente. Si es asi, cualquier evento que abra el menu, provocará la salida del emulador
z80_bit menu_desactivado={0};

//indica que el menu aparece en modo multitarea - mientras ejecuta codigo de emulacion de cpu
int menu_multitarea=1;

//indica que se ha pulsado ESC y por tanto debe aparecer el menu
//y tambien, la lectura de puertos de teclado (254) no devuelve nada
int menu_abierto=0;

//indica si hay pendiente un mensaje de error por mostrar
int if_pending_error_message=0;

//mensaje de error pendiente a mostrar
char pending_error_message[1024];

//Indica que hay que salir de todos los menus. Esto sucede, por ejemplo, al cargar snapshot
int salir_todos_menus;

//char *welcome_message_key=" Press ESC for menu ";

char *esc_key_message="ESC";
char *openmenu_key_message="F5/Button";


//Gestionar pulsaciones directas de teclado o joystick
//para quickload
z80_bit menu_button_quickload={0};
//para on screen keyboard
z80_bit menu_button_osdkeyboard={0};
z80_bit menu_button_osdkeyboard_return={0};

//Comun para zx8081 y spectrum
z80_bit menu_button_osdkeyboard_caps={0};
//Solo para spectrum
z80_bit menu_button_osdkeyboard_symbol={0};
//Solo para zx81
z80_bit menu_button_osdkeyboard_enter={0};

z80_bit menu_button_exit_emulator={0};

z80_bit menu_event_drag_drop={0};

//char menu_event_drag_drop_file[PATH_MAX];

//Para evento de entrada en paso a paso desde remote protocol
z80_bit menu_event_remote_protocol_enterstep={0};


//Si menus de confirmacion asumen siempre yes y no preguntan nunca
z80_bit force_confirm_yes={0};


void menu_dibuja_cuadrado(z80_byte x1,z80_byte y1,z80_byte x2,z80_byte y2,z80_byte color);
void menu_desactiva_cuadrado(void);
void menu_establece_cuadrado(z80_byte x1,z80_byte y1,z80_byte x2,z80_byte y2,z80_byte color);
int menu_confirm_yesno(char *texto_ventana);
int menu_confirm_yesno_texto(char *texto_ventana,char *texto_interior);

void menu_espera_tecla(void);
void menu_espera_tecla_timeout_tooltip(void);
z80_byte menu_da_todas_teclas(void);
void menu_cpu_core_loop(void);

void menu_reset_counters_tecla_repeticion(void);
void menu_textspeech_send_text(char *texto);

//si hay recuadro activo, y cuales son sus coordenadas y color

int cuadrado_activo=0;
z80_byte cuadrado_x1,cuadrado_y1,cuadrado_x2,cuadrado_y2,cuadrado_color;

int draw_bateria_contador=0;
int draw_cpu_use=0;
int draw_cpu_temp=0;
int draw_fps=0;

//Si driver de video soporta lectura de teclas F
int f_functions;

//Si hay que escribir las letras de atajos de menu en inverso. Esto solo sucede cuando salta el tooltip o cuando se pulsa una tecla
//que no es atajo
z80_bit menu_writing_inverse_color={0};

//Si forzar letras en color inverso siempre
z80_bit menu_force_writing_inverse_color={0};


int estilo_gui_activo=0;

estilos_gui definiciones_estilos_gui[ESTILOS_GUI]={
	{"ZEsarUX",7+8,0,
		0,1,1,0, 		//No mostrar cursor,mostrar recuadro,mostrar rainbow
		5+8,0, 		//Colores para opcion seleccionada
		7+8,2,7,2, 	//Colores para opcion no disponible
		0,7+8,        	//Colores para el titulo
		1,		//Color waveform
		7		//Color para zona no usada en visualmem
		},
	{"ZXSpectr",1,6,
		1,0,0,0,		//Mostrar cursor >, no mostrar recuadro, no mostrar rainbow
		1,6,		//Colores para opcion seleccionada
		1,6,1,6,	//Colores para opcion no disponible, iguales que para opcion disponible
		1,6,		//Colores para el titulo
		6,		//Color waveform
		0               //Color para zona no usada en visualmem
		},

        {"ZX80/81",7+8,0,
                1,0,0,1,          //Mostrar cursor >, no mostrar recuadro, no mostrar rainbow
                0,7+8,          //Colores para opcion seleccionada
                7+8,0,0,7+8,      //Colores para opcion no disponible
                7+8,0,          //Colores para el titulo
                0,              //Color waveform
                7               //Color para zona no usada en visualmem
                },

//Lo ideal en Z88 seria mismos colores que Z88... Pero habria que revisar para otros drivers, tal como curses o cacalib
//que no tienen esos colores en las fuentes
//Al menos hacemos colores sin brillo
        {"Z88",7,0,
                0,1,0,0,                //No mostrar cursor,mostrar recuadro,no mostrar rainbow
                4,0,          //Colores para opcion seleccionada
                7,2,4,2,      //Colores para opcion no disponible
                0,7,          //Colores para el titulo
                4,              //Color waveform
                4               //Color para zona no usada en visualmem
                },


        {"CPC",1,6+8,
                0,1,1,0,          //No mostrar cursor,mostrar recuadro,mostrar rainbow
                6+8,1,            //Colores para opcion seleccionada
                1,2,6+8,2,        //Colores para opcion no disponible
                6+8,1,            //Colores para el titulo
                6+8,              //Color waveform
                0               //Color para zona no usada en visualmem
                },

        {"Sam",7+8,0,
                0,1,1,0,                //No mostrar cursor,mostrar recuadro,mostrar rainbow
                5+8,0,          //Colores para opcion seleccionada
                7+8,2,7,2,      //Colores para opcion no disponible
                0,7+8,          //Colores para el titulo
                1,              //Color waveform
                7               //Color para zona no usada en visualmem
                },

						{"ManSoftware",7+8,0,
							0,1,1,0, 		//No mostrar cursor,mostrar recuadro,mostrar rainbow
							5+8,0, 		//Colores para opcion seleccionada
							7+8,2,7,2, 	//Colores para opcion no disponible
							0,7+8,        	//Colores para el titulo
							1,		//Color waveform
							7		//Color para zona no usada en visualmem
							},



};



//valores de la ventana mostrada

z80_byte ventana_x,ventana_y,ventana_ancho,ventana_alto;

#define MENU_OPCION_SEPARADOR 0
#define MENU_OPCION_NORMAL 1
#define MENU_OPCION_ESC 2

#define MENU_ITEM_PARAMETERS int valor_opcion GCC_UNUSED

//valores de retorno de seleccion de menu
//#define MENU_SELECCION_ESC -1

//funcion a la que salta al darle al enter. valor_opcion es un valor que quien crea el menu puede haber establecido,
//para cada item de menu, un valor diferente
//al darle enter se envia el valor de ese item seleccionado a la opcion de menu
typedef void (*t_menu_funcion)(MENU_ITEM_PARAMETERS);

//funcion que retorna 1 o 0 segun si opcion activa
typedef int (*t_menu_funcion_activo)(void);



//Aunque en driver xwindows no cabe mas de 30 caracteres, en stdout, por ejemplo, cabe mucho mas.
#define MAX_TEXTO_OPCION 60

struct s_menu_item {
	//texto de la opcion
	//char *texto;

	//Aunque en driver xwindows no cabe mas de 30 caracteres, en stdout, por ejemplo, cabe mucho mas
	char texto_opcion[MAX_TEXTO_OPCION];

	//texto de ayuda
	char *texto_ayuda;

	//texto de tooltip
	char *texto_tooltip;

	//atajo de teclado
	z80_byte atajo_tecla;

	//un valor enviado a la opcion, que puede establecer la funcion que agrega el item
	int valor_opcion;

	//tipo de la opcion
	int tipo_opcion;
	//funcion a la que debe saltar
	t_menu_funcion menu_funcion;
	//funcion que retorna 1 o 0 segun si opcion activa
	t_menu_funcion_activo menu_funcion_activo;

	//funcion a la que debe saltar al pulsar espacio
	t_menu_funcion menu_funcion_espacio;

	//siguiente item
	struct s_menu_item *next;
};

typedef struct s_menu_item menu_item;


//Elemento que identifica a un archivo en funcion de seleccion
struct s_filesel_item{
	//struct dirent *d;
	char d_name[PATH_MAX];
        unsigned char  d_type;

        //siguiente item
        struct s_filesel_item *next;
};

typedef struct s_filesel_item filesel_item;

int filesel_total_items;
filesel_item *primer_filesel_item;

//linea seleccionada en selector de archivos (relativa al primer archivo 0...20 maximo seguramente)
int filesel_linea_seleccionada;

//numero de archivo seleccionado en selector (0...ultimo archivo en directorio)
int filesel_archivo_seleccionado;

//indica en que zona del selector estamos:
//0: nombre archivo
//1: selector de archivo
//2: zona filtros
int filesel_zona_pantalla;


//nombre completo (nombre+path)del archivo seleccionado
char filesel_nombre_archivo_seleccionado[PATH_MAX];


void menu_add_item_menu_inicial(menu_item **m,char *texto,int tipo_opcion,t_menu_funcion menu_funcion,t_menu_funcion_activo menu_funcion_activo);
void menu_add_item_menu_inicial_format(menu_item **p,int tipo_opcion,t_menu_funcion menu_funcion,t_menu_funcion_activo menu_funcion_activo,const char * format , ...);
void menu_add_item_menu(menu_item *m,char *texto,int tipo_opcion,t_menu_funcion menu_funcion,t_menu_funcion_activo menu_funcion_activo);
void menu_add_item_menu_format(menu_item *m,int tipo_opcion,t_menu_funcion menu_funcion,t_menu_funcion_activo menu_funcion_activo,const char * format , ...);
void menu_add_item_menu_ayuda(menu_item *m,char *texto_ayuda);
void menu_add_item_menu_tooltip(menu_item *m,char *texto_tooltip);
void menu_add_item_menu_shortcut(menu_item *m,z80_byte tecla);
void menu_add_item_menu_valor_opcion(menu_item *m,int valor_opcion);

int menu_tooltip_counter;
#define TOOLTIP_SECONDS 4

z80_bit tooltip_enabled;




void menu_ventana_scanf(char *titulo,char *texto,int max_length);
int menu_display_cursesstdout_cond(void);

void menu_tape_settings_trunc_name(char *orig,char *dest,int max);

void menu_filesel_chdir(char *dir);

void menu_change_audio_driver(MENU_ITEM_PARAMETERS);

void menu_chardetection_settings(MENU_ITEM_PARAMETERS);

void menu_debug_disassemble(MENU_ITEM_PARAMETERS);

void menu_tape_settings(MENU_ITEM_PARAMETERS);

void menu_hardware_memory_settings(MENU_ITEM_PARAMETERS);

int menu_tape_settings_cond(void);

int menu_inicio_opcion_seleccionada=0;
int machine_selection_opcion_seleccionada=0;
int machine_selection_por_fabricante_opcion_seleccionada=0;
int display_settings_opcion_seleccionada=0;
int audio_settings_opcion_seleccionada=0;
int hardware_settings_opcion_seleccionada=0;
int hardware_memory_settings_opcion_seleccionada=0;
int tape_settings_opcion_seleccionada=0;
int snapshot_opcion_seleccionada=0;
int debug_settings_opcion_seleccionada=0;
int chardetection_settings_opcion_seleccionada=0;
int textspeech_opcion_seleccionada=0;
int about_opcion_seleccionada=0;
int interface_settings_opcion_seleccionada=0;
int hotswap_machine_opcion_seleccionada=0;
int hardware_advanced_opcion_seleccionada=0;
int custom_machine_opcion_seleccionada;
int hardware_printers_opcion_seleccionada=0;
int cpu_stats_opcion_seleccionada=0;
int input_file_keyboard_opcion_seleccionada=0;
int change_video_driver_opcion_seleccionada=0;
int change_audio_driver_opcion_seleccionada=0;
int hardware_realjoystick_opcion_seleccionada=0;
int hardware_realjoystick_event_opcion_seleccionada=0;
int hardware_realjoystick_keys_opcion_seleccionada=0;
int breakpoints_opcion_seleccionada=0;
int watches_opcion_seleccionada=0;
int z88_slots_opcion_seleccionada=0;
int z88_slot_insert_opcion_seleccionada=0;
int z88_eprom_size_opcion_seleccionada=0;
int z88_flash_intel_size_opcion_seleccionada=0;
int find_opcion_seleccionada=0;
int cpu_transaction_log_opcion_seleccionada=0;
int storage_settings_opcion_seleccionada=0;
int external_tools_config_opcion_seleccionada=0;
int debug_pok_file_opcion_seleccionada=0;
int poke_opcion_seleccionada=0;
int timexcart_opcion_seleccionada=0;
int mmc_divmmc_opcion_seleccionada=0;
int ide_divide_opcion_seleccionada=0;
int hardware_redefine_keys_opcion_seleccionada=0;
int ula_settings_opcion_seleccionada=0;
//int debug_configuration_opcion_seleccionada=0;
int dandanator_opcion_seleccionada=0;
int superupgrade_opcion_seleccionada=0;
int multiface_opcion_seleccionada=0;


int settings_opcion_seleccionada=0;
int settings_audio_opcion_seleccionada=0;
int settings_snapshot_opcion_seleccionada=0;
int settings_debug_opcion_seleccionada=0;
int settings_display_opcion_seleccionada=0;
int settings_storage_opcion_seleccionada=0;
int settings_tape_opcion_seleccionada=0;
int settings_config_file_opcion_seleccionada=0;
int ay_player_opcion_seleccionada=0;


//Indica que esta el splash activo o cualquier otro texto de splash, como el de cambio de modo de video
z80_bit menu_splash_text_active;

//segundos que le faltan para desactivarse
int menu_splash_segundos=0;

void menu_simple_ventana(char *titulo,char *texto);


//filtros activos
char **filesel_filtros;

int menu_filesel(char *titulo,char *filtros[],char *archivo);

int menu_avisa_si_extension_no_habitual(char *filtros[],char *archivo);

//filtros iniciales con los que se llama a la funcion
char **filesel_filtros_iniciales;

//filtro de todos archivos
char *filtros_todos_archivos[2];

//cinta seleccionada. tapefile apuntara aqui
char tape_open_file[PATH_MAX];
//cinta seleccionada. tape_out_file apuntara aqui
char tape_out_open_file[PATH_MAX];


//Ultimo directorio al salir con ESC desde fileselector
char menu_filesel_last_directory_seen[PATH_MAX];

char menu_buffer_textspeech_filter_program[PATH_MAX];
char menu_buffer_textspeech_stop_filter_program[PATH_MAX];

//cinta real seleccionada. realtape_name apuntara aqui
char menu_realtape_name[PATH_MAX];

//aofile. aofilename apuntara aqui
char aofilename_file[PATH_MAX];

//vofile. vofilename apuntara aqui
char vofilename_file[PATH_MAX];


//snapshot load. snapfile apuntara aqui
char snapshot_load_file[PATH_MAX];
char snapshot_save_file[PATH_MAX];

char binary_file_load[PATH_MAX]="";
char binary_file_save[PATH_MAX];

char file_viewer_file_name[PATH_MAX]="";

char *quickfile=NULL;
//quickload seleccionada. quickfile apuntara aqui
char quickload_file[PATH_MAX];

//archivo zxprinter bitmap
char zxprinter_bitmap_filename_buffer[PATH_MAX];
//archivo zxprinter texto ocr
char zxprinter_ocr_filename_buffer[PATH_MAX];


char last_timex_cart[PATH_MAX]="";
char last_ay_file[PATH_MAX]="";

void menu_debug_hexdump_with_ascii(char *dumpmemoria,menu_z80_moto_int dir_leida,int bytes_por_linea);
void menu_debug_dissassemble_una_instruccion(char *dumpassembler,menu_z80_moto_int dir,int *longitud_final_opcode);

//directorio inicial al entrar
char filesel_directorio_inicial[PATH_MAX];

void menu_warn_message(char *texto);
void menu_error_message(char *texto);

void menu_generic_message(char *titulo, const char * texto);
void menu_generic_message_format(char *titulo, const char * format , ...);


struct s_generic_message_tooltip_return {
	char texto_seleccionado[40];
	int linea_seleccionada;
	int estado_retorno; //Retorna 1 si sale con enter. Retorna 0 si sale con ESC
};

typedef struct s_generic_message_tooltip_return generic_message_tooltip_return;

void menu_generic_message_tooltip(char *titulo, int tooltip_enabled, int mostrar_cursor, generic_message_tooltip_return *retorno, const char * texto_format , ...);



//Cambia a directorio donde estan los archivos de instalacion (en share o en ..Resources)

void menu_chdir_sharedfiles(void)
{

	//cambia a los dos directorios. se quedara en el ultimo que exista
	debug_printf(VERBOSE_INFO,"Trying ../Resources");
	menu_filesel_chdir("../Resources");

	char installshare[PATH_MAX];
	sprintf (installshare,"%s/%s",INSTALL_PREFIX,"/share/zesarux/");
	debug_printf(VERBOSE_INFO,"Trying %s",installshare);
	menu_filesel_chdir(installshare);


}


//retorna dentro de un array de N teclas, la tecla pulsada
char menu_get_key_array_n_teclas(z80_byte valor_puerto,char *array_teclas,int teclas)
{

        int i;
        for (i=0;i<teclas;i++) {
                if ((valor_puerto&1)==0) return *array_teclas;
                valor_puerto=valor_puerto >> 1;
                array_teclas++;
        }

        return 0;

}



//retorna dentro de un array de 5 teclas, la tecla pulsada
char menu_get_key_array(z80_byte valor_puerto,char *array_teclas)
{

	return	menu_get_key_array_n_teclas(valor_puerto,array_teclas,5);

}

//funcion que retorna la tecla pulsada, solo tener en cuenta caracteres y numeros, sin modificador (mayus, etc)
//y por tanto solo 1 tecla a la vez

/*
z80_byte puerto_65278=255; //    db    		 255  ; V    C    X    Z    Sh    ;0
z80_byte puerto_65022=255; //    db    		 255  ; G    F    D    S    A     ;1
z80_byte puerto_64510=255; //    db              255  ; T    R    E    W    Q     ;2
z80_byte puerto_63486=255; //    db              255  ; 5    4    3    2    1     ;3
z80_byte puerto_61438=255; //    db              255  ; 6    7    8    9    0     ;4
z80_byte puerto_57342=255; //    db              255  ; Y    U    I    O    P     ;5
z80_byte puerto_49150=255; //    db              255  ; H    J    K    L    Enter ;6
z80_byte puerto_32766=255; //    db              255  ; B    N    M    Simb Space ;7

//puertos especiales no presentes en spectrum
z80_byte puerto_especial1=255; //   ;  .  .  .  . ESC ;
z80_byte puerto_especial2=255; //   F5 F4 F3 F2 F1
z80_byte puerto_especial3=255; //  F10 F9 F8 F7 F6
*/

static char menu_array_keys_65022[]="asdfg";
static char menu_array_keys_64510[]="qwert";
static char menu_array_keys_63486[]="12345";
static char menu_array_keys_61438[]="09876";
static char menu_array_keys_57342[]="poiuy";
static char menu_array_keys_49150[]="\x0dlkjh";

//arrays especiales
static char menu_array_keys_65278[]="zxcv";
static char menu_array_keys_32766[]="mnb";


/*
valores de teclas especiales:
2  ESC
8  cursor left
9  cursor right
10 cursor down
11 cursor up
12 Delete o joystick left
13 Enter o joystick fire
15 SYM+MAY(TAB)
24 PgUp
25 PgDn

Joystick izquierda funcionara como Delete, no como cursor left. Resto de direcciones de joystick (up, down, right) se mapean como cursores

*/

/*
Teclas que tienen que retornar estas funciones para todas las maquinas posibles: spectrum, zx80/81, z88, cpc, sam, etc:

Letras y numeros

.  , : / - + < > = ' ( ) "


Hay drivers que retornan otros simbolos adicionales, por ejemplo en Z88 el ;. Esto es porque debe retornar ":" y estos : se obtienen mediante
mayusculas + tecla ";"

*/

z80_byte menu_get_pressed_key_no_modifier(void)
{
	z80_byte tecla;

        //Y tecla "." en modo zx80/81, que sale con tecla symbol shift
        if (MACHINE_IS_ZX8081) {
                if ((puerto_32766&2)==0) {
                        return 'm';
                }
        }

	//Teclas para Z88. Esto hace falta para drivers de texto, tipo curses, que usan funcion ascii_to_keyboard_port_set_clear para todas teclas
	//Y que en caso de z88, por ejemplo con el '.', solo ponen el puerto de teclado de Z88 y no el symbol+m cuando estan en modo z88
        if (MACHINE_IS_Z88) {
                if ((blink_kbd_a15&4)==0) {
                        return '.';
                }

                if ((blink_kbd_a14&4)==0) {
                        return ',';
                }

		if ((blink_kbd_a10&128)==0) {
			return '=';
		}

		if ((blink_kbd_a11&128)==0) {
			return '-';
		}

		if ((blink_kbd_a15&2)==0) {
                        return '/';
                }

		if ((blink_kbd_a14&2)==0) {
                        return ';';
                }

		if ((blink_kbd_a14&1)==0) {
                        return '\'';
                }


		//Para '(' y ')'
		//if ((blink_kbd_a11&1)==0) {
                //        return '9';
		//}
		//if ((blink_kbd_a13&1)==0) {
                //        return '0';
		//}


        }

	if (MACHINE_IS_CPC) {
	        if ((cpc_keyboard_table[3]&128)==0) {
                        return '.';
                }

                if ((cpc_keyboard_table[4]&128)==0) {
                        return ',';
                }


		if ((cpc_keyboard_table[3]&64)==0) {
			return '/';
		}

		if ((cpc_keyboard_table[3]&32)==0) {
			return ':';
		}

		if ((cpc_keyboard_table[3]&2)==0) {
			return '-';
		}

		if ((cpc_keyboard_table[3]&16)==0) {
			return ';';
		}

	}

	if (MACHINE_IS_SAM) {
		if ((puerto_teclado_sam_7ff9&64)==0) {
			return '.';
		}

		if ((puerto_teclado_sam_7ff9&32)==0) {
			return ',';
		}

		if ((puerto_teclado_sam_bff9&64)==0) {
			return ':';
		}

		if ((puerto_teclado_sam_eff9&32)==0) {
			return '-';
		}

		if ((puerto_teclado_sam_eff9&64)==0) {
			return '+';
		}

		if ((puerto_teclado_sam_dff9&32)==0) {
			return '=';
		}

		if ((puerto_teclado_sam_dff9&64)==0) {
			return '"';
		}

	}


	//ESC significa Shift+Space en ZX-Uno y tambien ESC puerto_especial para menu.
	//Por tanto si se pulsa ESC, hay que leer como tal ESC antes que el resto de teclas (Espacio o Shift)
	if ((puerto_especial1&1)==0) return 2;

	tecla=menu_get_key_array(puerto_65022,menu_array_keys_65022);
	if (tecla) return tecla;

	tecla=menu_get_key_array(puerto_64510,menu_array_keys_64510);
	if (tecla) return tecla;

	tecla=menu_get_key_array(puerto_63486,menu_array_keys_63486);
	if (tecla) return tecla;

	tecla=menu_get_key_array(puerto_61438,menu_array_keys_61438);
	if (tecla) return tecla;

	tecla=menu_get_key_array(puerto_57342,menu_array_keys_57342);
	if (tecla) return tecla;

	tecla=menu_get_key_array(puerto_49150,menu_array_keys_49150);
	if (tecla) return tecla;

        tecla=menu_get_key_array_n_teclas(puerto_65278>>1,menu_array_keys_65278,4);
        if (tecla) return tecla;

        tecla=menu_get_key_array_n_teclas(puerto_32766>>2,menu_array_keys_32766,3);
        if (tecla) return tecla;

	//Y espacio
	if ((puerto_32766&1)==0) return ' ';

	//PgUp
	if ((puerto_especial1&2)==0) return 24;

	//PgDn
	if ((puerto_especial1&4)==0) return 25;



	return 0;
}


//Retorna si left shift o right shift de z88 pulsado
int menu_if_z88_shift(void)
{
	if ( (blink_kbd_a14&64)==0 || (blink_kbd_a15&128)==0 ) return 1;
	return 0;
}

//Retorna si shift de cpc pulsado
int menu_if_cpc_shift(void)
{
	if ( (cpc_keyboard_table[2]&32)==0 ) return 1;
        return 0;
}


//Retorna si shift de sam pulsado
int menu_if_sam_shift(void)
{
        if ( (puerto_65278 & 1)==0 ) return 1;
        return 0;
}


//devuelve tecla pulsada teniendo en cuenta mayus, sym shift
z80_byte menu_get_pressed_key(void)
{

	z80_byte tecla;

	//primero joystick
	if (puerto_especial_joystick) {
		//z80_byte puerto_especial_joystick=0; //Fire Up Down Left Right
		if ((puerto_especial_joystick&1)) return 9;

		if ((puerto_especial_joystick&2)) return 8;

		//left joystick hace delete en menu.NO
		//if ((puerto_especial_joystick&2)) return 12;

		if ((puerto_especial_joystick&4)) return 10;
		if ((puerto_especial_joystick&8)) return 11;
//8  cursor left
//9  cursor right
//10 cursor down
//11 cursor up

		//Fire igual que enter
		if ((puerto_especial_joystick&16)) return 13;
	}


	//Quiza estas dos teclas se podrian gestionar desde menu_get_pressed_key_no_modifier??
	//No estoy seguro , quiza esta aqui para que se interpreten antes que cualquier otra cosa de dentro de esa funcion
	//Mejor no mover ya que funciona....
	if (MACHINE_IS_SAM) {
		//TAB en sam coupe
		if ( (puerto_teclado_sam_f7f9 & 64)==0 ) {
			return 15;
		}

		//DEL en sam coupe
		if ( (puerto_teclado_sam_eff9 & 128)==0 ) {
			return 12;
		}

		//. en sam coupe
		//if ( (puerto_teclado_sam_7ff9 & 64)==0 ) {
		//	return '.';
		//}

	}


	tecla=menu_get_pressed_key_no_modifier();


	//Teclas < > = _, que al pulsarlas en Z88, como generan de tabla spectrum shift + symbol + m o n , se cree que es tab, y hay que leerlo antes
	//if (MACHINE_IS_Z88 && (blink_kbd_a14&64)==0 ) {

	//Z88 y shift
	if (MACHINE_IS_Z88 && menu_if_z88_shift() ) {
		if (tecla==',') return '<';
		if (tecla=='.') return '>';
		if (tecla=='=') return '+';
		if (tecla=='-') return '_';
		if (tecla=='/') return '?';
		if (tecla=='9') return '(';
		if (tecla=='0') return ')';
		if (tecla==';') return ':';
		if (tecla=='\'') return '"';
	}

	//CPC y shift
	if (MACHINE_IS_CPC && menu_if_cpc_shift() ) {
		if (tecla==';') return '+';
		if (tecla==',') return '<';
		if (tecla=='.') return '>';
		if (tecla=='-') return '=';
		if (tecla=='7') return '\'';
		if (tecla=='8') return '(';
		if (tecla=='9') return ')';
		if (tecla=='2') return '"';
	}

	//Sam coupe y shift
	if (MACHINE_IS_SAM && menu_if_sam_shift() ) {
		if (tecla=='-') return '/';
		if (tecla=='7') return '\'';
		if (tecla=='8') return '(';
		if (tecla=='9') return ')';
	}

	//Sam coupe y symbol
	if (MACHINE_IS_SAM && (puerto_32766 & 2)==0) {
		if (tecla=='q') return '<';
		if (tecla=='w') return '>';
	}


	//cuando es symbol + shift juntos, TAB->codigo 15
	if ( (puerto_65278 & 1)==0 && (puerto_32766 & 2)==0) return 15;

	if (tecla==0) return 0;


	//ver si hay algun modificador

	//mayus

//z80_byte puerto_65278=255; //    db              255  ; V    C    X    Z    Sh    ;0
	if ( (puerto_65278&1)==0) {

		//En modo zx80/81, teclas con shift
		if (MACHINE_IS_ZX8081) {
			if (tecla=='z') return ':';
			if (tecla=='v') return '/';
			if (tecla=='j') return '-';
			if (tecla=='k') return '+';
			if (tecla=='l') return '=';
			if (tecla=='n') return '<';
			if (tecla=='m') return '>';
			if (tecla=='i') return '(';
			if (tecla=='o') return ')';
			if (tecla=='p') return '"';
		}

		//si son letras, ponerlas en mayusculas
		if (tecla>='a' && tecla<='z') {
			tecla=tecla-('a'-'A');
			return tecla;
		}
		//seran numeros


		switch (tecla) {
			case '0':
				//delete
				return 12;
			break;

			/*
			//cursores
			case '5':
				return 8;
			break;

			case '6':
				return 10;
			break;

			case '7':
				return 11;
			break;

			case '8':
				return 9;
			break;
			*/
		}

	}

	//sym shift
	else if ( (puerto_32766&2)==0) {
		//ver casos concretos
		switch (tecla) {
			case 'm':
				return '.';
			break;

			case 'n':
				return ',';
			break;

			case 'z':
				return ':';
			break;

			case 'v':
				return '/';
			break;

			case 'j':
				return '-';
			break;

			case 'k':
				return '+';
			break;

			case 'r':
				return '<';
			break;

			case 't':
				return '>';
			break;

			case 'l':
				return '=';
			break;

			case '7':
				return '\'';
			break;

			case '8':
				return '(';
			break;

			case '9':
				return ')';
			break;

			case 'p':
				return '"';
			break;


			//no hace falta esta tecla. asi tambien evitamos que alguien la use en nombre de
			//archivo pensando que se puede introducir un filtro tipo *.tap, etc.
			//case 'b':
			//	return '*';
			//break;

		}
	}


	return tecla;

}

//escribe la cadena de texto
void menu_scanf_print_string(char *string,int offset_string,int max_length_shown,int x,int y)
{
	z80_byte papel=ESTILO_GUI_PAPEL_NORMAL;
	z80_byte tinta=ESTILO_GUI_TINTA_NORMAL;
	char cadena_buf[2];

	string=&string[offset_string];

	//contar que hay que escribir el cursor
	max_length_shown--;

	//y si offset>0, primer caracter sera '<'
	if (offset_string) {
		menu_escribe_texto(x,y,tinta,papel,"<");
		max_length_shown--;
		x++;
		string++;
	}

	for (;max_length_shown && (*string)!=0;max_length_shown--) {
		cadena_buf[0]=*string;
		cadena_buf[1]=0;
		menu_escribe_texto(x,y,tinta,papel,cadena_buf);
		x++;
		string++;
	}

        menu_escribe_texto(x,y,tinta,papel,"_");
        x++;


        for (;max_length_shown!=0;max_length_shown--) {
                menu_escribe_texto(x,y,tinta,papel," ");
                x++;
        }




}

//devuelve cadena de texto desde teclado
//max_length contando caracter 0 del final, es decir, para un texto de 4 caracteres, debemos especificar max_length=5
//ejemplo, si el array es de 50, se le debe pasar max_length a 50
int menu_scanf(char *string,unsigned int max_length,int max_length_shown,int x,int y)
{

	//Enviar a speech
	char buf_speech[MAX_BUFFER_SPEECH+1];
	sprintf (buf_speech,"Edit box: %s",string);
	menu_textspeech_send_text(buf_speech);


	z80_byte tecla;

	//ajustar offset sobre la cadena de texto visible en pantalla
	int offset_string;

	int j;
	j=strlen(string);
	if (j>max_length_shown-1) offset_string=j-max_length_shown+1;
	else offset_string=0;


	//max_length ancho maximo del texto, sin contar caracter 0
	//por tanto si el array es de 50, se le debe pasar max_length a 50

	max_length--;

	//cursor siempre al final del texto

	do {
		menu_scanf_print_string(string,offset_string,max_length_shown,x,y);

		if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();

		menu_espera_tecla();
		tecla=menu_get_pressed_key();
		menu_espera_no_tecla();

		//si tecla normal, agregar:
		if (tecla>31 && tecla<128) {
			if (strlen(string)<max_length) {
				int i;
				i=strlen(string);
				string[i++]=tecla;
				string[i]=0;

				//Enviar a speech letra pulsada
				menu_speech_tecla_pulsada=0;
			        sprintf (buf_speech,"%c",tecla);
        			menu_textspeech_send_text(buf_speech);


				if (i>=max_length_shown) offset_string++;

			}
		}

		//tecla borrar o tecla izquierda
		if (tecla==12 || tecla==8) {
			if (strlen(string)>0) {
                                int i;
                                i=strlen(string)-1;

                                //Enviar a speech letra borrada

				menu_speech_tecla_pulsada=0;
                                sprintf (buf_speech,"%c",string[i]);
                                menu_textspeech_send_text(buf_speech);


                                string[i]=0;
				if (offset_string>0) {
					offset_string--;
					//printf ("offset string: %d\n",offset_string);
				}
			}

		}

		//tecla tabulador. a efectos practicos, es ENTER igual
		//if (tecla==15) {
		//	printf ("TAB pulsado en scanf\n");
		//	tecla=13;
		//}

	} while (tecla!=13 && tecla!=15 && tecla!=2);

	//if (tecla==13) printf ("salimos con enter\n");
	//if (tecla==15) printf ("salimos con tab\n");

	menu_reset_counters_tecla_repeticion();
	return tecla;

//papel=7+8;
//tinta=0;

}



//funcion para asignar funcion de overlay
void set_menu_overlay_function(void (*funcion)(void) )
{

	menu_overlay_function=funcion;

	//para que al oscurecer la pantalla tambien refresque el border
	modificado_border.v=1;
	menu_overlay_activo=1;
}


//funcion para desactivar funcion de overlay
void reset_menu_overlay_function(void)
{
	//para que al oscurecer la pantalla tambien refresque el border
	modificado_border.v=1;

	//si hay segunda capa, no desactivar
	//if (menu_second_layer==0) {
	//	menu_overlay_activo=0;
	//}

	menu_overlay_activo=0;
}


//funcion para escribir un caracter en el buffer de overlay
//tinta y/o papel pueden tener brillo (color+8)
void putchar_menu_overlay(int x,int y,z80_byte caracter,z80_byte tinta,z80_byte papel)
{
	int pos_array=y*32+x;
	overlay_screen_array[pos_array].tinta=tinta;
	overlay_screen_array[pos_array].papel=papel;

	if (ESTILO_GUI_SOLO_MAYUSCULAS) overlay_screen_array[pos_array].caracter=letra_mayuscula(caracter);
	else overlay_screen_array[pos_array].caracter=caracter;
}


//funcion para escribir un caracter en el buffer de segundo overlay, tambien copiarlo en el primer buffer
/*
void putchar_menu_second_overlay(int x,int y,z80_byte caracter,z80_byte tinta,z80_byte papel)
{

        int pos_array=y*32+x;
       	second_overlay_screen_array[pos_array].tinta=tinta;
        second_overlay_screen_array[pos_array].papel=papel;
       	second_overlay_screen_array[pos_array].caracter=caracter;

	if (menu_second_layer) {

		putchar_menu_overlay(x,y,caracter,tinta,papel);

	}


}
*/


void menu_putchar_footer(int x,int y,z80_byte caracter,z80_byte tinta,z80_byte papel)
{
	if (!menu_footer) return;

	//Sin interlaced
	if (video_interlaced_mode.v==0) {
		scr_putchar_footer(x,y,caracter,tinta,papel);
		return;
	}


	//Con interlaced
	//Queremos que el footer se vea bien, no haga interlaced y no haga scanlines
	//Cuando se activa interlaced se cambia la funcion de putpixel, por tanto,
	//desactivando aqui interlaced no seria suficiente para que el putpixel saliese bien


	//No queremos que le afecte el scanlines
	z80_bit antes_scanlines;
	antes_scanlines.v=video_interlaced_scanlines.v;
	video_interlaced_scanlines.v=0;

	//Escribe texto pero como hay interlaced, lo hará en una linea de cada 2
	scr_putchar_footer(x,y,caracter,tinta,papel);

	//Dado que hay interlaced, simulamos que estamos en siguiente frame de pantalla para que dibuje la linea par/impar siguiente
	interlaced_numero_frame++;
	scr_putchar_footer(x,y,caracter,tinta,papel);
	interlaced_numero_frame--;

	//restaurar scanlines
	video_interlaced_scanlines.v=antes_scanlines.v;
}

void menu_putstring_footer(int x,int y,char *texto,z80_byte tinta,z80_byte papel)
{
	while ( (*texto)!=0) {
		menu_putchar_footer(x++,y,*texto,tinta,papel);
		texto++;
	}
}

//Escribir info tarjetas memoria Z88
void menu_footer_z88(void)
{

	if (!MACHINE_IS_Z88) return;

	char nombre_tarjeta[20];
	int x=0;

	//menu_putstring_footer(0,0,get_machine_name(current_machine_type),WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);

	//borramos esa zona primero
	//                         01234567890123456789012345678901
	menu_putstring_footer(0,2,"                                ",WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);
	int i;
	for (i=1;i<=3;i++) {
		if (z88_memory_slots[i].size==0) sprintf (nombre_tarjeta," Empty ");
		else sprintf (nombre_tarjeta," %s ",z88_memory_types[z88_memory_slots[i].type]);

		//Si ocupa el texto mas de 10, cortar texto
		if (strlen(nombre_tarjeta)>10) {
			nombre_tarjeta[9]=' ';
			nombre_tarjeta[10]=0;
		}

		menu_putstring_footer(x,2,nombre_tarjeta,WINDOW_FOOTER_PAPER,WINDOW_FOOTER_INK);
		x +=10;
	}
}


void menu_footer_f5_menu(void)
{

        //Decir F5 menu en linea de tarjetas de memoria de z88
        //Y si es la primera vez
        if (!MACHINE_IS_Z88)  {
                //Y si no hay texto por encima de cinta autodetectada
                if (tape_options_set_first_message_counter==0 && tape_options_set_second_message_counter==0) {
                        char texto_f_menu[32];
                        sprintf(texto_f_menu,"%s Menu",openmenu_key_message);
                        menu_putstring_footer(0,2,texto_f_menu,WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);
		}
	}

}

void menu_clear_footer(void)
{
	if (!menu_footer) return;

	//printf ("clear footer\n");

        //Borrar footer
        if (si_complete_video_driver() ) {

                int alto=WINDOW_FOOTER_SIZE;
                int ancho=screen_get_window_size_width_no_zoom_border_en();

                int yinicial=screen_get_window_size_height_no_zoom_border_en()-alto;

                int x,y;

                //Para no andar con problemas de putpixel en el caso de realvideo desactivado,
                //usamos putpixel tal cual y calculamos zoom nosotros manualmente

                alto *=zoom_y;
                ancho *=zoom_x;

                yinicial *=zoom_y;

                z80_byte color=WINDOW_FOOTER_PAPER;

                for (y=yinicial;y<yinicial+alto;y++) {
                        //printf ("%d ",y);
                        for (x=0;x<ancho;x++) {
                                scr_putpixel(x,y,color);
                        }
                }

        }

}


//Escribir textos iniciales en el footer
void menu_init_footer(void)
{
	if (!menu_footer) return;


        //int margeny_arr=screen_borde_superior*border_enabled.v;

        if (MACHINE_IS_Z88) {
                //no hay border. estas variables se leen en modo rainbow
                //margeny_arr=0;
        }

	//Si no hay driver video
	if (scr_putpixel==NULL || scr_putpixel_zoom==NULL) return;

	debug_printf (VERBOSE_INFO,"init_footer");

	//Al iniciar emulador, si aun no hay definidas funciones putpixel, volver


	//Borrar footer
	menu_clear_footer();


	//Borrar zona con espacios
	//Tantos espacios como el nombre mas largo de maquina (Microdigital TK90X (Spanish))
	menu_putstring_footer(0,0,"                            ",WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);

	//Obtener maquina activa
	menu_putstring_footer(0,0,get_machine_name(current_machine_type),WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);

	autoselect_options_put_footer();

	menu_footer_z88();

	//Si hay lectura de flash activa en ZX-Uno
	//Esto lo hago asi porque al iniciar ZX-Uno, se ha activado el contador de texto "FLASH",
	//y en driver xwindows suele generar un evento ConfigureNotify, que vuelve a llamar a init_footer y borraria dicho texto FLASH
	//y por lo tanto no se veria el texto "FLASH" al arrancar la maquina
	//en otros drivers de video en teoria no haria falta
	if (MACHINE_IS_ZXUNO) zxuno_footer_print_flash_operating();

}


//funcion para limpiar el buffer de segunda capa de overlay
/*
void cls_menu_second_overlay(void)
{
        clear_putpixel_cache();

        int i;
        for (i=0;i<32*24;i++) second_overlay_screen_array[i].caracter=0;

}
*/

//escribir la segunda capa en la primera. llamado en la funcion de cls de la primera capa
/*
void copy_second_first_overlay(void)
{

	if (menu_second_layer) {
                int i;
                z80_byte caracter;
                for (i=0;i<32*24;i++) {
			caracter=second_overlay_screen_array[i].caracter;
			if (caracter) {
	                        overlay_screen_array[i].caracter=caracter;
        	                overlay_screen_array[i].tinta=second_overlay_screen_array[i].tinta;
                	        overlay_screen_array[i].papel=second_overlay_screen_array[i].papel;
			}
                }
	}


}
*/

//funcion para limpiar el buffer de overlay y si hay cuadrado activo
void cls_menu_overlay(void)
{
	clear_putpixel_cache();

	int i;
	for (i=0;i<32*24;i++) overlay_screen_array[i].caracter=0;

	menu_desactiva_cuadrado();

        //si hay segunda capa, escribir la segunda capa en esta primera
	//copy_second_first_overlay();


	//Si en Z88 y driver grafico, redibujamos la zona inferior
	if (MACHINE_IS_Z88) {
		screen_z88_draw_lower_screen();
	}

	//Si es CPC, entonces aqui el border es variable y por tanto tenemos que redibujarlo, pues quiza el menu esta dentro de zona de border
	modificado_border.v=1;

}

/*
void menu_clear_pixels(int xorigen,int yorigen,int ancho,int alto)
{
	int x,y;
                for (x=xorigen;x<xorigen+ancho;x++) {
                        for (y=yorigen;y<yorigen+alto;y++) {
                                scr_putpixel_zoom(x,y,15);
                        }
                }
}
*/

/*
void enable_second_layer(void)
{

	menu_second_layer=1;

	//forzar redibujar algunos contadores
	draw_bateria_contador=0;

}

void disable_second_layer(void)
{

	menu_second_layer=0;

	//cls_menu_overlay();

}
*/

void enable_footer(void)
{

        menu_footer=1;

        //forzar redibujar algunos contadores
        draw_bateria_contador=0;

}

void disable_footer(void)
{

        menu_footer=0;

        //cls_menu_overlay();

}


void pruebas_texto_menu(void)
{

        scr_putchar_menu(0,0,'Z',6+8,1);
        scr_putchar_menu(1,0,'E',6,1);
        scr_putchar_menu(2,0,'s',6+8,1);
        scr_putchar_menu(3,0,'a',6,1);
        scr_putchar_menu(4,0,'r',6+8,1);
        scr_putchar_menu(5,0,'U',6,1);
        scr_putchar_menu(6,0,'X',6+8,1);
        scr_putchar_menu(7,0,' ',6,1);
        scr_putchar_menu(8,0,' ',6,1+8);

}


//retornar puntero a campo desde texto, separado por espacios. se permiten multiples espacios entre campos
char *menu_get_cpu_use_idle_value(char *m,int campo)
{

	char c;

	while (campo>1) {
		c=*m;

		if (c==' ') {
			while (*m==' ') m++;
			campo--;
		}

		else m++;
	}

	return m;
}


long menu_cpu_use_seconds_antes=0;
long menu_cpu_use_seconds_ahora=0;
long menu_cpu_use_idle_antes=0;
long menu_cpu_use_idle_ahora=0;

int menu_cpu_use_num_cpus=1;

//devuelve valor idle desde /proc/stat de cpu
//devuelve <0 si error

//long temp_idle;

long menu_get_cpu_use_idle(void)
{

//En Mac OS X, obtenemos consumo cpu de este proceso
#if defined(__APPLE__)

	struct rusage r_usage;

	if (getrusage(RUSAGE_SELF, &r_usage)) {
		return -1;
	    /* ... error handling ... */
	}

	//printf("Total User CPU = %ld.%d\n",        r_usage.ru_utime.tv_sec,        r_usage.ru_utime.tv_usec);

	long cpu_use_mac=r_usage.ru_utime.tv_sec*100+(r_usage.ru_utime.tv_usec/10000);   //el 10000 sale de /1000000*100

	//printf ("Valor retorno: %ld\n",cpu_use_mac);

	return cpu_use_mac;

#endif

//En Linux en cambio obtenemos uso de cpu de todo el sistema
//cat /proc/stat
//cpu  2383406 37572370 7299316 91227807 7207258 18372 473173 0 0 0
//     user    nice    system   idle

	//int max_leer=DEBUG_MAX_MESSAGE_LENGTH-200;

	#define MAX_LEER (DEBUG_MAX_MESSAGE_LENGTH-200)

	//dado que hacemos un debug_printf con este texto,
	//el maximo del debug_printf es DEBUG_MAX_MESSAGE_LENGTH. Quitamos 200 que da margen para poder escribir sin
	//hacer segmentation fault

	//metemos +1 para poder poner el 0 del final
	char procstat_buffer[MAX_LEER+1];

	//char buffer_nulo[100];
	//char buffer_idle[100];
	char *buffer_idle;
	long cpu_use_idle=0;

	char *archivo_cpuuse="/proc/stat";

	if (si_existe_archivo(archivo_cpuuse) ) {
		int leidos=lee_archivo(archivo_cpuuse,procstat_buffer,MAX_LEER);

			if (leidos<1) {
				debug_printf (VERBOSE_DEBUG,"Error reading cpu status on %s",archivo_cpuuse);
	                        return -1;
        	        }

			//leidos es >=1

			//temp
			//printf ("leidos: %d DEBUG_MAX_MESSAGE_LENGTH: %d sizeof: %d\n",leidos,DEBUG_MAX_MESSAGE_LENGTH,sizeof(procstat_buffer) );

			//establecemos final de cadena
			procstat_buffer[leidos]=0;

			debug_printf (VERBOSE_PARANOID,"procstat_buffer: %s",procstat_buffer);

			//miramos numero cpus
			menu_cpu_use_num_cpus=0;

			char *p;
			p=procstat_buffer;

			while (p!=NULL) {
				p=strstr(p,"cpu");
				if (p!=NULL) {
					p++;
					menu_cpu_use_num_cpus++;
				}
			}

			if (menu_cpu_use_num_cpus==0) {
				//como minimo habra 1
				menu_cpu_use_num_cpus=1;
			}

			else {
				//se encuentra cabecera con "cpu" y luego "cpu0, cpu1", etc, por tanto,restar 1
				menu_cpu_use_num_cpus--;
			}

			debug_printf (VERBOSE_DEBUG,"cpu number: %d",menu_cpu_use_num_cpus);

			//parsear valores, usamos scanf
			//fscanf(ptr_procstat,"%s %s %s %s %s",buffer_nulo,buffer_nulo,buffer_nulo,buffer_nulo,buffer_idle);

			//parsear valores, usamos funcion propia
			buffer_idle=menu_get_cpu_use_idle_value(procstat_buffer,5);



			if (buffer_idle!=NULL) {
				//ponemos 0 al final
				int i=0;
				while (buffer_idle[i]!=' ') {
					i++;
				}

				buffer_idle[i]=0;


				debug_printf (VERBOSE_DEBUG,"idle value: %s",buffer_idle);

				cpu_use_idle=atoi(buffer_idle);
			}
	}

	else {
		cpu_use_idle=-1;
	}

	return cpu_use_idle;

}

int menu_get_cpu_use_perc(void)
{

	int usocpu=0;

	struct timeval menu_cpu_use_time;

	gettimeofday(&menu_cpu_use_time, NULL);
	menu_cpu_use_seconds_ahora=menu_cpu_use_time.tv_sec;

	menu_cpu_use_idle_ahora=menu_get_cpu_use_idle();

	if (menu_cpu_use_idle_ahora<0) return -1;

	if (menu_cpu_use_seconds_antes!=0) {
		long dif_segundos=menu_cpu_use_seconds_ahora-menu_cpu_use_seconds_antes;
		long dif_cpu_idle=menu_cpu_use_idle_ahora-menu_cpu_use_idle_antes;

		debug_printf (VERBOSE_DEBUG,"sec now: %ld before: %ld cpu now: %ld before: %ld",menu_cpu_use_seconds_ahora,menu_cpu_use_seconds_antes,
			menu_cpu_use_idle_ahora,menu_cpu_use_idle_antes);

		long uso_cpu_idle;

		//proteger division por cero
		if (dif_segundos==0) uso_cpu_idle=100;
		else uso_cpu_idle=dif_cpu_idle/dif_segundos/menu_cpu_use_num_cpus;

#if defined(__APPLE__)
		debug_printf (VERBOSE_DEBUG,"cpu use: %ld",uso_cpu_idle);
		usocpu=uso_cpu_idle;
#else
		debug_printf (VERBOSE_DEBUG,"cpu idle: %ld",uso_cpu_idle);
		//pasamos a int
		usocpu=100-uso_cpu_idle;
#endif
	}

	menu_cpu_use_seconds_antes=menu_cpu_use_seconds_ahora;
	menu_cpu_use_idle_antes=menu_cpu_use_idle_ahora;

	return usocpu;
}

void menu_draw_cpu_use(void)
{
        //solo redibujarla de vez en cuando
        if (draw_cpu_use!=0) {
                draw_cpu_use--;
                return;
        }

        //cada 5 segundos
        draw_cpu_use=50*5;

	int cpu_use=menu_get_cpu_use_perc();
	debug_printf (VERBOSE_DEBUG,"cpu: %d",cpu_use );

	//error
	if (cpu_use<0) return;

	//control de rango
	if (cpu_use>100) cpu_use=100;
	if (cpu_use<0) cpu_use=0;

	//temp forzar
	//cpu_use=100;

	char buffer_perc[5];
	sprintf (buffer_perc,"%3d%%",cpu_use);

	int x;

	//escribimos el texto
	//x=WINDOW_FOOTER_ELEMENT_X_CPU_USE+4-strlen(buffer_perc);
	x=WINDOW_FOOTER_ELEMENT_X_CPU_USE;

	menu_putstring_footer(x,WINDOW_FOOTER_ELEMENT_Y_CPU_USE,buffer_perc,WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);

}


//Retorna -1 si hay algun error
int menu_get_cpu_temp(void)
{

	char procstat_buffer[10];
	char *archivo_cputemp="/sys/class/thermal/thermal_zone0/temp";

	//temp pruebas
	//return 56324

        if (si_existe_archivo(archivo_cputemp) ) {

		int leidos=lee_archivo(archivo_cputemp,procstat_buffer,9);

		if (leidos<1) {
                                debug_printf (VERBOSE_DEBUG,"Error reading cpu status on %s",archivo_cputemp);
                                return -1;
                }

                //establecemos final de cadena
                procstat_buffer[leidos]=0;


		return atoi(procstat_buffer);
	}

	return -1;
}

void menu_draw_cpu_temp(void)
{
        //solo redibujarla de vez en cuando
        if (draw_cpu_temp!=0) {
                draw_cpu_temp--;
                return;
        }

        //cada 10 segundos
        draw_cpu_temp=50*10;

        int cpu_temp=menu_get_cpu_temp();
        debug_printf (VERBOSE_DEBUG,"CPU temp: %d",cpu_temp );

	//algun error al leer temperatura
	if (cpu_temp<0) return;

        //control de rango
        if (cpu_temp>99999) cpu_temp=99999;


        //temp forzar
        //cpu_temp=100;

        char buffer_temp[6];
        sprintf (buffer_temp,"%d.%dC",cpu_temp/1000,(cpu_temp%1000)/100 );

        //primero liberar esas zonas
        int x;
        //for (x=WINDOW_FOOTER_ELEMENT_X_CPU_TEMP;x<WINDOW_FOOTER_ELEMENT_X_CPU_TEMP+5;x++) putchar_menu_second_overlay(x++,WINDOW_FOOTER_ELEMENT_Y_CPU_TEMP,0,0,0);
        //for (x=WINDOW_FOOTER_ELEMENT_X_CPU_TEMP;x<WINDOW_FOOTER_ELEMENT_X_CPU_TEMP+5;x++) menu_putchar_footer(x++,WINDOW_FOOTER_ELEMENT_Y_CPU_TEMP,' ',0,0);

        //luego escribimos el texto
        x=WINDOW_FOOTER_ELEMENT_X_CPU_TEMP+5-strlen(buffer_temp);
        //int i=0;

	/*
        while (buffer_temp[i]) {
                //putchar_menu_second_overlay(x++,WINDOW_FOOTER_ELEMENT_Y_CPU_TEMP,buffer_temp[i],7+8,0);
                menu_putchar_footer(x++,WINDOW_FOOTER_ELEMENT_Y_CPU_TEMP,buffer_temp[i],7+8,0);
                i++;
        }
	*/
	menu_putstring_footer(x,WINDOW_FOOTER_ELEMENT_Y_CPU_TEMP,buffer_temp,WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);
}

void menu_draw_fps(void)
{
        //solo redibujarla de vez en cuando
        if (draw_fps!=0) {
                draw_fps--;
                return;
        }

        //cada 1 segundo
        draw_fps=50*1;

        int fps=ultimo_fps;
        debug_printf (VERBOSE_DEBUG,"FPS: %d",fps);

        //algun error al leer fps
        if (fps<0) return;

        //control de rango
        if (fps>50) fps=50;

	const int ancho_maximo=6;

        char buffer_fps[ancho_maximo+1];
        sprintf (buffer_fps,"%02d FPS",fps);

        //primero liberar esas zonas
        int x;
        //for (x=WINDOW_FOOTER_ELEMENT_X_FPS;x<WINDOW_FOOTER_ELEMENT_X_FPS+ancho_maximo-1;x++) putchar_menu_second_overlay(x++,0,0,0,0);
        //for (x=WINDOW_FOOTER_ELEMENT_X_FPS;x<WINDOW_FOOTER_ELEMENT_X_FPS+ancho_maximo-1;x++) menu_putchar_footer(x++,1,' ',0,0);

        //luego escribimos el texto
        x=WINDOW_FOOTER_ELEMENT_X_FPS;
        //int i=0;

        /*while (buffer_fps[i]) {
                //putchar_menu_second_overlay(x++,0,buffer_fps[i],7+8,0);
                menu_putchar_footer(x++,1,buffer_fps[i],7+8,0);
                i++;
        }
	*/

	menu_putstring_footer(x,WINDOW_FOOTER_ELEMENT_Y_FPS,buffer_fps,WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);
}


int menu_get_bateria_perc(void)
{
        //temp forzar
        return 25;

}

void menu_draw_bateria(void)
{

	//solo redibujarla de vez en cuando
	if (draw_bateria_contador!=0) {
		draw_bateria_contador--;
		return;
	}

	//cada 10 segundos
	draw_bateria_contador=50*10;

	//printf ("redibujamos bateria\n");

	//int x,y;
	//simulacion indicador de bateria
        //ponemos esas zonas reservadas siempre
        //putchar_menu_second_overlay(WINDOW_FOOTER_ELEMENT_X_BATERIA,0,1,0,0);
        //putchar_menu_second_overlay(WINDOW_FOOTER_ELEMENT_X_BATERIA+1,0,1,0,0);
        menu_putchar_footer(WINDOW_FOOTER_ELEMENT_X_BATERIA,1,1,0,0);
        menu_putchar_footer(WINDOW_FOOTER_ELEMENT_X_BATERIA+1,1,1,0,0);

        //dibujamos cuadrado en blanco, ancho 16
        int ancho=16;
        //int alto=8;

        int bateriaperc=menu_get_bateria_perc();

	//comprobaciones de rango
	if (bateriaperc>100) bateriaperc=100;
	if (bateriaperc<0) bateriaperc=0;



        int anchobateria=(ancho*bateriaperc)/100;

	//al menos un pixel de ancho
	if (anchobateria==0) anchobateria=1;


	//int colorbateria=0;
	//if (bateriaperc<20) colorbateria=2;


	//A continuacion se dibuja mediante putpixel
	//Esto estaba pensado para la zona superior.
	//Si se quiere volver a habilitar hay que recalcularlo para la zona de footer
	/*

        //Bateria disponible
        for (x=256-ancho;x<256-ancho+anchobateria;x++) {
                for (y=0;y<alto;y++) {
                        scr_putpixel_zoom(x,y,colorbateria);
                }
        }

        //Resto en blanco

        for (;x<256;x++) {
                for (y=0;y<alto;y++) {
                        scr_putpixel_zoom(x,y,7);
                }
        }


        //dibujo rectangulo

	//arriba y abajo
        for (x=256-ancho;x<256-2;x++) {
                scr_putpixel_zoom(x,0,colorbateria);
                scr_putpixel_zoom(x,alto-1,colorbateria);
        }

	//zona derecha
	scr_putpixel_zoom(255,0,7);
	for (y=1;y<alto-1;y++) {
		scr_putpixel_zoom(255,y,colorbateria);
	}
	scr_putpixel_zoom(255,y,7);

	*/


}

//Muestra en pantalla los caracteres de la segunda capa e indicadores de cinta, etc
/*
void draw_second_layer_overlay(void)
{


#ifdef EMULATE_RASPBERRY
                menu_draw_cpu_temp();
#endif

	menu_draw_cpu_use();
	menu_draw_fps();


	//Escribir cualquier otra cosa que no sea texto, como dibujos o puntos, lineas, indicador de bateria, por ejemplo
	if (si_complete_video_driver() ) {
#ifdef EMULATE_RASPBERRY

		//De momento bateria solo en pruebas
		//menu_draw_bateria();
#endif
	}

	//copy_second_first_overlay();

}
*/


//Aqui se llama desde cada driver de video al refrescar la pantalla
//Importante que lo que se muestre en footer se haga cada cierto tiempo y no siempre, sino saturaria la cpu probablemente
void draw_footer(void)
{

	if (menu_footer==0) return;

#ifdef EMULATE_RASPBERRY
                menu_draw_cpu_temp();
#endif

        menu_draw_cpu_use();
        menu_draw_fps();


        //Escribir cualquier otra cosa que no sea texto, como dibujos o puntos, lineas, indicador de bateria, por ejemplo
        if (si_complete_video_driver() ) {
#ifdef EMULATE_RASPBERRY

                //De momento bateria solo en pruebas
                //menu_draw_bateria();
#endif
        }


}


//0 si no valido
//1 si valido
int si_valid_char(z80_byte caracter)
{
	if (si_complete_video_driver() ) {
		if (caracter<32 || caracter>MAX_CHARSET_GRAPHIC) return 0;
	}

	else {
		if (caracter<32 || caracter>127) return 0;
	}

	return 1;
}


//funcion normal de impresion de overlay de buffer de texto y cuadrado de lineas usado en los menus
void normal_overlay_texto_menu(void)
{

	int x,y;
	z80_byte tinta,papel,caracter;
	int pos_array=0;


	//printf ("normal_overlay_texto_menu\n");
	for (y=0;y<24;y++) {
		for (x=0;x<32;x++,pos_array++) {
			caracter=overlay_screen_array[pos_array].caracter;
			//si caracter es 0, no mostrar
			if (caracter) {
				//128 y 129 corresponden a franja de menu y a letra enye minuscula
				if (si_valid_char(caracter) ) {
					tinta=overlay_screen_array[pos_array].tinta;
					papel=overlay_screen_array[pos_array].papel;
					scr_putchar_menu(x,y,caracter,tinta,papel);
				}

				else if (caracter==255) {
					//Significa no mostrar caracter. Usado en pantalla panic
				}

				//Si caracter no valido, mostrar ?
				else {
					tinta=overlay_screen_array[pos_array].tinta;
					papel=overlay_screen_array[pos_array].papel;
					scr_putchar_menu(x,y,'?',tinta,papel);
				}
			}
		}
	}

	if (cuadrado_activo) menu_dibuja_cuadrado(cuadrado_x1,cuadrado_y1,cuadrado_x2,cuadrado_y2,cuadrado_color);

	//si hay segunda capa, escribirla. aqui normalmente solo se escriben dibujos que no se pueden meter como texto, como la bateria
	/*
	if (menu_second_layer) {
		draw_second_layer_overlay();
	}
	*/

	//if (menu_footer) {
	//	draw_footer();
	//}


}


//establece cuadrado activo usado en los menus para xwindows y fbdev
void menu_establece_cuadrado(z80_byte x1,z80_byte y1,z80_byte x2,z80_byte y2,z80_byte color)
{

	cuadrado_x1=x1;
	cuadrado_y1=y1;
	cuadrado_x2=x2;
	cuadrado_y2=y2;
	cuadrado_color=color;
	cuadrado_activo=1;

}

//desactiva cuadrado  usado en los menus para xwindows y fbdev
void menu_desactiva_cuadrado(void)
{
	cuadrado_activo=0;
}

//Devuelve 1 si hay dos ~~ seguidas en la posicion del indice
//Sino, 0
int menu_escribe_texto_si_inverso(char *texto, int indice)
{
	if (texto[indice++]!='~') return 0;
	if (texto[indice++]!='~') return 0;

	//Y siguiente caracter no es final de texto
	if (texto[indice]==0) return 0;

	return 1;
}


//escribe una linea de texto
//coordenadas relativas al interior de la pantalla de spectrum (0,0=inicio pantalla)

//Si codigo de color inverso, invertir una letra
//Codigo de color inverso: dos ~ seguidas
void menu_escribe_texto(z80_byte x,z80_byte y,z80_byte tinta,z80_byte papel,char *texto)
{
        unsigned int i;
	z80_byte letra;

        //y luego el texto
        for (i=0;i<strlen(texto);i++) {
		letra=texto[i];

		//ver si dos ~~ seguidas y cuidado al comparar que no nos vayamos mas alla del codigo 0 final
		if (menu_escribe_texto_si_inverso(texto,i)) {
			//y saltamos esos codigos de negado
			i +=2;
			letra=texto[i];

			if (menu_writing_inverse_color.v) putchar_menu_overlay(x,y,letra,papel,tinta);
			else putchar_menu_overlay(x,y,letra,tinta,papel);
		}

		else putchar_menu_overlay(x,y,letra,tinta,papel);
		x++;
	}

}



//escribe una linea de texto
//coordenadas relativas al interior de la ventana (0,0=inicio zona "blanca")
void menu_escribe_texto_ventana(z80_byte x,z80_byte y,z80_byte tinta,z80_byte papel,char *texto)
{

	menu_escribe_texto(ventana_x+x,ventana_y+y+1,tinta,papel,texto);

/*
	unsigned int i;

        //y luego el texto
        for (i=0;i<strlen(texto);i++) putchar_menu_overlay(ventana_x+x+i,ventana_y+y+1,texto[i],tinta,papel);
*/

}


void menu_textspeech_send_text(char *texto)
{

	if (textspeech_filter_program==NULL) return;
	if (textspeech_also_send_menu.v==0) return;
	if (menu_speech_tecla_pulsada) return;

	debug_printf (VERBOSE_DEBUG,"Send text to speech: %s",texto);


	//Eliminamos las ~~ del texto. Realmente eliminamos cualquier ~ aunque solo haya una
	//Si hay dos ~~, decir que atajo es al final del texto
	int orig,dest;
	orig=dest=0;
	char buf_speech[MAX_BUFFER_SPEECH+1];

	char letra_atajo=0;

	//printf ("texto: puntero: %d strlen: %d : %s\n",texto,strlen(texto),texto);

	for (;texto[orig]!=0;orig++) {

		if (texto[orig]=='~' && texto[orig+1]=='~') {
			letra_atajo=texto[orig+2];
		}

		//printf ("texto orig : %d\n",texto[orig]);
		//printf ("texto orig char: %c\n",texto[orig]);
		//Si no es ~, copiar e incrementar destino
		if (texto[orig]!='~') {
			buf_speech[dest]=texto[orig];
			dest++;
		}
	}


	//Si se ha encontrado letra atajo
	if (letra_atajo!=0) {
		buf_speech[dest++]='.';
		buf_speech[dest++]=' ';
		//Parece que en los sistemas de speech la letra mayuscula se lee con mas pausa (al menos testado con festival)
		buf_speech[dest++]=letra_mayuscula(letra_atajo);
		buf_speech[dest++]='.';
	}

	buf_speech[dest]=0;


	//Si hay texto <dir> cambiar por directory
	char *p;
	p=strstr(buf_speech,"<dir>");
	if (p) {
		//suponemos que esto va a final de texto
		sprintf (p,"%s","directory");
	}



	//Si es todo espacios sin ningun caracter, no enviar
	int vacio=1;
	for (orig=0;buf_speech[orig]!=0;orig++) {
		if (buf_speech[orig]!=' ') {
			vacio=0;
			break;
		}
	}

	if (vacio==1) {
		debug_printf (VERBOSE_DEBUG,"Empty line, do not send to speech");
		return;
	}



	textspeech_print_speech(buf_speech);
	//printf ("textspeech_print_speech: %s\n",buf_speech);

	//hacemos que el timeout de tooltip se reinicie porque sino cuando se haya leido el menu, acabara saltando el timeout
	//menu_tooltip_counter=0;

	z80_byte acumulado;


	/*
	int contador_refresco=0;
	int max_contador_refresco;

	//Algo mas de un frame de pantalla
	if (menu_multitarea==1) max_contador_refresco=100000;

	//Cada 20 ms
	//Nota: Ver funcion  menu_cpu_core_loop(void) , donde hay usleep(500) en casos de menu_multitarea 0
	else max_contador_refresco=40;
	*/

		//Parece que esto en maquinas lentas (especialmente en mi Windows virtual). Bueno creo que realmente no es un problema de Windows,
		//si no de que la maquina es muy lenta y tarda en refrescar la pantalla mientras esta
		//esperando una tecla y reproduciendo speech. Si quito esto, sucede que
		//si se pulsa el cursor rapido y hay speech, dicho cursor va "con retraso" una posicion
	all_interlace_scr_refresca_pantalla();

        do {
                if (textspeech_finalizado_hijo_speech() ) scrtextspeech_filter_run_pending();

                menu_cpu_core_loop();
                acumulado=menu_da_todas_teclas();


                        //Hay tecla pulsada
                        if ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) !=MENU_PUERTO_TECLADO_NINGUNA ) {
				//printf ("pulsada tecla\n");
                                //int tecla=menu_get_pressed_key();

				//de momento cualquier tecla anula speech
				textspeech_empty_speech_fifo();

				menu_speech_tecla_pulsada=1;

                        }

			//no hay tecla pulsada
			else {
				//Decir que no repeticion de tecla. Si no pusiesemos esto aqui,
				//pasaria que si entramos con repeticion activa, y
				//mientras esperamos a que acabe proceso hijo, no pulsamos una tecla,
				//la repeticion seguiria activa
				menu_reset_counters_tecla_repeticion();
			}

		//Parece que esto en maquinas lentas (especialmente en mi Windows virtual). Bueno creo que realmente no es un problema de Windows,
		//si no de que la maquina es muy lenta y tarda en refrescar la pantalla mientras esta
		//esperando una tecla y reproduciendo speech. Si quito esto, sucede que
		//si se pulsa el cursor rapido y hay speech, dicho cursor va "con retraso" una posicion
		/*contador_refresco++;
		if (contador_refresco==max_contador_refresco) {
			printf ("refrescar\n");
			contador_refresco=0;
			all_interlace_scr_refresca_pantalla();
		}
		*/


        } while (!textspeech_finalizado_hijo_speech() && menu_speech_tecla_pulsada==0);
	//hacemos que el timeout de tooltip se reinicie porque sino cuando se haya leido el menu, acabara saltando el timeout
	menu_tooltip_counter=0;

}


//escribe opcion de linea de texto
//coordenadas y relativa al interior de la ventana (0=inicio)
//opcion_actual indica que numero de linea es la seleccionada
//opcion activada indica a 1 que esa opcion es seleccionable
void menu_escribe_linea_opcion(z80_byte y,int opcion_actual,int opcion_activada,char *texto)
{

        if (!strcmp(scr_driver_name,"stdout")) {
		printf ("%s\n",texto);
		scrstdout_menu_print_speech_macro (texto);
		return;
	}


	z80_byte papel,tinta;
	int i;

	//tinta=0;

	/*
	4 combinaciones:
	opcion seleccionada, disponible (activada)
	opcion seleccionada, no disponible
	opcion no seleccionada, disponible
	opcion no seleccionada, no disponible
	*/

	if (opcion_actual==y) {
		if (opcion_activada==1) {
			papel=ESTILO_GUI_PAPEL_SELECCIONADO;
			tinta=ESTILO_GUI_TINTA_SELECCIONADO;
		}
		else {
			papel=ESTILO_GUI_PAPEL_SEL_NO_DISPONIBLE;
			tinta=ESTILO_GUI_TINTA_SEL_NO_DISPONIBLE;
		}
	}

	else {
		if (opcion_activada==1) {
                        papel=ESTILO_GUI_PAPEL_NORMAL;
                        tinta=ESTILO_GUI_TINTA_NORMAL;
                }
                else {
                        papel=ESTILO_GUI_PAPEL_NO_DISPONIBLE;
                        tinta=ESTILO_GUI_TINTA_NO_DISPONIBLE;
                }
        }



	//linea entera con espacios
	for (i=0;i<ventana_ancho;i++) menu_escribe_texto_ventana(i,y,0,papel," ");

	//y texto propiamente
        menu_escribe_texto_ventana(1,y,tinta,papel,texto);

	//si el driver de video no tiene colores o si el estilo de gui lo indica, indicamos opcion activa con un cursor
	if (!scr_tiene_colores || ESTILO_GUI_MUESTRA_CURSOR) {
		if (opcion_actual==y) {
			if (opcion_activada==1) menu_escribe_texto_ventana(0,y,tinta,papel,">");
			else menu_escribe_texto_ventana(0,y,tinta,papel,"x");
		}
	}
	menu_textspeech_send_text(texto);

}

void menu_retorna_margenes_border(int *miz, int *mar)
{
	//margenes de zona interior de pantalla. para modo rainbow
	int margenx_izq=screen_total_borde_izquierdo*border_enabled.v;
	int margeny_arr=screen_borde_superior*border_enabled.v;

if (MACHINE_IS_Z88) {
//margenes para realvideo
margenx_izq=margeny_arr=0;
}


	else if (MACHINE_IS_CPC) {
//margenes para realvideo
margenx_izq=CPC_LEFT_BORDER_NO_ZOOM*border_enabled.v;
					margeny_arr=CPC_TOP_BORDER_NO_ZOOM*border_enabled.v;
	}

	else if (MACHINE_IS_PRISM) {
//margenes para realvideo
margenx_izq=PRISM_LEFT_BORDER_NO_ZOOM*border_enabled.v;
					margeny_arr=PRISM_TOP_BORDER_NO_ZOOM*border_enabled.v;
	}

	else if (MACHINE_IS_SAM) {
					//margenes para realvideo
					margenx_izq=SAM_LEFT_BORDER_NO_ZOOM*border_enabled.v;
					margeny_arr=SAM_TOP_BORDER_NO_ZOOM*border_enabled.v;
	}

	else if (MACHINE_IS_QL) {
					//margenes para realvideo
					margenx_izq=QL_LEFT_BORDER_NO_ZOOM*border_enabled.v;
					margeny_arr=QL_TOP_BORDER_NO_ZOOM*border_enabled.v;
	}

	*miz=margenx_izq;
	*mar=margeny_arr;

}


//dibuja cuadrado (4 lineas) usado en los menus para xwindows y fbdev
//Entrada: x1,y1 punto superior izquierda,x2,y2 punto inferior derecha en resolucion de zx spectrum. Color
//nota: realmente no es un cuadrado porque el titulo ya hace de franja superior
void menu_dibuja_cuadrado(z80_byte x1,z80_byte y1,z80_byte x2,z80_byte y2,z80_byte color)
{

	if (!ESTILO_GUI_MUESTRA_RECUADRO) return;


	int x,y;


	int margenx_izq;
	int margeny_arr;
	menu_retorna_margenes_border(&margenx_izq,&margeny_arr);

/*
        //margenes de zona interior de pantalla. para modo rainbow
        int margenx_izq=screen_total_borde_izquierdo*border_enabled.v;
        int margeny_arr=screen_borde_superior*border_enabled.v;

	if (MACHINE_IS_Z88) {
		//margenes para realvideo
		margenx_izq=margeny_arr=0;
	}


        else if (MACHINE_IS_CPC) {
		//margenes para realvideo
		margenx_izq=CPC_LEFT_BORDER_NO_ZOOM*border_enabled.v;
                margeny_arr=CPC_TOP_BORDER_NO_ZOOM*border_enabled.v;
        }

        else if (MACHINE_IS_PRISM) {
		//margenes para realvideo
		margenx_izq=PRISM_LEFT_BORDER_NO_ZOOM*border_enabled.v;
                margeny_arr=PRISM_TOP_BORDER_NO_ZOOM*border_enabled.v;
        }

        else if (MACHINE_IS_SAM) {
                //margenes para realvideo
                margenx_izq=SAM_LEFT_BORDER_NO_ZOOM*border_enabled.v;
                margeny_arr=SAM_TOP_BORDER_NO_ZOOM*border_enabled.v;
        }

				else if (MACHINE_IS_QL) {
                //margenes para realvideo
                margenx_izq=QL_LEFT_BORDER_NO_ZOOM*border_enabled.v;
                margeny_arr=QL_TOP_BORDER_NO_ZOOM*border_enabled.v;
        }
*/

	//solo hacerlo en el caso de drivers completos
	if (si_complete_video_driver() ) {

		if (rainbow_enabled.v==0) {

			//parte inferior
			for (x=x1;x<=x2;x++) scr_putpixel_zoom(x,y2,color);

			//izquierda
			for (y=y1;y<=y2;y++) scr_putpixel_zoom(x1,y,color);

			//derecha
			for (y=y1;y<=y2;y++) scr_putpixel_zoom(x2,y,color);


		}

		else {
 	               //parte inferior
        	        for (x=x1;x<=x2;x++) scr_putpixel_zoom_rainbow(x+margenx_izq,y2+margeny_arr,color);

	                //izquierda
        	        for (y=y1;y<=y2;y++) scr_putpixel_zoom_rainbow(x1+margenx_izq,y+margeny_arr,color);

	                //derecha
        	        for (y=y1;y<=y2;y++) scr_putpixel_zoom_rainbow(x2+margenx_izq,y+margeny_arr,color);

		}
	}


}

void menu_muestra_pending_error_message(void)
{
	if (if_pending_error_message) {
					if_pending_error_message=0;
debug_printf (VERBOSE_INFO,"Showing pending error message on menu");
					menu_generic_message("ERROR",pending_error_message);
}
}

//dibuja ventana de menu, con:
//titulo
//contenido blanco
//recuadro de lineas
//Entrada: x,y posicion inicial. ancho, alto. Todo coordenadas en caracteres 0..31 y 0..23
void menu_dibuja_ventana(z80_byte x,z80_byte y,z80_byte ancho,z80_byte alto,char *titulo)
{


	menu_muestra_pending_error_message();

	//En el caso de stdout, solo escribimos el texto
        if (!strcmp(scr_driver_name,"stdout")) {
                printf ("%s\n",titulo);
		scrstdout_menu_print_speech_macro(titulo);
		printf ("------------------------\n\n");
		//paso de curses a stdout deja stdout que no hace flush nunca. forzar
		fflush(stdout);
                return;
        }

	//printf ("valor menu_speech_tecla_pulsada: %d\n",menu_speech_tecla_pulsada);

	//valores en pixeles
	int xpixel,ypixel,anchopixel,altopixel;
	z80_byte i,j;

	//guardamos valores globales de la ventana mostrada
	ventana_x=x;
	ventana_y=y;
	ventana_ancho=ancho;
	ventana_alto=alto;

	xpixel=x*8;
	ypixel=y*8;
	anchopixel=ancho*8;
	altopixel=alto*8;

	//contenido en blanco normalmente en estilo ZEsarUX
	for (i=0;i<alto-1;i++) {
		for (j=0;j<ancho;j++) {
			//putchar_menu_overlay(x+j,y+i+1,' ',0,7+8);
			putchar_menu_overlay(x+j,y+i+1,' ',ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL);
		}
	}

	//recuadro
	//menu_establece_cuadrado(xpixel,ypixel,xpixel+anchopixel-1,ypixel+altopixel-1,0);
	menu_establece_cuadrado(xpixel,ypixel,xpixel+anchopixel-1,ypixel+altopixel-1,ESTILO_GUI_PAPEL_TITULO);


        //titulo
        //primero franja toda negra normalmente en estilo ZEsarUX
        for (i=0;i<ancho;i++) putchar_menu_overlay(x+i,y,' ',ESTILO_GUI_TINTA_TITULO,ESTILO_GUI_PAPEL_TITULO);

        //y luego el texto
        for (i=0;i<strlen(titulo);i++) putchar_menu_overlay(x+i,y,titulo[i],ESTILO_GUI_TINTA_TITULO,ESTILO_GUI_PAPEL_TITULO);

        //y las franjas de color
	if (ESTILO_GUI_MUESTRA_RAINBOW) {
		//en el caso de drivers completos, hacerlo real
		if (si_complete_video_driver() ) {
	                putchar_menu_overlay(x+ancho-6,y,128,2+8,ESTILO_GUI_PAPEL_TITULO);
        	        putchar_menu_overlay(x+ancho-5,y,128,6+8,2+8);
                	putchar_menu_overlay(x+ancho-4,y,128,4+8,6+8);
	                putchar_menu_overlay(x+ancho-3,y,128,5+8,4+8);
        	        putchar_menu_overlay(x+ancho-2,y,128,ESTILO_GUI_PAPEL_TITULO,5+8);
	        }

		//en caso de curses o caca, hacerlo con lineas de colores
	        if (!strcmp(scr_driver_name,"curses") || !strcmp(scr_driver_name,"caca") ) {
        	        putchar_menu_overlay(x+ancho-6,y,'/',2+8,ESTILO_GUI_PAPEL_TITULO);
                	putchar_menu_overlay(x+ancho-5,y,'/',6+8,ESTILO_GUI_PAPEL_TITULO);
	                putchar_menu_overlay(x+ancho-4,y,'/',4+8,ESTILO_GUI_PAPEL_TITULO);
        	        putchar_menu_overlay(x+ancho-3,y,'/',5+8,ESTILO_GUI_PAPEL_TITULO);
	        }
	}


        char buffer_titulo[100];
        sprintf (buffer_titulo,"Window: %s",titulo);
        menu_textspeech_send_text(buffer_titulo);

        //Forzar que siempre suene
        //menu_speech_tecla_pulsada=0;



}

menu_item *menu_retorna_item(menu_item *m,int i)
{

        while (i>0)
        {
                m=m->next;
		i--;
        }

	return m;


}

void menu_cpu_core_loop(void)
{
                if (menu_multitarea==1) cpu_core_loop();
                else {
                        scr_actualiza_tablas_teclado();
			realjoystick_main();

                        //0.5 ms
                        usleep(500);


			//printf ("en menu_cpu_core_loop\n");
                }

}

int last_mouse_x,last_mouse_y;
int mouse_movido=0;

int menu_mouse_x=0;
int menu_mouse_y=0;

int si_menu_mouse_en_ventana(void)
{
	if (menu_mouse_x>=0 && menu_mouse_y>=0 && menu_mouse_x<ventana_ancho && menu_mouse_y<ventana_alto ) return 1;
	return 0;
}

void menu_calculate_mouse_xy(void)
{
	int x,y;
	if (si_complete_video_driver() ) {

		int mouse_en_emulador=0;
		//printf ("x: %d y: %d\n",mouse_x,mouse_y);
		if (mouse_x>=0 && mouse_y>=0
			&& mouse_x<=screen_get_window_size_width_zoom_border_en() && mouse_y<=screen_get_window_size_height_zoom_border_en() ) {
				//Si mouse esta dentro de la ventana del emulador
				mouse_en_emulador=1;
		}

		if (  (mouse_x!=last_mouse_x || mouse_y !=last_mouse_y) && mouse_en_emulador) {

			mouse_movido=1;
		}
		else mouse_movido=0;

		last_mouse_x=mouse_x;
		last_mouse_y=mouse_y;


		//Quitarle el zoom
		x=mouse_x/zoom_x;
		y=mouse_y/zoom_y;

		//Considerar borde pantalla

		//Todo lo que sea negativo o exceda border, nada.

		//printf ("x: %d y: %d\n",x,y);



        //margenes de zona interior de pantalla. para modo rainbow
				int margenx_izq;
				int margeny_arr;
				menu_retorna_margenes_border(&margenx_izq,&margeny_arr);

	x -=margenx_izq;
	y -=margeny_arr;

	x /=8;
	y /=8;

	x -=ventana_x;
	y -=ventana_y;

	menu_mouse_x=x;
	menu_mouse_y=y;

	//Coordenadas menu_mouse_x y tienen como origen 0,0 en zona superior izquierda de ventana (titulo ventana)
	//Y en coordenadas de linea (y=0 primera linea, y=1 segunda linea, etc)

	}
}

z80_byte menu_da_todas_teclas(void)
{
	z80_byte acumulado;

	acumulado=255;

	//symbol i shift no cuentan por separado
	acumulado=acumulado & (puerto_65278 | 1) & puerto_65022 & puerto_64510 & puerto_63486 & puerto_61438 & puerto_57342 & puerto_49150 & (puerto_32766 |2) & puerto_especial1 & puerto_especial2 & puerto_especial3;

	//no ignorar disparo
	z80_byte valor_joystick=(puerto_especial_joystick&31)^255;
	acumulado=acumulado & valor_joystick;

	//contar tambien botones mouse
	if (si_menu_mouse_activado()) {
		menu_calculate_mouse_xy();
		z80_byte valor_botones_mouse=(mouse_left | mouse_right | mouse_movido)^255;
		acumulado=acumulado & valor_botones_mouse;
	}




        //Modo Z88
        if (MACHINE_IS_Z88) {
		//no contar mayusculas
                acumulado = acumulado & blink_kbd_a8 & blink_kbd_a9 & blink_kbd_a10 & blink_kbd_a11 & blink_kbd_a12 & blink_kbd_a13 & (blink_kbd_a14 | 64) & (blink_kbd_a15 | 128);

        }

        else {
                //Poner los 3 bits superiores no usados a 1
                acumulado |=(128+64+32);
        }


	if ( (acumulado&MENU_PUERTO_TECLADO_NINGUNA) !=MENU_PUERTO_TECLADO_NINGUNA) return acumulado;

	//pero si que cuentan juntas (TAB)
	if ( (puerto_65278 & 1)==0 && (puerto_32766 & 2)==0) {
		//printf ("TAB\n");
		acumulado=acumulado & puerto_65278 & puerto_32766;
	}

	if (MACHINE_IS_SAM) {
		//Contar los 3 bits superiores de los puertos de teclas extendidas del sam
		z80_byte acum_sam=puerto_teclado_sam_fef9 & puerto_teclado_sam_fdf9 & puerto_teclado_sam_fbf9 & puerto_teclado_sam_f7f9 & puerto_teclado_sam_eff9 & puerto_teclado_sam_dff9 & puerto_teclado_sam_bff9 & puerto_teclado_sam_7ff9;

		//Rotarlo a 3 bits inferiores
		acum_sam = acum_sam >> 5;
		//Dejar los 3 bits inferiores y meter los bits que faltan a 1
		acum_sam = acum_sam & 7;
		acum_sam = acum_sam | (8+16+32+64+128);


		acumulado=acumulado & acum_sam;

		//printf ("acumulado: %d acum_sam: %d\n",acumulado,acum_sam);

		/*
		//TAB en SAM
		if ( (puerto_teclado_sam_f7f9 & 64)==0) {
			acumulado=acumulado & (255-3);
		}

		//DEL en SAM
		if (  (puerto_teclado_sam_eff9 & 128)==0) {
			//da igual el valor de acumulado, simplemente con cambiar un bit nos vale
			acumulado=acumulado & (255-1);
		}
		*/

	}

	//o si en modo zx80/81, cuenta el '.'
	if (MACHINE_IS_ZX8081) {
		if ((puerto_32766&2)==0) acumulado=acumulado & puerto_32766;
	}

	if (MACHINE_IS_CPC) {
		int j;
		for (j=0;j<16;j++) {
			//No contar mayusculas
			if (j==2) acumulado = acumulado & (cpc_keyboard_table[j] | 32);
			else acumulado = acumulado & cpc_keyboard_table[j];
		}
	}

	return acumulado;

//z80_byte puerto_65278=255; //    db              255  ; V    C    X    Z    Sh    ;0
//z80_byte puerto_32766=255; //    db              255  ; B    N    M    Simb Space ;7


}


//Para forzar desde remote command a salir de la funcion, sin haber pulsado tecla realmente
//z80_bit menu_espera_tecla_no_cpu_loop_flag_salir={0};

//Esperar a pulsar una tecla sin ejecutar cpu
void menu_espera_tecla_no_cpu_loop(void)
{

	z80_byte acumulado;

        do {

                scr_actualiza_tablas_teclado();
		realjoystick_main();

                //0.5 ms
                usleep(500);

                acumulado=menu_da_todas_teclas();

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA
				//&&	menu_espera_tecla_no_cpu_loop_flag_salir.v==0
							);


	//menu_espera_tecla_no_cpu_loop_flag_salir.v=0;

}



void menu_espera_no_tecla_no_cpu_loop(void)
{

        //Esperar a liberar teclas
        z80_byte acumulado;

        do {

                scr_actualiza_tablas_teclado();
		realjoystick_main();

                //0.5 ms
                usleep(500);

                acumulado=menu_da_todas_teclas();

        //printf ("menu_espera_no_tecla_no_cpu_loop acumulado: %d\n",acumulado);

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) != MENU_PUERTO_TECLADO_NINGUNA);

}


void menu_espera_tecla_timeout_tooltip(void)
{

        //Esperar a pulsar una tecla o timeout de tooltip
        z80_byte acumulado;

        do {
                menu_cpu_core_loop();


                acumulado=menu_da_todas_teclas();

		//printf ("menu_espera_tecla_timeout_tooltip acumulado: %d\n",acumulado);

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA && menu_tooltip_counter<TOOLTIP_SECONDS);

}


void menu_espera_tecla(void)
{

        //Esperar a pulsar una tecla
        z80_byte acumulado;

	//Si al entrar aqui ya hay tecla pulsada, volver
        acumulado=menu_da_todas_teclas();
        if ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) !=MENU_PUERTO_TECLADO_NINGUNA) return;


	do {
		menu_cpu_core_loop();


		acumulado=menu_da_todas_teclas();

		//printf ("menu_espera_tecla acumulado: %d\n",acumulado);

	} while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA);

	//Al salir del bucle, reseteamos contadores de repeticion
	menu_reset_counters_tecla_repeticion();

}

void menu_espera_tecla_o_joystick(void)
{
        //Esperar a pulsar una tecla o joystick
        z80_byte acumulado;

        do {
                menu_cpu_core_loop();


                acumulado=menu_da_todas_teclas();

		//printf ("menu_espera_tecla_o_joystick acumulado: %d\n",acumulado);

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA && !realjoystick_hit() );

}


void menu_espera_no_tecla(void)
{

        //Esperar a liberar teclas
        z80_byte acumulado;

        do {
		menu_cpu_core_loop();

		acumulado=menu_da_todas_teclas();

	//printf ("menu_espera_no_tecla acumulado: %d\n",acumulado);

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) != MENU_PUERTO_TECLADO_NINGUNA);

}

//#define CONTADOR_HASTA_REPETICION (MACHINE_IS_Z88 ? 75 : 25)
//#define CONTADOR_ENTRE_REPETICION (MACHINE_IS_Z88 ? 3 : 1)

#define CONTADOR_HASTA_REPETICION (25)
#define CONTADOR_ENTRE_REPETICION (1)


//0.5 segundos para empezar repeticion (25 frames)
int menu_contador_teclas_repeticion;

int menu_segundo_contador_teclas_repeticion;


void menu_reset_counters_tecla_repeticion(void)
{
                        menu_contador_teclas_repeticion=CONTADOR_HASTA_REPETICION;
                        menu_segundo_contador_teclas_repeticion=CONTADOR_ENTRE_REPETICION;
}

void menu_espera_no_tecla_con_repeticion(void)
{

        //Esperar a liberar teclas, pero si se deja pulsada una tecla el tiempo suficiente, se retorna
        z80_byte acumulado;

	//x frames de segundo entre repeticion
	menu_segundo_contador_teclas_repeticion=CONTADOR_ENTRE_REPETICION;

        do {
                menu_cpu_core_loop();

                acumulado=menu_da_todas_teclas();

        	//printf ("menu_espera_no_tecla_con_repeticion acumulado: %d\n",acumulado);

		//si no hay tecla pulsada, restablecer contadores
		if ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) == MENU_PUERTO_TECLADO_NINGUNA) menu_reset_counters_tecla_repeticion();

		//printf ("contadores: 1 %d  2 %d\n",menu_contador_teclas_repeticion,menu_segundo_contador_teclas_repeticion);

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) != MENU_PUERTO_TECLADO_NINGUNA && menu_segundo_contador_teclas_repeticion!=0);


}



void menu_view_screen(MENU_ITEM_PARAMETERS)
{
        menu_espera_no_tecla();

	//para que no se vea oscuro
	menu_abierto=0;
	modificado_border.v=1;
	all_interlace_scr_refresca_pantalla();
        menu_espera_tecla();
	menu_abierto=1;
	menu_espera_no_tecla();
	modificado_border.v=1;
}


//Quita de la linea los caracteres de atajo ~~
void menu_dibuja_menu_stdout_texto_sin_atajo(char *origen, char *destino)
{

	int indice_orig,indice_dest;

	indice_orig=indice_dest=0;

	while (origen[indice_orig]) {
		if (menu_escribe_texto_si_inverso(origen,indice_orig)) {
			indice_orig +=2;
		}


		destino[indice_dest++]=origen[indice_orig++];
	}

	destino[indice_dest]=0;

}


#define MENU_RETORNO_NORMAL 0
#define MENU_RETORNO_ESC -1
#define MENU_RETORNO_F1 -2
#define MENU_RETORNO_F2 -3
#define MENU_RETORNO_F10 -4


int menu_dibuja_menu_stdout(int *opcion_inicial,menu_item *item_seleccionado,menu_item *m,char *titulo)
{
	int linea_seleccionada=*opcion_inicial;
	char texto_linea_sin_shortcut[64];

	menu_item *aux;

	//Titulo
	printf ("\n");
        printf ("%s\n",titulo);
	printf ("------------------------\n\n");
	scrstdout_menu_print_speech_macro(titulo);

	aux=m;

        int max_opciones=0;

	int tecla=13;

	//para speech stdout. asumir no tecla pulsada. si se pulsa tecla, para leer menu
	menu_speech_tecla_pulsada=0;

        do {

		//scrstdout_menu_kbhit_macro();
                max_opciones++;

		if (aux->tipo_opcion!=MENU_OPCION_SEPARADOR) {

			//Ver si esta activa
                        t_menu_funcion_activo menu_funcion_activo;

                        menu_funcion_activo=aux->menu_funcion_activo;

                        if ( menu_funcion_activo!=NULL) {
				if (menu_funcion_activo()!=0) {
					printf ("%2d)",max_opciones);
					char buf[10];
					sprintf (buf,"%d",max_opciones);
					if (!menu_speech_tecla_pulsada) {
						scrstdout_menu_print_speech_macro(buf);
					}
				}

				else {
					printf ("x  ");
					if (!menu_speech_tecla_pulsada) {
						scrstdout_menu_print_speech_macro("Disabled option: ");
					}
				}
			}

			else {
				printf ("%2d)",max_opciones);
					char buf[10];
					sprintf (buf,"%d",max_opciones);
					if (!menu_speech_tecla_pulsada) {
						scrstdout_menu_print_speech_macro(buf);
					}
			}

			//Imprimir linea menu pero descartando los ~~ de los atajo de teclado
			menu_dibuja_menu_stdout_texto_sin_atajo(aux->texto_opcion,texto_linea_sin_shortcut);


			printf ( "%s",texto_linea_sin_shortcut);
			if (!menu_speech_tecla_pulsada) {
				scrstdout_menu_print_speech_macro (texto_linea_sin_shortcut);
			}

		}


		printf ("\n");

                aux=aux->next;
        } while (aux!=NULL);

	printf ("\n");

	char texto_opcion[256];

	int salir_opcion;


	do {

		salir_opcion=1;
		printf("Option number? (prepend the option with h for help, t for tooltip)");
		//printf ("menu_speech_tecla_pulsada: %d\n",menu_speech_tecla_pulsada);
		if (!menu_speech_tecla_pulsada) {
			scrstdout_menu_print_speech_macro("Option number? (prepend the option with h for help, t for tooltip)");
		}

		//paso de curses a stdout deja stdout que no hace flush nunca. forzar
		fflush(stdout);
		scanf("%s",texto_opcion);

		if (!strcmp(texto_opcion,"ESC")) {
			tecla=MENU_RETORNO_ESC;
		}

		else if (texto_opcion[0]=='h' || texto_opcion[0]=='t') {
				salir_opcion=0;
                                char *texto_ayuda;
				linea_seleccionada=atoi(&texto_opcion[1]);
				linea_seleccionada--;
				if (linea_seleccionada>=0 && linea_seleccionada<max_opciones) {

					char *titulo_texto;

					if (texto_opcion[0]=='h') {
		                                texto_ayuda=menu_retorna_item(m,linea_seleccionada)->texto_ayuda;
						titulo_texto="Help";
					}

					else {
						texto_ayuda=menu_retorna_item(m,linea_seleccionada)->texto_tooltip;
						titulo_texto="Tooltip";
					}


        	                        if (texto_ayuda!=NULL) {
                	                        menu_generic_message(titulo_texto,texto_ayuda);
					}
					else {
						printf ("Item has no %s\n",titulo_texto);
					}
                                }
				else {
					printf ("Invalid option number\n");
				}
		}

		else {
			linea_seleccionada=atoi(texto_opcion);

			if (linea_seleccionada<1 || linea_seleccionada>max_opciones) {
				printf ("Incorrect option number\n");
				salir_opcion=0;
			}

			//Numero correcto
			else {
				linea_seleccionada--;

				//Ver si item no es separador
				menu_item *item_seleccionado;
				item_seleccionado=menu_retorna_item(m,linea_seleccionada);

				if (item_seleccionado->tipo_opcion==MENU_OPCION_SEPARADOR) {
					salir_opcion=0;
					printf ("Item is a separator\n");
				}

				else {


					//Ver si item esta activo
        	                	t_menu_funcion_activo menu_funcion_activo;
	        	                menu_funcion_activo=item_seleccionado->menu_funcion_activo;

	                	        if ( menu_funcion_activo!=NULL) {

        	                	        if (menu_funcion_activo()==0) {
							//opcion inactiva
							salir_opcion=0;
							printf ("Item is disabled\n");
						}
					}
				}
                        }



		}

	} while (salir_opcion==0);

        menu_item *menu_sel;
        menu_sel=menu_retorna_item(m,linea_seleccionada);

        item_seleccionado->menu_funcion=menu_sel->menu_funcion;
        item_seleccionado->tipo_opcion=menu_sel->tipo_opcion;
	item_seleccionado->valor_opcion=menu_sel->valor_opcion;


        //Liberar memoria del menu
        aux=m;
        menu_item *nextfree;

        do {
                //printf ("Liberando %x\n",aux);
                nextfree=aux->next;
                free(aux);
                aux=nextfree;
        } while (aux!=NULL);

	*opcion_inicial=linea_seleccionada;


	if (tecla==MENU_RETORNO_ESC) return MENU_RETORNO_ESC;

	return MENU_RETORNO_NORMAL;
}


//devuelve a que numero de opcion corresponde el atajo pulsado
//-1 si a ninguno
//NULL si a ninguno
int menu_retorna_atajo(menu_item *m,z80_byte tecla)
{

	//Si letra en mayusculas, bajar a minusculas
	if (tecla>='A' && tecla<='Z') tecla +=('a'-'A');

	int linea=0;

	while (m!=NULL) {
		if (m->atajo_tecla==tecla) {
			debug_printf (VERBOSE_DEBUG,"Shortcut found at entry number: %d",linea);
			return linea;
		}
		m=m->next;
		linea++;
	}

	//no encontrado atajo. escribir entradas de menu con atajos para informar al usuario
	menu_writing_inverse_color.v=1;
	return -1;

}

int menu_active_item_primera_vez=1;

void menu_escribe_opciones(menu_item *aux,int linea_seleccionada,int max_opciones)
{
                int i;
                int opcion_activa;

		char texto_opcion_activa[100];
		//Asumimos por si acaso que no hay ninguna activa
		texto_opcion_activa[0]=0;


                for (i=0;i<max_opciones;i++) {

                        //si la opcion seleccionada es un separador, el cursor saltara a la siguiente
                        //Nota: el separador no puede ser final de menu
                        //if (linea_seleccionada==i && aux->tipo_opcion==MENU_OPCION_SEPARADOR) linea_seleccionada++;

                        t_menu_funcion_activo menu_funcion_activo;

                        menu_funcion_activo=aux->menu_funcion_activo;

                        if (menu_funcion_activo!=NULL) {
                                opcion_activa=menu_funcion_activo();
                        }

                        else {
				opcion_activa=1;
			}

			//Cuando haya opcion_activa, nos la apuntamos para decirla al final en speech.
			//Y si es la primera vez en ese menu, dice "Active item". Sino, solo dice el nombre de la opcion
			if (linea_seleccionada==i) {
				if (menu_active_item_primera_vez) {
					sprintf (texto_opcion_activa,"Active item: %s",aux->texto_opcion);
					menu_active_item_primera_vez=0;
				}

				else {
					sprintf (texto_opcion_activa,"%s",aux->texto_opcion);
				}
			}

                        menu_escribe_linea_opcion(i,linea_seleccionada,opcion_activa,aux->texto_opcion);

                        aux=aux->next;

                }

		if (texto_opcion_activa[0]!=0) {
			//Active item siempre quiero que se escuche

			//Guardamos estado actual
			int antes_menu_speech_tecla_pulsada=menu_speech_tecla_pulsada;
			menu_speech_tecla_pulsada=0;

			menu_textspeech_send_text(texto_opcion_activa);

			//Restauro estado
			//Pero si se ha pulsado tecla, no restaurar estado
			//Esto sino provocaria que , por ejemplo, en la ventana de confirmar yes/no,
			//se entra con menu_speech_tecla_pulsada=0, se pulsa tecla mientras se esta leyendo el item activo,
			//y luego al salir de aqui, se pierde el valor que se habia metido (1) y se vuelve a poner el 0 del principio
			//provocando que cada vez que se mueve el cursor, se relea la ventana entera
			if (menu_speech_tecla_pulsada==0) menu_speech_tecla_pulsada=antes_menu_speech_tecla_pulsada;
		}
}

int menu_dibuja_menu_cursor_arriba(int linea_seleccionada,int max_opciones,menu_item *m)
{
	if (linea_seleccionada==0) linea_seleccionada=max_opciones-1;
	else {
		linea_seleccionada--;
		//ver si la linea seleccionada es un separador
		int salir=0;
		while (menu_retorna_item(m,linea_seleccionada)->tipo_opcion==MENU_OPCION_SEPARADOR && !salir) {
			linea_seleccionada--;
			//si primera linea es separador, nos iremos a -1
			if (linea_seleccionada==-1) {
				linea_seleccionada=max_opciones-1;
				salir=1;
			}
		}
	}
	//si linea resultante es separador, decrementar
	while (menu_retorna_item(m,linea_seleccionada)->tipo_opcion==MENU_OPCION_SEPARADOR) linea_seleccionada--;

	//Decir que se ha pulsado tecla
	menu_speech_tecla_pulsada=1;

	return linea_seleccionada;
}

int menu_dibuja_menu_cursor_abajo(int linea_seleccionada,int max_opciones,menu_item *m)
{
	if (linea_seleccionada==max_opciones-1) linea_seleccionada=0;
	else {
		linea_seleccionada++;
		//ver si la linea seleccionada es un separador
		int salir=0;
		while (menu_retorna_item(m,linea_seleccionada)->tipo_opcion==MENU_OPCION_SEPARADOR && !salir) {
			linea_seleccionada++;
			//si ultima linea es separador, nos salimos de rango
			if (linea_seleccionada==max_opciones) {
				linea_seleccionada=0;
				salir=0;
			}
		}
	}
	//si linea resultante es separador, incrementar
	while (menu_retorna_item(m,linea_seleccionada)->tipo_opcion==MENU_OPCION_SEPARADOR) linea_seleccionada++;

	//Decir que se ha pulsado tecla
	menu_speech_tecla_pulsada=1;

	return linea_seleccionada;
}


//Funcion de gestion de menu
//Entrada: opcion_inicial: puntero a opcion inicial seleccionada
//m: estructura de menu (estructura en forma de lista con punteros)
//titulo: titulo de la ventana del menu
//Nota: x,y, ancho, alto de la ventana se calculan segun el contenido de la misma

//Retorno
//valor retorno: tecla pulsada: 0 normal (ENTER), 1 ESCAPE,... MENU_RETORNO_XXXX

//opcion_inicial contiene la opcion seleccionada.
//asigna en item_seleccionado valores de: tipo_opcion, menu_funcion (debe ser una estructura ya asignada)




int menu_dibuja_menu(int *opcion_inicial,menu_item *item_seleccionado,menu_item *m,char *titulo)
{

	//no escribir letras de atajos de teclado al entrar en un menu
	menu_writing_inverse_color.v=0;

	//Si se fuerza siempre que aparezcan letras de atajos
	if (menu_force_writing_inverse_color.v) menu_writing_inverse_color.v=1;

	//Primera vez decir active item. Luego solo el nombre del item
	menu_active_item_primera_vez=1;

        if (!strcmp(scr_driver_name,"stdout") ) {
                //se abre menu con driver stdout. Llamar a menu alternativo
                return menu_dibuja_menu_stdout(opcion_inicial,item_seleccionado,m,titulo);
        }
/*
        if (if_pending_error_message) {
                if_pending_error_message=0;
                menu_generic_message("ERROR",pending_error_message);
        }
*/


	//esto lo haremos ligeramente despues menu_speech_tecla_pulsada=0;

	menu_reset_counters_tecla_repeticion();


	//nota: parece que scr_actualiza_tablas_teclado se debe llamar en el caso de xwindows para que refresque la pantalla->seguramente viene por un evento


	int max_opciones;
	int linea_seleccionada=*opcion_inicial;

	//si la anterior opcion era la final (ESC), establecemos el cursor a 0
	//if (linea_seleccionada<0) linea_seleccionada=0;

	int x,y,ancho,alto;

	menu_item *aux;

	aux=m;

	//contar el numero de opciones totales
	//calcular ancho maximo de la ventana
	int ancho_calculado=0;

	//como minimo, lo que ocupa el titulo: texto + franjas de colores + margen
	ancho=strlen(titulo)+7;

	max_opciones=0;
	do {
		ancho_calculado=strlen(aux->texto_opcion)+2;
		//en calculo de ancho, tener en cuenta los "~~" del shortcut que no cuentan
		unsigned int l;
		for (l=0;l<strlen(aux->texto_opcion);l++) {
			if (menu_escribe_texto_si_inverso(aux->texto_opcion,l)) ancho_calculado-=2;
		}

		if (ancho_calculado>ancho) ancho=ancho_calculado;
		//printf ("%s\n",aux->texto);
		aux=aux->next;
		max_opciones++;
	} while (aux!=NULL);

	//printf ("Opciones totales: %d\n",max_opciones);

	alto=max_opciones+2;

	x=16-ancho/2;
	y=12-alto/2;

	if (x<0 || y<0 || x+ancho>32 || y+alto>24) {
		char window_error_message[100];
		sprintf(window_error_message,"Window out of bounds: x: %d y: %d ancho: %d alto: %d",x,y,ancho,alto);
		cpu_panic(window_error_message);
	}

	int redibuja_ventana;
	int tecla;

	//Entrar aqui cada vez que se dibuje otra subventana aparte, como tooltip o ayuda
	do {
	redibuja_ventana=0;
	//printf ("redibujar ventana\n");
	menu_dibuja_ventana(x,y,ancho,alto,titulo);

	menu_tooltip_counter=0;


	tecla=0;

        //si la opcion seleccionada es mayor que el total de opciones, seleccionamos linea 0
        //esto pasa por ejemplo cuando activamos realvideo, dejamos el cursor por debajo, y cambiamos a zxspectrum
        //printf ("linea %d max %d\n",linea_seleccionada,max_opciones);
        if (linea_seleccionada>=max_opciones) {
                debug_printf(VERBOSE_INFO,"Selected Option beyond limits. Set option to 0");
                linea_seleccionada=0;
        }


	//menu_retorna_item(m,linea_seleccionada)->tipo_opcion==MENU_OPCION_SEPARADOR
	//si opcion activa es un separador (que esto pasa por ejemplo cuando activamos realvideo, dejamos el cursor por debajo, y cambiamos a zxspectrum)
	//en ese caso, seleccionamos linea 0
	if (menu_retorna_item(m,linea_seleccionada)->tipo_opcion==MENU_OPCION_SEPARADOR) {
		debug_printf(VERBOSE_INFO,"Selected Option is a separator. Set option to 0");
		linea_seleccionada=0;
	}


	while (tecla!=13 && tecla!=32 && tecla!=MENU_RETORNO_ESC && tecla!=MENU_RETORNO_F1 && tecla!=MENU_RETORNO_F2 && tecla!=MENU_RETORNO_F10 && redibuja_ventana==0 && menu_tooltip_counter<TOOLTIP_SECONDS) {
		//escribir todas opciones
		menu_escribe_opciones(m,linea_seleccionada,max_opciones);


		//printf ("Linea seleccionada: %d\n",linea_seleccionada);

        	all_interlace_scr_refresca_pantalla();

		tecla=0;

		//la inicializamos a 0. aunque parece que no haga falta, podria ser que el bucle siguiente
		//no se entrase (porque menu_tooltip_counter<TOOLTIP_SECONDS) y entonces tecla_leida tendria valor indefinido
		int tecla_leida=0;


		//Si se estaba escuchando speech y se pulsa una tecla, esa tecla debe entrar aqui tal cual y por tanto, no hacemos espera_no_tecla
		//temp menu_espera_no_tecla();
		if (menu_speech_tecla_pulsada==0) menu_espera_no_tecla();
		menu_speech_tecla_pulsada=0;

		while (tecla==0 && redibuja_ventana==0 && menu_tooltip_counter<TOOLTIP_SECONDS) {

			menu_espera_tecla_timeout_tooltip();

			//leemos tecla de momento de dos maneras, con puerto y con get_pressed_key
			tecla_leida=menu_get_pressed_key();

			//printf ("tecla_leida: %d\n",tecla_leida);
			if (mouse_movido) {
				//printf ("mouse x: %d y: %d menu mouse x: %d y: %d\n",mouse_x,mouse_y,menu_mouse_x,menu_mouse_y);
				//printf ("ventana x %d y %d ancho %d alto %d\n",ventana_x,ventana_y,ventana_ancho,ventana_alto);
				if (si_menu_mouse_en_ventana() ) {
				//if (menu_mouse_x>=0 && menu_mouse_y>=0 && menu_mouse_x<ventana_ancho && menu_mouse_y<ventana_alto ) {
					//printf ("dentro ventana\n");
					//Descartar linea titulo y ultima linea
					if (menu_mouse_y>0 && menu_mouse_y<ventana_alto-1 ) {
						//printf ("dentro espacio efectivo ventana\n");
						//Ver si hay que subir o bajar cursor
						int posicion_raton_y=menu_mouse_y-1;

						//Si no se selecciona separador
						if (menu_retorna_item(m,posicion_raton_y)->tipo_opcion!=MENU_OPCION_SEPARADOR) {
							linea_seleccionada=posicion_raton_y;
							redibuja_ventana=1;
							menu_tooltip_counter=0;


						}

					}
					//else {
					//	printf ("En espacio ventana no usable\n");
					//}
				}
				//else {
				//	printf ("fuera ventana\n");
				//}
			}


			if (tecla_leida==11) tecla='7';
			else if (tecla_leida==10) tecla='6';
			else if (tecla_leida==13) tecla=13;


			else if ((puerto_especial1 & 1)==0) {
				//Enter
				tecla=MENU_RETORNO_ESC;
			}

			//En principio ya no volvemos mas con F1, dado que este se usa para ayuda contextual de cada funcion

			//F1 (ayuda) o h en drivers que no soportan F
                        else if ((puerto_especial2 & 1)==0 || (tecla_leida=='h' && f_functions==0) ) {
                                //F1
				char *texto_ayuda;
				texto_ayuda=menu_retorna_item(m,linea_seleccionada)->texto_ayuda;
				if (texto_ayuda!=NULL) {
					//Forzar que siempre suene
					//Esperamos antes a liberar tecla, sino lo que hara sera que esa misma tecla F1 cancelara el speech texto de ayuda
					menu_espera_no_tecla();
					menu_speech_tecla_pulsada=0;
					menu_generic_message("Help",texto_ayuda);
					redibuja_ventana=1;
					menu_tooltip_counter=0;
					//Y volver a decir "active item"
					menu_active_item_primera_vez=1;

				}
                        }


                        else if ((puerto_especial2 & 2)==0) {
                                //F2
                                tecla=MENU_RETORNO_F2;
                        }

                        else if ((puerto_especial3 & 16)==0) {
                                //F10
                                tecla=MENU_RETORNO_F10;
                        }


			//teclas de atajos. De momento solo admitido entre a y z
			else if ( (tecla_leida>='a' && tecla_leida<='z') || (tecla_leida>='A' && tecla_leida<='Z')) {
				debug_printf (VERBOSE_DEBUG,"Read key: %c. Possibly shortcut",tecla_leida);
				tecla=tecla_leida;
			}

			//tecla espacio. acciones adicionales. Ejemplo en breakpoints para desactivar
			else if (tecla_leida==32) {
				debug_printf (VERBOSE_DEBUG,"Pressed key space");
				tecla=32;
                        }


			else tecla=0;


			//printf ("menu tecla: %d\n",tecla);
		}

		//Si no se ha pulsado tecla de atajo:
		if (!((tecla_leida>='a' && tecla_leida<='z') || (tecla_leida>='A' && tecla_leida<='Z')) ) {
			menu_espera_no_tecla();
		}

                t_menu_funcion_activo sel_activo;

		t_menu_funcion funcion_espacio;

		if (tecla!=0) menu_tooltip_counter=0;

		switch (tecla) {
			case 13:
				//ver si la opcion seleccionada esta activa

				sel_activo=menu_retorna_item(m,linea_seleccionada)->menu_funcion_activo;

				if (sel_activo!=NULL) {
		                	if ( sel_activo()==0 ) tecla=0;  //desactivamos seleccion
				}
                        break;

			case '6':
				linea_seleccionada=menu_dibuja_menu_cursor_abajo(linea_seleccionada,max_opciones,m);


			break;

			case '7':
				linea_seleccionada=menu_dibuja_menu_cursor_arriba(linea_seleccionada,max_opciones,m);

			break;

			case 32:
				//Accion para tecla espacio
				//printf ("Pulsado espacio\n");
                                //decimos que se ha pulsado Enter
                                //tecla=13;

				//Ver si tecla asociada a espacio
				funcion_espacio=menu_retorna_item(m,linea_seleccionada)->menu_funcion_espacio;

				if (funcion_espacio==NULL) {
					debug_printf (VERBOSE_DEBUG,"No space key function associated to this menu item");
					tecla=0;
				}

				else {

					debug_printf (VERBOSE_DEBUG,"Found space key function associated to this menu item");

	                                //ver si la opcion seleccionada esta activa

        	                        sel_activo=menu_retorna_item(m,linea_seleccionada)->menu_funcion_activo;

                	                if (sel_activo!=NULL) {
                        	                if ( sel_activo()==0 ) {
							tecla=0;  //desactivamos seleccion
							debug_printf (VERBOSE_DEBUG,"Menu item is disabled");
						}
                                	}

				}

			break;



		}

		//teclas de atajos. De momento solo admitido entre a y z
		if ( (tecla>='a' && tecla<='z') || (tecla>='A' && tecla<='Z')) {
			//printf ("buscamos atajo\n");

			int entrada_atajo;
			entrada_atajo=menu_retorna_atajo(m,tecla);


			//Encontrado atajo
			if (entrada_atajo!=-1) {
				linea_seleccionada=entrada_atajo;

				//Mostrar por un momento opciones y letras
				menu_writing_inverse_color.v=1;
				menu_escribe_opciones(m,entrada_atajo,max_opciones);
				all_interlace_scr_refresca_pantalla();
				menu_espera_no_tecla();

				//decimos que se ha pulsado Enter
				tecla=13;

	                        //Ver si esa opcion esta habilitada o no
        	                t_menu_funcion_activo sel_activo;
                	        sel_activo=menu_retorna_item(m,linea_seleccionada)->menu_funcion_activo;
                        	if (sel_activo!=NULL) {
	                                //opcion no habilitada
        	                        if ( sel_activo()==0 ) {
                	                        debug_printf (VERBOSE_DEBUG,"Shortcut found at entry number %d but entry disabled",linea_seleccionada);
						tecla=0;
                                	}
	                        }


			}

			else {
				debug_printf (VERBOSE_DEBUG,"No shortcut found for read key: %c",tecla);
				tecla=0;
				menu_espera_no_tecla();
			}
		}



	}

	//NOTA: contador de tooltip se incrementa desde bucle de timer, ejecutado desde cpu loop
	//Si no hay multitask de menu, NO se incrementa contador y por tanto no hay tooltip

	if (menu_tooltip_counter>=TOOLTIP_SECONDS) {

        	redibuja_ventana=1;

		//Por defecto asumimos que no saltara tooltip y por tanto que no queremos que vuelva a enviar a speech la ventana
		//Aunque si que volvera a decir el "active item: ..." en casos que se este en una opcion sin tooltip,
		//no aparecera el tooltip pero vendra aqui con el timeout y esto hara redibujar la ventana por redibuja_ventana=1
		//si quitase ese redibujado, lo que pasaria es que no aparecerian los atajos de teclado para cada opcion
		//Entonces tal y como esta ahora:
		//Si la opcion seleccionada tiene tooltip, salta el tooltip
		//Si no tiene tooltip, no salta tooltip, pero vuelve a decir "Active item: ..."
		menu_speech_tecla_pulsada=1;

		if (tooltip_enabled.v) {
			char *texto_tooltip;
			texto_tooltip=menu_retorna_item(m,linea_seleccionada)->texto_tooltip;
			if (texto_tooltip!=NULL) {
				//printf ("mostramos tooltip\n");
				//Forzar que siempre suene
				menu_speech_tecla_pulsada=0;
				menu_generic_message_tooltip("Tooltip",1,0,NULL,"%s",texto_tooltip);


				//Esperar no tecla
				menu_espera_no_tecla();

			        //Desde los cambios de speech en menu, hace falta esto
				//Sino, lo que se vea en el fondo del spectrum y se este modificando,
				//se sobreescribe en el menu
			        clear_putpixel_cache();

				//Y volver a decir "active item"
				menu_active_item_primera_vez=1;

	                }
		}


		//Hay que dibujar las letras correspondientes en texto inverso
		menu_writing_inverse_color.v=1;

		menu_tooltip_counter=0;
	}

	} while (redibuja_ventana==1);

	*opcion_inicial=linea_seleccionada;

	//nos apuntamos valor de retorno

	menu_item *menu_sel;
	menu_sel=menu_retorna_item(m,linea_seleccionada);

	//Si tecla espacio
	if (tecla==32) {
		item_seleccionado->menu_funcion=menu_sel->menu_funcion_espacio;
		tecla=13;
	}
	else item_seleccionado->menu_funcion=menu_sel->menu_funcion;

	item_seleccionado->tipo_opcion=menu_sel->tipo_opcion;
	item_seleccionado->valor_opcion=menu_sel->valor_opcion;


	//Liberar memoria del menu
        aux=m;
	menu_item *nextfree;

        do {
		//printf ("Liberando %x\n",aux);
		nextfree=aux->next;
		free(aux);
                aux=nextfree;
        } while (aux!=NULL);


	//Salir del menu diciendo que no se ha pulsado tecla
	menu_speech_tecla_pulsada=0;



	if (tecla==MENU_RETORNO_ESC) return MENU_RETORNO_ESC;
	else if (tecla==MENU_RETORNO_F1) return MENU_RETORNO_F1;
	else if (tecla==MENU_RETORNO_F2) return MENU_RETORNO_F2;
	else if (tecla==MENU_RETORNO_F10) return MENU_RETORNO_F10;

	else return MENU_RETORNO_NORMAL;

}


//Agregar el item inicial del menu
//Parametros: puntero al puntero de menu_item inicial. texto
void menu_add_item_menu_inicial(menu_item **p,char *texto,int tipo_opcion,t_menu_funcion menu_funcion,t_menu_funcion_activo menu_funcion_activo)
{

	menu_item *m;

	m=malloc(sizeof(menu_item));

	//printf ("%d\n",sizeof(menu_item));

        if (m==NULL) cpu_panic("Cannot allocate initial menu item");

	//comprobacion de maximo
	if (strlen(texto)>MAX_TEXTO_OPCION) cpu_panic ("Text item greater than maximum");

	//m->texto=texto;
	strcpy(m->texto_opcion,texto);



	m->tipo_opcion=tipo_opcion;
	m->menu_funcion=menu_funcion;
	m->menu_funcion_activo=menu_funcion_activo;
	m->texto_ayuda=NULL;
	m->texto_tooltip=NULL;
	m->atajo_tecla=0;
	m->menu_funcion_espacio=NULL;
	m->next=NULL;


	*p=m;
}

//Agregar un item al menu
//Parametros: puntero de menu_item inicial. texto
void menu_add_item_menu(menu_item *m,char *texto,int tipo_opcion,t_menu_funcion menu_funcion,t_menu_funcion_activo menu_funcion_activo)
{
	//busca el ultimo item i le añade el indicado

	while (m->next!=NULL)
	{
		m=m->next;
	}

	menu_item *next;

	next=malloc(sizeof(menu_item));
	//printf ("%d\n",sizeof(menu_item));

	if (next==NULL) cpu_panic("Cannot allocate menu item");

	m->next=next;

	//comprobacion de maximo
	if (strlen(texto)>MAX_TEXTO_OPCION) cpu_panic ("Text item greater than maximum");

	//next->texto=texto;
	strcpy(next->texto_opcion,texto);

	next->tipo_opcion=tipo_opcion;
	next->menu_funcion=menu_funcion;
	next->menu_funcion_activo=menu_funcion_activo;
	next->texto_ayuda=NULL;
	next->texto_tooltip=NULL;
	next->atajo_tecla=0;
	next->menu_funcion_espacio=NULL;
	next->next=NULL;
}

//Agregar ayuda al ultimo item de menu
void menu_add_item_menu_ayuda(menu_item *m,char *texto_ayuda)
{
       //busca el ultimo item i le añade el indicado

        while (m->next!=NULL)
        {
                m=m->next;
        }

	m->texto_ayuda=texto_ayuda;
}

//Agregar tooltip al ultimo item de menu
void menu_add_item_menu_tooltip(menu_item *m,char *texto_tooltip)
{
       //busca el ultimo item i le añade el indicado

        while (m->next!=NULL)
        {
                m=m->next;
        }

        m->texto_tooltip=texto_tooltip;
}

//Agregar atajo de tecla al ultimo item de menu
void menu_add_item_menu_shortcut(menu_item *m,z80_byte tecla)
{
       //busca el ultimo item i le añade el indicado

        while (m->next!=NULL)
        {
                m=m->next;
        }

        m->atajo_tecla=tecla;
}


//Agregar funcion de gestion de tecla espacio
void menu_add_item_menu_espacio(menu_item *m,t_menu_funcion menu_funcion_espacio)
{
//busca el ultimo item i le añade el indicado

        while (m->next!=NULL)
        {
                m=m->next;
        }

        m->menu_funcion_espacio=menu_funcion_espacio;
}




//Agregar un valor como opcion al ultimo item de menu
//Esto sirve, por ejemplo, para que cuando esta en el menu de z88, insertar slot,
//se pueda saber que slot se ha seleccionado
void menu_add_item_menu_valor_opcion(menu_item *m,int valor_opcion)
{
       //busca el ultimo item i le añade el indicado

        while (m->next!=NULL)
        {
                m=m->next;
        }

	//printf ("temp. agregar valor opcion %d\n",valor_opcion);

        m->valor_opcion=valor_opcion;
}



//Agregar un item al menu
//Parametros: puntero de menu_item inicial. texto con formato
void menu_add_item_menu_format(menu_item *m,int tipo_opcion,t_menu_funcion menu_funcion,t_menu_funcion_activo menu_funcion_activo,const char * format , ...)
{
	char buffer[100];
	va_list args;
	va_start (args, format);
	vsprintf (buffer,format, args);
	va_end (args);

	menu_add_item_menu(m,buffer,tipo_opcion,menu_funcion,menu_funcion_activo);
}


//Agregar el item inicial del menu
//Parametros: puntero al puntero de menu_item inicial. texto con formato
void menu_add_item_menu_inicial_format(menu_item **p,int tipo_opcion,t_menu_funcion menu_funcion,t_menu_funcion_activo menu_funcion_activo,const char * format , ...)
{
        char buffer[100];
        va_list args;
        va_start (args, format);
        vsprintf (buffer,format, args);
	va_end (args);

        menu_add_item_menu_inicial(p,buffer,tipo_opcion,menu_funcion,menu_funcion_activo);

}

//Agrega item de ESC normalmente.  En caso de aalib y consola es con tecla TAB
void menu_add_ESC_item(menu_item *array_menu_item)
{

        char mensaje_esc_back[32];

        sprintf (mensaje_esc_back,"%s Back",esc_key_message);

        menu_add_item_menu(array_menu_item,mensaje_esc_back,MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);

}


void menu_display_emulate_fast_zx8081(MENU_ITEM_PARAMETERS)
{
	video_fast_mode_emulation.v ^=1;
	modificado_border.v=1;
}

/*
void menu_display_real_zx8081_display(void)
{
	beta_zx8081_video.v^=1;
	modificado_border.v=1;
}
*/

void menu_display_emulate_zx8081display_spec(MENU_ITEM_PARAMETERS)
{
	if (simulate_screen_zx8081.v==1) simulate_screen_zx8081.v=0;
	else {
		simulate_screen_zx8081.v=1;
		umbral_simulate_screen_zx8081=4;
	}
	modificado_border.v=1;
}

int menu_display_emulate_zx8081_cond(void)
{
	return simulate_screen_zx8081.v;
}

int menu_display_arttext_cond(void)
{

	if (!menu_display_cursesstdout_cond()) return 0;

	//en zx80 y 81 no hay umbral, no tiene sentido. ahora si. hay rainbow de zx8081
	//if (machine_type>=20 && machine_type<=21) return 0;

        return texto_artistico.v;
}


void menu_audio_envelopes(MENU_ITEM_PARAMETERS)
{
	ay_envelopes_enabled.v^=1;
}

void menu_audio_speech(MENU_ITEM_PARAMETERS)
{
        ay_speech_enabled.v^=1;
}

void menu_audio_sound_zx8081(MENU_ITEM_PARAMETERS)
{
	zx8081_vsync_sound.v^=1;
}

void menu_audio_zx8081_detect_vsync_sound(MENU_ITEM_PARAMETERS)
{
	zx8081_detect_vsync_sound.v ^=1;
}

int menu_cond_zx81(void)
{
        if (MACHINE_IS_ZX81) return 1;
        return 0;
}

int menu_cond_zx81_realvideo(void)
{
        if (menu_cond_zx81()==0) return 0;
        return rainbow_enabled.v;

}

int menu_cond_realvideo(void)
{
	return rainbow_enabled.v;

}




int menu_cond_zx8081(void)
{
	if (MACHINE_IS_ZX8081) return 1;
	return 0;
}

int menu_cond_zx8081_realvideo(void)
{
	if (menu_cond_zx8081()==0) return 0;
	return rainbow_enabled.v;
}

int menu_cond_zx8081_wrx(void)
{
        if (menu_cond_zx8081()==0) return 0;
        return wrx_present.v;
}

int menu_cond_zx8081_wrx_no_stabilization(void)
{
	if (menu_cond_zx8081_wrx()==0) return 0;
	return !video_zx8081_estabilizador_imagen.v;
}

int menu_cond_zx8081_no_realvideo(void)
{
        if (menu_cond_zx8081()==0) return 0;
        return !rainbow_enabled.v;
}

int menu_cond_curses(void)
{
	if (!strcmp(scr_driver_name,"curses")) return 1;
	return 0;
}

int menu_cond_stdout(void)
{
        if (!strcmp(scr_driver_name,"stdout")) return 1;

        return 0;
}

int menu_cond_simpletext(void)
{
        if (!strcmp(scr_driver_name,"simpletext")) return 1;

        return 0;
}


/*
int menu_cond_no_stdout(void)
{
        //esto solo se permite en drivers xwindows, caca, aa, curses. NO en stdout
        if (!strcmp(scr_driver_name,"stdout")) return 0;
        return 1;
}
*/

int menu_cond_no_curses_no_stdout(void)
{
        //esto solo se permite en drivers xwindows, caca, aa. NO en curses ni stdout
        if (!strcmp(scr_driver_name,"curses")) return 0;
        if (!strcmp(scr_driver_name,"stdout")) return 0;
	return 1;
}




int menu_cond_zx8081_no_curses_no_stdout(void)
{
	if (!menu_cond_zx8081()) return 0;
	return menu_cond_no_curses_no_stdout();

}

int menu_cond_spectrum(void)
{
	return (MACHINE_IS_SPECTRUM);
	//return !menu_cond_zx8081();
}

int menu_cond_ay_chip(void)
{
	return ay_chip_present.v;
}

void menu_audio_ay_chip(MENU_ITEM_PARAMETERS)
{
	ay_chip_present.v^=1;
}

void menu_audio_ay_chip_autoenable(MENU_ITEM_PARAMETERS)
{
	autoenable_ay_chip.v^=1;
}



//llena el string con el valor del volumen - para chip de sonido
void menu_string_volumen(char *texto,z80_byte registro_volumen)
{
	if ( (registro_volumen & 16)!=0) sprintf (texto,"ENV");
	else {
		registro_volumen=registro_volumen & 15;
		int i;

		for (i=0;i<registro_volumen;i++) {
			texto[i]='=';
		}

                for (;i<15;i++) {
                        texto[i]=' ';
                }

		texto[i]=0;
	}
}

int retorna_frecuencia(int registro,int chip)
{
        int freq_temp;
	int freq_tono;
        freq_temp=ay_3_8912_registros[chip][registro*2]+256*(ay_3_8912_registros[chip][registro*2+1] & 0x0F);
        //printf ("Valor freq_temp : %d\n",freq_temp);
        freq_temp=freq_temp*16;


        //controlamos divisiones por cero
        if (!freq_temp) freq_temp++;

        freq_tono=FRECUENCIA_AY/freq_temp;

	return freq_tono;
}

int menu_breakpoints_cond(void)
{
	return debug_breakpoints_enabled.v;
}


//breakpoint de pc no tiene sentido porque se puede hacer con condicion pc=
/*
void menu_breakpoints_set(void)
{
	//printf ("linea: %d\n",breakpoints_opcion_seleccionada);

	int breakpoint_index=breakpoints_opcion_seleccionada-1;

        int dir;

        char string_dir[6];

	dir=debug_breakpoints_array[breakpoint_index];
	if (dir==-1) sprintf (string_dir,"0");
	else  sprintf (string_dir,"%d",dir);

        menu_ventana_scanf("Address",string_dir,6);

	if (string_dir[0]==0) dir=-1;

	else  {

		dir=parse_string_to_number(string_dir);

        	if (dir<0 || dir>65535) {
                	debug_printf (VERBOSE_ERR,"Invalid address %d",dir);
	                return;
        	}
	}


	debug_breakpoints_array[breakpoint_index]=dir;


}

*/
void menu_breakpoints_conditions_set(MENU_ITEM_PARAMETERS)
{
        //printf ("linea: %d\n",breakpoints_opcion_seleccionada);

	//saltamos los breakpoints de registro pc y la primera linea
        //int breakpoint_index=breakpoints_opcion_seleccionada-MAX_BREAKPOINTS-1;

	//saltamos las primeras 2 lineas
	int breakpoint_index=breakpoints_opcion_seleccionada-2;

        char string_texto[MAX_BREAKPOINT_CONDITION_LENGTH];

        sprintf (string_texto,"%s",debug_breakpoints_conditions_array[breakpoint_index]);

        menu_ventana_scanf("Condition",string_texto,MAX_BREAKPOINT_CONDITION_LENGTH);

  debug_set_breakpoint(breakpoint_index,string_texto);

}

void menu_breakpoints_condition_evaluate(MENU_ITEM_PARAMETERS)
{

        char string_texto[MAX_BREAKPOINT_CONDITION_LENGTH];
	string_texto[0]=0;

        menu_ventana_scanf("Condition",string_texto,MAX_BREAKPOINT_CONDITION_LENGTH);

        int result=debug_breakpoint_condition_loop(string_texto,1);

        menu_generic_message_format("Result","%s -> %s",string_texto,(result ? "True" : "False " ));
}

void menu_breakpoints_condition_behaviour(MENU_ITEM_PARAMETERS)
{

	debug_breakpoints_cond_behaviour.v ^=1;

}


/*
Esto ya no tiene sentido. Se puede hacer con variable mra
void menu_breakpoints_peek_set(MENU_ITEM_PARAMETERS)
{
        //printf ("linea: %d\n",breakpoints_opcion_seleccionada);

	//saltamos los breakpoints de registro pc y de condicion y la primera linea
        //int breakpoint_index=breakpoints_opcion_seleccionada-MAX_BREAKPOINTS-MAX_BREAKPOINTS_CONDITIONS-1;

	//saltamos los breakpoints de condicion y las primeras 2 lineas
        int breakpoint_index=breakpoints_opcion_seleccionada-MAX_BREAKPOINTS_CONDITIONS-2;


	//printf ("%d\n",breakpoint_index);

        int dir;

        char string_dir[6];

        dir=debug_breakpoints_peek_array[breakpoint_index];
        if (dir==-1) sprintf (string_dir,"0");
        else  sprintf (string_dir,"%d",dir);

        menu_ventana_scanf("Address",string_dir,6);

	if (string_dir[0]==0) dir=-1;

	else {
	        dir=parse_string_to_number(string_dir);

        	if (dir<0 || dir>65535) {
	                debug_printf (VERBOSE_ERR,"Invalid address %d",dir);
        	        return;
	        }
	}


        debug_breakpoints_peek_array[breakpoint_index]=dir;


}
*/

void menu_breakpoints_enable_disable(MENU_ITEM_PARAMETERS)
{
        if (debug_breakpoints_enabled.v==0) {
                debug_breakpoints_enabled.v=1;

		breakpoints_enable();
        }


        else {
                debug_breakpoints_enabled.v=0;

		breakpoints_disable();
        }

}


void menu_breakpoints_condition_enable_disable(MENU_ITEM_PARAMETERS)
{
	//printf ("Ejecutada funcion para espacio, condicion: %d\n",valor_opcion);

	debug_breakpoints_conditions_enabled[valor_opcion] ^=1;

	//si queda activo, decir que no ha saltado aun ese breakpoint
	if (debug_breakpoints_conditions_enabled[valor_opcion]) {
		debug_breakpoints_conditions_saltado[valor_opcion]=0;
	}
}


void menu_breakpoints(MENU_ITEM_PARAMETERS)
{

	menu_espera_no_tecla();

        menu_item *array_menu_breakpoints;
        menu_item item_seleccionado;
        int retorno_menu;
        do {


		menu_add_item_menu_inicial_format(&array_menu_breakpoints,MENU_OPCION_NORMAL,menu_breakpoints_enable_disable,NULL,"~~Breakpoints: %s",
			(debug_breakpoints_enabled.v ? "On" : "Off") );
		menu_add_item_menu_shortcut(array_menu_breakpoints,'b');
		menu_add_item_menu_tooltip(array_menu_breakpoints,"Enable Breakpoints");
		menu_add_item_menu_ayuda(array_menu_breakpoints,"Enable Breakpoints");

		//char buffer_texto[40];

                int i;


		menu_add_item_menu_format(array_menu_breakpoints,MENU_OPCION_NORMAL,menu_breakpoints_condition_evaluate,NULL,"~~Evaluate Condition");
		menu_add_item_menu_shortcut(array_menu_breakpoints,'e');
		menu_add_item_menu_tooltip(array_menu_breakpoints,"Test if a condition is true");
		menu_add_item_menu_ayuda(array_menu_breakpoints,"It tests a condition using the same method as breakpoint conditions below");

                for (i=0;i<MAX_BREAKPOINTS_CONDITIONS;i++) {
			char string_condition_shown[15];
			if (debug_breakpoints_conditions_array[i][0]) {
				menu_tape_settings_trunc_name(debug_breakpoints_conditions_array[i],string_condition_shown,15);
			}
			else {
				sprintf(string_condition_shown,"None");
			}

			if (debug_breakpoints_conditions_enabled[i]==0 || debug_breakpoints_enabled.v==0) {
				menu_add_item_menu_format(array_menu_breakpoints,MENU_OPCION_NORMAL,menu_breakpoints_conditions_set,menu_breakpoints_cond,"Disabled %d: %s",i+1,string_condition_shown);
			}
                        else {
				menu_add_item_menu_format(array_menu_breakpoints,MENU_OPCION_NORMAL,menu_breakpoints_conditions_set,menu_breakpoints_cond,"Enabled %d: %s",i+1,string_condition_shown);
			}


			menu_add_item_menu_tooltip(array_menu_breakpoints,"Set a condition breakpoint. Press Space to disable or enable");

			menu_add_item_menu_espacio(array_menu_breakpoints,menu_breakpoints_condition_enable_disable);

			menu_add_item_menu_valor_opcion(array_menu_breakpoints,i);

			menu_add_item_menu_ayuda(array_menu_breakpoints,"Set a condition breakpoint. Press Space to disable or enable\n"

						"A condition breakpoint has the following format: \n"
						"[REGISTER][CONDITION][VALUE] [OPERATOR] [REGISTER][CONDITION][VALUE] [OPERATOR] .... where: \n"
						"[REGISTER] can be a CPU register or some pseudo variables: A,B,C,D,E,F,H,L,I,R,BC,DE,HL,SP,PC,IX,IY,"
						"(BC),(DE),(HL),(SP),(PC),(IX),(IY), (NN), IFF1, IFF2, OPCODE,\n"
						"RAM: RAM mapped on 49152-65535 on Spectrum 128 or Prism,\n"
						"ROM: ROM mapped on 0-16383 on Spectrum 128,\n"
						"SEG0, SEG1, SEG2, SEG3: memory banks mapped on each 4 memory segments on Z88\n"
						"MRV: value returned on read memory operation\n"
						"MWV: value written on write memory operation\n"
						"MRA: address used on read memory operation\n"
						"MWA: address used on write memory operation\n"
						"PRV: value returned on read port operation\n"
						"PWV: value written on write port operation\n"
						"PRA: address used on read port operation\n"
						"PWA: address used on write port operation\n"

						"\n"

						"ENTERROM: returns 1 the first time PC register is on ROM space (0-16383)\n"
						"EXITROM: returns 1 the first time PC register is out ROM space (16384-65535)\n"
						"Note: The last two only return 1 the first time the breakpoint is fired, or a watch is shown, "
						"it will return 1 again only exiting required space address and entering again\n"

						"\n"

						"TSTATES: t-states total in a frame\n"
						"TSTATESL: t-states in a scanline\n"
						"TSTATESP: t-states partial\n"
						"SCANLINE: scanline counter\n"

						"\n"

						"[CONDITION] must be one of: <,>,=,/  (/ means not equal)\n"
						"[VALUE] must be a numeric value\n"
						"[OPERATOR] must be one of the following: and, or, xor\n"
						"Examples of conditions:\n"
						"SP<32768 : it will match when SP register is below 32768\n"
						"A=10 and BC<33 : it will match when A register is 10 and BC is below 33\n"
						"OPCODE=ED4AH : it will match when running opcode ADC HL,BC\n"
						"OPCODE=21H : it will match when running opcode LD HL,NN\n"
						"OPCODE=210040H : it will match when running opcode LD HL,4000H\n"
						"SEG2=40H : when memory bank 40H is mapped to memory segment 2 (49152-65535 range) on Z88\n"
						"MWA<16384 : it will match when attempting to write in ROM\n"
						"ENTERROM=1 : it will match when entering ROM space address\n"
						"TSTATESP>69888 : it will match when partial counter has executed a 48k full video frame (you should reset it before)\n"
						"\nNote: Any condition in the whole list can trigger a breakpoint");

                }

		/*

                for (i=0;i<MAX_BREAKPOINTS_PEEK;i++) {


                        if (debug_breakpoints_peek_array[i]==-1) sprintf (buffer_texto,"None");
                        else sprintf (buffer_texto,"%d",debug_breakpoints_peek_array[i]);

                        menu_add_item_menu_format(array_menu_breakpoints,MENU_OPCION_NORMAL,menu_breakpoints_peek_set,menu_breakpoints_cond,"Breakpoint Peek %d: %s",i+1,buffer_texto);

                        menu_add_item_menu_tooltip(array_menu_breakpoints,"Set a PEEK breakpoint");
                        menu_add_item_menu_ayuda(array_menu_breakpoints,"Set a PEEK (read address) breakpoint"
						"\nNote: Any condition in the whole list can trigger a breakpoint");


                }
		*/




                menu_add_item_menu(array_menu_breakpoints,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                menu_add_ESC_item(array_menu_breakpoints);
                retorno_menu=menu_dibuja_menu(&breakpoints_opcion_seleccionada,&item_seleccionado,array_menu_breakpoints,"Breakpoints" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}



//Retorna la pagina mapeada para el segmento
void menu_debug_registers_get_mem_page(z80_byte segmento,char *texto_pagina)
{

	//Si es dandanator
	if (segmento==0 && dandanator_switched_on.v==1) {
		sprintf (texto_pagina,"DB%d",dandanator_active_bank);
		return;
	}

	//Si es superupgrade
	if (superupgrade_enabled.v) {
	        if (debug_paginas_memoria_mapeadas[segmento] & 128) {
        	        //ROM
                	sprintf (texto_pagina,"RO%d",debug_paginas_memoria_mapeadas[segmento] & 127);
	        }

        	else {
                	//RAM
	                sprintf (texto_pagina,"RA%d",debug_paginas_memoria_mapeadas[segmento]);
        	}
		return;

	}

	//Con multiface
	if (segmento==0 && multiface_enabled.v && multiface_switched_on.v) {
		strcpy(texto_pagina,"MLTF");
		return;
	}

	if (debug_paginas_memoria_mapeadas[segmento] & 128) {
		//ROM
		sprintf (texto_pagina,"ROM%d",debug_paginas_memoria_mapeadas[segmento] & 127);
	}

	else {
		//RAM
		sprintf (texto_pagina,"RAM%d",debug_paginas_memoria_mapeadas[segmento]);
	}
}

//Retorna la pagina mapeada para el segmento en zxuno
void menu_debug_registers_get_mem_page_zxuno_bootm(z80_byte segmento,char *texto_pagina)
{
        if (zxuno_debug_paginas_memoria_mapeadas_bootm[segmento] & 128) {
                //ROM. Solo hay una rom de 16k en zxuno
                sprintf (texto_pagina,"ROM");
        }

        else {
                //RAM
                sprintf (texto_pagina,"RAM%02d",zxuno_debug_paginas_memoria_mapeadas_bootm[segmento]);
        }

}



//Si se muestra ram baja de Inves
z80_bit menu_debug_hex_shows_inves_low_ram={0};

//Vuelca contenido hexa de memoria de spectrum en cadena de texto, finalizando con 0 la cadena de texto
void menu_debug_registers_dump_hex(char *texto,menu_z80_moto_int direccion,int longitud)
{

	z80_byte byte_leido;

	int puntero=0;

	for (;longitud>0;longitud--) {
		direccion=adjust_address_space_cpu(direccion);
		//Si mostramos RAM oculta de Inves
		if (MACHINE_IS_INVES && menu_debug_hex_shows_inves_low_ram.v) {
			byte_leido=memoria_spectrum[direccion++];
		}
		else byte_leido=peek_byte_z80_moto(direccion++);

		sprintf (&texto[puntero],"%02X",byte_leido);

		puntero+=2;

	}
}

//Vuelca contenido ascii de memoria de spectrum en cadena de texto
//modoascii: 0: normal. 1:zx80. 2:zx81
void menu_debug_registers_dump_ascii(char *texto,menu_z80_moto_int direccion,int longitud,int modoascii)
{

        z80_byte byte_leido;

        int puntero=0;

        for (;longitud>0;longitud--) {
							direccion=adjust_address_space_cpu(direccion);
                //Si mostramos RAM oculta de Inves
                if (MACHINE_IS_INVES && menu_debug_hex_shows_inves_low_ram.v) {
                        byte_leido=memoria_spectrum[direccion++];
                }

                else byte_leido=peek_byte_z80_moto(direccion++);



		if (modoascii==0) {
		if (byte_leido<32 || byte_leido>127) byte_leido='.';
		}

		else if (modoascii==1) {
			if (byte_leido>=64) byte_leido='.';
			else byte_leido=da_codigo_zx80_no_artistic(byte_leido);
		}

		else {
			if (byte_leido>=64) byte_leido='.';
                        else byte_leido=da_codigo_zx81_no_artistic(byte_leido);
                }


                sprintf (&texto[puntero],"%c",byte_leido);

                puntero+=1;

        }
}


//Si muestra:
//
//0=linea assembler, registros cpu, otros registros internos
//1=9 lineas assembler, otros registros internos
//2=15 lineas assembler
//3=9 lineas hexdump, otros registros internos
//4=15 lineas hexdump
int menu_debug_registers_mostrando=0;


void menu_debug_registers_print_register_aux_moto(char *textoregistros,int *linea,int numero,m68k_register_t registro_direccion,m68k_register_t registro_dato)
{

	sprintf (textoregistros,"A%d: %08X D%d: %08X",numero,m68k_get_reg(NULL, registro_direccion),numero,m68k_get_reg(NULL, registro_dato) );
	menu_escribe_linea_opcion(*linea,-1,1,textoregistros);
	(*linea)++;

}

int menu_debug_registers_print_registers(void)
{
	int linea=0;
	char textoregistros[32];

	char dumpmemoria[32];

	char dumpassembler[32];

	size_t longitud_opcode;

	menu_z80_moto_int copia_reg_pc;
	int i;




		if (menu_debug_registers_mostrando==0) {

			debugger_disassemble(dumpassembler,32,&longitud_opcode,get_pc_register() );
			menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);


			if (CPU_IS_MOTOROLA) {
				sprintf (textoregistros,"PC: %05X SP: %05X USP: %05X",get_pc_register(),m68k_get_reg(NULL, M68K_REG_SP),m68k_get_reg(NULL, M68K_REG_USP));
				/*
				case M68K_REG_A7:       return cpu->dar[15];
				case M68K_REG_SP:       return cpu->dar[15];
 				case M68K_REG_USP:      return cpu->s_flag ? cpu->sp[0] : cpu->dar[15];

				SP siempre muestra A7
				USP muestra: en modo supervisor, SSP. En modo no supervisor, SP/A7
				*/
				menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

				unsigned int registro_sr=m68k_get_reg(NULL, M68K_REG_SR);

				sprintf (textoregistros,"SR: %04X : %c%c%c%c%c%c%c%c%c%c",registro_sr,
				(registro_sr&32768 ? 'T' : ' '),
				(registro_sr&8192  ? 'S' : ' '),
				(registro_sr&1024  ? '2' : ' '),
				(registro_sr&512   ? '1' : ' '),
				(registro_sr&256   ? '0' : ' '),
						(registro_sr&16 ? 'X' : ' '),
						(registro_sr&8  ? 'N' : ' '),
						(registro_sr&4  ? 'Z' : ' '),
						(registro_sr&2  ? 'V' : ' '),
						(registro_sr&1  ? 'C' : ' ')

			);
				menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

				menu_debug_registers_print_register_aux_moto(textoregistros,&linea,0,M68K_REG_A0,M68K_REG_D0);
				menu_debug_registers_print_register_aux_moto(textoregistros,&linea,1,M68K_REG_A1,M68K_REG_D1);
				menu_debug_registers_print_register_aux_moto(textoregistros,&linea,2,M68K_REG_A2,M68K_REG_D2);
				menu_debug_registers_print_register_aux_moto(textoregistros,&linea,3,M68K_REG_A3,M68K_REG_D3);
				menu_debug_registers_print_register_aux_moto(textoregistros,&linea,4,M68K_REG_A4,M68K_REG_D4);
				menu_debug_registers_print_register_aux_moto(textoregistros,&linea,5,M68K_REG_A5,M68K_REG_D5);
				menu_debug_registers_print_register_aux_moto(textoregistros,&linea,6,M68K_REG_A6,M68K_REG_D6);
				menu_debug_registers_print_register_aux_moto(textoregistros,&linea,7,M68K_REG_A7,M68K_REG_D7);


				//sprintf (textoregistros,"A0: %08X D0: %08X",m68k_get_reg(NULL, M68K_REG_A0),m68k_get_reg(NULL, M68K_REG_D0) );
				//menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

			}

			else {

			menu_debug_registers_dump_hex(dumpmemoria,get_pc_register(),8);

                        sprintf (textoregistros,"PC: %04X : %s",get_pc_register(),dumpmemoria);
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);


			menu_debug_registers_dump_hex(dumpmemoria,reg_sp,8);
                        sprintf (textoregistros,"SP: %04X : %s",reg_sp,dumpmemoria);
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        sprintf (textoregistros,"A: %02X F: %c%c%c%c%c%c%c%c",reg_a,DEBUG_STRING_FLAGS);
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        sprintf (textoregistros,"A':%02X F':%c%c%c%c%c%c%c%c",reg_a_shadow,DEBUG_STRING_FLAGS_SHADOW);
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        sprintf (textoregistros,"HL: %04X DE: %04X BC: %04X",HL,DE,BC);
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        sprintf (textoregistros,"HL':%04X DE':%04X BC':%04X",(reg_h_shadow<<8)|reg_l_shadow,(reg_d_shadow<<8)|reg_e_shadow,(reg_b_shadow<<8)|reg_c_shadow);
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

	                        sprintf (textoregistros,"IX: %04X IY: %04X",reg_ix,reg_iy);
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

			char texto_nmi[10];
			if (MACHINE_IS_ZX81) {
				sprintf (texto_nmi,"%s",(nmi_generator_active.v ? "NMI: On" : "NMI: Off"));
			}

			else {
				texto_nmi[0]=0;
			}

                        sprintf (textoregistros,"R: %02X I: %02X %s IM%d %s",
				(reg_r&127)|(reg_r_bit7&128),
				reg_i,
				( iff1.v ? "EI" : "DI"),
				im_mode,
				texto_nmi);

			menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

		}


		}

		if (menu_debug_registers_mostrando==1 || menu_debug_registers_mostrando==2) {


				int longitud_op;
				int limite=15;
				if (menu_debug_registers_mostrando==1) limite=9;
				copia_reg_pc=get_pc_register();
				for (i=0;i<limite;i++) {
					//debugger_disassemble(dumpassembler,32,&longitud_opcode,copia_reg_pc);
					menu_debug_dissassemble_una_instruccion(dumpassembler,copia_reg_pc,&longitud_op);
					menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);
					copia_reg_pc +=longitud_op;
				}


		}

		if (menu_debug_registers_mostrando==3 || menu_debug_registers_mostrando==4) {

			int limite=15;
			int longitud_linea=8;
			if (menu_debug_registers_mostrando==3) limite=9;
			copia_reg_pc=get_pc_register();
			for (i=0;i<limite;i++) {
					menu_debug_hexdump_with_ascii(dumpassembler,copia_reg_pc,longitud_linea);
					//menu_debug_registers_dump_hex(dumpassembler,copia_reg_pc,longitud_linea);
					menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);
					copia_reg_pc +=longitud_linea;
				}

		}

if (menu_debug_registers_mostrando==0 || menu_debug_registers_mostrando==1 || menu_debug_registers_mostrando==3) {
                        //Separador
                        sprintf (textoregistros," ");
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        sprintf (textoregistros,"MEMPTR: %04X TSTATES: %05d",memptr,t_estados);
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

			int estadosparcial=debug_t_estados_parcial;
			char buffer_estadosparcial[32];

			if (estadosparcial>999999999) sprintf (buffer_estadosparcial,"%s","OVERFLOW");
			else sprintf (buffer_estadosparcial,"%09u",estadosparcial);

                        sprintf (textoregistros,"TSTATL: %03d TSTATP: %s",(t_estados % screen_testados_linea),buffer_estadosparcial );
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        //ULA:

			//no hacer autodeteccion de idle bus port, para que no se active por si solo
			z80_bit copia_autodetect_rainbow;
			copia_autodetect_rainbow.v=autodetect_rainbow.v;

			autodetect_rainbow.v=0;



			//Puerto FE, Idle port y flash. cada uno para la maquina que lo soporte
			char feporttext[20];
			if (MACHINE_IS_SPECTRUM) {
				 sprintf (feporttext,"FE: %02X ",out_254_original_value);
			}
			else feporttext[0]=0;

                        char flashtext[40];
                        if (MACHINE_IS_SPECTRUM) {
                                sprintf (flashtext,"FLASH: %d ",estado_parpadeo.v);
                        }

                        else if (MACHINE_IS_Z88) {
                                sprintf (flashtext,"FLASH: %d ",estado_parpadeo.v);
                        }


                        else flashtext[0]=0;



			char idleporttext[20];
			if (MACHINE_IS_SPECTRUM) {
				sprintf (idleporttext,"IDLEPORT: %02X",idle_bus_port(255) );
			}
			else idleporttext[0]=0;



                        sprintf (textoregistros,"%s%s%s",feporttext,flashtext,idleporttext );




			autodetect_rainbow.v=copia_autodetect_rainbow.v;

                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

    //Paginas memoria
    if (MACHINE_IS_SPECTRUM_128_P2_P2A || MACHINE_IS_ZXUNO_BOOTM_DISABLED || MACHINE_IS_TBBLUE || superupgrade_enabled.v) {
                                int pagina;
                                //4 paginas, texto 5 caracteres max
                                char texto_paginas[4][5];

      for (pagina=0;pagina<4;pagina++) {

					//Caso tbblue y modo config en pagina 0
					if (MACHINE_IS_TBBLUE && pagina==0) {
						//z80_byte maquina=(tbblue_config1>>6)&3;
						z80_byte maquina=(tbblue_registers[3])&3;
						if (maquina==0){
							if (tbblue_bootrom.v) strcpy (texto_paginas[0],"ROM");
							else {
								//z80_byte romram_page=(tbblue_config1&31);
								z80_byte romram_page=(tbblue_registers[4]&31);
								sprintf (texto_paginas[0],"SR%d",romram_page);
							}
						}
						else menu_debug_registers_get_mem_page(pagina,texto_paginas[pagina]);
					}
          else {
						menu_debug_registers_get_mem_page(pagina,texto_paginas[pagina]);
					}
        }

				sprintf (textoregistros,"%s %s %s %s SCR%d %s",texto_paginas[0],texto_paginas[1],texto_paginas[2],texto_paginas[3],
                                        ( (puerto_32765&8) ? 7 : 5) ,  ( (puerto_32765&32) ? "PDI" : "PEN"  ) );

//D5

                                menu_escribe_linea_opcion(linea++,-1,1,textoregistros);
                        }

			//Si dandanator y maquina 48kb
			if (MACHINE_IS_SPECTRUM_16_48 && dandanator_switched_on.v) {
				char texto_paginas[5];
				menu_debug_registers_get_mem_page(0,texto_paginas);
				menu_escribe_linea_opcion(linea++,-1,1,texto_paginas);
			}

			//Si multiface y maquina 48kb. TODO. Si esta dandanator y tambien multiface, muestra siempre dandanator
			if (MACHINE_IS_SPECTRUM_16_48 && multiface_enabled.v && multiface_switched_on.v) {
				char texto_paginas[5];
				menu_debug_registers_get_mem_page(0,texto_paginas);
				menu_escribe_linea_opcion(linea++,-1,1,texto_paginas);
			}
                        //Paginas memoria
                        if (MACHINE_IS_ZXUNO_BOOTM_ENABLED ) {
                                int pagina;
                                //4 paginas, texto 6 caracteres max
                                char texto_paginas[4][7];

                                for (pagina=0;pagina<4;pagina++) {
                                        menu_debug_registers_get_mem_page_zxuno_bootm(pagina,texto_paginas[pagina]);
                                }
                                sprintf (textoregistros,"%s %s %s %s",texto_paginas[0],texto_paginas[1],texto_paginas[2],texto_paginas[3]);

//D5

                                menu_escribe_linea_opcion(linea++,-1,1,textoregistros);
                        }


			//BANK PAGES y Sonido y Snooze
			if (MACHINE_IS_Z88) {
				sprintf (textoregistros,"BANK%02X BANK%02X BANK%02X BANK%02X",blink_mapped_memory_banks[0],blink_mapped_memory_banks[1],
				blink_mapped_memory_banks[2],blink_mapped_memory_banks[3]);
				menu_escribe_linea_opcion(linea++,-1,1,textoregistros);


				z80_byte srunsbit=blink_com >> 6;
				sprintf (textoregistros,"SRUN: %01d SBIT: %01d SNZ: %01d COM: %01d",(srunsbit>>1)&1,srunsbit&1,z88_snooze.v,z88_coma.v);
				menu_escribe_linea_opcion(linea++,-1,1,textoregistros);
			}

			//Paginas RAM en CHLOE
			if (MACHINE_IS_CHLOE) {
				char texto_paginas[8][3];
				//char tipo_memoria[3];
				int pagina;
				for (pagina=0;pagina<8;pagina++) {
					if (chloe_type_memory_paged[pagina]==CHLOE_MEMORY_TYPE_ROM)  sprintf (texto_paginas[pagina],"R%d",debug_chloe_paginas_memoria_mapeadas[pagina]);
					if (chloe_type_memory_paged[pagina]==CHLOE_MEMORY_TYPE_HOME) sprintf (texto_paginas[pagina],"H%d",debug_chloe_paginas_memoria_mapeadas[pagina]);
					if (chloe_type_memory_paged[pagina]==CHLOE_MEMORY_TYPE_DOCK) sprintf (texto_paginas[pagina],"%s","DO");
					if (chloe_type_memory_paged[pagina]==CHLOE_MEMORY_TYPE_EX)   sprintf (texto_paginas[pagina],"%s","EX");
					//sprintf (texto_paginas[pagina],"%c%d",tipo_memoria,debug_chloe_paginas_memoria_mapeadas[pagina]);
				}

				sprintf (textoregistros,"MEM %s %s %s %s %s %s %s %s",texto_paginas[0],texto_paginas[1],texto_paginas[2],texto_paginas[3],
					texto_paginas[4],texto_paginas[5],texto_paginas[6],texto_paginas[7]);
				menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

			}

			if (MACHINE_IS_PRISM) {
				//Si modo ram en rom
                		if (puerto_8189 & 1) {



		                  //Paginas RAM en PRISM
                                char texto_paginas[8][4];
                                		//char tipo_memoria[3];
		                                int pagina;

				   for (pagina=0;pagina<8;pagina++) {
                                                sprintf (texto_paginas[pagina],"A%02d",debug_prism_paginas_memoria_mapeadas[pagina]);
				  }

                                sprintf (textoregistros,"%s %s %s %s %s %s %s %s",texto_paginas[0],texto_paginas[1],texto_paginas[2],texto_paginas[3],
                                        texto_paginas[4],texto_paginas[5],texto_paginas[6],texto_paginas[7]);
                                menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

			}



			//Informacion VRAM en PRISM
			   else {
				//char texto_vram[32];

				//SI vram aperture
				if (prism_ula2_registers[1] & 1) sprintf (textoregistros,"VRAM0 VRAM1 aperture");

				else {
							//       012345678901234567890123456789012
					sprintf (textoregistros,"VRAM0 SRAM10 SRAM11 not apert.");
				}

				menu_escribe_linea_opcion(linea++,-1,1,textoregistros);


				//Paginas RAM en PRISM
                                char texto_paginas[8][4];
                                //char tipo_memoria[3];
                                int pagina;
				//TODO. como mostrar texto reducido aqui para paginas 2 y 3 segun vram aperture/no aperture??
                                for (pagina=0;pagina<8;pagina++) {
                                        if (prism_type_memory_paged[pagina]==PRISM_MEMORY_TYPE_ROM)  sprintf (texto_paginas[pagina],"O%02d",debug_prism_paginas_memoria_mapeadas[pagina]);

                                        if (prism_type_memory_paged[pagina]==PRISM_MEMORY_TYPE_HOME) {
						sprintf (texto_paginas[pagina],"A%02d",debug_prism_paginas_memoria_mapeadas[pagina]);
						if (pagina==2 || pagina==3) {
							//La info de segmentos 2 y 3 (vram aperture si/no) se muestra de info anterior
							sprintf (texto_paginas[pagina],"VRA");
						}
					}
                                        if (prism_type_memory_paged[pagina]==PRISM_MEMORY_TYPE_DOCK) sprintf (texto_paginas[pagina],"%s","DO");
                                        if (prism_type_memory_paged[pagina]==PRISM_MEMORY_TYPE_EX)   sprintf (texto_paginas[pagina],"%s","EX");

					//Si pagina rom failsafe
					if (prism_failsafe_mode.v) {
						if (pagina==0 || pagina==1) {
							sprintf (texto_paginas[pagina],"%s","FS");
						}
					}


					//Si paginando zona alta c000h con paginas 10,11 (que realmente son vram0 y 1) o paginas 14,15 (que realmente son vram 2 y 3)
					if (pagina==6 || pagina==7) {
						int pagina_mapeada=debug_prism_paginas_memoria_mapeadas[pagina];
						int vram_pagina=-1;
						switch (pagina_mapeada) {
							case 10:
								vram_pagina=0;
							break;

							case 11:
								vram_pagina=1;
							break;

							case 14:
								vram_pagina=2;
							break;

							case 15:
								vram_pagina=3;
							break;
						}

						if (vram_pagina!=-1) {
							sprintf (texto_paginas[pagina],"V%d",vram_pagina);
						}
					}
                                        //sprintf (texto_paginas[pagina],"%c%d",tipo_memoria,debug_prism_paginas_memoria_mapeadas[pagina]);
                                }

                                sprintf (textoregistros,"%s %s %s %s %s %s %s %s",texto_paginas[0],texto_paginas[1],texto_paginas[2],texto_paginas[3],
                                        texto_paginas[4],texto_paginas[5],texto_paginas[6],texto_paginas[7]);
                                menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                          }
			}

			  //Paginas RAM en TIMEX
                        if (MACHINE_IS_TIMEX_TS2068) {
                                char texto_paginas[8][3];
                                //char tipo_memoria;
                                int pagina;
                                for (pagina=0;pagina<8;pagina++) {
                                        if (timex_type_memory_paged[pagina]==TIMEX_MEMORY_TYPE_ROM)  sprintf (texto_paginas[pagina],"%s","RO");
                                        if (timex_type_memory_paged[pagina]==TIMEX_MEMORY_TYPE_HOME) sprintf (texto_paginas[pagina],"%s","HO");
                                        if (timex_type_memory_paged[pagina]==TIMEX_MEMORY_TYPE_DOCK) sprintf (texto_paginas[pagina],"%s","DO");
                                        if (timex_type_memory_paged[pagina]==TIMEX_MEMORY_TYPE_EX)   sprintf (texto_paginas[pagina],"%s","EX");
                                        //sprintf (texto_paginas[pagina],"%c%d",tipo_memoria,debug_timex_paginas_memoria_mapeadas[pagina]);
                                }

                                sprintf (textoregistros,"MEM %s %s %s %s %s %s %s %s",texto_paginas[0],texto_paginas[1],texto_paginas[2],texto_paginas[3],
                                        texto_paginas[4],texto_paginas[5],texto_paginas[6],texto_paginas[7]);
                                menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        }

			//Paginas RAM en CPC
//#define CPC_MEMORY_TYPE_ROM 0
//#define CPC_MEMORY_TYPE_RAM 1

//extern z80_byte debug_cpc_type_memory_paged_read[];
//extern z80_byte debug_cpc_paginas_memoria_mapeadas_read[];
			if (MACHINE_IS_CPC) {
                                char texto_paginas[4][5];
                                int pagina;
                                for (pagina=0;pagina<4;pagina++) {
                                        if (debug_cpc_type_memory_paged_read[pagina]==CPC_MEMORY_TYPE_ROM)
						sprintf (texto_paginas[pagina],"%s%d","ROM",debug_cpc_paginas_memoria_mapeadas_read[pagina]);

                                        if (debug_cpc_type_memory_paged_read[pagina]==CPC_MEMORY_TYPE_RAM)
						sprintf (texto_paginas[pagina],"%s%d","RAM",debug_cpc_paginas_memoria_mapeadas_read[pagina]);

                                }

                                sprintf (textoregistros,"MEM %s %s %s %s",texto_paginas[0],texto_paginas[1],texto_paginas[2],texto_paginas[3]);
                                menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        }

			//Paginas RAM en SAM
			if (MACHINE_IS_SAM) {
				char texto_paginas[4][6];
                                int pagina;
                                for (pagina=0;pagina<4;pagina++) {
                                        if (sam_memory_paged_type[pagina]==0)
                                                sprintf (texto_paginas[pagina],"%s%02d","RAM",debug_sam_paginas_memoria_mapeadas[pagina]);

                                        if (sam_memory_paged_type[pagina])
                                                sprintf (texto_paginas[pagina],"%s%02d","ROM",debug_sam_paginas_memoria_mapeadas[pagina]);


                                }

                                sprintf (textoregistros,"MEM %s %s %s %s",texto_paginas[0],texto_paginas[1],texto_paginas[2],texto_paginas[3]);
                                menu_escribe_linea_opcion(linea++,-1,1,textoregistros);

                        }



			if (MACHINE_IS_SPECTRUM || MACHINE_IS_ZX8081) {
                        sprintf (textoregistros,"AUDIO: BEEPER: %03d AY: %03d", (MACHINE_IS_ZX8081 ? da_amplitud_speaker_zx8081() :  value_beeper),da_output_ay() );
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);
			}



			if (MACHINE_IS_ZX8081) {
	                        sprintf (textoregistros,"FPS: %03d VPS: %03d SCANLIN: %03d",ultimo_fps,last_vsync_per_second,t_scanline_draw);
			}
			else {
	                        sprintf (textoregistros,"FPS: %03d SCANLINE: %03d",ultimo_fps,t_scanline_draw);
			}
                        menu_escribe_linea_opcion(linea++,-1,1,textoregistros);


			//Video zx80/81
			if (MACHINE_IS_ZX8081) {
				sprintf (textoregistros,"LNCTR: %x LCNTR %s ULAV: %s",(video_zx8081_linecntr &7),(video_zx8081_linecntr_enabled.v ? "On" : "Off"),
					(video_zx8081_ula_video_output == 0 ? "+5V" : "0V"));
				menu_escribe_linea_opcion(linea++,-1,1,textoregistros);
			}


			//Detectores de silencio. Lo quito pues no lo considero muy util y necesito lineas
			//sprintf (textoregistros,"Silence: %s Silence BEEP: %s",(silence_detection_counter==SILENCE_DETECTION_MAX ? "Yes" : "No"),(beeper_silence_detection_counter==SILENCE_DETECTION_MAX ? "Yes" : "No") );
			//menu_escribe_linea_opcion(linea++,-1,1,textoregistros);


		}




	return linea;

}

z80_bit menu_breakpoint_exception_pending_show={0};

void menu_debug_registers_ventana(void)
{
	if (menu_breakpoint_exception_pending_show.v==1 || menu_breakpoint_exception.v) menu_dibuja_ventana(0,0,32,23,"Debug CPU & ULA (brk cond)");
	else menu_dibuja_ventana(0,0,32,23,"Debug CPU & ULA");


	menu_breakpoint_exception_pending_show.v=0;
}


int continuous_step=0;

void menu_debug_registers_gestiona_breakpoint(void)
{
                menu_breakpoint_exception.v=0;
		menu_breakpoint_exception_pending_show.v=1;
                cpu_step_mode.v=1;
                //printf ("Reg pc: %d\n",reg_pc);
		continuous_step=0;

}

void menu_watches_view(MENU_ITEM_PARAMETERS)
{
	debug_watches_loop(debug_watches_text_to_watch,debug_watches_texto_destino);
	menu_generic_message("Watch result",debug_watches_texto_destino);
}


void menu_watches_conditions_set(MENU_ITEM_PARAMETERS)
{

        char string_texto[MAX_BREAKPOINT_CONDITION_LENGTH];

        sprintf (string_texto,"%s",debug_watches_text_to_watch);

        menu_ventana_scanf("Watch",string_texto,MAX_BREAKPOINT_CONDITION_LENGTH);

        sprintf (debug_watches_text_to_watch,"%s",string_texto);


		//esto tiene que activar los breakpoints, sino no se evalúa
      if (debug_breakpoints_enabled.v==0) menu_breakpoints_enable_disable(0);


}

void menu_watches_y_position(MENU_ITEM_PARAMETERS)
{
	if (debug_watches_y_position==0) debug_watches_y_position=12;
	else debug_watches_y_position=0;
}

void menu_watches(MENU_ITEM_PARAMETERS)
{

        menu_espera_no_tecla();

        menu_item *array_menu_watches;
        menu_item item_seleccionado;
        int retorno_menu;
        do {




                char string_condition_shown[15];
                        if (debug_watches_text_to_watch[0]) {
                                menu_tape_settings_trunc_name(debug_watches_text_to_watch,string_condition_shown,15);
                        }
                        else {
                                sprintf(string_condition_shown,"None");
                        }

                        menu_add_item_menu_inicial_format(&array_menu_watches,MENU_OPCION_NORMAL,menu_watches_conditions_set,NULL,"Watch: %s",string_condition_shown);
	                menu_add_item_menu_tooltip(array_menu_watches,"Add an expression to watch in real time");
			menu_add_item_menu_ayuda(array_menu_watches,"You can write registers and variable names, separated by only 1 space, to see "
						"their values in real time. They are shown 50 times per second on the display (with menu closed). "
						"Registers and variable names are the same used on Breakpoint conditions, for example: \n"
						"A BC IX PWA\n\n"
						"Note: Setting a watch enables breakpoints, it needed them to be enabled\n");

			//mostrar aqui ultimo valor watch
			menu_add_item_menu_format(array_menu_watches,MENU_OPCION_NORMAL,menu_watches_view,NULL,"View watch result");
			menu_add_item_menu_tooltip(array_menu_watches,"View watch result");
			menu_add_item_menu_ayuda(array_menu_watches,"View watch result");


			menu_add_item_menu_format(array_menu_watches,MENU_OPCION_NORMAL,menu_watches_y_position,NULL,"Watch y coord: %d",debug_watches_y_position);
			menu_add_item_menu_tooltip(array_menu_watches,"Changes y coordinate of watch message");
			menu_add_item_menu_ayuda(array_menu_watches,"Changes y coordinate of watch message");



 menu_add_item_menu(array_menu_watches,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                menu_add_ESC_item(array_menu_watches);
                retorno_menu=menu_dibuja_menu(&watches_opcion_seleccionada,&item_seleccionado,array_menu_watches,"Watches" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}

void menu_debug_configuration_stepover(MENU_ITEM_PARAMETERS)
{
	debug_core_evitamos_inter.v ^=1;
}

/*void menu_debug_configuration(MENU_ITEM_PARAMETERS)
{

        menu_espera_no_tecla();

        menu_item *array_menu_debug_configuration;
        menu_item item_seleccionado;
        int retorno_menu;
        do {


                        menu_add_item_menu_inicial_format(&array_menu_debug_configuration,MENU_OPCION_NORMAL,menu_debug_configuration_stepover,NULL,"Step over interrupt: %s",(debug_core_evitamos_inter.v ? "Yes" : "No") );
                        menu_add_item_menu_tooltip(array_menu_debug_configuration,"Avoid step to step or continuous execution of nmi or maskable interrupt routines");
                        menu_add_item_menu_ayuda(array_menu_debug_configuration,"Avoid step to step or continuous execution of nmi or maskable interrupt routines");


			menu_add_item_menu_format(array_menu_debug_configuration,MENU_OPCION_NORMAL, menu_breakpoints_condition_behaviour,NULL,"Breakp. behaviour: %s",(debug_breakpoints_cond_behaviour.v ? "On Change" : "Always") );
                        menu_add_item_menu_tooltip(array_menu_debug_configuration,"Indicates wether breakpoints are fired always or only on change from false to true");
                        menu_add_item_menu_ayuda(array_menu_debug_configuration,"Indicates wether breakpoints are fired always or only on change from false to true");


		menu_add_item_menu(array_menu_debug_configuration,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                menu_add_ESC_item(array_menu_debug_configuration);
                retorno_menu=menu_dibuja_menu(&debug_configuration_opcion_seleccionada,&item_seleccionado,array_menu_debug_configuration,"Debug Configuration" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}
*/

void menu_debug_registers_next_view(void)
{
	menu_debug_registers_mostrando++;
	if (menu_debug_registers_mostrando==5) menu_debug_registers_mostrando=0;
}


void menu_debug_registers(MENU_ITEM_PARAMETERS)
{

	z80_byte acumulado;

	//ninguna tecla pulsada inicialmente
	acumulado=MENU_PUERTO_TECLADO_NINGUNA;

	int linea;

	z80_byte tecla;

	int valor_contador_segundo_anterior;

valor_contador_segundo_anterior=contador_segundo;


	//Ver si hemos entrado desde un breakpoint
	if (menu_breakpoint_exception.v) menu_debug_registers_gestiona_breakpoint();

	else menu_espera_no_tecla();

	menu_debug_registers_ventana();

	do {


		//Si no esta el modo step de la cpu
		if (cpu_step_mode.v==0) {

	                //esto hara ejecutar esto 2 veces por segundo
        	        //if ( (contador_segundo%25) == 0 || menu_multitarea==0) {

			//Cuadrarlo cada 1/16 de segundo, justo lo mismo que el flash, asi
			//el valor de flash se ve coordinado
        	        //if ( (contador_segundo%(16*20)) == 0 || menu_multitarea==0) {
									if ( ((contador_segundo%(16*20)) == 0 && valor_contador_segundo_anterior!=contador_segundo ) || menu_multitarea==0) {
										//printf ("Refresco pantalla. contador_segundo=%d\n",contador_segundo);
										valor_contador_segundo_anterior=contador_segundo;


				linea=menu_debug_registers_print_registers();

                        	menu_escribe_linea_opcion(linea++,-1,1,"");
                        	menu_escribe_linea_opcion(linea++,-1,1,"S: Step mode D: Disassemble");

			        char mensaje_esc_back[32];
				//sprintf (mensaje_esc_back,"B: Breakpoints %s Back",esc_key_message);
				sprintf (mensaje_esc_back,"B: Breakpoints W: Watch");


                        	menu_escribe_linea_opcion(linea++,-1,1,mensaje_esc_back);

				menu_escribe_linea_opcion(linea++,-1,1,"P: Clr tstatesp G: Chg View");


                        	if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();

				//tecla=menu_get_pressed_key();

				//printf ("tecla: %d\n",tecla);

				//if (tecla=='s') cpu_step_mode.v=1;

	                }

        	        menu_cpu_core_loop();

			if (menu_breakpoint_exception.v) {
				menu_debug_registers_gestiona_breakpoint();
				//Y redibujar ventana para reflejar breakpoint cond
				menu_debug_registers_ventana();
			}

                	acumulado=menu_da_todas_teclas();

	                //si no hay multitarea, esperar tecla y salir
        	        if (menu_multitarea==0) {
                	        menu_espera_tecla();

                        	acumulado=0;
	                }

			//Hay tecla pulsada
			if ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) !=MENU_PUERTO_TECLADO_NINGUNA ) {
                                tecla=menu_get_pressed_key();

				menu_espera_no_tecla_no_cpu_loop();

                                //printf ("tecla: %d\n",tecla);

                                if (tecla=='s') cpu_step_mode.v=1;

				if (tecla=='d') {
					cls_menu_overlay();
					menu_debug_disassemble(0);
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
					menu_debug_registers_ventana();
				}

				if (tecla=='b') {
					cls_menu_overlay();
					menu_breakpoints(0);
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
					menu_debug_registers_ventana();
				}

				if (tecla=='w') {
                                        cls_menu_overlay();
                                        menu_watches(0);
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
                                        menu_debug_registers_ventana();
                                }

                              /*  if (tecla=='t') {
                                        cls_menu_overlay();
                                        menu_debug_configuration(0);
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
                                        menu_debug_registers_ventana();
                                }*/

                                if (tecla=='p') {
                                        cls_menu_overlay();
					debug_t_estados_parcial=0;
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
                                        menu_debug_registers_ventana();
                                }

																if (tecla=='g') {
                                        cls_menu_overlay();
																				menu_debug_registers_next_view();
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
                                        menu_debug_registers_ventana();
																}


			}

		}

		//En modo Step mode
		else {


			int si_ejecuta_una_instruccion=1;

                        linea=menu_debug_registers_print_registers();

                       	menu_escribe_linea_opcion(linea++,-1,1,"");

			if (continuous_step==0) {
				menu_escribe_linea_opcion(linea++,-1,1,"S: Exit step C: Cont step");
				menu_escribe_linea_opcion(linea++,-1,1,"B: Breakp W: Watch V: V.Scr");
				menu_escribe_linea_opcion(linea++,-1,1,"P: Clr tstatesp G: Chg View");
			}
			else {
				menu_escribe_linea_opcion(linea++,-1,1,"Any key: Stop cont step");
			}



			//Actualizamos pantalla
			all_interlace_scr_refresca_pantalla();


			//Esperamos tecla
			if (continuous_step==0) {
				menu_espera_tecla_no_cpu_loop();
                        	tecla=menu_get_pressed_key();


				//A cada pulsacion de tecla, mostramos la pantalla del ordenador emulado
				cls_menu_overlay();
				all_interlace_scr_refresca_pantalla();

				menu_espera_no_tecla_no_cpu_loop();


				//y mostramos ventana de nuevo
				//NOTA: Si no se hace este cls_menu_overlay, en modos zx80/81, se queda en color oscuro el texto de la ventana
				//porque? no estoy seguro, pero es necesario este cls_menu_overlay
				cls_menu_overlay();

				menu_debug_registers_ventana();

        	                if (tecla=='s') {
					cpu_step_mode.v=0;

					//Decimos que no hay tecla pulsada
					acumulado=MENU_PUERTO_TECLADO_NINGUNA;

					//decirle que despues de pulsar esta tecla no tiene que ejecutar siguiente instruccion
					si_ejecuta_una_instruccion=0;
				}

				if (tecla=='c') {
					continuous_step=1;
				}

                                if (tecla=='b') {
                                        cls_menu_overlay();
                                        menu_breakpoints(0);
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
					menu_debug_registers_ventana();

					//decirle que despues de pulsar esta tecla no tiene que ejecutar siguiente instruccion
					si_ejecuta_una_instruccion=0;
                                }

                                if (tecla=='w') {
                                        cls_menu_overlay();
                                        menu_watches(0);
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
                                        menu_debug_registers_ventana();

                                        //decirle que despues de pulsar esta tecla no tiene que ejecutar siguiente instruccion
                                        si_ejecuta_una_instruccion=0;
                                }

                              /*  if (tecla=='t') {
                                        cls_menu_overlay();
                                        menu_debug_configuration(0);
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
                                        menu_debug_registers_ventana();

                                        //decirle que despues de pulsar esta tecla no tiene que ejecutar siguiente instruccion
                                        si_ejecuta_una_instruccion=0;
                                }*/



                                if (tecla=='p') {
                                        cls_menu_overlay();
																				debug_t_estados_parcial=0;
                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
                                        menu_debug_registers_ventana();

                                        //decirle que despues de pulsar esta tecla no tiene que ejecutar siguiente instruccion
                                        si_ejecuta_una_instruccion=0;
                                }


																if (tecla=='g') {
                                        cls_menu_overlay();
																				menu_debug_registers_next_view();

                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;
                                        menu_debug_registers_ventana();

																				//decirle que despues de pulsar esta tecla no tiene que ejecutar siguiente instruccion
                                        si_ejecuta_una_instruccion=0;
																}



				if (tecla=='v') {
					cls_menu_overlay();
				        menu_espera_no_tecla_no_cpu_loop();

				        //para que no se vea oscuro
				        menu_abierto=0;
				        all_interlace_scr_refresca_pantalla();
				        menu_espera_tecla_no_cpu_loop();
				        menu_abierto=1;
					menu_espera_no_tecla_no_cpu_loop();
					menu_debug_registers_ventana();

					//decirle que despues de pulsar esta tecla no tiene que ejecutar siguiente instruccion
					si_ejecuta_una_instruccion=0;
				}

			}

			else {
				//Cualquier tecla Detiene el continuous loop
				//printf ("continuos loop\n");
				acumulado=menu_da_todas_teclas();
				if ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) != MENU_PUERTO_TECLADO_NINGUNA) {
					continuous_step=0;
					//printf ("cont step: %d\n",continuous_step);

                                        //Decimos que no hay tecla pulsada
                                        acumulado=MENU_PUERTO_TECLADO_NINGUNA;

					menu_espera_no_tecla_no_cpu_loop();

				}

			}


			//1 instruccion cpu
			if (si_ejecuta_una_instruccion) {
				//printf ("ejecutando instruccion en step-to-step o continuous\n");
				debug_core_lanzado_inter.v=0;
				cpu_core_loop();
				//Ver si se ha disparado interrupcion (nmi o maskable)
				if (debug_core_lanzado_inter.v && debug_core_evitamos_inter.v) {
					//Ejecutar hasta que registro PC vuelva a su valor anterior o lleguemos a un limite
					//873600 instrucciones es 50 frames de instrucciones de 4 t-estados (69888/4*50)
					int limite_instrucciones=0;
					int salir=0;
					while (limite_instrucciones<873600 && salir==0) {
						if (reg_pc==debug_core_lanzado_inter_retorno_pc_nmi ||
						reg_pc==debug_core_lanzado_inter_retorno_pc_maskable) {
							salir=1;
						}
						else {
							debug_printf (VERBOSE_DEBUG,"Running and step over interrupt handler. PC=0x%04X",reg_pc);
							cpu_core_loop();
							limite_instrucciones++;
						}
					}
				}
			}

			if (menu_breakpoint_exception.v) {
				menu_debug_registers_gestiona_breakpoint();
                                //Y redibujar ventana para reflejar breakpoint cond
                                menu_debug_registers_ventana();
			}

		}

	//Hacer mientras step mode este activo o no haya tecla pulsada
        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA || cpu_step_mode.v==1);

        cls_menu_overlay();

}

void menu_debug_ioports(MENU_ITEM_PARAMETERS)
{

	char stats_buffer[MAX_TEXTO_GENERIC_MESSAGE];


	debug_get_ioports(stats_buffer);

  menu_generic_message("IO Ports",stats_buffer);

}

int menu_debug_hexdump_change_pointer(int p)
{


        char string_address[8];

        sprintf (string_address,"%04XH",p);


        //menu_ventana_scanf("Address? (in hex)",string_address,6);
        menu_ventana_scanf("Address?",string_address,8);

	//p=strtol(string_address, NULL, 16);
	p=parse_string_to_number(string_address);


	return p;

}

void menu_debug_hexdump_ventana(void)
{
        menu_dibuja_ventana(0,1,32,21,"Hex Dump");
}

int menu_debug_hexdump_with_ascii_modo_ascii=0;

void menu_debug_hexdump_with_ascii(char *dumpmemoria,menu_z80_moto_int dir_leida,int bytes_por_linea)
{
	dir_leida=adjust_address_space_cpu(dir_leida);
	int longitud_direccion=4;
	if (CPU_IS_MOTOROLA) longitud_direccion=5;

	if (CPU_IS_MOTOROLA) sprintf (dumpmemoria,"%05X",dir_leida);
	else sprintf (dumpmemoria,"%04X",dir_leida);
	//cambiamos el 0 final por un espacio


	dumpmemoria[longitud_direccion]=' ';

	menu_debug_registers_dump_hex(&dumpmemoria[longitud_direccion+1],dir_leida,bytes_por_linea);

	//metemos espacio
	int offset=5+bytes_por_linea*2;
	if (!CPU_IS_MOTOROLA) dumpmemoria[offset]=' ';

	menu_debug_registers_dump_ascii(&dumpmemoria[offset+1],dir_leida,bytes_por_linea,menu_debug_hexdump_with_ascii_modo_ascii);

	//printf ("%s\n",dumpmemoria);
}

void menu_debug_hexdump(MENU_ITEM_PARAMETERS)
{
        menu_espera_no_tecla();
	menu_debug_hexdump_ventana();

	menu_reset_counters_tecla_repeticion();

        z80_byte tecla=0;

	//z80_int direccion=reg_pc;
	menu_z80_moto_int direccion=get_pc_register();

	int salir=0;



	if (MACHINE_IS_ZX80) menu_debug_hexdump_with_ascii_modo_ascii=1;
	else if (MACHINE_IS_ZX81) menu_debug_hexdump_with_ascii_modo_ascii=2;

	else menu_debug_hexdump_with_ascii_modo_ascii=0;

        do {

					//Si maquina no es QL, direccion siempre entre 0 y 65535
					direccion=adjust_address_space_cpu(direccion);

				int linea=0;

				int lineas_hex=0;
		const int bytes_por_linea=8;
		const int lineas_total=14;

		int bytes_por_ventana=bytes_por_linea*lineas_total;

		char dumpmemoria[32];

				char textoshow[32];
				sprintf (textoshow,"Showing %d bytes per page:",bytes_por_ventana);
                                menu_escribe_linea_opcion(linea++,-1,1,textoshow);
                                menu_escribe_linea_opcion(linea++,-1,1,"");

		for (;lineas_hex<lineas_total;lineas_hex++,linea++) {

			menu_z80_moto_int dir_leida=direccion+lineas_hex*bytes_por_linea;
			direccion=adjust_address_space_cpu(direccion);
			menu_debug_hexdump_with_ascii(dumpmemoria,dir_leida,bytes_por_linea);

			/*

			sprintf (dumpmemoria,"%04X",dir_leida);
			//cambiamos el 0 final por un espacio
			dumpmemoria[4]=' ';

			menu_debug_registers_dump_hex(&dumpmemoria[5],dir_leida,bytes_por_linea);

			//metemos espacio
			int offset=5+bytes_por_linea*2;
			dumpmemoria[offset]=' ';

			menu_debug_registers_dump_ascii(&dumpmemoria[offset+1],dir_leida,bytes_por_linea,menu_debug_hexdump_with_ascii_modo_ascii);
			*/

			menu_escribe_linea_opcion(linea,-1,1,dumpmemoria);
		}


                                menu_escribe_linea_opcion(linea++,-1,1,"");

				char buffer_linea[40];
				if (menu_debug_hexdump_with_ascii_modo_ascii==0) {
					sprintf (buffer_linea,"M: Change pointer C: ASCII");
				}

				else if (menu_debug_hexdump_with_ascii_modo_ascii==1) {
                                        sprintf (buffer_linea,"M: Change pointer C: ZX80");
                                }

				else sprintf (buffer_linea,"M: Change pointer C: ZX81");


				//menu_escribe_linea_opcion(linea++,-1,1,"M: Change pointer C:");
				menu_escribe_linea_opcion(linea++,-1,1,buffer_linea);

				if (MACHINE_IS_INVES) {
					if (menu_debug_hex_shows_inves_low_ram.v) menu_escribe_linea_opcion(linea++,-1,1,"L: Hide Inves Low RAM");
					else menu_escribe_linea_opcion(linea++,-1,1,"L: Show Inves Low RAM");
				}

				if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();


                                menu_espera_tecla();

                                tecla=menu_get_pressed_key();

                                menu_espera_no_tecla_con_repeticion();

				switch (tecla) {

					case 11:
						//arriba
						direccion -=bytes_por_linea;
					break;

					case 10:
						//abajo
						direccion +=bytes_por_linea;
					break;

					case 24:
						//PgUp
						direccion -=bytes_por_ventana;
					break;

					case 25:
						//PgDn
						direccion +=bytes_por_ventana;
					break;

					case 'm':
						direccion=menu_debug_hexdump_change_pointer(direccion);
						menu_debug_hexdump_ventana();
					break;

					case 'c':
						menu_debug_hexdump_with_ascii_modo_ascii++;
						if (menu_debug_hexdump_with_ascii_modo_ascii==3) menu_debug_hexdump_with_ascii_modo_ascii=0;
					break;

					case 'l':
						menu_debug_hex_shows_inves_low_ram.v ^=1;
					break;

					default:
						salir=1;
					break;
				}


        } while (salir==0);

	cls_menu_overlay();

}



#define SPRITES_X 1
#define SPRITES_Y 2
#define SPRITES_ANCHO 30
#define SPRITES_ALTO 20

menu_z80_moto_int view_sprites_direccion=0x3d00;

//ancho en bytes
z80_int view_sprites_ancho_sprite=1;

//alto en pixeles
z80_int view_sprites_alto_sprite=8*6;

void menu_debug_draw_sprites(void)
{

	normal_overlay_texto_menu();

        //int ancho=(SPRITES_ANCHO-2)*8;
        //int alto=(SPRITES_ALTO-4)*8;
        int xorigen=(SPRITES_X+1)*8;
        int yorigen=(SPRITES_Y+3)*8;


        int x,y,bit;
	z80_byte byte_leido;

	menu_z80_moto_int puntero=view_sprites_direccion;
	z80_byte color;

	for (y=0;y<view_sprites_alto_sprite;y++) {
		for (x=0;x<view_sprites_ancho_sprite;x++) {
			byte_leido=peek_byte_z80_moto(puntero++);

			for (bit=0;bit<8;bit++) {
				if ( (byte_leido & 128)==0 ) color=ESTILO_GUI_PAPEL_NORMAL;
				else color=ESTILO_GUI_TINTA_NORMAL;

				byte_leido <<=1;

                		//dibujamos valor actual
		                scr_putpixel_zoom(xorigen+x*8+bit,yorigen+y,color);
			}
		}
	}



}

menu_z80_moto_int menu_debug_view_sprites_change_pointer(menu_z80_moto_int p)
{

       //restauramos modo normal de texto de menu, porque sino, tendriamos la ventana
        //del cambio de direccion, y encima los sprites
       set_menu_overlay_function(normal_overlay_texto_menu);


        char string_address[8];

				util_sprintf_address_hex(p,string_address);

        //menu_ventana_scanf("Address? (in hex)",string_address,6);
        menu_ventana_scanf("Address?",string_address,8);

        //p=strtol(string_address, NULL, 16);

        p=parse_string_to_number(string_address);



        set_menu_overlay_function(menu_debug_draw_sprites);


        return p;

}



void menu_debug_view_sprites_ventana(void)
{

	menu_dibuja_ventana(SPRITES_X,SPRITES_Y,SPRITES_ANCHO,SPRITES_ALTO,"Sprites");

}

void menu_debug_view_sprites(MENU_ITEM_PARAMETERS)
{

        //Desactivamos interlace - si esta. Con interlace la forma de onda se dibuja encima continuamente, sin borrar
        z80_bit copia_video_interlaced_mode;
        copia_video_interlaced_mode.v=video_interlaced_mode.v;

        disable_interlace();


        menu_espera_no_tecla();
	menu_debug_view_sprites_ventana();

        z80_byte tecla=0;


        int salir=0;


        //Cambiamos funcion overlay de texto de menu
        //Se establece a la de funcion de onda + texto
        set_menu_overlay_function(menu_debug_draw_sprites);



        do {

		int bytes_por_linea=view_sprites_ancho_sprite;
		int bytes_por_ventana=view_sprites_ancho_sprite*view_sprites_alto_sprite;

		int linea=0;
		char buffer_texto[32];


		view_sprites_direccion=adjust_address_space_cpu(view_sprites_direccion);
		if (CPU_IS_MOTOROLA) sprintf (buffer_texto,"Address: %05X Size: %dX%d",view_sprites_direccion,view_sprites_ancho_sprite*8,view_sprites_alto_sprite);
		else sprintf (buffer_texto,"Address: %04X Size: %dX%d",view_sprites_direccion,view_sprites_ancho_sprite*8,view_sprites_alto_sprite);

		menu_escribe_linea_opcion(linea++,-1,1,buffer_texto);

		linea=SPRITES_ALTO-3;

		menu_escribe_linea_opcion(linea++,-1,1,"M: pointer    OPQA: Size");

		if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();

		menu_espera_tecla();


                                tecla=menu_get_pressed_key();

                                menu_espera_no_tecla_con_repeticion();

                                switch (tecla) {

                                        case 11:
                                                //arriba
                                                view_sprites_direccion -=bytes_por_linea;
                                        break;

                                        case 10:
                                                //abajo
                                                view_sprites_direccion +=bytes_por_linea;
                                        break;

                                        case 24:
                                                //PgUp
                                                view_sprites_direccion -=bytes_por_ventana;
                                        break;

                                        case 25:
                                                //PgDn
                                                view_sprites_direccion +=bytes_por_ventana;
                                        break;

                                        case 'm':
                                                view_sprites_direccion=menu_debug_view_sprites_change_pointer(view_sprites_direccion);
						menu_debug_view_sprites_ventana();
                                        break;

					case 'o':
						if (view_sprites_ancho_sprite>1) view_sprites_ancho_sprite--;
					break;

					case 'p':
						if (view_sprites_ancho_sprite<SPRITES_ANCHO-3) view_sprites_ancho_sprite++;
					break;

                                        case 'q':
                                                if (view_sprites_alto_sprite>1) view_sprites_alto_sprite--;
                                        break;

                                        case 'a':
                                                if (view_sprites_alto_sprite<(SPRITES_ALTO-6)*8)  view_sprites_alto_sprite++;
                                        break;


                                        default:
                                                salir=1;
                                        break;
                                }


        } while (salir==0);

        //Restauramos modo interlace
        if (copia_video_interlaced_mode.v) enable_interlace();

       //restauramos modo normal de texto de menu
       set_menu_overlay_function(normal_overlay_texto_menu);

        cls_menu_overlay();



}


void menu_debug_disassemble_ventana(void)
{
        menu_dibuja_ventana(0,1,32,20,"Disassemble");
}

menu_z80_moto_int menu_debug_disassemble_subir(menu_z80_moto_int dir_inicial)
{
	//Subir 1 opcode en el listado

	//Metodo:
	//Empezamos en direccion-10
	//Vamos leyendo opcodes. Cuando estemos en >=direccion, nos quedamos en direccion anterior
	//Esto va bien excepto cuando estamos mirando en las primeras lineas de la ROM (desde la 0 hasta la 10)
	//En ese caso, se cumple la condicion de que dir+longitud_opcode>=dir_inicial (dado que dir_inicial sera 655xx)
	//y por tanto vuelve al momento

	char buffer[32];
	size_t longitud_opcode;

	menu_z80_moto_int dir;

	if (CPU_IS_MOTOROLA) dir=dir_inicial-30; //En el caso de motorola mejor empezar antes
	else dir=dir_inicial-10;

	do {
		//dir=adjust_address_space_cpu(dir);
		debugger_disassemble(buffer,30,&longitud_opcode,dir);
		if (dir+longitud_opcode>=dir_inicial) return dir;

		dir +=longitud_opcode;
	} while (1);
}


void menu_debug_dissassemble_una_instruccion(char *dumpassembler,menu_z80_moto_int dir,int *longitud_final_opcode)
{
	//Formato de texto en buffer:
	//0123456789012345678901234567890
	//Para Z80
	//DDDD AABBCCDD OPCODE-----------
	//Para Motorola
	//DDDDD AABBCCDD OPCODE----------
	//DDDD: Direccion
	//AABBCCDD: Volcado hexa


	size_t longitud_opcode;

	//Metemos 30 espacios
	strcpy(dumpassembler,
	 "                               ");


	//Direccion
	dir=adjust_address_space_cpu(dir);
	int final_address=4;
	if (CPU_IS_MOTOROLA) {
		sprintf(dumpassembler,"%05X",dir);
		final_address=5;
	}

	else sprintf(dumpassembler,"%04X",dir);

	//metemos espacio en 0 final
	dumpassembler[final_address]=' ';


	//Assembler
	debugger_disassemble(&dumpassembler[final_address+10],17,&longitud_opcode,dir);

		//Volcado hexa
		char volcado_hexa[256];
	//menu_debug_registers_dump_hex(&dumpassembler[5],dir,longitud_opcode);
	menu_debug_registers_dump_hex(volcado_hexa,dir,longitud_opcode);

	//Copiar texto volcado hexa hasta llegar a maximo 8
	int final_hexa_limite=longitud_opcode*2;
	if (final_hexa_limite>8) {
		final_hexa_limite=8;
		dumpassembler[final_address+1+8]='+';
	}

	int i;
	for (i=0;i<final_hexa_limite;i++) dumpassembler[final_address+1+i]=volcado_hexa[i];

	//Poner espacio en 0 final
	//dumpassembler[5+longitud_opcode*2]=' ';





	*longitud_final_opcode=longitud_opcode;

}

void menu_debug_disassemble(MENU_ITEM_PARAMETERS)
{
        menu_espera_no_tecla();
        menu_debug_disassemble_ventana();

        menu_reset_counters_tecla_repeticion();

        z80_byte tecla=0;

        menu_z80_moto_int direccion=get_pc_register();

        int salir=0;

        do {
                int linea=0;

                int lineas_disass=0;
                //const int bytes_por_linea=8;
                const int lineas_total=15;

                //int bytes_por_ventana=bytes_por_linea*lineas_total;

                char dumpassembler[32];

		int longitud_opcode;
		int longitud_opcode_primera_linea;

		menu_z80_moto_int dir=direccion;

                for (;lineas_disass<lineas_total;lineas_disass++,linea++) {

			//Formato de texto en buffer:
			//0123456789012345678901234567890
			//DDDD AABBCCDD OPCODE-----------
			//DDDD: Direccion
			//AABBCCDD: Volcado hexa

			//Metemos 30 espacios
			/*
			strcpy(dumpassembler,
			 "                               ");


			//Direccion
			sprintf(dumpassembler,"%04X",dir);
			//metemos espacio en 0 final
			dumpassembler[4]=' ';


			//Assembler
			debugger_disassemble(&dumpassembler[14],17,&longitud_opcode,dir);



			//Volcado hexa
                        menu_debug_registers_dump_hex(&dumpassembler[5],dir,longitud_opcode);

			//Poner espacio en 0 final
			dumpassembler[5+longitud_opcode*2]=' ';
			*/


		 menu_debug_dissassemble_una_instruccion(dumpassembler,dir,&longitud_opcode);


			if (lineas_disass==0) longitud_opcode_primera_linea=longitud_opcode;

			dir +=longitud_opcode;

                        menu_escribe_linea_opcion(linea,-1,1,dumpassembler);
                }


                                menu_escribe_linea_opcion(linea++,-1,1,"");
                                menu_escribe_linea_opcion(linea++,-1,1,"M: Change pointer");

                                if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();


                                menu_espera_tecla();

                                tecla=menu_get_pressed_key();

                                menu_espera_no_tecla_con_repeticion();

				int i;

                                switch (tecla) {

                                        case 11:
                                                //arriba
						direccion=menu_debug_disassemble_subir(direccion);
                                        break;

                                        case 10:
                                                //abajo
                                                direccion +=longitud_opcode_primera_linea;
                                        break;

                                        case 24:
                                                //PgUp
						for (i=0;i<lineas_total;i++) direccion=menu_debug_disassemble_subir(direccion);
                                        break;

                                        case 25:
                                                //PgDn
						direccion=dir;
                                        break;

                                        case 'm':
						//Usamos misma funcion de menu_debug_hexdump_change_pointer
                                                direccion=menu_debug_hexdump_change_pointer(direccion);
                                                menu_debug_disassemble_ventana();
                                        break;

                                        default:
                                                salir=1;
                                        break;
                                }


        } while (salir==0);

        cls_menu_overlay();
}



void menu_debug_poke_128k(MENU_ITEM_PARAMETERS)
{

        int valor_poke,dir,veces;
	int bank;

        char string_poke[4];
        char string_dir[6];
	char string_bank[2];
	char string_veces[6];


	sprintf (string_bank,"0");
	menu_ventana_scanf("Bank",string_bank,2);
	bank=parse_string_to_number(string_bank);
	if (bank<0 || bank>7) {
		debug_printf (VERBOSE_ERR,"Invalid bank %d",bank);
		return;
	}


        sprintf (string_dir,"0");

        menu_ventana_scanf("Address",string_dir,6);

        dir=parse_string_to_number(string_dir);

        if (dir<0 || dir>65535) {
                debug_printf (VERBOSE_ERR,"Invalid address %d",dir);
                return;
        }



        sprintf (string_poke,"0");

        menu_ventana_scanf("Poke Value",string_poke,4);

        valor_poke=parse_string_to_number(string_poke);

        if (valor_poke<0 || valor_poke>255) {
                debug_printf (VERBOSE_ERR,"Invalid value %d",valor_poke);
                return;
        }


        sprintf (string_veces,"1");

        menu_ventana_scanf("How many bytes?",string_veces,6);

        veces=parse_string_to_number(string_veces);

        if (veces<1 || veces>65536) {
                debug_printf (VERBOSE_ERR,"Invalid quantity %d",veces);
                return;
        }


        for (;veces;veces--,dir++) {

					util_poke(bank,dir,valor_poke);

				}

}

//Ultima direccion pokeada
int last_debug_poke_dir=16384;

void menu_debug_poke(MENU_ITEM_PARAMETERS)
{

        int valor_poke,dir,veces;

        char string_poke[4];
        char string_dir[8];
	char string_veces[6];

        sprintf (string_dir,"%d",last_debug_poke_dir);

        menu_ventana_scanf("Address",string_dir,8);

        dir=parse_string_to_number(string_dir);

        if ( (dir<0 || dir>65535) && MACHINE_IS_SPECTRUM) {
                debug_printf (VERBOSE_ERR,"Invalid address %d",dir);
                return;
        }

				last_debug_poke_dir=dir;

        sprintf (string_poke,"0");

        menu_ventana_scanf("Poke Value",string_poke,4);

        valor_poke=parse_string_to_number(string_poke);

        if (valor_poke<0 || valor_poke>255) {
                debug_printf (VERBOSE_ERR,"Invalid value %d",valor_poke);
                return;
        }


	sprintf (string_veces,"1");

	menu_ventana_scanf("How many bytes?",string_veces,6);

	veces=parse_string_to_number(string_veces);

	if (veces<1 || veces>65536) {
                debug_printf (VERBOSE_ERR,"Invalid quantity %d",veces);
		return;
	}


	for (;veces;veces--,dir++) {

	        //poke_byte_no_time(dir,valor_poke);
					poke_byte_z80_moto(dir,valor_poke);

	}

}



void menu_debug_poke_pok_file(MENU_ITEM_PARAMETERS)
{

        char *filtros[2];

        filtros[0]="pok";
        filtros[1]=0;

	char pokfile[PATH_MAX];

        int ret;

        ret=menu_filesel("Select POK File",filtros,pokfile);

	//contenido
	//MAX_LINEAS_POK_FILE es maximo de lineas de pok file
	//normalmente la tabla de pokes sera menor que el numero de lineas en el archivo .pok
	struct s_pokfile tabla_pokes[MAX_LINEAS_POK_FILE];

	//punteros
	struct s_pokfile *punteros_pokes[MAX_LINEAS_POK_FILE];

	int i;
	for (i=0;i<MAX_LINEAS_POK_FILE;i++) punteros_pokes[i]=&tabla_pokes[i];


        if (ret==1) {

                cls_menu_overlay();
		int total=util_parse_pok_file(pokfile,punteros_pokes);

		if (total<1) {
			debug_printf (VERBOSE_ERR,"Error parsing POK file");
			return;
		}

		int j;
		for (j=0;j<total;j++) {
			debug_printf (VERBOSE_DEBUG,"menu poke index %d text %s bank %d address %d value %d value_orig %d",
				punteros_pokes[j]->indice_accion,
				punteros_pokes[j]->texto,
				punteros_pokes[j]->banco,
				punteros_pokes[j]->direccion,
				punteros_pokes[j]->valor,
				punteros_pokes[j]->valor_orig);
		}


		//Meter cada poke en un menu




        menu_item *array_menu_debug_pok_file;
        menu_item item_seleccionado;
        int retorno_menu;
	//Resetear siempre ultima linea = 0
	debug_pok_file_opcion_seleccionada=0;

	//temporal para mostrar todos los caracteres 0-255
	//int temp_conta=1;

        do {



		//Meter primer item de menu
		//truncar texto a 28 caracteres si excede de eso
		if (strlen(punteros_pokes[0]->texto)>28) punteros_pokes[0]->texto[28]=0;
                menu_add_item_menu_inicial_format(&array_menu_debug_pok_file,MENU_OPCION_NORMAL,NULL,NULL,"%s", punteros_pokes[0]->texto);


		//Luego recorrer array de pokes y cuando el numero de poke se incrementa, agregar
		int poke_anterior=0;

		int total_elementos=1;

		for (j=1;j<total;j++) {
			if (punteros_pokes[j]->indice_accion!=poke_anterior) {

				//temp para mostrar todos los caracteres 0-255
				//int kk;
				//for (kk=0;kk<strlen(punteros_pokes[j]->texto);kk++) {
				//	punteros_pokes[j]->texto[kk]=temp_conta++;
				//	if (temp_conta==256) temp_conta=1;
				//}

				poke_anterior=punteros_pokes[j]->indice_accion;
				//truncar texto a 28 caracteres si excede de eso
				if (strlen(punteros_pokes[j]->texto)>28) punteros_pokes[j]->texto[28]=0;
				menu_add_item_menu_format(array_menu_debug_pok_file,MENU_OPCION_NORMAL,NULL,NULL,"%s", punteros_pokes[j]->texto);

				total_elementos++;
				if (total_elementos==20) {
					debug_printf (VERBOSE_DEBUG,"Too many pokes to show on Window. Showing only first 20");
					menu_warn_message("Too many pokes to show on Window. Showing only first 20");
					break;
				}


			}
		}



                menu_add_item_menu(array_menu_debug_pok_file,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                //menu_add_item_menu(array_menu_debug_pok_file,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_debug_pok_file);

                retorno_menu=menu_dibuja_menu(&debug_pok_file_opcion_seleccionada,&item_seleccionado,array_menu_debug_pok_file,"Select Poke" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion

			//Hacer poke sabiendo la linea seleccionada. Desde ahi, ejecutar todos los pokes de dicha accion
			debug_printf (VERBOSE_DEBUG,"Doing poke/s from line %d",debug_pok_file_opcion_seleccionada);

			z80_byte banco;
			z80_int direccion;
			z80_byte valor;

			//buscar indice_accion
			int result_poke=0;
			for (j=0;j<total && result_poke==0;j++) {

				debug_printf (VERBOSE_DEBUG,"index %d looking %d current %d",j,debug_pok_file_opcion_seleccionada,punteros_pokes[j]->indice_accion);

				if (punteros_pokes[j]->indice_accion==debug_pok_file_opcion_seleccionada) {
					banco=punteros_pokes[j]->banco;
					direccion=punteros_pokes[j]->direccion;
					valor=punteros_pokes[j]->valor;
					debug_printf (VERBOSE_DEBUG,"Doing poke bank %d address %d value %d",banco,direccion,valor);
					result_poke=util_poke(banco,direccion,valor);
				}


                        //        cls_menu_overlay();

			}
			if (result_poke==0) menu_generic_message("Poke","OK. Poke applied");
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




        }

}












void menu_poke(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_poke;
        menu_item item_seleccionado;
        int retorno_menu;

        do {


                menu_add_item_menu_inicial_format(&array_menu_poke,MENU_OPCION_NORMAL,menu_debug_poke,NULL,"~~Poke");
                menu_add_item_menu_shortcut(array_menu_poke,'p');
                menu_add_item_menu_tooltip(array_menu_poke,"Poke address");
                menu_add_item_menu_ayuda(array_menu_poke,"Poke address for infinite lives, etc...");

		if (MACHINE_IS_SPECTRUM_128_P2_P2A || MACHINE_IS_ZXUNO_BOOTM_DISABLED) {
			menu_add_item_menu(array_menu_poke,"Poke 128~~k mode",MENU_OPCION_NORMAL,menu_debug_poke_128k,NULL);
			menu_add_item_menu_shortcut(array_menu_poke,'k');
			menu_add_item_menu_tooltip(array_menu_poke,"Poke bank & address");
			menu_add_item_menu_ayuda(array_menu_poke,"Poke bank & address");
		}

		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu(array_menu_poke,"Poke from .POK ~~file",MENU_OPCION_NORMAL,menu_debug_poke_pok_file,NULL);
			menu_add_item_menu_shortcut(array_menu_poke,'f');
			menu_add_item_menu_tooltip(array_menu_poke,"Poke reading .POK file");
			menu_add_item_menu_ayuda(array_menu_poke,"Poke reading .POK file");
		}


                menu_add_item_menu(array_menu_poke,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                //menu_add_item_menu(array_menu_poke,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_poke);

                retorno_menu=menu_dibuja_menu(&poke_opcion_seleccionada,&item_seleccionado,array_menu_poke,"Poke" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}






int menu_audio_draw_sound_wave_ycentro;

void menu_linea(int x,int y1,int y2,int color)
{

	int yorigen;
	int ydestino;


	//empezamos de menos a mas
	if (y1<y2) {
		yorigen=y1;
		ydestino=y2;
	}

	else {
		yorigen=y2;
		ydestino=y1;
	}


	for (;yorigen<=ydestino;yorigen++) {
		scr_putpixel_zoom(x,yorigen,color);
	}
}

#define SOUND_WAVE_X 1
#define SOUND_WAVE_Y 3
#define SOUND_WAVE_ANCHO 30
#define SOUND_WAVE_ALTO 15

int menu_sound_wave_llena=1;

char menu_audio_draw_sound_wave_valor_medio;
int menu_audio_draw_sound_wave_frecuencia_aproximada;

void menu_audio_draw_sound_wave(void)
{

	normal_overlay_texto_menu();



	int ancho=(SOUND_WAVE_ANCHO-2);
	int alto=(SOUND_WAVE_ALTO-4);
	int xorigen=(SOUND_WAVE_X+1);
	int yorigen=(SOUND_WAVE_Y+4);

	if (si_complete_video_driver() ) {
        	ancho *=8;
	        alto *=8;
        	xorigen *=8;
	        yorigen *=8;
	}


	//int ycentro=yorigen+alto/2;
	menu_audio_draw_sound_wave_ycentro=yorigen+alto/2;

	int x,y,lasty;


	//Para drivers de texto, borrar zona

	if (!si_complete_video_driver() ) {
	        for (x=xorigen;x<xorigen+ancho;x++) {
        	        for (y=yorigen;y<yorigen+alto;y++) {
				//putchar_menu_overlay(x,y,' ',0,7);
				//putchar_menu_overlay(x,y,' ',ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL);
				putchar_menu_overlay(x,y,' ',ESTILO_GUI_COLOR_WAVEFORM,ESTILO_GUI_PAPEL_NORMAL);
	                }
        	}
	}



	//Obtenemos antes valor medio total
	//Esto solo es necesario para dibujar onda llena

	//Obtenemos tambien cuantas veces cambia de signo (y por tanto, obtendremos frecuencia aproximada)
	int cambiossigno=0;
	int signoanterior=0;
	int signoactual=0;

	int audiomedio=0;

	char valor_sonido;


	z80_byte valor_sonido_sin_signo;
	z80_byte valor_anterior_sin_signo=0;

	int i;
	for (i=0;i<AUDIO_BUFFER_SIZE;i++) {
		valor_sonido=audio_buffer[i];
		audiomedio +=valor_sonido;

		valor_sonido_sin_signo=valor_sonido;

		if (valor_sonido_sin_signo>valor_anterior_sin_signo) signoactual=+1;
		if (valor_sonido_sin_signo<valor_anterior_sin_signo) signoactual=-1;

		valor_anterior_sin_signo=valor_sonido_sin_signo;

		if (signoactual!=signoanterior) {
			cambiossigno++;
			signoanterior=signoactual;
		}

	}

	//Calculo frecuencia aproximada
	menu_audio_draw_sound_wave_frecuencia_aproximada=((FRECUENCIA_SONIDO/AUDIO_BUFFER_SIZE)*cambiossigno)/2;

	//printf ("%d %d %d %d\n",FRECUENCIA_SONIDO,AUDIO_BUFFER_SIZE,cambiossigno,menu_audio_draw_sound_wave_frecuencia_aproximada);


	audiomedio /=AUDIO_BUFFER_SIZE;
	//printf ("valor medio: %d\n",audiomedio);
	menu_audio_draw_sound_wave_valor_medio=audiomedio;

	audiomedio=audiomedio*alto/256;

        //Lo situamos en el centro. Negativo hacia abajo (Y positiva)
        audiomedio=menu_audio_draw_sound_wave_ycentro-audiomedio;

	//printf ("valor medio en y: %d\n",audiomedio);
	//Fin Obtenemos antes valor medio




	int puntero_audio=0;
	char valor_audio;
	for (x=xorigen;x<xorigen+ancho;x++) {

		//Obtenemos valor medio de audio
		int valor_medio=0;

		//Calcular cuantos valores representa un pixel, teniendo en cuenta maximo buffer
		const int max_valores=AUDIO_BUFFER_SIZE/ancho;

		int valores=max_valores;
		for (;valores>0;valores--,puntero_audio++) {
			if (puntero_audio>=AUDIO_BUFFER_SIZE) {
				//por si el calculo no es correcto, salir.
				//esto no deberia suceder ya que el calculo de max_valores se hace en base al maximo
				cpu_panic("menu_audio_draw_sound_wave: pointer beyond AUDIO_BUFFER_SIZE");
			}
			valor_medio=valor_medio+audio_buffer[puntero_audio];

		}

		valor_medio=valor_medio/max_valores;

		valor_audio=valor_medio;

		//Lo escalamos a maximo alto

		y=valor_audio;
		y=valor_audio*alto/256;

		//Lo situamos en el centro. Negativo hacia abajo (Y positiva)
		y=menu_audio_draw_sound_wave_ycentro-y;


		//unimos valor anterior con actual con una linea vertical
		if (x!=xorigen) {
			if (si_complete_video_driver() ) {

				//Onda no llena
				if (!menu_sound_wave_llena) menu_linea(x,lasty,y,ESTILO_GUI_COLOR_WAVEFORM);

        			//dibujar la onda "llena", es decir, siempre contar desde centro
			        //el centro de la y de la onda es variable... se saca valor medio de todos los valores mostrados en pantalla

				//Onda llena
				else menu_linea(x,audiomedio,y,ESTILO_GUI_COLOR_WAVEFORM);



			}
		}

		lasty=y;

		//dibujamos valor actual
		if (si_complete_video_driver() ) {
			scr_putpixel_zoom(x,y,ESTILO_GUI_COLOR_WAVEFORM);
		}

		else {
			putchar_menu_overlay(x,y,'#',ESTILO_GUI_COLOR_WAVEFORM,ESTILO_GUI_PAPEL_NORMAL);
		}


	}

	//printf ("%d ",puntero_audio);

}

void menu_audio_espectro_sonido(MENU_ITEM_PARAMETERS)
{

	//Desactivamos interlace - si esta. Con interlace la forma de onda se dibuja encima continuamente, sin borrar
        z80_bit copia_video_interlaced_mode;
        copia_video_interlaced_mode.v=video_interlaced_mode.v;

	disable_interlace();

        menu_espera_no_tecla();
        menu_dibuja_ventana(SOUND_WAVE_X,SOUND_WAVE_Y-1,SOUND_WAVE_ANCHO,SOUND_WAVE_ALTO+3,"Waveform");

	//Damos un margen para escribir texto de tecla y valores average
	if (si_complete_video_driver() )
        	menu_escribe_linea_opcion(0,-1,1,"S: Change wave Shape");


        z80_byte acumulado;



        //Cambiamos funcion overlay de texto de menu
	//Se establece a la de funcion de onda + texto
        set_menu_overlay_function(menu_audio_draw_sound_wave);

				int valor_contador_segundo_anterior;

				valor_contador_segundo_anterior=contador_segundo;


        do {

          	//esto hara ejecutar esto 2 veces por segundo
                //if ( (contador_segundo%500) == 0 || menu_multitarea==0) {
								if ( ((contador_segundo%500) == 0 && valor_contador_segundo_anterior!=contador_segundo) || menu_multitarea==0) {
									valor_contador_segundo_anterior=contador_segundo;
									//printf ("Refrescando. contador_segundo=%d\n",contador_segundo);
                       if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();

			char buffer_texto_medio[40];
			sprintf (buffer_texto_medio,"Average level: %d",menu_audio_draw_sound_wave_valor_medio);
			menu_escribe_linea_opcion(1,-1,1,buffer_texto_medio);
			sprintf (buffer_texto_medio,"Average freq: %d Hz (%s)",
				menu_audio_draw_sound_wave_frecuencia_aproximada,get_note_name(menu_audio_draw_sound_wave_frecuencia_aproximada));
			menu_escribe_linea_opcion(2,-1,1,buffer_texto_medio);

                }

                menu_cpu_core_loop();
                acumulado=menu_da_todas_teclas();


                        //Hay tecla pulsada
                        if ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) !=MENU_PUERTO_TECLADO_NINGUNA ) {
                                int tecla=menu_get_pressed_key();
				//printf ("tecla: %c\n",tecla);
				if (tecla=='s') {
					menu_sound_wave_llena ^=1;
					menu_espera_no_tecla();
					acumulado=MENU_PUERTO_TECLADO_NINGUNA;
				}
			}



                //si no hay multitarea, esperar tecla y salir
                if (menu_multitarea==0) {
                        menu_espera_tecla();

                        acumulado=0;
                }

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA);

	//Restauramos modo interlace
	if (copia_video_interlaced_mode.v) enable_interlace();

       //restauramos modo normal de texto de menu
       set_menu_overlay_function(normal_overlay_texto_menu);


        cls_menu_overlay();

}





void menu_ay_registers_overlay(void)
{
        normal_overlay_texto_menu();

	char volumen[16],textovolumen[32],textotono[32];

	int total_chips=ay_retorna_numero_chips();

	int chip;

	int linea=0;

	for (chip=0;chip<total_chips;chip++) {


			menu_string_volumen(volumen,ay_3_8912_registros[chip][8]);
			sprintf (textovolumen,"Volume A: %s",volumen);
			menu_escribe_linea_opcion(linea++,-1,1,textovolumen);

			menu_string_volumen(volumen,ay_3_8912_registros[chip][9]);
			sprintf (textovolumen,"Volume B: %s",volumen);
			menu_escribe_linea_opcion(linea++,-1,1,textovolumen);

			menu_string_volumen(volumen,ay_3_8912_registros[chip][10]);
			sprintf (textovolumen,"Volume C: %s",volumen);
			menu_escribe_linea_opcion(linea++,-1,1,textovolumen);


			int freq_a=retorna_frecuencia(0,chip);
			int freq_b=retorna_frecuencia(1,chip);
			int freq_c=retorna_frecuencia(2,chip);
			sprintf (textotono,"Channel A:  %3s %7d Hz",get_note_name(freq_a),freq_a);
			menu_escribe_linea_opcion(linea++,-1,1,textotono);

			sprintf (textotono,"Channel B:  %3s %7d Hz",get_note_name(freq_b),freq_b);
			menu_escribe_linea_opcion(linea++,-1,1,textotono);

			sprintf (textotono,"Channel C:  %3s %7d Hz",get_note_name(freq_c),freq_c);
			menu_escribe_linea_opcion(linea++,-1,1,textotono);

			                        //Frecuencia ruido
                        int freq_temp=ay_3_8912_registros[chip][6] & 31;
                        //printf ("Valor registros ruido : %d Hz\n",freq_temp);
                        freq_temp=freq_temp*16;

                        //controlamos divisiones por cero
                        if (!freq_temp) freq_temp++;

                        int freq_ruido=FRECUENCIA_NOISE/freq_temp;

                        sprintf (textotono,"Frequency Noise: %6d Hz",freq_ruido);
                        menu_escribe_linea_opcion(linea++,-1,1,textotono);


			//Envelope

                        freq_temp=ay_3_8912_registros[chip][11]+256*(ay_3_8912_registros[chip][12] & 0xFF);


                        //controlamos divisiones por cero
                        if (!freq_temp) freq_temp++;
                        int freq_envelope=FRECUENCIA_ENVELOPE/freq_temp;

                        //sprintf (textotono,"Freq Envelope(*10): %5d Hz",freq_envelope);

			int freq_env_10=freq_envelope/10;
			int freq_env_decimal=freq_envelope-(freq_env_10*10);

			sprintf (textotono,"Freq Envelope:   %4d.%1d Hz",freq_env_10,freq_env_decimal);

                        menu_escribe_linea_opcion(linea++,-1,1,textotono);

			char envelope_name[32];
			z80_byte env_type=ay_3_8912_registros[chip][13] & 0x0F;
			return_envelope_name(env_type,envelope_name);
			sprintf (textotono,"Env.: %2d (%s)",env_type,envelope_name);
                        menu_escribe_linea_opcion(linea++,-1,1,textotono);

			sprintf (textotono,"Tone Channels:  %c %c %c",
				( (ay_3_8912_registros[chip][7]&1)==0 ? 'A' : ' '),
				( (ay_3_8912_registros[chip][7]&2)==0 ? 'B' : ' '),
				( (ay_3_8912_registros[chip][7]&4)==0 ? 'C' : ' '));
			menu_escribe_linea_opcion(linea++,-1,1,textotono);

                        sprintf (textotono,"Noise Channels: %c %c %c",
                                ( (ay_3_8912_registros[chip][7]&8)==0  ? 'A' : ' '),
                                ( (ay_3_8912_registros[chip][7]&16)==0 ? 'B' : ' '),
                                ( (ay_3_8912_registros[chip][7]&32)==0 ? 'C' : ' '));
                        menu_escribe_linea_opcion(linea++,-1,1,textotono);

	}

}



void menu_ay_registers(MENU_ITEM_PARAMETERS)
{
        menu_espera_no_tecla();

				if (!menu_multitarea) {
					menu_warn_message("This menu item needs multitask enabled");
					return;
				}

        if (ay_retorna_numero_chips()==1) menu_dibuja_ventana(1,5,30,14,"AY Registers");
	else menu_dibuja_ventana(1,0,30,24,"AY Registers");

        z80_byte acumulado;


        //Cambiamos funcion overlay de texto de menu
        //Se establece a la de funcion de onda + texto
        set_menu_overlay_function(menu_ay_registers_overlay);

				int valor_contador_segundo_anterior;

				valor_contador_segundo_anterior=contador_segundo;


   do {

                //esto hara ejecutar esto 2 veces por segundo
                //if ( (contador_segundo%500) == 0 || menu_multitarea==0) {
									if ( ((contador_segundo%500) == 0 && valor_contador_segundo_anterior!=contador_segundo) || menu_multitarea==0) {
		valor_contador_segundo_anterior=contador_segundo;

										//printf ("Refrescando. contador_segundo=%d\n",contador_segundo);
                       if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();


                }

                menu_cpu_core_loop();
                acumulado=menu_da_todas_teclas();




               //si no hay multitarea, esperar tecla y salir
                if (menu_multitarea==0) {
                        menu_espera_tecla();

                        acumulado=0;
                }

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA);


       //restauramos modo normal de texto de menu
       set_menu_overlay_function(normal_overlay_texto_menu);


        cls_menu_overlay();

}

void menu_ay_pianokeyboard_insert_inverse(char *origen_orig, int indice)
{
	char cadena_temporal[40];

	char *destino;

	destino=cadena_temporal;

	char *origen;
	origen=origen_orig;

	int i;

	for (i=0;*origen;origen++,i++) {
			if (i==indice) {
				*destino++='~';
				*destino++='~';
			}

			*destino++=*origen;
	}

	*destino=0;

	//copiar a cadena original
	strcpy(origen_orig,cadena_temporal);
}

#define PIANO_GRAPHIC_BASE_X 9

int piano_graphic_base_y=0;

void menu_ay_pianokeyboard_draw_graphical_piano_draw_pixel_zoom(int x,int y,int color)
{
	#define PIANO_ZOOM 3

	int offsetx=PIANO_GRAPHIC_BASE_X*8+12;
	int offsety=piano_graphic_base_y*8+18;

	x=offsetx+x*PIANO_ZOOM;
	y=offsety+y*PIANO_ZOOM;

	int xorig=x;
	int zx=0;
	int zy=0;

	for (zy=0;zy<PIANO_ZOOM;zy++) {
		x=xorig;
		for (zx=0;zx<PIANO_ZOOM;zx++) {
			scr_putpixel_zoom(x,y,color);

			x++;

		}
		y++;
	}

}

//Basandome en coordenadas basicas sin zoom
void menu_ay_pianokeyboard_draw_graphical_piano_draw_line(int x, int y, int stepx, int stepy, int length, int color)
{

	for (;length>0;length--) {
			menu_ay_pianokeyboard_draw_graphical_piano_draw_pixel_zoom(x,y,color);
			x +=stepx;
			y +=stepy;
	}

}

void menu_ay_piano_graph_dibujar_negra(int x, int y,int color)
{
 int alto=4;

	for (alto=0;alto<4;alto++) {
		menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x, y, +1, 0, 3, color);
		y++;
	}
}


//Como C, F
void menu_ay_piano_graph_dibujar_blanca_izquierda(int x, int y,int color)
{
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x, y, 0, +1, 7, color);
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x+1, y, 0, +1, 7, color);
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x+2, y+4, 0, +1, 3, color);
}

//Como D, G, A
void menu_ay_piano_graph_dibujar_blanca_media(int x, int y,int color)
{

	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x, y+4, 0, +1, 3, color);
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x+1, y, 0, +1, 7, color);
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x+2, y+4, 0, +1, 3, color);
}


//Como E, B
void menu_ay_piano_graph_dibujar_blanca_derecha(int x, int y,int color)
{
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x, y+4, 0, +1, 3, color);
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x+1, y, 0, +1, 7, color);
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x+2, y, 0, +1, 7, color);
}

void menu_ay_pianokeyboard_draw_graphical_piano(int linea GCC_UNUSED,int canal,char *note)
{
	/*
	Teclado:
	0123456789012345678901234567890

	xxxxxxxxxxxxxxxxxxxxxxxxxxxxx         0
	x  xxx xxx  x  xxx xxx xxx  x         1
	x  xxx xxx  x  xxx xxx xxx  x         2
	x  xxx xxx  x  xxx xxx xxx  x         3
	x  xxx xxx  x  xxx xxx xxx  x         4
	x   x   x   x   x   x   x   x         5
	x   x   x   x   x   x   x   x         6
	x   x   x   x   x   x   x   x         7

	0123456789012345678901234567890

    C   D   E   F   G   A   B
Altura, para 2 chips de sonido (6 canales), tenemos maximo 192/6=32
32 de alto maximo, podemos hacer zoom x3 del esquema basico, por tanto tendriamos 8x3x6=144 de alto con 2 chips de sonido


	*/

	//scr_putpixel_zoom(x,y,ESTILO_GUI_TINTA_NORMAL);

	int ybase=0; //TODO: depende de linea de entrada

	//printf ("linea: %d\n",linea);

	//temp
	ybase +=8*canal;

	//Recuadro en blanco
	int x,y;
	for (x=0;x<29;x++) {
		for (y=ybase;y<ybase+8;y++) {
			menu_ay_pianokeyboard_draw_graphical_piano_draw_pixel_zoom(x,y,7);
		}
	}

	//Linea superior
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(0, ybase+0, +1, 0, 29, 0);

	//Linea vertical izquierda
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(0, ybase+0, 0, +1, 8, 0);

	//Linea vertical derecha
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(28, ybase+0, 0, +1, 8, 0);

	//6 separaciones verticales pequeñas
	int i;
	x=4;
	for (i=0;i<6;i++) {
		menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x, ybase+5, 0, +1, 3, 0);
		x+=4;
	}

	//Linea vertical central
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(12, ybase+0, 0, +1, 8, 0);

	//Y ahora las 5 negras
	x=3;
	for (i=0;i<5;i++) {
		menu_ay_piano_graph_dibujar_negra(x,ybase+1,0);
		/*
		for (y=1;y<=4;y++) {
			menu_ay_pianokeyboard_draw_graphical_piano_draw_line(x, y, +1, 0, 3, 0);
		}
		*/

		x+=4;
		if (i==1) x+=4;  //Saltar posicion donde iria la "tercera" negra
	}


	//Dibujar la linea inferior. Realmente la linea inferior es siempre la linea superior del siguiente canal, excepto en el ultimo canal
	menu_ay_pianokeyboard_draw_graphical_piano_draw_line(0, ybase+8, +1, 0, 29, 0);

	//Y ahora destacar la que se pulsa
	char letra_nota=note[0];
	int es_negra=0;
	if (note[1]=='#') es_negra=1;

	if (es_negra) {

		//determinar posicion x
		switch (letra_nota)
		{
			case 'C':
				x=3;
			break;

			case 'D':
				x=7;
			break;

			case 'F':
				x=15;
			break;

			case 'G':
				x=19;
			break;

			case 'A':
				x=23;
			break;

			default:
				//por si acaso
				x=-1;
			break;
		}
		if (x!=-1) menu_ay_piano_graph_dibujar_negra(x,ybase+1,1); //Color 1 para probar
	}

	else {
		//blancas
		switch (letra_nota)
		{
			case 'C':
			 menu_ay_piano_graph_dibujar_blanca_izquierda(1, ybase+1,1);
			break;

			case 'D':
			//Como D, G, A
			 menu_ay_piano_graph_dibujar_blanca_media(5, ybase+1,1);
			break;

			case 'E':
				menu_ay_piano_graph_dibujar_blanca_derecha(9, ybase+1,1);
			break;

			case 'F':
			 menu_ay_piano_graph_dibujar_blanca_izquierda(13, ybase+1,1);
			break;

			case 'G':
			//Como D, G, A
			 menu_ay_piano_graph_dibujar_blanca_media(17, ybase+1,1);
			break;

			case 'A':
			//Como D, G, A
			 menu_ay_piano_graph_dibujar_blanca_media(21, ybase+1,1);
			break;

			case 'B':
				menu_ay_piano_graph_dibujar_blanca_derecha(25, ybase+1,1);
			break;
		}

	}


}

void menu_ay_pianokeyboard_draw_text_piano(int linea,int canal GCC_UNUSED,char *note)
{

	//Forzar a mostrar atajos
	z80_bit antes_menu_writing_inverse_color;
	antes_menu_writing_inverse_color.v=menu_writing_inverse_color.v;

	menu_writing_inverse_color.v=1;

	char linea_negras[40];
	char linea_blancas[40];
												//012345678901
	sprintf (linea_negras, " # #  # # #");
	sprintf (linea_blancas,"C D EF G A B");

	if (note==NULL || note[0]==0) {
	}

	else {
		//Marcar tecla piano pulsada con ~~
		//Interpretar Nota viene como C#4 o C4 por ejemplo
		char letra_nota=note[0];
		int es_negra=0;
		if (note[1]=='#') es_negra=1;

		//TODO: mostramos la octava?

		//Linea negras
		if (es_negra) {
				int indice_negra_marcar=0;
				switch (letra_nota)
				{
					case 'C':
						indice_negra_marcar=1;
					break;

					case 'D':
						indice_negra_marcar=3;
					break;

					case 'F':
						indice_negra_marcar=6;
					break;

					case 'G':
						indice_negra_marcar=8;
					break;

					case 'A':
						indice_negra_marcar=10;
					break;
				}

				//Reconstruimos la cadena introduciendo ~~donde indique el indice
				menu_ay_pianokeyboard_insert_inverse(linea_negras,indice_negra_marcar);
		}

		//Linea blancas
		else {
			int indice_blanca_marcar=0;
			//												//012345678901
			//	sprintf (linea_negras, " # #  # # #");
			//	sprintf (linea_blancas,"C D EF G A B");
			switch (letra_nota)
			{
			  case 'C':
			    indice_blanca_marcar=0;
			  break;

			  case 'D':
			    indice_blanca_marcar=2;
			  break;

			  case 'E':
			    indice_blanca_marcar=4;
			  break;

			  case 'F':
			    indice_blanca_marcar=5;
			  break;

			  case 'G':
			    indice_blanca_marcar=7;
			  break;

			  case 'A':
			    indice_blanca_marcar=9;
			  break;

			  case 'B':
			    indice_blanca_marcar=11;
			  break;
			}

			//Reconstruimos la cadena introduciendo ~~donde indique el indice
			menu_ay_pianokeyboard_insert_inverse(linea_blancas,indice_blanca_marcar);
		}
	}

	menu_escribe_linea_opcion(linea++,-1,1,linea_negras);
	menu_escribe_linea_opcion(linea++,-1,1,linea_blancas);
	menu_writing_inverse_color.v=antes_menu_writing_inverse_color.v;

}


//Si se muestra piano grafico en drivers grafico. Si no, muestra piano de texto en drivers graficos
z80_bit setting_mostrar_ay_piano_grafico={1};


//Dice si se muestra piano grafico o de texto.
//Si es un driver de solo texto, mostrar texto
//Si es un driver grafico y setting dice que lo mostremos en texto, mostrar texto
//Si nada de lo demas, mostrar grafico
int si_mostrar_ay_piano_grafico(void)
{
	if (!si_complete_video_driver()) return 0;

	if (!setting_mostrar_ay_piano_grafico.v) return 0;

	return 1;

}

void menu_ay_pianokeyboard_draw_piano(int linea,int canal,char *note)
{
	if (!si_mostrar_ay_piano_grafico()) {
		menu_ay_pianokeyboard_draw_text_piano(linea,canal,note);
	}
	else {
		menu_ay_pianokeyboard_draw_graphical_piano(linea,canal,note);
	}
}


void menu_ay_pianokeyboard_overlay(void)
{
        normal_overlay_texto_menu();

	//char volumen[16],textovolumen[32],textotono[32];

	int total_chips=ay_retorna_numero_chips();

	int chip;

	int linea=1;

	int canal=0;

	for (chip=0;chip<total_chips;chip++) {


			int freq_a=retorna_frecuencia(0,chip);
			int freq_b=retorna_frecuencia(1,chip);
			int freq_c=retorna_frecuencia(2,chip);

			char nota_a[4];
			sprintf(nota_a,"%s",get_note_name(freq_a) );

			char nota_b[4];
			sprintf(nota_b,"%s",get_note_name(freq_b) );

			char nota_c[4];
			sprintf(nota_c,"%s",get_note_name(freq_c) );

			//Si canales no suenan como tono, o volumen 0 meter cadena vacia en nota
			if (ay_3_8912_registros[chip][7]&1 || ay_3_8912_registros[chip][8]==0) nota_a[0]=0;
			if (ay_3_8912_registros[chip][7]&2 || ay_3_8912_registros[chip][9]==0) nota_b[0]=0;
			if (ay_3_8912_registros[chip][7]&4 || ay_3_8912_registros[chip][10]==0) nota_c[0]=0;


			menu_ay_pianokeyboard_draw_piano(linea,canal,nota_a);
			linea+=3;
			canal++;

			menu_ay_pianokeyboard_draw_piano(linea,canal,nota_b);
			linea+=3;
			canal++;

			menu_ay_pianokeyboard_draw_piano(linea,canal,nota_c);
			linea+=3;
			canal++;

	}

}



void menu_ay_pianokeyboard(MENU_ITEM_PARAMETERS)
{
        menu_espera_no_tecla();

				if (!menu_multitarea) {
					menu_warn_message("This menu item needs multitask enabled");
					return;
				}

				if (!si_mostrar_ay_piano_grafico()) {
					//Dibujar ay piano con texto
        	if (ay_retorna_numero_chips()==1) menu_dibuja_ventana(9,7,14,11,"AY Piano");
					else menu_dibuja_ventana(9,2,14,20,"AY Piano");
				}
				//#define PIANO_GRAPHIC_BASE_X 7
				//#define PIANO_GRAPHIC_BASE_Y 7
				else {
					//Dibujar ay piano con grafico
					if (ay_retorna_numero_chips()==1) {
						piano_graphic_base_y=5;
						menu_dibuja_ventana(PIANO_GRAPHIC_BASE_X,piano_graphic_base_y,14,13,"AY Piano");
					}
					else {
						piano_graphic_base_y=1;
						menu_dibuja_ventana(PIANO_GRAPHIC_BASE_X,piano_graphic_base_y,14,22,"AY Piano");
					}
				}

        z80_byte acumulado;


        //Cambiamos funcion overlay de texto de menu
        //Se establece a la de funcion de piano + texto
        set_menu_overlay_function(menu_ay_pianokeyboard_overlay);


				int valor_contador_segundo_anterior;

valor_contador_segundo_anterior=contador_segundo;

   do {

                //esto hara ejecutar esto 2 veces por segundo
                //if ( (contador_segundo%500) == 0 || menu_multitarea==0) {
								if ( ((contador_segundo%500) == 0 && valor_contador_segundo_anterior!=contador_segundo) || menu_multitarea==0) {
										valor_contador_segundo_anterior=contador_segundo;

										//printf ("Refrescando. contador_segundo=%d\n",contador_segundo);

                       if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();


                }

                menu_cpu_core_loop();
                acumulado=menu_da_todas_teclas();




               //si no hay multitarea, esperar tecla y salir
                if (menu_multitarea==0) {
                        menu_espera_tecla();

                        acumulado=0;
                }

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA);


       //restauramos modo normal de texto de menu
       set_menu_overlay_function(normal_overlay_texto_menu);


        cls_menu_overlay();

}



void menu_aofile_insert(MENU_ITEM_PARAMETERS)
{

	if (aofile_inserted.v==0) {
		init_aofile();

		//Si todo ha ido bien
		if (aofile_inserted.v) {
			menu_generic_message_format("File information","%s\n%s\n\n%s",
			last_message_helper_aofile_vofile_file_format,last_message_helper_aofile_vofile_bytes_minute_audio,last_message_helper_aofile_vofile_util);
		}

	}

        else if (aofile_inserted.v==1) {
                close_aofile();
        }

}

int menu_aofile_cond(void)
{
	if (aofilename!=NULL) return 1;
	else return 0;
}

void menu_aofile(MENU_ITEM_PARAMETERS)
{

	aofile_inserted.v=0;


        char *filtros[3];

#ifdef USE_SNDFILE
        filtros[0]="rwa";
        filtros[1]="wav";
        filtros[2]=0;
#else
        filtros[0]="rwa";
        filtros[1]=0;
#endif


        if (menu_filesel("Select Audio File",filtros,aofilename_file)==1) {

       	        if (si_existe_archivo(aofilename_file)) {

               	        if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) {
				aofilename=NULL;
				return;
			}

       	        }

                aofilename=aofilename_file;


        }

	else {
		aofilename=NULL;
	}


}

void menu_audio_beeper_real (MENU_ITEM_PARAMETERS)
{
	beeper_real_enabled ^=1;
}

void menu_audio_volume(MENU_ITEM_PARAMETERS)
{
        char string_perc[4];

        sprintf (string_perc,"%d",audiovolume);


        menu_ventana_scanf("Volume in %",string_perc,4);

        int v=parse_string_to_number(string_perc);

	if (v>100 || v<0) {
		debug_printf (VERBOSE_ERR,"Invalid volume value");
		return;
	}

	audiovolume=v;
}


int menu_cond_allow_write_rom(void)
{

	if (superupgrade_enabled.v) return 0;
	if (dandanator_enabled.v) return 0;

	if (MACHINE_IS_INVES) return 0;
	if (MACHINE_IS_SPECTRUM_16_48) return 1;
	if (MACHINE_IS_ZX8081) return 1;
	if (MACHINE_IS_ACE) return 1;
	if (MACHINE_IS_SAM) return 1;

	return 0;

}

void menu_hardware_allow_write_rom(MENU_ITEM_PARAMETERS)
{
	if (allow_write_rom.v) {
		reset_poke_byte_function_writerom();
                allow_write_rom.v=0;

	}

	else {
		set_poke_byte_function_writerom();
		allow_write_rom.v=1;
	}
}



void menu_audio_beep_filter_on_rom_save(MENU_ITEM_PARAMETERS)
{
	output_beep_filter_on_rom_save.v ^=1;
}


void menu_audio_beep_alter_volume(MENU_ITEM_PARAMETERS)
{
	output_beep_filter_alter_volume.v ^=1;
}


void menu_audio_beep_volume(MENU_ITEM_PARAMETERS)
{

        char string_vol[4];

        sprintf (string_vol,"%d",output_beep_filter_volume);


        menu_ventana_scanf("Volume (0-127)",string_vol,4);

        int v=parse_string_to_number(string_vol);

        if (v>127 || v<0) {
                debug_printf (VERBOSE_ERR,"Invalid volume value");
                return;
        }

        output_beep_filter_volume=v;
}

void menu_audio_turbosound(MENU_ITEM_PARAMETERS)
{
	if (turbosound_enabled.v) turbosound_disable();
	else turbosound_enable();
}


/*

void menu_ay_player_next_track(MENU_ITEM_PARAMETERS)
{

	ay_player_next_track();

}
*/

void menu_ay_player_load(MENU_ITEM_PARAMETERS)
{
	char *filtros[2];

	filtros[0]="ay";

	filtros[1]=0;

	//guardamos directorio actual
	char directorio_actual[PATH_MAX];
	getcwd(directorio_actual,PATH_MAX);

	//Obtenemos directorio de ultimo archivo
	//si no hay directorio, vamos a rutas predefinidas
	if (last_ay_file[0]==0) menu_chdir_sharedfiles();

	else {
					char directorio[PATH_MAX];
					util_get_dir(last_ay_file,directorio);
					//printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

					//cambiamos a ese directorio, siempre que no sea nulo
					if (directorio[0]!=0) {
									debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
									menu_filesel_chdir(directorio);
					}
	}


	int ret;

	ret=menu_filesel("Select AY File",filtros,last_ay_file);
	//volvemos a directorio inicial
	menu_filesel_chdir(directorio_actual);


	if (ret==1) {


		ay_player_load_and_play(last_ay_file);

	}
}

/*
void menu_ay_player_exit_tracker(MENU_ITEM_PARAMETERS)
{
	ay_player_stop_player();
}
*/

//Retorna indice a string teniendo en cuenta maximo en pantalla e incrementando en 1
int menu_ay_player_get_continuous_string(int indice_actual,int max_length,char *string,int *retardo)
{

	if ( (*retardo)<10 ) {
		(*retardo)++;
		return 0;
	}

	int longitud=strlen(&string[indice_actual]);
	if (longitud<=max_length) {
		indice_actual=0;
		*retardo=0;
	}
	else {
		indice_actual++;
	}

	return indice_actual;
}

void menu_ay_player_player_dibuja_ventana(void)
{
	menu_dibuja_ventana(0,4,32,16,"AY Player");
}

void menu_ay_player_player(MENU_ITEM_PARAMETERS)
{



        char textoplayer[40];

        menu_espera_no_tecla();

				if (!menu_multitarea) {
					menu_warn_message("This menu item needs multitask enabled");
					return;
				}

        menu_ay_player_player_dibuja_ventana();

        z80_byte acumulado;

        //char dumpassembler[32];

        //Empezar con espacio
        //dumpassembler[0]=' ';

				int contador_string_author=0;
				int contador_string_track_name=0;
				int contador_string_misc=0;

 				int retardo_song_name=0;
				int retardo_author=0;
				int retardo_misc=0;



				int valor_contador_segundo_anterior;

				valor_contador_segundo_anterior=contador_segundo;

int mostrar_player;

int mostrar_antes_player=-1;

//Forzar a mostrar atajos
z80_bit antes_menu_writing_inverse_color;
antes_menu_writing_inverse_color.v=menu_writing_inverse_color.v;

menu_writing_inverse_color.v=1;

        do {


                //esto hara ejecutar esto 2 veces por segundo
                if ( ((contador_segundo%500) == 0 && valor_contador_segundo_anterior!=contador_segundo) || menu_multitarea==0) {
												valor_contador_segundo_anterior=contador_segundo;
                        //contador_segundo_anterior=contador_segundo;
/*
29 ancho

Track: 255/255  (01:00/03:50)
[Track name]
Author
[author]
Misc
[misc]

[P]rev [S]top [N]ext
[L]oad
*/


mostrar_player=1;
if (audio_ay_player_mem==NULL) mostrar_player=0;
if (ay_player_playing.v==0) mostrar_player=0;

				//Si hay cambio
				if (mostrar_player!=mostrar_antes_player) menu_ay_player_player_dibuja_ventana();

				mostrar_antes_player=mostrar_player;


                        int linea=0;
                        //int opcode;

                        //unsigned int sumatotal;
                        //sumatotal=util_stats_sum_all_counters();
                        //sprintf (textostats,"Total opcodes run: %u",sumatotal);
                        //menu_escribe_linea_opcion(linea++,-1,1,textostats);


                        //menu_escribe_linea_opcion(linea++,-1,1,"Most used op. for each preffix");
/*
	//Si es infinito, no saltar nunca a siguiente cancion
	if (ay_song_length==0) return;
	ay_song_length_counter++;

	ay_player_pista_actual
*/

		if (mostrar_player) {

			z80_byte minutos,segundos,minutos_total,segundos_total;
			minutos=ay_song_length_counter/60/50;
			segundos=(ay_song_length_counter/50)%60;
			minutos_total=ay_song_length/60/50;
			segundos_total=(ay_song_length/50)%60;

//printf ("segundo. contador segundo: %d\n",contador_segundo);

			sprintf (textoplayer,"Track: %03d/%03d  (%02d:%02d/%02d:%02d)",ay_player_pista_actual,ay_player_total_songs(),minutos,segundos,minutos_total,segundos_total);
			menu_escribe_linea_opcion(linea++,-1,1,textoplayer);


			strncpy(textoplayer,&ay_player_file_song_name[contador_string_track_name],28);
			textoplayer[28]=0;
			menu_escribe_linea_opcion(linea++,-1,1,textoplayer);
			contador_string_track_name=menu_ay_player_get_continuous_string(contador_string_track_name,28,ay_player_file_song_name,&retardo_song_name);

			menu_escribe_linea_opcion(linea++,-1,1,"Author");
			strncpy(textoplayer,&ay_player_file_author[contador_string_author],28);
			textoplayer[28]=0;
			menu_escribe_linea_opcion(linea++,-1,1,textoplayer);
			contador_string_author=menu_ay_player_get_continuous_string(contador_string_author,28,ay_player_file_author,&retardo_author);

			menu_escribe_linea_opcion(linea++,-1,1,"Misc");
			strncpy(textoplayer,&ay_player_file_misc[contador_string_misc],28);
			textoplayer[28]=0;
			menu_escribe_linea_opcion(linea++,-1,1,textoplayer);
			contador_string_misc=menu_ay_player_get_continuous_string(contador_string_misc,28,ay_player_file_misc,&retardo_misc);

			linea++;

			menu_escribe_linea_opcion(linea++,-1,1,"~~Prev ~~Stop ~~Next");

			sprintf(textoplayer,"~~Repeat: %s  ~~Exit end: %s",
				(ay_player_repeat_file.v ? "Yes" : "No"),(ay_player_exit_emulator_when_finish.v ? "Yes" : "No") );
			menu_escribe_linea_opcion(linea++,-1,1,textoplayer);

			if (ay_player_limit_infinite_tracks==0) sprintf(textoplayer,"Length ~~infinite tracks: inf");
			else sprintf(textoplayer,"Length ~~infinite tracks: %d s",ay_player_limit_infinite_tracks/50);
			menu_escribe_linea_opcion(linea++,-1,1,textoplayer);

			if (ay_player_limit_any_track==0) sprintf(textoplayer,"Length ~~any track: No limit");
			else sprintf(textoplayer,"Length ~~any track: %d s",ay_player_limit_any_track/50);
			menu_escribe_linea_opcion(linea++,-1,1,textoplayer);

			sprintf(textoplayer,"~~CPC mode: %s",(ay_player_cpc_mode.v ? "Yes" : "No"));
			menu_escribe_linea_opcion(linea++,-1,1,textoplayer);

			linea++;

}

menu_escribe_linea_opcion(linea++,-1,1,"~~Load");

			//ay_player_file_author_misc[1024];
			//ay_player_file_song_name[1024];


        	if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();

    	}

                menu_cpu_core_loop();
                acumulado=menu_da_todas_teclas();


								//Hay tecla pulsada
								if ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) !=MENU_PUERTO_TECLADO_NINGUNA ) {
												int tecla=menu_get_pressed_key();
												//printf ("tecla: %c\n",tecla);

												if (mostrar_player) {
												if (tecla=='n') {
													ay_player_next_track();
													contador_string_author=contador_string_track_name=contador_string_misc=retardo_song_name=retardo_author=retardo_misc=0;
													menu_espera_no_tecla();
													acumulado=MENU_PUERTO_TECLADO_NINGUNA;

												}

												if (tecla=='p') {
													ay_player_previous_track();
													contador_string_author=contador_string_track_name=contador_string_misc=retardo_song_name=retardo_author=retardo_misc=0;
													menu_espera_no_tecla();
													acumulado=MENU_PUERTO_TECLADO_NINGUNA;

												}

												if (tecla=='s') {
													ay_player_stop_player();
													contador_string_author=contador_string_track_name=contador_string_misc=retardo_song_name=retardo_author=retardo_misc=0;
													menu_espera_no_tecla();
													acumulado=MENU_PUERTO_TECLADO_NINGUNA;
												}

												if (tecla=='r') {
													ay_player_repeat_file.v ^=1;
													menu_espera_no_tecla();
													acumulado=MENU_PUERTO_TECLADO_NINGUNA;
												}

												if (tecla=='e') {
													ay_player_exit_emulator_when_finish.v ^=1;
													menu_espera_no_tecla();
													acumulado=MENU_PUERTO_TECLADO_NINGUNA;
												}

												if (tecla=='c') {
													ay_player_cpc_mode.v ^=1;
													audio_ay_player_play_song(ay_player_pista_actual);
													menu_espera_no_tecla();
													acumulado=MENU_PUERTO_TECLADO_NINGUNA;
												}

												if (tecla=='i') {
													menu_espera_no_tecla(); //Si no pongo esto, luego al borrar pantalla, no borra bien del todo
													//por ejemplo, si estamos en maquina 128k, se superpone el menu de la rom del 128k... quiza porque no limpia el putpixel cache??
													cls_menu_overlay();

													char string_length[5];
													sprintf(string_length,"%d",ay_player_limit_infinite_tracks/50);

									        menu_ventana_scanf("Length (0-1310)",string_length,5);
													int l=parse_string_to_number(string_length);

													if (l<0 || l>1310) {
														menu_error_message("Invalid length value");
													}

													else ay_player_limit_infinite_tracks=l*50;

													menu_espera_no_tecla();
													cls_menu_overlay();
													acumulado=MENU_PUERTO_TECLADO_NINGUNA;
													menu_ay_player_player_dibuja_ventana();
												}


												if (tecla=='a') {
													menu_espera_no_tecla(); //Si no pongo esto, luego al borrar pantalla, no borra bien del todo
													//por ejemplo, si estamos en maquina 128k, se superpone el menu de la rom del 128k... quiza porque no limpia el putpixel cache??
													cls_menu_overlay();

													char string_length[5];
													sprintf(string_length,"%d",ay_player_limit_any_track/50);

													menu_ventana_scanf("Length (0-1310)",string_length,5);
													int l=parse_string_to_number(string_length);

													if (l<0 || l>1310) {
														menu_error_message("Invalid length value");
													}

													else ay_player_limit_any_track=l*50;

													menu_espera_no_tecla();
													cls_menu_overlay();
													acumulado=MENU_PUERTO_TECLADO_NINGUNA;
													menu_ay_player_player_dibuja_ventana();
												}

											}

											if (tecla=='l') {
												menu_espera_no_tecla(); //Si no pongo esto, luego al borrar pantalla, no borra bien del todo
												//por ejemplo, si estamos en maquina 128k, se superpone el menu de la rom del 128k... quiza porque no limpia el putpixel cache??
												cls_menu_overlay();
												menu_ay_player_load(0);
												contador_string_author=contador_string_track_name=contador_string_misc=retardo_song_name=retardo_author=retardo_misc=0;
												menu_espera_no_tecla();
												cls_menu_overlay();
												acumulado=MENU_PUERTO_TECLADO_NINGUNA;
												menu_ay_player_player_dibuja_ventana();
											}

								}


                //si no hay multitarea, esperar tecla y salir
                if (menu_multitarea==0) {
                        menu_espera_tecla();

                        acumulado=0;
                }

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA);

        cls_menu_overlay();
				menu_espera_no_tecla();


				menu_writing_inverse_color.v=antes_menu_writing_inverse_color.v;

}


//menu audio ay player
/*
void menu_ay_player(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_ay_player;
	menu_item item_seleccionado;
	int retorno_menu;

        do {


					menu_add_item_menu_inicial_format(&array_menu_ay_player,MENU_OPCION_NORMAL,menu_ay_player_load,NULL,"~~Load ay file");
					menu_add_item_menu_shortcut(array_menu_ay_player,'l');
					menu_add_item_menu_tooltip(array_menu_ay_player,"You should reselect machine when finishing playing because the normal ROM is changed for the player");
					menu_add_item_menu_ayuda(array_menu_ay_player,"You should reselect machine when finishing playing because the normal ROM is changed for the player");

					if (audio_ay_player_mem!=NULL) {

						menu_add_item_menu_format(array_menu_ay_player,MENU_OPCION_NORMAL,menu_ay_player_next_track,NULL,"~~Next track (%d/%d)",ay_player_pista_actual,ay_player_total_songs() );
						menu_add_item_menu_shortcut(array_menu_ay_player,'n');

						menu_add_item_menu_format(array_menu_ay_player,MENU_OPCION_NORMAL,menu_ay_player_exit_tracker,NULL,"~~Stop player");
						menu_add_item_menu_shortcut(array_menu_ay_player,'s');

					}


		menu_add_item_menu_format(array_menu_ay_player,MENU_OPCION_NORMAL,menu_ay_player_player,NULL,"Player");



                menu_add_item_menu(array_menu_ay_player,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                //menu_add_item_menu(array_menu_ay_player,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_ay_player);

                retorno_menu=menu_dibuja_menu(&ay_player_opcion_seleccionada,&item_seleccionado,array_menu_ay_player,"Audio" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
	                //llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
	                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
        	        }
		}

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}

*/
//menu audio settings
void menu_audio_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_audio_settings;
	menu_item item_seleccionado;
	int retorno_menu;

        do {


					menu_add_item_menu_inicial(&array_menu_audio_settings,"View AY ~~Registers",MENU_OPCION_NORMAL,menu_ay_registers,menu_cond_ay_chip);
					menu_add_item_menu_shortcut(array_menu_audio_settings,'r');

					menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_ay_pianokeyboard,menu_cond_ay_chip,"View AY P~~iano");
					menu_add_item_menu_shortcut(array_menu_audio_settings,'i');

					menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_espectro_sonido,NULL,"View ~~Waveform");
					menu_add_item_menu_shortcut(array_menu_audio_settings,'w');

					menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_ay_player_player,NULL,"AY ~~Player");
					menu_add_item_menu_shortcut(array_menu_audio_settings,'p');

/*
		menu_add_item_menu_inicial_format(&array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_volume,NULL,"Audio Output ~~Volume: %d %%", audiovolume);
		menu_add_item_menu_shortcut(array_menu_audio_settings,'v');

		menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_ay_chip,NULL,"~~AY Chip: %s", (ay_chip_present.v==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_audio_settings,'a');
		menu_add_item_menu_tooltip(array_menu_audio_settings,"Enable AY Chip on this machine");
		menu_add_item_menu_ayuda(array_menu_audio_settings,"It enables the AY Chip for the machine, by activating the following hardware:\n"
					"-Normal AY Chip for Spectrum\n"
					"-Fuller audio box for Spectrum\n"
					"-Quicksilva QS Sound board on ZX80/81\n"
					"-Bi-Pak ZON-X81 Sound on ZX80/81\n"
			);

		menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_turbosound,NULL,"~~Turbosound: %s",(turbosound_enabled.v==1 ? "On" : "Off"));

		menu_add_item_menu_shortcut(array_menu_audio_settings,'t');
		menu_add_item_menu_tooltip(array_menu_audio_settings,"Enable Turbosound");
		menu_add_item_menu_ayuda(array_menu_audio_settings,"Enable Turbosound");


		menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_ay_chip_autoenable,NULL,"Autoenable AY Chip: %s",(autoenable_ay_chip.v==1 ? "On" : "Off"));
		menu_add_item_menu_tooltip(array_menu_audio_settings,"Enable AY Chip automatically when it is needed");
		menu_add_item_menu_ayuda(array_menu_audio_settings,"This option is usefor for example on Spectrum 48k games that uses AY Chip "
					"and for some ZX80/81 games that also uses it (Bi-Pak ZON-X81, but not Quicksilva QS Sound board)");


		menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_envelopes,menu_cond_ay_chip,"AY ~~Envelopes: %s", (ay_envelopes_enabled.v==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_audio_settings,'e');
		menu_add_item_menu_tooltip(array_menu_audio_settings,"Enable or disable volume envelopes for the AY Chip");
		menu_add_item_menu_ayuda(array_menu_audio_settings,"Enable or disable volume envelopes for the AY Chip");

		menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_speech,menu_cond_ay_chip,"AY ~~Speech: %s", (ay_speech_enabled.v==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_audio_settings,'s');
		menu_add_item_menu_tooltip(array_menu_audio_settings,"Enable or disable AY Speech effects");
		menu_add_item_menu_ayuda(array_menu_audio_settings,"These effects are used, for example, in Chase H.Q.");

		*/





                //menu_add_item_menu(array_menu_audio_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);


/*		menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_beeper_real,NULL,"Real ~~Beeper: %s",(beeper_real_enabled==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_audio_settings,'b');
		menu_add_item_menu_tooltip(array_menu_audio_settings,"Enable or disable Real Beeper sound");
		menu_add_item_menu_ayuda(array_menu_audio_settings,"Real beeper produces beeper sound more realistic but uses a bit more cpu");


		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_beep_filter_on_rom_save,NULL,"Audio filter on ROM SAVE: %s",(output_beep_filter_on_rom_save.v ? "Yes" : "No"));
			menu_add_item_menu_tooltip(array_menu_audio_settings,"Apply filter on ROM save routines");
			menu_add_item_menu_ayuda(array_menu_audio_settings,"It detects when on ROM save routines and alter audio output to use only "
					"the MIC bit of the FEH port");

//extern z80_bit output_beep_filter_alter_volume;
//extern char output_beep_filter_volume;

			if (output_beep_filter_on_rom_save.v) {
				menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_beep_alter_volume,NULL,"Alter beeper volume: %s",
				(output_beep_filter_alter_volume.v ? "Yes" : "No") );

				menu_add_item_menu_tooltip(array_menu_audio_settings,"Alter output beeper volume");
				menu_add_item_menu_ayuda(array_menu_audio_settings,"Alter output beeper volume. You can set to a maximum to "
							"send the audio to a real spectrum to load it");


				if (output_beep_filter_alter_volume.v) {
					menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_beep_volume,NULL,"Volume: %d",output_beep_filter_volume);
				}
			}

		}
		*/


		//if (si_complete_video_driver() ) {

		//}

/*
		if (MACHINE_IS_ZX8081) {
			//sound on zx80/81

			menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_zx8081_detect_vsync_sound,menu_cond_zx8081,"Detect VSYNC Sound: %s",(zx8081_detect_vsync_sound.v ? "Yes" : "No"));
			menu_add_item_menu_tooltip(array_menu_audio_settings,"Tries to detect when vsync sound is played. This feature is experimental");
			menu_add_item_menu_ayuda(array_menu_audio_settings,"Tries to detect when vsync sound is played. This feature is experimental");


			menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_audio_sound_zx8081,menu_cond_zx8081,"VSYNC Sound on zx80/81: %s", (zx8081_vsync_sound.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_audio_settings,"Enables or disables VSYNC sound on ZX80 and ZX81");
			menu_add_item_menu_ayuda(array_menu_audio_settings,"This method uses the VSYNC signal on the TV to make sound");


                	menu_add_item_menu(array_menu_audio_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
		}


		char string_aofile_shown[10];
		menu_tape_settings_trunc_name(aofilename,string_aofile_shown,10);
		menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_aofile,NULL,"Audio ~~out to file: %s",string_aofile_shown);
		menu_add_item_menu_shortcut(array_menu_audio_settings,'o');
		menu_add_item_menu_tooltip(array_menu_audio_settings,"Saves the generated sound to a file");
		menu_add_item_menu_ayuda(array_menu_audio_settings,"You can save .raw format and if compiled with sndfile, to .wav format. "
					"You can see the file parameters on the console enabling verbose debug level to 2 minimum");



		menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_aofile_insert,menu_aofile_cond,"Audio file ~~inserted: %s",(aofile_inserted.v ? "Yes" : "No" ));
		menu_add_item_menu_shortcut(array_menu_audio_settings,'i');


                menu_add_item_menu_format(array_menu_audio_settings,MENU_OPCION_NORMAL,menu_change_audio_driver,NULL,"Change Audio Driver");

*/
                menu_add_item_menu(array_menu_audio_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                //menu_add_item_menu(array_menu_audio_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_audio_settings);

                retorno_menu=menu_dibuja_menu(&audio_settings_opcion_seleccionada,&item_seleccionado,array_menu_audio_settings,"Audio" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
	                //llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
	                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
        	        }
		}

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}

int menu_insert_slot_number;
int menu_insert_slot_ram_size;
int menu_insert_slot_type;
//char menu_insert_slot_rom_name[PATH_MAX];
char menu_insert_slot_eprom_name[PATH_MAX];
char menu_insert_slot_flash_intel_name[PATH_MAX];

void menu_z88_slot_insert_internal_ram(MENU_ITEM_PARAMETERS)
{

	//RAM interna. Siempre entre 32 y 512 K
        if (menu_insert_slot_ram_size==512*1024) menu_insert_slot_ram_size=32*1024;
        else menu_insert_slot_ram_size *=2;
}


void menu_z88_slot_insert_ram(MENU_ITEM_PARAMETERS)
{

	//RAM siempre entre 32 y 1024 K
	if (menu_insert_slot_ram_size==0) menu_insert_slot_ram_size=32768;
	else if (menu_insert_slot_ram_size==1024*1024) menu_insert_slot_ram_size=0;
	else menu_insert_slot_ram_size *=2;
}

/*
void old_menu_z88_slot_insert_rom(void)
{
        char *filtros[3];

        filtros[0]="epr";
        filtros[1]="63";
        filtros[2]=0;

        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de rom
        //si no hay directorio, vamos a rutas predefinidas


	//printf ("menu_insert_slot_rom_name: %s\n",menu_insert_slot_rom_name);

        if (menu_insert_slot_rom_name[0]==0) menu_chdir_sharedfiles();
        else {
                char directorio[PATH_MAX];
                util_get_dir(menu_insert_slot_rom_name,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }


        int ret;

        ret=menu_filesel("Select Rom File",filtros,menu_insert_slot_rom_name);
	menu_filesel_chdir(directorio_actual);

        if (ret==1) {
		//z88_load_rom_card(menu_insert_slot_rom_name,slot);
        }

	else menu_insert_slot_rom_name[0]=0;

}
*/



int menu_z88_eprom_size(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_z88_eprom_size;
        menu_item item_seleccionado;
        int retorno_menu;

        do {
               //32, 128, 256

                menu_add_item_menu_inicial_format(&array_menu_z88_eprom_size,MENU_OPCION_NORMAL,NULL,NULL,"32 Kb");

                menu_add_item_menu_format(array_menu_z88_eprom_size,MENU_OPCION_NORMAL,NULL,NULL,"128 Kb");

		menu_add_item_menu_format(array_menu_z88_eprom_size,MENU_OPCION_NORMAL,NULL,NULL,"256 Kb");


                menu_add_item_menu(array_menu_z88_eprom_size,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                menu_add_ESC_item(array_menu_z88_eprom_size);

                retorno_menu=menu_dibuja_menu(&z88_eprom_size_opcion_seleccionada,&item_seleccionado,array_menu_z88_eprom_size,"Eprom Size" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }

			//Devolver tamanyo eprom
			if (z88_eprom_size_opcion_seleccionada==0) return 32*1024;
			if (z88_eprom_size_opcion_seleccionada==1) return 128*1024;
			if (z88_eprom_size_opcion_seleccionada==2) return 256*1024;
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC);

	//Salimos con ESC, devolver 0
	return 0;
}


void menu_z88_slot_insert_eprom(MENU_ITEM_PARAMETERS)
{
        char *filtros[4];

        filtros[0]="eprom";
        filtros[1]="epr";
        filtros[2]="63";
        filtros[3]=0;

	//guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de rom
        //si no hay directorio, vamos a rutas predefinidas
        if (menu_insert_slot_eprom_name[0]==0) menu_chdir_sharedfiles();
        else {
                char directorio[PATH_MAX];
                util_get_dir(menu_insert_slot_eprom_name,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }



        int ret;

        ret=menu_filesel("Select existing or new",filtros,menu_insert_slot_eprom_name);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);

        if (ret==1) {

       		//Ver si archivo existe y preguntar si inicializar o no
                struct stat buf_stat;

                if (stat(menu_insert_slot_eprom_name, &buf_stat)!=0) {

			if (menu_confirm_yesno_texto("File does not exist","Create?")==0) {
				menu_insert_slot_eprom_name[0]=0;
				return;
			}

			//Preguntar tamanyo EPROM
			int size=menu_z88_eprom_size(0);

			if (size==0) {
				//ha salido con ESC
				menu_insert_slot_eprom_name[0]=0;
				return;
			}


			if (z88_create_blank_eprom_flash_file(menu_insert_slot_eprom_name,size)) {
				menu_insert_slot_eprom_name[0]=0;
			}

			return;
                }
        }

        else menu_insert_slot_eprom_name[0]=0;
}


void menu_z88_slot_insert_hybrid_eprom(MENU_ITEM_PARAMETERS)
{
        char *filtros[4];

        filtros[0]="eprom";
        filtros[1]="epr";
        //filtros[2]="63"; .63 extensions no admitidas para eprom que son hybridas
        filtros[2]=0;

        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de rom
        //si no hay directorio, vamos a rutas predefinidas
        if (menu_insert_slot_eprom_name[0]==0) menu_chdir_sharedfiles();
        else {
                char directorio[PATH_MAX];
                util_get_dir(menu_insert_slot_eprom_name,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }



        int ret;

        ret=menu_filesel("Select existing or new",filtros,menu_insert_slot_eprom_name);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);

  if (ret==1) {

                //Ver si archivo existe y preguntar si inicializar o no
                struct stat buf_stat;

                if (stat(menu_insert_slot_eprom_name, &buf_stat)!=0) {

                        if (menu_confirm_yesno_texto("File does not exist","Create?")==0) {
                                menu_insert_slot_eprom_name[0]=0;
                                return;
                        }

                        //Tamanyo de la parte EPROM es de 512kb
                        int size=512*1024;


                        if (z88_create_blank_eprom_flash_file(menu_insert_slot_eprom_name,size)) {
                                menu_insert_slot_eprom_name[0]=0;
                        }

                        return;
                }
        }

        else menu_insert_slot_eprom_name[0]=0;
}



int menu_z88_flash_intel_size(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_z88_flash_intel_size;
        menu_item item_seleccionado;
        int retorno_menu;

        do {
               //512,1024

                menu_add_item_menu_inicial_format(&array_menu_z88_flash_intel_size,MENU_OPCION_NORMAL,NULL,NULL,"512 Kb");

                menu_add_item_menu_format(array_menu_z88_flash_intel_size,MENU_OPCION_NORMAL,NULL,NULL,"1 Mb");


                menu_add_item_menu(array_menu_z88_flash_intel_size,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                menu_add_ESC_item(array_menu_z88_flash_intel_size);

                retorno_menu=menu_dibuja_menu(&z88_flash_intel_size_opcion_seleccionada,&item_seleccionado,array_menu_z88_flash_intel_size,"Flash Size" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }

                        //Devolver tamanyo flash_intel
                        if (z88_flash_intel_size_opcion_seleccionada==0) return 512*1024;
                        if (z88_flash_intel_size_opcion_seleccionada==1) return 1024*1024;
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC);

        //Salimos con ESC, devolver 0
        return 0;
}


void menu_z88_slot_insert_flash_intel(MENU_ITEM_PARAMETERS)
{
        char *filtros[2];

        filtros[0]="flash";
        filtros[1]=0;

        int ret;

        ret=menu_filesel("Select existing or new",filtros,menu_insert_slot_flash_intel_name);

        if (ret==1) {

                //Ver si archivo existe y preguntar si inicializar o no
                struct stat buf_stat;

                if (stat(menu_insert_slot_flash_intel_name, &buf_stat)!=0) {

                        if (menu_confirm_yesno_texto("File does not exist","Create?")==0) {
                                menu_insert_slot_flash_intel_name[0]=0;
                                return;
                        }

                        //Preguntar tamanyo Flash
                        int size=menu_z88_flash_intel_size(0);

                        if (size==0) {
                                //ha salido con ESC
                                menu_insert_slot_flash_intel_name[0]=0;
                                return;
                        }


                        if (z88_create_blank_eprom_flash_file(menu_insert_slot_flash_intel_name,size)) {
				menu_insert_slot_flash_intel_name[0]=0;
			}

                        return;
                }
        }

        else menu_insert_slot_flash_intel_name[0]=0;
}




//extern void z88_insert_ram_card(int size,int slot);

//Si se han aplicado cambios (insertar tarjeta memoria) para volver a menu anterior
z80_bit menu_z88_slot_insert_applied_changes={0};

void menu_z88_slot_insert_apply(MENU_ITEM_PARAMETERS)
{

	if (menu_insert_slot_number==0) {
		//Memoria interna

		if (menu_confirm_yesno("Need Hard Reset")==1) {
			z88_change_internal_ram(menu_insert_slot_ram_size);
			menu_z88_slot_insert_applied_changes.v=1;
		}
		return;
	}


	if (menu_insert_slot_type==0) {
		//RAM
		z88_insert_ram_card(menu_insert_slot_ram_size,menu_insert_slot_number);
		menu_z88_slot_insert_applied_changes.v=1;
	}


	/*
	if (menu_insert_slot_type==1) {
                //ROM
		if (menu_insert_slot_rom_name[0]==0) debug_printf (VERBOSE_ERR,"Empty ROM name");

		else {
			z88_load_rom_card(menu_insert_slot_rom_name,menu_insert_slot_number);
			menu_z88_slot_insert_applied_changes.v=1;
		}
	}
	*/

        if (menu_insert_slot_type==2) {
                //EPROM
                if (menu_insert_slot_eprom_name[0]==0) debug_printf (VERBOSE_ERR,"Empty EPROM name");

                else {
			z88_load_eprom_card(menu_insert_slot_eprom_name,menu_insert_slot_number);
			menu_z88_slot_insert_applied_changes.v=1;
		}
        }

        if (menu_insert_slot_type==3) {
                //Intel Flash
                if (menu_insert_slot_flash_intel_name[0]==0) debug_printf (VERBOSE_ERR,"Empty Flash name");

                else {
			z88_load_flash_intel_card(menu_insert_slot_flash_intel_name,menu_insert_slot_number);
			menu_z88_slot_insert_applied_changes.v=1;
		}
        }


        if (menu_insert_slot_type==4) {
                //Hybrida RAM+EPROM
                if (menu_insert_slot_eprom_name[0]==0) debug_printf (VERBOSE_ERR,"Empty EPROM name");

                else {
                        z88_load_hybrid_eprom_card(menu_insert_slot_eprom_name,menu_insert_slot_number);
                        menu_z88_slot_insert_applied_changes.v=1;
                }
        }





}

void menu_z88_slot_insert_change_type(MENU_ITEM_PARAMETERS)
{
	if (menu_insert_slot_ram_size==0) {
		//si no habia, activamos ram con 32kb
		menu_insert_slot_type=0;
		menu_insert_slot_ram_size=32768;
	}

	else if (menu_insert_slot_type==0) {
		//de ram pasamos a eprom
		menu_insert_slot_type=2;
	}

/*	else if (menu_insert_slot_type==1) {
		//de rom pasamos a eprom
		menu_insert_slot_type=2;
	}
*/

        else if (menu_insert_slot_type==2) {
                //de eprom pasamos a flash
                menu_insert_slot_type=3;
        }

        else if (menu_insert_slot_type==3) {
                //de flash pasamos a hybrida ram+eprom
                menu_insert_slot_type=4;
        }




	else {
		//y de flash pasamos a vacio - ram con 0 kb
                menu_insert_slot_type=0;
                menu_insert_slot_ram_size=0;
	}
}


//valor_opcion le viene dado desde la propia funcion de gestion de menu
void menu_z88_slot_insert(MENU_ITEM_PARAMETERS)
{
	//menu_insert_slot_number=z88_slots_opcion_seleccionada;


	menu_insert_slot_number=valor_opcion;

	debug_printf (VERBOSE_DEBUG,"Slot selected on menu: %d",menu_insert_slot_number);

	if (menu_insert_slot_number<0 || menu_insert_slot_number>3) cpu_panic ("Invalid slot number");

	menu_z88_slot_insert_applied_changes.v=0;


	if (menu_insert_slot_number>0) {
		//Memoria externa
		//guardamos tamanyo +1
		menu_insert_slot_ram_size=z88_memory_slots[menu_insert_slot_number].size+1;

		if (menu_insert_slot_ram_size==1) menu_insert_slot_ram_size=0;

		menu_insert_slot_type=z88_memory_slots[menu_insert_slot_number].type;
	}

	else {
		//Memoria interna
		//guardamos tamanyo +1

		menu_insert_slot_ram_size=z88_internal_ram_size+1;
	}



	//reseteamos nombre rom
	//menu_insert_slot_rom_name[0]=0;

	//copiamos nombre eprom/flash a las dos variables
	strcpy(menu_insert_slot_eprom_name,z88_memory_slots[menu_insert_slot_number].eprom_flash_nombre_archivo);

	strcpy(menu_insert_slot_flash_intel_name,z88_memory_slots[menu_insert_slot_number].eprom_flash_nombre_archivo);




        menu_item *array_menu_z88_slot_insert;
        menu_item item_seleccionado;
        int retorno_menu;
        //char string_slot_name_shown[20];
	char string_slot_eprom_name_shown[20],string_slot_flash_intel_name_shown[20];
	char string_memory_type[20];

        do {

		//menu_tape_settings_trunc_name(menu_insert_slot_rom_name,string_slot_name_shown,20);

		menu_tape_settings_trunc_name(menu_insert_slot_eprom_name,string_slot_eprom_name_shown,20);
		menu_tape_settings_trunc_name(menu_insert_slot_flash_intel_name,string_slot_flash_intel_name_shown,20);


                //int slot;


		if (menu_insert_slot_ram_size==0) sprintf (string_memory_type,"%s","Empty");
		else sprintf (string_memory_type,"%s",z88_memory_types[menu_insert_slot_type]);

		//Memorias externas

		if (menu_insert_slot_number>0) {

			menu_add_item_menu_inicial_format(&array_menu_z88_slot_insert,MENU_OPCION_NORMAL,menu_z88_slot_insert_change_type,NULL,"Memory type: %s",string_memory_type);
        	        menu_add_item_menu_tooltip(array_menu_z88_slot_insert,"Type of memory card if present");
                	menu_add_item_menu_ayuda(array_menu_z88_slot_insert,"Type of memory card if present");

			if (menu_insert_slot_type==0) {
				//RAM
				menu_add_item_menu_format(array_menu_z88_slot_insert,MENU_OPCION_NORMAL,menu_z88_slot_insert_ram,NULL,"Size: %d Kb",(menu_insert_slot_ram_size)/1024);
		                menu_add_item_menu_tooltip(array_menu_z88_slot_insert,"Size of RAM card");
        		        menu_add_item_menu_ayuda(array_menu_z88_slot_insert,"Size of RAM card");

			}

			if (menu_insert_slot_type==1) {
				cpu_panic("ROM cards do not exist on Z88");
				/*
				//ROM
				menu_add_item_menu_format(array_menu_z88_slot_insert,MENU_OPCION_NORMAL,menu_z88_slot_insert_rom,NULL,"Name: %s",string_slot_name_shown);
		                menu_add_item_menu_tooltip(array_menu_z88_slot_insert,"Rom file to load");
        		        menu_add_item_menu_ayuda(array_menu_z88_slot_insert,"Rom file to load. Valid formats are .epr and .63\n"
					"Note the emulator include some programs repeated in both formats");


				//printf ("en add item rom: %s trunc: %s\n",menu_insert_slot_rom_name,string_slot_name_shown);
				*/
			}

			if (menu_insert_slot_type==2) {
                	        //EPROM
                        	menu_add_item_menu_format(array_menu_z88_slot_insert,MENU_OPCION_NORMAL,menu_z88_slot_insert_eprom,NULL,"Name: %s",string_slot_eprom_name_shown);
		                menu_add_item_menu_tooltip(array_menu_z88_slot_insert,"EPROM file to use");
        		        menu_add_item_menu_ayuda(array_menu_z88_slot_insert,"EPROM file to use. Valid formats are .eprom. Select existing or new");

                	}

                        if (menu_insert_slot_type==3) {
                                //Intel Flash
                                menu_add_item_menu_format(array_menu_z88_slot_insert,MENU_OPCION_NORMAL,menu_z88_slot_insert_flash_intel,NULL,"Name: %s",string_slot_flash_intel_name_shown);
                                menu_add_item_menu_tooltip(array_menu_z88_slot_insert,"Intel Flash file to use");
                                menu_add_item_menu_ayuda(array_menu_z88_slot_insert,"Intel Flash file to use. Valid formats are .flash. Select existing or new");

                        }

			if (menu_insert_slot_type==4) {
                                //Hybrida RAM+EPROM
                                menu_add_item_menu_format(array_menu_z88_slot_insert,MENU_OPCION_NORMAL,menu_z88_slot_insert_hybrid_eprom,NULL,"Name: %s",string_slot_eprom_name_shown);
                                menu_add_item_menu_tooltip(array_menu_z88_slot_insert,"Hybrid RAM+EPROM file to use");
                                menu_add_item_menu_ayuda(array_menu_z88_slot_insert,"Hybrid RAM+EPROM file to use. Valid formats are .eprom. Select existing or new");

                        }



		}

		else {
			//Memoria interna
			menu_add_item_menu_inicial_format(&array_menu_z88_slot_insert,MENU_OPCION_NORMAL,menu_z88_slot_insert_internal_ram,NULL,"RAM Size: %d Kb",(menu_insert_slot_ram_size)/1024);
			menu_add_item_menu_tooltip(array_menu_z88_slot_insert,"Size of RAM card");
                        menu_add_item_menu_ayuda(array_menu_z88_slot_insert,"Size of RAM card");
		}




		menu_add_item_menu_format(array_menu_z88_slot_insert,MENU_OPCION_NORMAL,menu_z88_slot_insert_apply,NULL,"Apply changes");
                menu_add_item_menu_tooltip(array_menu_z88_slot_insert,"Apply slot changes");
                menu_add_item_menu_ayuda(array_menu_z88_slot_insert,"Apply slot changes");





                menu_add_item_menu(array_menu_z88_slot_insert,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                menu_add_ESC_item(array_menu_z88_slot_insert);

                //retorno_menu=menu_dibuja_menu(&z88_slot_insert_opcion_seleccionada,&item_seleccionado,array_menu_z88_slot_insert,"Z88 Memory Slots" );

		//Titulo de la ventana variable segun el nombre del slot
		char texto_titulo[32];
		sprintf (texto_titulo,"Z88 Memory Slot %d",menu_insert_slot_number);

                retorno_menu=menu_dibuja_menu(&z88_slot_insert_opcion_seleccionada,&item_seleccionado,array_menu_z88_slot_insert,texto_titulo);

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !menu_z88_slot_insert_applied_changes.v );


}

void menu_z88_slot_erase_eprom_flash(MENU_ITEM_PARAMETERS)
{
        if (menu_confirm_yesno("Erase Card")==1) {
		z88_erase_eprom_flash();
		menu_generic_message("Erase Card","OK. Card erased");
        }
}

void menu_z88_eprom_flash_reclaim_free_space(MENU_ITEM_PARAMETERS)
{
       if (menu_confirm_yesno("Reclaim Free Space")==1) {
                z80_long_int liberados=z88_eprom_flash_reclaim_free_space();
                menu_generic_message_format("Reclaim Free Space","OK. %d bytes reclaimed",liberados);
        }

}

void menu_z88_eprom_flash_undelete_files(MENU_ITEM_PARAMETERS)
{
       if (menu_confirm_yesno("Undelete Files")==1) {
                z80_long_int undeleted=z88_eprom_flash_recover_deleted();
                menu_generic_message_format("Undelete Files","OK. %d files undeleted",undeleted);
        }

}

char copy_to_eprom_nombrearchivo[PATH_MAX]="";

void menu_z88_slot_copy_to_eprom_flash(MENU_ITEM_PARAMETERS)
{

	//printf ("%d\n",sizeof(copy_to_eprom_nombrearchivo));

        char *filtros[2];

        filtros[0]="";
        filtros[1]=0;


        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        int ret;


        //Obtenemos ultimo directorio visitado
        if (copy_to_eprom_nombrearchivo[0]!=0) {
                char directorio[PATH_MAX];
                util_get_dir(copy_to_eprom_nombrearchivo,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }




        ret=menu_filesel("Select file to copy",filtros,copy_to_eprom_nombrearchivo);

        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);

        if (ret==1) {
		//Cargamos archivo en memoria. Maximo 128 KB
		z80_byte *datos;
		const z80_long_int max_load=131072;
		datos=malloc(max_load);

		FILE *ptr_load;
		ptr_load=fopen(copy_to_eprom_nombrearchivo,"rb");

		if (!ptr_load) {
			debug_printf (VERBOSE_ERR,"Unable to open file %s",copy_to_eprom_nombrearchivo);
			return;
		}


		z80_long_int leidos=fread(datos,1,max_load,ptr_load);
		if (leidos==max_load) {
			debug_printf (VERBOSE_ERR,"Can not load more than %d bytes",max_load);
			return;
		}


		if (leidos<1) {
			debug_printf (VERBOSE_ERR,"No bytes read");
			return;
		}


		//esto puede retornar algun error mediante debug_printf
		//nombre en eprom, quitando carpetas
		char nombre_eprom[256];
		util_get_file_no_directory(copy_to_eprom_nombrearchivo,nombre_eprom);


		//Preguntar nombre en destino. Truncar a maximo de nombre en tarjeta memoria
		nombre_eprom[Z88_MAX_CARD_FILENAME]=0;
		menu_ventana_scanf("Target file name?",nombre_eprom,Z88_MAX_CARD_FILENAME+1);


		if (z88_eprom_flash_fwrite(nombre_eprom,datos,leidos)==0) {
			menu_generic_message("Copy File","OK. File copied to Card");
		}

		free(datos);

	}

}


void menu_z88_slot_card_browser_aux(int slot,char *texto_buffer,int max_texto)
{
        z88_dir dir;
        z88_eprom_flash_file file;

        z88_eprom_flash_find_init(&dir,slot);

        char buffer_nombre[Z88_MAX_CARD_FILENAME+1];

        int retorno;
        int longitud;


        int indice_buffer=0;

        do {
                retorno=z88_eprom_flash_find_next(&dir,&file);
                if (retorno) {
                        z88_eprom_flash_get_file_name(&file,buffer_nombre);

			if (buffer_nombre[0]=='.') buffer_nombre[0]='D'; //archivo borrado

			//printf ("nombre: %s c1: %d\n",buffer_nombre,buffer_nombre[0]);
                        longitud=strlen(buffer_nombre)+1; //Agregar salto de linea
                        if (indice_buffer+longitud>max_texto-1) retorno=0;
                        else {
                                sprintf (&texto_buffer[indice_buffer],"%s\n",buffer_nombre);
                                indice_buffer +=longitud;
                        }

                }
        } while (retorno!=0);

        texto_buffer[indice_buffer]=0;

}

void menu_z88_slot_card_browser(MENU_ITEM_PARAMETERS)
{
#define MAX_TEXTO 4096
        char texto_buffer[MAX_TEXTO];

        int slot=valor_opcion;

        menu_z88_slot_card_browser_aux(slot,texto_buffer,MAX_TEXTO);


	//Si esta vacio
        if (texto_buffer[0]==0) {
                debug_printf(VERBOSE_ERR,"Card is empty");
                return;
        }

        menu_generic_message_tooltip("Card browser", 0, 1, NULL, "%s", texto_buffer);

}


void menu_z88_slot_copy_from_eprom(MENU_ITEM_PARAMETERS)
{
#define MAX_TEXTO 4096
	char texto_buffer[MAX_TEXTO];

	int slot=valor_opcion;

	menu_z88_slot_card_browser_aux(slot,texto_buffer,MAX_TEXTO);

	//Si esta vacio
	if (texto_buffer[0]==0) {
		debug_printf(VERBOSE_ERR,"Card is empty");
                return;
        }



	//printf ("archivos: %s\n",texto_buffer);
	generic_message_tooltip_return retorno_archivo;
	menu_generic_message_tooltip("Select file", 0, 1, &retorno_archivo, "%s", texto_buffer);

	//Si se sale con ESC
	if (retorno_archivo.estado_retorno==0) return;

       //        strcpy(retorno->texto_seleccionado,buffer_lineas[linea_final];
       //         retorno->linea_seleccionada=linea_final;
	//printf ("Seleccionado: (%s) linea: %d\n",retorno_archivo.texto_seleccionado,retorno_archivo.linea_seleccionada);
	//nombre archivo viene con espacio al final


	int longitud_nombre=strlen(retorno_archivo.texto_seleccionado);
	if (longitud_nombre) {
		if (retorno_archivo.texto_seleccionado[longitud_nombre-1] == ' ') {
			retorno_archivo.texto_seleccionado[longitud_nombre-1]=0;
		}
	}



        z88_dir dir;
        z88_eprom_flash_file file;


	z88_find_eprom_flash_file (&dir,&file,retorno_archivo.texto_seleccionado, slot);


	//con el puntero a dir, retornamos file

	//printf ("dir: %x bank: %x\n",dir.dir,dir.bank);

	//z88_return_eprom_flash_file (&dir,&file);

	//Si no es archivo valido
	if (file.namelength==0 || file.namelength==255) {
		debug_printf (VERBOSE_ERR,"File not found");
		return;
	}


	//Grabar archivo en disco. Supuestamente le damos el mismo nombre evitando la / inicial

	//Con selector de archivos
	char nombredestino[PATH_MAX];

	char *filtros[2];

        filtros[0]="";
        filtros[1]=0;


        if (menu_filesel("Select Target File",filtros,nombredestino)==0) return;


	//Ver si archivo existe
	if (si_existe_archivo(nombredestino)) {
		if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) return;
	}




	//Hacer que dir apunte a los datos
	dir.bank=file.datos.bank;
	dir.dir=file.datos.dir;

	z80_byte byte_leido;

                                FILE *ptr_file_save;
                                  ptr_file_save=fopen(nombredestino,"wb");
                                  if (!ptr_file_save)
                                {
                                      debug_printf (VERBOSE_ERR,"Unable to open Binary file %s",nombredestino);

                                  }
                                else {
					unsigned int i;
					z80_long_int size=file.size[0]+(file.size[1]*256)+(file.size[2]*65536)+(file.size[3]*16777216);

					for (i=0;i<size;i++) {
						byte_leido=peek_byte_no_time_z88_bank_no_check_low(dir.dir,dir.bank);
						z88_increment_pointer(&dir);
						fwrite(&byte_leido,1,1,ptr_file_save);
					}


                                  fclose(ptr_file_save);
				}
	menu_generic_message("Copy File","OK. File copied from Card to Disk");




}


//menu z88 slots
void menu_z88_slots(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_z88_slots;
        menu_item item_seleccionado;
        int retorno_menu;

        do {

		menu_add_item_menu_inicial_format(&array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_slot_insert,NULL,"ROM: %d Kb RAM: %d Kb",(z88_internal_rom_size+1)/1024,(z88_internal_ram_size+1)/1024);
                menu_add_item_menu_tooltip(array_menu_z88_slots,"Internal ROM and RAM");
                menu_add_item_menu_ayuda(array_menu_z88_slots,"Internal RAM can be maximum 512 KB. Internal ROM can be changed from \n"
					"Machine Menu->Custom Machine, and can also be maximum 512 KB");

                //establecemos numero slot como opcion de ese item de menu
                menu_add_item_menu_valor_opcion(array_menu_z88_slots,0);



		int slot;

		//int eprom_flash_invalida_en_slot_3=0;

		for (slot=1;slot<=3;slot++) {

			int eprom_flash_valida=0;
			int tipo_tarjeta=-1;
			char *tipos_tarjeta[]={"APP","FIL","MIX"};
			int type;

			//Si no hay slot insertado
			if (z88_memory_slots[slot].size==0) {
				menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_slot_insert,NULL,"Empty");
			}

			else {
				type=z88_memory_slots[slot].type;

				//Si es flash/eprom en slot de escritura(3), indicar a que archivo hace referencia
				if (slot==3 && (type==2 || type==3 || type==4) ) {
					char string_writable_card_shown[18];
					menu_tape_settings_trunc_name(z88_memory_slots[slot].eprom_flash_nombre_archivo,string_writable_card_shown,18);
					menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_slot_insert,NULL,"%s: %s",z88_memory_types[type],string_writable_card_shown);
				}
				else {
					menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_slot_insert,NULL,"%s",z88_memory_types[type]);
				}
			}

			//establecemos numero slot como opcion de ese item de menu
			menu_add_item_menu_valor_opcion(array_menu_z88_slots,slot);
	                menu_add_item_menu_tooltip(array_menu_z88_slots,"Type of memory card if present and file name in case of slot 3 and EPROM/Flash cards");
        	        menu_add_item_menu_ayuda(array_menu_z88_slots,"Type of memory card if present and file name in case of slot 3 and EPROM/Flash cards.\n"
						"Slot 1, 2 or 3 can contain "
						"EPROM or Intel Flash cards. But EPROMS and Flash cards can only be written on slot 3\n"
						"EPROM/Flash cards files on slots 1,2 are only used at insert time and loaded on Z88 memory\n"
						"EPROM/Flash cards files on slot 3 are loaded on Z88 memory at insert time but are written everytime \n"
						"a change is made on this Z88 memory, so, they are always used when they are inserted\n"
						"\n"
						"Flash card files and eprom files are internally compatible, but eprom size maximum is 256 Kb "
						"and flash minimum size is 512 Kb, so you can not load an eprom file as a flash or viceversa\n"
						);
						//"Note: if you use OZ rom 4.3.1 or higher, flash cards are recognized well; if you use a lower "
						//"version, flash cards are recognized as eprom cards");

			if (z88_memory_slots[slot].size!=0) {

				int size=(z88_memory_slots[slot].size+1)/1024;

				//Si es EPROM o Flash, decir espacio libre, detectando si hay una tarjeta con filesystem
				char string_info_tarjeta[40];
				if (type==2 || type==3 || type==4) {
					z80_long_int total_eprom,used_eprom, free_eprom;
					tipo_tarjeta=z88_return_card_type(slot);
					debug_printf (VERBOSE_DEBUG,"Card type: %d",tipo_tarjeta);

					if (tipo_tarjeta>0) {

						//No buscar filesystem en caso de tarjeta hibrida
						if (type==4) {
							sprintf (string_info_tarjeta," (%s) %d kb Free Unkn",tipos_tarjeta[tipo_tarjeta],size);
                                                }

						else {

							z88_eprom_flash_free(&total_eprom,&used_eprom, &free_eprom,slot);

							//Controlar si eprom corrupta, tamanyos logicos
							if (free_eprom>1024*1024) {
								sprintf (string_info_tarjeta," (%s) %d kb Free Unkn",tipos_tarjeta[tipo_tarjeta],size);
							}

							else {
								sprintf (string_info_tarjeta," (%s) %d K Free %d K",tipos_tarjeta[tipo_tarjeta],size,free_eprom/1024);
								eprom_flash_valida=1;
							}
						}
					}

					else {
						//0 o -1
						if (tipo_tarjeta==0) {
							sprintf (string_info_tarjeta," (%s) %d kb",tipos_tarjeta[tipo_tarjeta],size);
						}
						else {
							sprintf (string_info_tarjeta," (Unk) %d kb",size);
						}
					}
				}

				else {
					sprintf (string_info_tarjeta," %d K",size);
				}

				menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,NULL,NULL,"%s",string_info_tarjeta);
				menu_add_item_menu_tooltip(array_menu_z88_slots,"Card Information");
				menu_add_item_menu_ayuda(array_menu_z88_slots,"Size of Card, and in case of EPROM/Flash cards: \n"
							"-Type of Card: Applications, Files or Unknown type\n"
							"-Space Available, in case of Files Card\n");


			}

			//Si hay una eprom en slot 3, dar opcion de borrar
			if (slot==3 && z88_memory_slots[3].size!=0 && (z88_memory_slots[3].type==2 || z88_memory_slots[3].type==3) ) {
				menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_slot_erase_eprom_flash,NULL," Erase Card");
	                        menu_add_item_menu_tooltip(array_menu_z88_slots,"Card can only be erased on slot 3");
        	                menu_add_item_menu_ayuda(array_menu_z88_slots,"Card can only be erased on slot 3");
			}



			if (eprom_flash_valida && tipo_tarjeta>0) {

				if (slot==3) {

					menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_eprom_flash_reclaim_free_space,NULL," Reclaim Free Space");
					menu_add_item_menu_tooltip(array_menu_z88_slots,"It reclaims the space used by deleted files");
					menu_add_item_menu_ayuda(array_menu_z88_slots,"It reclaims the space used by deleted files");

					menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_eprom_flash_undelete_files,NULL," Undelete Files");
					menu_add_item_menu_tooltip(array_menu_z88_slots,"Undelete deleted files");
					menu_add_item_menu_ayuda(array_menu_z88_slots,"Undelete deleted files");


					menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_slot_copy_to_eprom_flash,NULL," Copy to Card");
					menu_add_item_menu_tooltip(array_menu_z88_slots,"Copy files from your hard drive to Card");
					menu_add_item_menu_ayuda(array_menu_z88_slots,"Copy files from your hard drive to Card. "
						"Card card must be initialized before you can copy files to it, "
						"I mean, if it's a new card, it must be erased and Z88 Filer must know it; usually it is needed to copy "
						"at least one file from Filer");
				}




				menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_slot_card_browser,NULL," Card browser");
				menu_add_item_menu_tooltip(array_menu_z88_slots,"Browse card");
				menu_add_item_menu_ayuda(array_menu_z88_slots,"Browse card");
	                        //establecemos numero slot como opcion de ese item de menu
        	                menu_add_item_menu_valor_opcion(array_menu_z88_slots,slot);

				menu_add_item_menu_format(array_menu_z88_slots,MENU_OPCION_NORMAL,menu_z88_slot_copy_from_eprom,NULL," Copy from Card");
				menu_add_item_menu_tooltip(array_menu_z88_slots,"Copy files from Card to your hard drive");
				menu_add_item_menu_ayuda(array_menu_z88_slots,"Copy files from Card to your hard drive");
	                        //establecemos numero slot como opcion de ese item de menu
        	                menu_add_item_menu_valor_opcion(array_menu_z88_slots,slot);


			}

		}


                menu_add_item_menu(array_menu_z88_slots,"",MENU_OPCION_SEPARADOR,NULL,NULL);


		menu_add_ESC_item(array_menu_z88_slots);

                retorno_menu=menu_dibuja_menu(&z88_slots_opcion_seleccionada,&item_seleccionado,array_menu_z88_slots,"Z88 Memory Slots" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC);


}











//Retorna 0 si ok
//Retorna -1 si fuera de rango
//Modifica valor de variable
int menu_hardware_advanced_input_value(int minimum,int maximum,char *texto,int *variable)
{

	int valor;

        char string_value[4];

        sprintf (string_value,"%d",*variable);


        menu_ventana_scanf(texto,string_value,4);

        valor=parse_string_to_number(string_value);

	if (valor<minimum || valor>maximum) {
		debug_printf (VERBOSE_ERR,"Value out of range. Minimum: %d Maximum: %d",minimum,maximum);
		return -1;
	}

	*variable=valor;
	return 0;


}

void menu_hardware_advanced_reload_display(void)
{

		screen_testados_linea=screen_total_borde_izquierdo/2+128+screen_total_borde_derecho/2+screen_invisible_borde_derecho/2;

	        //Recalcular algunos valores cacheados
	        recalcular_get_total_ancho_rainbow();
        	recalcular_get_total_alto_rainbow();

                screen_set_video_params_indices();
                inicializa_tabla_contend();

                init_rainbow();
                init_cache_putpixel();
}



void menu_hardware_advanced_hidden_top_border(MENU_ITEM_PARAMETERS)
{
	int max,min;
	min=7;
	max=16;
	if (MACHINE_IS_PRISM) {
		min=32;
		max=45;
	}

	if (menu_hardware_advanced_input_value(min,max,"Hidden top Border",&screen_invisible_borde_superior)==0) {
		menu_hardware_advanced_reload_display();
	}
}

void menu_hardware_advanced_visible_top_border(MENU_ITEM_PARAMETERS)
{

	int max,min;
	min=32;
	max=56;
	if (MACHINE_IS_PRISM) {
		min=32;
		max=48;
	}

        if (menu_hardware_advanced_input_value(min,max,"Visible top Border",&screen_borde_superior)==0) {
                menu_hardware_advanced_reload_display();
        }
}

void menu_hardware_advanced_visible_bottom_border(MENU_ITEM_PARAMETERS)
{
	int max,min;
	min=48;
	max=56;
	if (MACHINE_IS_PRISM) {
		min=32;
                max=48;
        }



        if (menu_hardware_advanced_input_value(min,max,"Visible bottom Border",&screen_total_borde_inferior)==0) {
                menu_hardware_advanced_reload_display();
        }
}

void menu_hardware_advanced_borde_izquierdo(MENU_ITEM_PARAMETERS)
{

	int valor_pixeles;

	valor_pixeles=screen_total_borde_izquierdo/2;

        int max,min;
	min=12;
	max=24;
	if (MACHINE_IS_PRISM) {
                min=20;
		max=32;
	}

	if (menu_hardware_advanced_input_value(min,max,"Left Border TLength",&valor_pixeles)==0) {
		screen_total_borde_izquierdo=valor_pixeles*2;
		 menu_hardware_advanced_reload_display();
        }
}

void menu_hardware_advanced_borde_derecho(MENU_ITEM_PARAMETERS)
{

        int valor_pixeles;

	valor_pixeles=screen_total_borde_derecho/2;

        int max,min;
	min=12;
        max=24;
	if (MACHINE_IS_PRISM) {
                min=20;
                max=32;
        }

        if (menu_hardware_advanced_input_value(min,max,"Right Border TLength",&valor_pixeles)==0) {
                screen_total_borde_derecho=valor_pixeles*2;
                 menu_hardware_advanced_reload_display();
        }
}

void menu_hardware_advanced_hidden_borde_derecho(MENU_ITEM_PARAMETERS)
{

        int valor_pixeles;

	valor_pixeles=screen_invisible_borde_derecho/2;

        int max,min;
	min=24;
	max=52;

	if (MACHINE_IS_PRISM) {
                min=60;
                max=79;
        }

        if (menu_hardware_advanced_input_value(min,max,"Right Hidden B.TLength",&valor_pixeles)==0) {
                screen_invisible_borde_derecho=valor_pixeles*2;
                 menu_hardware_advanced_reload_display();
        }
}


void menu_ula_advanced(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_hardware_advanced;
        menu_item item_seleccionado;
        int retorno_menu;
        do {
                menu_add_item_menu_inicial_format(&array_menu_hardware_advanced,MENU_OPCION_NORMAL,menu_hardware_advanced_hidden_top_border,NULL,"Hidden Top Border: %d",screen_invisible_borde_superior);

                menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,menu_hardware_advanced_visible_top_border,NULL,"Visible Top Border: %d",screen_borde_superior);
                menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,menu_hardware_advanced_visible_bottom_border,NULL,"Visible Bottom Border: %d",screen_total_borde_inferior);

		menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,menu_hardware_advanced_borde_izquierdo,NULL,"Left Border TLength: %d",screen_total_borde_izquierdo/2);
		menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,menu_hardware_advanced_borde_derecho,NULL,"Right Border TLength: %d",screen_total_borde_derecho/2);

		menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,menu_hardware_advanced_hidden_borde_derecho,NULL,"Right Hidden B. TLength: %d",screen_invisible_borde_derecho/2);


                menu_add_item_menu(array_menu_hardware_advanced,"",MENU_OPCION_SEPARADOR,NULL,NULL);

		menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,NULL,NULL,"Info:");
		menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,NULL,NULL,"Total line TLength: %d",screen_testados_linea);
		menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,NULL,NULL,"Total scanlines: %d",screen_scanlines);
		menu_add_item_menu_format(array_menu_hardware_advanced,MENU_OPCION_NORMAL,NULL,NULL,"Total T-states: %d",screen_testados_total);

                menu_add_item_menu(array_menu_hardware_advanced,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_hardware_advanced,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_hardware_advanced);

                retorno_menu=menu_dibuja_menu(&hardware_advanced_opcion_seleccionada,&item_seleccionado,array_menu_hardware_advanced,"Advanced Timing Settings" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}

void menu_hardware_printers_zxprinter_enable(MENU_ITEM_PARAMETERS)
{
	zxprinter_enabled.v ^=1;

	if (zxprinter_enabled.v==0) {
	        close_zxprinter_bitmap_file();
        	close_zxprinter_ocr_file();
	}

}

int menu_hardware_zxprinter_cond(void)
{
	return zxprinter_enabled.v;
}

void menu_hardware_zxprinter_bitmapfile(MENU_ITEM_PARAMETERS)
{


	close_zxprinter_bitmap_file();

        char *filtros[3];

        filtros[0]="txt";
        filtros[1]="pbm";
        filtros[2]=0;


        if (menu_filesel("Select Bitmap File",filtros,zxprinter_bitmap_filename_buffer)==1) {
                //Ver si archivo existe y preguntar
                struct stat buf_stat;

                if (stat(zxprinter_bitmap_filename_buffer, &buf_stat)==0) {

                        if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) return;

                }

                zxprinter_bitmap_filename=zxprinter_bitmap_filename_buffer;

		zxprinter_file_bitmap_init();


        }

//        else {
//		close_zxprinter_file();
//        }
}

void menu_hardware_zxprinter_ocrfile(MENU_ITEM_PARAMETERS)
{


        close_zxprinter_ocr_file();

        char *filtros[2];

        filtros[0]="txt";
        filtros[1]=0;


        if (menu_filesel("Select OCR File",filtros,zxprinter_ocr_filename_buffer)==1) {
                //Ver si archivo existe y preguntar
                struct stat buf_stat;

                if (stat(zxprinter_ocr_filename_buffer, &buf_stat)==0) {

                        if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) return;

                }

                zxprinter_ocr_filename=zxprinter_ocr_filename_buffer;

                zxprinter_file_ocr_init();


        }

//        else {
//              close_zxprinter_file();
//        }
}


void menu_hardware_zxprinter_copy(MENU_ITEM_PARAMETERS)
{
        push_valor(reg_pc);

	if (MACHINE_IS_SPECTRUM) {
	        reg_pc=0x0eac;
	}

	if (MACHINE_IS_ZX81) {
		reg_pc=0x0869;
	}


	if (menu_multitarea) menu_generic_message("COPY","OK. COPY executed");
	else menu_generic_message("COPY","Register PC set to the COPY routine. Return to the emulator to let the COPY routine to be run");


}



void menu_hardware_printers(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_hardware_printers;
        menu_item item_seleccionado;
        int retorno_menu;
        do {
                menu_add_item_menu_inicial_format(&array_menu_hardware_printers,MENU_OPCION_NORMAL,menu_hardware_printers_zxprinter_enable,NULL,"ZX Printer: %s",(zxprinter_enabled.v==1 ? "On" : "Off"));
		menu_add_item_menu_tooltip(array_menu_hardware_printers,"Enables or disables ZX Printer emulation");
		menu_add_item_menu_ayuda(array_menu_hardware_printers,"You must set it to off when finishing printing to close generated files");

                char string_bitmapfile_shown[16];
                menu_tape_settings_trunc_name(zxprinter_bitmap_filename,string_bitmapfile_shown,16);

                menu_add_item_menu_format(array_menu_hardware_printers,MENU_OPCION_NORMAL,menu_hardware_zxprinter_bitmapfile,menu_hardware_zxprinter_cond,"Bitmap file: %s",string_bitmapfile_shown);

                menu_add_item_menu_tooltip(array_menu_hardware_printers,"Sends printer output to image file");
                menu_add_item_menu_ayuda(array_menu_hardware_printers,"Printer output is saved to a image file. Supports pbm file format, and "
					"also supports text file, "
					"where every pixel is a character on text. "
					"It is recommended to close the image file when finishing printing, so its header is updated");


		char string_ocrfile_shown[19];
		menu_tape_settings_trunc_name(zxprinter_ocr_filename,string_ocrfile_shown,19);

                menu_add_item_menu_format(array_menu_hardware_printers,MENU_OPCION_NORMAL,menu_hardware_zxprinter_ocrfile,menu_hardware_zxprinter_cond,"OCR file: %s",string_ocrfile_shown);

                menu_add_item_menu_tooltip(array_menu_hardware_printers,"Sends printer output to text file using OCR method");
                menu_add_item_menu_ayuda(array_menu_hardware_printers,"Printer output is saved to a text file using OCR method to guess text. "
					"If you cancel a printing with SHIFT+SPACE on Basic, you have to re-select the ocr file to reset some "
					"internal counters. If you don't do that, OCR will not work");


		menu_add_item_menu_format(array_menu_hardware_printers,MENU_OPCION_NORMAL,menu_hardware_zxprinter_copy,menu_hardware_zxprinter_cond,"Run COPY routine");

                menu_add_item_menu_tooltip(array_menu_hardware_printers,"Runs ROM COPY routine");
		menu_add_item_menu_ayuda(array_menu_hardware_printers,"It calls ROM copy routine on Spectrum and ZX-81, like the COPY command on BASIC. \n"
					"I did not guarantee that the call will always work, because this function will probably "
					"use some structures and variables needed in BASIC and if you are running some game, maybe it "
					"has not these variables correct");



                menu_add_item_menu(array_menu_hardware_printers,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_hardware_printers,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_hardware_printers);

                retorno_menu=menu_dibuja_menu(&hardware_printers_opcion_seleccionada,&item_seleccionado,array_menu_hardware_printers,"Printing emulation" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}


void menu_hardware_keyboard_issue(MENU_ITEM_PARAMETERS)
{
	keyboard_issue2.v^=1;
}

void menu_hardware_azerty(MENU_ITEM_PARAMETERS)
{
	azerty_keyboard.v ^=1;
}


void menu_chloe_keyboard(MENU_ITEM_PARAMETERS)
{
	chloe_keyboard.v ^=1;
	scr_z88_cpc_load_keymap();
}

void menu_hardware_redefine_keys_set_keys(MENU_ITEM_PARAMETERS)
{


        z80_byte tecla_original,tecla_redefinida;
	tecla_original=lista_teclas_redefinidas[hardware_redefine_keys_opcion_seleccionada].tecla_original;
	tecla_redefinida=lista_teclas_redefinidas[hardware_redefine_keys_opcion_seleccionada].tecla_redefinida;

        char buffer_caracter_original[2];
	char buffer_caracter_redefinida[2];

	if (tecla_original==0) {
		buffer_caracter_original[0]=0;
		buffer_caracter_redefinida[0]=0;
	}


	else {
		buffer_caracter_original[0]=(tecla_original>=32 && tecla_original <=127 ? tecla_original : '?');
		buffer_caracter_redefinida[0]=(tecla_redefinida>=32 && tecla_redefinida <=127 ? tecla_redefinida : '?');
	}

        buffer_caracter_original[1]=0;
        buffer_caracter_redefinida[1]=0;




        menu_ventana_scanf("Original key",buffer_caracter_original,2);
        tecla_original=buffer_caracter_original[0];

	if (tecla_original==0) {
		lista_teclas_redefinidas[hardware_redefine_keys_opcion_seleccionada].tecla_original=0;
		return;
	}


        menu_ventana_scanf("Destination key",buffer_caracter_redefinida,2);
        tecla_redefinida=buffer_caracter_redefinida[0];

	if (tecla_redefinida==0) {
                lista_teclas_redefinidas[hardware_redefine_keys_opcion_seleccionada].tecla_original=0;
                return;
        }


	lista_teclas_redefinidas[hardware_redefine_keys_opcion_seleccionada].tecla_original=tecla_original;
	lista_teclas_redefinidas[hardware_redefine_keys_opcion_seleccionada].tecla_redefinida=tecla_redefinida;

}




void menu_hardware_redefine_keys(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_hardware_redefine_keys;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char buffer_texto[40];

                int i;
                for (i=0;i<MAX_TECLAS_REDEFINIDAS;i++) {
				z80_byte tecla_original=lista_teclas_redefinidas[i].tecla_original;
				z80_byte tecla_redefinida=lista_teclas_redefinidas[i].tecla_redefinida;
                        if (tecla_original) {
					sprintf(buffer_texto,"Key %c to %c",(tecla_original>=32 && tecla_original <=127 ? tecla_original : '?'),
					(tecla_redefinida>=32 && tecla_redefinida <=127 ? tecla_redefinida : '?') );

								}

                        else {
                                sprintf(buffer_texto,"Unused entry");
                        }



                        if (i==0) menu_add_item_menu_inicial_format(&array_menu_hardware_redefine_keys,MENU_OPCION_NORMAL,menu_hardware_redefine_keys_set_keys,NULL,buffer_texto);
                        else menu_add_item_menu_format(array_menu_hardware_redefine_keys,MENU_OPCION_NORMAL,menu_hardware_redefine_keys_set_keys,NULL,buffer_texto);


                        menu_add_item_menu_tooltip(array_menu_hardware_redefine_keys,"Redefine the key");
                        menu_add_item_menu_ayuda(array_menu_hardware_redefine_keys,"Indicates which key on the Spectrum keyboard is sent when "
                                                "pressed the original key");
                }



                menu_add_item_menu(array_menu_hardware_redefine_keys,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_hardware_redefine_keys,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_hardware_redefine_keys);

                retorno_menu=menu_dibuja_menu(&hardware_redefine_keys_opcion_seleccionada,&item_seleccionado,array_menu_hardware_redefine_keys,"Redefine keys" );

                cls_menu_overlay();


if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}



void menu_hardware_inves_poke(MENU_ITEM_PARAMETERS)
{


	char string_poke[4];

	sprintf (string_poke,"%d",last_inves_low_ram_poke_menu);


	menu_ventana_scanf("Poke low RAM with",string_poke,4);

	last_inves_low_ram_poke_menu=parse_string_to_number(string_poke);

	modificado_border.v=1;

	poke_inves_rom(last_inves_low_ram_poke_menu);

}

int menu_inves_cond(void)
{
        if (MACHINE_IS_INVES) return 1;
        else return 0;
}

int menu_inves_cond_realvideo(void)
{
	if (!menu_inves_cond()) return 0;
	return rainbow_enabled.v;

}


void menu_hardware_ace_ramtop(MENU_ITEM_PARAMETERS)
{


        char string_ramtop[3];

        //int valor_antes_ramtop=((ramtop_ace-16383)/1024)+3;
				int valor_antes_ramtop=get_ram_ace();

	//printf ("ramtop: %d\n",valor_antes_ramtop);

        sprintf (string_ramtop,"%d",valor_antes_ramtop);


        menu_ventana_scanf("RAM? (3, 19 or 35)",string_ramtop,3);

        int valor_leido_ramtop=parse_string_to_number(string_ramtop);

        //si mismo valor volver
        if (valor_leido_ramtop==valor_antes_ramtop) return;

	if (valor_leido_ramtop!=3 && valor_leido_ramtop!=19 && valor_leido_ramtop!=35) {
		debug_printf (VERBOSE_ERR,"Invalid RAM value");
		return;
	}


	set_ace_ramtop(valor_leido_ramtop);
        reset_cpu();

}



void menu_hardware_zx8081_ramtop(MENU_ITEM_PARAMETERS)
{


        char string_ramtop[3];

	//int valor_antes_ramtop=(ramtop_zx8081-16383)/1024;
	int valor_antes_ramtop=zx8081_get_standard_ram();

        sprintf (string_ramtop,"%d",valor_antes_ramtop);


        menu_ventana_scanf("Standard RAM",string_ramtop,3);

        int valor_leido_ramtop=parse_string_to_number(string_ramtop);

	//si mismo valor volver
	if (valor_leido_ramtop==valor_antes_ramtop) return;


	if (valor_leido_ramtop>0 && valor_leido_ramtop<17) {
		set_zx8081_ramtop(valor_leido_ramtop);
		//ramtop_zx8081=16383+1024*valor_leido_ramtop;
		reset_cpu();
	}

}

void menu_hardware_zx8081_ram_in_8192(MENU_ITEM_PARAMETERS)
{
	ram_in_8192.v ^=1;
}

void menu_hardware_zx8081_ram_in_49152(MENU_ITEM_PARAMETERS)
{
        if (ram_in_49152.v==0) enable_ram_in_49152();
        else ram_in_49152.v=0;

}

void menu_hardware_zx8081_ram_in_32768(MENU_ITEM_PARAMETERS)
{

	if (ram_in_32768.v==0) enable_ram_in_32768();
	else {
		ram_in_32768.v=0;
		ram_in_49152.v=0;
	}

}



void menu_display_zx8081_wrx(MENU_ITEM_PARAMETERS)
{
	if (wrx_present.v) {
		disable_wrx();
	}

	else {
		enable_wrx();
	}
	//wrx_present.v ^=1;
}


/*
void menu_display_zx8081_hrg(void)
{
        hrg_enabled.v ^=1;

	if (hrg_enabled.v) {
		menu_warn_message("Remember enable AFTER","loading hrg driver.","Setting RAMTOP to 22959","Follow next game with LOAD");
	}
}
*/


/*
void menu_display_t_offset_wrx(void)
{
	offset_zx8081_t_estados ++;
	if (offset_zx8081_t_estados>=25) offset_zx8081_t_estados=-16;
}
*/

void menu_display_x_offset(MENU_ITEM_PARAMETERS)
{
	offset_zx8081_t_coordx +=8;
        if (offset_zx8081_t_coordx>=30*8) offset_zx8081_t_coordx=-30*8;
}

/*
void menu_hardware_inves_delay_factor(MENU_ITEM_PARAMETERS)
{

	inves_ula_delay_factor++;
	if (inves_ula_delay_factor==5) inves_ula_delay_factor=1;
}
*/

//OLD: Solo permitimos autofire para Kempston, Fuller ,Zebra y mikrogen, para evitar que con cursor o con sinclair se este mandando una tecla y dificulte moverse por el menu
int menu_hardware_autofire_cond(void)
{
	//if (joystick_emulation==JOYSTICK_FULLER || joystick_emulation==JOYSTICK_KEMPSTON || joystick_emulation==JOYSTICK_ZEBRA || joystick_emulation==JOYSTICK_MIKROGEN) return 1;
	//return 0;

	return 1;
}


void menu_hardware_realjoystick_event_button(MENU_ITEM_PARAMETERS)
{
        menu_simple_ventana("Redefine event","Please press the button/axis");
	all_interlace_scr_refresca_pantalla();

	//Para xwindows hace falta esto, sino no refresca
	 scr_actualiza_tablas_teclado();

        //redefinir evento
        if (!realjoystick_redefine_event(hardware_realjoystick_event_opcion_seleccionada)) {
		//se ha salido con tecla. ver si es ESC
		if ((puerto_especial1&1)==0) {
			//desasignar evento
			realjoystick_events_array[hardware_realjoystick_event_opcion_seleccionada].asignado.v=0;
		}
	}

        cls_menu_overlay();
}

void menu_hardware_realjoystick_keys_button(MENU_ITEM_PARAMETERS)
{


        z80_byte caracter;

        char buffer_caracter[2];
        buffer_caracter[0]=0;

        menu_ventana_scanf("Please write the key",buffer_caracter,2);


        caracter=buffer_caracter[0];

        if (caracter==0) {
		//desasignamos
		realjoystick_keys_array[hardware_realjoystick_keys_opcion_seleccionada].asignado.v=0;
		return;
	}

	cls_menu_overlay();


        menu_simple_ventana("Redefine key","Please press the button/axis");
        all_interlace_scr_refresca_pantalla();

        //Para xwindows hace falta esto, sino no refresca
        scr_actualiza_tablas_teclado();

        //redefinir boton a tecla
        if (!realjoystick_redefine_key(hardware_realjoystick_keys_opcion_seleccionada,caracter)) {
		//se ha salido con tecla. ver si es ESC
                if ((puerto_especial1&1)==0) {
                        //desasignar evento
			realjoystick_keys_array[hardware_realjoystick_keys_opcion_seleccionada].asignado.v=0;
                }
        }



}

void menu_print_text_axis(char *buffer,int button_type,int button_number)
{

	char buffer_axis[2];

	if (button_type==0) sprintf (buffer_axis,"%s","");
                                        //este sprintf se hace asi para evitar warnings al compilar

	if (button_type<0) sprintf (buffer_axis,"-");
	if (button_type>0) sprintf (buffer_axis,"+");

	sprintf(buffer,"%s%d",buffer_axis,button_number);

}


void menu_hardware_realjoystick_clear_keys(MENU_ITEM_PARAMETERS)
{
        if (menu_confirm_yesno_texto("Clear list","Sure?")==1) {
                realjoystick_clear_keys_array();
        }
}


void menu_hardware_realjoystick_keys(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_hardware_realjoystick_keys;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char buffer_texto[40];
                char buffer_texto_boton[10];

                int i;
                for (i=0;i<MAX_KEYS_JOYSTICK;i++) {
                        if (realjoystick_keys_array[i].asignado.v) {
				menu_print_text_axis(buffer_texto_boton,realjoystick_keys_array[i].button_type,realjoystick_keys_array[i].button);

				z80_byte c=realjoystick_keys_array[i].caracter;
				if (c>=32 && c<=127) sprintf (buffer_texto,"Button %s sends: %c",buffer_texto_boton,c);
				else sprintf (buffer_texto,"Button %s sends: (%d)",buffer_texto_boton,c);
                        }

                        else {
                                sprintf(buffer_texto,"Unused entry");
			}



                        if (i==0) menu_add_item_menu_inicial_format(&array_menu_hardware_realjoystick_keys,MENU_OPCION_NORMAL,menu_hardware_realjoystick_keys_button,NULL,buffer_texto);
                        else menu_add_item_menu_format(array_menu_hardware_realjoystick_keys,MENU_OPCION_NORMAL,menu_hardware_realjoystick_keys_button,NULL,buffer_texto);


                        menu_add_item_menu_tooltip(array_menu_hardware_realjoystick_keys,"Redefine the button");
                        menu_add_item_menu_ayuda(array_menu_hardware_realjoystick_keys,"Indicates which key on the Spectrum keyboard is sent when "
						"pressed the button/axis on the real joystick");
                }

                menu_add_item_menu(array_menu_hardware_realjoystick_keys,"",MENU_OPCION_SEPARADOR,NULL,NULL);
		menu_add_item_menu_format(array_menu_hardware_realjoystick_keys,MENU_OPCION_NORMAL,menu_hardware_realjoystick_clear_keys,NULL,"Clear list");


                menu_add_item_menu(array_menu_hardware_realjoystick_keys,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_hardware_realjoystick_keys,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_hardware_realjoystick_keys);

                retorno_menu=menu_dibuja_menu(&hardware_realjoystick_keys_opcion_seleccionada,&item_seleccionado,array_menu_hardware_realjoystick_keys,"Joystick to key" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}



void menu_hardware_realjoystick_clear_events(MENU_ITEM_PARAMETERS)
{
	if (menu_confirm_yesno_texto("Clear list","Sure?")==1) {
		realjoystick_clear_events_array();
	}
}


void menu_hardware_realjoystick_event(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_hardware_realjoystick_event;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char buffer_texto[40];
                char buffer_texto_boton[10];

                int i;
                for (i=0;i<MAX_EVENTS_JOYSTICK;i++) {
                        if (realjoystick_events_array[i].asignado.v) {
				menu_print_text_axis(buffer_texto_boton,realjoystick_events_array[i].button_type,realjoystick_events_array[i].button);
                        }

                        else {
                                sprintf(buffer_texto_boton,"None");
                        }

                        sprintf (buffer_texto,"Button for %s: %s",realjoystick_event_names[i],buffer_texto_boton);


                        if (i==0) menu_add_item_menu_inicial_format(&array_menu_hardware_realjoystick_event,MENU_OPCION_NORMAL,menu_hardware_realjoystick_event_button,NULL,buffer_texto);
                        else menu_add_item_menu_format(array_menu_hardware_realjoystick_event,MENU_OPCION_NORMAL,menu_hardware_realjoystick_event_button,NULL,buffer_texto);


                        menu_add_item_menu_tooltip(array_menu_hardware_realjoystick_event,"Redefine the action");
                        menu_add_item_menu_ayuda(array_menu_hardware_realjoystick_event,"Redefine the action");
                }

                menu_add_item_menu(array_menu_hardware_realjoystick_event,"",MENU_OPCION_SEPARADOR,NULL,NULL);
		menu_add_item_menu_format(array_menu_hardware_realjoystick_event,MENU_OPCION_NORMAL,menu_hardware_realjoystick_clear_events,NULL,"Clear list");


                menu_add_item_menu(array_menu_hardware_realjoystick_event,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_hardware_realjoystick_event,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_hardware_realjoystick_event);

                retorno_menu=menu_dibuja_menu(&hardware_realjoystick_event_opcion_seleccionada,&item_seleccionado,array_menu_hardware_realjoystick_event,"Joystick to event" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}


void menu_hardware_realjoystick(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_hardware_realjoystick;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                menu_add_item_menu_inicial_format(&array_menu_hardware_realjoystick,MENU_OPCION_NORMAL,menu_hardware_realjoystick_event,NULL,"Joystick to events");

                menu_add_item_menu_tooltip(array_menu_hardware_realjoystick,"Define which events generate every button/movement of the joystick");
                menu_add_item_menu_ayuda(array_menu_hardware_realjoystick,"Define which events generate every button/movement of the joystick");



		menu_add_item_menu_format(array_menu_hardware_realjoystick,MENU_OPCION_NORMAL,menu_hardware_realjoystick_keys,NULL,"Joystick to keys");

                menu_add_item_menu_tooltip(array_menu_hardware_realjoystick,"Define which press key generate every button/movement of the joystick");
                menu_add_item_menu_ayuda(array_menu_hardware_realjoystick,"Define which press key generate every button/movement of the joystick");




                menu_add_item_menu(array_menu_hardware_realjoystick,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_hardware_realjoystick,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_hardware_realjoystick);

                retorno_menu=menu_dibuja_menu(&hardware_realjoystick_opcion_seleccionada,&item_seleccionado,array_menu_hardware_realjoystick,"Real joystick emulation" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}

void menu_hardware_joystick(MENU_ITEM_PARAMETERS)
{
	if (joystick_emulation==JOYSTICK_TOTAL) joystick_emulation=0;
	else joystick_emulation++;

	if (menu_hardware_autofire_cond()==0) {
		//desactivamos autofire
		joystick_autofire_frequency=0;
		//y ponemos tecla fire a 0, por si se habia quedado activa
		puerto_especial_joystick=0;
	}
}

void menu_hardware_gunstick(MENU_ITEM_PARAMETERS)
{
        if (gunstick_emulation==GUNSTICK_TOTAL) gunstick_emulation=0;
	else gunstick_emulation++;
}

void menu_hardware_gunstick_range_x(MENU_ITEM_PARAMETERS)
{
	if (gunstick_range_x==256) gunstick_range_x=1;
	else gunstick_range_x *=2;
}

void menu_hardware_gunstick_range_y(MENU_ITEM_PARAMETERS)
{
        if (gunstick_range_y==64) gunstick_range_y=1;
        else gunstick_range_y *=2;
}

void menu_hardware_gunstick_y_offset(MENU_ITEM_PARAMETERS)
{
	if (gunstick_y_offset==32) gunstick_y_offset=0;
	else gunstick_y_offset +=4;
}

void menu_hardware_gunstick_solo_brillo(MENU_ITEM_PARAMETERS)
{
	gunstick_solo_brillo ^=1;
}

int menu_hardware_gunstick_aychip_cond(void)
{
	if (gunstick_emulation==GUNSTICK_AYCHIP) return 1;
	else return 0;
}

void menu_ula_contend(MENU_ITEM_PARAMETERS)
{
	contend_enabled.v ^=1;

	inicializa_tabla_contend();

}


void menu_ula_late_timings(MENU_ITEM_PARAMETERS)
{
	ula_late_timings.v ^=1;
	inicializa_tabla_contend();
}


void menu_ula_im2_slow(MENU_ITEM_PARAMETERS)
{
        ula_im2_slow.v ^=1;
}


void menu_ula_pentagon_timing(MENU_ITEM_PARAMETERS)
{
	if (pentagon_timing.v) {
		contend_enabled.v=1;
		ula_disable_pentagon_timing();
	}

	else {
		contend_enabled.v=0;
		ula_enable_pentagon_timing();
	}


}


void menu_hardware_autofire(MENU_ITEM_PARAMETERS)
{
	if (joystick_autofire_frequency==0) joystick_autofire_frequency=1;
	else if (joystick_autofire_frequency==1) joystick_autofire_frequency=2;
	else if (joystick_autofire_frequency==2) joystick_autofire_frequency=5;
	else if (joystick_autofire_frequency==5) joystick_autofire_frequency=10;
	else if (joystick_autofire_frequency==10) joystick_autofire_frequency=25;
	else if (joystick_autofire_frequency==25) joystick_autofire_frequency=50;

	else joystick_autofire_frequency=0;
}

void menu_hardware_kempston_mouse(MENU_ITEM_PARAMETERS)
{
	kempston_mouse_emulation.v ^=1;
}


int menu_hardware_realjoystick_cond(void)
{
	return realjoystick_present.v;
}

int menu_hardware_keyboard_issue_cond(void)
{
	if (MACHINE_IS_SPECTRUM) return 1;
	return 0;
}

void menu_hardware_keymap_z88_cpc(MENU_ITEM_PARAMETERS) {
	//solo hay dos tipos
	z88_cpc_keymap_type ^=1;
	scr_z88_cpc_load_keymap();
}


void menu_hardware_memory_refresh(MENU_ITEM_PARAMETERS)
{
	if (machine_emulate_memory_refresh==0) {
		set_peek_byte_function_ram_refresh();
		machine_emulate_memory_refresh=1;
	}

	else {
		reset_peek_byte_function_ram_refresh();
                machine_emulate_memory_refresh=0;
	}
}

void menu_spritechip(MENU_ITEM_PARAMETERS)
{
	if (spritechip_enabled.v) spritechip_disable();
	else spritechip_enable();
}

void menu_zxuno_spi_write_enable(MENU_ITEM_PARAMETERS)
{
	if (zxuno_flash_write_to_disk_enable.v==0) {
           if (menu_confirm_yesno_texto("Will flush prev. changes","Enable?")==1) zxuno_flash_write_to_disk_enable.v=1;

	}

	else zxuno_flash_write_to_disk_enable.v=0;
}

void menu_zxuno_spi_flash_file(MENU_ITEM_PARAMETERS)
{
	char *filtros[2];

        filtros[0]="flash";
        filtros[1]=0;


        if (menu_filesel("Select Flash File",filtros,zxuno_flash_spi_name)==1) {

                //Ver si archivo existe y preguntar
                /*if (si_existe_archivo(zxuno_flash_spi_name) ) {

                        if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) return;

                }
		*/

		if (si_existe_archivo(zxuno_flash_spi_name) ) {

			if (menu_confirm_yesno_texto("File exists","Reload SPI Flash from file?")) {

				//Decir que no hay que hacer flush anteriores
				zxuno_flash_must_flush_to_disk=0;

				//Y sobreescribir ram spi flash con lo que tiene el archivo de disco
				zxuno_load_spi_flash();
			}

		}

		else {

			//Si archivo nuevo,
			//volcar contenido de la memoria flash en ram aqui
			//Suponemos que permisos de escritura estan activos
			zxuno_flash_must_flush_to_disk=1;
		}

        }

	//Sale con ESC
        else {
		//dejar archivo por defecto
		zxuno_flash_spi_name[0]=0;

		//Y por defecto solo lectura
		zxuno_flash_write_to_disk_enable.v=0;

		if (menu_confirm_yesno_texto("Default SPI Flash file","Reload SPI Flash from file?")) {
			zxuno_load_spi_flash();
                }

        }
}

void menu_storage_mmc_emulation(MENU_ITEM_PARAMETERS)
{
	if (mmc_enabled.v) mmc_disable();
	else mmc_enable();
}


int menu_storage_mmc_emulation_cond(void)
{
        if (mmc_file_name[0]==0) return 0;
        else return 1;
}

int menu_storage_mmc_if_enabled_cond(void)
{
	return mmc_enabled.v;
}

void menu_storage_zxmmc_emulation(MENU_ITEM_PARAMETERS)
{
	zxmmc_emulation.v ^=1;
}

void menu_storage_divmmc_mmc_ports_emulation(MENU_ITEM_PARAMETERS)
{
        if (divmmc_mmc_ports_enabled.v) divmmc_mmc_ports_disable();
        else divmmc_mmc_ports_enable();
}

void menu_storage_divmmc_diviface(MENU_ITEM_PARAMETERS)
{
	if (divmmc_diviface_enabled.v) divmmc_diviface_disable();
	else {
		divmmc_diviface_enable();
                //Tambien activamos puertos si esta mmc activado. Luego si quiere el usuario que los desactive
                if (mmc_enabled.v) divmmc_mmc_ports_enable();
	}
}


void menu_storage_mmc_file(MENU_ITEM_PARAMETERS)
{

	mmc_disable();

        char *filtros[2];

        filtros[0]="mmc";
        filtros[1]=0;


        if (menu_filesel("Select MMC File",filtros,mmc_file_name)==1) {
		if (!si_existe_archivo(mmc_file_name)) {
			if (menu_confirm_yesno_texto("File does not exist","Create?")==0) {
                                mmc_file_name[0]=0;
                                return;
                        }

			//Preguntar tamanyo en MB
			char string_tamanyo[5];
			sprintf (string_tamanyo,"32");
			menu_ventana_scanf("Size (in MB)",string_tamanyo,5);
			int size=parse_string_to_number(string_tamanyo);
			if (size<1) {
				debug_printf (VERBOSE_ERR,"Invalid file size");
				mmc_file_name[0]=0;
                                return;
			}

			if (size>=1024) {
				menu_warn_message("Using MMC bigger than 1 GB can be very slow");
			}


			//Crear archivo vacio
		        FILE *ptr_mmcfile;
			ptr_mmcfile=fopen(mmc_file_name,"wb");

		        long int totalsize=size;
			totalsize=totalsize*1024*1024;
			z80_byte valor_grabar=0;

		        if (ptr_mmcfile!=NULL) {
				while (totalsize) {
					fwrite(&valor_grabar,1,1,ptr_mmcfile);
					totalsize--;
				}
		                fclose(ptr_mmcfile);
		        }

		}

		else {
			//Comprobar aqui tambien el tamanyo
			long int size=get_file_size(mmc_file_name);
			if (size>1073741824L) {
				menu_warn_message("Using MMC bigger than 1 GB can be very slow");
                        }
		}


        }
        //Sale con ESC
        else {
                //Quitar nombre
                mmc_file_name[0]=0;


        }
}

void menu_storage_zxpand_enable(MENU_ITEM_PARAMETERS)
{
	if (zxpand_enabled.v) zxpand_disable();
	else zxpand_enable();
}

void menu_storage_zxpand_root_dir(MENU_ITEM_PARAMETERS)
{

        char *filtros[2];

        filtros[0]="";
        filtros[1]=0;


        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        int ret;


	char nada[PATH_MAX];

        //Obtenemos ultimo directorio visitado
	menu_filesel_chdir(zxpand_root_dir);


        ret=menu_filesel("Enter dir and press ESC",filtros,nada);


	//Si sale con ESC
	if (ret==0) {
		//Directorio root zxpand
		sprintf (zxpand_root_dir,"%s",menu_filesel_last_directory_seen);
		debug_printf (VERBOSE_DEBUG,"Selected directory: %s",zxpand_root_dir);

        	//directorio zxpand vacio
	        zxpand_cwd[0]=0;

	}


        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);


}


void menu_timexcart_load(MENU_ITEM_PARAMETERS)
{

        char *filtros[2];

        filtros[0]="dck";

        filtros[1]=0;



        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de ultimo archivo
        //si no hay directorio, vamos a rutas predefinidas
        if (last_timex_cart[0]==0) menu_chdir_sharedfiles();

        else {
                char directorio[PATH_MAX];
                util_get_dir(last_timex_cart,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }


        int ret;

        ret=menu_filesel("Select Cartridge",filtros,last_timex_cart);
        //volvemos a directorio inicial
		menu_filesel_chdir(directorio_actual);


        if (ret==1) {
		//                sprintf (last_timex_cart,"%s",timexcart_load_file);

                //sin overlay de texto, que queremos ver las franjas de carga con el color normal (no apagado)
                reset_menu_overlay_function();


                        timex_insert_dck_cartridge(last_timex_cart);

                //restauramos modo normal de texto de menu
                set_menu_overlay_function(normal_overlay_texto_menu);

                //Y salimos de todos los menus
                salir_todos_menus=1;
        }


}


void menu_timexcart_eject(MENU_ITEM_PARAMETERS)
{
	timex_empty_dock_space();
	menu_generic_message("Eject Cartridge","OK. Cartridge ejected");
}

void menu_dandanator_rom_file(MENU_ITEM_PARAMETERS)
{
	dandanator_disable();

        char *filtros[2];

        filtros[0]="rom";
        filtros[1]=0;


        if (menu_filesel("Select dandanator File",filtros,dandanator_rom_file_name)==1) {
                if (!si_existe_archivo(dandanator_rom_file_name)) {
                        menu_error_message("File does not exist");
                        dandanator_rom_file_name[0]=0;
                        return;



                }

                else {
                        //Comprobar aqui tambien el tamanyo
                        long int size=get_file_size(dandanator_rom_file_name);
                        if (size!=DANDANATOR_SIZE) {
                                menu_error_message("ROM file must be 512 KB lenght");
                                dandanator_rom_file_name[0]=0;
                                return;
                        }
                }


        }
        //Sale con ESC
        else {
                //Quitar nombre
                dandanator_rom_file_name[0]=0;


        }

}

int menu_storage_dandanator_emulation_cond(void)
{
	if (dandanator_rom_file_name[0]==0) return 0;
        return 1;
}

int menu_storage_dandanator_press_button_cond(void)
{
	return dandanator_enabled.v;
}


void menu_storage_dandanator_emulation(MENU_ITEM_PARAMETERS)
{
	if (dandanator_enabled.v) dandanator_disable();
	else dandanator_enable();
}

void menu_storage_dandanator_press_button(MENU_ITEM_PARAMETERS)
{
	dandanator_press_button();
	//Y salimos de todos los menus
	salir_todos_menus=1;

}

void menu_dandanator(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_dandanator;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char string_dandanator_file_shown[13];


                        menu_tape_settings_trunc_name(dandanator_rom_file_name,string_dandanator_file_shown,13);
                        menu_add_item_menu_inicial_format(&array_menu_dandanator,MENU_OPCION_NORMAL,menu_dandanator_rom_file,NULL,"~~ROM File: %s",string_dandanator_file_shown);
                        menu_add_item_menu_shortcut(array_menu_dandanator,'r');
                        menu_add_item_menu_tooltip(array_menu_dandanator,"ROM Emulation file");
                        menu_add_item_menu_ayuda(array_menu_dandanator,"ROM Emulation file");


                        			menu_add_item_menu_format(array_menu_dandanator,MENU_OPCION_NORMAL,menu_storage_dandanator_emulation,menu_storage_dandanator_emulation_cond,"~~ZX Dandanator Enabled: %s", (dandanator_enabled.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_dandanator,'d');
                        menu_add_item_menu_tooltip(array_menu_dandanator,"Enable dandanator");
                        menu_add_item_menu_ayuda(array_menu_dandanator,"Enable dandanator");


			menu_add_item_menu_format(array_menu_dandanator,MENU_OPCION_NORMAL,menu_storage_dandanator_press_button,menu_storage_dandanator_press_button_cond,"~~Press button");
			menu_add_item_menu_shortcut(array_menu_dandanator,'p');
                        menu_add_item_menu_tooltip(array_menu_dandanator,"Press button");
                        menu_add_item_menu_ayuda(array_menu_dandanator,"Press button");


                                menu_add_item_menu(array_menu_dandanator,"",MENU_OPCION_SEPARADOR,NULL,NULL);

                menu_add_ESC_item(array_menu_dandanator);

                retorno_menu=menu_dibuja_menu(&dandanator_opcion_seleccionada,&item_seleccionado,array_menu_dandanator,"ZX Dandanator settings" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}

void menu_superupgrade_rom_file(MENU_ITEM_PARAMETERS)
{
        superupgrade_disable();

        char *filtros[2];

        filtros[0]="flash";
        filtros[1]=0;


        if (menu_filesel("Select Superupgrade File",filtros,superupgrade_rom_file_name)==1) {
                if (!si_existe_archivo(superupgrade_rom_file_name)) {
                        menu_error_message("File does not exist");
                        superupgrade_rom_file_name[0]=0;
                        return;



                }

                else {
                        //Comprobar aqui tambien el tamanyo
                        long int size=get_file_size(superupgrade_rom_file_name);
                        if (size!=SUPERUPGRADE_ROM_SIZE) {
                                menu_error_message("Flash file must be 512 KB lenght");
                                superupgrade_rom_file_name[0]=0;
                                return;
                        }
                }


        }
        //Sale con ESC
        else {
                //Quitar nombre
                superupgrade_rom_file_name[0]=0;

        }

}

int menu_storage_superupgrade_emulation_cond(void)
{
        if (superupgrade_rom_file_name[0]==0) return 0;
        return 1;
}


void menu_storage_superupgrade_emulation(MENU_ITEM_PARAMETERS)
{
        if (superupgrade_enabled.v) superupgrade_disable();
        else superupgrade_enable(1);
}

void menu_storage_superupgrade_internal_rom(MENU_ITEM_PARAMETERS)
{
		//superupgrade_puerto_43b ^=0x20;
		//if ( (superupgrade_puerto_43b & (32+64))==32) return 1;

		superupgrade_puerto_43b &=(255-32-64);
		superupgrade_puerto_43b |=32;
}



void menu_superupgrade(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_superupgrade;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char string_superupgrade_file_shown[13];


                        menu_tape_settings_trunc_name(superupgrade_rom_file_name,string_superupgrade_file_shown,13);
                        menu_add_item_menu_inicial_format(&array_menu_superupgrade,MENU_OPCION_NORMAL,menu_superupgrade_rom_file,NULL,"~~Flash File: %s",string_superupgrade_file_shown);
                        menu_add_item_menu_shortcut(array_menu_superupgrade,'f');
                        menu_add_item_menu_tooltip(array_menu_superupgrade,"Flash Emulation file");
                        menu_add_item_menu_ayuda(array_menu_superupgrade,"Flash Emulation file");


                        menu_add_item_menu_format(array_menu_superupgrade,MENU_OPCION_NORMAL,menu_storage_superupgrade_emulation,menu_storage_superupgrade_emulation_cond,"~~Superupgrade Enabled: %s", (superupgrade_enabled.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_superupgrade,'s');
                        menu_add_item_menu_tooltip(array_menu_superupgrade,"Enable superupgrade");
                        menu_add_item_menu_ayuda(array_menu_superupgrade,"Enable superupgrade");


												menu_add_item_menu_format(array_menu_superupgrade,MENU_OPCION_NORMAL,menu_storage_superupgrade_internal_rom,menu_storage_superupgrade_emulation_cond,"Show ~~internal ROM: %s", (si_superupgrade_muestra_rom_interna() ? "Yes" : "No"));
												menu_add_item_menu_shortcut(array_menu_superupgrade,'i');
												menu_add_item_menu_tooltip(array_menu_superupgrade,"Show internal ROM instead of Superupgrade flash");
												menu_add_item_menu_ayuda(array_menu_superupgrade,"Show internal ROM instead of Superupgrade flash");



                                menu_add_item_menu(array_menu_superupgrade,"",MENU_OPCION_SEPARADOR,NULL,NULL);

                menu_add_ESC_item(array_menu_superupgrade);


retorno_menu=menu_dibuja_menu(&superupgrade_opcion_seleccionada,&item_seleccionado,array_menu_superupgrade,"Superupgrade settings" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}




void menu_if1_settings(MENU_ITEM_PARAMETERS)
{
	if (if1_enabled.v==0) enable_if1();
	else disable_if1();
}

void menu_storage_divmmc_diviface_total_ram(MENU_ITEM_PARAMETERS)
{
	diviface_current_ram_memory_bits++;
	if (diviface_current_ram_memory_bits==7) diviface_current_ram_memory_bits=2;
}


void menu_timexcart(MENU_ITEM_PARAMETERS)
{

        menu_item *array_menu_timexcart;
        menu_item item_seleccionado;
        int retorno_menu;

        do {


                menu_add_item_menu_inicial(&array_menu_timexcart,"~~Load Cartridge",MENU_OPCION_NORMAL,menu_timexcart_load,NULL);
                menu_add_item_menu_shortcut(array_menu_timexcart,'l');
                menu_add_item_menu_tooltip(array_menu_timexcart,"Load Timex Cartridge");
                menu_add_item_menu_ayuda(array_menu_timexcart,"Supported timex cartridge formats on load:\n"
                                        "DCK");

                menu_add_item_menu(array_menu_timexcart,"~~Eject Cartridge",MENU_OPCION_NORMAL,menu_timexcart_eject,NULL);
                menu_add_item_menu_shortcut(array_menu_timexcart,'e');
                menu_add_item_menu_tooltip(array_menu_timexcart,"Eject Cartridge");
                menu_add_item_menu_ayuda(array_menu_timexcart,"Eject Cartridge");


     				menu_add_item_menu(array_menu_timexcart,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                menu_add_ESC_item(array_menu_timexcart);

                retorno_menu=menu_dibuja_menu(&timexcart_opcion_seleccionada,&item_seleccionado,array_menu_timexcart,"Timex Cartridge" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}

void menu_storage_mmc_reload(MENU_ITEM_PARAMETERS)
{
	if (mmc_read_file_to_memory()==0) {
		menu_generic_message("Reload MMC","OK. MMC file reloaded");
	}
}

void menu_divmmc_rom_file(MENU_ITEM_PARAMETERS)
{


	//desactivamos diviface divmmc. Así obligamos que el usuario tenga que activarlo de nuevo, recargando del firmware
	divmmc_diviface_disable();


        char *filtros[3];

        filtros[0]="rom";
				filtros[1]="bin";
        filtros[2]=0;


        if (menu_filesel("Select ROM File",filtros, divmmc_rom_name)==1) {
				//Nada

        }
        //Sale con ESC
        else {
                //Quitar nombre
                divmmc_rom_name[0]=0;


        }

				menu_generic_message("Change DIVMMC ROM","OK. Remember to enable DIVMMC paging to load the firmware");
}

void menu_storage_diviface_eprom_write_jumper(MENU_ITEM_PARAMETERS)
{
	diviface_eprom_write_jumper.v ^=1;
}

//menu MMC/Divmmc
void menu_mmc_divmmc(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_mmc_divmmc;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char string_mmc_file_shown[13];
								char string_divmmc_rom_file_shown[10];


                        menu_tape_settings_trunc_name(mmc_file_name,string_mmc_file_shown,13);
                        menu_add_item_menu_inicial_format(&array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_storage_mmc_file,NULL,"~~MMC File: %s",string_mmc_file_shown);
                        menu_add_item_menu_shortcut(array_menu_mmc_divmmc,'m');
                        menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"MMC Emulation file");
                        menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"MMC Emulation file");


                        menu_add_item_menu_format(array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_storage_mmc_emulation,menu_storage_mmc_emulation_cond,"MMC ~~Emulation: %s", (mmc_enabled.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_mmc_divmmc,'e');
                        menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"MMC Emulation");
                        menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"MMC Emulation");

												if (mmc_enabled.v) {
												menu_add_item_menu_format(array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_storage_mmc_reload,NULL,"Reload MMC file");
												menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"Reload MMC contents from MMC file to emulator memory");
												menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"Reload MMC contents from MMC file to emulator memory. You can modify the MMC file "
																								"outside the emulator, and reload its contents without having to disable and enable MM.");
												}

			menu_add_item_menu_format(array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_storage_divmmc_diviface,NULL,"~~DIVMMC paging: %s",(divmmc_diviface_enabled.v ? "Yes" : "No") );
                        menu_add_item_menu_shortcut(array_menu_mmc_divmmc,'d');
			menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"Enables DIVMMC paging and firmware, and DIVMMC access ports if MMC emulation is enabled");
			menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"Enables DIVMMC paging and firmware, and DIVMMC access ports if MMC emulation is enabled");

			if (divmmc_diviface_enabled.v) {
				menu_add_item_menu_format(array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_storage_divmmc_diviface_total_ram,NULL,"DIVMMC RAM: %d KB",get_diviface_total_ram() );
				menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"Changes DIVMMC RAM");
				menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"Changes DIVMMC RAM");


			}

			if (divmmc_rom_name[0]==0) sprintf (string_divmmc_rom_file_shown,"Default");
			else menu_tape_settings_trunc_name(divmmc_rom_name, string_divmmc_rom_file_shown,10);
			menu_add_item_menu_format(array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_divmmc_rom_file,NULL,"DIVMMC EPROM File: %s", string_divmmc_rom_file_shown);

			menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"Changes DIVMMC firmware eprom file");
			menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"Changes DIVMMC firmware eprom file");


			if (divmmc_diviface_enabled.v) {
				menu_add_item_menu_format(array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_storage_diviface_eprom_write_jumper,NULL,"Allow diviface writes: %s",
				(diviface_eprom_write_jumper.v ? "Yes" : "No") );
				menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"Allows writing to DivIDE/DivMMC eprom");
				menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"Allows writing to DivIDE/DivMMC eprom. Changes are lost when you exit the emulator");
			}

                        menu_add_item_menu_format(array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_storage_divmmc_mmc_ports_emulation,menu_storage_mmc_if_enabled_cond,"DIVMMC ~~ports: %s",(divmmc_mmc_ports_enabled.v ? "Yes" : "No") );
                        menu_add_item_menu_shortcut(array_menu_mmc_divmmc,'p');
                        menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"Enables DIVMMC access ports");
                        menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"Enables DIVMMC access ports. Requires enabling MMC Emulation");


                        menu_add_item_menu_format(array_menu_mmc_divmmc,MENU_OPCION_NORMAL,menu_storage_zxmmc_emulation,menu_storage_mmc_if_enabled_cond,"~~ZXMMC Enabled: %s",(zxmmc_emulation.v ? "Yes" : "No") );
                        menu_add_item_menu_shortcut(array_menu_mmc_divmmc,'z');
                        menu_add_item_menu_tooltip(array_menu_mmc_divmmc,"Access MMC using ZXMMC");
                        menu_add_item_menu_ayuda(array_menu_mmc_divmmc,"Enables ZXMMC ports to access MMC");



				menu_add_item_menu(array_menu_mmc_divmmc,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_mmc_divmmc,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_mmc_divmmc);

                retorno_menu=menu_dibuja_menu(&mmc_divmmc_opcion_seleccionada,&item_seleccionado,array_menu_mmc_divmmc,"MMC settings" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}

void menu_tape_autoloadtape(MENU_ITEM_PARAMETERS)
{
        noautoload.v ^=1;
}

void menu_tape_autoselectfileopt(MENU_ITEM_PARAMETERS)
{
        autoselect_snaptape_options.v ^=1;
}




void menu_storage_ide_emulation(MENU_ITEM_PARAMETERS)
{
        if (ide_enabled.v) ide_disable();
        else ide_enable();
}


int menu_storage_ide_emulation_cond(void)
{
        if (ide_file_name[0]==0) return 0;
        else return 1;
}

/*
void menu_storage_divide_emulation(MENU_ITEM_PARAMETERS)
{
        if (divide_enabled.v) divide_disable();
        else divide_enable();
}
*/


void menu_storage_divide_ide_ports_emulation(MENU_ITEM_PARAMETERS)
{
        if (divide_ide_ports_enabled.v) divide_ide_ports_disable();
        else divide_ide_ports_enable();
}

void menu_storage_divide_diviface(MENU_ITEM_PARAMETERS)
{
        if (divide_diviface_enabled.v) divide_diviface_disable();
        else {
                divide_diviface_enable();
                //Tambien activamos puertos si esta ide activado. Luego si quiere el usuario que los desactive
		if (ide_enabled.v) divide_ide_ports_enable();
        }
}



void menu_storage_ide_file(MENU_ITEM_PARAMETERS)
{

        ide_disable();

        char *filtros[2];

        filtros[0]="ide";
        filtros[1]=0;


        if (menu_filesel("Select IDE File",filtros,ide_file_name)==1) {
                if (!si_existe_archivo(ide_file_name)) {
                        if (menu_confirm_yesno_texto("File does not exist","Create?")==0) {
                                ide_file_name[0]=0;
                                return;
                        }

                        //Preguntar tamanyo en MB
                        char string_tamanyo[5];
                        sprintf (string_tamanyo,"32");
                        menu_ventana_scanf("Size (in MB)",string_tamanyo,5);
                        int size=parse_string_to_number(string_tamanyo);
                        if (size<1) {
                                debug_printf (VERBOSE_ERR,"Invalid file size");
                                ide_file_name[0]=0;
                                return;
                        }

                        if (size>=1024) {
                                menu_warn_message("Using IDE bigger than 1 GB can be very slow");
                        }


                        //Crear archivo vacio
                        FILE *ptr_idefile;
                        ptr_idefile=fopen(ide_file_name,"wb");

   long int totalsize=size;
                        totalsize=totalsize*1024*1024;
                        z80_byte valor_grabar=0;

                        if (ptr_idefile!=NULL) {
                                while (totalsize) {
                                        fwrite(&valor_grabar,1,1,ptr_idefile);
                                        totalsize--;
                                }
                                fclose(ptr_idefile);
                        }

                }

                else {
                        //Comprobar aqui tambien el tamanyo
                        long int size=get_file_size(ide_file_name);
                        if (size>1073741824L) {
                                menu_warn_message("Using IDE bigger than 1 GB can be very slow");
                        }
                }


        }
        //Sale con ESC
        else {
                //Quitar nombre
                ide_file_name[0]=0;


        }
}


int menu_storage_ide_if_enabled_cond(void)
{
	return ide_enabled.v;
}

void menu_eightbitsimple_enable(MENU_ITEM_PARAMETERS)
{
	if (eight_bit_simple_ide_enabled.v) eight_bit_simple_ide_disable();
	else eight_bit_simple_ide_enable();
}

void menu_atomlite_enable(MENU_ITEM_PARAMETERS)
{
        int reset=0;

        if (atomlite_enabled.v) {

                reset=menu_confirm_yesno_texto("Confirm reset","Load normal rom and reset?");

                atomlite_enabled.v=0;
        }

        else {
                reset=menu_confirm_yesno_texto("Confirm reset","Load atomlite rom and reset?");
                atomlite_enabled.v=1;
        }

        if (reset) {
                set_machine(NULL);
                cold_start_cpu_registers();
                reset_cpu();
		salir_todos_menus=1;
        }

}

void menu_storage_ide_reload(MENU_ITEM_PARAMETERS)
{
	if (ide_read_file_to_memory()==0) {
		menu_generic_message("Reload IDE","OK. IDE file reloaded");
	}
}


void menu_divide_rom_file(MENU_ITEM_PARAMETERS)
{


	//desactivamos diviface divide. Así obligamos que el usuario tenga que activarlo de nuevo, recargando del firmware
	divide_diviface_disable();


        char *filtros[3];

        filtros[0]="rom";
				filtros[1]="bin";
        filtros[2]=0;


        if (menu_filesel("Select ROM File",filtros, divide_rom_name)==1) {
				//Nada

        }
        //Sale con ESC
        else {
                //Quitar nombre
                divide_rom_name[0]=0;


        }

				menu_generic_message("Change DIVIDE ROM","OK. Remember to enable DIVIDE paging to load the firmware");
}

//menu IDE/Divide
void menu_ide_divide(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_ide_divide;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char string_ide_file_shown[13];
								char string_divide_rom_file_shown[10];





                        menu_tape_settings_trunc_name(ide_file_name,string_ide_file_shown,13);
                        menu_add_item_menu_inicial_format(&array_menu_ide_divide,MENU_OPCION_NORMAL,menu_storage_ide_file,NULL,"~~IDE File: %s",string_ide_file_shown);
                        menu_add_item_menu_shortcut(array_menu_ide_divide,'i');
                        menu_add_item_menu_tooltip(array_menu_ide_divide,"IDE Emulation file");
                        menu_add_item_menu_ayuda(array_menu_ide_divide,"IDE Emulation file");


                        menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_storage_ide_emulation,menu_storage_ide_emulation_cond,"IDE ~~Emulation: %s", (ide_enabled.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_ide_divide,'e');
                        menu_add_item_menu_tooltip(array_menu_ide_divide,"IDE Emulation");
                        menu_add_item_menu_ayuda(array_menu_ide_divide,"IDE Emulation");

												if (ide_enabled.v) {
												menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_storage_ide_reload,NULL,"Reload IDE file");
												menu_add_item_menu_tooltip(array_menu_ide_divide,"Reload IDE contents from IDE file to emulator memory");
												menu_add_item_menu_ayuda(array_menu_ide_divide,"Reload IDE contents from IDE file to emulator memory. You can modify the IDE file "
												                        "outside the emulator, and reload its contents without having to disable and enable IDE");
												}


			if (MACHINE_IS_SPECTRUM) {

	                       menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_storage_divide_diviface,NULL,"~~DIVIDE paging: %s",(divide_diviface_enabled.v ? "Yes" : "No") );
        	                menu_add_item_menu_shortcut(array_menu_ide_divide,'d');
        	                menu_add_item_menu_tooltip(array_menu_ide_divide,"Enables DIVIDE paging and firmware, and DIVIDE access ports if IDE emulation is enabled");
        	                menu_add_item_menu_ayuda(array_menu_ide_divide,"Enables DIVIDE paging and firmware, and DIVIDE access ports if IDE emulation is enabled");

	                        if (divide_diviface_enabled.v) {
	                                menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_storage_divmmc_diviface_total_ram,NULL,"DIVIDE RAM: %d KB",get_diviface_total_ram() );
        	                        menu_add_item_menu_tooltip(array_menu_ide_divide,"Changes DIVIDE RAM");
                	                menu_add_item_menu_ayuda(array_menu_ide_divide,"Changes DIVIDE RAM");



               		         }
													 if (divide_rom_name[0]==0) sprintf (string_divide_rom_file_shown,"Default");
													 else menu_tape_settings_trunc_name(divide_rom_name, string_divide_rom_file_shown,10);
													 menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_divide_rom_file,NULL,"DIVIDE EPROM File: %s", string_divide_rom_file_shown);

													 menu_add_item_menu_tooltip(array_menu_ide_divide,"Changes DIVIDE firmware eprom file");
													 menu_add_item_menu_ayuda(array_menu_ide_divide,"Changes DIVIDE firmware eprom file");

													 if (divide_diviface_enabled.v) {
										 				menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_storage_diviface_eprom_write_jumper,NULL,"Allow diviface writes: %s",
										 				(diviface_eprom_write_jumper.v ? "Yes" : "No") );
										 				menu_add_item_menu_tooltip(array_menu_ide_divide,"Allows writing to DivIDE/DivMMC eprom");
										 				menu_add_item_menu_ayuda(array_menu_ide_divide,"Allows writing to DivIDE/DivMMC eprom. Changes are lost when you exit the emulator");
										 			}



	                        menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_storage_divide_ide_ports_emulation,menu_storage_ide_if_enabled_cond,"DIVIDE ~~ports: %s",(divide_ide_ports_enabled.v ? "Yes" : "No") );
        	                menu_add_item_menu_shortcut(array_menu_ide_divide,'p');
                	        menu_add_item_menu_tooltip(array_menu_ide_divide,"Enables DIVIDE access ports");
                        	menu_add_item_menu_ayuda(array_menu_ide_divide,"Enables DIVIDE access ports. Requires enabling IDE Emulation");


			}

			if (MACHINE_IS_SPECTRUM) {
				menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_eightbitsimple_enable,menu_storage_ide_if_enabled_cond,"8-bit simple IDE: %s",(eight_bit_simple_ide_enabled.v ? "Yes" : "No") );
			}


        if (MACHINE_IS_SAM) {
                        menu_add_item_menu_format(array_menu_ide_divide,MENU_OPCION_NORMAL,menu_atomlite_enable,NULL,"~~Atom Lite enabled: %s",(atomlite_enabled.v ? "Yes" : "No" ) );
                        menu_add_item_menu_shortcut(array_menu_ide_divide,'a');
                        menu_add_item_menu_tooltip(array_menu_ide_divide,"Enable Atom Lite");
                        menu_add_item_menu_ayuda(array_menu_ide_divide,"Enable Atom Lite");
                }



                                menu_add_item_menu(array_menu_ide_divide,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_ide_divide,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_ide_divide);

                retorno_menu=menu_dibuja_menu(&ide_divide_opcion_seleccionada,&item_seleccionado,array_menu_ide_divide,"IDE settings" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}




//menu storage settings
void menu_storage_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_storage_settings;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                //char string_spi_flash_file_shown[13]; //,string_mmc_file_shown[13];


		//Primer menu sera Z88 memory slots (si Z88) o Tape settings en cualquier otro

		if (MACHINE_IS_Z88) {
			menu_add_item_menu_inicial_format(&array_menu_storage_settings,MENU_OPCION_NORMAL,menu_z88_slots,NULL,"Z88 Memory ~~Slots");
                        menu_add_item_menu_shortcut(array_menu_storage_settings,'s');

                        menu_add_item_menu_tooltip(array_menu_storage_settings,"Z88 Memory Slots");
                        menu_add_item_menu_ayuda(array_menu_storage_settings,"Selects Memory Slots to use on Z88");

		}


		else {
	            	menu_add_item_menu_inicial_format(&array_menu_storage_settings,MENU_OPCION_NORMAL,menu_tape_settings,menu_tape_settings_cond,"~~Tape");
                	menu_add_item_menu_shortcut(array_menu_storage_settings,'t');
                	menu_add_item_menu_tooltip(array_menu_storage_settings,"Select tape and options");
                	menu_add_item_menu_ayuda(array_menu_storage_settings,"Select tape for input (read) or output (write). \n"
                                                              "You use them as real tapes, with BASIC functions like LOAD and SAVE\n"
                                                              "Also select different options to change tape behaviour");
		}


		if (MACHINE_IS_TIMEX_TS2068) {
			menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_timexcart,NULL,"Timex ~~Cartridge");
			menu_add_item_menu_shortcut(array_menu_storage_settings,'c');
			menu_add_item_menu_tooltip(array_menu_storage_settings,"Timex Cartridge Settings");
			menu_add_item_menu_ayuda(array_menu_storage_settings,"Timex Cartridge Settings");
		}



		if (MACHINE_IS_ZX8081) {
			menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_storage_zxpand_enable,NULL,"ZX~~pand emulation: %s",(zxpand_enabled.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_storage_settings,'p');
			menu_add_item_menu_tooltip(array_menu_storage_settings,"Enable ZXpand emulation");
			menu_add_item_menu_ayuda(array_menu_storage_settings,"Enable ZXpand emulation");


			if (zxpand_enabled.v) {
				char string_zxpand_root_folder_shown[20];
				menu_tape_settings_trunc_name(zxpand_root_dir,string_zxpand_root_folder_shown,20);
				menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_storage_zxpand_root_dir,NULL,"~~Root dir: %s",string_zxpand_root_folder_shown);
                        	menu_add_item_menu_shortcut(array_menu_storage_settings,'r');
				menu_add_item_menu_tooltip(array_menu_storage_settings,"Sets the root directory for ZXpand filesystem");
				menu_add_item_menu_ayuda(array_menu_storage_settings,"Sets the root directory for ZXpand filesystem. "
					"Only file and folder names valid for zxpand will be shown:\n"
					"-Maximum 8 characters for name and 3 for extension\n"
					"-Files and folders will be shown always in uppercase. Folders which are not uppercase, are shown but can not be accessed\n"
					);

			}
		}

		if (MACHINE_IS_SPECTRUM) {
			//menu_tape_settings_trunc_name(mmc_file_name,string_mmc_file_shown,13);
			menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_mmc_divmmc,NULL,"~~MMC");
			menu_add_item_menu_shortcut(array_menu_storage_settings,'m');
			menu_add_item_menu_tooltip(array_menu_storage_settings,"MMC, DivMMC and ZXMMC settings");
			menu_add_item_menu_ayuda(array_menu_storage_settings,"MMC, DivMMC and ZXMMC settings");



		}



		if (MACHINE_IS_SPECTRUM || MACHINE_IS_SAM) {

                        menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_ide_divide,NULL,"~~IDE");
                        menu_add_item_menu_shortcut(array_menu_storage_settings,'i');
                        menu_add_item_menu_tooltip(array_menu_storage_settings,"IDE, DivIDE and 8-bit simple settings");
                        menu_add_item_menu_ayuda(array_menu_storage_settings,"IDE, DivIDE and 8-bit simple settings");


		}




     if (MACHINE_IS_SPECTRUM && !MACHINE_IS_ZXUNO) {

                        menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_dandanator,NULL,"ZX ~~Dandanator");
                        menu_add_item_menu_shortcut(array_menu_storage_settings,'d');
                        menu_add_item_menu_tooltip(array_menu_storage_settings,"ZX Dandanator settings");
                        menu_add_item_menu_ayuda(array_menu_storage_settings,"ZX Dandanator settings");

                }

		if (superupgrade_supported_machine() ) {
			    menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_superupgrade,NULL,"Speccy Superup~~grade");
                        menu_add_item_menu_shortcut(array_menu_storage_settings,'g');
                        menu_add_item_menu_tooltip(array_menu_storage_settings,"Superupgrade settings");
                        menu_add_item_menu_ayuda(array_menu_storage_settings,"Superupgrade settings");

		}



                /*menu_add_item_menu(array_menu_storage_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);

								menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_tape_autoloadtape,NULL,"~~Autoload medium: %s", (noautoload.v==0 ? "On" : "Off"));
                menu_add_item_menu_shortcut(array_menu_storage_settings,'a');

                menu_add_item_menu_tooltip(array_menu_storage_settings,"Autoload medium and set machine");
                menu_add_item_menu_ayuda(array_menu_storage_settings,"This option first change to the machine that handles the medium file type selected (tape, cartridge, etc), resets it, set some default machine values, and then, it sends "
                                                "a LOAD sentence to load the medium\n"
                                                "Note: The machine is changed only using smartload. Inserting a medium only resets the machine but does not change it");


                menu_add_item_menu_format(array_menu_storage_settings,MENU_OPCION_NORMAL,menu_tape_autoselectfileopt,NULL,"A~~utoselect medium opts: %s", (autoselect_snaptape_options.v==1 ? "On" : "Off"));
                menu_add_item_menu_shortcut(array_menu_storage_settings,'u');
                menu_add_item_menu_tooltip(array_menu_storage_settings,"Detect options for the selected medium file and the needed machine");
                menu_add_item_menu_ayuda(array_menu_storage_settings,"The emulator uses a database for different included programs "
                                "(and some other not included) and reads .config files to select emulator settings and the needed machine "
                                "to run them. If you disable this, the database nor the .config files are read");
																*/



                menu_add_item_menu(array_menu_storage_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_storage_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_storage_settings);

                retorno_menu=menu_dibuja_menu(&storage_settings_opcion_seleccionada,&item_seleccionado,array_menu_storage_settings,"Storage" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}

void menu_ula_disable_rom_paging(MENU_ITEM_PARAMETERS)
{
	ula_disabled_rom_paging.v ^=1;
}

void menu_ula_disable_ram_paging(MENU_ITEM_PARAMETERS)
{
        ula_disabled_ram_paging.v ^=1;
}




//menu ula settings
void menu_ula_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_ula_settings;
        menu_item item_seleccionado;
        int retorno_menu;
        do {


		menu_add_item_menu_inicial_format(&array_menu_ula_settings,MENU_OPCION_NORMAL,menu_ula_advanced,menu_cond_realvideo,"~~Advanced timing settings");
                menu_add_item_menu_shortcut(array_menu_ula_settings,'a');
                menu_add_item_menu_tooltip(array_menu_ula_settings,"Advanced timing settings. Requires realvideo");
                menu_add_item_menu_ayuda(array_menu_ula_settings,"Change and view some timings for the machine. Requires realvideo");

#ifdef EMULATE_CONTEND

                if (MACHINE_IS_SPECTRUM) {

                        menu_add_item_menu_format(array_menu_ula_settings,MENU_OPCION_NORMAL,menu_ula_contend,NULL,"~~Contended memory: %s", (contend_enabled.v==1 ? "On" : "Off"));
                        menu_add_item_menu_shortcut(array_menu_ula_settings,'c');
                        menu_add_item_menu_tooltip(array_menu_ula_settings,"Enable contended memory & ports emulation");
                        menu_add_item_menu_ayuda(array_menu_ula_settings,"Contended memory & ports is the native way of some of the emulated machines");

                        menu_add_item_menu_format(array_menu_ula_settings,MENU_OPCION_NORMAL,menu_ula_late_timings,NULL,"ULA ~~timing: %s",(ula_late_timings.v ? "Late" : "Early"));
                        menu_add_item_menu_shortcut(array_menu_ula_settings,'t');
                        menu_add_item_menu_tooltip(array_menu_ula_settings,"Use ULA early or late timings");
                        menu_add_item_menu_ayuda(array_menu_ula_settings,"Late timings have the contended memory table start one t-state later");


			menu_add_item_menu_format(array_menu_ula_settings,MENU_OPCION_NORMAL,menu_ula_im2_slow,NULL,"ULA IM2 slow: %s",(ula_im2_slow.v ? "Yes" : "No"));
			menu_add_item_menu_tooltip(array_menu_ula_settings,"Add one t-state when an IM2 is fired");
			menu_add_item_menu_ayuda(array_menu_ula_settings,"It improves visualization on some demos, like overscan, ula128 and scroll2017");
                }

#endif


		if (MACHINE_IS_SPECTRUM) {

                        menu_add_item_menu_format(array_menu_ula_settings,MENU_OPCION_NORMAL,menu_ula_pentagon_timing,NULL,"~~Pentagon timing: %s",(pentagon_timing.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_ula_settings,'p');
                        menu_add_item_menu_tooltip(array_menu_ula_settings,"Enable Pentagon timings");
                        menu_add_item_menu_ayuda(array_menu_ula_settings,"Pentagon does not have contended memory/ports and have different display timings");

		}


		if (MACHINE_IS_SPECTRUM_128_P2_P2A) {
			menu_add_item_menu_format(array_menu_ula_settings,MENU_OPCION_NORMAL,menu_ula_disable_rom_paging,NULL,"ROM Paging: %s",(ula_disabled_rom_paging.v==0 ? "Yes" : "No"));
			menu_add_item_menu_format(array_menu_ula_settings,MENU_OPCION_NORMAL,menu_ula_disable_ram_paging,NULL,"RAM Paging: %s",(ula_disabled_ram_paging.v==0 ? "Yes" : "No"));
		}


                menu_add_item_menu(array_menu_ula_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_ula_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_ula_settings);

                retorno_menu=menu_dibuja_menu(&ula_settings_opcion_seleccionada,&item_seleccionado,array_menu_ula_settings,"ULA Settings" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}

#define OSD_KEYBOARD_ANCHO_VENTANA 25
#define OSD_KEYBOARD_ALTO_VENTANA 11
#define OSD_KEYBOARD_X_VENTANA (16-OSD_KEYBOARD_ANCHO_VENTANA/2)
#define OSD_KEYBOARD_Y_VENTANA (12-OSD_KEYBOARD_ALTO_VENTANA/2)

        struct s_osd_teclas {
        char tecla[5]; //4 de longitud mas 0 final
        int x; //posicion X en pantalla. Normalmente todos van separados dos caracteres excepto Symbol Shift, Enter etc que ocupan mas
        int ancho_tecla; //1 para todas, excepto enter, cap shift, etc
        };



        typedef struct s_osd_teclas osd_teclas;
/*

01234567890123456789012345

1 2 3 4 5 6 7 8 9 0
 Q W E R T Y U I O P
 A S D F G H J K L ENT
CS Z X C V B N M SS SP
*/

        //Primeras 10 teclas son de la primera linea, siguientes 10, siguiente linea, etc
        osd_teclas teclas_osd[40]={
        {"1",0,1}, {"2",2,1}, {"3",4,1}, {"4",6,1}, {"5",8,1}, {"6",10,1}, {"7",12,1}, {"8",14,1}, {"9",16,1}, {"0",18,1},
 {"Q",1,1}, {"W",3,1}, {"E",5,1}, {"R",7,1}, {"T",9,1}, {"Y",11,1}, {"U",13,1}, {"I",15,1}, {"O",17,1}, {"P",19,1},
 {"A",1,1}, {"S",3,1}, {"D",5,1}, {"F",7,1}, {"G",9,1}, {"H",11,1}, {"J",13,1}, {"K",15,1}, {"L",17,1}, {"ENT",19,3},
{"CS",0,2}, {"Z",3,1}, {"X",5,1}, {"C",7,1}, {"V",9,1}, {"B",11,1}, {"N",13,1}, {"M",15,1}, {"SS",17,2}, {"SP",20,2}
        };


int osd_keyboard_cursor_x=0; //Vale 0..9
int osd_keyboard_cursor_y=0; //Vale 0..3

int menu_onscreen_keyboard_return_index_cursor(void)
{
	return osd_keyboard_cursor_y*10+osd_keyboard_cursor_x;
}

void menu_onscreen_keyboard_dibuja_cursor_aux(char *s,int x,int y)
{
	char *textocursor;
	char textospeech[32];
	textocursor=s;

        //Parchecillo para ZX80/81
        if (MACHINE_IS_ZX8081) {
				 		if (!strcmp(s,"SS")) textocursor=".";
						if (!strcmp(s,"ENT")) textocursor="NL";
        }

	menu_escribe_texto_ventana(x,y,ESTILO_GUI_PAPEL_NORMAL,ESTILO_GUI_TINTA_NORMAL,textocursor);

	//Enviar a speech
	string_a_minusculas(textocursor,textospeech);

	//Casos especiales para speech
	if (!strcmp(textospeech,".")) strcpy (textospeech,"dot");
	else if (!strcmp(textospeech,"cs")) strcpy (textospeech,"caps shift");
	else if (!strcmp(textospeech,"ss")) strcpy (textospeech,"symbol shift");
	else if (!strcmp(textospeech,"ent")) strcpy (textospeech,"enter");
	else if (!strcmp(textospeech,"sp")) strcpy (textospeech,"space");

	//Forzar que siempre suene en speech
	menu_speech_tecla_pulsada=0;
	menu_textspeech_send_text(textospeech);
}

void menu_onscreen_keyboard_dibuja_cursor(void)
{

	int offset_x=1;
	int offset_y=1;

	//Calcular posicion del cursor
	int x,y;
	y=osd_keyboard_cursor_y*2;
	//la x hay que buscarla en el array

	int indice=menu_onscreen_keyboard_return_index_cursor();
	x=teclas_osd[indice].x;

	menu_onscreen_keyboard_dibuja_cursor_aux(teclas_osd[indice].tecla,offset_x+x,offset_y+y);

	//Luego destacar CS y SS si estan seleccionados
	if (menu_button_osdkeyboard_caps.v) {
		indice=30;
		x=teclas_osd[indice].x;
		y=3*2;
		menu_onscreen_keyboard_dibuja_cursor_aux("CS",offset_x+x,offset_y+y);
	}

	if (menu_button_osdkeyboard_symbol.v) {
                indice=38;
                x=teclas_osd[indice].x;
                y=3*2;
                menu_onscreen_keyboard_dibuja_cursor_aux("SS",offset_x+x,offset_y+y);
        }

	//Enter para ZX81
	if (menu_button_osdkeyboard_enter.v) {
		indice=29;
                x=teclas_osd[indice].x;
                y=2*2;
                menu_onscreen_keyboard_dibuja_cursor_aux("ENT",offset_x+x,offset_y+y);
        }



}

void menu_onscreen_keyboard(MENU_ITEM_PARAMETERS)
{
	//Si maquina no es Spectrum o zx80/81, volver
	if (!MACHINE_IS_SPECTRUM && !MACHINE_IS_ZX8081) return;

	//Si estaban caps y simbol activo a la vez, los quitamos (no es util activarlos mas de una vez)
	if (menu_button_osdkeyboard_caps.v && menu_button_osdkeyboard_symbol.v) {
		menu_button_osdkeyboard_caps.v=0;
		menu_button_osdkeyboard_symbol.v=0;
	}

	//Mismo caso para CS+Enter en ZX81
	if (menu_button_osdkeyboard_caps.v && menu_button_osdkeyboard_enter.v) {
                menu_button_osdkeyboard_caps.v=0;
                menu_button_osdkeyboard_enter.v=0;
	}


	menu_dibuja_ventana(OSD_KEYBOARD_X_VENTANA,OSD_KEYBOARD_Y_VENTANA,
				OSD_KEYBOARD_ANCHO_VENTANA,OSD_KEYBOARD_ALTO_VENTANA,"On Screen Keyboard");
	z80_byte tecla;

	int salir=0;

	int indice;

	do {

	        int linea=1;

				//01234567890123456789012345678901
        	char textoventana[32];

		int fila_tecla;
		int pos_tecla_en_fila;

		int indice_tecla=0;

		for (fila_tecla=0;fila_tecla<4;fila_tecla++) {
		//Inicializar texto linea con 31 espacios
					  //1234567890123456789012345678901
			sprintf (textoventana,"%s","                               ");
			for (pos_tecla_en_fila=0;pos_tecla_en_fila<10;pos_tecla_en_fila++) {
				//Copiar texto tecla a buffer linea
				int len=teclas_osd[indice_tecla].ancho_tecla;
				int tecla_x=teclas_osd[indice_tecla].x;
				int i;

				//parchecillo para ZX80/81. Si es Symbol Shift, Realmente es el .
				if (MACHINE_IS_ZX8081 && !strcmp(teclas_osd[indice_tecla].tecla,"SS")) {
					textoventana[tecla_x+0]='.';
					textoventana[tecla_x+1]=' ';  //Como ancho de tecla realmente es dos, meter un espacio
				}

				//parchecillo para ZX80/81. Si es ENT, Realmente es NL
				else if (MACHINE_IS_ZX8081 && !strcmp(teclas_osd[indice_tecla].tecla,"ENT")) {
					textoventana[tecla_x+0]='N';
					textoventana[tecla_x+1]='L';
					textoventana[tecla_x+2]=' ';  //Como ancho de tecla realmente es tres, meter un espacio
				}

				else for (i=0;i<len;i++) {
					textoventana[tecla_x+i+0]=teclas_osd[indice_tecla].tecla[i];
				}

				indice_tecla++;
			}

			//Meter final de cadena
			textoventana[OSD_KEYBOARD_ANCHO_VENTANA-2]=0;


			//No queremos que se envie cada linea a speech
			z80_bit old_textspeech_also_send_menu;
			old_textspeech_also_send_menu.v=textspeech_also_send_menu.v;
			textspeech_also_send_menu.v=0;

	        	menu_escribe_linea_opcion(linea++,-1,1,textoventana);

			//Restaurar parametro speech
			textspeech_also_send_menu.v=old_textspeech_also_send_menu.v;

			linea++;
		}

		menu_onscreen_keyboard_dibuja_cursor();

       		if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();
		menu_espera_tecla();

		tecla=menu_get_pressed_key();

		menu_espera_no_tecla_con_repeticion();

		//tambien permitir mover con joystick aunque no este mapeado a cursor key
		switch (tecla) {
			case 11:
				//arriba
				if (osd_keyboard_cursor_y>0) osd_keyboard_cursor_y--;
			break;

			case 10:
				//abajo
				if (osd_keyboard_cursor_y<3) osd_keyboard_cursor_y++;
			break;

			case 8:
				//izquierda
				if (osd_keyboard_cursor_x>0) osd_keyboard_cursor_x--;
			break;

			case 9:
				//derecha
				if (osd_keyboard_cursor_x<9) osd_keyboard_cursor_x++;
			break;

			case 2: //ESC
			case 13: //Enter
				indice=menu_onscreen_keyboard_return_index_cursor();
				//Si se ha pulsado caps o symbol, no salir, solo activar/desactivar flag
				if (!strcmp(teclas_osd[indice].tecla,"CS")) menu_button_osdkeyboard_caps.v ^=1;
				else if (!strcmp(teclas_osd[indice].tecla,"SS") && MACHINE_IS_SPECTRUM) menu_button_osdkeyboard_symbol.v ^=1;

				//Enter solo actua de modificador cuando ya hay CS pulsado
				else if (!strcmp(teclas_osd[indice].tecla,"ENT") && MACHINE_IS_ZX81 && menu_button_osdkeyboard_caps.v) menu_button_osdkeyboard_enter.v ^=1;
				else salir=1;

				//Pero si estan los dos a la vez, es modo extendido, enviarlos
				if (menu_button_osdkeyboard_caps.v && menu_button_osdkeyboard_symbol.v) salir=1;

				//Similar para ZX81
				if (menu_button_osdkeyboard_caps.v && menu_button_osdkeyboard_enter.v) salir=1;
			break;
		}

	} while (salir==0);

	menu_espera_no_tecla();

	//Si salido con Enter o Fire joystick
	if (tecla==13) {
		//Liberar otras teclas, por si acaso
		reset_keyboard_ports();

	        indice=menu_onscreen_keyboard_return_index_cursor();
		debug_printf (VERBOSE_DEBUG,"Sending key %s",teclas_osd[indice].tecla);

		//Enviar tecla ascii tal cual, excepto Enter, CS, SS, SP
		if (strlen(teclas_osd[indice].tecla)==1) {
			z80_byte tecla=letra_minuscula(teclas_osd[indice].tecla[0]);
			debug_printf (VERBOSE_DEBUG,"Sending Ascii key: %c",tecla);
			convert_numeros_letras_puerto_teclado_continue(tecla,1);
		}
		else {
/*
 A S D F G H J K L ENT
CS Z X C V B N M SS SP
*/
			if (!strcmp(teclas_osd[indice].tecla,"ENT")) util_set_reset_key(UTIL_KEY_ENTER,1);

			//CS y SS los enviamos asi porque en algunos teclados, util_set_reset_key los gestiona diferente y no queremos

			//Esto solo para zx80/81, el .
			else if (!strcmp(teclas_osd[indice].tecla,"SS") && MACHINE_IS_ZX8081) {
				puerto_32766 &=255-2;
				//menu_button_osdkeyboard_symbol.v=1;
			}


			else if (!strcmp(teclas_osd[indice].tecla,"SP")) util_set_reset_key(UTIL_KEY_SPACE,1);


		}

		//Si estan los flags de CS o SS, activarlos
		if (menu_button_osdkeyboard_caps.v) puerto_65278  &=255-1;
		if (menu_button_osdkeyboard_symbol.v) puerto_32766 &=255-2;
		if (menu_button_osdkeyboard_enter.v) puerto_49150 &=255-1;

		salir_todos_menus=1;
		timer_on_screen_key=25; //durante medio segundo

	}

	cls_menu_overlay();

	//Si no se ha salido con escape, hacer que vuelva y quitar pulsaciones de caps y symbol
	if (tecla!=2) {
		menu_button_osdkeyboard_return.v=1;
	}
	else {
		//se sale con esc, quitar pulsaciones de caps y symbol
		menu_button_osdkeyboard_caps.v=0;
		menu_button_osdkeyboard_symbol.v=0;
		menu_button_osdkeyboard_enter.v=0;
	}

}

void menu_hardware_top_speed(MENU_ITEM_PARAMETERS)
{
	top_speed_timer.v ^=1;
}

void menu_hardware_sam_ram(MENU_ITEM_PARAMETERS)
{
	if (sam_memoria_total_mascara==15) sam_memoria_total_mascara=31;
	else sam_memoria_total_mascara=15;
}

void menu_pd765(MENU_ITEM_PARAMETERS)
{
        if (pd765_enabled.v) pd765_disable();
	else pd765_enable();
}

void menu_turbo_mode(MENU_ITEM_PARAMETERS)
{
	if (cpu_turbo_speed==MAX_CPU_TURBO_SPEED) cpu_turbo_speed=1;
	else cpu_turbo_speed *=2;

	cpu_set_turbo_speed();
}


void menu_cpu_speed(MENU_ITEM_PARAMETERS)
{

        char string_speed[5];

        sprintf (string_speed,"%d",porcentaje_velocidad_emulador);

        menu_ventana_scanf("Emulator Speed (%)",string_speed,5);

        porcentaje_velocidad_emulador=parse_string_to_number(string_speed);
        if (porcentaje_velocidad_emulador<1 || porcentaje_velocidad_emulador>9999) porcentaje_velocidad_emulador=100;

	set_emulator_speed();

}

void menu_zxuno_deny_turbo_bios_boot(MENU_ITEM_PARAMETERS)
{
	zxuno_deny_turbo_bios_boot.v ^=1;
}



void menu_multiface_rom_file(MENU_ITEM_PARAMETERS)
{
	multiface_disable();

        char *filtros[2];

        filtros[0]="rom";
        filtros[1]=0;


        if (menu_filesel("Select multiface File",filtros,multiface_rom_file_name)==1) {
                if (!si_existe_archivo(multiface_rom_file_name)) {
                        menu_error_message("File does not exist");
                        multiface_rom_file_name[0]=0;
                        return;



                }

                else {
                        //Comprobar aqui tambien el tamanyo
                        long int size=get_file_size(multiface_rom_file_name);
                        if (size!=8192) {
                                menu_error_message("ROM file must be 8 KB lenght");
                                multiface_rom_file_name[0]=0;
                                return;
                        }
                }


        }
        //Sale con ESC
        else {
                //Quitar nombre
                multiface_rom_file_name[0]=0;


        }

}


void menu_hardware_multiface_enable(MENU_ITEM_PARAMETERS)
{
	if (multiface_enabled.v) multiface_disable();
	else multiface_enable();
}

void menu_hardware_multiface_type(MENU_ITEM_PARAMETERS)
{
  multiface_type++;
  if (multiface_type==MULTIFACE_TOTAL_TYPES)  multiface_type=0;
}

int menu_hardware_multiface_type_cond(void)
{
	return !multiface_enabled.v;
}

void menu_multiface(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_multiface;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char string_multiface_file_shown[13];


                        menu_tape_settings_trunc_name(multiface_rom_file_name,string_multiface_file_shown,13);
                        menu_add_item_menu_inicial_format(&array_menu_multiface,MENU_OPCION_NORMAL,menu_multiface_rom_file,NULL,"~~ROM File: %s",string_multiface_file_shown);
                        menu_add_item_menu_shortcut(array_menu_multiface,'r');
                        menu_add_item_menu_tooltip(array_menu_multiface,"ROM Emulation file");
                        menu_add_item_menu_ayuda(array_menu_multiface,"ROM Emulation file");

                        menu_add_item_menu_format(array_menu_multiface,MENU_OPCION_NORMAL,menu_hardware_multiface_type,menu_hardware_multiface_type_cond,"~~Type: %s",multiface_types_string[multiface_type]);
                  menu_add_item_menu_shortcut(array_menu_multiface,'t');
                  menu_add_item_menu_tooltip(array_menu_multiface,"Multiface type. You must first disable it if you want to change type");
                  menu_add_item_menu_ayuda(array_menu_multiface,"Multiface type. You must first disable it if you want to change type");


                        			menu_add_item_menu_format(array_menu_multiface,MENU_OPCION_NORMAL,menu_hardware_multiface_enable,NULL,"~~Multiface Enabled: %s", (multiface_enabled.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_multiface,'m');
                        menu_add_item_menu_tooltip(array_menu_multiface,"Enable multiface");
                        menu_add_item_menu_ayuda(array_menu_multiface,"Enable multiface");





                                menu_add_item_menu(array_menu_multiface,"",MENU_OPCION_SEPARADOR,NULL,NULL);

                menu_add_ESC_item(array_menu_multiface);

                retorno_menu=menu_dibuja_menu(&multiface_opcion_seleccionada,&item_seleccionado,array_menu_multiface,"ZX Multiface settings" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}




//menu hardware settings
void menu_hardware_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_hardware_settings;
	menu_item item_seleccionado;
	int retorno_menu;
        do {


		menu_add_item_menu_inicial_format(&array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_keyboard_issue,menu_hardware_keyboard_issue_cond,"~~Keyboard Issue %s", (keyboard_issue2.v==1 ? "2" : "3"));
		menu_add_item_menu_shortcut(array_menu_hardware_settings,'k');
		menu_add_item_menu_tooltip(array_menu_hardware_settings,"Type of Spectrum keyboard emulated");
		menu_add_item_menu_ayuda(array_menu_hardware_settings,"Changes the way the Spectrum keyboard port returns its value: Issue 3 returns bit 6 off, and Issue 2 has bit 6 on");


		//Soporte para Azerty keyboard

		if (!strcmp(scr_driver_name,"xwindows")) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_azerty,NULL,"Azerty keyboard: %s",(azerty_keyboard.v ? "Yes" : "No") );
			menu_add_item_menu_tooltip(array_menu_hardware_settings,"Enables azerty keyboard");
			menu_add_item_menu_ayuda(array_menu_hardware_settings,"Only used on xwindows driver by now. Enables to use numeric keys on Azerty keyboard, without having "
						"to press Shift. Note we are referring to the numeric keys (up to letter A, Z, etc) and not to the numeric keypad.");
		}

		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_chloe_keyboard,NULL,"Chloe Keyboard: %s",(chloe_keyboard.v ? "Yes" : "No") );
		}

		if (MACHINE_IS_Z88 || MACHINE_IS_CPC || chloe_keyboard.v || MACHINE_IS_SAM || MACHINE_IS_QL)  {
			//keymap solo hace falta con xwindows y sdl. fbdev y cocoa siempre leen en raw como teclado english
			if (!strcmp(scr_driver_name,"xwindows")  || !strcmp(scr_driver_name,"sdl") ) {
				if (MACHINE_IS_Z88) menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_keymap_z88_cpc,NULL,"Z88 K~~eymap: %s",(z88_cpc_keymap_type == 1 ? "Spanish" : "Default" ));
				else if (MACHINE_IS_CPC) menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_keymap_z88_cpc,NULL,"CPC K~~eymap: %s",(z88_cpc_keymap_type == 1 ? "Spanish" : "Default" ));
				else if (MACHINE_IS_SAM) menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_keymap_z88_cpc,NULL,"SAM K~~eymap: %s",(z88_cpc_keymap_type == 1 ? "Spanish" : "Default" ));
				else if (MACHINE_IS_QL) menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_keymap_z88_cpc,NULL,"QL K~~eymap: %s",(z88_cpc_keymap_type == 1 ? "Spanish" : "Default" ));

				else menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_keymap_z88_cpc,NULL,"Chloe K~~eymap: %s",(z88_cpc_keymap_type == 1 ? "Spanish" : "Default" ));
				menu_add_item_menu_shortcut(array_menu_hardware_settings,'e');
				menu_add_item_menu_tooltip(array_menu_hardware_settings,"Keyboard Layout");
				menu_add_item_menu_ayuda(array_menu_hardware_settings,"Used on Z88, CPC, Sam and Chloe machines, needed to map symbol keys. "
						"You must indicate here which kind of physical keyboard you have. The keyboard will "
						"be mapped always to a Z88/CPC/Sam/Chloe English keyboard, to the absolute positions of the keys. "
						"You have two physical keyboard choices: Default (English) and Spanish");

			}
		}



		//Redefine keys
		menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_redefine_keys,NULL,"Rede~~fine keys");
		menu_add_item_menu_shortcut(array_menu_hardware_settings,'f');
		menu_add_item_menu_tooltip(array_menu_hardware_settings,"Redefine one key to another");
		menu_add_item_menu_ayuda(array_menu_hardware_settings,"Redefine one key to another");


		if (MACHINE_IS_SPECTRUM || MACHINE_IS_ZX8081 || MACHINE_IS_SAM) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_joystick,NULL,"~~Joystick type: %s",joystick_texto[joystick_emulation]);
			menu_add_item_menu_shortcut(array_menu_hardware_settings,'j');
        	        menu_add_item_menu_tooltip(array_menu_hardware_settings,"Decide which joystick type is emulated");
                	menu_add_item_menu_ayuda(array_menu_hardware_settings,"Joystick is emulated with:\n"
					"-A real joystick connected to an USB port\n"
					"-Cursor keys on the keyboard for the directions and Home key for fire"
			);


	                if (joystick_autofire_frequency==0) menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_autofire,menu_hardware_autofire_cond,"~~Autofire: Off");
        	        else {
                	        menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_autofire,NULL,"~~Autofire frequency: %d Hz",50/joystick_autofire_frequency);
	                }

			menu_add_item_menu_shortcut(array_menu_hardware_settings,'a');

        	        menu_add_item_menu_tooltip(array_menu_hardware_settings,"Frequency for the joystick autofire");
                	menu_add_item_menu_ayuda(array_menu_hardware_settings,"Times per second (Hz) the joystick fire is auto-switched from pressed to not pressed and viceversa. "
                                        "Autofire can only be enabled on Kempston, Fuller, Zebra and Mikrogen; Sinclair, Cursor, and OPQA can not have "
                                        "autofire because this function can interfiere with the menu (it might think a key is pressed)");


			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_realjoystick,menu_hardware_realjoystick_cond,"~~Real joystick emulation");
			menu_add_item_menu_shortcut(array_menu_hardware_settings,'r');
			menu_add_item_menu_tooltip(array_menu_hardware_settings,"Settings for the real joystick");
			menu_add_item_menu_ayuda(array_menu_hardware_settings,"Settings for the real joystick");

		}


		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_gunstick,NULL,"~~Lightgun emulate: %s",gunstick_texto[gunstick_emulation]);
			menu_add_item_menu_shortcut(array_menu_hardware_settings,'l');
			menu_add_item_menu_tooltip(array_menu_hardware_settings,"Decide which kind of lightgun is emulated with the mouse");
			menu_add_item_menu_ayuda(array_menu_hardware_settings,"Lightgun emulation supports the following two models:\n\n"
					"Gunstick from MHT Ingenieros S.L: all types except AYChip\n\n"
					"Magnum Light Phaser (experimental): supported by AYChip type");


			if (menu_hardware_gunstick_aychip_cond()) {
				menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_gunstick_range_x,NULL,"X Range: %d",gunstick_range_x);
				menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_gunstick_range_y,NULL,"Y Range: %d",gunstick_range_y);
				menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_gunstick_y_offset,NULL,"Y Offset: %s%d",(gunstick_y_offset ? "-" : "" ), gunstick_y_offset);
				menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_gunstick_solo_brillo,NULL,"Detect only white bright: %s",(gunstick_solo_brillo ? "On" : "Off"));
		}


			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_kempston_mouse,NULL,"Kempston Mou~~se emulation: %s",(kempston_mouse_emulation.v==1 ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_hardware_settings,'s');


		}




		if (MACHINE_IS_SPECTRUM || MACHINE_IS_ZX81) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_printers,NULL,"~~Printing emulation");
			menu_add_item_menu_shortcut(array_menu_hardware_settings,'p');
		}



		/*
		if (MACHINE_IS_INVES) {

			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_inves_delay_factor,menu_inves_cond_realvideo,"Inves Ula Delay Every: %d",inves_ula_delay_factor);
			menu_add_item_menu_tooltip(array_menu_hardware_settings,"Step for the ula delay bug on Inves");
			menu_add_item_menu_ayuda(array_menu_hardware_settings,"This step can be changed so I do not know which is the exact step on a real Inves");
		}
		*/


		menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_memory_settings,NULL,"~~Memory Settings");
		menu_add_item_menu_shortcut(array_menu_hardware_settings,'m');





		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_ula_settings,NULL,"~~ULA Settings");
			menu_add_item_menu_shortcut(array_menu_hardware_settings,'u');
	                menu_add_item_menu_tooltip(array_menu_hardware_settings,"Change some ULA settings");
	                menu_add_item_menu_ayuda(array_menu_hardware_settings,"Change some ULA settings");


		}


		/* De momento no activo
		if (MACHINE_IS_SPECTRUM_P2A) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_pd765,NULL,"PD765 enabled: %s",(pd765_enabled.v ? "Yes" : "No") );
		}
		*/


		if (!MACHINE_IS_Z88) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_hardware_top_speed,NULL,"~~Top Speed: %s",(top_speed_timer.v ? "Yes" : "No") );
			menu_add_item_menu_shortcut(array_menu_hardware_settings,'t');
			menu_add_item_menu_tooltip(array_menu_hardware_settings,"Runs at maximum speed, when menu closed. Not available on Z88");
			menu_add_item_menu_ayuda(array_menu_hardware_settings,"Runs at maximum speed, using 100% of CPU of host machine, when menu closed. "
						"The display is refreshed 1 time per second. This mode is also entered when loading a real tape and "
						"accelerate loaders setting is enabled. Not available on Z88");

		}

		menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_cpu_speed,NULL,"Emulator Spee~~d: %d%%",porcentaje_velocidad_emulador);
		menu_add_item_menu_shortcut(array_menu_hardware_settings,'d');
		menu_add_item_menu_tooltip(array_menu_hardware_settings,"Change the emulator Speed");
		menu_add_item_menu_ayuda(array_menu_hardware_settings,"Changes all the emulator speed by setting a different interval between display frames. "
		"Also changes audio frequency");


		/* De momento esto desactivado
		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_if1_settings,NULL,"Interface 1: %s",(if1_enabled.v ? "Yes" : "No") );
		}
		*/

		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_multiface,NULL,"Multiface emulation"  );
		}

		menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_turbo_mode,NULL,"Turbo: %dX",cpu_turbo_speed);
		menu_add_item_menu_tooltip(array_menu_hardware_settings,"Changes only the Z80 speed");
		menu_add_item_menu_ayuda(array_menu_hardware_settings,"Changes only the Z80 speed. Do not modify FPS, interrupts or any other parameter. "
					"Some machines, like ZX-Uno or Chloe, change this setting");

		if (MACHINE_IS_ZXUNO) {
					menu_add_item_menu_format(array_menu_hardware_settings,MENU_OPCION_NORMAL,menu_zxuno_deny_turbo_bios_boot,NULL,"Deny turbo on boot: %s",
							(zxuno_deny_turbo_bios_boot.v ? "Yes" : "No") );
					menu_add_item_menu_tooltip(array_menu_hardware_settings,"Denies changing turbo mode when booting ZX-Uno and on bios");
					menu_add_item_menu_ayuda(array_menu_hardware_settings,"Denies changing turbo mode when booting ZX-Uno and on bios");
	  }



                menu_add_item_menu(array_menu_hardware_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_hardware_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_hardware_settings);

                retorno_menu=menu_dibuja_menu(&hardware_settings_opcion_seleccionada,&item_seleccionado,array_menu_hardware_settings,"Hardware Settings" );

                cls_menu_overlay();
		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
			//llamamos por valor de funcion
	                if (item_seleccionado.menu_funcion!=NULL) {
        	                //printf ("actuamos por funcion\n");
                	        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
	                }
		}

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}



//menu hardware settings
void menu_hardware_memory_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_hardware_memory_settings;
        menu_item item_seleccionado;
        int retorno_menu;
        do {


		menu_add_item_menu_inicial_format(&array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_allow_write_rom,menu_cond_allow_write_rom,"Allow ~~write in ROM: %s",
			(allow_write_rom.v ? "Yes" : "No") );
		menu_add_item_menu_shortcut(array_menu_hardware_memory_settings,'w');
		menu_add_item_menu_tooltip(array_menu_hardware_memory_settings,"Allow write in ROM");
		menu_add_item_menu_ayuda(array_menu_hardware_memory_settings,"Allow write in ROM. Only allowed on Spectrum 48k/16k models, ZX80, ZX81, Sam Coupe and Jupiter Ace (and not on Inves)");

		if (menu_cond_zx8081() ) {

                        //int ram_zx8081=(ramtop_zx8081-16383)/1024;
												int ram_zx8081=zx8081_get_standard_ram();
                        menu_add_item_menu_format(array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_zx8081_ramtop,menu_cond_zx8081,"ZX80/81 Standard RAM: %d KB",ram_zx8081);
                        menu_add_item_menu_tooltip(array_menu_hardware_memory_settings,"Standard RAM for the ZX80/81");
                        menu_add_item_menu_ayuda(array_menu_hardware_memory_settings,"Standard RAM for the ZX80/81");


                        menu_add_item_menu_format(array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_zx8081_ram_in_8192,menu_cond_zx8081,"ZX80/81 8K RAM in 2000H: %s", (ram_in_8192.v ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_hardware_memory_settings,"8KB RAM at address 2000H");
                        menu_add_item_menu_ayuda(array_menu_hardware_memory_settings,"8KB RAM at address 2000H. Used on some wrx games");

                        menu_add_item_menu_format(array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_zx8081_ram_in_32768,menu_cond_zx8081,"ZX80/81 16K RAM in 8000H: %s", (ram_in_32768.v ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_hardware_memory_settings,"16KB RAM at address 8000H");
                        menu_add_item_menu_ayuda(array_menu_hardware_memory_settings,"16KB RAM at address 8000H");
                        menu_add_item_menu_format(array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_zx8081_ram_in_49152,menu_cond_zx8081,"ZX80/81 16K RAM in C000H: %s", (ram_in_49152.v ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_hardware_memory_settings,"16KB RAM at address C000H");
                        menu_add_item_menu_ayuda(array_menu_hardware_memory_settings,"16KB RAM at address C000H. It requires the previous RAM at 8000H");


                }


		if (MACHINE_IS_SAM) {
			menu_add_item_menu_format(array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_sam_ram,NULL,"Sam Coupe RAM: %d KB",(sam_memoria_total_mascara==15 ? 256 : 512) );
		}


		if (MACHINE_IS_ACE) {
			//int ram_ace=((ramtop_ace-16383)/1024)+3;
			int ram_ace=get_ram_ace();
			menu_add_item_menu_format(array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_ace_ramtop,NULL,"Jupiter Ace RAM: %d KB",ram_ace);
		}



      if (MACHINE_IS_SPECTRUM_48) {
                        menu_add_item_menu_format(array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_memory_refresh,NULL,"RAM Refresh emulation: %s", (machine_emulate_memory_refresh==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_hardware_memory_settings,"Enable RAM R~~efresh emulation");
                        menu_add_item_menu_shortcut(array_menu_hardware_memory_settings,'e');
                        menu_add_item_menu_ayuda(array_menu_hardware_memory_settings,"RAM Refresh emulation consists, in a real Spectrum 48k, "
                                        "to refresh the upper 32kb RAM using the R register. On a real Spectrum 48k, if you modify "
                                        "the R register very fast, you can lose RAM contents.\n"
                                        "This option emulates this behaviour, and sure you don't need to enable it on the 99.99 percent of the "
                                        "situations ;) ");
                }


	  if (MACHINE_IS_INVES) {
                        menu_add_item_menu_format(array_menu_hardware_memory_settings,MENU_OPCION_NORMAL,menu_hardware_inves_poke,menu_inves_cond,"Poke Inves Low RAM");
                        menu_add_item_menu_tooltip(array_menu_hardware_memory_settings,"Poke Inves low RAM");
                        menu_add_item_menu_ayuda(array_menu_hardware_memory_settings,"You can alter the way Inves work with ULA port (sound & border). "
                                        "You change here the contents of the low (hidden) RAM of the Inves (addresses 0-16383). Choosing this option "
                                        "is the same as poke at the whole low RAM addresses (0 until 16383). I suggest to poke with value 15 or 23 "
                                        "on games that you can not hear well the music: Lemmings, ATV, Batman caped crusader...");

                }




		menu_add_item_menu(array_menu_hardware_memory_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);

                menu_add_ESC_item(array_menu_hardware_memory_settings);

                retorno_menu=menu_dibuja_menu(&hardware_memory_settings_opcion_seleccionada,&item_seleccionado,array_menu_hardware_memory_settings,"Memory Settings" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}



void menu_tape_any_flag(MENU_ITEM_PARAMETERS)
{
	tape_any_flag_loading.v^=1;
}

int menu_tape_input_insert_cond(void)
{
	if (tapefile==NULL) return 0;
	else return 1;
}

void menu_tape_input_insert(MENU_ITEM_PARAMETERS)
{

	if (tapefile==NULL) return;

	if ((tape_loadsave_inserted & TAPE_LOAD_INSERTED)==0) {
		tap_open();
	}

	else {
		tap_close();
	}
}

//truncar el nombre del archivo a un maximo
//si origen es NULL, poner en destino cadena vacia
void menu_tape_settings_trunc_name(char *orig,char *dest,int max)
{

	//en maximo se incluye caracter 0 del final
	max--;

                if (orig!=0) {

                        int longitud=strlen(orig);
                        int indice=longitud-max;

			if (indice<0) indice=0;

                        strncpy(dest,&orig[indice],max);


			//printf ("copiamos %d max caracteres\n",max);

                        //si cadena es mayor, acabar con 0

			//en teoria max siempre es mayor de cero, pero por si acaso...
			if (max>0) dest[max]=0;

			//else printf ("max=%d\n",max);


			if (indice>0) dest[0]='<';

                }

             else {
                        strcpy(dest,"");
                }

}


int menu_tape_output_insert_cond(void)
{
        if (tape_out_file==NULL) return 0;
        else return 1;
}

void menu_tape_output_insert(MENU_ITEM_PARAMETERS)
{

        if (tape_out_file==NULL) return;

	if ((tape_loadsave_inserted & TAPE_SAVE_INSERTED)==0) {
                tap_out_open();
        }

        else {
                tap_out_close();
        }
}


//devuelve 1 si el directorio cumple el filtro
//realmente lo que hacemos aqui es ocultar/mostrar carpetas que empiezan con .
int menu_file_filter_dir(const char *name,char *filtros[])
{

        int i;
        //char extension[1024];

        //directorio ".." siempre se muestra
        if (!strcmp(name,"..")) return 1;

        char *f;


        //Bucle por cada filtro
        for (i=0;filtros[i];i++) {
                //si filtro es "", significa todo (*)
                //supuestamente si hay filtro "" no habrian mas filtros pasados en el array...

 	       f=filtros[i];
                if (f[0]==0) return 1;

                //si filtro no es *, ocultamos los que empiezan por "."
                if (name[0]=='.') return 0;

        }


	//y finalmente mostramos el directorio
        return 1;

}



//devuelve 1 si el archivo cumple el filtro
int menu_file_filter(const char *name,char *filtros[])
{

	int i;
	char extension[NAME_MAX];

	/*
	//obtener extension del nombre
	//buscar ultimo punto

	int j;
	j=strlen(name);
	if (j==0) extension[0]=0;
	else {
		for (;j>=0 && name[j]!='.';j--);

		if (j>=0) strcpy(extension,&name[j+1]);
		else extension[0]=0;
	}

	//printf ("Extension: %s\n",extension);

	*/

	util_get_file_extension((char *) name,extension);

	char *f;


	//Bucle por cada filtro
	for (i=0;filtros[i];i++) {
		//si filtro es "", significa todo (*)
		//supuestamente si hay filtro "" no habrian mas filtros pasados en el array...

		f=filtros[i];
		//printf ("f: %d\n",f);
		if (f[0]==0) return 1;

		//si filtro no es *, ocultamos los que empiezan por "."
		//Aparentemente esto no tiene mucho sentido, con esto ocultariamos archivo de nombre tipo ".xxxx.tap" por ejemplo
		//Pero bueno, para volumenes que vienen de mac os x, los metadatos se guardan en archivos tipo:
		//._0186.tap
		if (name[0]=='.') return 0;


		//comparamos extension
		if (!strcasecmp(extension,f)) return 1;
	}

	//Si es zip, tambien lo soportamos
	if (!strcasecmp(extension,"zip")) return 1;

	//Si es gz, tambien lo soportamos
	if (!strcasecmp(extension,"gz")) return 1;

	//Si es tar, tambien lo soportamos
	if (!strcasecmp(extension,"tar")) return 1;

	//Si es rar, tambien lo soportamos
	if (!strcasecmp(extension,"rar")) return 1;


	return 0;

}

int menu_filesel_filter_func(const struct dirent *d)
{

	int tipo_archivo=get_file_type(d->d_type,(char *)d->d_name);


	//si es directorio, ver si empieza con . y segun el filtro activo
	if (tipo_archivo == 2) {
		if (menu_file_filter_dir(d->d_name,filesel_filtros)==1) return 1;
		return 0;
	}

	//Si no es archivo ni link, no ok

	if (tipo_archivo  == 0) {
		debug_printf (VERBOSE_DEBUG,"Item is not a directory, file or link. Type: %d",d->d_type);
		return 0;
	}

	//es un archivo. ver el nombre

	if (menu_file_filter(d->d_name,filesel_filtros)==1) return 1;


	return 0;
}

int menu_filesel_alphasort(const struct dirent **d1, const struct dirent **d2)
{

	//printf ("menu_filesel_alphasort %s %s\n",(*d1)->d_name,(*d2)->d_name );

	//compara nombre
	return (strcasecmp((*d1)->d_name,(*d2)->d_name));
}

void menu_filesel_readdir(void)
{

/*
       lowing macro constants for the value returned in d_type:

       DT_BLK      This is a block device.

       DT_CHR      This is a character device.

       DT_DIR      This is a directory.

       DT_FIFO     This is a named pipe (FIFO).

       DT_LNK      This is a symbolic link.

       DT_REG      This is a regular file.

       DT_SOCK     This is a UNIX domain socket.

       DT_UNKNOWN  The file type is unknown.

*/


filesel_total_items=0;
primer_filesel_item=NULL;


    struct dirent **namelist;

	struct dirent *nombreactual;

    int n;
//printf ("usando scandir\n");

	filesel_item *item;
	filesel_item *itemanterior;


#ifndef MINGW
	n = scandir(".", &namelist, menu_filesel_filter_func, menu_filesel_alphasort);
#else
	//alternativa scandir, creada por mi
	n = scandir_mingw(".", &namelist, menu_filesel_filter_func, menu_filesel_alphasort);
#endif

    if (n < 0) debug_printf (VERBOSE_ERR,"Error scandir");

    else {
        int i;

	//printf("total elementos directorio: %d\n",n);

        for (i=0;i<n;i++) {
		nombreactual=namelist[i];
            //printf("%s\n", nombreactual->d_name);
            //printf("%d\n", nombreactual->d_type);


		item=malloc(sizeof(filesel_item));
		if (item==NULL) cpu_panic("Error allocating file item");

		strcpy(item->d_name,nombreactual->d_name);

		item->d_type=nombreactual->d_type;
		item->next=NULL;

		//primer item
		if (primer_filesel_item==NULL) {
			primer_filesel_item=item;
		}

		//siguientes items
		else {
			itemanterior->next=item;
		}

		itemanterior=item;
		free(namelist[i]);


		filesel_total_items++;
        }

    }

	free(namelist);

}


//Retorna 1 si ok
//Retorna 0 si no ok
int menu_avisa_si_extension_no_habitual(char *filtros[],char *archivo)
{

	int i;

	for (i=0;filtros[i];i++) {
		if (!util_compare_file_extension(archivo,filtros[i])) return 1;

		//si filtro es "", significa todo (*)
		if (!strcmp(filtros[i],"")) return 1;

	}


	//no es extension habitual. Avisar
	return menu_confirm_yesno_texto("Unusual file extension","Do you want to use this file?");
}


void menu_quickload(MENU_ITEM_PARAMETERS)
{

        char *filtros[25];

        filtros[0]="zx";
        filtros[1]="sp";
        filtros[2]="z80";
        filtros[3]="sna";

        filtros[4]="o";
        filtros[5]="p";
        filtros[6]="80";
        filtros[7]="81";
	filtros[8]="z81";

        filtros[9]="tzx";
        filtros[10]="tap";

	filtros[11]="rwa";
	filtros[12]="smp";
	filtros[13]="wav";

	filtros[14]="epr";
	filtros[15]="63";
	filtros[16]="eprom";
	filtros[17]="flash";

	filtros[18]="ace";

	filtros[19]="dck";

	filtros[20]="cdt";

	filtros[21]="ay";

	filtros[22]="scr";

	filtros[23]="rzx";

	filtros[24]=0;


        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de snap
        //si no hay directorio, vamos a rutas predefinidas
        if (quickfile==NULL) menu_chdir_sharedfiles();
	else {
                char directorio[PATH_MAX];
                util_get_dir(quickfile,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }



        int ret;

        ret=menu_filesel("Select File",filtros,quickload_file);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);

        if (ret==1) {

		quickfile=quickload_file;

                //sin overlay de texto, que queremos ver las franjas de carga con el color normal (no apagado)
                reset_menu_overlay_function();


		if (quickload(quickload_file)) {
			debug_printf (VERBOSE_ERR,"Unknown file format");
		}

                //restauramos modo normal de texto de menu
                set_menu_overlay_function(normal_overlay_texto_menu);

                //Y salimos de todos los menus
                salir_todos_menus=1;
        }


}


void menu_tape_out_open(MENU_ITEM_PARAMETERS)
{

        char *filtros[5];
	char mensaje_existe[20];

        if (MACHINE_IS_ZX8081) {

		if (MACHINE_IS_ZX80) filtros[0]="o";
		else filtros[0]="p";

		filtros[1]=0;

		strcpy(mensaje_existe,"Overwrite?");
        }

        else {
        filtros[0]="tzx";
        filtros[1]="tap";
        filtros[2]=0;
		strcpy(mensaje_existe,"Append?");
        }


        if (menu_filesel("Select Output Tape",filtros,tape_out_open_file)==1) {

		//Ver si archivo existe y preguntar
                struct stat buf_stat;

                if (stat(tape_out_open_file, &buf_stat)==0) {

                        if (menu_confirm_yesno_texto("File exists",mensaje_existe)==0) {
				tape_out_file=NULL;
				tap_out_close();
				return;
			}

                }

                tape_out_file=tape_out_open_file;
                tape_out_init();
        }


        else {
                tape_out_file=NULL;
		tap_out_close();
        }



}


void menu_tape_open(MENU_ITEM_PARAMETERS)
{

        char *filtros[7];

	if (MACHINE_IS_ZX80) {
		filtros[0]="80";
        	filtros[1]="o";
        	filtros[2]="rwa";
        	filtros[3]="smp";
        	filtros[4]="wav";
        	filtros[5]="z81";
        	filtros[6]=0;
	}

	else if (MACHINE_IS_ZX81) {
                filtros[0]="p";
                filtros[1]="81";
                filtros[2]="rwa";
                filtros[3]="smp";
                filtros[4]="z81";
                filtros[5]="wav";
                filtros[6]=0;
        }

	else {
        filtros[0]="tzx";
        filtros[1]="tap";
        filtros[2]="rwa";
        filtros[3]="smp";
        filtros[4]="wav";
        filtros[5]=0;
	}


	//guardamos directorio actual
	char directorio_actual[PATH_MAX];
	getcwd(directorio_actual,PATH_MAX);

	//Obtenemos directorio de cinta
	//si no hay directorio, vamos a rutas predefinidas
	if (tapefile==NULL) menu_chdir_sharedfiles();

	else {
	        char directorio[PATH_MAX];
	        util_get_dir(tapefile,directorio);
	        //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

		//cambiamos a ese directorio, siempre que no sea nulo
		if (directorio[0]!=0) {
			debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
			menu_filesel_chdir(directorio);
		}
	}



        int ret;

        ret=menu_filesel("Select Input Tape",filtros,tape_open_file);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);


	if (ret==1) {
		tapefile=tape_open_file;
		tape_init();
	}


}


void menu_tape_simulate_real_load(MENU_ITEM_PARAMETERS)
{
	tape_loading_simulate.v ^=1;

	//Al activar carga real, tambien activamos realvideo
	if (tape_loading_simulate.v==1) rainbow_enabled.v=1;
}

void menu_tape_simulate_real_load_fast(MENU_ITEM_PARAMETERS)
{
        tape_loading_simulate_fast.v ^=1;
}


int menu_tape_simulate_real_load_cond(void)
{
	return tape_loading_simulate.v==1;
}


void menu_realtape_insert(MENU_ITEM_PARAMETERS)
{
	if (realtape_inserted.v==0) realtape_insert();
	else realtape_eject();
}

void menu_realtape_play(MENU_ITEM_PARAMETERS)
{
	if (realtape_playing.v) realtape_stop_playing();
	else realtape_start_playing();
}

void menu_realtape_volumen(MENU_ITEM_PARAMETERS)
{
	realtape_volumen++;
	if (realtape_volumen==16) realtape_volumen=0;
}

int menu_realtape_cond(void)
{
	if (realtape_name==NULL) return 0;
	else return 1;
}

int menu_realtape_inserted_cond(void)
{
	if (menu_realtape_cond()==0) return 0;
	return realtape_inserted.v;
}

void menu_realtape_open(MENU_ITEM_PARAMETERS)
{

        char *filtros[9];

        filtros[0]="smp";
        filtros[1]="rwa";
        filtros[2]="wav";
        filtros[3]="tzx";
        filtros[4]="p";
        filtros[5]="o";
        filtros[6]="tap";
        filtros[7]="cdt";
        filtros[8]=0;


        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de cinta
        //si no hay directorio, vamos a rutas predefinidas
        if (realtape_name==NULL) menu_chdir_sharedfiles();

        else {
                char directorio[PATH_MAX];
                util_get_dir(realtape_name,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

     		//cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }



        int ret;

        ret=menu_filesel("Select Input Tape",filtros,menu_realtape_name);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);


        if (ret==1) {
                realtape_name=menu_realtape_name;
        	realtape_insert();
	}


}

void menu_realtape_wave_offset(MENU_ITEM_PARAMETERS)
{
        int valor_offset;

        char string_offset[5];


        sprintf (string_offset,"%d",realtape_wave_offset);

        menu_ventana_scanf("Offset",string_offset,5);

        valor_offset=parse_string_to_number(string_offset);

	if (valor_offset<-128 || valor_offset>127) {
		debug_printf (VERBOSE_ERR,"Invalid offset");
		return;
	}

	realtape_wave_offset=valor_offset;

}



void menu_standard_to_real_tape_fallback(MENU_ITEM_PARAMETERS)
{
	standard_to_real_tape_fallback.v ^=1;

}

void menu_realtape_accelerate_loaders(MENU_ITEM_PARAMETERS)
{
	accelerate_loaders.v ^=1;

}

void menu_realtape_loading_sound(MENU_ITEM_PARAMETERS)
{
	realtape_loading_sound.v ^=1;
}

void menu_tape_browser(MENU_ITEM_PARAMETERS)
{
	//tapefile
	if (util_compare_file_extension(tapefile,"tap")!=0) {
		debug_printf(VERBOSE_ERR,"Tape browser not supported for this tape type (yet)");
		return;
	}

	//Leemos cinta en memoria
	int total_mem=get_file_size(tapefile);

	z80_byte *taperead;



        FILE *ptr_tapebrowser;
        ptr_tapebrowser=fopen(tapefile,"rb");

        if (!ptr_tapebrowser) {
		debug_printf(VERBOSE_ERR,"Unable to open tape");
		return;
	}

	taperead=malloc(total_mem);
	if (taperead==NULL) cpu_panic("Error allocating memory for tape browser");

	z80_byte *puntero_lectura;
	puntero_lectura=taperead;


        int leidos=fread(taperead,1,total_mem,ptr_tapebrowser);

	if (leidos==0) {
                debug_printf(VERBOSE_ERR,"Error reading tape");
		free(taperead);
                return;
        }


        fclose(ptr_tapebrowser);

	char buffer_texto[40];

	int longitud_bloque;

	int longitud_texto;
#define MAX_TEXTO_BROWSER 4096
	char texto_browser[MAX_TEXTO_BROWSER];
	int indice_buffer=0;

	while(total_mem>0) {
		longitud_bloque=util_tape_tap_get_info(puntero_lectura,buffer_texto);
		total_mem-=longitud_bloque;
		puntero_lectura +=longitud_bloque;
		debug_printf (VERBOSE_DEBUG,"Tape browser. Block: %s",buffer_texto);


     //printf ("nombre: %s c1: %d\n",buffer_nombre,buffer_nombre[0]);
                        longitud_texto=strlen(buffer_texto)+1; //Agregar salto de linea
                        if (indice_buffer+longitud_texto>MAX_TEXTO_BROWSER-1) {
				debug_printf (VERBOSE_ERR,"Too much headers. Showing only the allowed in memory");
				total_mem=0; //Finalizar bloque
			}

                        else {
                                sprintf (&texto_browser[indice_buffer],"%s\n",buffer_texto);
                                indice_buffer +=longitud_texto;
                        }

	}

	texto_browser[indice_buffer]=0;
	menu_generic_message_tooltip("Tape browser", 0, 1, NULL, "%s", texto_browser);

	//int util_tape_tap_get_info(z80_byte *tape,char *texto)

	free(taperead);

}

//menu tape settings
void menu_tape_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_tape_settings;
	menu_item item_seleccionado;
	int retorno_menu;

        do {
                char string_tape_load_shown[20],string_tape_load_inserted[50],string_tape_save_shown[20],string_tape_save_inserted[50];
		char string_realtape_shown[23];

		menu_add_item_menu_inicial_format(&array_menu_tape_settings,MENU_OPCION_NORMAL,NULL,NULL,"--Standard Tape--");
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Select Standard tape for Input and Output");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"Standard tapes are those handled by ROM routines and "
					"have normal speed (no turbo). These tapes are handled by the emulator and loaded or saved "
					"very quickly. The input formats supported are: o, p, tap, tzx (binary only), rwa, wav and smp; "
					"output formats are: o, p, tap and tzx. Those input audio formats (rwa, wav or smp) "
					"are converted by the emulator to bytes and loaded on the machine memory. For every other non standard "
					"tapes (turbo or handled by non-ROM routines like loading stripes on different colours) you must use "
					"Real Input tape for load, and Audio output to file for saving");


		menu_tape_settings_trunc_name(tapefile,string_tape_load_shown,20);
		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_tape_open,NULL,"~~Input: %s",string_tape_load_shown);
		menu_add_item_menu_shortcut(array_menu_tape_settings,'i');


		sprintf (string_tape_load_inserted,"Input tape inserted: %s",((tape_loadsave_inserted & TAPE_LOAD_INSERTED)!=0 ? "Yes" : "No"));
		menu_add_item_menu(array_menu_tape_settings,string_tape_load_inserted,MENU_OPCION_NORMAL,menu_tape_input_insert,menu_tape_input_insert_cond);

		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_tape_browser,menu_tape_input_insert_cond,"Tape ~~Browser");
		menu_add_item_menu_shortcut(array_menu_tape_settings,'b');
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Browse tape");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"Browse tape");


                menu_tape_settings_trunc_name(tape_out_file,string_tape_save_shown,20);
		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_tape_out_open,NULL,"~~Output: %s",string_tape_save_shown);
		menu_add_item_menu_shortcut(array_menu_tape_settings,'o');

                sprintf (string_tape_save_inserted,"Output tape inserted: %s",((tape_loadsave_inserted & TAPE_SAVE_INSERTED)!=0 ? "Yes" : "No"));
                menu_add_item_menu(array_menu_tape_settings,string_tape_save_inserted,MENU_OPCION_NORMAL,menu_tape_output_insert,menu_tape_output_insert_cond);

/*
		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_standard_to_real_tape_fallback,NULL,"Fa~~llback to real tape: %s",(standard_to_real_tape_fallback.v ? "Yes" : "No") );
		menu_add_item_menu_shortcut(array_menu_tape_settings,'l');
		menu_add_item_menu_tooltip(array_menu_tape_settings,"If this standard tape is detected as real tape, reinsert tape as real tape");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"While loading the standard tape, if a custom loading routine is detected, "
					"the tape will be ejected from standard tape and inserted it as real tape. If autoload tape is enabled, "
					"the machine will be resetted and loaded the tape from the beginning");


		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_tape_any_flag,NULL,"A~~ny flag loading: %s", (tape_any_flag_loading.v==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_tape_settings,'n');
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Enables tape load routine to load without knowing block flag");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"Enables tape load routine to load without knowing block flag. You must enable it on Tape Copy programs");



                //menu_add_item_menu(array_menu_tape_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);


			menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_tape_simulate_real_load,NULL,"~~Simulate real load: %s", (tape_loading_simulate.v==1 ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_tape_settings,'s');
			menu_add_item_menu_tooltip(array_menu_tape_settings,"Simulate sound and loading stripes");
			menu_add_item_menu_ayuda(array_menu_tape_settings,"Simulate sound and loading stripes. You can skip simulation pressing any key (and the data is loaded)");

			menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_tape_simulate_real_load_fast,menu_tape_simulate_real_load_cond,"Fast Simulate real load: %s", (tape_loading_simulate_fast.v==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_tape_settings,"Simulate sound and loading stripes at faster speed");
                        menu_add_item_menu_ayuda(array_menu_tape_settings,"Simulate sound and loading stripes at faster speed");

*/
        	        menu_add_item_menu(array_menu_tape_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);


		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,NULL,NULL,"--Input Real Tape--");
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Input Real Tape at normal loading Speed");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"You may select any input valid tape: o, p, tap, tzx, rwa, wav, smp. "
					"This tape is handled the same way as the real machine does, at normal loading speed, and may "
					"select tapes with different loading methods instead of the ROM: turbo loading, alkatraz, etc...\n"
					"When inserted real tape, realvideo is enabled, only to show real loading stripes on screen, but it is "
					"not necessary, you may disable realvideo if you want");


                menu_tape_settings_trunc_name(realtape_name,string_realtape_shown,23);
                menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_realtape_open,NULL,"~~File: %s",string_realtape_shown);
		menu_add_item_menu_shortcut(array_menu_tape_settings,'f');
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Audio file to use as the input audio");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"Audio file to use as the input audio");


		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_realtape_insert,menu_realtape_cond,"Inserted: %s", (realtape_inserted.v==1 ? "Yes" : "No"));
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Insert the audio file");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"Insert the audio file");



		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_realtape_play,menu_realtape_inserted_cond,"~~Playing: %s", (realtape_playing.v==1 ? "Yes" : "No"));
		menu_add_item_menu_shortcut(array_menu_tape_settings,'p');
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Start playing the audio tape");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"Start playing the audio tape");

/*
		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_realtape_loading_sound,NULL,"Loading sound: %s", (realtape_loading_sound.v==1 ? "Yes" : "No"));
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Enable loading sound");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"Enable loading sound. With sound disabled, the tape is also loaded");



		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_realtape_volumen,NULL,"Volume bit 1 range: %s%d",(realtape_volumen>0 ? "+" : ""),realtape_volumen);
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Volume bit 1 starting range value");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"The input audio value read (considering range from -128 to +127) is treated "
					"normally as 1 if the value is in range 0...+127, and 0 if it is in range -127...-1. This setting "
					"increases this 0 (of range 0...+127) to consider it is a bit 1. I have found this value is better to be 0 "
					"on Spectrum, and 2 on ZX80/81");



		menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_realtape_wave_offset,NULL,"Level Offset: %d",realtape_wave_offset);
		menu_add_item_menu_tooltip(array_menu_tape_settings,"Apply offset to sound value read");
		menu_add_item_menu_ayuda(array_menu_tape_settings,"Indicates some value (positive or negative) to sum to the raw value read "
					"(considering range from -128 to +127) to the input audio value read");

		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_tape_settings,MENU_OPCION_NORMAL,menu_realtape_accelerate_loaders,NULL,"A~~ccelerate loaders: %s",
				(accelerate_loaders.v==1 ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_tape_settings,'c');
			menu_add_item_menu_tooltip(array_menu_tape_settings,"Set top speed setting when loading a real tape");
			menu_add_item_menu_ayuda(array_menu_tape_settings,"Set top speed setting when loading a real tape");
		}


*/


                menu_add_item_menu(array_menu_tape_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);




                //menu_add_item_menu(array_menu_tape_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_tape_settings);

                retorno_menu=menu_dibuja_menu(&tape_settings_opcion_seleccionada,&item_seleccionado,array_menu_tape_settings,"Tape Settings" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
			//llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
	                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
        	        }
		}

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC);


}


void menu_snapshot_load(MENU_ITEM_PARAMETERS)
{

        char *filtros[12];

        filtros[0]="zx";
        filtros[1]="sp";
        filtros[2]="z80";
        filtros[3]="sna";
        filtros[4]="o";
        filtros[5]="p";

        filtros[6]="80";
        filtros[7]="81";
        filtros[8]="z81";
        filtros[9]="ace";
				filtros[10]="rzx";
        filtros[11]=0;



        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de snap
        //si no hay directorio, vamos a rutas predefinidas
        if (snapfile==NULL) menu_chdir_sharedfiles();

	else {
	        char directorio[PATH_MAX];
	        util_get_dir(snapfile,directorio);
	        //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

	        //cambiamos a ese directorio, siempre que no sea nulo
	        if (directorio[0]!=0) {
	                debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
	                menu_filesel_chdir(directorio);
        	}
	}


	int ret;

	ret=menu_filesel("Select Snapshot",filtros,snapshot_load_file);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);


        if (ret==1) {
		snapfile=snapshot_load_file;

		//sin overlay de texto, que queremos ver las franjas de carga con el color normal (no apagado)
		reset_menu_overlay_function();


			snapshot_load();

		//restauramos modo normal de texto de menu
		set_menu_overlay_function(normal_overlay_texto_menu);

		//Y salimos de todos los menus
		salir_todos_menus=1;
        }


}

void menu_snapshot_save(MENU_ITEM_PARAMETERS)
{

        char *filtros[4];

        if (MACHINE_IS_ZX8081) {
        	filtros[0]="zx";

		if (MACHINE_IS_ZX80) filtros[1]="o";
		else filtros[1]="p";

	        filtros[2]=0;
	}

	else if (MACHINE_IS_Z88) {
		filtros[0]="zx";
		filtros[1]=0;
	}

	else if (MACHINE_IS_SPECTRUM_16_48) {
        	filtros[0]="zx";
	        filtros[1]="z80";
        	filtros[2]="sp";
	        filtros[3]=0;
	}

	else if (MACHINE_IS_ACE) {
		filtros[0]="zx";
		filtros[1]="ace";
		filtros[2]=0;
	}

	else if (MACHINE_IS_CPC) {
                filtros[0]="zx";
                filtros[1]=0;
        }


	else {
        	filtros[0]="zx";
	        filtros[1]="z80";
	        filtros[2]=0;
	}


        if (menu_filesel("Snapshot file",filtros,snapshot_save_file)==1) {

                //Ver si archivo existe y preguntar
                struct stat buf_stat;

                if (stat(snapshot_save_file, &buf_stat)==0) {

			if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) return;

                }


		snapshot_save(snapshot_save_file);

		//Si ha ido bien la grabacion
		if (!if_pending_error_message) menu_generic_message("Save Snapshot","OK. Snapshot saved");

                //Y salimos de todos los menus
                salir_todos_menus=1;

        }


}


void menu_snapshot_save_version(MENU_ITEM_PARAMETERS)
{
	snap_zx_version_save++;
	if (snap_zx_version_save>CURRENT_ZX_VERSION) snap_zx_version_save=1;
}

void menu_snapshot_permitir_versiones_desconocidas(MENU_ITEM_PARAMETERS)
{
	snap_zx_permitir_versiones_desconocidas.v ^=1;
}

void menu_snapshot_autosave_at_interval(MENU_ITEM_PARAMETERS)
{
	snapshot_autosave_interval_enabled.v ^=1;

	//resetear contador
	snapshot_autosave_interval_current_counter=0;
}

void menu_snapshot_autosave_at_interval_seconds(MENU_ITEM_PARAMETERS)
{
	char string_segundos[3];

sprintf (string_segundos,"%d",snapshot_autosave_interval_seconds);

	menu_ventana_scanf("Seconds: ",string_segundos,3);

	int valor_leido=parse_string_to_number(string_segundos);

	if (valor_leido<0 || valor_leido>999) debug_printf(VERBOSE_ERR,"Invalid interval");

	else snapshot_autosave_interval_seconds=valor_leido;
}

void menu_snapshot_autosave_at_interval_prefix(MENU_ITEM_PARAMETERS)
{
	char string_prefix[30];
	//Aunque el limite real es PATH_MAX, lo limito a 30

	sprintf (string_prefix,"%s",snapshot_autosave_interval_name);

	menu_ventana_scanf("Name prefix: ",string_prefix,30);

	if (string_prefix[0]==0) return;

	strcpy(snapshot_autosave_interval_name,string_prefix);
}


void menu_snapshot_autosave_at_interval_directory(MENU_ITEM_PARAMETERS)
{

        char *filtros[2];

        filtros[0]="";
        filtros[1]=0;


        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        int ret;


	char nada[PATH_MAX];

        //Obtenemos ultimo directorio visitado
	menu_filesel_chdir(snapshot_autosave_interval_directory);


        ret=menu_filesel("Enter dir and press ESC",filtros,nada);


	//Si sale con ESC
	if (ret==0) {
		//Directorio root
		sprintf (snapshot_autosave_interval_directory,"%s",menu_filesel_last_directory_seen);
		debug_printf (VERBOSE_DEBUG,"Selected directory: %s",snapshot_autosave_interval_directory);


	}


        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);


}


void menu_snapshot_stop_rzx_play(MENU_ITEM_PARAMETERS)
{
	eject_rzx_file();
}

void menu_snapshot(MENU_ITEM_PARAMETERS)
{

        menu_item *array_menu_snapshot;
        menu_item item_seleccionado;
        int retorno_menu;

        do {
					char string_autosave_interval_prefix[16];
					menu_tape_settings_trunc_name(snapshot_autosave_interval_name,string_autosave_interval_prefix,16);

					char string_autosave_interval_path[16];
					menu_tape_settings_trunc_name(snapshot_autosave_interval_directory,string_autosave_interval_path,16);

                menu_add_item_menu_inicial(&array_menu_snapshot,"~~Load snapshot",MENU_OPCION_NORMAL,menu_snapshot_load,NULL);
		menu_add_item_menu_shortcut(array_menu_snapshot,'l');
		menu_add_item_menu_tooltip(array_menu_snapshot,"Load snapshot");
		menu_add_item_menu_ayuda(array_menu_snapshot,"Supported snapshot formats on load:\n"
					"Z80, ZX, SP, SNA, O, 80, P, 81, Z81");

                menu_add_item_menu(array_menu_snapshot,"~~Save snapshot",MENU_OPCION_NORMAL,menu_snapshot_save,NULL);
		menu_add_item_menu_shortcut(array_menu_snapshot,'s');
		menu_add_item_menu_tooltip(array_menu_snapshot,"Save snapshot of the current machine state");
		menu_add_item_menu_ayuda(array_menu_snapshot,"Supported snapshot formats on save:\n"
					"Z80, ZX, SP, P, O\n"
					"You must write the file name with the extension");

					menu_add_item_menu(array_menu_snapshot,"",MENU_OPCION_SEPARADOR,NULL,NULL);

					if (rzx_reproduciendo) {
						menu_add_item_menu(array_menu_snapshot,"Stop RZX Play",MENU_OPCION_NORMAL,menu_snapshot_stop_rzx_play,NULL);
						menu_add_item_menu(array_menu_snapshot,"",MENU_OPCION_SEPARADOR,NULL,NULL);
					}


					menu_add_item_menu_format(array_menu_snapshot,MENU_OPCION_NORMAL,menu_snapshot_autosave_at_interval,NULL,"Autosave at interval: %s",
									(snapshot_autosave_interval_enabled.v ? "Yes" : "No") );
					menu_add_item_menu_tooltip(array_menu_snapshot,"Autosave snapshot every fixed interval");
					menu_add_item_menu_ayuda(array_menu_snapshot,"Autosave snapshot every fixed interval");

					menu_add_item_menu_format(array_menu_snapshot,MENU_OPCION_NORMAL,menu_snapshot_autosave_at_interval_seconds,NULL," Seconds: %d",snapshot_autosave_interval_seconds);
					menu_add_item_menu_tooltip(array_menu_snapshot,"Save snapshot every desired interval");
					menu_add_item_menu_ayuda(array_menu_snapshot,"Save snapshot every desired interval");

					menu_add_item_menu_format(array_menu_snapshot,MENU_OPCION_NORMAL,menu_snapshot_autosave_at_interval_prefix,NULL," Name Prefix: %s",string_autosave_interval_prefix);
					menu_add_item_menu_tooltip(array_menu_snapshot,"Name prefix for the saved snapshots");
					menu_add_item_menu_ayuda(array_menu_snapshot,"Name prefix for the saved snapshots. The final name will be: prefix-date-hour.zx");

						menu_add_item_menu_format(array_menu_snapshot,MENU_OPCION_NORMAL,menu_snapshot_autosave_at_interval_directory,NULL," Path: %s",string_autosave_interval_path);
						menu_add_item_menu_tooltip(array_menu_snapshot,"Path to save autosnapshots");
						menu_add_item_menu_ayuda(array_menu_snapshot,"Path to save autosnapshots. If not set, will use current directory");


                menu_add_item_menu(array_menu_snapshot,"",MENU_OPCION_SEPARADOR,NULL,NULL);
		menu_add_ESC_item(array_menu_snapshot);

                retorno_menu=menu_dibuja_menu(&snapshot_opcion_seleccionada,&item_seleccionado,array_menu_snapshot,"Snapshot" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}



void menu_debug_reset(MENU_ITEM_PARAMETERS)
{
	if (menu_confirm_yesno("Reset CPU")==1) {
		reset_cpu();
                //Y salimos de todos los menus
                salir_todos_menus=1;
	}

}


/*void menu_debug_prism_usr0(MENU_ITEM_PARAMETERS)
{
	if (menu_confirm_yesno("Set PC=0?")==1) {
		reg_pc=0;
		menu_generic_message("Prism Set PC=0","OK. Set PC=0");
		salir_todos_menus=1;
	}
}
*/


void menu_debug_prism_failsafe(MENU_ITEM_PARAMETERS)
{
	if (menu_confirm_yesno("Reset to failsafe?")==1) {
                reset_cpu();

		prism_failsafe_mode.v=1;
		//Para aplicar cambio de pagina rom
		prism_set_memory_pages();

                //Y salimos de todos los menus
                salir_todos_menus=1;
	}
}

void menu_debug_hard_reset(MENU_ITEM_PARAMETERS)
{
        if (menu_confirm_yesno("Hard Reset CPU")==1) {
                hard_reset_cpu();
                //Y salimos de todos los menus
                salir_todos_menus=1;
        }

}


void menu_debug_nmi(MENU_ITEM_PARAMETERS)
{
        if (menu_confirm_yesno("Generate NMI")==1) {

		generate_nmi();

                //Y salimos de todos los menus
                salir_todos_menus=1;
        }

}

void menu_debug_special_nmi(MENU_ITEM_PARAMETERS)
{
        if (menu_confirm_yesno("Generate Special NMI")==1) {

		char string_nmi[4];
		int valor_nmi;

		sprintf (string_nmi,"0");

		//Pedir valor de NMIEVENT
	        menu_ventana_scanf("NMIEVENT value (0-255)",string_nmi,4);

        	valor_nmi=parse_string_to_number(string_nmi);

	        if (valor_nmi<0 || valor_nmi>255) {
        	        debug_printf (VERBOSE_ERR,"Invalid value %d",valor_nmi);
                	return;
	        }


		generate_nmi();

		//meter BOOTM a 1 (bit 0)
		zxuno_ports[0] |=1;

		//Mapear sram 13
		zxuno_page_ram(13);

		//Valor nmievent
		zxuno_ports[8]=valor_nmi;

                //Y salimos de todos los menus
                salir_todos_menus=1;
        }

}


void menu_debug_verbose(MENU_ITEM_PARAMETERS)
{
	verbose_level++;
	if (verbose_level>4) verbose_level=0;
}

int menu_run_mantransfer_cond(void)
{
	if (MACHINE_IS_SPECTRUM_16_48) return 1;
	return 0;
}

void menu_run_mantransfer(MENU_ITEM_PARAMETERS)
{
	//Cargar mantransfer.bin
	char *mantransfefilename="mantransfev3.bin";
                FILE *ptr_mantransfebin;
                int leidos;

                open_sharedfile(mantransfefilename,&ptr_mantransfebin);
                if (!ptr_mantransfebin)
                {
                        debug_printf(VERBOSE_ERR,"Unable to open mantransfer binary file %s",mantransfefilename);
                        return;
                }

	#define MAX_MANTRANSFE_BIN 1024
	z80_byte buffer[MAX_MANTRANSFE_BIN];

	leidos=fread(buffer,1,MAX_MANTRANSFE_BIN,ptr_mantransfebin);

	//pasarlo a memoria
	int i;

	for (i=0;i<leidos;i++) {
		poke_byte_no_time(16384+i,buffer[i]);
	}

	fclose(ptr_mantransfebin);

	//Establecer modo im1 o im2
	//Al inicio hay
	//regsp defw 0
	//im 1  (0xed,0x56)

	//im2 seria 0xed, 0x5e

	int offset_im_opcode=16384+2+1;
	z80_byte opcode;

	if (im_mode==1 || im_mode==0) opcode=0x56;
	//im2
	else opcode=0x5e;

	poke_byte_no_time(offset_im_opcode,opcode);

	debug_printf (VERBOSE_INFO,"Running mantransfer saving routine");

	//y saltar a la rutina de grabacion de mantransfe
	//Si se cambia la rutina, hay que cambiar este salto tambien
        push_valor(reg_pc);
        reg_pc=16384+0x32;

        //Y salimos de todos los menus
        salir_todos_menus=1;

}




menu_z80_moto_int load_binary_last_address=16384;

void menu_debug_load_binary(MENU_ITEM_PARAMETERS)
{

	char *filtros[2];

	filtros[0]="";
	filtros[1]=0;


        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

	int ret;

        //Obtenemos ultimo directorio visitado
        if (binary_file_load[0]!=0) {
                char directorio[PATH_MAX];
                util_get_dir(binary_file_load,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }

        ret=menu_filesel("Select Binary File",filtros,binary_file_load);

        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);

        if (ret==1) {

		cls_menu_overlay();

        	char string_direccion[8];

		sprintf (string_direccion,"%d",load_binary_last_address);

	        menu_ventana_scanf("Address: ",string_direccion,8);

	        int valor_leido_direccion=parse_string_to_number(string_direccion);

					//printf ("valor: %d\n",valor_leido_direccion);

		if (valor_leido_direccion>65535 && MACHINE_IS_SPECTRUM) {
			debug_printf (VERBOSE_ERR,"Invalid address %d",valor_leido_direccion);
			//menu_generic_message ("Error","Invalid address");
			return;
		}

		load_binary_last_address=valor_leido_direccion;

		cls_menu_overlay();

                char string_longitud[6];

		sprintf (string_longitud,"0");

                menu_ventana_scanf("Length: 0 - all",string_longitud,6);

                int valor_leido_longitud=parse_string_to_number(string_longitud);

		load_binary_file(binary_file_load,valor_leido_direccion,valor_leido_longitud);


		//Y salimos de todos los menus
		salir_todos_menus=1;


        }




}

void menu_debug_save_binary(MENU_ITEM_PARAMETERS)
{

     char *filtros[2];

                filtros[0]="";
                filtros[1]=0;

       int ret;

        ret=menu_filesel("Select Binary File",filtros,binary_file_save);

        if (ret==1) {

		//Ver si archivo existe y preguntar
	        struct stat buf_stat;

                if (stat(binary_file_save, &buf_stat)==0) {

			if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) return;

		}

                cls_menu_overlay();

                char string_direccion[8];

                sprintf (string_direccion,"16384");

                menu_ventana_scanf("Address: ",string_direccion,8);

                int valor_leido_direccion=parse_string_to_number(string_direccion);

                if (MACHINE_IS_SPECTRUM && valor_leido_direccion>65535) {
                        debug_printf (VERBOSE_ERR,"Invalid address %d",valor_leido_direccion);
			//menu_generic_message ("Error","Invalid address");
                        return;
                }

                cls_menu_overlay();

                char string_longitud[6];

                sprintf (string_longitud,"1");

                menu_ventana_scanf("Length: ",string_longitud,6);

                int valor_leido_longitud=parse_string_to_number(string_longitud);

                                        //maximo 64kb
																			if (MACHINE_IS_SPECTRUM) {
                                        if (valor_leido_longitud==0 || valor_leido_longitud>65536) valor_leido_longitud=65536;
																			}
		debug_printf(VERBOSE_INFO,"Saving %s file at %d address with %d bytes",binary_file_save,valor_leido_direccion,valor_leido_longitud);

                                FILE *ptr_binaryfile_save;
                                  ptr_binaryfile_save=fopen(binary_file_save,"wb");
                                  if (!ptr_binaryfile_save)
                                {
                                      debug_printf (VERBOSE_ERR,"Unable to open Binary file %s",binary_file_save);
					//menu_generic_message ("Error","Unable to open Binary file %s",binary_file_save);

                                  }
                                else {



                                                int escritos=1;
                                                z80_byte byte_leido;
                                                while (valor_leido_longitud>0 && escritos>0) {
							//byte_leido=peek_byte_no_time(valor_leido_direccion);
							byte_leido=peek_byte_z80_moto(valor_leido_direccion);
							escritos=fwrite(&byte_leido,1,1,ptr_binaryfile_save);
                                                        valor_leido_direccion++;
                                                        valor_leido_longitud--;
                                                }


                                  fclose(ptr_binaryfile_save);

		                //Y salimos de todos los menus
                		salir_todos_menus=1;

                                }




        }




}

void menu_debug_registers_console(MENU_ITEM_PARAMETERS) {
	debug_registers ^=1;
}

#ifdef EMULATE_VISUALMEM

int visualmem_ancho_variable=30;
int visualmem_alto_variable=22;

#define VISUALMEM_X 1
#define VISUALMEM_Y 1
//#define VISUALMEM_ANCHO 30
#define VISUALMEM_ANCHO (visualmem_ancho_variable)
//#define VISUALMEM_ALTO 15
#define VISUALMEM_ALTO (visualmem_alto_variable)

void menu_debug_draw_visualmem(void)
{

        normal_overlay_texto_menu();


        int ancho=(VISUALMEM_ANCHO-2);
        int alto=(VISUALMEM_ALTO-6);
        int xorigen=(VISUALMEM_X+1);
        int yorigen=(VISUALMEM_Y+4);

        if (si_complete_video_driver() ) {
                ancho *=8;
                alto *=8;
                xorigen *=8;
                yorigen *=8;
        }


	int tamanyo_total=ancho*alto;

        int x,y;


        int inicio_puntero_membuffer,final_puntero_membuffer;

	//Por defecto
	inicio_puntero_membuffer=16384;
	final_puntero_membuffer=65536;

	//printf ("ancho: %d alto: %d\n",ancho,alto);

	//En spectrum 16kb
	if (MACHINE_IS_SPECTRUM_16) {
		//printf ("spec 16kb\n");
		final_puntero_membuffer=32768;
	}

	if (MACHINE_IS_Z88) {
		        inicio_puntero_membuffer=0;
	}

	//En Inves, mostramos modificaciones a la RAM baja
	if (MACHINE_IS_INVES) {
                        inicio_puntero_membuffer=0;
        }



	//En zx80, zx81 mostrar desde 8192 por si tenemos expansion packs
	if (MACHINE_IS_ZX8081) {
		//por defecto
		//printf ("ramtop_zx8081: %d\n",ramtop_zx8081);
		final_puntero_membuffer=ramtop_zx8081+1;

		if (ram_in_8192.v) {
			//printf ("memoria en 8192\n");
			inicio_puntero_membuffer=8192;
		}

        	if (ram_in_32768.v) {
			//printf ("memoria en 32768\n");
			final_puntero_membuffer=49152;
		}

	        if (ram_in_49152.v) {
			//printf ("memoria en 49152\n");
			final_puntero_membuffer=65536;
		}

	}

        //En Jupiter Ace, desde 8192
        if (MACHINE_IS_ACE) {
                //por defecto
                final_puntero_membuffer=ramtop_ace+1;
                inicio_puntero_membuffer=8192;

        }


	//En CPC, desde 0
	if (MACHINE_IS_CPC) {
		inicio_puntero_membuffer=0;
	}

	if (MACHINE_IS_SAM) {
                inicio_puntero_membuffer=0;
        }



	//En modos de RAM en ROM de +2a en puntero apuntara a direccion 0
	if (MACHINE_IS_SPECTRUM_P2A) {
		if ( (puerto_32765 & 32) == 0 ) {
			//paginacion habilitada

			if ( (puerto_8189 & 1) ) {
				//paginacion de ram en rom
				//printf ("paginacion de ram en rom\n");
				inicio_puntero_membuffer=0;
			}
		}
	}

	if (MACHINE_IS_QL) {
		//inicio_puntero_membuffer=0x18000;
		//la ram propiamente empieza en 20000H
		inicio_puntero_membuffer=0x20000;
		final_puntero_membuffer=QL_MEM_LIMIT+1;
	}

        char si_modificado;


             //Calcular cuantos bytes modificados representa un pixel, teniendo en cuenta maximo buffer
	int max_valores=(final_puntero_membuffer-inicio_puntero_membuffer)/tamanyo_total;

	//printf ("max_valores: %d\n",max_valores);
	//le damos uno mas para poder llenar la ventana
	//printf ("inicio: %06XH final: %06XH\n",inicio_puntero_membuffer,final_puntero_membuffer);
	max_valores++;

	for (y=yorigen;y<yorigen+alto;y++) {
        for (x=xorigen;x<xorigen+ancho;x++) {

                //Obtenemos conjunto de bytes modificados

                int valores=max_valores;

		si_modificado=0;
                for (;valores>0;valores--,inicio_puntero_membuffer++) {
                        if (inicio_puntero_membuffer>=final_puntero_membuffer) {
				//printf ("llegado a final con x: %d y: %d ",x,y);
				si_modificado=2;
                        }
			else {
                        	if (visualmem_buffer[inicio_puntero_membuffer]) si_modificado=1;
				clear_visualmembuffer(inicio_puntero_membuffer);
			}
                }

                //dibujamos valor actual
                if (si_modificado==1) {
			if (si_complete_video_driver() ) {
				scr_putpixel_zoom(x,y,ESTILO_GUI_TINTA_NORMAL);
			}

			else {
				putchar_menu_overlay(x,y,'#',ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL);
			}
		}

		//color ficticio para indicar fuera de memoria y por tanto final de ventana... para saber donde acaba
		else if (si_modificado==2) {
			if (si_complete_video_driver() ) {
				scr_putpixel_zoom(x,y,ESTILO_GUI_COLOR_UNUSED_VISUALMEM);
			}
			else {
				putchar_menu_overlay(x,y,'-',ESTILO_GUI_COLOR_UNUSED_VISUALMEM,ESTILO_GUI_PAPEL_NORMAL);
                        }

		}

		else {
			if (si_complete_video_driver() ) {
				scr_putpixel_zoom(x,y,ESTILO_GUI_PAPEL_NORMAL);
			}
			else {
				putchar_menu_overlay(x,y,' ',ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL);
			}
		}

        }
	}

}

void menu_debug_visualmem_dibuja_ventana(void)
{
	menu_dibuja_ventana(VISUALMEM_X,VISUALMEM_Y,VISUALMEM_ANCHO,VISUALMEM_ALTO,"Visual memory");

	menu_escribe_linea_opcion(VISUALMEM_Y,-1,1,"Resize: OPQA");
}

void menu_debug_visualmem(MENU_ITEM_PARAMETERS)
{

        //Desactivamos interlace - si esta. Con interlace la forma de onda se dibuja encima continuamente, sin borrar
        z80_bit copia_video_interlaced_mode;
        copia_video_interlaced_mode.v=video_interlaced_mode.v;

        disable_interlace();


        menu_espera_no_tecla();
	menu_debug_visualmem_dibuja_ventana();

        z80_byte acumulado;




        //Cambiamos funcion overlay de texto de menu
        //Se establece a la de funcion de visualmem + texto
        set_menu_overlay_function(menu_debug_draw_visualmem);


				int valor_contador_segundo_anterior;

				valor_contador_segundo_anterior=contador_segundo;


        do {

          //esto hara ejecutar esto 2 veces por segundo
                //if ( (contador_segundo%500) == 0 || menu_multitarea==0) {
								if ( ((contador_segundo%500) == 0 && valor_contador_segundo_anterior!=contador_segundo) || menu_multitarea==0) {
											valor_contador_segundo_anterior=contador_segundo;
											//printf ("Refrescando contador_segundo=%d\n",contador_segundo);
                       if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();

                }

                menu_cpu_core_loop();
                acumulado=menu_da_todas_teclas();

                //si no hay multitarea, esperar tecla y salir
                if (menu_multitarea==0) {
                        menu_espera_tecla();

                        acumulado=0;
                }

		z80_byte tecla;
		tecla=menu_get_pressed_key();

		if (tecla=='o') {
			menu_espera_no_tecla();
			if (visualmem_ancho_variable>14) visualmem_ancho_variable--;

			cls_menu_overlay();
			menu_debug_visualmem_dibuja_ventana();

			//decir que no hay tecla pulsada
			acumulado = MENU_PUERTO_TECLADO_NINGUNA;


		}

                if (tecla=='p') {
			menu_espera_no_tecla();
                        if (visualmem_ancho_variable<30) visualmem_ancho_variable++;

			cls_menu_overlay();
			menu_debug_visualmem_dibuja_ventana();

                        //decir que no hay tecla pulsada
                        acumulado = MENU_PUERTO_TECLADO_NINGUNA;
                }

                if (tecla=='q') {
                        menu_espera_no_tecla();
                        if (visualmem_alto_variable>7) visualmem_alto_variable--;

                        cls_menu_overlay();
                        menu_debug_visualmem_dibuja_ventana();

                        //decir que no hay tecla pulsada
                        acumulado = MENU_PUERTO_TECLADO_NINGUNA;


                }

                if (tecla=='a') {
                        menu_espera_no_tecla();
                        if (visualmem_alto_variable<22) visualmem_alto_variable++;

                        cls_menu_overlay();
                        menu_debug_visualmem_dibuja_ventana();

                        //decir que no hay tecla pulsada
                        acumulado = MENU_PUERTO_TECLADO_NINGUNA;
                }


        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA);

        //Restauramos modo interlace
        if (copia_video_interlaced_mode.v) enable_interlace();

       //restauramos modo normal de texto de menu
       set_menu_overlay_function(normal_overlay_texto_menu);


        cls_menu_overlay();

}

#endif


void menu_debug_lost_vsync(MENU_ITEM_PARAMETERS)
{
	simulate_lost_vsync.v ^=1;
}

void menu_input_file_keyboard_insert(MENU_ITEM_PARAMETERS)
{

        if (input_file_keyboard_inserted.v==0) {
                input_file_keyboard_init();
        }

        else if (input_file_keyboard_inserted.v==1) {
                input_file_keyboard_close();
        }

}



int menu_input_file_keyboard_cond(void)
{
        if (input_file_keyboard_name!=NULL) return 1;
        else return 0;
}

void menu_input_file_keyboard_delay(MENU_ITEM_PARAMETERS)
{
        if (input_file_keyboard_delay==1) input_file_keyboard_delay=2;
        else if (input_file_keyboard_delay==2) input_file_keyboard_delay=5;
        else if (input_file_keyboard_delay==5) input_file_keyboard_delay=10;
        else if (input_file_keyboard_delay==10) input_file_keyboard_delay=25;
        else if (input_file_keyboard_delay==25) input_file_keyboard_delay=50;

        else input_file_keyboard_delay=1;
}

void menu_input_file_keyboard(MENU_ITEM_PARAMETERS)
{

        input_file_keyboard_inserted.v=0;


        char *filtros[2];

        filtros[0]="txt";
        filtros[1]=0;


        if (menu_filesel("Select Spool File",filtros,input_file_keyboard_name_buffer)==1) {
		input_file_keyboard_name=input_file_keyboard_name_buffer;

        }

        else {
		input_file_keyboard_name=NULL;
		eject_input_file_keyboard();
        }
}


void menu_input_file_keyboard_send_pause(MENU_ITEM_PARAMETERS)
{
	input_file_keyboard_send_pause.v ^=1;
}


void menu_input_file_keyboard_turbo(MENU_ITEM_PARAMETERS)
{

	if (input_file_keyboard_turbo.v) {
		reset_peek_byte_function_spoolturbo();
		input_file_keyboard_turbo.v=0;
	}

	else {
		set_peek_byte_function_spoolturbo();
		input_file_keyboard_turbo.v=1;
	}
}


int menu_input_file_keyboard_turbo_cond(void)
{
	if (MACHINE_IS_SPECTRUM) return 1;
	return 0;
}

#ifdef EMULATE_CPU_STATS

void menu_debug_cpu_stats_clear_disassemble_array(void)
{
	int i;

	for (i=0;i<DISASSEMBLE_ARRAY_LENGTH;i++) disassemble_array[i]=0;
}

void menu_debug_cpu_stats_diss_no_print(z80_byte index,z80_byte opcode,char *dumpassembler)
{

        size_t longitud_opcode;

        disassemble_array[index]=opcode;

        debugger_disassemble_array(dumpassembler,31,&longitud_opcode,0);
}


/*
void menu_debug_cpu_stats_diss(z80_byte index,z80_byte opcode,int linea)
{

	char dumpassembler[32];

	//Empezar con espacio
	dumpassembler[0]=' ';

	menu_debug_cpu_stats_diss_no_print(index,opcode,&dumpassembler[1]);

	menu_escribe_linea_opcion(linea,-1,1,dumpassembler);
}
*/

void menu_debug_cpu_stats_diss_complete_no_print (z80_byte opcode,char *buffer,z80_byte preffix1,z80_byte preffix2)
{

	//Sin prefijo
	if (preffix1==0) {
                        menu_debug_cpu_stats_clear_disassemble_array();
                        menu_debug_cpu_stats_diss_no_print(0,opcode,buffer);
	}

	//Con 1 solo prefijo
	else if (preffix2==0) {
                        menu_debug_cpu_stats_clear_disassemble_array();
                        disassemble_array[0]=preffix1;
                        menu_debug_cpu_stats_diss_no_print(1,opcode,buffer);
	}

	//Con 2 prefijos (DD/FD + CB)
	else {
                        //Opcode
                        menu_debug_cpu_stats_clear_disassemble_array();
                        disassemble_array[0]=preffix1;
                        disassemble_array[1]=preffix2;
                        disassemble_array[2]=0;
                        menu_debug_cpu_stats_diss_no_print(3,opcode,buffer);
	}
}


void menu_debug_cpu_resumen_stats(MENU_ITEM_PARAMETERS)
{

        char textostats[32];

        menu_espera_no_tecla();
        menu_dibuja_ventana(0,1,32,18,"CPU Compact Statistics");

        z80_byte acumulado;

        char dumpassembler[32];

        //Empezar con espacio
        dumpassembler[0]=' ';

				int valor_contador_segundo_anterior;

				valor_contador_segundo_anterior=contador_segundo;


        do {


                //esto hara ejecutar esto 2 veces por segundo
                //if ( (contador_segundo%500) == 0 || menu_multitarea==0) {
									if ( ((contador_segundo%500) == 0 && valor_contador_segundo_anterior!=contador_segundo) || menu_multitarea==0) {
											valor_contador_segundo_anterior=contador_segundo;
                        //contador_segundo_anterior=contador_segundo;
												//printf ("Refrescando. contador_segundo=%d\n",contador_segundo);

			int linea=0;
                        int opcode;

			unsigned int sumatotal;
                        sumatotal=util_stats_sum_all_counters();
                        sprintf (textostats,"Total opcodes run: %u",sumatotal);
                        menu_escribe_linea_opcion(linea++,-1,1,textostats);


			menu_escribe_linea_opcion(linea++,-1,1,"Most used op. for each preffix");

                        opcode=util_stats_find_max_counter(stats_codsinpr);
                        sprintf (textostats,"Op nopref:    %02XH: %u",opcode,util_stats_get_counter(stats_codsinpr,opcode) );
                        menu_escribe_linea_opcion(linea++,-1,1,textostats);

                        //Opcode
			menu_debug_cpu_stats_diss_complete_no_print(opcode,&dumpassembler[1],0,0);
			menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);

			//menu_debug_cpu_stats_clear_disassemble_array();
                        //menu_debug_cpu_stats_diss(0,opcode,linea);
                        //linea++;



                        opcode=util_stats_find_max_counter(stats_codpred);
                        sprintf (textostats,"Op pref ED:   %02XH: %u",opcode,util_stats_get_counter(stats_codpred,opcode) );
                        menu_escribe_linea_opcion(linea++,-1,1,textostats);

                        //Opcode
                        menu_debug_cpu_stats_diss_complete_no_print(opcode,&dumpassembler[1],237,0);
                        menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);

			//menu_debug_cpu_stats_clear_disassemble_array();
			//disassemble_array[0]=237;
                        //menu_debug_cpu_stats_diss(1,opcode,linea);
                        //linea++;




                        opcode=util_stats_find_max_counter(stats_codprcb);
                        sprintf (textostats,"Op pref CB:   %02XH: %u",opcode,util_stats_get_counter(stats_codprcb,opcode) );
                        menu_escribe_linea_opcion(linea++,-1,1,textostats);

                        //Opcode
                        menu_debug_cpu_stats_diss_complete_no_print(opcode,&dumpassembler[1],203,0);
                        menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);

                        //menu_debug_cpu_stats_clear_disassemble_array();
                        //disassemble_array[0]=203;
                        //menu_debug_cpu_stats_diss(1,opcode,linea);
                        //linea++;


                        opcode=util_stats_find_max_counter(stats_codprdd);
                        sprintf (textostats,"Op pref DD:   %02XH: %u",opcode,util_stats_get_counter(stats_codprdd,opcode) );
                        menu_escribe_linea_opcion(linea++,-1,1,textostats);

                        //Opcode
                        menu_debug_cpu_stats_diss_complete_no_print(opcode,&dumpassembler[1],221,0);
                        menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);

                        //menu_debug_cpu_stats_clear_disassemble_array();
                        //disassemble_array[0]=221;
                        //menu_debug_cpu_stats_diss(1,opcode,linea);
                        //linea++;


                        opcode=util_stats_find_max_counter(stats_codprfd);
                        sprintf (textostats,"Op pref FD:   %02XH: %u",opcode,util_stats_get_counter(stats_codprfd,opcode) );
                        menu_escribe_linea_opcion(linea++,-1,1,textostats);

                        //Opcode
                        menu_debug_cpu_stats_diss_complete_no_print(opcode,&dumpassembler[1],253,0);
                        menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);

                        //menu_debug_cpu_stats_clear_disassemble_array();
                        //disassemble_array[0]=253;
                        //menu_debug_cpu_stats_diss(1,opcode,linea);
                        //linea++;



                        opcode=util_stats_find_max_counter(stats_codprddcb);
                        sprintf (textostats,"Op pref DDCB: %02XH: %u",opcode,util_stats_get_counter(stats_codprddcb,opcode) );
                        menu_escribe_linea_opcion(linea++,-1,1,textostats);

                        //Opcode
                        menu_debug_cpu_stats_diss_complete_no_print(opcode,&dumpassembler[1],221,203);
                        menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);

                        //menu_debug_cpu_stats_clear_disassemble_array();
                        //disassemble_array[0]=221;
                        //disassemble_array[1]=203;
                        //disassemble_array[2]=0;
                        //menu_debug_cpu_stats_diss(3,opcode,linea);
                        //linea++;


                        opcode=util_stats_find_max_counter(stats_codprfdcb);
                        sprintf (textostats,"Op pref FDCB: %02XH: %u",opcode,util_stats_get_counter(stats_codprfdcb,opcode) );
                        menu_escribe_linea_opcion(linea++,-1,1,textostats);

                        //Opcode
                        menu_debug_cpu_stats_diss_complete_no_print(opcode,&dumpassembler[1],253,203);
                        menu_escribe_linea_opcion(linea++,-1,1,dumpassembler);

                        //menu_debug_cpu_stats_clear_disassemble_array();
                        //disassemble_array[0]=253;
                        //disassemble_array[1]=203;
                        //disassemble_array[2]=0;
                        //menu_debug_cpu_stats_diss(3,opcode,linea);
                        //linea++;



                        if (menu_multitarea==0) all_interlace_scr_refresca_pantalla();

                }

                menu_cpu_core_loop();
                acumulado=menu_da_todas_teclas();

                //si no hay multitarea, esperar tecla y salir
                if (menu_multitarea==0) {
                        menu_espera_tecla();

                        acumulado=0;
                }

        } while ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) ==MENU_PUERTO_TECLADO_NINGUNA);

        cls_menu_overlay();

}

void menu_cpu_full_stats(unsigned int *stats_table,char *title,z80_byte preffix1,z80_byte preffix2)
{

	int index_op,index_buffer;
	unsigned int counter;

	char stats_buffer[MAX_TEXTO_GENERIC_MESSAGE];

	char dumpassembler[32];

	//margen suficiente para que quepa una linea y un contador int de 32 bits...
	//aunque si pasa el ancho de linea, la rutina de generic_message lo troceara
	char buf_linea[64];

	index_buffer=0;

	for (index_op=0;index_op<256;index_op++) {
		counter=util_stats_get_counter(stats_table,index_op);

		menu_debug_cpu_stats_diss_complete_no_print(index_op,dumpassembler,preffix1,preffix2);

		sprintf (buf_linea,"%02X %-16s: %u \n",index_op,dumpassembler,counter);
		//16 ocupa la instruccion mas larga: LD B,RLC (IX+dd)

		sprintf (&stats_buffer[index_buffer],"%s\n",buf_linea);
		//sprintf (&stats_buffer[index_buffer],"%02X: %11d\n",index_op,counter);
		//index_buffer +=16;
		index_buffer +=strlen(buf_linea);
	}

	stats_buffer[index_buffer]=0;

	menu_generic_message(title,stats_buffer);

}

void menu_cpu_full_stats_codsinpr(MENU_ITEM_PARAMETERS)
{
	menu_cpu_full_stats(stats_codsinpr,"Full statistic no preffix",0,0);
}

void menu_cpu_full_stats_codpred(MENU_ITEM_PARAMETERS)
{
        menu_cpu_full_stats(stats_codpred,"Full statistic pref ED",0xED,0);
}

void menu_cpu_full_stats_codprcb(MENU_ITEM_PARAMETERS)
{
        menu_cpu_full_stats(stats_codprcb,"Full statistic pref CB",0xCB,0);
}

void menu_cpu_full_stats_codprdd(MENU_ITEM_PARAMETERS)
{
        menu_cpu_full_stats(stats_codprdd,"Full statistic pref DD",0xDD,0);
}

void menu_cpu_full_stats_codprfd(MENU_ITEM_PARAMETERS)
{
        menu_cpu_full_stats(stats_codprfd,"Full statistic pref FD",0xFD,0);
}

void menu_cpu_full_stats_codprddcb(MENU_ITEM_PARAMETERS)
{
        menu_cpu_full_stats(stats_codprddcb,"Full statistic pref DDCB",0xDD,0xCB);
}

void menu_cpu_full_stats_codprfdcb(MENU_ITEM_PARAMETERS)
{
        menu_cpu_full_stats(stats_codprfdcb,"Full statistic pref FDCB",0xFD,0xCB);
}


void menu_cpu_full_stats_clear(MENU_ITEM_PARAMETERS)
{
	util_stats_init();

	menu_generic_message("Clear CPU statistics","OK. Statistics cleared");
}


void menu_debug_cpu_stats(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_cpu_stats;
        menu_item item_seleccionado;
        int retorno_menu;
        do {
                menu_add_item_menu_inicial_format(&array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_debug_cpu_resumen_stats,NULL,"Compact Statistics");
                menu_add_item_menu_tooltip(array_menu_cpu_stats,"Shows Compact CPU Statistics");
                menu_add_item_menu_ayuda(array_menu_cpu_stats,"Shows the most used opcode for every type: without preffix, with ED preffix, "
					"etc. CPU Statistics are reset when changing machine or resetting CPU.");


                menu_add_item_menu_format(array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_cpu_full_stats_codsinpr,NULL,"Full Statistics No pref");
		menu_add_item_menu_format(array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_cpu_full_stats_codpred,NULL,"Full Statistics Pref ED");
		menu_add_item_menu_format(array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_cpu_full_stats_codprcb,NULL,"Full Statistics Pref CB");
		menu_add_item_menu_format(array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_cpu_full_stats_codprdd,NULL,"Full Statistics Pref DD");
		menu_add_item_menu_format(array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_cpu_full_stats_codprfd,NULL,"Full Statistics Pref FD");
		menu_add_item_menu_format(array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_cpu_full_stats_codprddcb,NULL,"Full Statistics Pref DDCB");
		menu_add_item_menu_format(array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_cpu_full_stats_codprfdcb,NULL,"Full Statistics Pref FDCB");
		menu_add_item_menu_format(array_menu_cpu_stats,MENU_OPCION_NORMAL,menu_cpu_full_stats_clear,NULL,"Clear Statistics");


                menu_add_item_menu(array_menu_cpu_stats,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_cpu_stats,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_cpu_stats);

                retorno_menu=menu_dibuja_menu(&cpu_stats_opcion_seleccionada,&item_seleccionado,array_menu_cpu_stats,"CPU Statistics" );
		cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}

#endif


int get_efectivo_tamanyo_find_buffer(void)
{
	if (MACHINE_IS_QL) return QL_MEM_LIMIT+1;
	return 65536;
}

//int menu_find_total_items=0;


//Indica si esta vacio o no; esto se usa para saber si la busqueda se hace sobre la busqueda anterior o no
int menu_find_empty=1;

unsigned char *menu_find_mem_pointer=NULL;
/* Estructura del array de busqueda:
65536 items del array. Cada item es un unsigned char
El valor indica:
0: en esa posicion de la memoria no se encuentra el byte
1: en esa posicion de la memoria si se encuentra el byte
otros valores de momento no tienen significado
*/

void menu_find_clear_results(MENU_ITEM_PARAMETERS)
{
        //inicializamos array
        int i;

        for (i=0;i<get_efectivo_tamanyo_find_buffer();i++) menu_find_mem_pointer[i]=0;

        menu_find_empty=1;

        menu_generic_message("Clear Results","OK. Results cleared");
}

void menu_find_view_results(MENU_ITEM_PARAMETERS)
{

        int index_find,index_buffer;

        char results_buffer[MAX_TEXTO_GENERIC_MESSAGE];

        //margen suficiente para que quepa una linea
        //direccion+salto linea+codigo 0
        char buf_linea[9];

        index_buffer=0;

        int encontrados=0;

        int salir=0;

        for (index_find=0;index_find<get_efectivo_tamanyo_find_buffer() && salir==0;index_find++) {
                if (menu_find_mem_pointer[index_find]) {
                        sprintf (buf_linea,"%d\n",index_find);
                        sprintf (&results_buffer[index_buffer],"%s\n",buf_linea);
                        index_buffer +=strlen(buf_linea);
                        encontrados++;
                }

                //controlar maximo
                //20 bytes de margen
                if (index_buffer>MAX_TEXTO_GENERIC_MESSAGE-20) {
                        debug_printf (VERBOSE_ERR,"Too many results to show. Showing only the first %d",encontrados);
                        //forzar salir
                        salir=1;
                }

        }

        results_buffer[index_buffer]=0;

        menu_generic_message("View Results",results_buffer);
}

void menu_find_find(MENU_ITEM_PARAMETERS)
{

        //Buscar en la memoria direccionable (0...65535) si se encuentra el byte
        z80_byte byte_to_find;


        char string_find[4];

        sprintf (string_find,"0");

        menu_ventana_scanf("Find byte",string_find,4);

        int valor_find=parse_string_to_number(string_find);

        if (valor_find<0 || valor_find>255) {
                debug_printf (VERBOSE_ERR,"Invalid value %d",valor_find);
                return;
        }


        byte_to_find=valor_find;


        int dir;
        int total_items_found=0;
				int final_find=get_efectivo_tamanyo_find_buffer();

        //Busqueda con array no inicializado
        if (menu_find_empty) {

                debug_printf (VERBOSE_INFO,"Starting Search with no previous results");


                //asumimos que no va a encontrar nada
                menu_find_empty=1;

                for (dir=0;dir<final_find;dir++) {
                        if (peek_byte_z80_moto(dir)==byte_to_find) {
                                menu_find_mem_pointer[dir]=1;

                                //al menos hay un resultado
                                menu_find_empty=0;

                                //incrementamos contador de resultados para mostrar al final
                                total_items_found++;
                        }
                }

        }

        else {
                //Busqueda con array ya con contenido
                //examinar solo las direcciones que indique el array

                debug_printf (VERBOSE_INFO,"Starting Search using previous results");

                //asumimos que no va a encontrar nada
                menu_find_empty=1;

                int i;
                for (i=0;i<final_find;i++) {
                        if (menu_find_mem_pointer[i]) {
                                //Ver el contenido de esa direccion
                                if (peek_byte_z80_moto(i)==byte_to_find) {
                                        //al menos hay un resultado
                                        menu_find_empty=0;
                                        //incrementamos contador de resultados para mostrar al final
                                        total_items_found++;
                                }
                                else {
                                        //el byte ya no esta en esa direccion
                                        menu_find_mem_pointer[i]=0;
                                }
                        }
                }
        }


        menu_generic_message_format("Find","Total addresses found: %d",total_items_found);


}



//int total_tamanyo_find_buffer=0;



void menu_find(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_find;
        menu_item item_seleccionado;
        int retorno_menu;

        //Si puntero memoria no esta asignado, asignarlo, o si hemos cambiado de tipo de maquina
        if (menu_find_mem_pointer==NULL) {

                //65536 elementos del array, cada uno de tamanyo unsigned char
								//total_tamanyo_find_buffer=get_total_tamanyo_find_buffer();

                menu_find_mem_pointer=malloc(QL_MEM_LIMIT+1); //Asignamos el maximo (maquina QL)
                if (menu_find_mem_pointer==NULL) cpu_panic("Error allocating find array");

                //inicializamos array
                int i;
                for (i=0;i<get_efectivo_tamanyo_find_buffer();i++) menu_find_mem_pointer[i]=0;
        }


        do {

                menu_add_item_menu_inicial_format(&array_menu_find,MENU_OPCION_NORMAL,menu_find_find,NULL,"Find byte");
                menu_add_item_menu_tooltip(array_menu_find,"Find one byte on memory");
                menu_add_item_menu_ayuda(array_menu_find,"Find one byte on the 64 KB of mapped memory, considering the last address found (if any).\n"
                        "It can be used to find POKEs, it's very easy: \n"
                        "I first recommend to disable Multitasking menu, to avoid losing lives where in the menu.\n"
                        "As an example, you are in a game with 4 lives. Enter find byte "
                        "with value 4. It will find a lot of addresses, don't panic.\n"
                        "Then, you must lose one live, you have now 3. Then you must search for byte with value 3; "
                        "the search will be made considering the last search results. Normally here you will find only "
                        "one address with value 3. At this moment you know the address where the lives are stored.\n"
                        "The following to find, for example, infinite lives, is to search where this life value is decremented. "
                        "You can find it by making a MRA breakpoint to the address where the lives are stored, or a condition breakpoint "
			"(NN)=value, setting NN to the address where lives are stored, and value to the desired number of lives, for example: (51308)=2.\n"
                        "When the breakpoint is caught, you will probably have the section of code where the lives are decremented ;) ");

                menu_add_item_menu_format(array_menu_find,MENU_OPCION_NORMAL,menu_find_view_results,NULL,"View results");
                menu_add_item_menu_tooltip(array_menu_find,"View results");
                menu_add_item_menu_ayuda(array_menu_find,"View results");

                menu_add_item_menu_format(array_menu_find,MENU_OPCION_NORMAL,menu_find_clear_results,NULL,"Clear results");
                menu_add_item_menu_tooltip(array_menu_find,"Clear results");
                menu_add_item_menu_ayuda(array_menu_find,"Clear results");




                menu_add_item_menu(array_menu_find,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                menu_add_ESC_item(array_menu_find);

                retorno_menu=menu_dibuja_menu(&find_opcion_seleccionada,&item_seleccionado,array_menu_find,"Find" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC);
}








void menu_debug_input_file_keyboard(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_input_file_keyboard;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char string_input_file_keyboard_shown[16];
                menu_tape_settings_trunc_name(input_file_keyboard_name,string_input_file_keyboard_shown,16);
                menu_add_item_menu_inicial_format(&array_menu_input_file_keyboard,MENU_OPCION_NORMAL,menu_input_file_keyboard,NULL,"Spool file: %s",string_input_file_keyboard_shown);


                menu_add_item_menu_format(array_menu_input_file_keyboard,MENU_OPCION_NORMAL,menu_input_file_keyboard_insert,menu_input_file_keyboard_cond,"Spool file inserted: %s",(input_file_keyboard_inserted.v ? "Yes" : "No" ));
                if (input_file_keyboard_inserted.v) {


			menu_add_item_menu_format(array_menu_input_file_keyboard,MENU_OPCION_NORMAL,menu_input_file_keyboard_turbo,menu_input_file_keyboard_turbo_cond,"Turbo mode: %s",(input_file_keyboard_turbo.v ? "Yes" : "No") );
			menu_add_item_menu_tooltip(array_menu_input_file_keyboard,"Allow turbo mode on Spectrum models");
			menu_add_item_menu_ayuda(array_menu_input_file_keyboard,"Allow turbo mode on Spectrum models. It traps calls to function ROMS when keyboard is read");


			if (input_file_keyboard_turbo.v==0) {

	                        menu_add_item_menu_format(array_menu_input_file_keyboard,MENU_OPCION_NORMAL,menu_input_file_keyboard_delay,NULL,"Key length: %d ms",input_file_keyboard_delay*1000/50);
        	                menu_add_item_menu_tooltip(array_menu_input_file_keyboard,"Length of every key pressed");
                	        menu_add_item_menu_ayuda(array_menu_input_file_keyboard,"I recommend 100 ms for entering lines on Spectrum BASIC. I also suggest to send some manual delays, using unhandled character, like \\, to assure entering lines is correct ");

	                        menu_add_item_menu_format(array_menu_input_file_keyboard,MENU_OPCION_NORMAL,menu_input_file_keyboard_send_pause,NULL,"Delay after every key: %s",(input_file_keyboard_send_pause.v==1 ? "Yes" : "No") );
        	                menu_add_item_menu_tooltip(array_menu_input_file_keyboard,"Send or not a delay of the same duration after every key");
                	        menu_add_item_menu_ayuda(array_menu_input_file_keyboard,"I recommend enabling this for entering lines on Spectrum BASIC");


			}
                }


                menu_add_item_menu(array_menu_input_file_keyboard,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_input_file_keyboard,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_input_file_keyboard);

                retorno_menu=menu_dibuja_menu(&input_file_keyboard_opcion_seleccionada,&item_seleccionado,array_menu_input_file_keyboard,"Input File Spooling" );
		cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}


void menu_cpu_transaction_log_enable(MENU_ITEM_PARAMETERS)
{
	if (cpu_transaction_log_enabled.v) {
		reset_cpu_core_transaction_log();
	}
	else {

		if (menu_confirm_yesno_texto("May use lot of disk","Sure?")==1)
			set_cpu_core_transaction_log();
	}
}

void menu_cpu_transaction_log_file(MENU_ITEM_PARAMETERS)
{

	if (cpu_transaction_log_enabled.v) reset_cpu_core_transaction_log();

        char *filtros[2];

        filtros[0]="log";
        filtros[1]=0;


        if (menu_filesel("Select Log File",filtros,transaction_log_filename)==1) {
                //Ver si archivo existe y preguntar
		if (si_existe_archivo(transaction_log_filename)) {
                        if (menu_confirm_yesno_texto("File exists","Append?")==0) {
				transaction_log_filename[0]=0;
				return;
			}
                }

        }

	else transaction_log_filename[0]=0;

}


void menu_cpu_transaction_log_enable_address(MENU_ITEM_PARAMETERS)
{
	cpu_transaction_log_store_address.v ^=1;
}

void menu_cpu_transaction_log_enable_datetime(MENU_ITEM_PARAMETERS)
{
        cpu_transaction_log_store_datetime.v ^=1;
}


void menu_cpu_transaction_log_enable_tstates(MENU_ITEM_PARAMETERS)
{
        cpu_transaction_log_store_tstates.v ^=1;
}

void menu_cpu_transaction_log_enable_opcode(MENU_ITEM_PARAMETERS)
{
        cpu_transaction_log_store_opcode.v ^=1;
}

void menu_cpu_transaction_log_enable_registers(MENU_ITEM_PARAMETERS)
{
        cpu_transaction_log_store_registers.v ^=1;
}



void menu_cpu_transaction_log(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_cpu_transaction_log;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char string_transactionlog_shown[18];
                menu_tape_settings_trunc_name(transaction_log_filename,string_transactionlog_shown,18);

                menu_add_item_menu_inicial_format(&array_menu_cpu_transaction_log,MENU_OPCION_NORMAL,menu_cpu_transaction_log_file,NULL,"Log file: %s",string_transactionlog_shown );


                if (transaction_log_filename[0]!=0) {
                        menu_add_item_menu_format(array_menu_cpu_transaction_log,MENU_OPCION_NORMAL,menu_cpu_transaction_log_enable,NULL,"Transaction log enabled: %s",(cpu_transaction_log_enabled.v ? "Yes" : "No") );
                }


		menu_add_item_menu_format(array_menu_cpu_transaction_log,MENU_OPCION_NORMAL,menu_cpu_transaction_log_enable_datetime,NULL,"Store Date & Time: %s",(cpu_transaction_log_store_datetime.v ? "Yes" : "No"));
		menu_add_item_menu_format(array_menu_cpu_transaction_log,MENU_OPCION_NORMAL,menu_cpu_transaction_log_enable_tstates,NULL,"Store T-States: %s",(cpu_transaction_log_store_tstates.v ? "Yes" : "No"));
		menu_add_item_menu_format(array_menu_cpu_transaction_log,MENU_OPCION_NORMAL,menu_cpu_transaction_log_enable_address,NULL,"Store Address: %s",(cpu_transaction_log_store_address.v ? "Yes" : "No"));
		menu_add_item_menu_format(array_menu_cpu_transaction_log,MENU_OPCION_NORMAL,menu_cpu_transaction_log_enable_opcode,NULL,"Store Opcode: %s",(cpu_transaction_log_store_opcode.v ? "Yes" : "No"));
		menu_add_item_menu_format(array_menu_cpu_transaction_log,MENU_OPCION_NORMAL,menu_cpu_transaction_log_enable_registers,NULL,"Store Registers: %s",(cpu_transaction_log_store_registers.v ? "Yes" : "No"));


               menu_add_item_menu(array_menu_cpu_transaction_log,"",MENU_OPCION_SEPARADOR,NULL,NULL);

                menu_add_ESC_item(array_menu_cpu_transaction_log);

                retorno_menu=menu_dibuja_menu(&cpu_transaction_log_opcion_seleccionada,&item_seleccionado,array_menu_cpu_transaction_log,"CPU Transaction Log" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}



void menu_debug_view_basic(MENU_ITEM_PARAMETERS)
{

	char results_buffer[MAX_TEXTO_GENERIC_MESSAGE];
	debug_view_basic(results_buffer);

  menu_generic_message_format("View Basic","%s",results_buffer);
}


void menu_file_viewer_read_file(char *title,char *file_name)
{

	char file_read_memory[MAX_TEXTO_GENERIC_MESSAGE];



	debug_printf (VERBOSE_INFO,"Loading %s File",file_name);
	FILE *ptr_file_name;
	ptr_file_name=fopen(file_name,"rb");


	if (!ptr_file_name)
	{
		debug_printf (VERBOSE_ERR,"Unable to open %s file",file_name);
	}
	else {

		int leidos=fread(file_read_memory,1,MAX_TEXTO_GENERIC_MESSAGE,ptr_file_name);
		debug_printf (VERBOSE_INFO,"Read %d bytes of file: %s",leidos,file_name);

		if (leidos==MAX_TEXTO_GENERIC_MESSAGE) {
			debug_printf (VERBOSE_ERR,"Read max text buffer: %d bytes. Showing only these",leidos);
			leidos--;
		}

		file_read_memory[leidos]=0;


		fclose(ptr_file_name);

		menu_generic_message(title,file_read_memory);

	}

}



void menu_debug_file_viewer(MENU_ITEM_PARAMETERS)
{


	int ret=1;

	while (ret!=0) {
			char *filtros[2];

			filtros[0]="";
			filtros[1]=0;


        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);


        //Obtenemos ultimo directorio visitado
        if (file_viewer_file_name[0]!=0) {
                char directorio[PATH_MAX];
                util_get_dir(file_viewer_file_name,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }

        ret=menu_filesel("Select File",filtros,file_viewer_file_name);

        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);

        if (ret==1) {

						cls_menu_overlay();
						menu_file_viewer_read_file("File view",file_viewer_file_name);
				}

    }



}


int menu_debug_view_basic_cond(void)
{
	if (MACHINE_IS_Z88) return 0;
	if (MACHINE_IS_ACE) return 0;
	if (MACHINE_IS_CPC) return 0;
	if (MACHINE_IS_SAM) return 0;
	if (MACHINE_IS_QL) return 0;
	return 1;
}

//menu debug settings
void menu_debug_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_debug_settings;
        menu_item item_seleccionado;
	int retorno_menu;
        do {
                menu_add_item_menu_inicial(&array_menu_debug_settings,"~~Reset",MENU_OPCION_NORMAL,menu_debug_reset,NULL);
		menu_add_item_menu_shortcut(array_menu_debug_settings,'r');

		if (MACHINE_IS_PRISM) {
			//Reset to failsafe
			menu_add_item_menu(array_menu_debug_settings,"Reset to Failsafe mode",MENU_OPCION_NORMAL,menu_debug_prism_failsafe,NULL);

			//Para Testeo. usr0
			//menu_add_item_menu(array_menu_debug_settings,"Set PC=0",MENU_OPCION_NORMAL,menu_debug_prism_usr0,NULL);

		}

		if (MACHINE_IS_Z88 || MACHINE_IS_ZXUNO || MACHINE_IS_PRISM || MACHINE_IS_TBBLUE || superupgrade_enabled.v) {
	                menu_add_item_menu(array_menu_debug_settings,"~~Hard Reset",MENU_OPCION_NORMAL,menu_debug_hard_reset,NULL);
			menu_add_item_menu_shortcut(array_menu_debug_settings,'h');
	                menu_add_item_menu_tooltip(array_menu_debug_settings,"Hard resets the machine");
	                menu_add_item_menu_ayuda(array_menu_debug_settings,"Hard resets the machine.\n"
				"On Z88, it's the same as opening flap and pressing reset button.\n"
				"On ZX-Uno, it's the same as pressing Ctrl-Alt-Backspace or powering off and on the machine"
				);
		}

		if (!CPU_IS_MOTOROLA) {
    menu_add_item_menu(array_menu_debug_settings,"Generate ~~NMI",MENU_OPCION_NORMAL,menu_debug_nmi,NULL);
		menu_add_item_menu_shortcut(array_menu_debug_settings,'n');
		}

		if (MACHINE_IS_ZXUNO_BOOTM_DISABLED) {
	                menu_add_item_menu(array_menu_debug_settings,"Generate Special NMI",MENU_OPCION_NORMAL,menu_debug_special_nmi,NULL);
		}

		menu_add_item_menu(array_menu_debug_settings,"~~Debug CPU & ULA",MENU_OPCION_NORMAL,menu_debug_registers,NULL);
		menu_add_item_menu_shortcut(array_menu_debug_settings,'d');
		menu_add_item_menu_tooltip(array_menu_debug_settings,"Open debug window");
		menu_add_item_menu_ayuda(array_menu_debug_settings,"This window opens the debugger. You can see there some Z80 registers "
					"easily recognizable. Some other variables and entries need further explanation:\n"
					"TSTATES: T-states total in a frame\n"
					"TSTATL: T-states total in a scanline\n"
					"TSTATP: T-states partial. This is a counter that you can reset with key P"



					);

		if (!CPU_IS_MOTOROLA) {
		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_ioports,NULL,"Debug ~~I/O Ports");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'i');

		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_cpu_transaction_log,NULL,"~~CPU Transaction Log");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'c');
		}

		menu_add_item_menu(array_menu_debug_settings,"View He~~xdump",MENU_OPCION_NORMAL,menu_debug_hexdump,NULL);
		menu_add_item_menu_shortcut(array_menu_debug_settings,'x');

		menu_add_item_menu(array_menu_debug_settings,"View ~~Basic",MENU_OPCION_NORMAL,menu_debug_view_basic,menu_debug_view_basic_cond);
		menu_add_item_menu_shortcut(array_menu_debug_settings,'b');

		if (si_complete_video_driver() ) {
			menu_add_item_menu(array_menu_debug_settings,"View ~~Sprites",MENU_OPCION_NORMAL,menu_debug_view_sprites,NULL);
			menu_add_item_menu_shortcut(array_menu_debug_settings,'s');
		}

#ifdef EMULATE_CPU_STATS
		if (!CPU_IS_MOTOROLA) {
		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_cpu_stats,NULL,"View CPU S~~tatistics");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'t');
		}

#endif

#ifdef EMULATE_VISUALMEM
			//if (!CPU_IS_MOTOROLA) {
			menu_add_item_menu(array_menu_debug_settings,"~~Visual memory",MENU_OPCION_NORMAL,menu_debug_visualmem,NULL);
			menu_add_item_menu_shortcut(array_menu_debug_settings,'v');
	                menu_add_item_menu_tooltip(array_menu_debug_settings,"Show which memory zones are changed");
	                menu_add_item_menu_ayuda(array_menu_debug_settings,"Show which memory zones are changed");
			//}
#endif

    menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_find,NULL,"~~Find byte");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'f');
    menu_add_item_menu_tooltip(array_menu_debug_settings,"Find one byte on memory");
    menu_add_item_menu_ayuda(array_menu_debug_settings,"Find one byte on the 64 KB of mapped memory. It can be used to find POKEs");


		menu_add_item_menu(array_menu_debug_settings,"~~Poke",MENU_OPCION_NORMAL,menu_poke,NULL);
		menu_add_item_menu_shortcut(array_menu_debug_settings,'p');
		menu_add_item_menu_tooltip(array_menu_debug_settings,"Poke address manually or from .POK file");
		menu_add_item_menu_ayuda(array_menu_debug_settings,"Poke address for infinite lives, etc...");



		if (menu_cond_zx8081() ) {

			menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_lost_vsync,NULL,
				"Simulate lost VSYNC: %s",(simulate_lost_vsync.v==1 ? "On" : "Off"));
		}




		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_load_binary,NULL,"L~~oad binary block");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'o');

		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_save_binary,NULL,"S~~ave binary block");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'a');

		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_file_viewer,NULL,"Fi~~le viewer");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'l');


		if (!CPU_IS_MOTOROLA) {
		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_input_file_keyboard,NULL,"Input File Spoolin~~g");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'g');
                menu_add_item_menu_tooltip(array_menu_debug_settings,"Sends every character from a text file as keyboard presses");
                menu_add_item_menu_ayuda(array_menu_debug_settings,"Every character from a text file is sent as keyboard presses. Only Ascii characters, not UFT, Unicode or others. "
                                                                   "Symbols that require extended mode on Spectrum are not sent: [ ] (c) ~ \\ { }. These can be used "
                                                                   "as a delay.\n"
								"Note: symbol | means Shift+1 (Edit)");
		}


	/*	menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_registers_console,NULL,"Show r~~egisters in console: %s",(debug_registers==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_debug_settings,'e');

		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_debug_verbose,NULL,"Verbose ~~level: %d",verbose_level);
		menu_add_item_menu_shortcut(array_menu_debug_settings,'l');*/


		menu_add_item_menu_format(array_menu_debug_settings,MENU_OPCION_NORMAL,menu_run_mantransfer,menu_run_mantransfer_cond,"Run ~~mantransfer");
		menu_add_item_menu_shortcut(array_menu_debug_settings,'m');
		menu_add_item_menu_tooltip(array_menu_debug_settings,"Run mantransfer, which dumps ram memory contents (snapshot) to Spectrum Tape\n"
					"Only Spectrum 48k/16k models supported");
		menu_add_item_menu_ayuda(array_menu_debug_settings,"The difference between this option and the Save snapshot option is that "
					"this option runs a Spectrum machine program (mantransfev3.bin) which dumps the ram contents to tape, "
					"so you can use a .tap file to save it or even a real tape connected to line out of your soundcard.\n"
					"It uses a small amount of RAM on memory display and some bytes on the stack, so it is not a perfect "
					"routine and sometimes may fail.\n"
					"The source code can be found on mantransfev3.tap\n"
					"Note: Although mantransfe is a Spectrum program and it could run on a real spectrum or another emulator, "
					"the saving routine needs that ZEsarUX emulator tells which im mode the cpu is (IM1 or IM2), "
					"so, a saved program can be run on a real spectrum or another emulator, "
					"but the saving routine sees im1 by default, so, saving from a real spectrum or another emulator "
					"instead ZEsarUX will only work if the cpu is in IM1 mode (and not IM2)");


                menu_add_item_menu(array_menu_debug_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_debug_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_debug_settings);

                retorno_menu=menu_dibuja_menu(&debug_settings_opcion_seleccionada,&item_seleccionado,array_menu_debug_settings,"Debug" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
                        }
                }

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}


void menu_textspeech_filter_program(MENU_ITEM_PARAMETERS)
{

	char *filtros[2];

        filtros[0]="";
        filtros[1]=0;

/*
	char string_program[PATH_MAX];
	if (textspeech_filter_program!=NULL) {
		sprintf (string_program,"%s",textspeech_filter_program);
	}

	else {
		string_program[0]=0;
	}



	int ret=menu_filesel("Select Speech Program",filtros,string_program);


	if (ret==1) {
		sprintf (menu_buffer_textspeech_filter_program,"%s",string_program);
		textspeech_filter_program=menu_buffer_textspeech_filter_program;
		textspeech_filter_program_check_spaces();
	}

	else {
		textspeech_filter_program=NULL;
	}
*/

        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de speech program
        //si no hay directorio, vamos a rutas predefinidas
        if (textspeech_filter_program==NULL) menu_chdir_sharedfiles();
        else {
                char directorio[PATH_MAX];
                util_get_dir(textspeech_filter_program,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }

        int ret;

        ret=menu_filesel("Select Speech Program",filtros,menu_buffer_textspeech_filter_program);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);


        if (ret==1) {

                textspeech_filter_program=menu_buffer_textspeech_filter_program;
					textspeech_filter_program_check_spaces();
			}

		else {
			textspeech_filter_program=NULL;
        }



}

void menu_textspeech_stop_filter_program(MENU_ITEM_PARAMETERS)
{

        char *filtros[2];

        filtros[0]="";
        filtros[1]=0;


/*
        char string_program[PATH_MAX];
        if (textspeech_stop_filter_program!=NULL) {
                sprintf (string_program,"%s",textspeech_stop_filter_program);
        }

        else {
                string_program[0]=0;
        }



        int ret=menu_filesel("Select Stop Speech Prg",filtros,string_program);


        if (ret==1) {
                sprintf (menu_buffer_textspeech_stop_filter_program,"%s",string_program);
                textspeech_stop_filter_program=menu_buffer_textspeech_stop_filter_program;
                textspeech_stop_filter_program_check_spaces();
        }

        else {
                textspeech_stop_filter_program=NULL;
        }
*/

        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de speech program
        //si no hay directorio, vamos a rutas predefinidas
        if (textspeech_stop_filter_program==NULL) menu_chdir_sharedfiles();
        else {
                char directorio[PATH_MAX];
                util_get_dir(textspeech_stop_filter_program,directorio);
                //printf ("strlen directorio: %d directorio: %s\n",strlen(directorio),directorio);

                //cambiamos a ese directorio, siempre que no sea nulo
                if (directorio[0]!=0) {
                        debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
                        menu_filesel_chdir(directorio);
                }
        }

        int ret;

        ret=menu_filesel("Select Stop Speech Prg",filtros,menu_buffer_textspeech_stop_filter_program);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);


        if (ret==1) {

                textspeech_stop_filter_program=menu_buffer_textspeech_stop_filter_program;
                                        textspeech_stop_filter_program_check_spaces();
                        }

                else {
                        textspeech_stop_filter_program=NULL;
        }




}

void menu_textspeech_filter_timeout(MENU_ITEM_PARAMETERS)
{

       int valor;

        char string_value[3];

        sprintf (string_value,"%d",textspeech_timeout_no_enter);


        menu_ventana_scanf("Timeout (0=never)",string_value,3);

        valor=parse_string_to_number(string_value);

	if (valor<0) debug_printf (VERBOSE_ERR,"Timeout must be 0 minimum");

	else textspeech_timeout_no_enter=valor;


}

void menu_textspeech_program_wait(MENU_ITEM_PARAMETERS)
{
	textspeech_filter_program_wait.v ^=1;
}

void menu_textspeech_send_menu(MENU_ITEM_PARAMETERS)
{
        textspeech_also_send_menu.v ^=1;
}

void menu_textspeech(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_textspeech;
        menu_item item_seleccionado;
        int retorno_menu;

        do {



                char string_filterprogram_shown[14];
		char string_stop_filterprogram_shown[14];

		if (textspeech_filter_program!=NULL) {
	                menu_tape_settings_trunc_name(textspeech_filter_program,string_filterprogram_shown,14);
		}

		else {
		sprintf (string_filterprogram_shown,"None");
		}



                if (textspeech_stop_filter_program!=NULL) {
                        menu_tape_settings_trunc_name(textspeech_stop_filter_program,string_stop_filterprogram_shown,14);
                }

                else {
                sprintf (string_stop_filterprogram_shown,"None");
                }


                        menu_add_item_menu_inicial_format(&array_menu_textspeech,MENU_OPCION_NORMAL,menu_textspeech_filter_program,NULL,"~~Speech program: %s",string_filterprogram_shown);
			menu_add_item_menu_shortcut(array_menu_textspeech,'s');
        	        menu_add_item_menu_tooltip(array_menu_textspeech,"Specify which program to send generated text");
        	        menu_add_item_menu_ayuda(array_menu_textspeech,"Specify which program to send generated text. Text is send to the program "
						"to its standard input on Unix versions (Linux, Mac, etc) or sent as the first parameter on "
						"Windows (MINGW) version\n"
						"Pressing a key on the menu (or ESC with menu closed) forces the following queded speech entries to flush, and running the "
						"Stop Program to stop the current speech script.\n");


			if (textspeech_filter_program!=NULL) {

				menu_add_item_menu_format(array_menu_textspeech,MENU_OPCION_NORMAL,menu_textspeech_stop_filter_program,NULL,"Stop program: %s",string_stop_filterprogram_shown);

        	                menu_add_item_menu_tooltip(array_menu_textspeech,"Specify a path to a program or script in charge of stopping the running speech program");
                	        menu_add_item_menu_ayuda(array_menu_textspeech,"Specify a path to a program or script in charge of stopping the running speech program. If not specified, the current speech script can't be stopped");




				menu_add_item_menu_format(array_menu_textspeech,MENU_OPCION_NORMAL,menu_textspeech_filter_timeout,NULL,"~~Timeout no enter: %d",textspeech_timeout_no_enter);
				menu_add_item_menu_shortcut(array_menu_textspeech,'t');
				menu_add_item_menu_tooltip(array_menu_textspeech,"After some seconds the text will be sent to the Speech program when no "
						"new line is sent");
				menu_add_item_menu_ayuda(array_menu_textspeech,"After some seconds the text will be sent to the Speech program when no "
						"new line is sent. 0=never");



				menu_add_item_menu_format(array_menu_textspeech,MENU_OPCION_NORMAL,menu_textspeech_program_wait,NULL,"~~Wait program to exit: %s",(textspeech_filter_program_wait.v ? "Yes" : "No") );
				menu_add_item_menu_shortcut(array_menu_textspeech,'w');
                	        menu_add_item_menu_tooltip(array_menu_textspeech,"Wait and pause the emulator until the Speech program returns");
                        	menu_add_item_menu_ayuda(array_menu_textspeech,"Wait and pause the emulator until the Speech program returns");


				menu_add_item_menu_format(array_menu_textspeech,MENU_OPCION_NORMAL,menu_textspeech_send_menu,NULL,"Also send ~~menu: %s",(textspeech_also_send_menu.v ? "Yes" : "No" ));
				menu_add_item_menu_shortcut(array_menu_textspeech,'m');
				menu_add_item_menu_tooltip(array_menu_textspeech,"Also send text menu entries to Speech program");
				menu_add_item_menu_ayuda(array_menu_textspeech,"Also send text menu entries to Speech program");

			}


          menu_add_item_menu(array_menu_textspeech,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_textspeech,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_textspeech);

                retorno_menu=menu_dibuja_menu(&textspeech_opcion_seleccionada,&item_seleccionado,array_menu_textspeech,"Text to Speech" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}


void menu_interface_fullscreen(MENU_ITEM_PARAMETERS)
{

	if (ventana_fullscreen==0) {
		scr_set_fullscreen();
	}

	else {
		scr_reset_fullscreen();
	}

	menu_init_footer();

}

void menu_interface_rgb_inverse_common(void)
{
	modificado_border.v=1;
	screen_init_colour_table();
	menu_init_footer();
}


void menu_interface_red(MENU_ITEM_PARAMETERS)
{
	screen_gray_mode ^= 4;
	menu_interface_rgb_inverse_common();
}

void menu_interface_green(MENU_ITEM_PARAMETERS)
{
        screen_gray_mode ^= 2;
	menu_interface_rgb_inverse_common();
}

void menu_interface_blue(MENU_ITEM_PARAMETERS)
{
        screen_gray_mode ^= 1;
	menu_interface_rgb_inverse_common();
}

void menu_interface_inverse_video(MENU_ITEM_PARAMETERS)
{
        inverse_video.v ^= 1;
	menu_interface_rgb_inverse_common();
}

void menu_interface_border(MENU_ITEM_PARAMETERS)
{
        debug_printf(VERBOSE_INFO,"End Screen");
        scr_end_pantalla();

	if (border_enabled.v) disable_border();
	else enable_border();

        scr_init_pantalla();
        debug_printf(VERBOSE_INFO,"Creating Screen");

	menu_init_footer();
}



int menu_interface_border_cond(void)
{
	if (ventana_fullscreen) return 0;
	return 1;
}

int menu_interface_zoom_cond(void)
{
        if (ventana_fullscreen) return 0;
        return 1;
}


int num_menu_audio_driver;
int num_previo_menu_audio_driver;


//Determina cual es el audio driver actual
void menu_change_audio_driver_get(void)
{
        int i;
        for (i=0;i<num_audio_driver_array;i++) {
		//printf ("actual: %s buscado: %s indice: %d\n",audio_driver_name,audio_driver_array[i].driver_name,i);
                if (!strcmp(audio_driver_name,audio_driver_array[i].driver_name)) {
                        num_menu_audio_driver=i;
                        num_previo_menu_audio_driver=i;
			return;
                }

        }

}


void menu_change_audio_driver_change(MENU_ITEM_PARAMETERS)
{
        num_menu_audio_driver++;
        if (num_menu_audio_driver==num_audio_driver_array) num_menu_audio_driver=0;
}

void menu_change_audio_driver_apply(MENU_ITEM_PARAMETERS)
{

	audio_end();

        int (*funcion_init) ();
        int (*funcion_set) ();

        funcion_init=audio_driver_array[num_menu_audio_driver].funcion_init;
        funcion_set=audio_driver_array[num_menu_audio_driver].funcion_set;
                if ( (funcion_init()) ==0) {
                        funcion_set();
			menu_generic_message("Apply Driver","OK. Driver applied");
			salir_todos_menus=1;
                }

                else {
                        debug_printf(VERBOSE_ERR,"Can not set audio driver. Restoring to previous driver %s",audio_driver_name);
			menu_change_audio_driver_get();

                        //Restaurar audio driver
                        funcion_init=audio_driver_array[num_previo_menu_audio_driver].funcion_init;
                        funcion_set=audio_driver_array[num_previo_menu_audio_driver].funcion_set;

                        funcion_init();
                        funcion_set();
                }



}


void menu_change_audio_driver(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_change_audio_driver;
        menu_item item_seleccionado;
        int retorno_menu;

       	menu_change_audio_driver_get();

        do {

                menu_add_item_menu_inicial_format(&array_menu_change_audio_driver,MENU_OPCION_NORMAL,menu_change_audio_driver_change,NULL,"Audio Driver: %s",audio_driver_array[num_menu_audio_driver].driver_name );

                menu_add_item_menu_format(array_menu_change_audio_driver,MENU_OPCION_NORMAL,menu_change_audio_driver_apply,NULL,"Apply Driver" );

                menu_add_item_menu(array_menu_change_audio_driver,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_change_audio_driver,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_change_audio_driver);

                retorno_menu=menu_dibuja_menu(&change_audio_driver_opcion_seleccionada,&item_seleccionado,array_menu_change_audio_driver,"Change Audio Driver" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}


void menu_tool_path(char *tool_path,char *name)
{

        char *filtros[2];

        filtros[0]="";
        filtros[1]=0;

	char buffer_tool_path[PATH_MAX];

        //guardamos directorio actual
        char directorio_actual[PATH_MAX];
        getcwd(directorio_actual,PATH_MAX);

        //Obtenemos directorio de la tool

        char directorio[PATH_MAX];
        util_get_dir(tool_path,directorio);
        debug_printf (VERBOSE_INFO,"Last directory: %s",directorio);

        //cambiamos a ese directorio, siempre que no sea nulo
        if (directorio[0]!=0) {
		debug_printf (VERBOSE_INFO,"Changing to last directory: %s",directorio);
		menu_filesel_chdir(directorio);
        }


        int ret;

        char ventana_titulo[40];
	sprintf (ventana_titulo,"Select %s tool",name);

        ret=menu_filesel(ventana_titulo,filtros,buffer_tool_path);
        //volvemos a directorio inicial
        menu_filesel_chdir(directorio_actual);


        if (ret==1) {
		sprintf (tool_path,"%s",buffer_tool_path);
        }

}


void menu_external_tool_sox(MENU_ITEM_PARAMETERS)
{
	menu_tool_path(external_tool_sox,"sox");
}

void menu_external_tool_unzip(MENU_ITEM_PARAMETERS)
{
	menu_tool_path(external_tool_unzip,"unzip");
}

void menu_external_tool_gunzip(MENU_ITEM_PARAMETERS)
{
	menu_tool_path(external_tool_gunzip,"gunzip");
}

void menu_external_tool_tar(MENU_ITEM_PARAMETERS)
{
	menu_tool_path(external_tool_tar,"tar");
}

void menu_external_tool_unrar(MENU_ITEM_PARAMETERS)
{
	menu_tool_path(external_tool_unrar,"unrar");
}



void menu_external_tools_config(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_external_tools_config;
        menu_item item_seleccionado;
        int retorno_menu;


	char string_sox[20];
	char string_unzip[20];
	char string_gunzip[20];
	char string_tar[20];
	char string_unrar[20];


        do {

		menu_tape_settings_trunc_name(external_tool_sox,string_sox,20);
		menu_tape_settings_trunc_name(external_tool_unzip,string_unzip,20);
		menu_tape_settings_trunc_name(external_tool_gunzip,string_gunzip,20);
		menu_tape_settings_trunc_name(external_tool_tar,string_tar,20);
		menu_tape_settings_trunc_name(external_tool_unrar,string_unrar,20);

                menu_add_item_menu_inicial_format(&array_menu_external_tools_config,MENU_OPCION_NORMAL,menu_external_tool_sox,NULL,"~~Sox: %s",string_sox);
		menu_add_item_menu_shortcut(array_menu_external_tools_config,'s');
                menu_add_item_menu_tooltip(array_menu_external_tools_config,"Change Sox Path");
                menu_add_item_menu_ayuda(array_menu_external_tools_config,"Change Sox Path. Path can not include spaces");


                menu_add_item_menu_format(array_menu_external_tools_config,MENU_OPCION_NORMAL,menu_external_tool_unzip,NULL,"Un~~zip: %s",string_unzip);
		menu_add_item_menu_shortcut(array_menu_external_tools_config,'z');
                menu_add_item_menu_tooltip(array_menu_external_tools_config,"Change Unzip Path");
                menu_add_item_menu_ayuda(array_menu_external_tools_config,"Change Unzip Path. Path can not include spaces");



                menu_add_item_menu_format(array_menu_external_tools_config,MENU_OPCION_NORMAL,menu_external_tool_gunzip,NULL,"~~Gunzip: %s",string_gunzip);
		menu_add_item_menu_shortcut(array_menu_external_tools_config,'g');
                menu_add_item_menu_tooltip(array_menu_external_tools_config,"Change Gunzip Path");
                menu_add_item_menu_ayuda(array_menu_external_tools_config,"Change Gunzip Path. Path can not include spaces");



                menu_add_item_menu_format(array_menu_external_tools_config,MENU_OPCION_NORMAL,menu_external_tool_tar,NULL,"~~Tar: %s",string_tar);
		menu_add_item_menu_shortcut(array_menu_external_tools_config,'t');
                menu_add_item_menu_tooltip(array_menu_external_tools_config,"Change Tar Path");
                menu_add_item_menu_ayuda(array_menu_external_tools_config,"Change Tar Path. Path can not include spaces");


                menu_add_item_menu_format(array_menu_external_tools_config,MENU_OPCION_NORMAL,menu_external_tool_unrar,NULL,"Un~~rar: %s",string_unrar);
		menu_add_item_menu_shortcut(array_menu_external_tools_config,'r');
                menu_add_item_menu_tooltip(array_menu_external_tools_config,"Change Unrar Path");
                menu_add_item_menu_ayuda(array_menu_external_tools_config,"Change Unrar Path. Path can not include spaces");



                menu_add_item_menu(array_menu_external_tools_config,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_external_tools_config,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_external_tools_config);

                retorno_menu=menu_dibuja_menu(&external_tools_config_opcion_seleccionada,&item_seleccionado,array_menu_external_tools_config,"External tools paths" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}




int num_menu_scr_driver;
int num_previo_menu_scr_driver;


//Determina cual es el video driver actual
void menu_change_video_driver_get(void)
{
	int i;
        for (i=0;i<num_scr_driver_array;i++) {
		if (!strcmp(scr_driver_name,scr_driver_array[i].driver_name)) {
			num_menu_scr_driver=i;
			num_previo_menu_scr_driver=i;
			return;
		}

        }

}

void menu_change_video_driver_change(MENU_ITEM_PARAMETERS)
{
	num_menu_scr_driver++;
	if (num_menu_scr_driver==num_scr_driver_array) num_menu_scr_driver=0;
}

void menu_change_video_driver_apply(MENU_ITEM_PARAMETERS)
{

	//Si driver null, avisar
	if (!strcmp(scr_driver_array[num_menu_scr_driver].driver_name,"null")) {
		if (menu_confirm_yesno_texto("Driver is null","Sure?")==0) return;
	}

	//Si driver es cocoa, no dejar cambiar a cocoa
	if (!strcmp(scr_driver_array[num_menu_scr_driver].driver_name,"cocoa")) {
		debug_printf(VERBOSE_ERR,"You can not set cocoa driver from menu. "
				"You must start emulator with cocoa driver (with --vo cocoa or without any --vo setting)");
		return;
	}


        scr_end_pantalla();

	screen_reset_scr_driver_params();

        int (*funcion_init) ();
        int (*funcion_set) ();

        funcion_init=scr_driver_array[num_menu_scr_driver].funcion_init;
        funcion_set=scr_driver_array[num_menu_scr_driver].funcion_set;
                if ( (funcion_init()) ==0) {
                        funcion_set();
			menu_generic_message("Apply Driver","OK. Driver applied");
	                //Y salimos de todos los menus
	                salir_todos_menus=1;

                }

		else {
			debug_printf(VERBOSE_ERR,"Can not set video driver. Restoring to previous driver %s",scr_driver_name);
			menu_change_video_driver_get();

			//Restaurar video driver
			screen_reset_scr_driver_params();
		        funcion_init=scr_driver_array[num_previo_menu_scr_driver].funcion_init;
		        funcion_set=scr_driver_array[num_previo_menu_scr_driver].funcion_set;

			funcion_init();
			funcion_set();
		}

        //scr_init_pantalla();

	modificado_border.v=1;
	menu_init_footer();


	if (!strcmp(scr_driver_name,"aa")) {
		menu_generic_message_format("Warning","Remember that on aa video driver, menu is opened with %s",openmenu_key_message);
	}

	//TODO
	//Para aalib, tanto aqui como en cambio de border, no se ve el cursor del menu .... en cuanto se redimensiona la ventana, se arregla


}

void menu_change_video_driver(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_change_video_driver;
        menu_item item_seleccionado;
        int retorno_menu;

	menu_change_video_driver_get();

        do {

                menu_add_item_menu_inicial_format(&array_menu_change_video_driver,MENU_OPCION_NORMAL,menu_change_video_driver_change,NULL,"Video Driver: %s",scr_driver_array[num_menu_scr_driver].driver_name );

                menu_add_item_menu_format(array_menu_change_video_driver,MENU_OPCION_NORMAL,menu_change_video_driver_apply,NULL,"Apply Driver" );

                menu_add_item_menu(array_menu_change_video_driver,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_change_video_driver,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_change_video_driver);

                retorno_menu=menu_dibuja_menu(&change_video_driver_opcion_seleccionada,&item_seleccionado,array_menu_change_video_driver,"Change Video Driver" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}

int menu_change_video_driver_cond(void)
{
	if (ventana_fullscreen) return 0;
	else return 1;
}


/*
void menu_interface_second_layer(MENU_ITEM_PARAMETERS)
{

        debug_printf(VERBOSE_INFO,"End Screen");
        scr_end_pantalla();


	if (menu_second_layer==0) enable_second_layer();
	else {
		disable_second_layer();
		cls_menu_overlay();
	}


	modificado_border.v=1;
        debug_printf(VERBOSE_INFO,"Creating Screen");
        scr_init_pantalla();

}
*/

void menu_interface_footer(MENU_ITEM_PARAMETERS)
{

        debug_printf(VERBOSE_INFO,"End Screen");
        scr_end_pantalla();


        if (menu_footer==0) {
		enable_footer();
	}

        else {
                disable_footer();
                //cls_menu_overlay();
        }


        modificado_border.v=1;
        debug_printf(VERBOSE_INFO,"Creating Screen");
        scr_init_pantalla();


	if (menu_footer) menu_init_footer();

}


void menu_interface_frameskip(MENU_ITEM_PARAMETERS)
{

        menu_hardware_advanced_input_value(0,49,"Frameskip",&frameskip);
}

void menu_interface_show_splash_texts(MENU_ITEM_PARAMETERS)
{
	screen_show_splash_texts.v ^=1;
}

void menu_interface_tooltip(MENU_ITEM_PARAMETERS)
{
	tooltip_enabled.v ^=1;
	menu_tooltip_counter=0;
}

void menu_interface_force_atajo(MENU_ITEM_PARAMETERS)
{
        menu_force_writing_inverse_color.v ^=1;
}


void menu_interface_zoom(MENU_ITEM_PARAMETERS)
{
        char string_zoom[2];
	int temp_zoom;


	//comprobaciones previas para no petar el sprintf
	if (zoom_x>9 || zoom_x<1) zoom_x=1;

        sprintf (string_zoom,"%d",zoom_x);


        menu_ventana_scanf("Window Zoom",string_zoom,2);

        temp_zoom=parse_string_to_number(string_zoom);
	if (temp_zoom>9 || temp_zoom<1) {
		debug_printf (VERBOSE_ERR,"Invalid zoom value %s",string_zoom);
		return;
	}

	scr_end_pantalla();

	zoom_x=zoom_y=temp_zoom;
	modificado_border.v=1;

	scr_init_pantalla();
	set_putpixel_zoom();


	menu_init_footer();

}


void menu_interface_change_gui_style(MENU_ITEM_PARAMETERS)
{
	estilo_gui_activo++;
	if (estilo_gui_activo==ESTILOS_GUI) estilo_gui_activo=0;
	set_charset();
}

void menu_interface_multitask(MENU_ITEM_PARAMETERS)
{

	menu_multitarea=menu_multitarea^1;
	if (menu_multitarea==0) {
		//audio_thread_finish();
		audio_playing.v=0;
	}
	timer_reset();

}

void menu_interface_autoframeskip(MENU_ITEM_PARAMETERS)
{
	autoframeskip.v ^=1;
}

void menu_interface_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_interface_settings;
        menu_item item_seleccionado;
        int retorno_menu;
        do {
                menu_add_item_menu_inicial_format(&array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_fullscreen,NULL,"~~Full Screen: %s",(ventana_fullscreen ? "On" : "Off") );
		menu_add_item_menu_shortcut(array_menu_interface_settings,'f');

		if (!MACHINE_IS_Z88) {
	                menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_border,menu_interface_border_cond,"~~Border enabled: %s", (border_enabled.v==1 ? "On" : "Off") );
			menu_add_item_menu_shortcut(array_menu_interface_settings,'b');
		}


                if (si_complete_video_driver() ) {
                        menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_zoom,menu_interface_zoom_cond,"Window Size ~~Zoom: %d",zoom_x);
			menu_add_item_menu_shortcut(array_menu_interface_settings,'z');
                        menu_add_item_menu_tooltip(array_menu_interface_settings,"Change Window Zoom");
                        menu_add_item_menu_ayuda(array_menu_interface_settings,"Changes Window Size Zoom (width and height)");
                }



		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_multitask,NULL,"M~~ultitask menu: %s", (menu_multitarea==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_interface_settings,'u');
		menu_add_item_menu_tooltip(array_menu_interface_settings,"Enable menu with multitask");
		menu_add_item_menu_ayuda(array_menu_interface_settings,"Setting multitask on makes the emulation does not stop when the menu is active");

		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_footer,NULL,"Window F~~ooter: %s",(menu_footer ? "Yes" : "No") );
		menu_add_item_menu_shortcut(array_menu_interface_settings,'o');
		menu_add_item_menu_tooltip(array_menu_interface_settings,"Show on footer some machine information");
		menu_add_item_menu_ayuda(array_menu_interface_settings,"Show on footer some machine information, like tape loading");


		//Teclado en pantalla
		if (MACHINE_IS_SPECTRUM || MACHINE_IS_ZX8081) {
			menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_onscreen_keyboard,NULL,"~~On Screen Keyboard");
			menu_add_item_menu_shortcut(array_menu_interface_settings,'o');
			menu_add_item_menu_tooltip(array_menu_interface_settings,"Open on screen keyboard");
			menu_add_item_menu_ayuda(array_menu_interface_settings,"You can also get this pressing F8 with menu closed, only for Spectrum and ZX80/81 machines");
		}


		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_show_splash_texts,NULL,"Show splash texts: %s",(screen_show_splash_texts.v ? "Yes" : "No") );
		menu_add_item_menu_tooltip(array_menu_interface_settings,"Show on display some splash texts, like display mode change");
		menu_add_item_menu_ayuda(array_menu_interface_settings,"Show on display some splash texts, like display mode change");


		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_tooltip,NULL,"Tooltips: %s",(tooltip_enabled.v ? "Enabled" : "Disabled") );
		menu_add_item_menu_tooltip(array_menu_interface_settings,"Enable or disable tooltips");
		menu_add_item_menu_ayuda(array_menu_interface_settings,"Enable or disable tooltips");

		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_force_atajo,NULL,"Force visible hotkeys:  %s",(menu_force_writing_inverse_color.v ? "Yes" : "No") );
                menu_add_item_menu_tooltip(array_menu_interface_settings,"Force always show hotkeys");
                menu_add_item_menu_ayuda(array_menu_interface_settings,"Force always show hotkeys. By default it will only be shown after a timeout or wrong key pressed");


		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_frameskip,NULL,"F~~rameskip: %d",frameskip);
		menu_add_item_menu_shortcut(array_menu_interface_settings,'r');
			menu_add_item_menu_tooltip(array_menu_interface_settings,"Sets the number of frames to skip every time the screen needs to be refreshed");
			menu_add_item_menu_ayuda(array_menu_interface_settings,"Sets the number of frames to skip every time the screen needs to be refreshed");


		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_autoframeskip,NULL,"Auto Frameskip: %s",
				(autoframeskip.v ? "Yes" : "No"));
			menu_add_item_menu_tooltip(array_menu_interface_settings,"Let ZEsarUX decide when to skip frames");
			menu_add_item_menu_ayuda(array_menu_interface_settings,"ZEsarUX skips frames when the host cpu use is too high. Then skiping frames the cpu use decreases");

			menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_chardetection_settings,NULL,"~~Print char traps");
			menu_add_item_menu_shortcut(array_menu_interface_settings,'p');
			menu_add_item_menu_tooltip(array_menu_interface_settings,"Settings on capture print character routines");
			menu_add_item_menu_ayuda(array_menu_interface_settings,"Settings on capture print character routines");


			menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_textspeech,NULL,"~~Text to speech");
			menu_add_item_menu_shortcut(array_menu_interface_settings,'t');
			menu_add_item_menu_tooltip(array_menu_interface_settings,"Specify a script or program to send all text generated, "
						"from Spectrum display or emulator menu, "
						"usually used on text to speech");
			menu_add_item_menu_ayuda(array_menu_interface_settings,"Specify a script or program to send all text generated, "
						"from Spectrum display or emulator menu, "
						"usually used on text to speech. "
						"When running the script: \n"
						"ESC means abort next executions on queue.\n"
						"Enter means run pending execution.\n");

		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_red,NULL,"Red display: %s",(screen_gray_mode & 4 ? "On" : "Off") );
		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_green,NULL,"Green display: %s",(screen_gray_mode & 2 ? "On" : "Off") );
		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_blue,NULL,"Blue display: %s",(screen_gray_mode & 1 ? "On" : "Off") );

		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_inverse_video,NULL,"Inverse video: %s",(inverse_video.v==1 ? "On" : "Off") );
		menu_add_item_menu_tooltip(array_menu_interface_settings,"Inverse Color Palette");
		menu_add_item_menu_ayuda(array_menu_interface_settings,"Inverses all the colours used on the emulator, including menu");


		//Con driver cocoa, no permitimos cambiar a otro driver
		if (strcmp(scr_driver_name,"cocoa")) {
			menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_change_video_driver,menu_change_video_driver_cond,"Change Video Driver");
		}

		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_external_tools_config,NULL,"External tools paths");


		menu_add_item_menu_format(array_menu_interface_settings,MENU_OPCION_NORMAL,menu_interface_change_gui_style,NULL,"GUI ~~style: %s",
						definiciones_estilos_gui[estilo_gui_activo].nombre_estilo);
		menu_add_item_menu_shortcut(array_menu_interface_settings,'s');
		menu_add_item_menu_tooltip(array_menu_interface_settings,"Change GUI Style");
                menu_add_item_menu_ayuda(array_menu_interface_settings,"You can switch between:\n"
					"ZEsarUX: default style\n"
					"ZXSpectr: my first old emulator, that worked on MS-DOS and Windows. Celebrate its 20th anniversay with this style! :) \n"
					"ZX80/81: ZX80&81 style\n"
					"CPC: Amstrad CPC style\n"
					);



                menu_add_item_menu(array_menu_interface_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_interface_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_interface_settings);

                retorno_menu=menu_dibuja_menu(&interface_settings_opcion_seleccionada,&item_seleccionado,array_menu_interface_settings,"GUI Settings" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}



void menu_chardetection_settings_trap_rst16(MENU_ITEM_PARAMETERS)
{
        chardetect_printchar_enabled.v ^=1;
}



void menu_chardetection_settings_second_trap(MENU_ITEM_PARAMETERS)
{

        char string_dir[6];

        int dir;


        sprintf (string_dir,"%d",chardetect_second_trap_char_dir);

        menu_ventana_scanf("Address (0=none)",string_dir,6);

        dir=parse_string_to_number(string_dir);

        if (dir<0 || dir>65535) {
                debug_printf (VERBOSE_ERR,"Invalid address %d",dir);
                return;
        }

        chardetect_second_trap_char_dir=dir;

}

void menu_chardetection_settings_third_trap(MENU_ITEM_PARAMETERS)
{

        char string_dir[6];

        int dir;


        sprintf (string_dir,"%d",chardetect_third_trap_char_dir);

        menu_ventana_scanf("Address (0=none)",string_dir,6);

        dir=parse_string_to_number(string_dir);

        if (dir<0 || dir>65535) {
                debug_printf (VERBOSE_ERR,"Invalid address %d",dir);
                return;
        }

        chardetect_third_trap_char_dir=dir;

}

void menu_chardetection_settings_stdout_trap_detection(MENU_ITEM_PARAMETERS)
{


        trap_char_detection_routine_number++;
        if (trap_char_detection_routine_number==TRAP_CHAR_DETECTION_ROUTINES_TOTAL) trap_char_detection_routine_number=0;

        chardetect_init_trap_detection_routine();

}

void menu_chardetection_settings_chardetect_char_filter(MENU_ITEM_PARAMETERS)
{
        chardetect_char_filter++;
        if (chardetect_char_filter==CHAR_FILTER_TOTAL) chardetect_char_filter=0;
}

void menu_chardetection_settings_stdout_line_width(MENU_ITEM_PARAMETERS)
{

        char string_width[3];

        int width;


        sprintf (string_width,"%d",chardetect_line_width);

        menu_ventana_scanf("Line width (0=no limit)",string_width,3);

        width=parse_string_to_number(string_width);

        //if (width>999) {
        //        debug_printf (VERBOSE_ERR,"Invalid width %d",width);
        //        return;
        //}
        chardetect_line_width=width;

}

void menu_chardetection_settings_second_trap_sum32(MENU_ITEM_PARAMETERS)
{

        chardetect_second_trap_sum32.v ^=1;

        //y ponemos el contador al maximo para que no se cambie por si solo
        chardetect_second_trap_sum32_counter=MAX_STDOUT_SUM32_COUNTER;


}


void menu_chardetection_settings_second_trap_range_min(MENU_ITEM_PARAMETERS)
{

        char string_dir[6];

        int dir;


        sprintf (string_dir,"%d",chardetect_second_trap_detect_pc_min);

        menu_ventana_scanf("Address",string_dir,6);

        dir=parse_string_to_number(string_dir);

        if (dir<0 || dir>65535) {
                debug_printf (VERBOSE_ERR,"Invalid address %d",dir);
                return;
        }

        chardetect_second_trap_detect_pc_min=dir;

}

void menu_chardetection_settings_second_trap_range_max(MENU_ITEM_PARAMETERS)
{

        char string_dir[6];

        int dir;


        sprintf (string_dir,"%d",chardetect_second_trap_detect_pc_max);

        menu_ventana_scanf("Address",string_dir,6);

        dir=parse_string_to_number(string_dir);

        if (dir<0 || dir>65535) {
                debug_printf (VERBOSE_ERR,"Invalid address %d",dir);
                return;
        }

        chardetect_second_trap_detect_pc_max=dir;

}


void menu_chardetection_settings_stdout_line_witdh_space(MENU_ITEM_PARAMETERS)
{
        chardetect_line_width_wait_space.v ^=1;
}


void menu_chardetection_settings_enable(MENU_ITEM_PARAMETERS)
{
	chardetect_detect_char_enabled.v ^=1;
}



//menu chardetection settings
void menu_chardetection_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_chardetection_settings;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                        menu_add_item_menu_inicial_format(&array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_trap_rst16,NULL,"~~Trap print: %s", (chardetect_printchar_enabled.v==1 ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_chardetection_settings,'t');
                        menu_add_item_menu_tooltip(array_menu_chardetection_settings,"It enables the emulator to show the text sent to standard rom print call routines and non standard, generated from some games, specially text adventures");
                        menu_add_item_menu_ayuda(array_menu_chardetection_settings,"It enables the emulator to show the text sent to standard rom print call routines and generated from some games, specially text adventures. "
                                                "On Spectrum, ZX80, ZX81 machines, standard rom calls are RST 10H. On Z88, it traps OS_OUT and some other calls. Non standard calls are the ones indicated on Second and Third trap");


			if (chardetect_printchar_enabled.v) {


	                        menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_second_trap,NULL,"~~Second trap address: %d",chardetect_second_trap_char_dir);
				menu_add_item_menu_shortcut(array_menu_chardetection_settings,'s');
        	                menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Address of the second print routine");
                	        menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Address of the second print routine");

	                        menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_second_trap_sum32,NULL,"Second trap s~~um 32: %s",(chardetect_second_trap_sum32.v ? "Yes" : "No"));
				menu_add_item_menu_shortcut(array_menu_chardetection_settings,'u');
				menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Sums 32 to the ASCII value read");
				menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Sums 32 to the ASCII value read");


        	                menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_third_trap,NULL,"T~~hird trap address: %d",chardetect_third_trap_char_dir);
				menu_add_item_menu_shortcut(array_menu_chardetection_settings,'h');
                	        menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Address of the third print routine");
                        	menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Address of the third print routine");

                        menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_stdout_line_width,NULL,"Line ~~width: %d",chardetect_line_width);
			menu_add_item_menu_shortcut(array_menu_chardetection_settings,'w');
			menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Line width");
			menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Line width. Setting 0 means no limit, so "
						"even when a carriage return is received, the text will not be sent unless a Enter "
						"key is pressed or when timeout no enter no text to speech is reached\n");


                        menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_stdout_line_witdh_space,NULL,"Line width w~~ait space: %s",(chardetect_line_width_wait_space.v==1 ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_chardetection_settings,'a');
			menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Wait for a space before jumping to a new line");
			menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Wait for a space before jumping to a new line");


                        menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_chardetect_char_filter,NULL,"Char ~~filter: %s",chardetect_char_filter_names[chardetect_char_filter]);
			menu_add_item_menu_shortcut(array_menu_chardetection_settings,'f');
			menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Send characters to an internal filter");
			menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Send characters to an internal filter");


			}

                menu_add_item_menu(array_menu_chardetection_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);



			menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_enable,NULL,"Enable 2nd trap ~~detection: %s",(chardetect_detect_char_enabled.v ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_chardetection_settings,'d');
			menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Enable char detection method to guess Second Trap address");
			menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Enable char detection method to guess Second Trap address");





			if (chardetect_detect_char_enabled.v) {


	                        menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_stdout_trap_detection,NULL,"Detect ~~routine: %s",trap_char_detection_routines_texto[trap_char_detection_routine_number]);
				menu_add_item_menu_shortcut(array_menu_chardetection_settings,'r');
        			 menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Selects method for second trap character routine detection");
	                        menu_add_item_menu_ayuda(array_menu_chardetection_settings,"This function enables second trap character routine detection for programs "
                                                "that does not use RST16 calls to ROM for printing characters, on Spectrum models. "
                                                "It tries to guess where the printing "
                                                "routine is located and set Second Trap address when it finds it. This function has some pre-defined known "
                                                "detection call printing routines (for example AD Adventures) and one other totally automatic method, "
                                        	"which first tries to find automatically an aproximate range where the routine is, and then, "
						"it finds which routine is, trying all on this list. "
						"This automatic method "
                                                "makes writing operations a bit slower (only while running the detection routine)");


        	                if (trap_char_detection_routine_number!=TRAP_CHAR_DETECTION_ROUTINE_AUTOMATIC && trap_char_detection_routine_number!=TRAP_CHAR_DETECTION_ROUTINE_NONE)  {
                        	        menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_second_trap_range_min,NULL,"Detection routine mi~~n: %d",chardetect_second_trap_detect_pc_min);
					menu_add_item_menu_shortcut(array_menu_chardetection_settings,'n');
					menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Lower address limit to find character routine");
					menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Lower address limit to find character routine");


                	                menu_add_item_menu_format(array_menu_chardetection_settings,MENU_OPCION_NORMAL,menu_chardetection_settings_second_trap_range_max,NULL,"Detection routine ma~~x: %d",chardetect_second_trap_detect_pc_max);
					menu_add_item_menu_shortcut(array_menu_chardetection_settings,'x');
					menu_add_item_menu_tooltip(array_menu_chardetection_settings,"Higher address limit to find character routine");
					menu_add_item_menu_ayuda(array_menu_chardetection_settings,"Higher address limit to find character routine");
	                        }


			}






                menu_add_item_menu(array_menu_chardetection_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_chardetection_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_chardetection_settings);

                retorno_menu=menu_dibuja_menu(&chardetection_settings_opcion_seleccionada,&item_seleccionado,array_menu_chardetection_settings,"Print char traps" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}







int menu_display_settings_disp_zx8081_spectrum(void)
{

	//esto solo en spectrum y si el driver no es curses y si no hay rainbow
	if (!strcmp(scr_driver_name,"curses")) return 0;
	if (rainbow_enabled.v==1) return 0;

	return !menu_cond_zx8081();
}


void menu_display_arttext(MENU_ITEM_PARAMETERS)
{
	texto_artistico.v ^=1;
}

int menu_display_cursesstdout_cond(void)
{
	if (!strcmp(scr_driver_name,"curses")) return 1;
	if (!strcmp(scr_driver_name,"stdout")) return 1;
	return 0;
}


#ifdef COMPILE_AA
void menu_display_slowaa(MENU_ITEM_PARAMETERS)
{
	scraa_fast ^=1;
}
#else
void menu_display_slowaa(MENU_ITEM_PARAMETERS){}
#endif


int menu_display_curses_cond(void)
{
	if (!strcmp(scr_driver_name,"curses")) return 1;

	else return 0;
}

int menu_display_aa_cond(void)
{
        if (!strcmp(scr_driver_name,"aa")) return 1;

        else return 0;
}



void menu_display_emulate_zx8081_thres(MENU_ITEM_PARAMETERS)
{

        char string_thres[3];

        sprintf (string_thres,"%d",umbral_simulate_screen_zx8081);

        menu_ventana_scanf("Pixel Threshold",string_thres,3);

	umbral_simulate_screen_zx8081=parse_string_to_number(string_thres);
	if (umbral_simulate_screen_zx8081<1 || umbral_simulate_screen_zx8081>16) umbral_simulate_screen_zx8081=4;

}

void menu_display_arttext_thres(MENU_ITEM_PARAMETERS)
{

        char string_thres[3];

        sprintf (string_thres,"%d",umbral_arttext);

        menu_ventana_scanf("Pixel Threshold",string_thres,3);

        umbral_arttext=parse_string_to_number(string_thres);
        if (umbral_arttext<1 || umbral_arttext>16) umbral_arttext=4;

}

void menu_display_load_screen(MENU_ITEM_PARAMETERS)
{

char screen_load_file[PATH_MAX];

  char *filtros[2];

        filtros[0]="scr";
        filtros[1]=0;


        if (menu_filesel("Select Screen File",filtros,screen_load_file)==1) {
		load_screen(screen_load_file);
                //Y salimos de todos los menus
                salir_todos_menus=1;

        }

}

void menu_display_save_screen(MENU_ITEM_PARAMETERS)
{

char screen_save_file[PATH_MAX];

  char *filtros[2];

        filtros[0]="scr";
        filtros[1]=0;


        if (menu_filesel("Select Screen File",filtros,screen_save_file)==1) {

                //Ver si archivo existe y preguntar
                struct stat buf_stat;

                if (stat(screen_save_file, &buf_stat)==0) {

			if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) return;

                }


                save_screen(screen_save_file);
                //Y salimos de todos los menus
                salir_todos_menus=1;

        }

}

void menu_display_rainbow(MENU_ITEM_PARAMETERS)
{
	if (rainbow_enabled.v==0) enable_rainbow();
	else disable_rainbow();
//		rainbow_enabled.v=0;
//		modificado_border.v=1;
//	}
}

/*
void menu_display_shows_vsync_on_display(void)
{
	video_zx8081_shows_vsync_on_display.v ^=1;
}
*/


void menu_display_slow_adjust(MENU_ITEM_PARAMETERS)
{
	video_zx8081_lnctr_adjust.v ^=1;
}



void menu_display_estabilizador_imagen(MENU_ITEM_PARAMETERS)
{
	video_zx8081_estabilizador_imagen.v ^=1;
}

void menu_display_interlace(MENU_ITEM_PARAMETERS)
{
	if (video_interlaced_mode.v) disable_interlace();
	else enable_interlace();
}


void menu_display_interlace_scanlines(MENU_ITEM_PARAMETERS)
{
	if (video_interlaced_scanlines.v) disable_scanlines();
	else enable_scanlines();
}

void menu_display_gigascreen(MENU_ITEM_PARAMETERS)
{
        if (gigascreen_enabled.v) disable_gigascreen();
        else enable_gigascreen();
}

/*void menu_display_zx8081_wrx_first_column(void)
{
	wrx_mueve_primera_columna.v ^=1;
}
*/

int menu_cond_wrx(void)
{
	return wrx_present.v;
}

void menu_vofile_insert(MENU_ITEM_PARAMETERS)
{

        if (vofile_inserted.v==0) {
                init_vofile();
                //Si todo ha ido bien
                if (vofile_inserted.v) {
                        menu_generic_message_format("File information","%s\n%s\n\n%s",
												last_message_helper_aofile_vofile_file_format,last_message_helper_aofile_vofile_bytes_minute_video,last_message_helper_aofile_vofile_util);
                }

        }

        else if (vofile_inserted.v==1) {
                close_vofile();
        }

}


int menu_vofile_cond(void)
{
        if (vofilename!=NULL) return 1;
        else return 0;
}

void menu_vofile(MENU_ITEM_PARAMETERS)
{

        vofile_inserted.v=0;


        char *filtros[2];

        filtros[0]="rwv";
        filtros[1]=0;


        if (menu_filesel("Select Video File",filtros,vofilename_file)==1) {

                 //Ver si archivo existe y preguntar
                struct stat buf_stat;

                if (stat(vofilename_file, &buf_stat)==0) {

                        if (menu_confirm_yesno_texto("File exists","Overwrite?")==0) {
                                vofilename=NULL;
                                return;
                        }

                }



                vofilename=vofilename_file;
        }

        else {
                vofilename=NULL;
        }


}

void menu_vofile_fps(MENU_ITEM_PARAMETERS)
{
	if (vofile_fps==1) {
		vofile_fps=50;
		return;
	}

        if (vofile_fps==2) {
                vofile_fps=1;
                return;
        }


        if (vofile_fps==5) {
                vofile_fps=2;
                return;
        }

        if (vofile_fps==10) {
                vofile_fps=5;
                return;
        }

        if (vofile_fps==25) {
                vofile_fps=10;
                return;
        }


        if (vofile_fps==50) {
                vofile_fps=25;
                return;
        }

}

void menu_display_autodetect_rainbow(MENU_ITEM_PARAMETERS)
{
	autodetect_rainbow.v ^=1;
}

void menu_display_autodetect_wrx(MENU_ITEM_PARAMETERS)
{
        autodetect_wrx.v ^=1;
}


void menu_display_snow_effect(MENU_ITEM_PARAMETERS)
{
	snow_effect_enabled.v ^=1;
}

void menu_display_stdout_simpletext_automatic_redraw(MENU_ITEM_PARAMETERS)
{
	stdout_simpletext_automatic_redraw.v ^=1;
}



void menu_display_send_ansi(MENU_ITEM_PARAMETERS)
{
	screen_text_accept_ansi ^=1;
}












//En curses y stdout solo se permite para zx8081
int menu_cond_realvideo_curses_stdout_zx8081(void)
{
	if (menu_cond_stdout() || menu_cond_curses() ) {
		if (menu_cond_spectrum() ) return 0;
	}

	return 1;
}


int menu_display_rainbow_cond(void)
{
	//if (MACHINE_IS_Z88) return 0;
	return 1;
}


void menu_display_chroma81(MENU_ITEM_PARAMETERS)
{
        if (chroma81.v) disable_chroma81();
        else enable_chroma81();
}

void menu_display_ulaplus(MENU_ITEM_PARAMETERS)
{
        if (ulaplus_presente.v) disable_ulaplus();
        else enable_ulaplus();
}


void menu_display_autodetect_chroma81(MENU_ITEM_PARAMETERS)
{
	autodetect_chroma81.v ^=1;
}


void menu_display_spectra(MENU_ITEM_PARAMETERS)
{
	if (spectra_enabled.v) spectra_disable();
	else spectra_enable();
}

void menu_display_snow_effect_margin(MENU_ITEM_PARAMETERS)
{
	snow_effect_min_value++;
	if (snow_effect_min_value==8) snow_effect_min_value=1;
}

void menu_display_timex_video(MENU_ITEM_PARAMETERS)
{
	if (timex_video_emulation.v) disable_timex_video();
	else enable_timex_video();
}

void menu_display_minimo_vsync(MENU_ITEM_PARAMETERS)
{

        menu_hardware_advanced_input_value(100,999,"Minimum vsync lenght",&minimo_duracion_vsync);
}

void menu_display_timex_video_512192(MENU_ITEM_PARAMETERS)
{

	timex_mode_512192_real.v ^=1;
}

void menu_display_cpc_force_mode(MENU_ITEM_PARAMETERS)
{
	if (cpc_forzar_modo_video.v==0) {
		cpc_forzar_modo_video.v=1;
		cpc_forzar_modo_video_modo=0;
	}
	else {
		cpc_forzar_modo_video_modo++;
		if (cpc_forzar_modo_video_modo==4) {
			cpc_forzar_modo_video_modo=0;
			cpc_forzar_modo_video.v=0;
		}
	}
}

void menu_display_refresca_sin_colores(MENU_ITEM_PARAMETERS)
{
	scr_refresca_sin_colores.v ^=1;
	modificado_border.v=1;
}


void menu_display_inves_ula_bright_error(MENU_ITEM_PARAMETERS)
{
	inves_ula_bright_error.v ^=1;
}


//menu display settings
void menu_display_settings(MENU_ITEM_PARAMETERS)
{

	menu_item *array_menu_display_settings;
	menu_item item_seleccionado;
	int retorno_menu;
	do {

		//char string_aux[50],string_aux2[50],emulate_zx8081_disp[50],string_arttext[50],string_aaslow[50],emulate_zx8081_thres[50],string_arttext_threshold[50];
		//char buffer_string[50];

		if (menu_cond_spectrum()) {
			menu_add_item_menu_inicial(&array_menu_display_settings,"~~Load Screen",MENU_OPCION_NORMAL,menu_display_load_screen,menu_cond_spectrum);
			menu_add_item_menu_shortcut(array_menu_display_settings,'l');

			menu_add_item_menu(array_menu_display_settings,"~~Save Screen",MENU_OPCION_NORMAL,menu_display_save_screen,menu_cond_spectrum);
			menu_add_item_menu_shortcut(array_menu_display_settings,'s');

			menu_add_item_menu(array_menu_display_settings,"~~View Screen",MENU_OPCION_NORMAL,menu_view_screen,NULL);
			menu_add_item_menu_shortcut(array_menu_display_settings,'v');
		}

		else {
			menu_add_item_menu_inicial(&array_menu_display_settings,"~~View Screen",MENU_OPCION_NORMAL,menu_view_screen,NULL);
			menu_add_item_menu_shortcut(array_menu_display_settings,'v');
		}

    /*            char string_vofile_shown[10];
                menu_tape_settings_trunc_name(vofilename,string_vofile_shown,10);


	                menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_vofile,NULL,"Video out to file: %s",string_vofile_shown);
        	        menu_add_item_menu_tooltip(array_menu_display_settings,"Saves the video output to a file");
                	menu_add_item_menu_ayuda(array_menu_display_settings,"The generated file have raw format. You can see the file parameters "
					"on the console enabling verbose debug level to 2 minimum.\n"
					"Note: Gigascreen, Interlaced effects or menu windows are not saved to file."

					);

			if (menu_vofile_cond() ) {
				menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_vofile_fps,menu_vofile_cond,"FPS Video file: %d",50/vofile_fps);
	        	        menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_vofile_insert,menu_vofile_cond,"Video file inserted: %s",(vofile_inserted.v ? "Yes" : "No" ));
			}

			else {

                	  menu_add_item_menu(array_menu_display_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
			}



                if (!MACHINE_IS_Z88) {


                        menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_autodetect_rainbow,NULL,"Autodetect Real Video: %s",(autodetect_rainbow.v==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_display_settings,"Autodetect the need to enable Real Video");
                        menu_add_item_menu_ayuda(array_menu_display_settings,"This option detects whenever is needed to enable Real Video. "
                                        "On Spectrum, it detects the reading of idle bus or repeated border changes. "
                                        "On ZX80/81, it detects the I register on a non-normal value when executing video display"
                                        );
                }




			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_rainbow,menu_display_rainbow_cond,"~~Real Video: %s",(rainbow_enabled.v==1 ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_display_settings,'r');

			menu_add_item_menu_tooltip(array_menu_display_settings,"Enable Real Video. Enabling it makes display as a real machine");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Real Video makes display works as in the real machine. It uses a bit more CPU than disabling it.\n\n"
					"On Spectrum, display is drawn every scanline. "
					"It enables hi-res colour (rainbow) on the screen and on the border, Gigascreen, Interlaced, ULAplus, Spectra, Timex Video, snow effect, idle bus reading and some other advanced features. "
					"Also enables all the Inves effects.\n"
					"Disabling it, the screen is drawn once per frame (1/50) and the previous effects "
					"are not supported.\n\n"
					"On ZX80/ZX81, enables hi-res display and loading/saving stripes on the screen, and the screen is drawn every scanline.\n"
					"By disabling it, the screen is drawn once per frame, no hi-res display, and only text mode is supported.\n\n"
					"On Z88, display is drawn the same way as disabling it; it is only used when enabling Video out to file.\n\n"
					"Real Video can be enabled on all the video drivers, but on curses, stdout and simpletext (in Spectrum and Z88 machines), the display drawn is the same "
					"as on non-Real Video, but you can have idle bus support on these drivers. "
					"Curses, stdout and simpletext drivers on ZX80/81 machines do have Real Video display."
					);

		if (MACHINE_IS_CPC) {
				if (cpc_forzar_modo_video.v==0)
					menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_cpc_force_mode,NULL,"Force Video Mode: No");
				else
					menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_cpc_force_mode,NULL,"Force Video Mode: %d",
						cpc_forzar_modo_video_modo);
		}


		if (!MACHINE_IS_Z88) {


			if (menu_cond_realvideo() ) {
				menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_interlace,menu_cond_realvideo,"~~Interlaced mode: %s", (video_interlaced_mode.v==1 ? "On" : "Off"));
				menu_add_item_menu_shortcut(array_menu_display_settings,'i');
				menu_add_item_menu_tooltip(array_menu_display_settings,"Enable interlaced mode");
				menu_add_item_menu_ayuda(array_menu_display_settings,"Interlaced mode draws the screen like the machine on a real TV: "
					"Every odd frame, odd lines on TV are drawn; every even frame, even lines on TV are drawn. It can be used "
					"to emulate twice the vertical resolution of the machine (384) or simulate different colours. "
					"This effect is only emulated with vertical zoom multiple of two: 2,4,6... etc");


				if (video_interlaced_mode.v) {
					menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_interlace_scanlines,NULL,"S~~canlines: %s", (video_interlaced_scanlines.v==1 ? "On" : "Off"));
					menu_add_item_menu_shortcut(array_menu_display_settings,'c');
					menu_add_item_menu_tooltip(array_menu_display_settings,"Enable scanlines on interlaced mode");
					menu_add_item_menu_ayuda(array_menu_display_settings,"Scanlines draws odd lines a bit darker than even lines");
				}


				menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_gigascreen,NULL,"~~Gigascreen: %s",(gigascreen_enabled.v==1 ? "On" : "Off"));
				menu_add_item_menu_shortcut(array_menu_display_settings,'g');
				menu_add_item_menu_tooltip(array_menu_display_settings,"Enable gigascreen colours");
				menu_add_item_menu_ayuda(array_menu_display_settings,"Gigascreen enables more than 15 colours by combining pixels "
							"of even and odd frames. The total number of different colours is 102");




				if (menu_cond_spectrum()) {
					menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_snow_effect,NULL,"Snow effect support: %s", (snow_effect_enabled.v==1 ? "On" : "Off"));
					menu_add_item_menu_tooltip(array_menu_display_settings,"Enable snow effect on Spectrum");
					menu_add_item_menu_ayuda(array_menu_display_settings,"Snow effect is a bug on some Spectrum models "
						"(models except +2A and +3) that draws corrupted pixels when I register is pointed to "
						"slow RAM.");
						// Even on 48k models it resets the machine after some seconds drawing corrupted pixels");

					if (snow_effect_enabled.v==1) {
						menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_snow_effect_margin,NULL,"Snow effect threshold: %d",snow_effect_min_value);
					}
				}


				if (MACHINE_IS_INVES) {
					menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_inves_ula_bright_error,NULL,"Inves bright error: %s",(inves_ula_bright_error.v ? "yes" : "no"));
					menu_add_item_menu_tooltip(array_menu_display_settings,"Emulate Inves oddity when black colour and change from bright 0 to bright 1");
					menu_add_item_menu_ayuda(array_menu_display_settings,"Emulate Inves oddity when black colour and change from bright 0 to bright 1");

				}



			}
		}

		//para stdout
#ifdef COMPILE_STDOUT
		if (menu_cond_stdout() ) {
			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_stdout_simpletext_automatic_redraw,NULL,"Stdout automatic redraw: %s", (stdout_simpletext_automatic_redraw.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_display_settings,"It enables automatic display redraw");
			menu_add_item_menu_ayuda(array_menu_display_settings,"It enables automatic display redraw");


			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_send_ansi,NULL,"Send ANSI Control Sequence: %s",(screen_text_accept_ansi==1 ? "On" : "Off"));

		}

#endif



		if (menu_cond_zx8081_realvideo()) {

		//z80_bit video_zx8081_estabilizador_imagen;

			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_estabilizador_imagen,menu_cond_zx8081_realvideo,"Horizontal stabilization: %s", (video_zx8081_estabilizador_imagen.v==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_display_settings,"Horizontal image stabilization");
                        menu_add_item_menu_ayuda(array_menu_display_settings,"Horizontal image stabilization. Usually enabled.");


			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_slow_adjust,menu_cond_zx8081_realvideo,"~~LNCTR video adjust:  %s", (video_zx8081_lnctr_adjust.v==1 ? "On" : "Off"));
			//l repetida con load screen, pero como esa es de spectrum, no coinciden
			menu_add_item_menu_shortcut(array_menu_display_settings,'l');
			menu_add_item_menu_tooltip(array_menu_display_settings,"LNCTR video adjust");
			menu_add_item_menu_ayuda(array_menu_display_settings,"LNCTR video adjust change sprite offset when drawing video images. "
				"If you see your hi-res image is not displayed well, try changing it");




        	        menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_x_offset,menu_cond_zx8081_realvideo,"Video x_offset: %d",offset_zx8081_t_coordx);
			menu_add_item_menu_tooltip(array_menu_display_settings,"Video horizontal image offset");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Video horizontal image offset, usually you don't need to change this");


			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_minimo_vsync,menu_cond_zx8081_realvideo,"Video min. vsync lenght: %d",minimo_duracion_vsync);
			menu_add_item_menu_tooltip(array_menu_display_settings,"Video minimum vsync lenght in t-states");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Video minimum vsync lenght in t-states");


                        menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_autodetect_wrx,NULL,"Autodetect WRX: %s",(autodetect_wrx.v==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_display_settings,"Autodetect the need to enable WRX mode on ZX80/81");
                        menu_add_item_menu_ayuda(array_menu_display_settings,"This option detects whenever is needed to enable WRX. "
                                                "On ZX80/81, it detects the I register on a non-normal value when executing video display. "
						"In some cases, chr$128 and udg modes are detected incorrectly as WRX");


	                menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_zx8081_wrx,menu_cond_zx8081_realvideo,"~~WRX: %s", (wrx_present.v ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_display_settings,'w');
			menu_add_item_menu_tooltip(array_menu_display_settings,"Enables WRX hi-res mode");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Enables WRX hi-res mode");



		}

		else {

			if (menu_cond_zx8081() ) {
				menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_emulate_fast_zx8081,menu_cond_zx8081_no_realvideo,"ZX80/81 detect fast mode: %s", (video_fast_mode_emulation.v==1 ? "On" : "Off"));
				menu_add_item_menu_tooltip(array_menu_display_settings,"Detect fast mode and simulate it, on non-realvideo mode");
				menu_add_item_menu_ayuda(array_menu_display_settings,"Detect fast mode and simulate it, on non-realvideo mode");
			}

		}

		if (MACHINE_IS_ZX8081) {

			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_autodetect_chroma81,NULL,"Autodetect Chroma81: %s",(autodetect_chroma81.v ? "Yes" : "No"));
			menu_add_item_menu_tooltip(array_menu_display_settings,"Autodetect Chroma81");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Detects when Chroma81 video mode is needed and enable it");


			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_chroma81,NULL,"Chro~~ma81 support: %s",(chroma81.v ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_display_settings,'m');
			menu_add_item_menu_tooltip(array_menu_display_settings,"Enables Chroma81 colour video mode");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Enables Chroma81 colour video mode");

		}


		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_ulaplus,NULL,"ULA~~plus support: %s",(ulaplus_presente.v ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_display_settings,'p');
			menu_add_item_menu_tooltip(array_menu_display_settings,"Enables ULAplus support");
			menu_add_item_menu_ayuda(array_menu_display_settings,"The following ULAplus modes are supported:\n"
						"Mode 1: Standard 256x192 64 colours\n"
						"Mode 3: Linear mode 128x96, 16 colours per pixel (radastan mode)\n"
						"Mode 5: Linear mode 256x96, 16 colours per pixel (ZEsarUX mode 0)\n"
						"Mode 7: Linear mode 128x192, 16 colours per pixel (ZEsarUX mode 1)\n"
						"Mode 9: Linear mode 256x192, 16 colours per pixel (ZEsarUX mode 2)\n"
			);



			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_timex_video,NULL,"~~Timex video support: %s",(timex_video_emulation.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_display_settings,'t');
                        menu_add_item_menu_tooltip(array_menu_display_settings,"Enables Timex Video modes");
                        menu_add_item_menu_ayuda(array_menu_display_settings,"The following Timex Video modes are emulated:\n"
						"Mode 0: Video data at address 16384 and 8x8 color attributes at address 22528 (like on ordinary Spectrum)\n"
						"Mode 1: Video data at address 24576 and 8x8 color attributes at address 30720\n"
						"Mode 2: Multicolor mode: video data at address 16384 and 8x1 color attributes at address 24576\n"
						"Mode 6: Hi-res mode 512x192, monochrome.");

			if (timex_video_emulation.v) {
				menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_timex_video_512192,NULL,"Timex Real 512x192: %s",(timex_mode_512192_real.v ? "Yes" : "No"));
				menu_add_item_menu_tooltip(array_menu_display_settings,"Selects between real 512x192 or scaled 256x192");
				menu_add_item_menu_ayuda(array_menu_display_settings,"Real 512x192 does not support scanline effects (it draws the display at once). "
							"If not enabled real, it draws scaled 256x192 but does support scanline effects");
			}



			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_spectra,NULL,"Sp~~ectra support: %s",(spectra_enabled.v ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_display_settings,'e');
			menu_add_item_menu_tooltip(array_menu_display_settings,"Enables Spectra video modes");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Enables Spectra video modes. All video modes are fully emulated");


			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_spritechip,NULL,"~~ZGX Sprite Chip: %s",(spritechip_enabled.v ? "Yes" : "No") );
			menu_add_item_menu_shortcut(array_menu_display_settings,'z');
			menu_add_item_menu_tooltip(array_menu_display_settings,"Enables ZGX Sprite Chip");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Enables ZGX Sprite Chip");


		}




		if (MACHINE_IS_SPECTRUM && rainbow_enabled.v==0) {
	                menu_add_item_menu(array_menu_display_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);


			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_emulate_zx8081display_spec,menu_display_settings_disp_zx8081_spectrum,"ZX80/81 Display on Speccy: %s", (simulate_screen_zx8081.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_display_settings,"Simulates the resolution of ZX80/81 on the Spectrum");
			menu_add_item_menu_ayuda(array_menu_display_settings,"It makes the resolution of display on Spectrum like a ZX80/81, with no colour. "
					"This mode is not supported with real video enabled");



			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_emulate_zx8081_thres,menu_display_emulate_zx8081_cond,"Pixel threshold: %d",umbral_simulate_screen_zx8081);
			menu_add_item_menu_tooltip(array_menu_display_settings,"Pixel Threshold to draw black or white in a 4x4 rectangle, "
					   "when ZX80/81 Display on Speccy enabled");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Pixel Threshold to draw black or white in a 4x4 rectangle, "
					   "when ZX80/81 Display on Speccy enabled");


			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_refresca_sin_colores,NULL,"Colours enabled: %s",(scr_refresca_sin_colores.v ? "No" : "Yes"));
			menu_add_item_menu_tooltip(array_menu_display_settings,"Disables colours for Spectrum display");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Disables colours for Spectrum display");

		}


		if (menu_display_cursesstdout_cond() ) {
	                //solo en caso de curses o stdout
			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_arttext,menu_display_cursesstdout_cond,"Text artistic emulation: %s", (texto_artistico.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_display_settings,"Write different artistic characters for unknown 4x4 rectangles, "
					"on stdout and curses drivers");

			menu_add_item_menu_ayuda(array_menu_display_settings,"Write different artistic characters for unknown 4x4 rectangles, "
					"on curses, stdout and simpletext drivers. "
					"If disabled, unknown characters are written with ?");


			menu_add_item_menu_format(array_menu_display_settings,MENU_OPCION_NORMAL,menu_display_arttext_thres,menu_display_arttext_cond,"Pixel threshold: %d",umbral_arttext);
			menu_add_item_menu_tooltip(array_menu_display_settings,"Pixel Threshold to decide which artistic character write in a 4x4 rectangle, "
					"on curses, stdout and simpletext drivers with text artistic emulation enabled");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Pixel Threshold to decide which artistic character write in a 4x4 rectangle, "
					"on curses, stdout and simpletext drivers with text artistic emulation enabled");

		}


		if (menu_display_aa_cond() ) {

#ifdef COMPILE_AA
			sprintf (buffer_string,"Slow AAlib emulation: %s", (scraa_fast==0 ? "On" : "Off"));
#else
			sprintf (buffer_string,"Slow AAlib emulation: Off");
#endif
			menu_add_item_menu(array_menu_display_settings,buffer_string,MENU_OPCION_NORMAL,menu_display_slowaa,menu_display_aa_cond);

			menu_add_item_menu_tooltip(array_menu_display_settings,"Enable slow aalib emulation; slow is a little better");
			menu_add_item_menu_ayuda(array_menu_display_settings,"Enable slow aalib emulation; slow is a little better");

		}
*/

                menu_add_item_menu(array_menu_display_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_display_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_display_settings);

                retorno_menu=menu_dibuja_menu(&display_settings_opcion_seleccionada,&item_seleccionado,array_menu_display_settings,"Display" );

                cls_menu_overlay();

		//NOTA: no llamar por numero de opcion dado que hay opciones que ocultamos (relacionadas con real video)

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {

			//llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
	                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
        	        }
		}

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}

void hotswap_zxuno_to_p2as_set_pages(void)
{

                int i;
                //Punteros a paginas de la ROM
                for (i=0;i<4;i++) {
                        rom_mem_table[i]=zxuno_sram_mem_table[i+8];
                }

                //Punteros a paginas de la RAM
                for (i=0;i<8;i++) {
                        ram_mem_table[i]=zxuno_sram_mem_table[i];
                }


                //Paginas mapeadas actuales
                for (i=0;i<4;i++) {
                        memory_paged[i]=zxuno_no_bootm_memory_paged[i];
                }
}


//Hace hotswap de cualquier maquina a spectrum 48. simplemente hace copia de los 64kb
/*NOTA: no uso esta funcion pues al pasar de zxuno a spectrum
la pantalla refresca "de otro sitio", y solo se puede ver si se habilita interfaz spectra...
es raro, es un error pero no tiene que ver con spectra, tiene que ver con divmmc:
-si hago hotswap de zxuno a 48k con divmmc, luego la pantalla no se refresca
-si hago hotswap de zxuno a 48k sin divmmc, si se ve bien
No se exactamente porque ocurre, seguramente algo que ver con peek_byte_no_time y paginas de divmmc
Uso la otra funcion de hotswap_any_machine_to_spec48 que hay mas abajo
*/
void to_delete_hotswap_any_machine_to_spec48(void)
{

	//Asignamos 64kb RAM
	z80_byte *memoria_spectrum_final;
	memoria_spectrum_final=malloc(65536);

	if (memoria_spectrum_final==NULL) {
		cpu_panic ("Error. Cannot allocate Machine memory");
	}

        //Copiamos ROM y RAM a destino
        int i;
        //ROM y RAM
        for (i=0;i<65536;i++) memoria_spectrum_final[i]=peek_byte_no_time(i);

	free(memoria_spectrum);
        memoria_spectrum=memoria_spectrum_final;

	current_machine_type=1;

        set_machine_params();
        post_set_machine(NULL);

}

void hotswap_any_machine_to_spec48(void)
{

        //Asignamos 64kb RAM
        z80_byte *memoria_buffer;
        memoria_buffer=malloc(65536);

        if (memoria_buffer==NULL) {
                cpu_panic ("Error. Cannot allocate Machine memory");
        }

        //Copiamos ROM y RAM en buffer
        int i;
        for (i=0;i<65536;i++) memoria_buffer[i]=peek_byte_no_time(i);

        //Spectrum 48k
        current_machine_type=1;

        set_machine(NULL);

        //Copiar contenido de buffer en ROM y RAM
	//Por tanto la rom de destino sera la que habia antes del hotswap
	memcpy(memoria_spectrum,memoria_buffer,65536);

        free(memoria_buffer);
}

void hotswap_p2a_to_128(void)
{
	//Copiamos ROM0 y ROM3 en ROM 0 y 1 de spectrum 128k
	//La ram la copiamos tal cual
	z80_byte old_puerto_32765=puerto_32765;

	//Asignamos 32+128k de memoria
	z80_byte *memoria_buffer;
	memoria_buffer=malloc((32+128)*1024);

	if (memoria_buffer==NULL) {
					cpu_panic ("Error. Cannot allocate Machine memory");
	}

	//Copiamos rom 0 en buffer
	int i;
	z80_byte *puntero;
	puntero=rom_mem_table[0];
	for (i=0;i<16384;i++) {
		memoria_buffer[i]=*puntero;
		puntero++;
	}

	//Copiamos rom 3 en buffer
	puntero=rom_mem_table[3];
	for (i=0;i<16384;i++) {
		memoria_buffer[16384+i]=*puntero;
		puntero++;
	}

	//Copiamos paginas de ram
	int pagina;
	for (pagina=0;pagina<8;pagina++) {
		puntero=ram_mem_table[pagina];
		for (i=0;i<16384;i++) {
			memoria_buffer[32768+pagina*16384+i]=*puntero;
			puntero++;
		}
	}

	//Spectrum 128k
        current_machine_type=6;

        set_machine(NULL);

 //Mapear como estaba
 puerto_32765=old_puerto_32765;

	//asignar ram
	mem_page_ram_128k();

	//asignar rom
	mem_page_rom_128k();

	//Copiar contenido de buffer en ROM y RAM
	//No nos complicamos la vida, como sabemos que viene lineal las dos roms y la ram, volcamos

	for (i=0;i<(32+128)*1024;i++) memoria_spectrum[i]=memoria_buffer[i];

  free(memoria_buffer);



}



void hotswap_128_to_p2a(void)
{
	//Copiamos ROM0 en ROM0, ROM1 en ROM1, ROM0 en ROM2 y ROM1 en ROM3 de +2a
	//La ram la copiamos tal cual
	z80_byte old_puerto_32765=puerto_32765;

	//Asignamos 64+128k de memoria
	z80_byte *memoria_buffer;
	memoria_buffer=malloc((64+128)*1024);

	if (memoria_buffer==NULL) {
					cpu_panic ("Error. Cannot allocate Machine memory");
	}

	//Copiamos rom 0 y en buffer
	int i;
	z80_byte *puntero;
	puntero=rom_mem_table[0];
	for (i=0;i<16384;i++) {
		memoria_buffer[i]=*puntero;
		memoria_buffer[32768+i]=*puntero;
		puntero++;
	}

	//Copiamos rom 3 en buffer
	puntero=rom_mem_table[1];
	for (i=0;i<16384;i++) {
		memoria_buffer[16384+i]=*puntero;
		memoria_buffer[49152+i]=*puntero;
		puntero++;
	}

	//Copiamos paginas de ram
	int pagina;
	for (pagina=0;pagina<8;pagina++) {
		puntero=ram_mem_table[pagina];
		for (i=0;i<16384;i++) {
			memoria_buffer[65536+pagina*16384+i]=*puntero;
			puntero++;
		}
	}

	//Spectrum +2A
        current_machine_type=11;

        set_machine(NULL);

 //Mapear como estaba
 puerto_32765=old_puerto_32765;

 puerto_8189=0;

	//asignar ram
	mem_page_ram_p2a();

	//asignar rom
	mem_page_rom_p2a();

	//Copiar contenido de buffer en ROM y RAM
	//No nos complicamos la vida, como sabemos que viene lineal las  roms y la ram, volcamos

	for (i=0;i<(64+128)*1024;i++) memoria_spectrum[i]=memoria_buffer[i];

  free(memoria_buffer);



}


//Hace hotswap de cualquier maquina 48 a spectrum 128. Se guarda los 48kb de ram en un buffer, cambia maquina, y vuelca contenido ram
void hotswap_any_machine_to_spec128(void)
{

        //Asignamos 48kb RAM
        z80_byte *memoria_buffer;
        memoria_buffer=malloc(49152);

        if (memoria_buffer==NULL) {
                cpu_panic ("Error. Cannot allocate Machine memory");
        }

        //Copiamos RAM en buffer
        int i;
        for (i=0;i<49152;i++) memoria_buffer[i]=peek_byte_no_time(16384+i);

	//Spectrum 128k
        current_machine_type=6;

        set_machine(NULL);

	//Paginar ROM 1 y RAM 0
	puerto_32765=16;

	//asignar ram
	mem_page_ram_128k();

	//asignar rom
	mem_page_rom_128k();

	//Copiar contenido de buffer en RAM
	for (i=0;i<49152;i++) poke_byte_no_time(16384+i,memoria_buffer[i]);

        free(memoria_buffer);
}



void menu_hotswap_machine(MENU_ITEM_PARAMETERS)
{

                menu_item *array_menu_machine_selection;
                menu_item item_seleccionado;
                int retorno_menu;

                do {

			//casos maquinas 16k, 48k
			if (MACHINE_IS_SPECTRUM_16_48) {
				hotswap_machine_opcion_seleccionada=current_machine_type;
        	                menu_add_item_menu_inicial(&array_menu_machine_selection,"Spectrum 16k",MENU_OPCION_NORMAL,NULL,NULL);
                	        menu_add_item_menu(array_menu_machine_selection,"Spectrum 48k",MENU_OPCION_NORMAL,NULL,NULL);
                        	menu_add_item_menu(array_menu_machine_selection,"Inves Spectrum +",MENU_OPCION_NORMAL,NULL,NULL);
	                        menu_add_item_menu(array_menu_machine_selection,"Microdigital TK90X",MENU_OPCION_NORMAL,NULL,NULL);
        	                menu_add_item_menu(array_menu_machine_selection,"Microdigital TK90X (Spanish)",MENU_OPCION_NORMAL,NULL,NULL);
                	        menu_add_item_menu(array_menu_machine_selection,"Microdigital TK95",MENU_OPCION_NORMAL,NULL,NULL);
				menu_add_item_menu(array_menu_machine_selection,"Spectrum 128",MENU_OPCION_NORMAL,NULL,NULL);
			}

			//casos maquinas 128k,+2 (y no +2a)
			if (MACHINE_IS_SPECTRUM_128_P2) {
				hotswap_machine_opcion_seleccionada=current_machine_type-6;
	                        menu_add_item_menu_inicial(&array_menu_machine_selection,"Spectrum 128k",MENU_OPCION_NORMAL,NULL,NULL);
        	                menu_add_item_menu(array_menu_machine_selection,"Spectrum 128k (Spanish)",MENU_OPCION_NORMAL,NULL,NULL);
                	        menu_add_item_menu(array_menu_machine_selection,"Spectrum +2",MENU_OPCION_NORMAL,NULL,NULL);
                        	menu_add_item_menu(array_menu_machine_selection,"Spectrum +2 (French)",MENU_OPCION_NORMAL,NULL,NULL);
	                        menu_add_item_menu(array_menu_machine_selection,"Spectrum +2 (Spanish)",MENU_OPCION_NORMAL,NULL,NULL);
													menu_add_item_menu(array_menu_machine_selection,"Spectrum +2A (ROM v4.0)",MENU_OPCION_NORMAL,NULL,NULL);
				menu_add_item_menu(array_menu_machine_selection,"Spectrum 48k",MENU_OPCION_NORMAL,NULL,NULL);
			}

			//maquinas p2a
			if (MACHINE_IS_SPECTRUM_P2A) {
				hotswap_machine_opcion_seleccionada=current_machine_type-11;
	                        menu_add_item_menu_inicial(&array_menu_machine_selection,"Spectrum +2A (ROM v4.0)",MENU_OPCION_NORMAL,NULL,NULL);
        	                menu_add_item_menu(array_menu_machine_selection,"Spectrum +2A (ROM v4.1)",MENU_OPCION_NORMAL,NULL,NULL);
                	        menu_add_item_menu(array_menu_machine_selection,"Spectrum +2A (Spanish)",MENU_OPCION_NORMAL,NULL,NULL);
													menu_add_item_menu(array_menu_machine_selection,"Spectrum 128k",MENU_OPCION_NORMAL,NULL,NULL);
				menu_add_item_menu(array_menu_machine_selection,"Spectrum 48k",MENU_OPCION_NORMAL,NULL,NULL);
			}

                        //maquinas zxuno
                        if (MACHINE_IS_ZXUNO) {
                                hotswap_machine_opcion_seleccionada=current_machine_type-14;
                                menu_add_item_menu_inicial(&array_menu_machine_selection,"Spectrum +2A (ROM v4.0)",MENU_OPCION_NORMAL,NULL,NULL);

	                        menu_add_item_menu_tooltip(array_menu_machine_selection,"The final machine type is "
							"Spectrum +2A (ROM v4.0) but the data ROM really comes from ZX-Uno");
	                        menu_add_item_menu_ayuda(array_menu_machine_selection,"The final machine type is "
							"Spectrum +2A (ROM v4.0) but the data ROM really comes from ZX-Uno");


				menu_add_item_menu(array_menu_machine_selection,"Spectrum 48k",MENU_OPCION_NORMAL,NULL,NULL);


                        }


			//maquinas zx80, zx81
			if (MACHINE_IS_ZX8081) {
				hotswap_machine_opcion_seleccionada=current_machine_type-120;
				menu_add_item_menu_inicial(&array_menu_machine_selection,"ZX-80",MENU_OPCION_NORMAL,NULL,NULL);
	                        menu_add_item_menu(array_menu_machine_selection,"ZX-81",MENU_OPCION_NORMAL,NULL,NULL);
			}

                        //maquinas z88
                        if (MACHINE_IS_Z88) {
                                hotswap_machine_opcion_seleccionada=current_machine_type-130;
                                menu_add_item_menu_inicial(&array_menu_machine_selection,"Z88",MENU_OPCION_NORMAL,NULL,NULL);
                        }

			//maquinas chloe
			if (MACHINE_IS_CHLOE) {
				if (MACHINE_IS_CHLOE_140SE) hotswap_machine_opcion_seleccionada=0;
				if (MACHINE_IS_CHLOE_280SE) hotswap_machine_opcion_seleccionada=1;

				menu_add_item_menu_inicial(&array_menu_machine_selection,"Chloe 140SE",MENU_OPCION_NORMAL,NULL,NULL);
				menu_add_item_menu(array_menu_machine_selection,"Chloe 280SE",MENU_OPCION_NORMAL,NULL,NULL);
				menu_add_item_menu(array_menu_machine_selection,"Spectrum 48k",MENU_OPCION_NORMAL,NULL,NULL);


			}

			//maquina Prism
			if (MACHINE_IS_PRISM) {
				hotswap_machine_opcion_seleccionada=0;
				menu_add_item_menu_inicial(&array_menu_machine_selection,"Spectrum 48k",MENU_OPCION_NORMAL,NULL,NULL);
			}

			//Timex ts 2068
			if  (MACHINE_IS_TIMEX_TS2068) {
                                hotswap_machine_opcion_seleccionada=0;
                                menu_add_item_menu_inicial(&array_menu_machine_selection,"Spectrum 48k",MENU_OPCION_NORMAL,NULL,NULL);
                        }

			if (MACHINE_IS_TBBLUE) {
                                hotswap_machine_opcion_seleccionada=0;
                                menu_add_item_menu_inicial(&array_menu_machine_selection,"Spectrum 48k",MENU_OPCION_NORMAL,NULL,NULL);
                        }




                        menu_add_item_menu(array_menu_machine_selection,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                        //menu_add_item_menu(array_menu_machine_selection,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
			menu_add_ESC_item(array_menu_machine_selection);

			retorno_menu=menu_dibuja_menu(&hotswap_machine_opcion_seleccionada,&item_seleccionado,array_menu_machine_selection,"Hotswap Machine" );

	                cls_menu_overlay();


                        if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
				//casos maquinas 16k, 48k
	                        if (MACHINE_IS_SPECTRUM_16_48) {
					//Caso especial cuando se cambia entre maquina Inves, porque la asignacion de memoria es diferente
					if (MACHINE_IS_INVES || hotswap_machine_opcion_seleccionada==2) {
						//si misma maquina inves origen o destino, no hacer nada

						//Cambiamos de Inves a otra
						if (MACHINE_IS_INVES && hotswap_machine_opcion_seleccionada!=2) {
							//Asignamos 64kb RAM
							z80_byte *memoria_spectrum_final;
						        memoria_spectrum_final=malloc(65536);

						        if (memoria_spectrum_final==NULL) {
						                cpu_panic ("Error. Cannot allocate Machine memory");
						        }

							//Copiamos ROM y RAM a destino
							int i;
							//ROM
							for (i=0;i<16384;i++) memoria_spectrum_final[i]=memoria_spectrum[65536+i];

							//RAM
							for (i=16384;i<65536;i++) memoria_spectrum_final[i]=memoria_spectrum[i];

							free(memoria_spectrum);
							memoria_spectrum=memoria_spectrum_final;

						}
						if (!(MACHINE_IS_INVES) && hotswap_machine_opcion_seleccionada==2) {
							//Asignamos 80 kb RAM
                                                        z80_byte *memoria_spectrum_final;
                                                        memoria_spectrum_final=malloc(65536+16384);

                                                        if (memoria_spectrum_final==NULL) {
                                                                cpu_panic ("Error. Cannot allocate Machine memory");
                                                        }

							//Copiamos ROM y RAM a destino
							int i;
							//ROM
                                                        for (i=0;i<16384;i++) memoria_spectrum_final[65536+i]=memoria_spectrum[i];

                                                        //RAM
                                                        for (i=16384;i<65536;i++) memoria_spectrum_final[i]=memoria_spectrum[i];

                                                        free(memoria_spectrum);
                                                        memoria_spectrum=memoria_spectrum_final;

							//Establecemos valores de low ram inves
							random_ram_inves(memoria_spectrum,16384);


						}
					}

					//Hotswap a 128
          if (hotswap_machine_opcion_seleccionada==6) {
						debug_printf (VERBOSE_INFO,"Hotswapping to Spectrum 128");
                                                hotswap_any_machine_to_spec128();
                                        }

					else {
						current_machine_type=hotswap_machine_opcion_seleccionada;
				        	set_machine_params();
				        	post_set_machine(NULL);
					}
                                        //Y salimos de todos los menus
                                        salir_todos_menus=1;
					return; //Para evitar saltar a otro if
				}

				if (MACHINE_IS_SPECTRUM_128_P2) {
					if (hotswap_machine_opcion_seleccionada==6) {
						hotswap_any_machine_to_spec48();
					}

					else if (hotswap_machine_opcion_seleccionada==5) {
						hotswap_128_to_p2a();
						menu_warn_message("Note that ROM data are the previous data coming from 128K");
					}

					else {
						current_machine_type=hotswap_machine_opcion_seleccionada+6;
        	                                set_machine_params();
                	                        post_set_machine(NULL);
					}
                        	        //Y salimos de todos los menus
                                	salir_todos_menus=1;
					return; //Para evitar saltar a otro if
                                }

				if (MACHINE_IS_SPECTRUM_P2A) {
                                        if (hotswap_machine_opcion_seleccionada==4) {
                                                hotswap_any_machine_to_spec48();
                                        }
					else if (hotswap_machine_opcion_seleccionada==3) {
						//De +2A a 128k
						hotswap_p2a_to_128();
						menu_warn_message("Note that ROM data are the previous data coming from +2A");
					}
					else {
	                                        current_machine_type=hotswap_machine_opcion_seleccionada+11;
        	                                set_machine_params();
                	                        post_set_machine(NULL);
					}
                                        //Y salimos de todos los menus
                                        salir_todos_menus=1;
					return; //Para evitar saltar a otro if
                                }

				if (MACHINE_IS_ZX8081) {
					current_machine_type=hotswap_machine_opcion_seleccionada+120;
					set_machine_params();
                                        post_set_machine(NULL);

					//ajustar algunos registros
					if (MACHINE_IS_ZX80) reg_i=0x0E;

					if (MACHINE_IS_ZX81) {
						reg_i=0x1E;
						nmi_generator_active.v=0;
					}

                                        //Y salimos de todos los menus
                                        salir_todos_menus=1;
					return; //Para evitar saltar a otro if
                                }

				if (MACHINE_IS_ZXUNO) {
                                        if (hotswap_machine_opcion_seleccionada==1) {
                                                hotswap_any_machine_to_spec48();
                                        }

					else {
						current_machine_type=hotswap_machine_opcion_seleccionada+11;
						set_machine_params();

						//no cargar rom, la rom sera la que haya activa en las paginas del zxuno
						post_set_machine_no_rom_load();

						//dejamos toda la memoria que hay asignada del zx-uno, solo que
						//reasignamos los punteros de paginacion del +2a

						hotswap_zxuno_to_p2as_set_pages();


						//Si teniamos el divmmc activo. Llamar a esto manualmente, no a todo divmmc_enable(),
						//pues cargaria por ejemplo el firmware esxdos de disco, y mejor conservamos el mismo firmware
						//que haya cargado el ZX-Uno
						if (diviface_enabled.v) {
							diviface_set_peek_poke_functions();
							//diviface_paginacion_manual_activa.v=0;
							diviface_control_register&=(255-128);
							diviface_paginacion_automatica_activa.v=0;
						}

						menu_warn_message("Note that ROM data are the previous data coming from ZX-Uno");

					}

					salir_todos_menus=1;
					return; //Para evitar saltar a otro if
				}

				if (MACHINE_IS_Z88) {
					//no hacer nada
					salir_todos_menus=1;
					return; //Para evitar saltar a otro if
				}

				if (MACHINE_IS_CHLOE) {
                                        if (hotswap_machine_opcion_seleccionada==2) {
                                                hotswap_any_machine_to_spec48();
                                        }

					else {
						current_machine_type=hotswap_machine_opcion_seleccionada+15;
        	                                set_machine_params();
                	                        post_set_machine(NULL);

					}
                                        //Y salimos de todos los menus
                                        salir_todos_menus=1;
					return; //Para evitar saltar a otro if
				}

				if (MACHINE_IS_PRISM) {
					hotswap_any_machine_to_spec48();
                                        salir_todos_menus=1;
					return; //Para evitar saltar a otro if
                                }

				if (MACHINE_IS_TIMEX_TS2068) {
                                        hotswap_any_machine_to_spec48();
                                        salir_todos_menus=1;
					return; //Para evitar saltar a otro if
                                }

				if (MACHINE_IS_TBBLUE) {
                                        hotswap_any_machine_to_spec48();
                                        salir_todos_menus=1;
                                        return; //Para evitar saltar a otro if
                                }




                        }

		} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}

int custom_machine_type=0;
char custom_romfile[PATH_MAX]="";

static char *custom_machine_names[]={
                "Spectrum 16k",
                "Spectrum 48k",

		"TK90X/95",

                "Spectrum 128k/+2",
                "Spectrum +2A",
		"ZX-Uno",

		"Chloe 140 SE",
		"Chloe 280 SE",
		"Timex TS 2068",
		"Prism",

                "ZX80",
                "ZX81",
		"Jupiter Ace",
		"Z88",
		"Amstrad CPC 464",
		"Sam Coupe",
		"QL"
};


void menu_custom_machine_romfile(MENU_ITEM_PARAMETERS)
{

        char *filtros[2];

        filtros[0]="rom";
        filtros[1]=0;


        if (menu_filesel("Select ROM File",filtros,custom_romfile)==1) {
                //

        }

        else {
                custom_romfile[0]=0;
        }
}




void menu_custom_machine_change(MENU_ITEM_PARAMETERS)
{
	custom_machine_type++;
	if (custom_machine_type==17) custom_machine_type=0;
}

void menu_custom_machine_run(MENU_ITEM_PARAMETERS)
{

	int minimum_size=0;
	int next_machine_type;

	switch (custom_machine_type) {

		case 0:
			next_machine_type=0;
		break;

		case 1:
			next_machine_type=1;
		break;

		case 2:
			next_machine_type=3;
		break;


		case 3:
			next_machine_type=6;
		break;

		case 4:
			next_machine_type=11;
		break;

		case 5:
			//ZX-Uno
			next_machine_type=14;
		break;

/*

                "Chloe 140 SE",
                "Chloe 280 SE",
                "Timex TS 2068",
                "Prism",
                "Amstrad CPC 464",
*/

/*
15=Chloe 140SE
16=Chloe 280SE
17=Timex TS2068
18=Prism
40=amstrad cpc464
*/

		case 6:
			//Chloe 140SE
			next_machine_type=15;
		break;

		case 7:
			//Chloe 280SE
			next_machine_type=16;
		break;

		case 8:
			//Timex TS2068
			next_machine_type=17;
		break;

		case 9:
			//Prism
			next_machine_type=18;
		break;



		case 10:
			//ZX-80
			next_machine_type=120;
		break;

		case 11:
			//ZX-81
			next_machine_type=121;
		break;

		case 12:
			//ACE
			next_machine_type=122;
		break;

		case 13:
			//Z88
			next_machine_type=130;
		break;

		case 14:
			//Amstrad CPC 464
			next_machine_type=140;
		break;

		case 15:
			//Sam Coupe
			next_machine_type=150;
		break;

		case 16:
			//QL
			next_machine_type=MACHINE_ID_QL_STANDARD;
		break;

		default:
			cpu_panic("Custom machine type unknown");
			//este return no hace falta, solo es para silenciar los warning de variable next_machine_type no inicializada
			return;
		break;

	}

	//Ver tamanyo archivo rom
	minimum_size=get_rom_size(next_machine_type);

	struct stat buf_stat;


                if (stat(custom_romfile, &buf_stat)!=0) {
                        debug_printf(VERBOSE_ERR,"Unable to find rom file %s",custom_romfile);
			return;
                }

                else {
                        //Tamaño del archivo es >=minimum_size
                        if (buf_stat.st_size<minimum_size) {
				debug_printf(VERBOSE_ERR,"ROM file must be at least %d bytes length",minimum_size);
                                return;
                        }
                }

	current_machine_type=next_machine_type;
	set_machine(custom_romfile);
	cold_start_cpu_registers();
	reset_cpu();

	//Y salimos de todos los menus
	salir_todos_menus=1;

}

void menu_custom_machine(MENU_ITEM_PARAMETERS)
{
   menu_item *array_menu_custom_machine;
        menu_item item_seleccionado;
        int retorno_menu;

	//Tipo de maquina: 16k,48k,128k/+2,+2a,zx80,zx81,ace,z88
	//Archivo ROM

	//sprintf(custom_romfile,"%s","alternaterom_plus2b.rom");

        do {
                menu_add_item_menu_inicial_format(&array_menu_custom_machine,MENU_OPCION_NORMAL,menu_custom_machine_change,NULL,"Machine Type: %s",custom_machine_names[custom_machine_type] );

		char string_romfile_shown[16];
                menu_tape_settings_trunc_name(custom_romfile,string_romfile_shown,16);

                menu_add_item_menu_format(array_menu_custom_machine,MENU_OPCION_NORMAL,menu_custom_machine_romfile,NULL,"Rom File: %s",string_romfile_shown);

		menu_add_item_menu_format(array_menu_custom_machine,MENU_OPCION_NORMAL,menu_custom_machine_run,NULL,"Run machine");



                menu_add_item_menu(array_menu_custom_machine,"",MENU_OPCION_SEPARADOR,NULL,NULL);

                menu_add_ESC_item(array_menu_custom_machine);

                retorno_menu=menu_dibuja_menu(&custom_machine_opcion_seleccionada,&item_seleccionado,array_menu_custom_machine,"Custom Machine" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}

int menu_hotswap_machine_cond(void) {

	if (MACHINE_IS_Z88) return 0;
	if (MACHINE_IS_ACE) return 0;
	//if (MACHINE_IS_TIMEX_TS2068) return 0;
	if (MACHINE_IS_CPC) return 0;
	if (MACHINE_IS_SAM) return 0;
	if (MACHINE_IS_QL) return 0;
	//if (MACHINE_IS_PRISM) return 0;

	return 1;
}

void menu_machine_selection_for_manufacturer(int fabricante)
{
	int i;
	int *maquinas;

	maquinas=return_maquinas_fabricante(fabricante);

	//cambiar linea seleccionada a maquina en cuestion
	int indice_maquina=return_machine_position(maquinas,current_machine_type);
	if (indice_maquina!=255) machine_selection_por_fabricante_opcion_seleccionada=indice_maquina;
	else {
		//Maquina no es de este menu. Resetear linea a 0
		machine_selection_por_fabricante_opcion_seleccionada=0;
	}

	char *nombre_maquina;

	int total_maquinas;

	int m;

        //Seleccion por fabricante
                menu_item *array_menu_machine_selection_por_fabricante;
                menu_item item_seleccionado;
                int retorno_menu;
	do {

		for (i=0;maquinas[i]!=255;i++) {
			m=maquinas[i];
			//printf ("%d\n",m);
			nombre_maquina=get_machine_name(m);
			//printf ("%d %s\n",m,nombre_maquina);


			if (i==0) {
				//Primer fabricante
	                        menu_add_item_menu_inicial_format(&array_menu_machine_selection_por_fabricante,MENU_OPCION_NORMAL,NULL,NULL,"%s",nombre_maquina);

			}

			else {
				menu_add_item_menu_format(array_menu_machine_selection_por_fabricante,MENU_OPCION_NORMAL,NULL,NULL,"%s",nombre_maquina);
			}


		}

		total_maquinas=i;




      menu_add_item_menu(array_menu_machine_selection_por_fabricante,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                        //menu_add_item_menu(array_menu_machine_selection_por_fabricante,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                        menu_add_ESC_item(array_menu_machine_selection_por_fabricante);



                        retorno_menu=menu_dibuja_menu(&machine_selection_por_fabricante_opcion_seleccionada,&item_seleccionado,array_menu_machine_selection_por_fabricante,"Select machine" );

                        //printf ("Opcion seleccionada: %d\n",machine_selection_por_fabricante_opcion_seleccionada);

                        cls_menu_overlay();

                        if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {

                                if (machine_selection_por_fabricante_opcion_seleccionada>=0 && machine_selection_por_fabricante_opcion_seleccionada<=total_maquinas) {



					//printf ("Seleccion opcion=%d\n",machine_selection_por_fabricante_opcion_seleccionada);
					int id_maquina=maquinas[machine_selection_por_fabricante_opcion_seleccionada];
					//printf ("Maquina= %d %s\n",id_maquina, get_machine_name(id_maquina) );

					current_machine_type=id_maquina;

				     set_machine(NULL);
                                        cold_start_cpu_registers();
                                        reset_cpu();

                                        //desactivar autoload
                                        //noautoload.v=1;
                                        //initial_tap_load.v=0;


                                        //expulsamos cintas
                                        eject_tape_load();
                                        eject_tape_save();

                                        //Y salimos de todos los menus
                                        salir_todos_menus=1;


                              }
                                //llamamos por valor de funcion
                                if (item_seleccionado.menu_funcion!=NULL) {
                                        //printf ("actuamos por funcion\n");
                                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                        cls_menu_overlay();
                                }


            //if (machine_selection_por_fabricante_opcion_seleccionada==17) {
                                        //hotswap machine
                                        //menu_hotswap_machine();
                                //}
                        }

                //} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC);
                } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);






}

void menu_machine_selection(MENU_ITEM_PARAMETERS)
{

	//Seleccion por fabricante
                menu_item *array_menu_machine_selection;
                menu_item item_seleccionado;
                int retorno_menu;

//return_fabricante_maquina
		//Establecemos linea menu segun fabricante activo
		machine_selection_opcion_seleccionada=return_fabricante_maquina(current_machine_type);

                do {

			//Primer fabricante
                        menu_add_item_menu_inicial_format(&array_menu_machine_selection,MENU_OPCION_NORMAL,NULL,NULL,"%s",array_fabricantes_hotkey[0]);
			menu_add_item_menu_shortcut(array_menu_machine_selection,array_fabricantes_hotkey_letra[0]);

		//Siguientes fabricantes
			int i;
			for (i=1;i<TOTAL_FABRICANTES;i++) {
				menu_add_item_menu_format(array_menu_machine_selection,MENU_OPCION_NORMAL,NULL,NULL,"%s",array_fabricantes_hotkey[i]);
				menu_add_item_menu_shortcut(array_menu_machine_selection,array_fabricantes_hotkey_letra[i]);
			}


                       menu_add_item_menu(array_menu_machine_selection,"",MENU_OPCION_SEPARADOR,NULL,NULL);

                        //Hotswap de Z88 o Jupiter Ace o CHLOE no existe
                        menu_add_item_menu(array_menu_machine_selection,"~~Hotswap machine",MENU_OPCION_NORMAL,menu_hotswap_machine,menu_hotswap_machine_cond);
                        menu_add_item_menu_shortcut(array_menu_machine_selection,'h');
                        menu_add_item_menu_tooltip(array_menu_machine_selection,"Change machine type without resetting");
                        menu_add_item_menu_ayuda(array_menu_machine_selection,"Change machine type without resetting.");

                        menu_add_item_menu(array_menu_machine_selection,"Cust~~om machine",MENU_OPCION_NORMAL,menu_custom_machine,NULL);
                        menu_add_item_menu_shortcut(array_menu_machine_selection,'o');
                        menu_add_item_menu_tooltip(array_menu_machine_selection,"Specify custom machine type & ROM");
                        menu_add_item_menu_ayuda(array_menu_machine_selection,"Specify custom machine type & ROM");

                        menu_add_item_menu(array_menu_machine_selection,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                        //menu_add_item_menu(array_menu_machine_selection,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                        menu_add_ESC_item(array_menu_machine_selection);



                        retorno_menu=menu_dibuja_menu(&machine_selection_opcion_seleccionada,&item_seleccionado,array_menu_machine_selection,"Select manufacturer" );

                        //printf ("Opcion seleccionada: %d\n",machine_selection_opcion_seleccionada);

                        cls_menu_overlay();

                        if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {

                                if (machine_selection_opcion_seleccionada>=0 && machine_selection_opcion_seleccionada<=TOTAL_FABRICANTES) {

					//printf ("Seleccionado fabricante %s\n",array_fabricantes[machine_selection_opcion_seleccionada]);

                                        //int last_machine_type=machine_type;


					menu_machine_selection_for_manufacturer(machine_selection_opcion_seleccionada);



			      }
                                //llamamos por valor de funcion
                                if (item_seleccionado.menu_funcion!=NULL) {
                                        //printf ("actuamos por funcion\n");
                                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                        cls_menu_overlay();
                                }


                                //if (machine_selection_opcion_seleccionada==17) {
                                        //hotswap machine
                                        //menu_hotswap_machine();
                                //}
                        }

                //} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC);
                } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);


}





void menu_warn_message(char *texto)
{
	menu_generic_message("Warning",texto);

}

void menu_error_message(char *texto)
{
	menu_generic_message("Error",texto);

}

//Similar a snprintf
void menu_generic_message_aux_copia(char *origen,char *destino, int longitud)
{
	while (longitud) {
		*destino=*origen;
		origen++;
		destino++;
		longitud--;
	}
}

//Aplicar filtros para caracteres extranyos y cortar linea en saltos de linea
int menu_generic_message_aux_filter(char *texto,int inicio, int final)
{
	//int copia_inicio=inicio;

	unsigned char caracter;

        while (inicio!=final) {
		caracter=texto[inicio];

                if (caracter=='\n' || caracter=='\r') {
			//printf ("detectado salto de linea en posicion %d\n",inicio);
			texto[inicio]=' ';
			return inicio+1;
		}

		//TAB. Lo cambiamos por espacio
		else if (caracter==9) {
			texto[inicio]=' ';
		}

		else if ( !(si_valid_char(caracter)) ) {
			//printf ("detectado caracter extranyo %d en posicion %d\n",caracter,inicio);

			texto[inicio]='?';
		}

                inicio++;
        }

        return final;

}


//Cortar las lineas, si se puede, por espacio entre palabras
int menu_generic_message_aux_wordwrap(char *texto,int inicio, int final)
{

	int copia_final=final;

	//ya acaba en espacio, volver
	//if (texto[final]==' ') return final;

	while (final!=inicio) {
		if (texto[final]==' ' || texto[final]=='\n' || texto[final]=='\r') return final+1;
		final--;
	}

	return copia_final;
}

int menu_generic_message_cursor_arriba(int primera_linea)
{
	if (primera_linea>0) primera_linea--;
	return primera_linea;
}

int menu_generic_message_cursor_arriba_mostrar_cursor(int primera_linea,int mostrar_cursor,int *linea_cursor)
{
                                     if (mostrar_cursor) {
                                                        int off=0;
                                                        //no limitar primera linea if (primera_linea) off++;
                                                        if (*linea_cursor>off) (*linea_cursor)--;
                                                        else primera_linea=menu_generic_message_cursor_arriba(primera_linea);
                                                }
                                                else {
                                                        primera_linea=menu_generic_message_cursor_arriba(primera_linea);
                                                }

	return primera_linea;
}



int menu_generic_message_cursor_abajo (int primera_linea,int alto_ventana,int indice_linea)
{


	//if (primera_linea<indice_linea-2) primera_linea++;
	if (primera_linea+alto_ventana-2<indice_linea) primera_linea++;
	return primera_linea;


}


int menu_generic_message_cursor_abajo_mostrar_cursor(int primera_linea,int alto_ventana,int indice_linea,int mostrar_cursor,int *linea_cursor)
{
                                                if (mostrar_cursor) {
                                                        if (*linea_cursor<alto_ventana-3) (*linea_cursor)++;
                                                        else primera_linea=menu_generic_message_cursor_abajo(primera_linea,alto_ventana,indice_linea);
                                                }
                                                else {
                                                        primera_linea=menu_generic_message_cursor_abajo(primera_linea,alto_ventana,indice_linea);
                                                }

	return primera_linea;
}


//int menu_generic_message_final_abajo(int primera_linea,int alto_ventana,int indice_linea,int mostrar_cursor,int linea_cursor)
int menu_generic_message_final_abajo(int primera_linea,int alto_ventana,int indice_linea)
{
	/*if (mostrar_cursor) {
		if (linea_cursor<alto_ventana-3) return 1;
	}

	else*/ if (primera_linea+alto_ventana-2<indice_linea) return 1;

	return 0;
}


//dibuja ventana simple, una sola linea de texto interior, sin esperar tecla
void menu_simple_ventana(char *titulo,char *texto)
{


	unsigned int ancho_ventana=strlen(titulo);
	if (strlen(texto)>ancho_ventana) ancho_ventana=strlen(texto);

	int alto_ventana=3;

	ancho_ventana +=2;

	if (ancho_ventana>32) {
		cpu_panic("window width too big");
	}

        int xventana=15-ancho_ventana/2;
        int yventana=12-alto_ventana/2;


        menu_dibuja_ventana(xventana,yventana,ancho_ventana,alto_ventana,titulo);

	menu_escribe_linea_opcion(0,-1,1,texto);

}


//Muestra un mensaje en ventana troceando el texto en varias lineas de texto de maximo 25 caracteres
void menu_generic_message_tooltip(char *titulo, int tooltip_enabled, int mostrar_cursor, generic_message_tooltip_return *retorno, const char * texto_format , ...)
{

	//Buffer de entrada

        char texto[MAX_TEXTO_GENERIC_MESSAGE];
        va_list args;
        va_start (args, texto_format);
        vsprintf (texto,texto_format, args);
	va_end (args);


	//linea cursor en el caso que se muestre cursor
	int linea_cursor=0;

	//En caso de stdout, es mas simple, mostrar texto y esperar tecla
        if (!strcmp(scr_driver_name,"stdout")) {
		//printf ("%d\n",strlen(texto));


		printf ("-%s-\n",titulo);
		printf ("\n");
		printf ("%s\n",texto);

		scrstdout_menu_print_speech_macro(titulo);
		scrstdout_menu_print_speech_macro(texto);

		menu_espera_no_tecla();
		menu_espera_tecla();

		return;
        }

	//En caso de simpletext, solo mostrar texto sin esperar tecla
	if (!strcmp(scr_driver_name,"simpletext")) {
                printf ("-%s-\n",titulo);
                printf ("\n");
                printf ("%s\n",texto);

		return;
	}


	int tecla;


	//texto que contiene cada linea con ajuste de palabra. Al trocear las lineas aumentan
	char buffer_lineas[MAX_LINEAS_TOTAL_GENERIC_MESSAGE][MAX_ANCHO_LINEAS_GENERIC_MESSAGE];

	const int max_ancho_texto=30;

	//Primera linea que mostramos en la ventana
	int primera_linea=0;

	int indice_linea=0;  //Numero total de lineas??
	int indice_texto=0;
	int ultimo_indice_texto=0;
	int longitud=strlen(texto);

	//int indice_segunda_linea;

	int texto_no_cabe=0;

	do {
		indice_texto+=max_ancho_texto;

		//temp
		//printf ("indice_linea: %d\n",indice_linea);

		//Controlar final de texto
		if (indice_texto>=longitud) indice_texto=longitud;

		//Si no, miramos si hay que separar por espacios
		else indice_texto=menu_generic_message_aux_wordwrap(texto,ultimo_indice_texto,indice_texto);

		//Separamos por salto de linea, filtramos caracteres extranyos
		indice_texto=menu_generic_message_aux_filter(texto,ultimo_indice_texto,indice_texto);

		//copiar texto
		int longitud_texto=indice_texto-ultimo_indice_texto;


		//snprintf(buffer_lineas[indice_linea],longitud_texto,&texto[ultimo_indice_texto]);


		menu_generic_message_aux_copia(&texto[ultimo_indice_texto],buffer_lineas[indice_linea],longitud_texto);
		buffer_lineas[indice_linea++][longitud_texto]=0;
		//printf ("copiado %d caracteres desde %d hasta %d: %s\n",longitud_texto,ultimo_indice_texto,indice_texto,buffer_lineas[indice_linea-1]);


		//printf ("texto: %s\n",buffer_lineas[indice_linea-1]);

		if (indice_linea==MAX_LINEAS_TOTAL_GENERIC_MESSAGE) {
                        //cpu_panic("Max lines on menu_generic_message reached");
			debug_printf(VERBOSE_INFO,"Max lines on menu_generic_message reached (%d)",MAX_LINEAS_TOTAL_GENERIC_MESSAGE);
			//finalizamos bucle
			indice_texto=longitud;
		}

		ultimo_indice_texto=indice_texto;
		//printf ("ultimo indice: %d %c\n",ultimo_indice_texto,texto[ultimo_indice_texto]);

	} while (indice_texto<longitud);


	debug_printf (VERBOSE_INFO,"Read %d lines (word wrapped)",indice_linea);
	int primera_linea_a_speech=0;
	int ultima_linea_a_speech=0;

	do {

	//printf ("primera_linea: %d\n",primera_linea);

	int alto_ventana=indice_linea+2;
	if (alto_ventana-2>MAX_LINEAS_VENTANA_GENERIC_MESSAGE) {
		alto_ventana=MAX_LINEAS_VENTANA_GENERIC_MESSAGE+2;
		texto_no_cabe=1;
	}


	//printf ("alto ventana: %d\n",alto_ventana);
	//int ancho_ventana=max_ancho_texto+2;
	int ancho_ventana=max_ancho_texto+2;

	int xventana=16-ancho_ventana/2;
	int yventana=12-alto_ventana/2;

	if (tooltip_enabled==0) {
		menu_espera_no_tecla_con_repeticion();
		cls_menu_overlay();
	}

	else {

		//Importante que este el clear_putpixel_cache
		//sino con modo zx8081 la ventana se ve oscuro
		clear_putpixel_cache();
	}

	if (tooltip_enabled==0) cls_menu_overlay();

        menu_dibuja_ventana(xventana,yventana,ancho_ventana,alto_ventana,titulo);

	int i;
	for (i=0;i<indice_linea-primera_linea && i<MAX_LINEAS_VENTANA_GENERIC_MESSAGE;i++) {
		if (mostrar_cursor) {
			menu_escribe_linea_opcion(i,linea_cursor,1,buffer_lineas[i+primera_linea]);
		}
        	else menu_escribe_linea_opcion(i,-1,1,buffer_lineas[i+primera_linea]);
		//printf ("i: %d linea_cursor: %d primera_linea: %d\n",i,linea_cursor,primera_linea);
		//printf ("Linea seleccionada: %d (%s)\n",linea_cursor+primera_linea,buffer_lineas[linea_cursor+primera_linea]);
	}


	//Enviar primera linea o ultima a speech

	//La primera linea puede estar oculta por .., aunque para speech mejor que diga esa primera linea oculta
	debug_printf (VERBOSE_DEBUG,"First line: %s",buffer_lineas[primera_linea]);
	debug_printf (VERBOSE_DEBUG,"Last line: %s",buffer_lineas[i+primera_linea-1]);

	if (primera_linea_a_speech) {
		menu_speech_tecla_pulsada=0;
		menu_textspeech_send_text(buffer_lineas[primera_linea]);
	}
	if (ultima_linea_a_speech) {
		menu_speech_tecla_pulsada=0;
		menu_textspeech_send_text(buffer_lineas[i+primera_linea-1]);
	}


	primera_linea_a_speech=0;
	ultima_linea_a_speech=0;
	menu_speech_tecla_pulsada=1;


	//Escribir cursores en linea inferior
	//26 espacios
	//char cursores[]="                          ";
	//abajo
	//if (primera_linea<indice_linea-1) cursores[20]='D';
	//Meter puntos suspensivos en la ultima linea indicando que hay mas abajo
	if (primera_linea+alto_ventana-2<indice_linea) {
		menu_escribe_linea_opcion(MAX_LINEAS_VENTANA_GENERIC_MESSAGE,-1,1,"...");
	}

	if (texto_no_cabe) {
		// mostrar * a la derecha para indicar donde estamos en porcentaje
		//menu_dibuja_ventana(xventana,yventana,ancho_ventana,alto_ventana,titulo);
		int ybase=yventana+1;
		int porcentaje=((primera_linea+linea_cursor)*100)/(indice_linea+1); //+1 para no hacer division por cero
		//int porcentaje=((primera_linea+alto_ventana)*100)/(indice_linea+1); //+1 para no hacer division por cero

		//if (menu_generic_message_final_abajo(primera_linea,alto_ventana,indice_linea,mostrar_cursor,linea_cursor)==0)
		if (menu_generic_message_final_abajo(primera_linea,alto_ventana,indice_linea)==0)
			porcentaje=100;

		debug_printf (VERBOSE_DEBUG,"Percentage reading window: %d",porcentaje);

		int sumaralto=((alto_ventana-3)*porcentaje)/100;
		putchar_menu_overlay(xventana+ancho_ventana-1,ybase+sumaralto,'*',ESTILO_GUI_PAPEL_NORMAL,ESTILO_GUI_TINTA_NORMAL);

	}

	//arriba
	//if (primera_linea>0) cursores[21]='U';
	//Meter puntos suspensivos en la primera linea indicando que hay mas arriba

	//if (primera_linea>0) menu_escribe_linea_opcion(0,-1,1,"... ");

	/* No mostrar puntos suspensivos arriba
	if (primera_linea>0) {
		char buffer_temp[MAX_ANCHO_LINEAS_GENERIC_MESSAGE];
		sprintf (buffer_temp,"%s",buffer_lineas[primera_linea]);
		buffer_temp[0]='.';
		buffer_temp[1]='.';
		buffer_temp[2]='.';
		buffer_temp[3]=' ';
		//Si texto impreso era menor que esos "... "
		if (strlen(buffer_lineas[primera_linea])<4) buffer_temp[4]=0;

		menu_escribe_linea_opcion(0,-1,1,buffer_temp);
	}
	*/


        all_interlace_scr_refresca_pantalla();
        menu_espera_tecla();

                menu_espera_tecla();
                tecla=menu_get_pressed_key();

								if (mouse_movido) {
												//printf ("mouse x: %d y: %d menu mouse x: %d y: %d\n",mouse_x,mouse_y,menu_mouse_x,menu_mouse_y);
												//printf ("ventana x %d y %d ancho %d alto %d\n",ventana_x,ventana_y,ventana_ancho,ventana_alto);
												if (si_menu_mouse_en_ventana() ) {
														//printf ("mouse en ventana\n");
														//Si en zona inferior
														if (menu_mouse_y==ventana_alto-1) {
															//printf ("mouse en linea inferior\n");
															tecla=10;
														}
														else if (menu_mouse_y==1) {
															//printf ("mouse en linea superior\n");
															tecla=11;
														}
												}
								}

		if (tooltip_enabled==0) menu_espera_no_tecla_con_repeticion();

		//Decir que no se ha pulsado tecla para que se relea esto cada vez
		//menu_speech_tecla_pulsada=0;

		int contador_pgdnup;

                               switch (tecla) {
                                        //abajo
                                        case 10:
						primera_linea=menu_generic_message_cursor_abajo_mostrar_cursor(primera_linea,alto_ventana,indice_linea,mostrar_cursor,&linea_cursor);

						//Decir que se ha pulsado tecla para que no se relea
						menu_speech_tecla_pulsada=1;
						ultima_linea_a_speech=1;
                                        break;

                                        //arriba
                                        case 11:
						primera_linea=menu_generic_message_cursor_arriba_mostrar_cursor(primera_linea,mostrar_cursor,&linea_cursor);

						//Decir que se ha pulsado tecla para que no se relea
						menu_speech_tecla_pulsada=1;
						primera_linea_a_speech=1;
                                        break;

					//PgUp
					case 24:
						for (contador_pgdnup=0;contador_pgdnup<MAX_LINEAS_VENTANA_GENERIC_MESSAGE-1;contador_pgdnup++) {
							primera_linea=menu_generic_message_cursor_arriba_mostrar_cursor(primera_linea,mostrar_cursor,&linea_cursor);
						}
						//Decir que no se ha pulsado tecla para que se relea
						menu_speech_tecla_pulsada=0;
					break;

                                        //PgDn
                                        case 25:
                                                for (contador_pgdnup=0;contador_pgdnup<MAX_LINEAS_VENTANA_GENERIC_MESSAGE-1;contador_pgdnup++) {
							primera_linea=menu_generic_message_cursor_abajo_mostrar_cursor(primera_linea,alto_ventana,indice_linea,mostrar_cursor,&linea_cursor);
                                                }

						//Si esta en primera linea cursor, mover
						//if (mostrar_cursor) {
						//	if (linea_cursor==0) linea_cursor=1;
						//}
						//Decir que no se ha pulsado tecla para que se relea
						menu_speech_tecla_pulsada=0;
                                        break;

				}

	//Salir con Enter o ESC o fin de tooltip
	} while (tecla!=13 && tecla!=2 && tooltip_enabled==0);

	if (retorno!=NULL) {
		int linea_final=linea_cursor+primera_linea;
		strcpy(retorno->texto_seleccionado,buffer_lineas[linea_final]);
		retorno->linea_seleccionada=linea_final;

		// int estado_retorno; //Retorna 1 si sale con enter. Retorna 0 si sale con ESC
		if (tecla==2) retorno->estado_retorno=0;
		else retorno->estado_retorno=1;

		//printf ("\n\nLinea seleccionada: %d (%s)\n",linea_cursor+primera_linea,buffer_lineas[linea_cursor+primera_linea]);

	}

        cls_menu_overlay();

        if (tooltip_enabled==0) menu_espera_no_tecla_con_repeticion();

	cls_menu_overlay();


	//if (tooltip_enabled==0)


}


//Muestra un mensaje en ventana troceando el texto en varias lineas de texto de maximo 25 caracteres
/*void menu_generic_message_tooltip(char *titulo, int tooltip_enabled, const char * texto_format , ...)
{
	menu_generic_message_tooltip_consincursor(titulo, tooltip_enabled,0,texto_format);
}
*/

//Esta rutina estaba originalmente en screen.c pero dado que se ha modificado para usar rutinas auxiliares de aqui, mejor que este aqui
void screen_print_splash_text(z80_byte y,z80_byte tinta,z80_byte papel,char *texto)
{

        //Si no hay driver video
        if (scr_putpixel==NULL || scr_putpixel_zoom==NULL) return;


  if (menu_abierto==0 && screen_show_splash_texts.v==1) {
                cls_menu_overlay();

                int x;

#define MAX_LINEAS_SPLASH 24
        const int max_ancho_texto=31;
	//al trocear, si hay un espacio despues, se agrega, y por tanto puede haber linea de 31+1=32 caracteres

        //texto que contiene cada linea con ajuste de palabra. Al trocear las lineas aumentan
	//33 es ancho total linea(32)+1
        char buffer_lineas[MAX_LINEAS_SPLASH][33];



        int indice_linea=0;
        int indice_texto=0;
        int ultimo_indice_texto=0;
        int longitud=strlen(texto);

        //int indice_segunda_linea;


        do {
                indice_texto+=max_ancho_texto;

                //Controlar final de texto
                if (indice_texto>=longitud) indice_texto=longitud;

                //Si no, miramos si hay que separar por espacios
                else indice_texto=menu_generic_message_aux_wordwrap(texto,ultimo_indice_texto,indice_texto);

                //Separamos por salto de linea, filtramos caracteres extranyos
                indice_texto=menu_generic_message_aux_filter(texto,ultimo_indice_texto,indice_texto);

                //copiar texto
                int longitud_texto=indice_texto-ultimo_indice_texto;



                menu_generic_message_aux_copia(&texto[ultimo_indice_texto],buffer_lineas[indice_linea],longitud_texto);
                buffer_lineas[indice_linea++][longitud_texto]=0;
                //printf ("copiado %d caracteres desde %d hasta %d: %s\n",longitud_texto,ultimo_indice_texto,indice_texto,buffer_lineas[indice_linea-1]);


        //printf ("texto indice: %d : longitud: %d: -%s-\n",indice_linea-1,longitud_texto,buffer_lineas[indice_linea-1]);
		//printf ("indice_linea: %d indice_linea+y: %d MAX: %d\n",indice_linea,indice_linea+y,MAX_LINEAS_SPLASH);

                if (indice_linea==MAX_LINEAS_SPLASH) {
                        //cpu_panic("Max lines on menu_generic_message reached");
                        debug_printf(VERBOSE_INFO,"Max lines on screen_print_splash_text reached (%d)",MAX_LINEAS_SPLASH);
                        //finalizamos bucle
                        indice_texto=longitud;
                }

                ultimo_indice_texto=indice_texto;
                //printf ("ultimo indice: %d %c\n",ultimo_indice_texto,texto[ultimo_indice_texto]);

        } while (indice_texto<longitud);

	int i;
	for (i=0;i<indice_linea && y<24;i++) {
		debug_printf (VERBOSE_DEBUG,"line %d y: %d length: %d contents: -%s-",i,y,strlen(buffer_lineas[i]),buffer_lineas[i]);
		x=16-strlen(buffer_lineas[i])/2;
		if (x<0) x=0;
		menu_escribe_texto(x,y,tinta,papel,buffer_lineas[i]);
		y++;
	}


        set_menu_overlay_function(normal_overlay_texto_menu);
        menu_splash_text_active.v=1;
        menu_splash_segundos=5;
   }

}


//retorna 1 si y
//otra cosa, 0
int menu_confirm_yesno_texto(char *texto_ventana,char *texto_interior)
{

	//Si se fuerza siempre yes
	if (force_confirm_yes.v) return 1;


	//printf ("confirm\n");

        //En caso de stdout, es mas simple, mostrar texto y esperar tecla
        if (!strcmp(scr_driver_name,"stdout")) {
		char buffer_texto[256];
                printf ("%s\n%s\n",texto_ventana,texto_interior);

		scrstdout_menu_print_speech_macro(texto_ventana);
		scrstdout_menu_print_speech_macro(texto_interior);

		fflush(stdout);
                scanf("%s",buffer_texto);
		if (buffer_texto[0]=='y' || buffer_texto[0]=='Y') return 1;
		return 0;
        }


        cls_menu_overlay();

        menu_espera_no_tecla();


	menu_item *array_menu_confirm_yes_no;
        menu_item item_seleccionado;
        int retorno_menu;

	//Siempre indicamos el NO
	int confirm_yes_no_opcion_seleccionada=2;
        do {

		menu_add_item_menu_inicial_format(&array_menu_confirm_yes_no,MENU_OPCION_SEPARADOR,NULL,NULL,texto_interior);

                menu_add_item_menu_format(array_menu_confirm_yes_no,MENU_OPCION_NORMAL,NULL,NULL,"~~Yes");
		menu_add_item_menu_shortcut(array_menu_confirm_yes_no,'y');

                menu_add_item_menu_format(array_menu_confirm_yes_no,MENU_OPCION_NORMAL,NULL,NULL,"~~No");
		menu_add_item_menu_shortcut(array_menu_confirm_yes_no,'n');

                //separador adicional para que quede mas grande la ventana y mas mono
                menu_add_item_menu_format(array_menu_confirm_yes_no,MENU_OPCION_SEPARADOR,NULL,NULL," ");



                retorno_menu=menu_dibuja_menu(&confirm_yes_no_opcion_seleccionada,&item_seleccionado,array_menu_confirm_yes_no,texto_ventana);

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
			if (confirm_yes_no_opcion_seleccionada==1) return 1;
			else return 0;
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

	return 0;


}

void menu_generic_message_format(char *titulo, const char * texto_format , ...)
{

        //Buffer de entrada
        char texto[MAX_TEXTO_GENERIC_MESSAGE];
        va_list args;
        va_start (args, texto_format);
        vsprintf (texto,texto_format, args);
        va_end (args);


	menu_generic_message_tooltip(titulo, 0, 0, NULL, "%s", texto);


	//En Linux esto funciona bien sin tener que hacer las funciones va_ previas:
	//menu_generic_message_tooltip(titulo, 0, texto_format)
	//Pero en Mac OS X no obtiene los valores de los parametros adicionales
}

void menu_generic_message(char *titulo, const char * texto)
{

        menu_generic_message_tooltip(titulo, 0, 0, NULL, "%s", texto);
}


int menu_confirm_yesno(char *texto_ventana)
{
	return menu_confirm_yesno_texto(texto_ventana,"Sure?");
}

//max_length contando caracter 0 del final, es decir, para un texto de 4 caracteres, debemos especificar max_length=5
void menu_ventana_scanf(char *titulo,char *texto,int max_length)
{

        //En caso de stdout, es mas simple, mostrar texto y esperar texto
        if (!strcmp(scr_driver_name,"stdout")) {
		printf ("%s\n",titulo);
		scrstdout_menu_print_speech_macro(titulo);
		scanf("%s",texto);

		return;
	}


        menu_espera_no_tecla();
        menu_dibuja_ventana(2,10,28,3,titulo);

	menu_scanf(texto,max_length,20,3,11);

}


void menu_about_read_file(char *title,char *aboutfile)
{

	char about_file[MAX_TEXTO_GENERIC_MESSAGE];

	debug_printf (VERBOSE_INFO,"Loading %s File",aboutfile);
	FILE *ptr_aboutfile;
	//ptr_aboutfile=fopen(aboutfile,"rb");
	open_sharedfile(aboutfile,&ptr_aboutfile);

	if (!ptr_aboutfile)
	{
		debug_printf (VERBOSE_ERR,"Unable to open %s file",aboutfile);
	}
	else {

		int leidos=fread(about_file,1,MAX_TEXTO_GENERIC_MESSAGE,ptr_aboutfile);
		debug_printf (VERBOSE_INFO,"Read %d bytes of file: %s",leidos,aboutfile);

		if (leidos==MAX_TEXTO_GENERIC_MESSAGE) {
			debug_printf (VERBOSE_ERR,"Read max text buffer: %d bytes. Showing only these",leidos);
			leidos--;
		}

		about_file[leidos]=0;


		fclose(ptr_aboutfile);

		menu_generic_message(title,about_file);

	}

}

void menu_about_changelog(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Changelog","Changelog");
}


void menu_about_history(MENU_ITEM_PARAMETERS)
{
	menu_about_read_file("History","HISTORY");
}

void menu_about_features(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Features","FEATURES");
}

void menu_about_readme(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Readme","README");
}


void menu_about_install(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Install","INSTALL");
}

void menu_about_installwindows(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Install on Windows","INSTALLWINDOWS");
}

void menu_about_alternate_roms(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Alternate ROMS","ALTERNATEROMS");
}

void menu_about_included_tapes(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Included Tapes","INCLUDEDTAPES");
}



void menu_about_acknowledgements(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Acknowledgements","ACKNOWLEDGEMENTS");
}

void menu_about_faq(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("FAQ","FAQ");
}

void menu_about_license(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("License","LICENSE");
}

void menu_about_license_motorola_core(MENU_ITEM_PARAMETERS)
{
        menu_about_read_file("Motorola License","LICENSE_MOTOROLA_CORE");
}

void menu_about_statistics(MENU_ITEM_PARAMETERS)
{

	menu_generic_message_format("Statistics",
		"Source code lines: %d\n"
		"Edited with vim and atom\n"
		"Developed on Debian 8, macOS Sierra, Raspbian and MinGW environment on Windows\n"
		,LINES_SOURCE);
}

void menu_about_running_info(MENU_ITEM_PARAMETERS)
{

	char string_video_drivers[1024];
	char string_audio_drivers[1024];
	int driver;

	int i;

	i=0;
        for (driver=0;driver<num_scr_driver_array;driver++) {
		sprintf(&string_video_drivers[i],"%s ",scr_driver_array[driver].driver_name);
		i=i+strlen(scr_driver_array[driver].driver_name)+1;
	}
	string_video_drivers[i]=0;


	i=0;
        for (driver=0;driver<num_audio_driver_array;driver++) {
                sprintf(&string_audio_drivers[i],"%s ",audio_driver_array[driver].driver_name);
                i=i+strlen(audio_driver_array[driver].driver_name)+1;
        }
        string_audio_drivers[i]=0;


	menu_generic_message_format("Running info",
		"Video Driver: %s\nAvailable video drivers: %s\nAudio Driver: %s\nAvailable audio drivers: %s\n",
		scr_driver_name,string_video_drivers,audio_driver_name,string_audio_drivers);


	//temp prueba
	/*
        scr_end_pantalla();

                int (*funcion_init) ();
                int (*funcion_set) ();

		//1 es video caca
		//3 es video curses
                funcion_init=scr_driver_array[3].funcion_init;
                funcion_set=scr_driver_array[3].funcion_set;
                if ( (funcion_init()) ==0) {
                        //printf ("Ok video driver i:%d\n",i);
                        funcion_set();
		}

        scr_init_pantalla();
	*/

}



void menu_about_about(MENU_ITEM_PARAMETERS)
{

	char mensaje_about[1024];
	unsigned char letra_enye;

	if (si_complete_video_driver() ) {
		//mensaje completo con enye en segundo apellido
		letra_enye=129;
	}

	else {
		//mensaje con n en vez de enye
		letra_enye='n';
	}

	sprintf (mensaje_about,"ZEsarUX V." EMULATOR_VERSION " (" EMULATOR_SHORT_DATE ")\n"
                        " - " EMULATOR_EDITION_NAME " - \n"

#ifdef SNAPSHOT_VERSION
                "Build number: " BUILDNUMBER "\n"
#endif

                        "(C) 2013 Cesar Hernandez Ba%co\n",letra_enye);

	menu_generic_message("About",mensaje_about);


}


void menu_about_compile_info(MENU_ITEM_PARAMETERS)
{
        char buffer[MAX_COMPILE_INFO_LENGTH];
        get_compile_info(buffer);

	menu_generic_message("Compile info",
		buffer
	);
}


void menu_about_help(MENU_ITEM_PARAMETERS)
{

	if (!menu_cond_stdout() ) {
		menu_generic_message("Help",
			"Use cursor keys to move\n"
			"Press F1 (or h on some video drivers) on an option to see its help\n"
			"Press Enter to select a menu item\n"
			"Press Space on some menu items to enable/disable\n"
			"Press a key between a and z to select an entry with shortcuts; shortcut clues appear when you wait some seconds or press a key "
			"not associated with any shortcut.\n"
			"Disabled options may be hidden, or disabled, which are shown with red colour or with x cursor on some video drivers\n"
			"ESC Key gives you to the previous menu, except in the case with aalib driver and pure text console, which is changed to another key (shown on the menu). On curses driver, ESC key is a bit slow, you have to wait one second after pressing it. "
			"\n\n"
			"On fileselector:\n"
			"- Use cursors and PgDn/Up\n"
			"- Use TAB to change section\n"
			"- Use Space/cursor on filter\n"
			"- Press the initial letter\n"
			"  for faster searching"
			"\n\n"
			"On numeric input fields, numbers can be written on decimal, hexadecimal (with suffix H) or as a character (with quotes '' or \"\")\n\n"
			"Symbols on menu must be written according to the current machine keyboard mapping, so for example, to write the symbol minus (<), you have to press "
			"ctrl(symbol shift)+r on Spectrum, shift+n on ZX80/81 or shift+, on Z88.\n\n"

			"Inside a machine, the keys are mapped this way:\n"
			"CTRL/ALT: Symbol shift\n"
			"TAB: Extended mode (symbol shift + caps shift)\n"
			"\n"
			"F4: Send display content to speech program, including only known characters (unknown characters are shown as a space)\n"
			"F5: Open menu\n"
			"F8: Open On Screen Keyboard (only on Spectrum & ZX80/81\n"
			"F9: Open Smart Load window\n"
			"\n"
			"On Z88, the following keys have special meaning:\n"
			"F1: Help\n"
			"F2: Index\n"
			"F3: Menu\n"
			"CTRL: Diamond\n"
			"ALT: Square\n"
			"\n"
			"Note: Drivers curses, aa, caca do not read CTRL or ALT. Instead, you can use F6 for CTRL and F7 for ALT on these drivers.\n"
	);
	}

	else {
		menu_generic_message("Help","Press h<num> to see help for <num> option, for example: h3\n"
				"Press t<num> to see tooltip for <num> option\n"
				"Disabled options may be hidden, or disabled, which are shown with x cursor\n"
				"ESC text gives you to the previous menu\n"
				"On fileselector, write the file without spaces\n"
				"On input fields, numbers can be written on decimal or hexadecimal (with suffix H)\n"
	);
	}

}


//menu about settings
void menu_about(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_about;
        menu_item item_seleccionado;
        int retorno_menu;
        do {
                menu_add_item_menu_inicial(&array_menu_about,"~~About",MENU_OPCION_NORMAL,menu_about_about,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'a');

		menu_add_item_menu(array_menu_about,"~~Help",MENU_OPCION_NORMAL,menu_about_help,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'h');

		menu_add_item_menu(array_menu_about,"~~Readme",MENU_OPCION_NORMAL,menu_about_readme,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'r');

		menu_add_item_menu(array_menu_about,"~~Features",MENU_OPCION_NORMAL,menu_about_features,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'f');

                menu_add_item_menu(array_menu_about,"H~~istory",MENU_OPCION_NORMAL,menu_about_history,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'i');

                menu_add_item_menu(array_menu_about,"A~~cknowledgements",MENU_OPCION_NORMAL,menu_about_acknowledgements,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'c');

		menu_add_item_menu(array_menu_about,"FA~~Q",MENU_OPCION_NORMAL,menu_about_faq,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'q');

		menu_add_item_menu(array_menu_about,"Cha~~ngelog",MENU_OPCION_NORMAL,menu_about_changelog,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'n');

		menu_add_item_menu(array_menu_about,"Alternate RO~~MS",MENU_OPCION_NORMAL,menu_about_alternate_roms,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'m');

		menu_add_item_menu(array_menu_about,"Included ~~tapes",MENU_OPCION_NORMAL,menu_about_included_tapes,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'t');

		menu_add_item_menu(array_menu_about,"Insta~~ll",MENU_OPCION_NORMAL,menu_about_install,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'l');

		menu_add_item_menu(array_menu_about,"Install on ~~Windows",MENU_OPCION_NORMAL,menu_about_installwindows,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'w');

                menu_add_item_menu(array_menu_about,"~~Statistics",MENU_OPCION_NORMAL,menu_about_statistics,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'s');

                menu_add_item_menu(array_menu_about,"C~~ompile info",MENU_OPCION_NORMAL,menu_about_compile_info,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'o');

		menu_add_item_menu(array_menu_about,"R~~unning info",MENU_OPCION_NORMAL,menu_about_running_info,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'u');

                menu_add_item_menu(array_menu_about,"Lic~~ense",MENU_OPCION_NORMAL,menu_about_license,NULL);
		menu_add_item_menu_shortcut(array_menu_about,'e');

								menu_add_item_menu(array_menu_about,"Motorola Core License",MENU_OPCION_NORMAL,menu_about_license_motorola_core,NULL);


                menu_add_item_menu(array_menu_about,"",MENU_OPCION_SEPARADOR,NULL,NULL);

                menu_add_ESC_item(array_menu_about);

                retorno_menu=menu_dibuja_menu(&about_opcion_seleccionada,&item_seleccionado,array_menu_about,"Help" );

		cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}


//menu tape settings
void menu_settings_tape(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_settings_tape;
	menu_item item_seleccionado;
	int retorno_menu;

        do {
                //char string_tape_load_shown[20],string_tape_load_inserted[50],string_tape_save_shown[20],string_tape_save_inserted[50];
		//char string_realtape_shown[23];

		menu_add_item_menu_inicial_format(&array_menu_settings_tape,MENU_OPCION_NORMAL,NULL,NULL,"--Standard Tape--");


		menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,menu_standard_to_real_tape_fallback,NULL,"Fa~~llback to real tape: %s",(standard_to_real_tape_fallback.v ? "Yes" : "No") );
		menu_add_item_menu_shortcut(array_menu_settings_tape,'l');
		menu_add_item_menu_tooltip(array_menu_settings_tape,"If this standard tape is detected as real tape, reinsert tape as real tape");
		menu_add_item_menu_ayuda(array_menu_settings_tape,"While loading the standard tape, if a custom loading routine is detected, "
					"the tape will be ejected from standard tape and inserted it as real tape. If autoload tape is enabled, "
					"the machine will be resetted and loaded the tape from the beginning");


		menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,menu_tape_any_flag,NULL,"A~~ny flag loading: %s", (tape_any_flag_loading.v==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_settings_tape,'n');
		menu_add_item_menu_tooltip(array_menu_settings_tape,"Enables tape load routine to load without knowing block flag");
		menu_add_item_menu_ayuda(array_menu_settings_tape,"Enables tape load routine to load without knowing block flag. You must enable it on Tape Copy programs");



                //menu_add_item_menu(array_menu_settings_tape,"",MENU_OPCION_SEPARADOR,NULL,NULL);


			menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,menu_tape_simulate_real_load,NULL,"~~Simulate real load: %s", (tape_loading_simulate.v==1 ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_settings_tape,'s');
			menu_add_item_menu_tooltip(array_menu_settings_tape,"Simulate sound and loading stripes");
			menu_add_item_menu_ayuda(array_menu_settings_tape,"Simulate sound and loading stripes. You can skip simulation pressing any key (and the data is loaded)");

			menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,menu_tape_simulate_real_load_fast,menu_tape_simulate_real_load_cond,"Fast Simulate real load: %s", (tape_loading_simulate_fast.v==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_settings_tape,"Simulate sound and loading stripes at faster speed");
                        menu_add_item_menu_ayuda(array_menu_settings_tape,"Simulate sound and loading stripes at faster speed");


        	        menu_add_item_menu(array_menu_settings_tape,"",MENU_OPCION_SEPARADOR,NULL,NULL);


		menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,NULL,NULL,"--Input Real Tape--");



		menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,menu_realtape_loading_sound,NULL,"Loading sound: %s", (realtape_loading_sound.v==1 ? "Yes" : "No"));
		menu_add_item_menu_tooltip(array_menu_settings_tape,"Enable loading sound");
		menu_add_item_menu_ayuda(array_menu_settings_tape,"Enable loading sound. With sound disabled, the tape is also loaded");



		menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,menu_realtape_volumen,NULL,"Volume bit 1 range: %s%d",(realtape_volumen>0 ? "+" : ""),realtape_volumen);
		menu_add_item_menu_tooltip(array_menu_settings_tape,"Volume bit 1 starting range value");
		menu_add_item_menu_ayuda(array_menu_settings_tape,"The input audio value read (considering range from -128 to +127) is treated "
					"normally as 1 if the value is in range 0...+127, and 0 if it is in range -127...-1. This setting "
					"increases this 0 (of range 0...+127) to consider it is a bit 1. I have found this value is better to be 0 "
					"on Spectrum, and 2 on ZX80/81");



		menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,menu_realtape_wave_offset,NULL,"Level Offset: %d",realtape_wave_offset);
		menu_add_item_menu_tooltip(array_menu_settings_tape,"Apply offset to sound value read");
		menu_add_item_menu_ayuda(array_menu_settings_tape,"Indicates some value (positive or negative) to sum to the raw value read "
					"(considering range from -128 to +127) to the input audio value read");

		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_settings_tape,MENU_OPCION_NORMAL,menu_realtape_accelerate_loaders,NULL,"A~~ccelerate loaders: %s",
				(accelerate_loaders.v==1 ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_settings_tape,'c');
			menu_add_item_menu_tooltip(array_menu_settings_tape,"Set top speed setting when loading a real tape");
			menu_add_item_menu_ayuda(array_menu_settings_tape,"Set top speed setting when loading a real tape");
		}





                menu_add_item_menu(array_menu_settings_tape,"",MENU_OPCION_SEPARADOR,NULL,NULL);




                //menu_add_item_menu(array_menu_settings_tape,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_settings_tape);

                retorno_menu=menu_dibuja_menu(&settings_tape_opcion_seleccionada,&item_seleccionado,array_menu_settings_tape,"Tape Settings" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
			//llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
	                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
        	        }
		}

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC);


}


//menu storage settings
void menu_settings_storage(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_settings_storage;
        menu_item item_seleccionado;
        int retorno_menu;
        do {

                char string_spi_flash_file_shown[13]; //,string_mmc_file_shown[13];



                menu_add_item_menu_inicial_format(&array_menu_settings_storage,MENU_OPCION_NORMAL,menu_tape_autoloadtape,NULL,"~~Autoload medium: %s", (noautoload.v==0 ? "On" : "Off"));
                            menu_add_item_menu_shortcut(array_menu_settings_storage,'a');

                            menu_add_item_menu_tooltip(array_menu_settings_storage,"Autoload medium and set machine");
                            menu_add_item_menu_ayuda(array_menu_settings_storage,"This option first change to the machine that handles the medium file type selected (tape, cartridge, etc), resets it, set some default machine values, and then, it sends "
                                                            "a LOAD sentence to load the medium\n"
                                                            "Note: The machine is changed only using smartload. Inserting a medium only resets the machine but does not change it");


                            menu_add_item_menu_format(array_menu_settings_storage,MENU_OPCION_NORMAL,menu_tape_autoselectfileopt,NULL,"A~~utoselect medium opts: %s", (autoselect_snaptape_options.v==1 ? "On" : "Off"));
                            menu_add_item_menu_shortcut(array_menu_settings_storage,'u');
                            menu_add_item_menu_tooltip(array_menu_settings_storage,"Detect options for the selected medium file and the needed machine");
                            menu_add_item_menu_ayuda(array_menu_settings_storage,"The emulator uses a database for different included programs "
                                            "(and some other not included) and reads .config files to select emulator settings and the needed machine "
                                            "to run them. If you disable this, the database nor the .config files are read");


							if (!MACHINE_IS_Z88) {
								menu_add_item_menu_format(array_menu_settings_storage,MENU_OPCION_NORMAL,menu_settings_tape,NULL,"~~Tape");
								menu_add_item_menu_shortcut(array_menu_settings_storage,'t');
						}


						if (MACHINE_IS_ZXUNO) {
						menu_add_item_menu(array_menu_settings_storage,"",MENU_OPCION_SEPARADOR,NULL,NULL);
										if (zxuno_flash_spi_name[0]==0) sprintf (string_spi_flash_file_shown,"Default");
										else menu_tape_settings_trunc_name(zxuno_flash_spi_name,string_spi_flash_file_shown,13);

										menu_add_item_menu_format(array_menu_settings_storage,MENU_OPCION_NORMAL,menu_zxuno_spi_flash_file,NULL,"ZX-Uno ~~SPI File: %s",string_spi_flash_file_shown);
										menu_add_item_menu_shortcut(array_menu_settings_storage,'s');
										menu_add_item_menu_tooltip(array_menu_settings_storage,"File used for the ZX-Uno SPI Flash");
										menu_add_item_menu_ayuda(array_menu_settings_storage,"File used for the ZX-Uno SPI Flash");


										menu_add_item_menu_format(array_menu_settings_storage,MENU_OPCION_NORMAL,menu_zxuno_spi_write_enable,NULL,"ZX-Uno SPI Disk ~~Write: %s",
										(zxuno_flash_write_to_disk_enable.v ? "Yes" : "No") );
										menu_add_item_menu_shortcut(array_menu_settings_storage,'w');
										menu_add_item_menu_tooltip(array_menu_settings_storage,"Tells if ZX-Uno SPI Flash writes are saved to disk");
										menu_add_item_menu_ayuda(array_menu_settings_storage,"Tells if ZX-Uno SPI Flash writes are saved to disk. "
																						"When you enable it, all previous changes (before enable it and since machine boot) and "
																						"future changes made to spi flash will be saved to disk.\n"
																						"Note: all writing operations to SPI Flash are always saved to internal memory, but this setting "
																						"tells if these changes are written to disk or not."
																						);
						}

                menu_add_item_menu(array_menu_settings_storage,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_settings_storage,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
                menu_add_ESC_item(array_menu_settings_storage);

                retorno_menu=menu_dibuja_menu(&settings_storage_opcion_seleccionada,&item_seleccionado,array_menu_settings_storage,"Storage Settings" );

                cls_menu_overlay();
                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
                                cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}



//menu display settings
void menu_settings_display(MENU_ITEM_PARAMETERS)
{

	menu_item *array_menu_settings_display;
	menu_item item_seleccionado;
	int retorno_menu;
	do {

		//char string_aux[50],string_aux2[50],emulate_zx8081_disp[50],string_arttext[50],string_aaslow[50],emulate_zx8081_thres[50],string_arttext_threshold[50];
		char buffer_string[50];


                char string_vofile_shown[10];
                menu_tape_settings_trunc_name(vofilename,string_vofile_shown,10);


	                menu_add_item_menu_inicial_format(&array_menu_settings_display,MENU_OPCION_NORMAL,menu_vofile,NULL,"Video out to file: %s",string_vofile_shown);
        	        menu_add_item_menu_tooltip(array_menu_settings_display,"Saves the video output to a file");
                	menu_add_item_menu_ayuda(array_menu_settings_display,"The generated file have raw format. You can see the file parameters "
					"on the console enabling verbose debug level to 2 minimum.\n"
					"Note: Gigascreen, Interlaced effects or menu windows are not saved to file."

					);

			if (menu_vofile_cond() ) {
				menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_vofile_fps,menu_vofile_cond,"FPS Video file: %d",50/vofile_fps);
	        	        menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_vofile_insert,menu_vofile_cond,"Video file inserted: %s",(vofile_inserted.v ? "Yes" : "No" ));
			}

			else {

                	  menu_add_item_menu(array_menu_settings_display,"",MENU_OPCION_SEPARADOR,NULL,NULL);
			}



                if (!MACHINE_IS_Z88) {


                        menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_autodetect_rainbow,NULL,"Autodetect Real Video: %s",(autodetect_rainbow.v==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_settings_display,"Autodetect the need to enable Real Video");
                        menu_add_item_menu_ayuda(array_menu_settings_display,"This option detects whenever is needed to enable Real Video. "
                                        "On Spectrum, it detects the reading of idle bus or repeated border changes. "
                                        "On ZX80/81, it detects the I register on a non-normal value when executing video display"
                                        );
                }




			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_rainbow,menu_display_rainbow_cond,"~~Real Video: %s",(rainbow_enabled.v==1 ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_settings_display,'r');

			menu_add_item_menu_tooltip(array_menu_settings_display,"Enable Real Video. Enabling it makes display as a real machine");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Real Video makes display works as in the real machine. It uses a bit more CPU than disabling it.\n\n"
					"On Spectrum, display is drawn every scanline. "
					"It enables hi-res colour (rainbow) on the screen and on the border, Gigascreen, Interlaced, ULAplus, Spectra, Timex Video, snow effect, idle bus reading and some other advanced features. "
					"Also enables all the Inves effects.\n"
					"Disabling it, the screen is drawn once per frame (1/50) and the previous effects "
					"are not supported.\n\n"
					"On ZX80/ZX81, enables hi-res display and loading/saving stripes on the screen, and the screen is drawn every scanline.\n"
					"By disabling it, the screen is drawn once per frame, no hi-res display, and only text mode is supported.\n\n"
					"On Z88, display is drawn the same way as disabling it; it is only used when enabling Video out to file.\n\n"
					"Real Video can be enabled on all the video drivers, but on curses, stdout and simpletext (in Spectrum and Z88 machines), the display drawn is the same "
					"as on non-Real Video, but you can have idle bus support on these drivers. "
					"Curses, stdout and simpletext drivers on ZX80/81 machines do have Real Video display."
					);

		if (MACHINE_IS_CPC) {
				if (cpc_forzar_modo_video.v==0)
					menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_cpc_force_mode,NULL,"Force Video Mode: No");
				else
					menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_cpc_force_mode,NULL,"Force Video Mode: %d",
						cpc_forzar_modo_video_modo);
		}


		if (!MACHINE_IS_Z88) {


			if (menu_cond_realvideo() ) {
				menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_interlace,menu_cond_realvideo,"~~Interlaced mode: %s", (video_interlaced_mode.v==1 ? "On" : "Off"));
				menu_add_item_menu_shortcut(array_menu_settings_display,'i');
				menu_add_item_menu_tooltip(array_menu_settings_display,"Enable interlaced mode");
				menu_add_item_menu_ayuda(array_menu_settings_display,"Interlaced mode draws the screen like the machine on a real TV: "
					"Every odd frame, odd lines on TV are drawn; every even frame, even lines on TV are drawn. It can be used "
					"to emulate twice the vertical resolution of the machine (384) or simulate different colours. "
					"This effect is only emulated with vertical zoom multiple of two: 2,4,6... etc");


				if (video_interlaced_mode.v) {
					menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_interlace_scanlines,NULL,"S~~canlines: %s", (video_interlaced_scanlines.v==1 ? "On" : "Off"));
					menu_add_item_menu_shortcut(array_menu_settings_display,'c');
					menu_add_item_menu_tooltip(array_menu_settings_display,"Enable scanlines on interlaced mode");
					menu_add_item_menu_ayuda(array_menu_settings_display,"Scanlines draws odd lines a bit darker than even lines");
				}


				menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_gigascreen,NULL,"~~Gigascreen: %s",(gigascreen_enabled.v==1 ? "On" : "Off"));
				menu_add_item_menu_shortcut(array_menu_settings_display,'g');
				menu_add_item_menu_tooltip(array_menu_settings_display,"Enable gigascreen colours");
				menu_add_item_menu_ayuda(array_menu_settings_display,"Gigascreen enables more than 15 colours by combining pixels "
							"of even and odd frames. The total number of different colours is 102");



				if (menu_cond_spectrum()) {
					menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_snow_effect,NULL,"Snow effect support: %s", (snow_effect_enabled.v==1 ? "On" : "Off"));
					menu_add_item_menu_tooltip(array_menu_settings_display,"Enable snow effect on Spectrum");
					menu_add_item_menu_ayuda(array_menu_settings_display,"Snow effect is a bug on some Spectrum models "
						"(models except +2A and +3) that draws corrupted pixels when I register is pointed to "
						"slow RAM.");
						// Even on 48k models it resets the machine after some seconds drawing corrupted pixels");

					if (snow_effect_enabled.v==1) {
						menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_snow_effect_margin,NULL,"Snow effect threshold: %d",snow_effect_min_value);
					}
				}


				if (MACHINE_IS_INVES) {
					menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_inves_ula_bright_error,NULL,"Inves bright error: %s",(inves_ula_bright_error.v ? "Yes" : "No"));
					menu_add_item_menu_tooltip(array_menu_settings_display,"Emulate Inves oddity when black colour and change from bright 0 to bright 1");
					menu_add_item_menu_ayuda(array_menu_settings_display,"Emulate Inves oddity when black colour and change from bright 0 to bright 1");

				}



			}
		}

		//para stdout
#ifdef COMPILE_STDOUT
		if (menu_cond_stdout() ) {
			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_stdout_simpletext_automatic_redraw,NULL,"Stdout automatic redraw: %s", (stdout_simpletext_automatic_redraw.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_settings_display,"It enables automatic display redraw");
			menu_add_item_menu_ayuda(array_menu_settings_display,"It enables automatic display redraw");


			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_send_ansi,NULL,"Send ANSI Control Sequence: %s",(screen_text_accept_ansi==1 ? "On" : "Off"));

		}

#endif



		if (menu_cond_zx8081_realvideo()) {

		//z80_bit video_zx8081_estabilizador_imagen;

			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_estabilizador_imagen,menu_cond_zx8081_realvideo,"Horizontal stabilization: %s", (video_zx8081_estabilizador_imagen.v==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_settings_display,"Horizontal image stabilization");
                        menu_add_item_menu_ayuda(array_menu_settings_display,"Horizontal image stabilization. Usually enabled.");


			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_slow_adjust,menu_cond_zx8081_realvideo,"~~LNCTR video adjust:  %s", (video_zx8081_lnctr_adjust.v==1 ? "On" : "Off"));
			//l repetida con load screen, pero como esa es de spectrum, no coinciden
			menu_add_item_menu_shortcut(array_menu_settings_display,'l');
			menu_add_item_menu_tooltip(array_menu_settings_display,"LNCTR video adjust");
			menu_add_item_menu_ayuda(array_menu_settings_display,"LNCTR video adjust change sprite offset when drawing video images. "
				"If you see your hi-res image is not displayed well, try changing it");




        	        menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_x_offset,menu_cond_zx8081_realvideo,"Video x_offset: %d",offset_zx8081_t_coordx);
			menu_add_item_menu_tooltip(array_menu_settings_display,"Video horizontal image offset");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Video horizontal image offset, usually you don't need to change this");


			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_minimo_vsync,menu_cond_zx8081_realvideo,"Video min. vsync lenght: %d",minimo_duracion_vsync);
			menu_add_item_menu_tooltip(array_menu_settings_display,"Video minimum vsync lenght in t-states");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Video minimum vsync lenght in t-states");


                        menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_autodetect_wrx,NULL,"Autodetect WRX: %s",(autodetect_wrx.v==1 ? "On" : "Off"));
                        menu_add_item_menu_tooltip(array_menu_settings_display,"Autodetect the need to enable WRX mode on ZX80/81");
                        menu_add_item_menu_ayuda(array_menu_settings_display,"This option detects whenever is needed to enable WRX. "
                                                "On ZX80/81, it detects the I register on a non-normal value when executing video display. "
						"In some cases, chr$128 and udg modes are detected incorrectly as WRX");


	                menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_zx8081_wrx,menu_cond_zx8081_realvideo,"~~WRX: %s", (wrx_present.v ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_settings_display,'w');
			menu_add_item_menu_tooltip(array_menu_settings_display,"Enables WRX hi-res mode");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Enables WRX hi-res mode");



		}

		else {

			if (menu_cond_zx8081() ) {
				menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_emulate_fast_zx8081,menu_cond_zx8081_no_realvideo,"ZX80/81 detect fast mode: %s", (video_fast_mode_emulation.v==1 ? "On" : "Off"));
				menu_add_item_menu_tooltip(array_menu_settings_display,"Detect fast mode and simulate it, on non-realvideo mode");
				menu_add_item_menu_ayuda(array_menu_settings_display,"Detect fast mode and simulate it, on non-realvideo mode");
			}

		}

		if (MACHINE_IS_ZX8081) {

			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_autodetect_chroma81,NULL,"Autodetect Chroma81: %s",(autodetect_chroma81.v ? "Yes" : "No"));
			menu_add_item_menu_tooltip(array_menu_settings_display,"Autodetect Chroma81");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Detects when Chroma81 video mode is needed and enable it");


			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_chroma81,NULL,"Chro~~ma81 support: %s",(chroma81.v ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_settings_display,'m');
			menu_add_item_menu_tooltip(array_menu_settings_display,"Enables Chroma81 colour video mode");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Enables Chroma81 colour video mode");

		}


		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_ulaplus,NULL,"ULA~~plus support: %s",(ulaplus_presente.v ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_settings_display,'p');
			menu_add_item_menu_tooltip(array_menu_settings_display,"Enables ULAplus support");
			menu_add_item_menu_ayuda(array_menu_settings_display,"The following ULAplus modes are supported:\n"
						"Mode 1: Standard 256x192 64 colours\n"
						"Mode 3: Linear mode 128x96, 16 colours per pixel (radastan mode)\n"
						"Mode 5: Linear mode 256x96, 16 colours per pixel (ZEsarUX mode 0)\n"
						"Mode 7: Linear mode 128x192, 16 colours per pixel (ZEsarUX mode 1)\n"
						"Mode 9: Linear mode 256x192, 16 colours per pixel (ZEsarUX mode 2)\n"
			);



			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_timex_video,NULL,"~~Timex video support: %s",(timex_video_emulation.v ? "Yes" : "No"));
                        menu_add_item_menu_shortcut(array_menu_settings_display,'t');
                        menu_add_item_menu_tooltip(array_menu_settings_display,"Enables Timex Video modes");
                        menu_add_item_menu_ayuda(array_menu_settings_display,"The following Timex Video modes are emulated:\n"
						"Mode 0: Video data at address 16384 and 8x8 color attributes at address 22528 (like on ordinary Spectrum)\n"
						"Mode 1: Video data at address 24576 and 8x8 color attributes at address 30720\n"
						"Mode 2: Multicolor mode: video data at address 16384 and 8x1 color attributes at address 24576\n"
						"Mode 6: Hi-res mode 512x192, monochrome.");

			if (timex_video_emulation.v) {
				menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_timex_video_512192,NULL,"Timex Real 512x192: %s",(timex_mode_512192_real.v ? "Yes" : "No"));
				menu_add_item_menu_tooltip(array_menu_settings_display,"Selects between real 512x192 or scaled 256x192");
				menu_add_item_menu_ayuda(array_menu_settings_display,"Real 512x192 does not support scanline effects (it draws the display at once). "
							"If not enabled real, it draws scaled 256x192 but does support scanline effects");
			}



			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_spectra,NULL,"Sp~~ectra support: %s",(spectra_enabled.v ? "Yes" : "No"));
			menu_add_item_menu_shortcut(array_menu_settings_display,'e');
			menu_add_item_menu_tooltip(array_menu_settings_display,"Enables Spectra video modes");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Enables Spectra video modes. All video modes are fully emulated");


			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_spritechip,NULL,"~~ZGX Sprite Chip: %s",(spritechip_enabled.v ? "Yes" : "No") );
			menu_add_item_menu_shortcut(array_menu_settings_display,'z');
			menu_add_item_menu_tooltip(array_menu_settings_display,"Enables ZGX Sprite Chip");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Enables ZGX Sprite Chip");


		}




		if (MACHINE_IS_SPECTRUM && rainbow_enabled.v==0) {
	                menu_add_item_menu(array_menu_settings_display,"",MENU_OPCION_SEPARADOR,NULL,NULL);


			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_emulate_zx8081display_spec,menu_display_settings_disp_zx8081_spectrum,"ZX80/81 Display on Speccy: %s", (simulate_screen_zx8081.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_settings_display,"Simulates the resolution of ZX80/81 on the Spectrum");
			menu_add_item_menu_ayuda(array_menu_settings_display,"It makes the resolution of display on Spectrum like a ZX80/81, with no colour. "
					"This mode is not supported with real video enabled");



			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_emulate_zx8081_thres,menu_display_emulate_zx8081_cond,"Pixel threshold: %d",umbral_simulate_screen_zx8081);
			menu_add_item_menu_tooltip(array_menu_settings_display,"Pixel Threshold to draw black or white in a 4x4 rectangle, "
					   "when ZX80/81 Display on Speccy enabled");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Pixel Threshold to draw black or white in a 4x4 rectangle, "
					   "when ZX80/81 Display on Speccy enabled");


			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_refresca_sin_colores,NULL,"Colours enabled: %s",(scr_refresca_sin_colores.v ? "No" : "Yes"));
			menu_add_item_menu_tooltip(array_menu_settings_display,"Disables colours for Spectrum display");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Disables colours for Spectrum display");

		}


		if (menu_display_cursesstdout_cond() ) {
	                //solo en caso de curses o stdout
			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_arttext,menu_display_cursesstdout_cond,"Text artistic emulation: %s", (texto_artistico.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_settings_display,"Write different artistic characters for unknown 4x4 rectangles, "
					"on stdout and curses drivers");

			menu_add_item_menu_ayuda(array_menu_settings_display,"Write different artistic characters for unknown 4x4 rectangles, "
					"on curses, stdout and simpletext drivers. "
					"If disabled, unknown characters are written with ?");


			menu_add_item_menu_format(array_menu_settings_display,MENU_OPCION_NORMAL,menu_display_arttext_thres,menu_display_arttext_cond,"Pixel threshold: %d",umbral_arttext);
			menu_add_item_menu_tooltip(array_menu_settings_display,"Pixel Threshold to decide which artistic character write in a 4x4 rectangle, "
					"on curses, stdout and simpletext drivers with text artistic emulation enabled");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Pixel Threshold to decide which artistic character write in a 4x4 rectangle, "
					"on curses, stdout and simpletext drivers with text artistic emulation enabled");

		}


		if (menu_display_aa_cond() ) {

#ifdef COMPILE_AA
			sprintf (buffer_string,"Slow AAlib emulation: %s", (scraa_fast==0 ? "On" : "Off"));
#else
			sprintf (buffer_string,"Slow AAlib emulation: Off");
#endif
			menu_add_item_menu(array_menu_settings_display,buffer_string,MENU_OPCION_NORMAL,menu_display_slowaa,menu_display_aa_cond);

			menu_add_item_menu_tooltip(array_menu_settings_display,"Enable slow aalib emulation; slow is a little better");
			menu_add_item_menu_ayuda(array_menu_settings_display,"Enable slow aalib emulation; slow is a little better");

		}


                menu_add_item_menu(array_menu_settings_display,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_settings_display,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_settings_display);

                retorno_menu=menu_dibuja_menu(&settings_display_opcion_seleccionada,&item_seleccionado,array_menu_settings_display,"Display Settings" );

                cls_menu_overlay();

		//NOTA: no llamar por numero de opcion dado que hay opciones que ocultamos (relacionadas con real video)

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {

			//llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
	                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
        	        }
		}

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}


void menu_settings_snapshot(MENU_ITEM_PARAMETERS)
{

        menu_item *array_menu_settings_snapshot;
        menu_item item_seleccionado;
        int retorno_menu;

        do {




		menu_add_item_menu_inicial_format(&array_menu_settings_snapshot,MENU_OPCION_NORMAL,menu_snapshot_permitir_versiones_desconocidas,NULL,"Allow Unknown ZX versions: %s",(snap_zx_permitir_versiones_desconocidas.v ? "Yes" : "No"));
		menu_add_item_menu_tooltip(array_menu_settings_snapshot,"Allow loading ZX Snapshots of unknown versions");
		menu_add_item_menu_ayuda(array_menu_settings_snapshot,"This setting permits loading of ZX Snapshots files of unknown versions. "
					"It can be used to load snapshots saved on higher emulator versions than this one");


		menu_add_item_menu_format(array_menu_settings_snapshot,MENU_OPCION_NORMAL,menu_snapshot_save_version,NULL,"Save ZX Snapshot version: %d",snap_zx_version_save);
                menu_add_item_menu_tooltip(array_menu_settings_snapshot,"Decide which kind of .ZX version file is saved");
                menu_add_item_menu_ayuda(array_menu_settings_snapshot,"Version 1,2,3 works on ZEsarUX and ZXSpectr\n"
					"Version 4 works on ZEsarUX V1.3 and higher\n"
					"Version 5 works on ZEsarUX V2 and higher\n"
				);


                menu_add_item_menu(array_menu_settings_snapshot,"",MENU_OPCION_SEPARADOR,NULL,NULL);
		menu_add_ESC_item(array_menu_settings_snapshot);

                retorno_menu=menu_dibuja_menu(&settings_snapshot_opcion_seleccionada,&item_seleccionado,array_menu_settings_snapshot,"Snapshot Settings" );

                cls_menu_overlay();

                if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
                        }
                }

        } while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);




}

void menu_setting_ay_piano_grafico(MENU_ITEM_PARAMETERS)
{
	setting_mostrar_ay_piano_grafico.v ^=1;
}

void menu_audio_specdrum(MENU_ITEM_PARAMETERS)
{
	specdrum_enabled.v ^=1;
}


void menu_audio_beeper(MENU_ITEM_PARAMETERS)
{
	beeper_enabled.v ^=1;
}

void menu_settings_audio(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_settings_audio;
	menu_item item_seleccionado;
	int retorno_menu;

        do {

		menu_add_item_menu_inicial_format(&array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_volume,NULL,"Audio Output ~~Volume: %d %%", audiovolume);
		menu_add_item_menu_shortcut(array_menu_settings_audio,'v');

		menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_ay_chip,NULL,"~~AY Chip: %s", (ay_chip_present.v==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_settings_audio,'a');
		menu_add_item_menu_tooltip(array_menu_settings_audio,"Enable AY Chip on this machine");
		menu_add_item_menu_ayuda(array_menu_settings_audio,"It enables the AY Chip for the machine, by activating the following hardware:\n"
					"-Normal AY Chip for Spectrum\n"
					"-Fuller audio box for Spectrum\n"
					"-Quicksilva QS Sound board on ZX80/81\n"
					"-Bi-Pak ZON-X81 Sound on ZX80/81\n"
			);

		menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_turbosound,NULL,"~~Turbosound: %s",(turbosound_enabled.v==1 ? "On" : "Off"));

		menu_add_item_menu_shortcut(array_menu_settings_audio,'t');
		menu_add_item_menu_tooltip(array_menu_settings_audio,"Enable Turbosound");
		menu_add_item_menu_ayuda(array_menu_settings_audio,"Enable Turbosound");


		menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_ay_chip_autoenable,NULL,"Autoenable AY Chip: %s",(autoenable_ay_chip.v==1 ? "On" : "Off"));
		menu_add_item_menu_tooltip(array_menu_settings_audio,"Enable AY Chip automatically when it is needed");
		menu_add_item_menu_ayuda(array_menu_settings_audio,"This option is usefor for example on Spectrum 48k games that uses AY Chip "
					"and for some ZX80/81 games that also uses it (Bi-Pak ZON-X81, but not Quicksilva QS Sound board)");


		menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_envelopes,menu_cond_ay_chip,"AY ~~Envelopes: %s", (ay_envelopes_enabled.v==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_settings_audio,'e');
		menu_add_item_menu_tooltip(array_menu_settings_audio,"Enable or disable volume envelopes for the AY Chip");
		menu_add_item_menu_ayuda(array_menu_settings_audio,"Enable or disable volume envelopes for the AY Chip");

		menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_speech,menu_cond_ay_chip,"AY ~~Speech: %s", (ay_speech_enabled.v==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_settings_audio,'s');
		menu_add_item_menu_tooltip(array_menu_settings_audio,"Enable or disable AY Speech effects");
		menu_add_item_menu_ayuda(array_menu_settings_audio,"These effects are used, for example, in Chase H.Q.");





		if (si_complete_video_driver() ) {
			menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_setting_ay_piano_grafico,NULL,"Show AY ~~Piano: %s",
					(setting_mostrar_ay_piano_grafico.v ? "Graphic" : "Text") );
			menu_add_item_menu_shortcut(array_menu_settings_audio,'p');
			menu_add_item_menu_tooltip(array_menu_settings_audio,"Shows AY Piano menu with graphic or with text");
			menu_add_item_menu_ayuda(array_menu_settings_audio,"Shows AY Piano menu with graphic or with text");

		}


		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_specdrum,NULL,"Specdrum: %s",(specdrum_enabled.v ? "On" : "Off" ));
		}


                menu_add_item_menu(array_menu_settings_audio,"",MENU_OPCION_SEPARADOR,NULL,NULL);


		if (!MACHINE_IS_ZX8081) {

			menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_beeper,NULL,"Beeper: %s",(beeper_enabled.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_settings_audio,"Enable or disable beeper output");
			menu_add_item_menu_ayuda(array_menu_settings_audio,"Enable or disable beeper output");

		}



		if (MACHINE_IS_ZX8081) {
			//sound on zx80/81

			menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_zx8081_detect_vsync_sound,menu_cond_zx8081,"Detect VSYNC Sound: %s",(zx8081_detect_vsync_sound.v ? "Yes" : "No"));
			menu_add_item_menu_tooltip(array_menu_settings_audio,"Tries to detect when vsync sound is played. This feature is experimental");
			menu_add_item_menu_ayuda(array_menu_settings_audio,"Tries to detect when vsync sound is played. This feature is experimental");


			menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_sound_zx8081,menu_cond_zx8081,"VSYNC Sound on zx80/81: %s", (zx8081_vsync_sound.v==1 ? "On" : "Off"));
			menu_add_item_menu_tooltip(array_menu_settings_audio,"Enables or disables VSYNC sound on ZX80 and ZX81");
			menu_add_item_menu_ayuda(array_menu_settings_audio,"This method uses the VSYNC signal on the TV to make sound");


		}




		int mostrar_real_beeper=0;

		if (MACHINE_IS_ZX8081) {
			if (zx8081_vsync_sound.v) mostrar_real_beeper=1;
		}

		else {
			if (beeper_enabled.v) mostrar_real_beeper=1;
		}

		if (mostrar_real_beeper) {

			menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_beeper_real,NULL,"Real ~~Beeper: %s",(beeper_real_enabled==1 ? "On" : "Off"));
			menu_add_item_menu_shortcut(array_menu_settings_audio,'b');
			menu_add_item_menu_tooltip(array_menu_settings_audio,"Enable or disable Real Beeper enhanced sound. ");
			menu_add_item_menu_ayuda(array_menu_settings_audio,"Real beeper produces beeper sound more realistic but uses a bit more cpu. Needs beeper enabled (or vsync sound on zx80/81)");
		}


		if (MACHINE_IS_SPECTRUM) {
			menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_beep_filter_on_rom_save,NULL,"Audio filter on ROM SAVE: %s",(output_beep_filter_on_rom_save.v ? "Yes" : "No"));
			menu_add_item_menu_tooltip(array_menu_settings_audio,"Apply filter on ROM save routines");
			menu_add_item_menu_ayuda(array_menu_settings_audio,"It detects when on ROM save routines and alter audio output to use only "
					"the MIC bit of the FEH port");

//extern z80_bit output_beep_filter_alter_volume;
//extern char output_beep_filter_volume;

			if (output_beep_filter_on_rom_save.v) {
				menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_beep_alter_volume,NULL,"Alter beeper volume: %s",
				(output_beep_filter_alter_volume.v ? "Yes" : "No") );

				menu_add_item_menu_tooltip(array_menu_settings_audio,"Alter output beeper volume");
				menu_add_item_menu_ayuda(array_menu_settings_audio,"Alter output beeper volume. You can set to a maximum to "
							"send the audio to a real spectrum to load it");


				if (output_beep_filter_alter_volume.v) {
					menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_beep_volume,NULL,"Volume: %d",output_beep_filter_volume);
				}
			}

		}

		//if (si_complete_video_driver() ) {
			//menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_audio_espectro_sonido,NULL,"View ~~Waveform");
			//menu_add_item_menu_shortcut(array_menu_settings_audio,'w');
        //	        menu_add_item_menu(array_menu_settings_audio,"",MENU_OPCION_SEPARADOR,NULL,NULL);
		//}



		menu_add_item_menu(array_menu_settings_audio,"",MENU_OPCION_SEPARADOR,NULL,NULL);


		char string_aofile_shown[10];
		menu_tape_settings_trunc_name(aofilename,string_aofile_shown,10);
		menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_aofile,NULL,"Audio ~~out to file: %s",string_aofile_shown);
		menu_add_item_menu_shortcut(array_menu_settings_audio,'o');
		menu_add_item_menu_tooltip(array_menu_settings_audio,"Saves the generated sound to a file");
		menu_add_item_menu_ayuda(array_menu_settings_audio,"You can save .raw format and if compiled with sndfile, to .wav format. "
					"You can see the file parameters on the console enabling verbose debug level to 2 minimum");



		menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_aofile_insert,menu_aofile_cond,"Audio file ~~inserted: %s",(aofile_inserted.v ? "Yes" : "No" ));
		menu_add_item_menu_shortcut(array_menu_settings_audio,'i');


                menu_add_item_menu_format(array_menu_settings_audio,MENU_OPCION_NORMAL,menu_change_audio_driver,NULL,"Change Audio Driver");


                menu_add_item_menu(array_menu_settings_audio,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                //menu_add_item_menu(array_menu_settings_audio,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_settings_audio);

                retorno_menu=menu_dibuja_menu(&settings_audio_opcion_seleccionada,&item_seleccionado,array_menu_settings_audio,"Audio Settings" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
	                //llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
	                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
        	        }
		}

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}


void menu_debug_configuration_remoteproto(MENU_ITEM_PARAMETERS)
{
	if (remote_protocol_enabled.v) {
		end_remote_protocol();
		remote_protocol_enabled.v=0;
	}

	else {
		remote_protocol_enabled.v=1;
		init_remote_protocol();
	}
}

void menu_debug_configuration_remoteproto_port(MENU_ITEM_PARAMETERS)
{
	char string_port[6];
	int port;

	sprintf (string_port,"%d",remote_protocol_port);

	menu_ventana_scanf("Port",string_port,6);

	if (string_port[0]==0) return;

	else {
			port=parse_string_to_number(string_port);

			if (port<1 || port>65535) {
								debug_printf (VERBOSE_ERR,"Invalid port %d",port);
								return;
			}


			end_remote_protocol();
			remote_protocol_port=port;
			init_remote_protocol();
	}

}

void menu_hardware_debug_port(MENU_ITEM_PARAMETERS)
{
	hardware_debug_port.v ^=1;
}

//menu debug settings
void menu_settings_debug(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_settings_debug;
        menu_item item_seleccionado;
	int retorno_menu;
        do {




		menu_add_item_menu_inicial_format(&array_menu_settings_debug,MENU_OPCION_NORMAL,menu_debug_registers_console,NULL,"Show r~~egisters in console: %s",(debug_registers==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_settings_debug,'e');

		menu_add_item_menu_format(array_menu_settings_debug,MENU_OPCION_NORMAL,menu_debug_verbose,NULL,"Verbose ~~level: %d",verbose_level);
		menu_add_item_menu_shortcut(array_menu_settings_debug,'l');

		menu_add_item_menu_format(array_menu_settings_debug,MENU_OPCION_NORMAL,menu_debug_configuration_stepover,NULL,"Step over interrupt: %s",(debug_core_evitamos_inter.v ? "Yes" : "No") );
		menu_add_item_menu_tooltip(array_menu_settings_debug,"Avoid step to step or continuous execution of nmi or maskable interrupt routines on debug cpu menu");
		menu_add_item_menu_ayuda(array_menu_settings_debug,"Avoid step to step or continuous execution of nmi or maskable interrupt routines on debug cpu menu");


menu_add_item_menu_format(array_menu_settings_debug,MENU_OPCION_NORMAL, menu_breakpoints_condition_behaviour,NULL,"Breakp. behaviour: %s",(debug_breakpoints_cond_behaviour.v ? "On Change" : "Always") );
		menu_add_item_menu_tooltip(array_menu_settings_debug,"Indicates whether breakpoints are fired always or only on change from false to true");
		menu_add_item_menu_ayuda(array_menu_settings_debug,"Indicates whether breakpoints are fired always or only on change from false to true");


#ifdef USE_PTHREADS
		menu_add_item_menu_format(array_menu_settings_debug,MENU_OPCION_NORMAL, menu_debug_configuration_remoteproto,NULL,"Remote protocol: %s",(remote_protocol_enabled.v ? "Enabled" : "Disabled") );
		menu_add_item_menu_tooltip(array_menu_settings_debug,"Enables or disables remote command protocol");
		menu_add_item_menu_ayuda(array_menu_settings_debug,"Enables or disables remote command protocol");

		if (remote_protocol_enabled.v) {
			menu_add_item_menu_format(array_menu_settings_debug,MENU_OPCION_NORMAL, menu_debug_configuration_remoteproto_port,NULL,"Remote protocol port: %d",remote_protocol_port );
			menu_add_item_menu_tooltip(array_menu_settings_debug,"Changes remote command protocol port");
			menu_add_item_menu_ayuda(array_menu_settings_debug,"Changes remote command protocol port");
		}

#endif


		menu_add_item_menu_format(array_menu_settings_debug,MENU_OPCION_NORMAL, menu_hardware_debug_port,NULL,"Hardware debug ports: %s",(hardware_debug_port.v ? "Yes" : "No") );
		menu_add_item_menu_tooltip(array_menu_settings_debug,"If hardware debug ports are enabled");
		menu_add_item_menu_ayuda(array_menu_settings_debug,"It shows a ASCII character or a number on console sending some OUT sequence to ports. "
														"Read file docs/zesarux_zxi_registers.txt for more information");


                menu_add_item_menu(array_menu_settings_debug,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_settings_debug,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_settings_debug);

                retorno_menu=menu_dibuja_menu(&settings_debug_opcion_seleccionada,&item_seleccionado,array_menu_settings_debug,"Debug Settings" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
                        }
                }

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}

void menu_settings_config_file_save_config(MENU_ITEM_PARAMETERS)
{
	if (util_write_configfile()) {
		menu_generic_message("Save configuration","OK. Configuration saved");
	};
}

void menu_settings_config_file_save_on_exit(MENU_ITEM_PARAMETERS)
{
	if (save_configuration_file_on_exit.v) {
		if (menu_confirm_yesno_texto("Write configuration","To disable setting saveconf")==0) return;
		save_configuration_file_on_exit.v=0;
		util_write_configfile();
		menu_generic_message("Save configuration","OK. Configuration saved");
	}
	else save_configuration_file_on_exit.v=1;
}


//menu config_file settings
void menu_settings_config_file(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_settings_config_file;
        menu_item item_seleccionado;
	int retorno_menu;
        do {




		menu_add_item_menu_inicial_format(&array_menu_settings_config_file,MENU_OPCION_NORMAL,menu_settings_config_file_save_config,NULL,"~~Save configuration");
		menu_add_item_menu_shortcut(array_menu_settings_config_file,'s');
		menu_add_item_menu_tooltip(array_menu_settings_config_file,"Overwrite your configuration file with current settings");
		menu_add_item_menu_ayuda(array_menu_settings_config_file,"Overwrite your configuration file with current settings");


		menu_add_item_menu_format(array_menu_settings_config_file,MENU_OPCION_NORMAL,menu_settings_config_file_save_on_exit,NULL,"~~Autosave on exit: %s",(save_configuration_file_on_exit.v ? "Yes" : "No"));
		menu_add_item_menu_shortcut(array_menu_settings_config_file,'a');
		menu_add_item_menu_tooltip(array_menu_settings_config_file,"Auto save configuration on exit emulator");
		menu_add_item_menu_ayuda(array_menu_settings_config_file,"Auto save configuration on exit emulator and overwrite it. Note: not all settings are saved");



                menu_add_item_menu(array_menu_settings_config_file,"",MENU_OPCION_SEPARADOR,NULL,NULL);
                //menu_add_item_menu(array_menu_settings_config_file,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_settings_config_file);

                retorno_menu=menu_dibuja_menu(&settings_config_file_opcion_seleccionada,&item_seleccionado,array_menu_settings_config_file,"Configuration File" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
                        //llamamos por valor de funcion
                        if (item_seleccionado.menu_funcion!=NULL) {
                                //printf ("actuamos por funcion\n");
                                item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
                        }
                }

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);

}


//menu settings
void menu_settings(MENU_ITEM_PARAMETERS)
{
        menu_item *array_menu_settings;
	menu_item item_seleccionado;
	int retorno_menu;

        do {


		menu_add_item_menu_inicial_format(&array_menu_settings,MENU_OPCION_NORMAL,menu_settings_snapshot,NULL,"~~Snapshot");
		menu_add_item_menu_shortcut(array_menu_settings,'s');
		menu_add_item_menu_tooltip(array_menu_settings,"Snapshot settings");
		menu_add_item_menu_ayuda(array_menu_settings,"Snapshot settings");

		menu_add_item_menu(array_menu_settings,"S~~torage",MENU_OPCION_NORMAL,menu_settings_storage,NULL);
		menu_add_item_menu_shortcut(array_menu_settings,'t');
		menu_add_item_menu_tooltip(array_menu_settings,"Storage settings");
		menu_add_item_menu_ayuda(array_menu_settings,"Storage settings");

		menu_add_item_menu_format(array_menu_settings,MENU_OPCION_NORMAL,menu_settings_audio,NULL,"~~Audio");
		menu_add_item_menu_shortcut(array_menu_settings,'a');
		menu_add_item_menu_tooltip(array_menu_settings,"Audio settings");
		menu_add_item_menu_ayuda(array_menu_settings,"Audio settings");

		menu_add_item_menu(array_menu_settings,"~~Display",MENU_OPCION_NORMAL,menu_settings_display,NULL);
		menu_add_item_menu_shortcut(array_menu_settings,'d');
		menu_add_item_menu_tooltip(array_menu_settings,"Display settings");
		menu_add_item_menu_ayuda(array_menu_settings,"Display settings");

		menu_add_item_menu(array_menu_settings,"~~GUI",MENU_OPCION_NORMAL,menu_interface_settings,NULL);
		menu_add_item_menu_shortcut(array_menu_settings,'g');
		menu_add_item_menu_tooltip(array_menu_settings,"Settings for the GUI");
		menu_add_item_menu_ayuda(array_menu_settings,"These settings are related to the GUI interface");

		menu_add_item_menu_format(array_menu_settings,MENU_OPCION_NORMAL,menu_hardware_settings,NULL,"~~Hardware");
		menu_add_item_menu_shortcut(array_menu_settings,'h');
		menu_add_item_menu_tooltip(array_menu_settings,"Hardware options for the running machine");
		menu_add_item_menu_ayuda(array_menu_settings,"Select different options for the machine and change its behaviour");

		menu_add_item_menu(array_menu_settings,"D~~ebug",MENU_OPCION_NORMAL,menu_settings_debug,NULL);
		menu_add_item_menu_shortcut(array_menu_settings,'e');
		menu_add_item_menu_tooltip(array_menu_settings,"Debug settings");
		menu_add_item_menu_ayuda(array_menu_settings,"Debug settings");

		menu_add_item_menu(array_menu_settings,"~~Configuration file",MENU_OPCION_NORMAL,menu_settings_config_file,NULL);
		menu_add_item_menu_shortcut(array_menu_settings,'c');
		menu_add_item_menu_tooltip(array_menu_settings,"Configuration file");
		menu_add_item_menu_ayuda(array_menu_settings,"Configuration file");




  menu_add_item_menu(array_menu_settings,"",MENU_OPCION_SEPARADOR,NULL,NULL);


                //menu_add_item_menu(array_menu_settings,"ESC Back",MENU_OPCION_NORMAL|MENU_OPCION_ESC,NULL,NULL);
		menu_add_ESC_item(array_menu_settings);

                retorno_menu=menu_dibuja_menu(&settings_opcion_seleccionada,&item_seleccionado,array_menu_settings,"Settings" );

                cls_menu_overlay();

		if ((item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu>=0) {
	                //llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
	                        item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();
        	        }
		}

	} while ( (item_seleccionado.tipo_opcion&MENU_OPCION_ESC)==0 && retorno_menu!=MENU_RETORNO_ESC && !salir_todos_menus);
}





void menu_exit_emulator(MENU_ITEM_PARAMETERS)
{

	menu_reset_counters_tecla_repeticion();

	int salir=0;

	//Si quickexit, no preguntar
	if (quickexit.v) salir=1;

	else salir=menu_confirm_yesno("Exit ZEsarUX");

                        if (salir) {

				//menu_footer=0;

                                cls_menu_overlay();

                                reset_menu_overlay_function();
                                menu_abierto=0;

                                if (autosave_snapshot_on_exit.v) autosave_snapshot();

                                end_emulator();

                        }
                        cls_menu_overlay();
}

int menu_tape_settings_cond(void)
{
	return !(MACHINE_IS_Z88);
}



void menu_inicio_bucle(void)
{

			int retorno_menu;

		menu_item *array_menu_principal;
		menu_item item_seleccionado;

		int salir_menu=0;


		do {

		if (strcmp(scr_driver_name,"xwindows")==0 || strcmp(scr_driver_name,"sdl")==0 || strcmp(scr_driver_name,"caca")==0 || strcmp(scr_driver_name,"fbdev")==0 || strcmp(scr_driver_name,"cocoa")==0 || strcmp(scr_driver_name,"curses")==0) f_functions=1;
		else f_functions=0;


		menu_add_item_menu_inicial(&array_menu_principal,"~~Smart load",MENU_OPCION_NORMAL,menu_quickload,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'s');
                menu_add_item_menu_tooltip(array_menu_principal,"Smart load tape, snapshot, Z88 memory card or Timex Cartridge");
                menu_add_item_menu_ayuda(array_menu_principal,"This option loads the file depending on its type: \n"
					"-Binary tapes are inserted as standard tapes and loaded quickly\n"
					"-Audio tapes are loaded as real tapes\n"
					"-Snapshots are loaded at once\n"
					"-Timex Cartridges are inserted on the machine and you should do a reset to run the cartridge\n"
					"-Memory cards on Z88 are inserted on the machine\n\n"
					"Note: Tapes will be autoloaded if the autoload setting is on (by default)"

					);

		menu_add_item_menu(array_menu_principal,"~~Machine",MENU_OPCION_NORMAL,menu_machine_selection,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'m');
		menu_add_item_menu_tooltip(array_menu_principal,"Change active machine");
		menu_add_item_menu_ayuda(array_menu_principal,"You can switch to another machine. It also resets the machine");

		menu_add_item_menu(array_menu_principal,"S~~napshot",MENU_OPCION_NORMAL,menu_snapshot,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'n');
		menu_add_item_menu_tooltip(array_menu_principal,"Load or save snapshots");
		menu_add_item_menu_ayuda(array_menu_principal,"Load or save different snapshot images. Snapshot images are loaded or saved at once");

		menu_add_item_menu_format(array_menu_principal,MENU_OPCION_NORMAL,menu_storage_settings,NULL,"S~~torage");
		menu_add_item_menu_shortcut(array_menu_principal,'t');
		menu_add_item_menu_tooltip(array_menu_principal,"Select storage mediums, like tape, MMC, IDE, etc");
		menu_add_item_menu_ayuda(array_menu_principal,"Select storage mediums, like tape, MMC, IDE, etc");

		/*menu_add_item_menu(array_menu_principal,"~~Hardware Settings",MENU_OPCION_NORMAL,menu_hardware_settings,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'h');
		menu_add_item_menu_tooltip(array_menu_principal,"Hardware options for the running machine");
		menu_add_item_menu_ayuda(array_menu_principal,"Select different options for the machine and change its behaviour");*/

		//menu_add_item_menu(array_menu_principal,"~~Audio Settings",MENU_OPCION_NORMAL,menu_audio_settings,NULL);
		menu_add_item_menu(array_menu_principal,"~~Audio",MENU_OPCION_NORMAL,menu_audio_settings,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'a');
		menu_add_item_menu_tooltip(array_menu_principal,"Audio related actions");
		menu_add_item_menu_ayuda(array_menu_principal,"Audio related actions");


		menu_add_item_menu(array_menu_principal,"~~Display",MENU_OPCION_NORMAL,menu_display_settings,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'d');
		menu_add_item_menu_tooltip(array_menu_principal,"Display related actions");
		menu_add_item_menu_ayuda(array_menu_principal,"Display related actions");

		menu_add_item_menu(array_menu_principal,"D~~ebug",MENU_OPCION_NORMAL,menu_debug_settings,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'e');
		menu_add_item_menu_tooltip(array_menu_principal,"Debug tools");
		menu_add_item_menu_ayuda(array_menu_principal,"Tools to debug the machine");

		/*menu_add_item_menu(array_menu_principal,"~~GUI Settings",MENU_OPCION_NORMAL,menu_interface_settings,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'g');
		menu_add_item_menu_tooltip(array_menu_principal,"Settings for the GUI");
		menu_add_item_menu_ayuda(array_menu_principal,"These settings are related to the GUI interface");*/


		menu_add_item_menu(array_menu_principal,"Sett~~ings",MENU_OPCION_NORMAL,menu_settings,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'i');
		menu_add_item_menu_tooltip(array_menu_principal,"General Settings");
		menu_add_item_menu_ayuda(array_menu_principal,"General Settings");

		//menu_add_item_menu(array_menu_principal,"",MENU_OPCION_SEPARADOR,NULL,NULL);

		/*menu_add_item_menu_format(array_menu_principal,MENU_OPCION_NORMAL,NULL,NULL,"M~~ultitask menu: %s", (menu_multitarea==1 ? "On" : "Off"));
		menu_add_item_menu_shortcut(array_menu_principal,'u');
		menu_add_item_menu_tooltip(array_menu_principal,"Enable menu with multitask");
		menu_add_item_menu_ayuda(array_menu_principal,"Setting multitask on makes the emulation does not stop when the menu is active");*/

                /*menu_add_item_menu_format(array_menu_principal,MENU_OPCION_NORMAL,menu_cpu_speed,NULL,"Emulator S~~peed: %d%%",porcentaje_velocidad_emulador);
		menu_add_item_menu_shortcut(array_menu_principal,'p');
		menu_add_item_menu_tooltip(array_menu_principal,"Change the emulator Speed");
		menu_add_item_menu_ayuda(array_menu_principal,"Changes the emulator speed by setting a different interval between display frames. "
								"Also changes audio frequency");*/


		menu_add_item_menu(array_menu_principal,"",MENU_OPCION_SEPARADOR,NULL,NULL);
		menu_add_item_menu(array_menu_principal,"He~~lp...",MENU_OPCION_NORMAL,menu_about,NULL);
		menu_add_item_menu_shortcut(array_menu_principal,'l');
		menu_add_item_menu_tooltip(array_menu_principal,"Help menu");
		menu_add_item_menu_ayuda(array_menu_principal,"Some help and related files");




		//menu_add_item_menu_format(array_menu_principal,MENU_OPCION_NORMAL,NULL,NULL,"%sBack to the emulator",(f_functions==1 ? "F2  ": "") );

		menu_add_ESC_item(array_menu_principal);

		//menu_add_item_menu_format(array_menu_principal,MENU_OPCION_NORMAL,NULL,NULL,"%sBack to the emulator",(f_functions==1 ? "F2  ": "") );
		//menu_add_item_menu_tooltip(array_menu_principal,"Back to the emulator");
		//menu_add_item_menu_ayuda(array_menu_principal,"Close menu and go back to the emulator");

		menu_add_item_menu_format(array_menu_principal,MENU_OPCION_NORMAL,NULL,NULL,"%sExit emulator",(f_functions==1 ? "F10 ": "") );
		menu_add_item_menu_tooltip(array_menu_principal,"Exit emulator");
		menu_add_item_menu_ayuda(array_menu_principal,"Exit emulator");


		retorno_menu=menu_dibuja_menu(&menu_inicio_opcion_seleccionada,&item_seleccionado,array_menu_principal,"ZEsarUX v." EMULATOR_VERSION );

		//printf ("Opcion seleccionada: %d\n",menu_inicio_opcion_seleccionada);
		//printf ("Tipo opcion: %d\n",item_seleccionado.tipo_opcion);
		//printf ("Retorno menu: %d\n",retorno_menu);

		cls_menu_overlay();

		//opcion 11 es F10 salir del emulador
		//if ( (retorno_menu!=MENU_RETORNO_ESC && retorno_menu!=MENU_RETORNO_F2) &&  (menu_inicio_opcion_seleccionada==15 || retorno_menu==MENU_RETORNO_F10)) {
		if ( (retorno_menu!=MENU_RETORNO_ESC) &&  (menu_inicio_opcion_seleccionada==11 || retorno_menu==MENU_RETORNO_F10)) {

			menu_exit_emulator(0);

	        }


		else if (retorno_menu>=0) {
			//llamamos por valor de funcion
        	        if (item_seleccionado.menu_funcion!=NULL) {
                	        //printf ("actuamos por funcion\n");
				//cls_menu_overlay();
                        	item_seleccionado.menu_funcion(item_seleccionado.valor_opcion);
				cls_menu_overlay();

				//si ha generado error, no salir
				if (if_pending_error_message) salir_todos_menus=0;
	                }

			else {
			//o por numero de opcion
				//printf ("actuamos por numero de opcion\n");
				switch (menu_inicio_opcion_seleccionada) {

					//8 es opcion multitarea
					/*case 8:
						menu_multitarea=menu_multitarea^1;
						if (menu_multitarea==0) {
							//audio_thread_finish();
							audio_playing.v=0;
						}
						timer_reset();
					break;*/
				}

			}
		}

                //printf ("Opcion seleccionada: %d\n",menu_inicio_opcion_seleccionada);
                //printf ("Tipo opcion: %d\n",item_seleccionado.tipo_opcion);
                //printf ("Retorno menu: %d\n",retorno_menu);

		//if (retorno_menu==MENU_RETORNO_F2) salir_menu=1;

		//opcion numero 10: ESC back

		if (retorno_menu!=MENU_RETORNO_ESC && menu_inicio_opcion_seleccionada==10) salir_menu=1;
		if (retorno_menu==MENU_RETORNO_ESC) salir_menu=1;

	} while (!salir_menu && !salir_todos_menus);

}

void menu_inicio_pre_retorno(void)
{
        //desactivar botones de acceso directo
        menu_button_quickload.v=0;
        menu_button_osdkeyboard.v=0;
        menu_button_exit_emulator.v=0;
	menu_event_drag_drop.v=0;
        menu_breakpoint_exception.v=0;
				menu_event_remote_protocol_enterstep.v=0;

        reset_menu_overlay_function();
        menu_abierto=0;

        timer_reset();

}


//menu principal
void menu_inicio(void)
{
	if (menu_desactivado.v) end_emulator();

	menu_contador_teclas_repeticion=CONTADOR_HASTA_REPETICION;


	//Resetear todas teclas. Por si esta el spool file activo. Conservando estado de tecla ESC pulsada o no
	z80_byte estado_ESC=puerto_especial1&1;
	reset_keyboard_ports();

	//estaba pulsado ESC
	if (!estado_ESC) puerto_especial1 &=(255-1);

	//Desactivar fire, por si esta disparador automatico
	joystick_release_fire();

	menu_espera_no_tecla();


        if (!strcmp(scr_driver_name,"stdout")) {
		//desactivar menu multitarea con stdout
		menu_multitarea=0;
        }

	//simpletext no soporta menu
        if (!strcmp(scr_driver_name,"simpletext")) {
		printf ("Can not open menu: simpletext video driver does not support menu.\n");
		menu_inicio_pre_retorno();
		return;
        }



	if (menu_multitarea==0) {
		audio_playing.v=0;
		//audio_thread_finish();
	}


	//quitar splash text por si acaso
	menu_splash_segundos=1;
	reset_splash_text();

	cls_menu_overlay();
        set_menu_overlay_function(normal_overlay_texto_menu);



	//Establecemos variable de salida de todos menus a 0
	salir_todos_menus=0;



	//char multitarea_opc[30];



	//Gestionar pulsaciones directas de teclado o joystick
	if (menu_button_quickload.v) {
		//Pulsado smartload
		//menu_button_quickload.v=0;

		//para evitar que entre con la pulsacion de teclas activa
		//menu_espera_no_tecla_con_repeticion();
		menu_espera_no_tecla();

		menu_quickload(0);
		cls_menu_overlay();
	}

	else if (menu_button_osdkeyboard.v) {
		menu_espera_no_tecla();
		menu_onscreen_keyboard(0);
		cls_menu_overlay();
        }



	else if (menu_button_exit_emulator.v) {
		//Pulsado salir del emulador
                //para evitar que entre con la pulsacion de teclas activa
                //menu_espera_no_tecla_con_repeticion();
                menu_espera_no_tecla();

                menu_exit_emulator(0);
                cls_menu_overlay();
	}

        else if (menu_event_drag_drop.v) {
							debug_printf(VERBOSE_INFO,"Received drag and drop event with file %s",quickload_file);
		//Entrado drag-drop de archivo
                //para evitar que entre con la pulsacion de teclas activa
                //menu_espera_no_tecla_con_repeticion();
                menu_espera_no_tecla();
		quickfile=quickload_file;

                if (quickload(quickload_file)) {
                        debug_printf (VERBOSE_ERR,"Unknown file format");

												//menu_generic_message("ERROR","Unknown file format");
                }
								menu_muestra_pending_error_message(); //Si se genera un error derivado del quickload
                cls_menu_overlay();
        }


	//ha saltado un breakpoint
	else if (menu_breakpoint_exception.v) {
                //menu_espera_no_tecla();
		menu_generic_message_format("Breakpoint","Catch Breakpoint: %s",catch_breakpoint_message);

		menu_debug_registers(0);
                cls_menu_overlay();


		//Y despues de un breakpoint hacer que aparezca el menu normal y no vuelva a la ejecucion
		menu_inicio_bucle();


	}

	else if (menu_event_remote_protocol_enterstep.v) {
		//Entrada
		menu_espera_no_tecla();

		remote_ack_enter_cpu_step.v=1; //Avisar que nos hemos enterado
		//Mientras no se salga del modo step to step del remote protocol
		while (menu_event_remote_protocol_enterstep.v) {
			sleep(1);
		}

		//Salida
		cls_menu_overlay();
	}

	else {

		//Cualquier otra cosa abrir menu normal
		menu_inicio_bucle();

	}


	//printf ("salir menu\n");


	//Volver
	menu_inicio_pre_retorno();



}


void set_splash_text(void)
{
	cls_menu_overlay();
	char texto_welcome[40];
	sprintf(texto_welcome," Welcome to ZEsarUX v." EMULATOR_VERSION " ");

	//centramos texto
	int x=16-strlen(texto_welcome)/2;
	if (x<0) x=0;

	menu_escribe_texto(x,2,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,texto_welcome);


        char texto_edition[40];
        sprintf(texto_edition," " EMULATOR_EDITION_NAME " ");

        //centramos texto
        x=16-strlen(texto_edition)/2;
        if (x<0) x=0;

        menu_escribe_texto(x,3,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,texto_edition);




	char texto_esc_menu[32];
	sprintf(texto_esc_menu," Press %s for menu ",openmenu_key_message);
	menu_escribe_texto(3,4,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,texto_esc_menu);

	set_menu_overlay_function(normal_overlay_texto_menu);
	menu_splash_text_active.v=1;
	menu_splash_segundos=4;


	//Enviar texto de bienvenida tambien a speech
	//stdout y simpletext no
	if (!strcmp(scr_driver_name,"stdout")) return;
	if (!strcmp(scr_driver_name,"simpletext")) return;

	textspeech_print_speech(texto_welcome);
	textspeech_print_speech(texto_esc_menu);

}

void reset_splash_text(void)
{
	if (menu_splash_text_active.v==1) {

		menu_splash_segundos--;
		if (menu_splash_segundos==0) {
			menu_splash_text_active.v=0;
			cls_menu_overlay();
			reset_menu_overlay_function();
			debug_printf (VERBOSE_DEBUG,"End splash text");

			//Quitamos el splash text pero dejamos el F5 menu abajo en el footer, hasta que algo borre ese mensaje
			//(por ejemplo que cargamos una cinta/snap con configuracion y genera mensaje en texto inverso en el footer)
			menu_footer_f5_menu();

		}
	}
}

#define FILESEL_X 1
#define FILESEL_Y 1
#define FILESEL_ANCHO 30
#define FILESEL_ALTO 22
#define FILESEL_ALTO_DIR FILESEL_ALTO-9
#define FILESEL_POS_FILTER FILESEL_ALTO-4
#define FILESEL_POS_LEYENDA FILESEL_ALTO-3
#define FILESEL_INICIO_DIR 4

void menu_filesel_print_filters(char *filtros[])
{
	//texto para mostrar filtros. darle bastante margen aunque no quepa en pantalla
	char buffer_filtros[200];


	char *f;

	int i,p;
	p=0;
	sprintf(buffer_filtros,"Filter: ");

	p=p+8;


        for (i=0;filtros[i];i++) {
                //si filtro es "", significa todo (*)

                f=filtros[i];
                if (f[0]==0) f="*";

                //copiamos
		//sprintf(&buffer_filtros[p],"*.%s ",f);
		sprintf(&buffer_filtros[p],"%s ",f);
		p=p+strlen(f)+1;

        }


	//Si texto filtros pasa del tope, rellenar con "..."
	if (p>FILESEL_ANCHO-2) {
		p=FILESEL_ANCHO-2;
		buffer_filtros[p-1]='.';
		buffer_filtros[p-2]='.';
		buffer_filtros[p-3]='.';
	}


	buffer_filtros[p]=0;


	//borramos primero con espacios
	//menu_escribe_texto_ventana(9,FILESEL_ALTO-3,0,7+8,"               ");
	menu_escribe_linea_opcion(FILESEL_POS_FILTER,-1,1,"               ");


	//y luego escribimos
	//menu_escribe_texto_ventana(1,FILESEL_ALTO-3,0,7+8,buffer_filtros);

	//si esta filesel_zona_pantalla=2, lo ponemos en otro color
	int activo=-1;
	if (filesel_zona_pantalla==2) activo=FILESEL_POS_FILTER;

	menu_escribe_linea_opcion(FILESEL_POS_FILTER,activo,1,buffer_filtros);
}

void menu_filesel_print_legend(void)
{
	menu_escribe_linea_opcion(FILESEL_POS_LEYENDA,-1,1,"TAB: Changes section");
}

filesel_item *menu_get_filesel_item(int index)
{
	filesel_item *p;

	p=primer_filesel_item;

	int i;

	for(i=0;i<index;i++) {
		p=p->next;
	}

	return p;

}

//obtiene linea a escribir con nombre de archivo + carpeta
void menu_filesel_print_file_get(char *buffer, char *s,unsigned char  d_type,unsigned int max_length_shown)
{
	unsigned int i;

        for (i=0;i<max_length_shown && (s[i])!=0;i++) {
                buffer[i]=s[i];
        }


        //si sobra espacio, rellenar con espacios
        for (;i<max_length_shown;i++) {
                buffer[i]=' ';
        }

        buffer[i]=0;


        //si no cabe, poner puntos suspensivos
        if (strlen(s)>max_length_shown) {
                buffer[i-1]='.';
                buffer[i-2]='.';
                buffer[i-3]='.';
        }

        //y si es un directorio (sin nombre nulo ni espacio), escribir "<dir>
	//nota: se envia nombre " " (un espacio) cuando se lista el directorio y sobran lineas en blanco al final

	int test_dir=1;

	if (s[0]==0) test_dir=0;
	if (s[0]==' ' && s[1]==0) test_dir=0;

	if (test_dir) {
	        if (get_file_type(d_type,s) == 2) {
        	        buffer[i-1]='>';
                	buffer[i-2]='r';
	                buffer[i-3]='i';
        	        buffer[i-4]='d';
                	buffer[i-5]='<';
	        }
	}


}

//escribe el nombre de archivo o carpeta
void menu_filesel_print_file(char *s,unsigned char  d_type,unsigned int max_length_shown,int y)
{
        //z80_byte papel=7+8;
        //z80_byte tinta=0;

	char buffer[PATH_MAX];

	int linea_seleccionada;

	linea_seleccionada=FILESEL_INICIO_DIR+filesel_linea_seleccionada;

	menu_filesel_print_file_get(buffer, s, d_type, max_length_shown);

	//si estamos en esa zona, destacar archivo seleccionado
	int activo=linea_seleccionada;

	if (filesel_zona_pantalla!=1) activo=-1;
	menu_escribe_linea_opcion(y,activo,1,buffer);

}


void menu_print_dir(int inicial)
{
	//printf ("\nmenu_print_dir\n");

	//escribir en ventana directorio de archivos

	//Para speech
	char texto_opcion_activa[100];
	//Asumimos por si acaso que no hay ninguna activa
	texto_opcion_activa[0]=0;



	filesel_item *p;
	int i;

	int mostrados_en_pantalla=FILESEL_ALTO_DIR;

	p=menu_get_filesel_item(inicial);

	for (i=0;i<mostrados_en_pantalla && p!=NULL;i++) {
		//printf ("file: %s\n",p->d_name);
		menu_filesel_print_file(p->d_name,p->d_type,FILESEL_ANCHO-2,FILESEL_Y+3+i);

		if (filesel_linea_seleccionada==i) {
			char buffer[50],buffer2[50];
			//primero borrar con espacios

			menu_escribe_texto_ventana(7,1,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,"                      ");


			strcpy(filesel_nombre_archivo_seleccionado,p->d_name);

			menu_tape_settings_trunc_name(filesel_nombre_archivo_seleccionado,buffer,22);
			sprintf (buffer2,"File: %s",buffer);
			menu_escribe_texto_ventana(1,1,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,buffer2);


				debug_printf (VERBOSE_DEBUG,"Selected: %s. filesel_zona_pantalla: %d",p->d_name,filesel_zona_pantalla);
				//Para speech
				//Si estamos en zona central del selector de archivos, decirlo
				if (filesel_zona_pantalla==1) {

	                                if (menu_active_item_primera_vez) {
						//menu_filesel_print_file_get(texto_opcion_activa,p->d_name,p->d_type,FILESEL_ANCHO-2);

        	                                sprintf (texto_opcion_activa,"Active item: %s %s",p->d_name,(get_file_type(p->d_type,p->d_name) == 2 ? "directory" : ""));
                	                        menu_active_item_primera_vez=0;
                        	        }

                                	else {
	                                        sprintf (texto_opcion_activa,"%s %s",p->d_name,(get_file_type(p->d_type,p->d_name) == 2 ? "directory" : ""));
        	                        }

				}


		}

		p=p->next;

        }



	//espacios al final.
        for (;i<mostrados_en_pantalla;i++) {
                //printf ("espacios\n");
                menu_filesel_print_file(" ",DT_REG,FILESEL_ANCHO-2,FILESEL_Y+3+i);
        }


	//Imprimir directorio actual
	//primero borrar con espacios

        menu_escribe_texto_ventana(14,0,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,"               ");


	char current_dir[PATH_MAX];
	char buffer_dir[50];
	char buffer3[50];
	getcwd(current_dir,PATH_MAX);

	menu_tape_settings_trunc_name(current_dir,buffer_dir,15);
	sprintf (buffer3,"Current dir: %s",buffer_dir);
	menu_escribe_texto_ventana(1,0,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,buffer3);


                if (texto_opcion_activa[0]!=0) {

			debug_printf (VERBOSE_DEBUG,"Send active line to speech: %s",texto_opcion_activa);
                        //Active item siempre quiero que se escuche

                        //Guardamos estado actual
                        int antes_menu_speech_tecla_pulsada=menu_speech_tecla_pulsada;
                        menu_speech_tecla_pulsada=0;

                        menu_textspeech_send_text(texto_opcion_activa);

                        //Restauro estado
                        //Pero si se ha pulsado tecla, no restaurar estado
                        //Esto sino provocaria que , por ejemplo, en la ventana de confirmar yes/no,
                        //se entra con menu_speech_tecla_pulsada=0, se pulsa tecla mientras se esta leyendo el item activo,
                        //y luego al salir de aqui, se pierde el valor que se habia metido (1) y se vuelve a poner el 0 del principio
                        //provocando que cada vez que se mueve el cursor, se relea la ventana entera
                        if (menu_speech_tecla_pulsada==0) menu_speech_tecla_pulsada=antes_menu_speech_tecla_pulsada;
                }



}

void menu_filesel_switch_filters(void)
{

	//si filtro inicial, ponemos el *.*
	if (filesel_filtros==filesel_filtros_iniciales)
		filesel_filtros=filtros_todos_archivos;

	//si filtro *.* , ponemos el filtro inicial
	else filesel_filtros=filesel_filtros_iniciales;

}

void menu_filesel_chdir(char *dir)
{
	chdir(dir);
}

char menu_minus_letra(char letra)
{
	if (letra>='A' && letra<='Z') letra=letra+('a'-'A');
	return letra;
}

void menu_filesel_localiza_letra(char letra)
{

	int i;
	filesel_item *p;
        p=primer_filesel_item;

	for (i=0;i<filesel_total_items;i++) {
		if (menu_minus_letra(p->d_name[0])>=menu_minus_letra(letra)) {
            		filesel_linea_seleccionada=0;
                	filesel_archivo_seleccionado=i;
			return;
		}


                p=p->next;
        }

}


int si_menu_filesel_no_mas_alla_ultimo_item(int linea)
{
	if (filesel_archivo_seleccionado+linea<filesel_total_items-1) return 1;
	return 0;
}

void menu_filesel_cursor_abajo(void)
{
//ver que no sea ultimo archivo
	if (si_menu_filesel_no_mas_alla_ultimo_item(filesel_linea_seleccionada)) {
                                        //if (filesel_archivo_seleccionado+filesel_linea_seleccionada<filesel_total_items-1)      {
                                                //ver si es final de pantalla
                                                if (filesel_linea_seleccionada==FILESEL_ALTO_DIR-1) {
                                                        filesel_archivo_seleccionado++;
                                                }
                                                else {
                                                        filesel_linea_seleccionada++;
                                                }
                                        }

}

void menu_filesel_cursor_arriba(void)
{
//ver que no sea primer archivo
                                        if (filesel_archivo_seleccionado+filesel_linea_seleccionada!=0) {
                                                //ver si es principio de pantalla
                                                if (filesel_linea_seleccionada==0) {
                                                        filesel_archivo_seleccionado--;
                                                }
                                                else {
                                                        filesel_linea_seleccionada--;
                                                }
                                        }
}



//liberar memoria usada por la lista de archivos
void menu_filesel_free_mem(void)
{

	filesel_item *aux;
        filesel_item *nextfree;


        aux=primer_filesel_item;

        //do {

	//puede que no haya ningun archivo, por tanto esto es NULL
	//sucede con las carpetas /home en macos por ejemplo
	while (aux!=NULL) {

                //printf ("Liberando %p : %s\n",aux,aux->d_name);
                nextfree=aux->next;
                free(aux);

                aux=nextfree;
        };
        //} while (aux!=NULL);

	//printf ("fin liberar filesel\n");


}


int menu_filesel_mkdir(char *directory)
{
#ifndef MINGW
     int tmpdirret=mkdir(directory,S_IRWXU);
#else
	int tmpdirret=mkdir(directory);
#endif

     if (tmpdirret!=0 && errno!=EEXIST) {
                  debug_printf (VERBOSE_ERR,"Error creating %s directory : %s",directory,strerror(errno) );
     }

	return tmpdirret;

}

void menu_filesel_exist_ESC(void)
{
                                                cls_menu_overlay();
                                                menu_espera_no_tecla();
                                                menu_filesel_chdir(filesel_directorio_inicial);
                                                menu_filesel_free_mem();
}

//Elimina la extension de un nombre
void menu_filesel_file_no_ext(char *origen, char *destino)
{

	//char *copiadestino;

	//copiadestino=destino;

	int j;

        j=strlen(origen);

	//buscamos desde el punto final

        for (;j>=0 && origen[j]!='.';j--);

	if (j==-1) {
		//no hay punto
		j=strlen(origen);
	}

	//printf ("posicion final: %d\n",j);


	//y copiamos
	for (;j>0;j--) {
		*destino=*origen;
		origen++;
		destino++;
	}

	//Y final de cadena
	*destino = 0;

	//printf ("archivo sin extension: %s\n",copiadestino);

}


//Devuelve 0 si ok

int menu_filesel_uncompress (char *archivo,char *tmpdir)
{

#define COMPRESSED_ZIP 1
#define COMPRESSED_GZ  2
#define COMPRESSED_TAR 3
#define COMPRESSED_RAR 4
  int compressed_type=0;

	//if ( strstr(archivo,".zip")!=NULL || strstr(archivo,".ZIP")!=NULL) {
	if ( !util_compare_file_extension(archivo,"zip") ) {
		debug_printf (VERBOSE_DEBUG,"Is a zip file");
		compressed_type=COMPRESSED_ZIP;
	}

        else if ( !util_compare_file_extension(archivo,"gz") ) {
                debug_printf (VERBOSE_DEBUG,"Is a gz file");
                compressed_type=COMPRESSED_GZ;
        }

        else if ( !util_compare_file_extension(archivo,"tar") ) {
                debug_printf (VERBOSE_DEBUG,"Is a tar file");
                compressed_type=COMPRESSED_TAR;
        }

        else if ( !util_compare_file_extension(archivo,"rar") ) {
                debug_printf (VERBOSE_DEBUG,"Is a rar file");
                compressed_type=COMPRESSED_RAR;
        }





//descomprimir creando carpeta TMPDIR_BASE/zipname

 //descomprimir
 sprintf (tmpdir,"%s/%s",get_tmpdir_base(),archivo);

 menu_filesel_mkdir(tmpdir);

 char uncompress_command[PATH_MAX];

 char uncompress_program[PATH_MAX];

		char archivo_no_ext[PATH_MAX];

switch (compressed_type) {

	case COMPRESSED_ZIP:
		//sprintf (uncompress_program,"/usr/bin/unzip");
 		//-n no sobreescribir
		//sprintf (uncompress_command,"unzip -n \"%s\" -d %s",archivo,tmpdir);

		sprintf (uncompress_program,"%s",external_tool_unzip);
 		//-n no sobreescribir
		sprintf (uncompress_command,"%s -n \"%s\" -d %s",external_tool_unzip,archivo,tmpdir);
	break;

        case COMPRESSED_GZ:
		menu_filesel_file_no_ext(archivo,archivo_no_ext);
                //sprintf (uncompress_program,"/bin/gunzip");
                //sprintf (uncompress_command,"gunzip -c \"%s\" > \"%s/%s\" ",archivo,tmpdir,archivo_no_ext);
                sprintf (uncompress_program,"%s",external_tool_gunzip);
                sprintf (uncompress_command,"%s -c \"%s\" > \"%s/%s\" ",external_tool_gunzip,archivo,tmpdir,archivo_no_ext);
        break;

        case COMPRESSED_TAR:
                //sprintf (uncompress_program,"/bin/tar");
                //sprintf (uncompress_command,"tar -xvf \"%s\" -C %s",archivo,tmpdir);
                sprintf (uncompress_program,"%s",external_tool_tar);
                sprintf (uncompress_command,"%s -xvf \"%s\" -C %s",external_tool_tar,archivo,tmpdir);
        break;

        case COMPRESSED_RAR:
                //sprintf (uncompress_program,"/usr/bin/unrar");
                //sprintf (uncompress_command,"unrar x -o+ \"%s\" %s",archivo,tmpdir);
                sprintf (uncompress_program,"%s",external_tool_unrar);
                sprintf (uncompress_command,"%s x -o+ \"%s\" %s",external_tool_unrar,archivo,tmpdir);
        break;




	default:
		debug_printf(VERBOSE_ERR,"Unknown compressed file");
		return 1;
	break;

}

 //unzip
 struct stat buf_stat;


 if (stat(uncompress_program, &buf_stat)!=0) {
debug_printf(VERBOSE_ERR,"Unable to find uncompress program: %s",uncompress_program);
return 1;


 }

debug_printf (VERBOSE_DEBUG,"Running %s",uncompress_command);

 if (system (uncompress_command)==-1) {
debug_printf (VERBOSE_DEBUG,"Error running command %s",uncompress_command);
return 1;
 }


return 0;

}

#define MENU_LAST_DIR_FILE_NAME "zesarux_last_dir.txt"
//escribir archivo que indique directorio anterior
void menu_filesel_write_file_last_dir(char *directorio_anterior)
{
	debug_printf (VERBOSE_DEBUG,"Writing temp file " MENU_LAST_DIR_FILE_NAME " to tell last directory before uncompress (%s)",directorio_anterior);


        FILE *ptr_lastdir;
        ptr_lastdir=fopen(MENU_LAST_DIR_FILE_NAME,"wb");
	if (ptr_lastdir!=NULL) {
	        fwrite(directorio_anterior,1,strlen(directorio_anterior),ptr_lastdir);
        	fclose(ptr_lastdir);
	}
}

//leer contenido de archivo que indique directorio anterior
//Devuelve 0 si OK. 1 si error
int menu_filesel_read_file_last_dir(char *directorio_anterior)
{

        FILE *ptr_lastdir;
        ptr_lastdir=fopen(MENU_LAST_DIR_FILE_NAME,"rb");

	if (ptr_lastdir==NULL) {
		debug_printf (VERBOSE_DEBUG,"Error opening " MENU_LAST_DIR_FILE_NAME);
                return 1;
        }


        int leidos=fread(directorio_anterior,1,PATH_MAX,ptr_lastdir);
        fclose(ptr_lastdir);

	if (leidos) {
		directorio_anterior[leidos]=0;
	}
	else {
		if (leidos==0) debug_printf (VERBOSE_DEBUG,"Error. Read 0 bytes from " MENU_LAST_DIR_FILE_NAME);
		if (leidos<0) debug_printf (VERBOSE_DEBUG,"Error reading from " MENU_LAST_DIR_FILE_NAME);
		return 1;
	}

	return 0;
}


void menu_textspeech_say_current_directory(void)
{

	//printf ("\nmenu_textspeech_say_current_directory\n");

        char current_dir[PATH_MAX];
        char buffer_texto[PATH_MAX+200];
        getcwd(current_dir,PATH_MAX);

        sprintf (buffer_texto,"Current directory: %s",current_dir);

	//Quiero que siempre se escuche
	menu_speech_tecla_pulsada=0;
	menu_textspeech_send_text(buffer_texto);
}

int si_mouse_zona_archivos(void)
{
	int inicio_y_dir=1+FILESEL_INICIO_DIR;

	if (menu_mouse_y>=inicio_y_dir && menu_mouse_y<inicio_y_dir+FILESEL_ALTO_DIR) return 1;
	return 0;
}

//Retorna 1 si seleccionado archivo. Retorna 0 si sale con ESC
//Si seleccionado archivo, lo guarda en variable *archivo
//Si sale con ESC, devuelve en menu_filesel_last_directory_seen ultimo directorio
int menu_filesel(char *titulo,char *filtros[],char *archivo)
{

	//En el caso de stdout es mucho mas simple
        if (!strcmp(scr_driver_name,"stdout")) {
		printf ("%s :\n",titulo);
		scrstdout_menu_print_speech_macro(titulo);
		scanf("%s",archivo);
		return 1;
        }



	menu_reset_counters_tecla_repeticion();

	int tecla;


	filesel_zona_pantalla=1;

	getcwd(filesel_directorio_inicial,PATH_MAX);


        //printf ("confirm\n");

        menu_espera_no_tecla();
        menu_dibuja_ventana(FILESEL_X,FILESEL_Y,FILESEL_ANCHO,FILESEL_ALTO,titulo);

	//guardamos filtros originales
	filesel_filtros_iniciales=filtros;



        filtros_todos_archivos[0]="";
        filtros_todos_archivos[1]=0;

	menu_escribe_texto_ventana(1,2,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,"Directory Contents:");
	filesel_filtros=filtros;

	filesel_item *item_seleccionado;

	int aux_pgdnup;
	//menu_active_item_primera_vez=1;

	//Decir directorio activo
	menu_textspeech_say_current_directory();

	do {
		menu_speech_tecla_pulsada=0;
		menu_active_item_primera_vez=1;
		filesel_linea_seleccionada=0;
		filesel_archivo_seleccionado=0;
		//leer todos archivos
		menu_filesel_print_filters(filesel_filtros);
		menu_filesel_print_legend();
		menu_filesel_readdir();

		int releer_directorio=0;


		//El menu_print_dir aqui no hace falta porque ya entrara en el switch (filesel_zona_pantalla) inicialmente cuando filesel_zona_pantalla vale 1
		//menu_print_dir(filesel_archivo_seleccionado);

		all_interlace_scr_refresca_pantalla();


		do {
			//printf ("\nReleer directorio\n");

		switch (filesel_zona_pantalla) {
			case 0:
				//zona superior de nombre de archivo
                                menu_print_dir(filesel_archivo_seleccionado);
                                //para que haga lectura del edit box
                                menu_speech_tecla_pulsada=0;

				tecla=menu_scanf(filesel_nombre_archivo_seleccionado,PATH_MAX,22,FILESEL_X+7,FILESEL_Y+2);

				if (tecla==15) {
					//printf ("TAB. siguiente seccion\n");
					menu_reset_counters_tecla_repeticion();
					filesel_zona_pantalla=1;
					//no releer todos archivos
					menu_speech_tecla_pulsada=1;
				}

				//ESC
                                if (tecla==2) {
                                                                        menu_filesel_exist_ESC();
                                                                        return 0;

				}

				if (tecla==13) {

					//Si es Windows y se escribe unidad: (ejemplo: "D:") hacer chdir
				int unidadwindows=0;
#ifdef MINGW
					if (filesel_nombre_archivo_seleccionado[0] &&
					filesel_nombre_archivo_seleccionado[1]==':' &&
					filesel_nombre_archivo_seleccionado[2]==0 )
					{
						debug_printf (VERBOSE_INFO,"%s is a Windows drive",filesel_nombre_archivo_seleccionado);
						unidadwindows=1;
					}
#endif


				//si es directorio, cambiamos
					struct stat buf_stat;
					int stat_valor;
					stat_valor=stat(filesel_nombre_archivo_seleccionado, &buf_stat);
					if (
						(stat_valor==0 && S_ISDIR(buf_stat.st_mode) ) ||
						(unidadwindows)
						) {
						debug_printf (VERBOSE_DEBUG,"%s Is a directory or windows drive. Change",filesel_nombre_archivo_seleccionado);
                                                menu_filesel_chdir(filesel_nombre_archivo_seleccionado);
						menu_filesel_free_mem();
                                                releer_directorio=1;
						filesel_zona_pantalla=1;

					        //Decir directorio activo
						//Esperar a liberar tecla si no la tecla invalida el speech
						menu_espera_no_tecla();
					        menu_textspeech_say_current_directory();


					}


					//sino, devolvemos nombre con path
					else {
                                                        cls_menu_overlay();
                                                        menu_espera_no_tecla();

                                                       //unimos directorio y nombre archivo. siempre que inicio != '/'
							if (filesel_nombre_archivo_seleccionado[0]!='/') {
                                                          getcwd(archivo,PATH_MAX);
                                                          sprintf(&archivo[strlen(archivo)],"/%s",filesel_nombre_archivo_seleccionado);
							}
							else sprintf(archivo,"%s",filesel_nombre_archivo_seleccionado);


                                                        menu_filesel_chdir(filesel_directorio_inicial);
							menu_filesel_free_mem();

							return menu_avisa_si_extension_no_habitual(filtros,archivo);

							//Volver con OK
                                                        //return 1;

					}
				}

				break;
			break;
			case 1:
				//zona selector de archivos

				debug_printf (VERBOSE_DEBUG,"Read directory. menu_speech_tecla_pulsada=%d",menu_speech_tecla_pulsada);
				menu_print_dir(filesel_archivo_seleccionado);
				//Para no releer todas las entradas
				menu_speech_tecla_pulsada=1;

		                all_interlace_scr_refresca_pantalla();
				menu_espera_tecla();
				tecla=menu_get_pressed_key();


				if (mouse_movido) {
					//printf ("mouse x: %d y: %d menu mouse x: %d y: %d\n",mouse_x,mouse_y,menu_mouse_x,menu_mouse_y);
					//printf ("ventana x %d y %d ancho %d alto %d\n",ventana_x,ventana_y,ventana_ancho,ventana_alto);
					if (si_menu_mouse_en_ventana() ) {
							//printf ("dentro ventana\n");
							//Ver en que zona esta
							int inicio_y_dir=1+FILESEL_INICIO_DIR;
							if (si_mouse_zona_archivos()) {
							//if (menu_mouse_y>=inicio_y_dir && menu_mouse_y<inicio_y_dir+FILESEL_ALTO_DIR) {
								//printf ("Dentro lista archivos\n");

								//Ver si linea dentro de rango
								int linea_final=menu_mouse_y-inicio_y_dir;

								//filesel_linea_seleccionada=menu_mouse_y-inicio_y_dir;

								if (si_menu_filesel_no_mas_alla_ultimo_item(linea_final-1)) {
									filesel_linea_seleccionada=linea_final;
									menu_speech_tecla_pulsada=1;
								}
								else {
									//printf ("Cursor mas alla del ultimo item\n");
								}

							}
							else if (menu_mouse_y==inicio_y_dir-1) {
								//Justo en la linea superior del directorio
								menu_filesel_cursor_arriba();
								menu_speech_tecla_pulsada=1;
							}
							else if (menu_mouse_y==inicio_y_dir+FILESEL_ALTO_DIR) {
								//Justo en la linea inferior del directorio
								menu_filesel_cursor_abajo();
								menu_speech_tecla_pulsada=1;
							}
							else if (menu_mouse_y==FILESEL_POS_FILTER+1) {
								//En la linea de filtros
								//nada en especial
								//printf ("En linea de filtros\n");
							}
				  }
					else {
						//printf ("fuera ventana\n");
					}

				}

				switch (tecla) {
					//abajo
					case 10:
					menu_filesel_cursor_abajo();
					//Para no releer todas las entradas
					menu_speech_tecla_pulsada=1;
					break;

					//arriba
					case 11:
					menu_filesel_cursor_arriba();
					//Para no releer todas las entradas
					menu_speech_tecla_pulsada=1;
					break;

					//PgDn
					case 25:
						for (aux_pgdnup=0;aux_pgdnup<FILESEL_ALTO_DIR;aux_pgdnup++)
							menu_filesel_cursor_abajo();
						//releer todas entradas
						menu_speech_tecla_pulsada=0;
						//y decir active item
						menu_active_item_primera_vez=1;
                                        break;

					//PgUp
					case 24:
						for (aux_pgdnup=0;aux_pgdnup<FILESEL_ALTO_DIR;aux_pgdnup++)
							menu_filesel_cursor_arriba();
						//releer todas entradas
						menu_speech_tecla_pulsada=0;
						//y decir active item
						menu_active_item_primera_vez=1;
                                        break;




					case 15:
					//tabulador
						menu_reset_counters_tecla_repeticion();
						filesel_zona_pantalla=2;
					break;

					//ESC
					case 2:
						//meter en menu_filesel_last_directory_seen nombre directorio
						//getcwd(archivo,PATH_MAX);
						getcwd(menu_filesel_last_directory_seen,PATH_MAX);
						//printf ("salimos con ESC. nombre directorio: %s\n",archivo);
                                                                        menu_filesel_exist_ESC();

                                                                        return 0;

					break;

					case 13:
						//Si se ha pulsado boton de raton
						if (mouse_left){
							//printf ("Boton pulsado\n");
							//Si en linea de filtros
							if (menu_mouse_y==FILESEL_POS_FILTER+1) {
								/*menu_reset_counters_tecla_repeticion();
								filesel_zona_pantalla=2;
								break;*/
							}

							//Si en linea de "File"
							if (menu_mouse_y==2) {
								menu_reset_counters_tecla_repeticion();
								filesel_zona_pantalla=0;
								break;
							}

							//Si se ha pulsado boton pero fuera de la zona de archivos, no hacer nada
							if (!si_mouse_zona_archivos() ) break;
						}
						//si seleccion es directorio
						item_seleccionado=menu_get_filesel_item(filesel_archivo_seleccionado+filesel_linea_seleccionada);
						menu_reset_counters_tecla_repeticion();

						//printf ("despues de get filesel item. item_seleccionado=%p\n",item_seleccionado);

						if (item_seleccionado==NULL) {
							//Esto pasa en las carpetas vacias, como /home en Mac OS
                                                                        menu_filesel_exist_ESC();
                                                                        return 0;


						}

						if (get_file_type(item_seleccionado->d_type,item_seleccionado->d_name)==2) {
							debug_printf (VERBOSE_DEBUG,"Is a directory. Change");
							char *directorio_a_cambiar;

							//suponemos esto:
							directorio_a_cambiar=item_seleccionado->d_name;
							char last_directory[PATH_MAX];

							//si es "..", ver si directorio actual contiene archivo que indica ultimo directorio
							//en caso de descompresiones
							if (!strcmp(item_seleccionado->d_name,"..")) {
								debug_printf (VERBOSE_DEBUG,"Is directory ..");
								if (si_existe_archivo(MENU_LAST_DIR_FILE_NAME)) {
									debug_printf (VERBOSE_DEBUG,"Directory has file " MENU_LAST_DIR_FILE_NAME " Changing "
											"to previous directory");

									if (menu_filesel_read_file_last_dir(last_directory)==0) {
										debug_printf (VERBOSE_DEBUG,"Previous directory was: %s",last_directory);

										directorio_a_cambiar=last_directory;
									}

								}
							}

							debug_printf (VERBOSE_DEBUG,"Changing to directory %s",directorio_a_cambiar);

							menu_filesel_chdir(directorio_a_cambiar);


							menu_filesel_free_mem();
							releer_directorio=1;

						        //Decir directorio activo
							//Esperar a liberar tecla si no la tecla invalida el speech
							menu_espera_no_tecla();
						        menu_textspeech_say_current_directory();

						}

						else {

							//Si seleccion es archivo comprimido
							if (
							    //strstr(item_seleccionado->d_name,".zip")!=NULL ||
							    !util_compare_file_extension(item_seleccionado->d_name,"zip") ||
                                                            !util_compare_file_extension(item_seleccionado->d_name,"gz") ||
                                                            !util_compare_file_extension(item_seleccionado->d_name,"tar") ||
                                                            !util_compare_file_extension(item_seleccionado->d_name,"rar")


							) {
								debug_printf (VERBOSE_DEBUG,"Is a compressed file");

								char tmpdir[PATH_MAX];

								if (menu_filesel_uncompress(item_seleccionado->d_name,tmpdir) ) {
									menu_filesel_exist_ESC();
									return 0;
								}

								else {
	                                                                //cambiar a tmp dir.

									//TODO. Dejar antes un archivo temporal en ese directorio que indique directorio anterior
									char directorio_actual[PATH_MAX];
									getcwd(directorio_actual,PATH_MAX);

        	                                                        menu_filesel_chdir(tmpdir);

									//escribir archivo que indique directorio anterior
									menu_filesel_write_file_last_dir(directorio_actual);

                	                                                menu_filesel_free_mem();
                        	                                        releer_directorio=1;
								}

							}

							else {

					                        cls_menu_overlay();
        	                                        	menu_espera_no_tecla();

								//unimos directorio y nombre archivo
								getcwd(archivo,PATH_MAX);
								sprintf(&archivo[strlen(archivo)],"/%s",item_seleccionado->d_name);

								menu_filesel_chdir(filesel_directorio_inicial);
								menu_filesel_free_mem();

								return menu_avisa_si_extension_no_habitual(filtros,archivo);

								//Volver con OK
								//return 1;
							}

						}
					break;
				}

				//entre a y z y numeros
				if ( (tecla>='a' && tecla<='z') || (tecla>='0' && tecla<='9') ) {
					menu_filesel_localiza_letra(tecla);
				}

				//menu_espera_no_tecla();
				menu_espera_no_tecla_con_repeticion();




			break;

			case 2:
				//zona filtros
                                menu_print_dir(filesel_archivo_seleccionado);

                                //para que haga lectura de los filtros
                                menu_speech_tecla_pulsada=0;

				menu_filesel_print_filters(filesel_filtros);
		                all_interlace_scr_refresca_pantalla();

				menu_espera_tecla();
				tecla=menu_get_pressed_key();
				menu_espera_no_tecla();

		                //ESC
                                if (tecla==2) {
                                                cls_menu_overlay();
                                                menu_espera_no_tecla();
                                                menu_filesel_chdir(filesel_directorio_inicial);
						menu_filesel_free_mem();
                                                return 0;
                                }

				//cambiar de zona con tab
				//if (tecla==15 || tecla==13) {
				if (tecla==15) {
					menu_reset_counters_tecla_repeticion();
					filesel_zona_pantalla=0;
					menu_filesel_print_filters(filesel_filtros);
					//no releer todos archivos
					menu_speech_tecla_pulsada=1;
				}

				else {

					//printf ("conmutar filtros\n");

					//conmutar filtros
					menu_filesel_switch_filters();

				        menu_filesel_print_filters(filesel_filtros);
					releer_directorio=1;
				}

			break;
			}

		} while (releer_directorio==0);
	} while (1);


	//Aqui no se va a llegar nunca
	//cls_menu_overlay();
        //menu_espera_no_tecla();


}


void estilo_gui_retorna_nombres(void)
{
	int i;

	for (i=0;i<ESTILOS_GUI;i++) {
		printf ("%s ",definiciones_estilos_gui[i].nombre_estilo);
	}
}

void set_charset(void)
{

	if (estilo_gui_activo==ESTILO_GUI_CPC) char_set=char_set_cpc;
	else if (estilo_gui_activo==ESTILO_GUI_Z88) char_set=char_set_z88;
	else if (estilo_gui_activo==ESTILO_GUI_SAM) char_set=char_set_sam;
	else if (estilo_gui_activo==ESTILO_GUI_MANSOFTWARE) char_set=char_set_mansoftware;
	else char_set=char_set_spectrum;
}
