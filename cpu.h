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

#ifndef CPU_H
#define CPU_H

#include "compileoptions.h"

//#define EMULATOR_VERSION "6.0"

#define EMULATOR_VERSION "6.0-RC"
//#define EMULATOR_VERSION "6.0-SN"
//#define SNAPSHOT_VERSION

#define EMULATOR_DATE "30 October 2017"
#define EMULATOR_SHORT_DATE "30/10/2017"
#define EMULATOR_EDITION_NAME "Gunfright edition"

//8 bits
typedef unsigned char z80_byte;
//16 bits
typedef unsigned short z80_int;
//32 bits
typedef unsigned int z80_long_int;

//Usado en menu, para registros de 16 bits de z80 y de 32 de motorola
typedef unsigned int menu_z80_moto_int;

struct s_z80_bit {
	unsigned int v:1;
};


typedef union {
#ifdef WORDS_BIGENDIAN
  struct { z80_byte h,l; } b;
#else
  struct { z80_byte l,h; } b;
#endif

  z80_int w;
} z80_registro;


typedef struct s_z80_bit z80_bit;


extern int zoom_x,zoom_y;
extern int zoom_x_original,zoom_y_original;


extern z80_registro registro_hl;
#define reg_h registro_hl.b.h
#define reg_l registro_hl.b.l
#define HL registro_hl.w
#define reg_hl registro_hl.w


extern z80_registro registro_de;
#define reg_d registro_de.b.h
#define reg_e registro_de.b.l
#define DE registro_de.w
#define reg_de registro_de.w


extern z80_registro registro_bc;
#define reg_b registro_bc.b.h
#define reg_c registro_bc.b.l
#define BC registro_bc.w
#define reg_bc registro_bc.w


extern z80_byte reg_i;
extern z80_byte reg_r,reg_r_bit7;

#define IR ( ( reg_i ) << 8 | ( reg_r_bit7 & 0x80 ) | ( reg_r & 0x7f ) )

extern z80_byte reg_h_shadow,reg_l_shadow;
extern z80_byte reg_b_shadow,reg_c_shadow;
extern z80_byte reg_d_shadow,reg_e_shadow;
extern z80_byte reg_a_shadow;

#define REG_AF (value_8_to_16(reg_a,Z80_FLAGS))

#define REG_AF_SHADOW (value_8_to_16(reg_a_shadow,Z80_FLAGS_SHADOW))
#define REG_HL_SHADOW (value_8_to_16(reg_h_shadow,reg_l_shadow))
#define REG_BC_SHADOW (value_8_to_16(reg_b_shadow,reg_c_shadow))
#define REG_DE_SHADOW (value_8_to_16(reg_d_shadow,reg_e_shadow))

extern void set_machine(char *romfile);
extern void set_machine_params(void);
extern void post_set_machine(char *romfile);
extern void post_set_machine_no_rom_load(void);

extern z80_int reg_pc;
extern z80_byte reg_a;
extern z80_int reg_sp;
extern z80_int reg_ix;
extern z80_int reg_iy;

extern z80_byte Z80_FLAGS;
extern z80_byte Z80_FLAGS_SHADOW;

#define FLAG_C  0x01
#define FLAG_N  0x02
#define FLAG_PV 0x04
#define FLAG_3  0x08
#define FLAG_H  0x10
#define FLAG_5  0x20
#define FLAG_Z  0x40
#define FLAG_S  0x80

extern z80_int memptr;

extern z80_bit iff1,iff2;

extern z80_bit interrupcion_pendiente;
extern z80_bit z80_ejecutando_halt;
extern z80_byte im_mode;
extern z80_bit cpu_step_mode;

#ifndef GCC_UNUSED

#ifdef __GNUC__
#  define GCC_UNUSED __attribute__((unused))
#else
#  define GCC_UNUSED
#endif

#endif


extern z80_byte puerto_65278; //    db    255  ; V    C    X    Z    Sh    ;0
extern z80_byte puerto_65022; //    db    255  ; G    F    D    S    A     ;1
extern z80_byte puerto_64510; //    db              255  ; T    R    E    W    Q     ;2
extern z80_byte puerto_63486; //    db              255  ; 5    4    3    2    1     ;3
extern z80_byte puerto_61438; //    db              255  ; 6    7    8    9    0     ;4
extern z80_byte puerto_57342; //    db              255  ; Y    U    I    O    P     ;5
extern z80_byte puerto_49150; //    db              255  ; H                J         K      L    Enter ;6
extern z80_byte puerto_32766; //    db              255  ; B    N    M    Simb Space ;7
extern z80_byte puerto_especial1;
extern z80_byte puerto_especial2;
extern z80_byte puerto_especial3;
extern z80_byte puerto_especial4;


extern z80_int *registro_ixiy;

extern z80_bit border_enabled;


