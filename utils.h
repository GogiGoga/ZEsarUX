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

#ifndef UTILS_H
#define UTILS_H

#include "cpu.h"
#include "compileoptions.h"

#include <stdio.h>
#include <dirent.h>

#ifdef MINGW
#include <stdlib.h>
#define PATH_MAX MAX_PATH
#define NAME_MAX MAX_PATH
#endif

extern void util_get_file_extension(char *filename,char *extension);
extern void util_get_file_no_directory(char *filename,char *file_no_dir);
extern void util_get_file_without_extension(char *filename,char *filename_without_extension);
extern void util_get_complete_path(char *dir,char *name,char *fullpath);


extern int util_compare_file_extension(char *filename,char *extension_compare);

extern void util_get_dir(char *ruta,char *directorio);

extern void set_symshift(void);

extern void clear_symshift(void);

extern void open_sharedfile(char *archivo,FILE **f);

#define MAX_COMPILE_INFO_LENGTH 4096

extern void get_compile_info(char *s);

extern void show_compile_info(void);

extern FILE *ptr_input_file_keyboard;
extern char input_file_keyboard_name_buffer[];
extern char *input_file_keyboard_name;
extern z80_bit input_file_keyboard_inserted;
extern int input_file_keyboard_delay;
extern int input_file_keyboard_delay_counter;
extern z80_bit input_file_keyboard_pending_next;
extern unsigned char input_file_keyboard_last_key;
extern z80_bit input_file_keyboard_is_pause;
extern z80_bit input_file_keyboard_send_pause;
extern z80_bit input_file_keyboard_turbo;


extern void insert_input_file_keyboard(void);
extern void eject_input_file_keyboard(void);
extern int input_file_keyboard_init(void);

extern void reset_keyboard_ports(void);
extern void ascii_to_keyboard_port(unsigned tecla);
extern void ascii_to_keyboard_port_set_clear(unsigned tecla,int pressrelease);
extern void input_file_keyboard_get_key(void);
extern void input_file_keyboard_close(void);


#ifdef EMULATE_CPU_STATS
	extern unsigned int stats_codsinpr[];
	extern unsigned int stats_codpred[];
	extern unsigned int stats_codprcb[];
	extern unsigned int stats_codprdd[];
	extern unsigned int stats_codprfd[];
	extern unsigned int stats_codprddcb[];
	extern unsigned int stats_codprfdcb[];

	extern void util_stats_increment_counter(unsigned int *stats_array,int index);
	extern unsigned int util_stats_get_counter(unsigned int *stats_array,int index);
	extern void util_stats_set_counter(unsigned int *stats_array,int index,unsigned int value);
	extern void util_stats_init(void);
	extern int util_stats_find_max_counter(unsigned int *stats_array);
	extern unsigned int util_stats_sum_all_counters(void);

#endif

struct x_tabla_teclado
{
        z80_byte *puerto;
        z80_byte mascara;
};

extern struct x_tabla_teclado tabla_teclado_numeros[];
extern struct x_tabla_teclado tabla_teclado_letras[];
extern void convert_numeros_letras_puerto_teclado(z80_byte tecla,int pressrelease);

extern struct x_tabla_teclado ql_tabla_teclado_letras[];
extern struct x_tabla_teclado ql_tabla_teclado_numeros[];

extern int get_rom_size(int machine);
extern void configfile_parse(void);
extern char *configfile_argv[];
extern int configfile_argc;

extern z80_bit debug_parse_config_file;