extern void (*codsinpr[]) ();
extern void (*codprcb[]) ();
extern void (*codpred[]) ();

extern void (*codprddfd[]) ();

extern z80_byte *memoria_spectrum;

//extern z80_byte inves_ula_delay_factor;
extern z80_byte current_machine_type;
extern z80_bit modificado_border;

extern z80_bit inves_ula_bright_error;




extern void set_undocumented_flags_bits(z80_byte value);
extern void set_flags_zero_sign(z80_byte value);
extern void set_flags_zero_sign_16(z80_int value);
extern void set_flags_carry_suma(z80_byte antes,z80_byte result);
extern void set_flags_carry_resta(z80_byte antes,z80_byte result);
extern void set_flags_carry_16_suma(z80_int antes,z80_int result);
extern void set_flags_carry_16_resta(z80_int antes,z80_int result);

extern void reset_cpu(void);
extern void cold_start_cpu_registers(void);

#define value_8_to_16(h,l) (((h)<<8)|l)

#define value_16_to_8l(hl) ((hl) & 0xFF)
#define value_16_to_8h(hl) (((hl)>>8) & 0xFF)

extern z80_byte *devuelve_reg_offset(z80_byte valor);

extern int video_zx8081_linecntr;
extern z80_bit video_zx8081_linecntr_enabled;

extern z80_byte video_zx8081_ula_video_output;


//#define TEMP_LINEAS_ZX8081 192
//#define TEMP_BORDE_SUP_ZX8081 56


//#define macro_invalid_opcode(MESSAGE) debug_printf(VERBOSE_ERR,"Invalid opcode " MESSAGE ". Final PC: %X",reg_pc)

//extern z80_bit debug_cpu_core_loop;
//extern int cpu_core_loop_active;
//extern void (*cpu_core_loop) (void);

#define CPU_CORE_SPECTRUM 1
#define CPU_CORE_ZX8081 2
#define CPU_CORE_Z88 3
#define CPU_CORE_ACE 4
#define CPU_CORE_CPC 5
#define CPU_CORE_SAM 6
#define CPU_CORE_QL 7
#define CPU_CORE_MK14 8

extern struct timeval z80_interrupts_timer_antes, z80_interrupts_timer_ahora;
extern long z80_timer_difftime, z80_timer_seconds, z80_timer_useconds;
extern struct timeval zesarux_start_time;
extern z80_bit interrupcion_timer_generada;
extern int contador_frames_veces_buffer_audio;
extern z80_bit esperando_tiempo_final_t_estados;
extern int debug_registers;
extern z80_bit interrupcion_maskable_generada;
extern z80_bit interrupcion_non_maskable_generada;
extern z80_bit interrupcion_timer_generada;
extern z80_byte reg_r_antes_zx8081;
extern z80_bit temp_zx8081_lineasparimpar;
extern char *get_machine_name(z80_byte m);

extern int porcentaje_velocidad_emulador;
extern void set_emulator_speed(void);

//T-estados totales del frame
extern int t_estados;

extern int t_scanline;

//Scan line actual
extern int t_scanline_draw;
extern int t_scanline_draw_timeout;

//Autoseleccionar opciones de emulacion (audio, realvideo, etc) segun snap o cinta cargada
extern z80_bit autoselect_snaptape_options;

extern z80_bit tape_loading_simulate;
extern z80_bit tape_loading_simulate_fast;
extern void end_emulator(void);

extern z80_bit snow_effect_enabled;

struct s_driver_struct
{
        char driver_name[30];
        int (*funcion_init) () ;
        int (*funcion_set) () ;
};

typedef struct s_driver_struct driver_struct;

extern driver_struct scr_driver_array[];
extern int num_scr_driver_array;

extern driver_struct audio_driver_array[];
extern int num_audio_driver_array;

#define MAX_SCR_INIT 10
#define MAX_AUDIO_INIT 10

extern z80_bit stdout_simpletext_automatic_redraw;
//Valores para stdout. Estan aqui porque se graban en archivo .zx, aunque no este el driver stdout compilado
//fin valores para stdout


#define MACHINE_ID_SPECTRUM_48 1

#define MACHINE_ID_PENTAGON 21
#define MACHINE_ID_CHROME 22

#define MACHINE_ID_TSCONF 23
#define MACHINE_ID_BASECONF 24


#define MACHINE_ID_CPC_464     140
#define MACHINE_ID_QL_STANDARD     160
#define MACHINE_ID_MK14_STANDARD 180