//valores teclas en modo raw
#define RAWKEY_Escape 0x01
#define RAWKEY_minus 0x0c
#define RAWKEY_equal 0x0d
#define RAWKEY_BackSpace 0x0e
#define RAWKEY_Tab 0x0f
#define RAWKEY_bracket_left 0x1a
#define RAWKEY_bracket_right 0x1b
#define RAWKEY_Return 0x1c
#define RAWKEY_Control_L 0x1d
#define RAWKEY_semicolon 0x27
#define RAWKEY_apostrophe 0x28
#define RAWKEY_grave 0x29
#define RAWKEY_Shift_L 0x2a
#define RAWKEY_backslash 0x2b
#define RAWKEY_comma 0x33
#define RAWKEY_period 0x34
#define RAWKEY_slash 0x35
#define RAWKEY_Shift_R 0x36

#define RAWKEY_Alt_L 0x38
#define RAWKEY_Space 0x39

#define RAWKEY_Caps_Lock 0x3a

#define RAWKEY_F1 0x3b
#define RAWKEY_F2 0x3c
#define RAWKEY_F3 0x3d
#define RAWKEY_F4 0x3e
#define RAWKEY_F5 0x3f
#define RAWKEY_F6 0x40
#define RAWKEY_F7 0x41

#define RAWKEY_F8 0x42
#define RAWKEY_F9 0x43
#define RAWKEY_F10 0x44



//Estas home,up,left... hasta pgdown son las generadas en el teclado numerico
#define RAWKEY_Keypad_Home 0x47
#define RAWKEY_Keypad_Up 0x48
#define RAWKEY_Keypad_Left 0x4b
#define RAWKEY_Keypad_Right 0x4d
#define RAWKEY_Keypad_Down 0x50

#define RAWKEY_Keypad_Page_Up 0x49
#define RAWKEY_Keypad_Page_Down 0x51

//Tecla a la izquierda de la Z
#define RAWKEY_leftz 0x56



#define RAWKEY_KP_Subtract 0x4a
//#define RAWKEY_plus 0x4e
#define RAWKEY_KP_Add 0x4e

//#define RAWKEY_slash 0x35
#define RAWKEY_KP_Divide 0x35

//#define RAWKEY_asterisk 0x37
#define RAWKEY_KP_Multiply 0x37


/*
47 (Keypad-7/Home), 48 (Keypad-8/Up), 49 (Keypad-9/PgUp)
4a (Keypad--)
4b (Keypad-4/Left), 4c (Keypad-5), 4d (Keypad-6/Right), 4e (Keypad-+)
4f (Keypad-1/End), 50 (Keypad-2/Down), 51 (Keypad-3/PgDn)
52 (Keypad-0/Ins), 53 (Keypad-./Del)

*/

//En raspberry:

#define RAWKEY_RPI_Home 0x66
#define RAWKEY_RPI_Up 0x67
#define RAWKEY_RPI_Left 0x69
#define RAWKEY_RPI_Right 0x6a
#define RAWKEY_RPI_Down 0x6c


//#define RAWKEY_Control_R 0xe01d
//#define RAWKEY_Alt_R 0xe038



extern int quickload(char *nombre);
extern int quickload_valid_extension(char *nombre);
extern void insert_tape_cmdline(char *s);
extern void insert_snap_cmdline(char *s);

extern int si_existe_archivo(char *nombre);

extern long int get_file_size(char *nombre);

extern int lee_archivo(char *nombre,char *buffer,int max_longitud);

extern int util_get_configfile_name(char *configfile);

//extern void util_print_second_overlay(char *texto, int x, int y);

//valores usados en funcion util_set_reset_mouse
enum util_mouse_buttons
{
	UTIL_MOUSE_LEFT_BUTTON,
	UTIL_MOUSE_RIGHT_BUTTON
};


extern void util_set_reset_mouse(enum util_mouse_buttons boton,int pressrelease);

//valores usados en funcion util_set_reset_key
enum util_teclas
{
	UTIL_KEY_NONE,  //None se usa en teclado chloe