//Condiciones de maquinas activas
#define MACHINE_IS_SPECTRUM (current_machine_type<30)
#define MACHINE_IS_SPECTRUM_16 (current_machine_type==0)
#define MACHINE_IS_SPECTRUM_48 (current_machine_type==MACHINE_ID_SPECTRUM_48)
#define MACHINE_IS_INVES (current_machine_type==2)
#define MACHINE_IS_SPECTRUM_16_48 ( (current_machine_type<=5) || current_machine_type==20 )
#define MACHINE_IS_SPECTRUM_128_P2 ( (current_machine_type>=6 && current_machine_type<=10) || MACHINE_IS_PENTAGON)
#define MACHINE_IS_SPECTRUM_P2A (current_machine_type>=11 && current_machine_type<=13)
#define MACHINE_IS_SPECTRUM_128_P2_P2A ( (current_machine_type>=6 && current_machine_type<=13) || MACHINE_IS_PENTAGON)
#define MACHINE_IS_ZXUNO (current_machine_type==14)

#define MACHINE_IS_CHLOE_140SE (current_machine_type==15)
#define MACHINE_IS_CHLOE_280SE (current_machine_type==16)
#define MACHINE_IS_CHLOE (current_machine_type==15 || current_machine_type==16)
#define MACHINE_IS_TIMEX_TS2068 (current_machine_type==17)
#define MACHINE_IS_PRISM (current_machine_type==18)
#define MACHINE_IS_TBBLUE (current_machine_type==19)
#define MACHINE_IS_SPECTRUM_PLUS_SPA (current_machine_type==20)
#define MACHINE_IS_PENTAGON (current_machine_type==MACHINE_ID_PENTAGON)
#define MACHINE_IS_CHROME (current_machine_type==MACHINE_ID_CHROME)


#define MACHINE_IS_TSCONF (current_machine_type==MACHINE_ID_TSCONF)
#define MACHINE_IS_BASECONF (current_machine_type==MACHINE_ID_BASECONF)

#define MACHINE_IS_EVO (MACHINE_IS_TSCONF || MACHINE_IS_BASECONF)

#define MACHINE_IS_ZX80 (current_machine_type==120)
#define MACHINE_IS_ZX81 (current_machine_type==121)
#define MACHINE_IS_ZX8081 (current_machine_type==120 || current_machine_type==121)
#define MACHINE_IS_ACE (current_machine_type==122)
#define MACHINE_IS_ZX8081ACE (MACHINE_IS_ZX8081 || MACHINE_IS_ACE)

#define MACHINE_IS_Z88 (current_machine_type==130)

#define MACHINE_IS_CPC_464 (current_machine_type==MACHINE_ID_CPC_464)
#define MACHINE_IS_CPC (current_machine_type>=140 && current_machine_type<=149)
#define MACHINE_IS_SAM (current_machine_type==150)

/*
160=QL
161-179 reservado para otros QL
*/

#define MACHINE_IS_QL_STANDARD (current_machine_type==MACHINE_ID_QL_STANDARD)
#define MACHINE_IS_QL (current_machine_type>=160 && current_machine_type<=179)

/*
180=MK14 Standard
181-189 reservado para otros MK14
*/

#define MACHINE_IS_MK14_STANDARD (current_machine_type==MACHINE_ID_MK14_STANDARD)
#define MACHINE_IS_MK14 (current_machine_type>=180 && current_machine_type<=189)


#define CPU_IS_MOTOROLA (MACHINE_IS_QL)
#define CPU_IS_SCMP (MACHINE_IS_MK14)
#define CPU_IS_Z80 (!CPU_IS_MOTOROLA && !CPU_IS_SCMP)


extern int machine_emulate_memory_refresh;
extern int machine_emulate_memory_refresh_counter;

extern z80_byte last_inves_low_ram_poke_menu;

extern void random_ram_inves(z80_byte *puntero,int longitud);



//valor obtenido probando
#define MAX_EMULATE_MEMORY_REFRESH_COUNTER 1500000
#define MAX_EMULATE_MEMORY_REFRESH_LIMIT (MAX_EMULATE_MEMORY_REFRESH_COUNTER/2)


extern int zesarux_main (int main_argc,char *main_argv[]);
extern z80_bit no_cambio_parametros_maquinas_lentas;
extern z80_bit opcion_no_splash;

extern int argc;
extern char **argv;
extern int puntero_parametro;
extern int siguiente_parametro(void);

extern void siguiente_parametro_argumento(void);

extern int joystickkey_definidas;
extern void rom_load(char *romfilename);
extern void hard_reset_cpu(void);

extern z80_bit zxmmc_emulation;

extern z80_bit quickexit;

extern z80_bit azerty_keyboard;

extern z80_int ramtop_ace;

extern z80_bit allow_write_rom;

extern int z88_cpc_keymap_type;

extern z80_bit chloe_keyboard;

extern unsigned int debug_t_estados_parcial;

#define MAX_CPU_TURBO_SPEED 16

extern int cpu_turbo_speed;

extern void cpu_set_turbo_speed(void);

extern z80_bit windows_no_disable_console;

extern char *string_machines_list_description;

extern void set_menu_gui_zoom(void);

#endif