	UTIL_KEY_SPACE,
	UTIL_KEY_ENTER,
	UTIL_KEY_SHIFT_L,
	UTIL_KEY_SHIFT_R,
	UTIL_KEY_ALT_L,
	UTIL_KEY_ALT_R,
	UTIL_KEY_CONTROL_L,
	UTIL_KEY_CONTROL_R,
	UTIL_KEY_BACKSPACE,
	UTIL_KEY_HOME,
	UTIL_KEY_LEFT,
	UTIL_KEY_RIGHT,
	UTIL_KEY_DOWN,
	UTIL_KEY_UP,
	UTIL_KEY_TAB,
	UTIL_KEY_CAPS_LOCK,
	UTIL_KEY_COMMA,
	UTIL_KEY_PERIOD,
	UTIL_KEY_MINUS,
	UTIL_KEY_PLUS,
	UTIL_KEY_SLASH,
	UTIL_KEY_ASTERISK,
	UTIL_KEY_F1,
	UTIL_KEY_F2,
	UTIL_KEY_F3,
	UTIL_KEY_F4,
	UTIL_KEY_F5,
	UTIL_KEY_F6,
	UTIL_KEY_F7,
	UTIL_KEY_F8,
	UTIL_KEY_F9,
	UTIL_KEY_F10,
	UTIL_KEY_F11,
	UTIL_KEY_F12,
	UTIL_KEY_F13,
	UTIL_KEY_F14,
	UTIL_KEY_F15,
	UTIL_KEY_ESC,
	UTIL_KEY_PAGE_UP,
	UTIL_KEY_PAGE_DOWN,
	UTIL_KEY_KP0,
	UTIL_KEY_KP1,
	UTIL_KEY_KP2,
	UTIL_KEY_KP3,
	UTIL_KEY_KP4,
	UTIL_KEY_KP5,
	UTIL_KEY_KP6,
	UTIL_KEY_KP7,
	UTIL_KEY_KP8,
	UTIL_KEY_KP9,
	UTIL_KEY_KP_COMMA,
	UTIL_KEY_KP_ENTER,
	UTIL_KEY_WINKEY
};

//valores usados en funcion util_set_reset_key_z88_keymap
//son teclas que vienen de conversion de keymap
enum util_teclas_z88_keymap
{
        UTIL_KEY_Z88_MINUS,
	UTIL_KEY_Z88_EQUAL,
	UTIL_KEY_Z88_BACKSLASH,
	UTIL_KEY_Z88_BRACKET_LEFT,
	UTIL_KEY_Z88_BRACKET_RIGHT,
	UTIL_KEY_Z88_SEMICOLON,
	UTIL_KEY_Z88_APOSTROPHE,
	UTIL_KEY_Z88_POUND,
	UTIL_KEY_Z88_COMMA,
	UTIL_KEY_Z88_PERIOD,
	UTIL_KEY_Z88_SLASH
};


//Usados en mapeo de keymap que deberia ser comun a todas maquinas (z88, cpc, chloe, sam)
//De momento solo usado en sam
enum util_teclas_common_keymap
{
        UTIL_KEY_COMMON_KEYMAP_MINUS,
        UTIL_KEY_COMMON_KEYMAP_EQUAL,
        UTIL_KEY_COMMON_KEYMAP_BACKSLASH,
        UTIL_KEY_COMMON_KEYMAP_BRACKET_LEFT,
        UTIL_KEY_COMMON_KEYMAP_BRACKET_RIGHT,
        UTIL_KEY_COMMON_KEYMAP_SEMICOLON,
        UTIL_KEY_COMMON_KEYMAP_APOSTROPHE,
        UTIL_KEY_COMMON_KEYMAP_POUND,
        UTIL_KEY_COMMON_KEYMAP_COMMA,
        UTIL_KEY_COMMON_KEYMAP_PERIOD,
        UTIL_KEY_COMMON_KEYMAP_SLASH
};

//valores usados en funcion util_set_reset_key_cpc_keymap
//son teclas que vienen de conversion de keymap
enum util_teclas_cpc_keymap
{
        UTIL_KEY_CPC_MINUS,
        UTIL_KEY_CPC_CIRCUNFLEJO,
	UTIL_KEY_CPC_ARROBA,
        UTIL_KEY_CPC_BRACKET_LEFT,
	UTIL_KEY_CPC_COLON,
        UTIL_KEY_CPC_SEMICOLON,
        UTIL_KEY_CPC_BRACKET_RIGHT,
        UTIL_KEY_CPC_COMMA,
        UTIL_KEY_CPC_PERIOD,
        UTIL_KEY_CPC_SLASH,
        UTIL_KEY_CPC_BACKSLASH
};


enum util_teclas_chloe_keymap
{
        UTIL_KEY_CHLOE_MINUS,
        UTIL_KEY_CHLOE_EQUAL,
        UTIL_KEY_CHLOE_BACKSLASH,
        UTIL_KEY_CHLOE_BRACKET_LEFT,
        UTIL_KEY_CHLOE_BRACKET_RIGHT,
        UTIL_KEY_CHLOE_SEMICOLON,
        UTIL_KEY_CHLOE_APOSTROPHE,
        UTIL_KEY_CHLOE_POUND,
        UTIL_KEY_CHLOE_COMMA,
        UTIL_KEY_CHLOE_PERIOD,
        UTIL_KEY_CHLOE_SLASH,
        UTIL_KEY_CHLOE_LEFTZ
};


extern void util_set_reset_key(enum util_teclas tecla,int pressrelease);
extern void util_set_reset_key_z88_keymap(enum util_teclas_z88_keymap tecla,int pressrelease);
extern void util_set_reset_key_cpc_keymap(enum util_teclas_cpc_keymap tecla,int pressrelease);
extern void util_set_reset_key_chloe_keymap(enum util_teclas_chloe_keymap tecla,int pressrelease);
extern void util_set_reset_key_common_keymap(enum util_teclas_common_keymap tecla,int pressrelease);

extern unsigned int parse_string_to_number(char *texto);

//#define TMPDIR_BASE "/tmp/zesarux"

extern char *get_tmpdir_base(void);

extern int convert_smp_to_rwa(char *origen, char *destino);
extern int convert_wav_to_rwa(char *origen, char *destino);
extern int convert_tzx_to_rwa(char *origen, char *destino);
extern int convert_o_to_rwa(char *origen, char *destino);
extern int convert_p_to_rwa(char *origen, char *destino);
extern int convert_tap_to_rwa(char *origen, char *destino);
extern z80_bit quickload_guessing_tzx_type;

extern int load_binary_file(char *binary_file_load,int valor_leido_direccion,int valor_leido_longitud);
extern void parse_customfile_options(void);
extern void parse_custom_file_config(char *archivo);

extern int set_machine_type_by_name(char *machine_name);
extern void open_sharedfile_write(char *archivo,FILE **f);
extern int scandir_mingw(const char *dir, struct dirent ***namelist,
              int (*filter)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **));

extern void customconfig_help(void);

extern char letra_mayuscula(char c);

extern char letra_minuscula(char c);

extern void string_a_minusculas(char *origen, char *destino);

extern int si_ruta_absoluta(char *ruta);

extern int get_file_type(int d_type, char *nombre);

extern char external_tool_sox[];
extern char external_tool_unzip[];
extern char external_tool_gunzip[];
extern char external_tool_tar[];
extern char external_tool_unrar[];

extern void convert_relative_to_absolute(char *relative_path,char *final_path);

#define MAX_LINEAS_POK_FILE 100
#define MAX_LENGTH_LINE_POKE_FILE 90

struct s_pokfile
{
        int indice_accion;
        char texto[MAX_LENGTH_LINE_POKE_FILE+1];
	z80_byte banco;
	z80_int direccion;
	z80_byte valor;
	z80_byte valor_orig;
};


extern int util_parse_pok_file(char *file,struct s_pokfile **tabla_pokes);

extern int util_poke(z80_byte banco,z80_int direccion,z80_byte valor);

extern int util_busca_archivo_nocase(char *archivo,char *directorio,char *nombreencontrado);

extern void set_peek_byte_function_spoolturbo(void);
extern void reset_peek_byte_function_spoolturbo(void);


extern void set_poke_byte_function_writerom(void);
extern void reset_poke_byte_function_writerom(void);

/*
Lista fabricantes
Sinclair
Timex Sinclair
Cambridge computers
Investronica
Microdigital Eletronica
Amstrad
Júpiter cantab
Zxuno team
Chloe Corportation
Jeff Braine
Miles Gordon Tech
Pentagon
Mario Prato
*/

#define TOTAL_FABRICANTES 14
#define FABRICANTE_SINCLAIR 0
#define FABRICANTE_AMSTRAD 1
#define FABRICANTE_TIMEX_SINCLAIR 2
#define FABRICANTE_INVESTRONICA 3
#define FABRICANTE_MICRODIGITAL_ELECTRONICA 4
#define FABRICANTE_ZXUNO_TEAM 5
#define FABRICANTE_CHLOE_CORPORATION 6
#define FABRICANTE_JEFF_BRAINE 7
#define FABRICANTE_TBBLUE 8
#define FABRICANTE_CAMBRIDGE_COMPUTERS 9
#define FABRICANTE_JUPITER_CANTAB 10
#define FABRICANTE_MILES_GORDON 11
#define FABRICANTE_PENTAGON 12
#define FABRICANTE_MARIOPRATO 13

extern char *array_fabricantes[];
extern char *array_fabricantes_hotkey[];
extern char array_fabricantes_hotkey_letra[];

extern int *return_maquinas_fabricante(int fabricante);

extern int return_fabricante_maquina(int maquina);

extern int return_machine_position(int *array_maquinas,int id_maquina);


struct s_tecla_redefinida
{
        z80_byte tecla_original;   //Tecla a redefinir. Si no, vale 0
        z80_byte tecla_redefinida; //Tecla resultante. Si no, da igual su valor
};

typedef struct s_tecla_redefinida tecla_redefinida;

#define MAX_TECLAS_REDEFINIDAS 10
extern tecla_redefinida lista_teclas_redefinidas[];


extern void clear_lista_teclas_redefinidas(void);

extern int util_add_redefinir_tecla(z80_byte p_tecla_original, z80_byte p_tecla_redefinida);

extern void clear_lista_teclas_redefinidas(void);

extern void util_set_reset_key_continue(enum util_teclas tecla,int pressrelease);

extern void convert_numeros_letras_puerto_teclado_continue(z80_byte tecla,int pressrelease);

extern int timer_on_screen_key;


extern int util_tape_tap_get_info(z80_byte *tape,char *texto);

#define ZESARUX_CONFIG_FILE ".zesaruxrc"

extern void get_machine_config_name_by_number(char *machine_name,int machine_number);

extern int util_write_configfile(void);

extern z80_bit save_configuration_file_on_exit;

extern z80_byte peek_byte_z80_moto(unsigned int address);
extern void poke_byte_z80_moto(unsigned int address,z80_byte valor);

extern unsigned int get_ql_pc(void);

extern unsigned int adjust_address_space_cpu(unsigned int direccion);

extern unsigned int get_pc_register(void);

extern void convert_signed_unsigned(char *origen, unsigned char *destino,int longitud);

extern int uncompress_gz(char *origen,char *destino);

extern char *util_strcasestr(char *string, char *string_a_buscar);

extern void util_sprintf_address_hex(menu_z80_moto_int p,char *string_address);

extern int si_menu_mouse_activado(void);

//extern long long int parse_string_to_long_number(char *texto);

extern int util_parse_commands_argvc(char *texto, char *parm_argv[], int maximo);

extern int get_machine_id_by_name(char *machine_name);

#endif
