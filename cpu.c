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
#ifndef MINGW
	#include <unistd.h>
#endif
#include <string.h>

#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include <signal.h>

#ifdef MINGW
	//Para llamar a FreeConsole
	#include <windows.h>
#endif



#include "cpu.h"
#include "scrnull.h"
#include "operaciones.h"
#include "debug.h"
#include "compileoptions.h"
#include "tape.h"
#include "tape_tap.h"
#include "tape_tzx.h"
#include "tape_smp.h"
#include "audio.h"
#include "screen.h"
#include "ay38912.h"
#include "mem128.h"
#include "zx8081.h"
#include "snap.h"
#include "menu.h"
#include "core_spectrum.h"
#include "core_zx8081.h"
#include "timer.h"
#include "contend.h"
#include "utils.h"
#include "ula.h"
#include "printers.h"
#include "joystick.h"
#include "realjoystick.h"
#include "z88.h"
#include "ulaplus.h"
#include "zxuno.h"
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
#include "tbblue.h"
#include "dandanator.h"
#include "superupgrade.h"
#include "ql.h"
#include "m68k.h"
#include "remote.h"
#include "snap_rzx.h"
#include "multiface.h"

#ifdef COMPILE_STDOUT
#include "scrstdout.h"
#endif


#ifdef COMPILE_SIMPLETEXT
#include "scrsimpletext.h"
#endif


#ifdef COMPILE_CURSES
#include "scrcurses.h"
#endif

#ifdef COMPILE_AA
#include "scraa.h"
#endif

#ifdef COMPILE_CACA
#include "scrcaca.h"
#endif


#ifdef USE_COCOA
#include "scrcocoa.h"
#endif


#ifdef COMPILE_XWINDOWS
#include "scrxwindows.h"
#endif

#ifdef COMPILE_SDL

	#ifdef COMPILE_SDL2
		#include "scrsdl2.h"
	#else
		#include "scrsdl.h"
	#endif
#endif


#ifdef COMPILE_FBDEV
#include "scrfbdev.h"
#endif


#ifdef COMPILE_DSP
#include "audiodsp.h"
#endif

#ifdef COMPILE_SDL
#include "audiosdl.h"
#endif


#ifdef COMPILE_ALSA
#include "audioalsa.h"
#endif

#ifdef COMPILE_PULSE
#include "audiopulse.h"
#endif


#ifdef COMPILE_COREAUDIO
#include "audiocoreaudio.h"
#endif

#ifdef USE_REALTIME
#include <sched.h>
#endif

#include "audionull.h"

#ifdef USE_PTHREADS
#include <pthread.h>

pthread_t thread_main_loop;

#endif



#include "autoselectoptions.h"



char *romfilename;

//Maquina actual
z80_byte current_machine_type;

//Ultima maquina seleccionada desde post_set_machine
z80_byte last_machine_type=255;

char *scrfile;

int zoom_x=2,zoom_y=2;
int zoom_x_original,zoom_y_original;

struct timeval z80_interrupts_timer_antes, z80_interrupts_timer_ahora;
long z80_timer_difftime, z80_timer_seconds, z80_timer_useconds;

//Indica si se tiene que probar dispositivos. Solo se hace cuando no se especifica uno concreto por linea de comandos
z80_bit try_fallback_video;
z80_bit try_fallback_audio;

//punteros a funciones de inicio para hacer fallback de video, funciones de set
driver_struct scr_driver_array[MAX_SCR_INIT];
int num_scr_driver_array=0;

//punteros a funciones de inicio para hacer fallback de audio, funciones de set
driver_struct audio_driver_array[MAX_SCR_INIT];
int num_audio_driver_array=0;

//Los inicializamos a cadena vacia... No poner NULL, dado que hay varios strcmp que se comparan contra esto
char *driver_screen="";
char *driver_audio="";


//porcentaje de velocidad de la cpu
int porcentaje_velocidad_emulador=100;

int anterior_porcentaje_velocidad_emulador=100;


//parametro pasado por linea de comandos
//int initial_porcentaje_velocidad_emulador=100;




//linea actual (scanline) segun cuantas interrupciones maskables se han generado
//en modo slow del zx81 hay un "offset" para que se ajuste a la pantalla
int video_zx8081_linecntr=0;

z80_bit video_zx8081_linecntr_enabled;

//se supone que es el siguiente valor que tendra una linea entera segun la ULA, util para emular fast/slow
z80_byte video_zx8081_ula_video_output;

//simular franjas de carga y sonido. De momento solo para ZX80/81
z80_bit tape_loading_simulate;

//franjas de carga y sonido a velocidad real o rapido
z80_bit tape_loading_simulate_fast;

//Autoseleccionar opciones de emulacion (audio, realvideo, etc) segun snap o cinta cargada
z80_bit autoselect_snaptape_options;



int argc;
char **argv;
int puntero_parametro;

int debug_registers=0;

z80_bit border_enabled;

//contador que indica cuantos frames de pantalla entero se han enviado, para contar cuando enviamos el sonido
int contador_frames_veces_buffer_audio=0;

int final_nombre;


//T-estados totales del frame
int t_estados=0;

//t-estados parcial. Usados solo en debug
unsigned int debug_t_estados_parcial=0;

//Scan line actual. Este siempre indica la linea actual dentro del frame total. No alterable por vsync
int t_scanline=0;

//Scan line actual para dibujar en pantalla. En ZX spectrum siempre va adelante. En ZX80/81 lo altera el vsync
int t_scanline_draw=0;
//Linea actual desde el ultimo vsync para zx80/81. util para controlar timeout y forzar vsync
int t_scanline_draw_timeout=0;


//Si soporte de snow effect esta activo o no
z80_bit snow_effect_enabled;


void add_scr_init_array(char *name,int (*funcion_init) () , int (*funcion_set) () )
{
	if (num_scr_driver_array==MAX_SCR_INIT) {
                cpu_panic("Error. Maximum number of screen drivers");
	}


	strcpy(scr_driver_array[num_scr_driver_array].driver_name,name);
	scr_driver_array[num_scr_driver_array].funcion_init=funcion_init;
	scr_driver_array[num_scr_driver_array].funcion_set=funcion_set;

	num_scr_driver_array++;
}


void add_audio_init_array(char *name,int (*funcion_init) () , int (*funcion_set) () )
{
        if (num_audio_driver_array==MAX_AUDIO_INIT) {
                cpu_panic("Error. Maximum number of audio drivers");
        }


        strcpy(audio_driver_array[num_audio_driver_array].driver_name,name);
        audio_driver_array[num_audio_driver_array].funcion_init=funcion_init;
        audio_driver_array[num_audio_driver_array].funcion_set=funcion_set;

        num_audio_driver_array++;
}


void do_fallback_video(void)
{
	debug_printf(VERBOSE_INFO,"Guessing video driver");

	int i;

	for (i=0;i<num_scr_driver_array;i++) {

		screen_reset_scr_driver_params();

		int (*funcion_init) ();
		int (*funcion_set) ();

		funcion_init=scr_driver_array[i].funcion_init;
		funcion_set=scr_driver_array[i].funcion_set;
		if ( (funcion_init()) ==0) {
			debug_printf(VERBOSE_DEBUG,"Ok video driver i:%d %s",i,scr_driver_name);
			funcion_set();
			return;
		}

		debug_printf(VERBOSE_INFO,"Fallback to next video driver");
	}


	printf ("No valid video driver found\n");
	exit(1);
}

void do_fallback_audio(void)
{
        debug_printf(VERBOSE_INFO,"Guessing audio driver");

        int i;

        for (i=0;i<num_audio_driver_array;i++) {
                int (*funcion_init) ();
                int (*funcion_set) ();

                funcion_init=audio_driver_array[i].funcion_init;
                funcion_set=audio_driver_array[i].funcion_set;
                if ( (funcion_init()) ==0) {
                        debug_printf (VERBOSE_DEBUG,"Ok audio driver i:%d %s",i,audio_driver_name);
                        funcion_set();
                        return;
                }

		debug_printf(VERBOSE_INFO,"Fallback to next audio driver");
        }


        printf ("No valid audio driver found\n");
        exit(1);
}


int siguiente_parametro(void)
{
	argc--;
	if (argc==0) {
		//printf ("Error sintaxis\n");
		return 1;
	}
	puntero_parametro++;
	return 0;
}

void siguiente_parametro_argumento(void)
{
	if (siguiente_parametro()) {
		printf ("Syntax error. Parameter %s missing value\n",argv[puntero_parametro]);
		exit(1);
	}
}

z80_registro registro_hl;

z80_registro registro_de;

z80_registro registro_bc;

z80_byte reg_h_shadow,reg_l_shadow;
z80_byte reg_b_shadow,reg_c_shadow;
z80_byte reg_d_shadow,reg_e_shadow;
z80_byte reg_a_shadow;


z80_byte reg_i;
z80_byte reg_r,reg_r_bit7;

//usado en zx80/81
z80_byte reg_r_antes_zx8081;

z80_int reg_pc;
z80_byte reg_a;
z80_int reg_sp;
z80_int reg_ix;
z80_int reg_iy;

//Nueva gestion de flags
z80_byte Z80_FLAGS;
z80_byte Z80_FLAGS_SHADOW;

//MEMPTR. Solo se usara si se ha activado en el configure
z80_int memptr;


//Emulacion de refresco de memoria.
int machine_emulate_memory_refresh=0;
int machine_emulate_memory_refresh_counter=0;

//A 0 si interrupts disabled
//A 1 si interrupts enabled
//z80_bit interrupts;
z80_bit iff1;
/*
These flip flops are simultaneously set or reset by the EI and DI instructions. IFF1 determines whether interrupts are allowed, but its value cannot be read. The value of IFF2 is copied to the P/V flag by LD A,I and LD A,R. When an NMI occurs, IFF1 is reset, thereby disallowing further [maskable] interrupts, but IFF2 is left unchanged. This enables the NMI service routine to check whether the interrupted program had enabled or disabled maskable interrupts. So, Spectrum snapshot software can only read IFF2, but most emulators will emulate both, and then the one that matters most is IFF1.
*/

z80_bit iff2;

z80_byte im_mode=0;
z80_bit cpu_step_mode;

//border se ha modificado
z80_bit modificado_border;


z80_bit zxmmc_emulation={0};

//Inves. Poke ROM. valor por defecto
//z80_byte valor_poke_rom=255;


//Inves. Ultimo valor hecho poke a RAM baja (0...16383) desde menu
z80_byte last_inves_low_ram_poke_menu=255;

z80_bit inves_ula_bright_error={1};

//Inves. Contador ula delay. mas o menos exagerado
//maximo 1: a cada atributo
//z80_byte inves_ula_delay_factor=3;


//Tablas teclado
z80_byte puerto_65278=255; //    db    255  ; V    C    X    Z    Sh    ;0
z80_byte puerto_65022=255; //    db    255  ; G    F    D    S    A     ;1
z80_byte puerto_64510=255; //    db              255  ; T    R    E    W    Q     ;2
z80_byte puerto_63486=255; //    db              255  ; 5    4    3    2    1     ;3
z80_byte puerto_61438=255; //    db              255  ; 6    7    8    9    0     ;4
z80_byte puerto_57342=255; //    db              255  ; Y    U    I    O    P     ;5
z80_byte puerto_49150=255; //    db              255  ; H                J         K      L    Enter ;6
z80_byte puerto_32766=255; //    db              255  ; B    N    M    Simb Space ;7

//puertos especiales no presentes en spectrum
z80_byte puerto_especial1=255; //   ;  .  .  PgDn  PgUp ESC ;
z80_byte puerto_especial2=255; //   F5 F4 F3 F2 F1
z80_byte puerto_especial3=255; //  F10 F9 F8 F7 F6

z80_bit chloe_keyboard={0};



//se ha llegado al final de instrucciones en un frame de pantalla y esperamos
//a que se genere una interrupcion de 1/50s
z80_bit esperando_tiempo_final_t_estados;

//se ha generado interrupcion maskable de la cpu
//en spectrum pasa con el timer
//en zx80/zx81 pasa con el cambio de bit 6 en reg_r
z80_bit interrupcion_maskable_generada;


//se ha generado interrupcion non maskable de la cpu.
//pasa en zx81 cada 64 microsegundos y si el nmi generator esta activo
z80_bit interrupcion_non_maskable_generada;


//se ha generado interrupcion de timer 1/50s
z80_bit interrupcion_timer_generada;


z80_bit z80_ejecutando_halt;


z80_byte *memoria_spectrum;


z80_bit quickload_inicial;
char *quickload_nombre;


//Si no cambiamos parametros de frameskip y otros cuando es maquina lenta (raspberry, cocoa mac os x, etc)
z80_bit no_cambio_parametros_maquinas_lentas={0};



//Indica si se repinta automaticamente la pantalla
z80_bit stdout_simpletext_automatic_redraw={0};




//fin valores para stdout


z80_bit windows_no_disable_console={0};


//Si salir rapido del emulador
z80_bit quickexit={0};


//Si soporte azerty para xwindows
z80_bit azerty_keyboard={0};


z80_int ramtop_ace;

//Permitir escritura en ROM
z80_bit allow_write_rom={0};

//0=default
//1=spanish
int z88_cpc_keymap_type=0;

//Modo turbo. 1=normal. 2=7 Mhz, 3=14 Mhz, etc
int cpu_turbo_speed=1;

void cpu_set_turbo_speed(void)
{

	debug_printf (VERBOSE_INFO,"Setting turbo mode %dX",cpu_turbo_speed);

	if (cpu_turbo_speed>MAX_CPU_TURBO_SPEED) {
		debug_printf (VERBOSE_INFO,"Turbo mode higher than maximum. Setting to %d",MAX_CPU_TURBO_SPEED);
		cpu_turbo_speed=MAX_CPU_TURBO_SPEED;
	}

	//Si esta divmmc/divide, volver a aplicar funciones poke
	if (diviface_enabled.v) diviface_restore_peek_poke_functions();


	//Variable turbo se sobreescribe al llamar a set_machine_params. Guardar y restaurar luego
	int speed=cpu_turbo_speed;

	set_machine_params();

	cpu_turbo_speed=speed;

	screen_testados_linea *=cpu_turbo_speed;
        screen_set_video_params_indices();
        inicializa_tabla_contend();

        //Recalcular algunos valores cacheados
        recalcular_get_total_ancho_rainbow();
        recalcular_get_total_alto_rainbow();


        init_rainbow();
        init_cache_putpixel();

	if (diviface_enabled.v) diviface_set_peek_poke_functions();
}



//Establecer la mayoria de registros a valores indefinidos (a 255), que asi se establecen al arrancar una maquina
//al pulsar el boton de reset aqui no se llama
void cold_start_cpu_registers(void)
{

	//Probar RANDOMIZE USR 46578 y debe aparecer pantalla con colores y al pulsar Espacio, franjas en el borde
	//Si esto se ejecuta despues de un reset, no sucede
	//Ver http://foro.speccy.org/viewtopic.php?f=11&t=2319
	//y http://www.worldofspectrum.org/forums/discussion/comment/539714#Comment_539714

        /*
        AF=BC=DE=HL=IX=IY=SP=FFFFH
        AF'=BC'=DE'=HL'=FFFFH
        IR=0000H
        */

        reg_a=0xff;
        Z80_FLAGS=0xff;
        BC=HL=DE=0xffff;
        reg_ix=reg_iy=reg_sp=0xffff;

        reg_h_shadow=reg_l_shadow=reg_b_shadow=reg_c_shadow=reg_d_shadow=reg_e_shadow=reg_a_shadow=Z80_FLAGS_SHADOW=0xff;
        reg_i=0;
        reg_r=reg_r_bit7=0;

	out_254_original_value=out_254=0xff;

	//Parece que en Inves esto es asi:
	if (MACHINE_IS_INVES) {
		out_254_original_value=out_254=0;
	}

	modificado_border.v=1;

	//Modo BOOTM y cambio de otros settings del menu emulador
	if (MACHINE_IS_ZXUNO) {

		//activar BOOTM
		zxuno_ports[0]=1;

		//metemos registro SCRATCH / COLDBOOT a 0
		zxuno_ports[0xfe]=0;


        	zxuno_ports[0x0B]=0;
        	zxuno_ports[0x0C]=255;
	        zxuno_ports[0x0D]=1;
        	zxuno_ports[0x0E]=0;
	        zxuno_ports[0x0F]=0;
        	zxuno_ports[0x40]=0;



		zxuno_set_emulator_setting_i2kb();
		zxuno_set_emulator_setting_timing();
		zxuno_set_emulator_setting_contend();
		zxuno_set_emulator_setting_diven();
		zxuno_set_emulator_setting_disd();
		zxuno_set_emulator_setting_devcontrol_diay();
		zxuno_set_emulator_setting_devcontrol_ditay();
		zxuno_set_emulator_setting_scandblctrl();
		zxuno_set_emulator_setting_ditimex();
		zxuno_set_emulator_setting_diulaplus();

		//quitamos write enable de la spi flash zxuno
		zxuno_spi_clear_write_enable();
	}

	if (MACHINE_IS_PRISM) {
		hard_reset_cpu_prism();
		prism_set_emulator_setting_cpuspeed();
	}

	if (MACHINE_IS_TBBLUE) {
		tbblue_hard_reset();
	}
}


//Para maquinas Z88 y zxuno y prism
void hard_reset_cpu(void)
{
	if (MACHINE_IS_Z88) hard_reset_cpu_z88();

	else if (MACHINE_IS_ZXUNO) {
		hard_reset_cpu_zxuno();
	}

	else if (MACHINE_IS_PRISM) {
		hard_reset_cpu_prism();
		reset_cpu();
	}

	else if (MACHINE_IS_TBBLUE) {
    tbblue_hard_reset();
		reset_cpu();
  }

	else if (superupgrade_enabled.v) {
		superupgrade_hard_reset();
		reset_cpu();
	}

}

void reset_cpu(void)
{

	debug_printf (VERBOSE_INFO,"Reset cpu");

	if (rzx_reproduciendo) {
		eject_rzx_file();
	}

	reg_pc=0;
	reg_i=0;

	//mapear rom 0 en modos 128k y paginas RAM normales
	puerto_32765=0;
	puerto_8189=0;

	zesarux_zxi_last_register=0;
	zesarux_zxi_registers_array[0]=0;


	interrupcion_maskable_generada.v=0;
	interrupcion_non_maskable_generada.v=0;
        interrupcion_timer_generada.v=0;
	iff1.v=iff2.v=0;
        im_mode=0;

	if1_rom_paged.v=0;


	//Algunos otros registros
	reg_a=0xff;
	Z80_FLAGS=0xff;
	reg_sp=0xffff;


	diviface_control_register&=(255-128);
	diviface_paginacion_automatica_activa.v=0;


	//si no se pone esto a 0, al cambiar de zx80 a zx81 suele colgarse
        z80_ejecutando_halt.v=0;


        esperando_tiempo_final_t_estados.v=0;


	if (MACHINE_IS_ZX8081) {
		//algunos reseteos para zx80/81
		nmi_generator_active.v=0;
                hsync_generator_active.v=0;
		timeout_linea_vsync=NORMAL_TIMEOUT_LINEA_VSYNC;
		chroma81_port_7FEF=0;

		//Prueba ajuste por defecto, para intentar eliminar lnctr
		//if (MACHINE_IS_ZX80) video_zx8081_lnctr_adjust.v=0;
		//else if (MACHINE_IS_ZX81) video_zx8081_lnctr_adjust.v=1;

		//Si zxpand, habilitar overlay rom
		if (zxpand_enabled.v) {
			zxpand_overlay_rom.v=1;
			dragons_lair_hack.v=0;
		}

	}


	if (MACHINE_IS_SPECTRUM_128_P2) {
		mem_set_normal_pages_128k();
	}

	if (MACHINE_IS_SPECTRUM_P2A) {
		mem_set_normal_pages_p2a();
	}

	if (MACHINE_IS_ZXUNO) {
		mem_set_normal_pages_zxuno();

		//Desactivar flags interrupciones raster
		zxuno_ports[0x0d]=0;

		//indice de mensaje a coreid
		zxuno_core_id_indice=0;
	}

	//Modos extendidos ulaplus desactivar, sea en maquina zxuno o no
	zxuno_ports[0x40]=0;


	if (MACHINE_IS_Z88) {
		z88_set_default_memory_pages();
		z88_snooze.v=0;
		z88_coma.v=0;

		blink_tim[0] = 0x98;
		blink_tim[1] = blink_tim[2] = blink_tim[3] = blink_tim[4] = 0;

		//registros de video a 0 tambien
		blink_pixel_base[0]=blink_pixel_base[1]=blink_pixel_base[2]=blink_pixel_base[3]=0;
		blink_sbr=0;

		//resetear speaker. por si acaso se queda activo el sonido a 3200 khz
		//7           SRUN        Speaker source (0=SBIT, 1=TxD or 3200Khz
		blink_com &= (255-128);

	}

	t_estados=0;
	t_scanline=0;
	t_scanline_draw=0;

        if (MACHINE_IS_INVES) {
		//Inves
		t_scanline_draw=screen_indice_inicio_pant;
        }

	init_chip_ay();

#ifdef EMULATE_CPU_STATS
util_stats_init();
#endif



	if (MACHINE_IS_SPECTRUM) {
		//Modo spectra 0
		spectra_display_mode_register=0;

		if (ulaplus_presente.v) ulaplus_set_mode(0);
		if (ulaplus_presente.v) ulaplus_set_extended_mode(0);
	}


	//Resetear modo timex
	timex_port_ff=0;

	//Resetear paginacion timex
	timex_port_f4=0;


	if (MACHINE_IS_CHLOE) chloe_set_memory_pages();

	if (MACHINE_IS_PRISM) {
		//Cambiar pagina rom tal cual como si pusiesemos bit de rom de puerto 32765 a 0 y el bit de 8189 tambien a 0
		//Asi por ejemplo, si desde prism vamos a maquina +2A, luego vamos a 48 Basic,
		//haciendo un reset normal, volvemos a menu +2A. Sino cambiasemos la pagina de rom asi,
		//al hacer reset volveria a 48 Basic

		//desactivado
		//porque reset desde maquina 48 (rom page 2) lo envia a prism boot
		//		prism_rom_page &=(255-3);


		prism_set_memory_pages();
	}

	if (MACHINE_IS_TBBLUE) {
		tbblue_set_memory_pages();
		//tbblue_read_port_24d5_index=0;
	}

	if (MACHINE_IS_TIMEX_TS2068) timex_set_memory_pages();

	if (MACHINE_IS_CPC) {
		cpc_gate_registers[0]=cpc_gate_registers[1]=cpc_gate_registers[2]=cpc_gate_registers[3]=0;
		cpc_set_memory_pages();
		cpc_scanline_counter=0;
	}

	if (MACHINE_IS_SAM) {
		sam_vmpr=sam_hmpr=sam_lmpr=0;

		sam_set_memory_pages();
	}


        if (mmc_enabled.v) mmc_reset();
        if (ide_enabled.v) ide_reset();

	if (superupgrade_enabled.v) superupgrade_set_memory_pages();

	ay_player_playing.v=0;

	if (multiface_enabled.v) {
		multiface_lockout=0;
		multiface_unmap_memory();
	}

	if (MACHINE_IS_QL) {
        m68k_init();
        m68k_set_cpu_type(M68K_CPU_TYPE_68000);
        m68k_pulse_reset();

		//HWReset();
		//printf ("Reg PC QL: %08XH\n",pc);
		//sleep(2);
		ql_ipc_reset();
	}

}



void cpu_help(void)
{
	printf ("Usage:\n"
		"[--tape] file      Insert input standard tape file. Supported formats: Spectrum: .TAP, .TZX -- ZX80: .O, .80, .Z81 -- ZX81: .P, .81, .Z81 -- All machines: .RWA, .SMP, .WAV\n"
		"[--realtape] file  Insert input real tape file. Supported formats: Spectrum: .TAP, .TZX -- ZX80: .O, .80, .Z81 -- ZX81: .P, .81, .Z81 -- All machines: .RWA, .SMP, .WAV\n"
		"[--snap] file      Load snapshot file. Supported formats: Spectrum: .Z80, .ZX, .SP, .SNA -- ZX80: .ZX, .O, .80 -- ZX81: .ZX, .P, .81, .Z81\n"
		"[--slotcard] file  Insert Z88 EPROM/Flash file. Supported formats: .EPR, .63, .EPROM, .FLASH\n"
		"Note: if you write a tape/snapshot/card file name without --tape, --realtape, --snap or --slotcard parameters, the emulator will try to guess file type (it's the same as SmartLoad on the menu)\n"
		"\n"

		"--outtape file     Insert output standard tape file. Supported formats: Spectrum: .TAP, .TZX -- ZX80: .O -- ZX81: .P\n"

		"--zoom n           Total Zoom Factor\n"
		"--vo driver        Video output driver. Valid drivers: ");

#ifdef USE_COCOA
	printf ("cocoa ");
#endif

#ifdef COMPILE_XWINDOWS
	printf ("xwindows ");
#endif

#ifdef COMPILE_SDL
        printf ("sdl ");
#endif


#ifdef COMPILE_FBDEV
	printf ("fbdev ");
#endif


#ifdef COMPILE_CACA
        printf ("caca ");
#endif

#ifdef COMPILE_AA
        printf ("aa ");
#endif

#ifdef COMPILE_CURSES
	printf ("curses ");
#endif


#ifdef COMPILE_STDOUT
	printf ("stdout ");
#endif

#ifdef COMPILE_SIMPLETEXT
        printf ("simpletext ");
#endif


	printf ("null\n");


	printf ("--vofile file      Also output video to raw file\n");
	printf ("--vofilefps n      FPS of the output video [1|2|5|10|25|50] (default:5)\n");



	printf ("--ao driver        Audio output driver. Valid drivers: ");

#ifdef COMPILE_PULSE
        printf ("pulse ");
#endif

#ifdef COMPILE_ALSA
        printf ("alsa ");
#endif

#ifdef COMPILE_SDL
        printf ("sdl ");
#endif

#ifdef COMPILE_DSP
        printf ("dsp ");
#endif

#ifdef COMPILE_COREAUDIO
        printf ("coreaudio ");
#endif


        printf ("null\n");




#ifdef USE_SNDFILE
	printf ("--aofile file      Also output sound to wav or raw file\n");
#else
	printf ("--aofile file      Also output sound to raw file\n");
#endif

	printf ("\n");

	printf ("--machine          Machine type: \n"
							" ZX80     ZX-80\n"
							" ZX81     ZX-81\n"
              " 16k      Spectrum 16k\n"
              " 48k      Spectrum 48k\n"
              " 48ks     Spectrum 48k (Spanish)\n"
              " Inves    Inves Spectrum+\n"
					    " TK90X    Microdigital TK90X\n"
					    " TK90XS   Microdigital TK90X (Spanish)\n"
					    " TK95     Microdigital TK95\n"
              " 128k     Spectrum 128k\n"
              " 128ks    Spectrum 128k (Spanish)\n"
              " P2       Spectrum +2\n"
              " P2F      Spectrum +2 (French)\n"
              " P2S      Spectrum +2 (Spanish)\n"
              " P2A40    Spectrum +2A (ROM v4.0)\n"
              " P2A41    Spectrum +2A (ROM v4.1)\n"
              " P2AS     Spectrum +2A (Spanish)\n"
							" QL       QL\n"
							" Z88      Cambridge Z88\n"
							" TS2068   Timex TS 2068\n"
							" Sam      Sam Coupe\n"
					    " Pentagon Pentagon 128\n"
							" Chloe140 Chloe 140 SE\n"
							" Chloe280 Chloe 280 SE\n"
							" ZXUNO    ZX-Uno\n"
							" Prism    Prism\n"
							" TBBlue   TBBlue/ZX Spectrum Next\n"
					    " ACE      Jupiter Ace\n"
					    " CPC464   Amstrad CPC 464\n"

					"\n"
		"--noconfigfile     Do not load configuration file. This parameter must be the first and it's ignored if written on config file\n"
		"--experthelp       Show expert options\n"
		"\n"
		"Any command line setting shown here or on experthelp can be written on a configuration file,\n"
		"this configuration file is on your home directory with name: " ZESARUX_CONFIG_FILE "\n"

		"\n"

#ifdef MINGW
		"Note: On Windows command prompt, all emulator messages are supressed (except the initial copyright message).\n"
		"To avoid this, you must specify at least one parameter on command line.\n\n"
#endif

              );

}

void cpu_help_expert(void)
{

	printf ("Expert options, in sections: \n"
		"\n"
		"\n"
		"Debugging\n"
		"---------\n"
		"\n"
		"--verbose n                Verbose level n (0=only errors, 1=warning and errors, 2=info, warning and errors, 3=debug, 4=lots of messages)\n"
		"--debugregisters           Debug CPU Registers on text console\n"
	        "--showcompileinfo          Show compilation information\n"
		"--debugconfigfile          Debug parsing of configuration file (and .config files). This parameter must be the first and it's ignored if written on config file\n"







		"--romfile file             Select custom ROM file\n"
		"--loadbinary file addr len Load binary file \"file\" at address \"addr\" with length \"len\". Set ln to 0 to load the entire file in memory\n"
		"--loadbinarypath path      Select initial Load Binary path\n"
		"--keyboardspoolfile file   Insert spool file for keyboard presses\n"
#ifdef USE_PTHREADS
		"--enable-remoteprotocol    Enable remote protocol\n"
		"--remoteprotocol-port n    Set remote protocol port (default: 10000)\n"
#endif

#ifdef MINGW
		"--nodisableconsole         Do not disable text output on this console. On Windows, text output is disabled unless you specify "
		"at least one parameter on command line, or this parameter on command line or on configuration file. \n"
#endif

);

printf(
		"--set-breakpoint n s       Set breakpoint with string s at position n. n must be between 1 and %d. string s must be written in \"\" if has spaces\n",MAX_BREAKPOINTS_CONDITIONS
);


printf (
	  "--hardware-debug-ports     Enables hardware debug ports to be able to show on console numbers or ascii characters\n"
		"\n"
		"\n"
		"CPU Core\n"
		"--------\n"
		"\n"


		"--cpuspeed n               Set CPU speed in percentage\n"
		"--denyturbozxunoboot       Deny setting turbo mode on ZX-Uno boot\n"

		"\n"
		"\n"
		"Tapes, Snapshots\n"
		"----------------\n"
		"\n"
		"--noautoload               No autoload tape file on Spectrum, ZX80 or ZX81\n"
		"--noautoselectfileopt      Do not autoselect emulation options for known snap and tape files\n"
		"--simulaterealload         Simulate real tape loading\n"
		"--simulaterealloadfast     Enable fast real tape loading\n"
		"--smartloadpath path       Select initial smartload path\n"
		"--autoloadsnap             Load last snapshot on start\n"
		"--autosavesnap             Save snapshot on exit\n"
		"--autosnappath path        Folder to save/load automatic snapshots\n"

		"\n"
		"\n"
		"Audio Features\n"
		"--------------\n"
		"\n"

		"--disableayspeech          Disable AY Speech sounds\n"
		"--disableenvelopes         Disable AY Envelopes\n"
		"--disablebeeper            Disable Beeper\n"
    "--disablerealbeeper        Disable real Beeper sound\n"
		"--enableturbosound         Enable Turbo Sound emulation\n"
		"--enablespecdrum           Enable Specdrum emulation\n"
		"--audiovolume n            Sets the audio output volume to percentage n\n"
		"--zx8081vsyncsound         Enable vsync/tape sound on ZX80/81\n"

		"--ayplayer-end-exit        Exit emulator when end playing current ay file\n"
		"--ayplayer-end-no-repeat   Do not repeat playing from the beginning when end playing current ay file\n"
		"--ayplayer-inf-length n    Limit to n seconds to ay tracks with infinite lenght\n"
		"--ayplayer-any-length n    Limit to n seconds to all ay tracks\n"
		"--ayplayer-cpc             Set AY Player to CPC mode (default: Spectrum)\n"


		"\n"
		"\n"
		"Audio Driver Settings\n"
		"---------------------\n"
		"\n"


#ifdef COMPILE_ALSA
		"--alsaperiodsize n         Alsa audio periodsize multiplier (2 or 4). Default 2. Lower values reduce latency but can increase cpu usage\n"


#ifdef USE_PTHREADS

		"--fifoalsabuffersize n     Alsa fifo buffer size multiplier (4 to 10). Default 4. Lower values reduce latency but can increase cpu usage\n"

#endif
#endif



#ifdef COMPILE_PULSE
#ifdef USE_PTHREADS
                "--pulseperiodsize n        Pulse audio periodsize multiplier (1 to 4). Default 1. Lower values reduce latency but can increase cpu usage\n"
                "--fifopulsebuffersize n    Pulse fifo buffer size multiplier (4 to 10). Default 10. Lower values reduce latency but can increase cpu usage\n"
#endif
#endif


#ifdef COMPILE_SDL
		);


		//Cerramos el printf para que quede mas claro que hacemos un printf con un parametro
		printf (
		"--sdlsamplesize n          SDL audio sample size (128 to 2048). Default %d. Lower values reduce latency but can increase cpu usage\n",DEFAULT_AUDIOSDL_SAMPLES);



		printf (
#endif



		"\n"
		"\n"
		"Video Features\n"
		"--------------\n"
		"\n"

		"--realvideo                Enable real video display - for Spectrum (rainbow and other advanced effects) and ZX80/81 (non standard & hi-res modes)\n"
		"--snoweffect               Enable snow effect support for Spectrum\n"
		"--enableulaplus            Enable ULAplus video modes\n"
		"--enablespectra            Enable Spectra video modes\n"
		"--enabletimexvideo         Enable Timex video modes\n"
		"--enablezgx                Enable ZGX Sprite chip\n"
		"--autodetectwrx            Enable WRX autodetect setting on ZX80/ZX81\n"
		"--wrx                      Enable WRX mode on ZX80/ZX81\n"
		"--vsync-minimum-length n   Set ZX80/81 Vsync minimum lenght in t-states (minimum 100, maximum 999)\n"
		"--chroma81                 Enable Chroma81 support on ZX80/ZX81\n"
		"--videozx8081 n            Emulate ZX80/81 Display on Spectrum. n=pixel threshold (1..16. 4=normal)\n"
		"--videofastblack           Emulate black screen on fast mode on ZX80/ZX81\n"
		"--scr file                 Load Screen File at startup\n"



		"\n"
		"\n"
		"Video Driver Settings\n"
		"---------------------\n"
		"\n"


#ifdef USE_XEXT
	        "--disableshm               Disable X11 Shared Memory\n"
#endif

		"--nochangeslowparameters   Do not change any performance parameters (frameskip, realvideo, etc) "
		"on slow machines like raspberry, etc\n"

#ifdef COMPILE_AA
                "--aaslow                   Use slow rendering on aalib\n"
#endif

	        "--arttextthresold n        Pixel threshold for artistic emulation for curses & stdout & simpletext (1..16. 4=normal)\n"
	        "--disablearttext           Disable artistic emulation for curses & stdout & simpletext\n"
		"--autoredrawstdout         Enable automatic display redraw for stdout & simpletext drivers\n"
		"--sendansi                 Sends ANSI terminal control escape sequences for stdout & simpletext drivers\n"

#ifdef COMPILE_FBDEV
                "--no-use-ttyfbdev          Do not use a tty on fbdev driver. It disables keyboard\n"
                "--no-use-ttyrawfbdev       Do not use keyboard on raw mode for fbdev driver\n"
                "--use-all-res-fbdev        Use all virtual resolution on fbdev driver. Experimental feature\n"
                "--decimal-full-scale-fbdev Use non integer zoom to fill the display with full screen mode on fbdev driver\n"
#ifdef EMULATE_RASPBERRY
                "--fbdev-margin-width n     Increment fbdev width size on n pixels on Raspberry full screen\n"
                "--fbdev-margin-height n    Increment fbdev width height on n pixels on Raspberry full screen\n"
#endif

#endif


		"\n"
		"\n"
		"GUI Settings\n"
		"---------------------\n"
		"\n"


		//"--invespokerom n           Inves rom poke value\n"

		"--zoomx n                  Horizontal Zoom Factor\n"
		"--zoomy n                  Vertical Zoom Factor\n"
		"--frameskip n              Set frameskip (0=none, 1=25 FPS, 2=16 FPS, etc)\n"
		"--disable-autoframeskip    Disable autoframeskip\n"


		"--fullscreen               Enable full screen\n"
		"--disableborder            Disable Border\n"
		"--hidemousepointer         Hide Mouse Pointer. Not all video drivers support this\n"

		//"--overlayinfo              Overlay on screen some machine info, like when loading tape\n"

		"--disablefooter            Disable window footer\n"
		"--disablemultitaskmenu     Disable multitasking menu\n"
		"--nosplash                 Disable all splash texts\n"
		"--nowelcomemessage         Disable welcome message\n"
		"--red                      Force display mode with red colour\n"
		"--green                    Force display mode with green colour\n"
		"--blue                     Force display mode with blue colour\n"
		"  Note: You can combine colours, for example, --red --green for Yellow display, or --red --green --blue for Gray display\n"
		"--inversevideo             Inverse display colours\n"
		"--disabletooltips          Disable tooltips on menu\n"
		"--forcevisiblehotkeys      Force always show hotkeys. By default it will only be shown after a timeout or wrong key pressed\n"
		"--forceconfirmyes          Force confirmation dialogs yes/no always to yes\n"
		"--gui-style s              Set GUI style. Available: ");

		estilo_gui_retorna_nombres();


printf (
		"\n"
		"--disablemenu              Disable menu. Any event that opens the menu will exit the emulator\n"
		"\n"
		"\n"
		"Hardware Settings\n"
		"-----------------\n"
		"\n"

		"--printerbitmapfile f      Sends printer output to image file. Supported formats: pbm, txt\n"
		"--printertextfile f        Sends printer output to text file using OCR method. Printer output is saved to a text file using OCR method to guess text.\n"
		"--redefinekey src dest     Redefine key scr to be key dest. You can write maximum 10 redefined keys\n"
                "                           Key must be ascii character numbers or a character included in escaped quotes, like: 97 (for 'a') or \\'q\\'\n"
                "                           (the escaped quotes are used only in command line; on configuration file, they are normal quotes '')\n"



		"\n"
                "\n"
                "Memory Settings\n"
                "-----------------\n"
                "\n"

                "--zx8081mem n              Emulate 1,2,...16 kb of memory on ZX80/ZX81\n"
                "--zx8081ram8K2000          Emulate 8K RAM in 2000H for ZX80/ZX81\n"
                "--zx8081ram16K8000         Emulate 16K RAM in 8000H for ZX80/ZX81\n"
                "--zx8081ram16KC000         Emulate 16K RAM in C000H for ZX80/ZX81\n"
		"--acemem n                 Emulate 3, 19 or 35 kb of memory on Jupiter Ace\n"



		"\n"
		"\n"
		"Print char traps & Text to Speech\n"
		"---------------------------------\n"
		"\n"


		"--enableprintchartrap      Enable traps for standard ROM print char calls and non standard second & third traps. On Spectrum, ZX80, ZX81 machines, standard ROM calls are those using RST10H. On Z88, are those using OS_OUT and some other functions. Note: it is enabled by default on stdout & simpletext drivers\n"
		"--disableprintchartrap     Disable traps for ROM print char calls and second & third traps.\n"

		"--linewidth n              Print char line width\n"

		"--automaticdetectchar      Enable automatic detection & try all method to find print character routines, for games not using RST 10H\n"
		"--secondtrapchar n         Print Char second trap address\n"
		"--secondtrapsum32          Print Char second trap sum 32 to character\n"
		"--thirdtrapchar n          Print Char third trap address\n"
		"--linewidthwaitspace       Print Char wait for a space, or a comma, or a dot, or a semicolon, to end a line\n"
		"--textspeechprogram p      Specify a path to a program or script to be sent the emulator text shown. For example, for text to speech: speech_filters/festival_filter.sh or speech_filters/macos_say_filter.sh\n"
		"--textspeechstopprogram p  Specify a path to a program or script in charge of stopping the running speech program. For example, speech_filters/stop_festival_filter.sh\n"
		"--textspeechwait           Wait and pause the emulator until the Speech program returns\n"
		"--textspeechmenu           Also send text menu entries to Speech program\n"
		"--textspeechtimeout n      After some seconds the text will be sent to the Speech program when no new line is sent. Between 0 and 99. 0 means never\n"




		"\n"
		"\n"
		"Storage Settings\n"
		"----------------\n"
		"\n"


		"--mmc-file f               Set mmc image file\n"
		"--enable-mmc               Enable MMC emulation. Usually requires --mmc-file\n"
		"--enable-divmmc-ports      Enable DIVMMC emulation ports only, but not paging. Usually requires --enable-mmc\n"
		"--enable-divmmc            Enable DIVMMC emulation: ports & paging. Usually requires --enable-mmc\n"
		"--divmmc-rom f             Sets divmmc firmware rom. If not set, uses default file\n"
		"--enable-zxmmc             Enable ZXMMC emulation. Usually requires --enable-mmc\n"
		"--enable-zxpand            Enable ZXpand emulation\n"
		"--zxpand-root-dir p        Set ZXpand root directory for sd/mmc filesystem. Uses current directory by default.\n"
		"                           Note: ZXpand does not use --mmc-file setting\n"

                "--ide-file f               Set ide image file\n"
                "--enable-ide               Enable IDE emulation. Usually requires --ide-file\n"
                "--enable-divide            Enable DIVIDE emulation. Usually requires --enable-ide\n"
								"--divide-rom f             Sets divide firmware rom. If not set, uses default file\n"
		"--enable-8bit-ide          Enable 8-bit simple IDE emulation. Requires --enable-ide\n"
                "--dandanator-rom f         Set ZX Dandanator rom file\n"
		"--zxunospifile path        File to use on ZX-Uno as SPI Flash. Default: zxuno.flash\n"
		"--zxunospiwriteenable      Enable writing to disk on ZX-Uno SPI Flash\n"
                "--enable-dandanator        Enable ZX Dandanator emulation. Requires --dandanator-rom\n"
                "--dandanator-press-button  Simulates pressing button on ZX Dandanator. Requires --enable-dandanator\n"
		"--superupgrade-flash f     Set Superupgrade flash file\n"
		"--enable-superupgrade      Enable Superupgrade emulation. Requires --superupgrade-flash\n"




		"\n"
                "\n"
                "External Tools\n"
                "--------------\n"
                "\n"


                "--tool-sox-path p          Set external tool sox path. Path can not include spaces\n"
                "--tool-unzip-path p        Set external tool unzip path. Path can not include spaces\n"
                "--tool-gunzip-path p       Set external tool gunzip path. Path can not include spaces\n"
                "--tool-tar-path p          Set external tool tar path. Path can not include spaces\n"
                "--tool-unrar-path p        Set external tool unrar path. Path can not include spaces\n"


		"\n"
		"\n"
		"Joystick\n"
		"--------\n"
		"\n"


		"--joystickemulated type    Type of joystick emulated. Type can be one of: ");

	joystick_print_types();
		printf (" (default: %s).\n",joystick_texto[joystick_emulation]);
		printf ("  Note: if a joystick type has spaces in its name, you must write it between \"\"\n");


	printf(""
		"--disablerealjoystick      Disable real joystick emulation\n"

                "--joystickevent but evt    Set a joystick button or axis to an event (changes joystick to event table)\n"
                "                           If it's a button (not axis), must be specified with its number, without sign, for example: 2\n"
                "                           If it's axis, must be specified with its number and sign, for example: +2 or -2\n"
                "                           Event must be one of: ");

                realjoystick_print_event_keys();

	printf ("\n"
		"--joystickkeybt but key    Define a key pressed when a joystick button pressed (changes joystick to key table)\n"
		"                           If it's a button (not axis), must be specified with its number, without sign, for example: 2\n"
                "                           If it's axis, must be specified with its number and sign, for example: +2 or -2\n"
                "                           Key must be an ascii character number or a character included in escaped quotes, like: 13 (for enter) or \\'q\\'\n"
		"                           (the escaped quotes are used only in command line; on configuration file, they are normal quotes '')\n"



                "--joystickkeyev evt key    Define a key pressed when a joystick event is generated (changes joystick to key table)\n"
                "                           Event must be one of: ");

                realjoystick_print_event_keys();


	printf ("\n"
		"                           Key must be an ascii character number or a character included in escaped quotes, like: 13 (for enter) or \\'q\\' \n"
		"                           (the escaped quotes are used only in command line; on configuration file, they are normal quotes '')\n"

        	"\n"
		"  Note: As you may see, --joystickkeyev is not dependent on the real joystick type you use, because it sets an event to a key, "
		"and --joystickkeybt and --joystickevent are dependent on the real joystick type, because they set a button/axis number to "
		"an event or key, and button/axis number changes depending on the joystick (the exception here is the axis up/down/left/right "
		"which are the same for all joysticks: up: -1, down: +1, left: -1, right: +1)\n"

		"\n"
		"--clearkeylistonsmart      Clear all joystick (events and buttons) to keys table every smart loading.\n"
		"                           Joystick to events table is never cleared using this setting\n"
		"--cleareventlist           Clears joystick to events table\n"



		"\n"
		"\n"
		"Miscellaneous\n"
		"-------------\n"
		"\n"



		"--saveconf-on-exit         Always save configuration when exiting emulator\n"
		"--helpcustomconfig         Show help for autoconfig files\n"
		"--quickexit                Exit emulator quickly: no yes/no confirmation and no fadeout\n"


		"\n\n"

	);

}

#ifdef USE_COCOA
int set_scrdriver_cocoa(void)
{
                        scr_refresca_pantalla=scrcocoa_refresca_pantalla;
												scr_refresca_pantalla_solo_driver=scrcocoa_refresca_pantalla_solo_driver;
                        scr_init_pantalla=scrcocoa_init;
                        scr_end_pantalla=scrcocoa_end;
                        scr_lee_puerto=scrcocoa_lee_puerto;
                        scr_actualiza_tablas_teclado=scrcocoa_actualiza_tablas_teclado;
        return 0;
}

#endif


#ifdef COMPILE_XWINDOWS
int set_scrdriver_xwindows(void)
{
                        scr_refresca_pantalla=scrxwindows_refresca_pantalla;
												scr_refresca_pantalla_solo_driver=scrxwindows_refresca_pantalla_solo_driver;
                        scr_init_pantalla=scrxwindows_init;
                        scr_end_pantalla=scrxwindows_end;
                        scr_lee_puerto=scrxwindows_lee_puerto;
                        scr_actualiza_tablas_teclado=scrxwindows_actualiza_tablas_teclado;
                        //scr_debug_registers=scrxwindows_debug_registers;
			//scr_messages_debug=scrxwindows_messages_debug;
	return 0;
}

#endif

#ifdef COMPILE_SDL
int set_scrdriver_sdl(void)
{
                        scr_refresca_pantalla=scrsdl_refresca_pantalla;
												scr_refresca_pantalla_solo_driver=scrsdl_refresca_pantalla_solo_driver;
                        scr_init_pantalla=scrsdl_init;
                        scr_end_pantalla=scrsdl_end;
                        scr_lee_puerto=scrsdl_lee_puerto;
                        scr_actualiza_tablas_teclado=scrsdl_actualiza_tablas_teclado;
        return 0;
}

#endif



#ifdef COMPILE_FBDEV
int set_scrdriver_fbdev(void)
{
                        scr_refresca_pantalla=scrfbdev_refresca_pantalla;
												scr_refresca_pantalla_solo_driver=scrfbdev_refresca_pantalla_solo_driver;
                        scr_init_pantalla=scrfbdev_init;
                        scr_end_pantalla=scrfbdev_end;
                        scr_lee_puerto=scrfbdev_lee_puerto;

			//la rutina de tabla teclado la establecemos en el init... pues se cambia segun si modo raw o no
                        //scr_actualiza_tablas_teclado=scrfbdev_actualiza_tablas_teclado;
                        //scr_debug_registers=scrfbdev_debug_registers;
                        //scr_messages_debug=scrfbdev_messages_debug;
        return 0;
}

#endif


#ifdef COMPILE_CURSES
int set_scrdriver_curses(void)
{
//asignar pantalla curses
scr_refresca_pantalla=scrcurses_refresca_pantalla;
scr_refresca_pantalla_solo_driver=scrcurses_refresca_pantalla_solo_driver;
scr_init_pantalla=scrcurses_init;
scr_end_pantalla=scrcurses_end;
scr_lee_puerto=scrcurses_lee_puerto;
scr_actualiza_tablas_teclado=scrcurses_actualiza_tablas_teclado;
//scr_debug_registers=scrcurses_debug_registers;
//			scr_messages_debug=scrcurses_messages_debug;
	return 0;
}
#endif

#ifdef COMPILE_AA
int set_scrdriver_aa(void)
{
//asignar pantalla aa
scr_refresca_pantalla=scraa_refresca_pantalla;
scr_refresca_pantalla_solo_driver=scraa_refresca_pantalla_solo_driver;
scr_init_pantalla=scraa_init;
scr_end_pantalla=scraa_end;
scr_lee_puerto=scraa_lee_puerto;
scr_actualiza_tablas_teclado=scraa_actualiza_tablas_teclado;


//scr_debug_registers=scraa_debug_registers;
//scr_messages_debug=scraa_messages_debug;
	return 0;
}
#endif

#ifdef COMPILE_CACA
int set_scrdriver_caca(void)
{
//asignar pantalla caca
scr_refresca_pantalla=scrcaca_refresca_pantalla;
scr_refresca_pantalla_solo_driver=scrcaca_refresca_pantalla_solo_driver;
scr_init_pantalla=scrcaca_init;
scr_end_pantalla=scrcaca_end;
scr_lee_puerto=scrcaca_lee_puerto;
scr_actualiza_tablas_teclado=scrcaca_actualiza_tablas_teclado;
//scr_debug_registers=scrcaca_debug_registers;
//			scr_messages_debug=scrcaca_messages_debug;
	return 0;
}
#endif




int set_scrdriver_null(void) {
scr_refresca_pantalla=scrnull_refresca_pantalla;
scr_refresca_pantalla_solo_driver=scrnull_refresca_pantalla_solo_driver;
scr_init_pantalla=scrnull_init;
scr_end_pantalla=scrnull_end;
scr_lee_puerto=scrnull_lee_puerto;
scr_actualiza_tablas_teclado=scrnull_actualiza_tablas_teclado;
//scr_debug_registers=scrnull_debug_registers;
//			scr_messages_debug=scrnull_messages_debug;
	return 0;
}

#ifdef COMPILE_STDOUT

int set_scrdriver_stdout(void) {
scr_refresca_pantalla=scrstdout_refresca_pantalla;
scr_refresca_pantalla_solo_driver=scrstdout_refresca_pantalla_solo_driver;
scr_init_pantalla=scrstdout_init;
scr_end_pantalla=scrstdout_end;
scr_lee_puerto=scrstdout_lee_puerto;
scr_actualiza_tablas_teclado=scrstdout_actualiza_tablas_teclado;
//scr_debug_registers=scrstdout_debug_registers;
//                      scr_messages_debug=scrstdout_messages_debug;
        return 0;
}

#endif

#ifdef COMPILE_SIMPLETEXT

int set_scrdriver_simpletext(void) {
scr_refresca_pantalla=scrsimpletext_refresca_pantalla;
scr_refresca_pantalla_solo_driver=scrsimpletext_refresca_pantalla_solo_driver;
scr_init_pantalla=scrsimpletext_init;
scr_end_pantalla=scrsimpletext_end;
scr_lee_puerto=scrsimpletext_lee_puerto;
scr_actualiza_tablas_teclado=scrsimpletext_actualiza_tablas_teclado;

        return 0;
}

#endif



#ifdef COMPILE_DSP
int set_audiodriver_dsp(void) {
                        audio_init=audiodsp_init;
                        audio_send_frame=audiodsp_send_frame;
			audio_thread_finish=audiodsp_thread_finish;
			audio_end=audiodsp_end;
			return 0;

                }
#endif

#ifdef COMPILE_SDL
int set_audiodriver_sdl(void) {
                        audio_init=audiosdl_init;
                        audio_send_frame=audiosdl_send_frame;
                        audio_thread_finish=audiosdl_thread_finish;
                        audio_end=audiosdl_end;
                        return 0;

                }
#endif


#ifdef COMPILE_ALSA
int set_audiodriver_alsa(void) {
                        audio_init=audioalsa_init;
                        audio_send_frame=audioalsa_send_frame;
			audio_thread_finish=audioalsa_thread_finish;
			audio_end=audioalsa_end;
			return 0;

                }
#endif

#ifdef COMPILE_PULSE
int set_audiodriver_pulse(void) {
                        audio_init=audiopulse_init;
                        audio_send_frame=audiopulse_send_frame;
                        audio_thread_finish=audiopulse_thread_finish;
			audio_end=audiopulse_end;
                        return 0;

                }
#endif


#ifdef COMPILE_COREAUDIO
int set_audiodriver_coreaudio(void) {
                        audio_init=audiocoreaudio_init;
                        audio_send_frame=audiocoreaudio_send_frame;
                        audio_thread_finish=audiocoreaudio_thread_finish;
			audio_end=audiocoreaudio_end;
                        return 0;

                }
#endif

//Randomize Usando segundos del reloj
//Usado en AY Chip y Random ram
void init_randomize_noise_value(void)
{

                int chips=ay_retorna_numero_chips();

                int i;

                for (i=0;i<chips;i++) {

	                gettimeofday(&z80_interrupts_timer_antes, NULL);
        	        randomize_noise[i]=z80_interrupts_timer_antes.tv_sec & 0xFFFF;
                	//printf ("randomize vale: %d\n",randomize_noise);

		}
}


void random_ram(z80_byte *puntero,int longitud)
{

	//Cada vez que se inicializa una maquina, reasignar valor randomize
	init_randomize_noise_value();

	z80_byte valor;

	z80_byte valor_h, valor_l;

	for (;longitud;longitud--,puntero++) {


		ay_randomize(0);

		//randomize_noise es valor de 16 bits. sacar uno de 8 bits
		valor_h=value_16_to_8h(randomize_noise[0]);
		valor_l=value_16_to_8l(randomize_noise[0]);

		//valor=randomize_noise & 0xFF;

		//hacemos xor con los dos
		valor=valor_h ^ valor_l;

		//Si es Jupiter Ace, como la maquina no inicializa la RAM a 0, la inicializamos nosotros
		//Ademas esto beneficia al grabar snapshots, para no guardar datos aleatorios que no se usan
		if (MACHINE_IS_ACE) valor=0;

		//Lo mismo para ZX-Uno
		if (MACHINE_IS_ZXUNO) valor=0;

		//Lo mismo para Chloe
		if (MACHINE_IS_CHLOE) valor=0;

		//Si es Timex, tambien inicializamos a 0
		//if (MACHINE_IS_TIMEX_TS2068) valor=0;


		//printf ("random:%d\n",valor);
		*puntero=valor;
	}

}


//Patron de llenado para inves: FF,00,FF,00, etc...
void random_ram_inves(z80_byte *puntero,int longitud)
{

        z80_byte valor=255;

        for (;longitud;longitud--,puntero++) {

                //printf ("random:%d\n",valor);
                *puntero=valor;

		valor = valor ^255;
        }

	//asumimos que el ultimo valor enviado desde menu sera el por defecto (255)
	last_inves_low_ram_poke_menu=255;

}

struct s_machine_names {
	char nombre_maquina[40];  //40 mas que suficiente
	int id;
};

struct s_machine_names machine_names[]={

//char *machine_names[]={
                                            {"Spectrum 16k",              	0},
                                            {"Spectrum 48k", 			1},
                                            {"Inves Spectrum+",			2},
                                            {"Microdigital TK90X",		3},
                                            {"Microdigital TK90X (Spanish)",	4},
                                            {"Microdigital TK95",		5},
                                            {"Spectrum 128k",			6},
                                            {"Spectrum 128k (Spanish)",		7},
                                            {"Spectrum +2",			8},
                                            {"Spectrum +2 (French)",		9},
                                            {"Spectrum +2 (Spanish)",		10},
                                            {"Spectrum +2A (ROM v4.0)",		11},
                                            {"Spectrum +2A (ROM v4.1)",		12},
                                            {"Spectrum +2A (Spanish)",		13},
					    {"ZX-Uno",         			14},
					    {"Chloe 140SE",    			15},
					    {"Chloe 280SE",    			16},
			   		    {"Timex TS2068",   			17},
					    {"Prism",       			18},
					    {"TBBLue",   			19},
					    {"Spectrum 48k (Spanish)",		20},
					    {"Pentagon 128",		21},
                                            {"ZX-80",  				120},
                                            {"ZX-81",  				121},
					    {"Jupiter Ace",  			122},
					    {"Z88",  				130},
				            {"CPC 464",  			140},
					    {"Sam Coupe", 			150},
					    {"QL",				160},

						//Indicador de final
					{"",0}

};


/*char *get_machine_name(z80_byte m)
{
	if (m>=2 && m<=29) {
		if (m==20) m=2;
		else m++;
	}
	else if (m==120) m=21; //ZX80
	else if (m==121) m=22; //ZX81
	else if (m==122) m=23; //Jupiter ace
	else if (m==130) m=24;  //Z88
	else if (m==140) m=25; //CPC
	else if (m==150) m=26; //SAM
	return machine_names[m];
}*/

char *get_machine_name(z80_byte machine)
{
	int i;

	for (i=0;i<99999;i++) {
		if (machine_names[i].nombre_maquina[0]==0) {
			char mensaje[200];
			sprintf (mensaje,"No machine name found for machine id: %d",machine);
			cpu_panic(mensaje);
		}

		if (machine_names[i].id==machine) return machine_names[i].nombre_maquina;
	}

	//Aunque aqui no se llega nunca, para que no se queje el compilador
	return NULL;
}

//int maquina_anterior_cambio_cpu_speed=-1;

//ajustar timer de final de cada frame de pantalla. En vez de lanzar un frame cada 20 ms, hacerlo mas o menos rapido
void set_emulator_speed(void)
{

/*
	int forzarcambio=0;

	//Si se cambia entre maquina Z88 y cualquier otra, forzar
	if (MACHINE_IS_Z88 && maquina_anterior_cambio_cpu_speed!=30 ||
	    !MACHINE_IS_Z88 && maquina_anterior_cambio_cpu_speed==30) {
		forzarcambio=1;
		debug_printf ("
	}


	maquina_anterior_cambio_cpu_speed=machine_type;
*/

	/*
	if (porcentaje_velocidad_emulador==100) {
		//Si velocidad es la normal, no hacer ningun recalculo
		timer_sleep_machine=original_timer_sleep_machine;
	}

	else {
		timer_sleep_machine=original_timer_sleep_machine*100/porcentaje_velocidad_emulador;
		if (timer_sleep_machine==0) timer_sleep_machine=1;

	}
	*/

	//Calcular velocidad. caso normal que porcentaje=100, valor queda timer_sleep_machine=original_timer_sleep_machine*100/100 = original_timer_sleep_machine
	timer_sleep_machine=original_timer_sleep_machine*100/porcentaje_velocidad_emulador;
	if (timer_sleep_machine==0) timer_sleep_machine=1;

	//Si ha cambiado velocidad, reiniciar driver audio con frecuencia adecuada
	if (anterior_porcentaje_velocidad_emulador!=porcentaje_velocidad_emulador) {
		if (audio_end!=NULL) audio_end();
		frecuencia_sonido_variable=FRECUENCIA_CONSTANTE_NORMAL_SONIDO*porcentaje_velocidad_emulador/100;
		if (audio_init!=NULL) {
			if (audio_init()) {
				//Error
				fallback_audio_null();
			}
		}
	}

	anterior_porcentaje_velocidad_emulador=porcentaje_velocidad_emulador;

	debug_printf (VERBOSE_INFO,"Setting timer_sleep_machine to %d us",timer_sleep_machine);

	/*
	Lo anterior cubre los casos:
	-cambio de maquina de spectrum a z88: se recalcula timer_sleep_machine, sin tener que reiniciar driver audio
	-cambio de porcentaje cpu: se recalcula timer_sleep_machine y se reinicia driver audio si hay cambio porcentaje velocidad
	*/

}

/*
void set_emulator_speed(void)
{
	//ajuste mediante t estados por linea. requiere desactivar realvideo
	//ademas la tabla de memoria contended se saldra de rango...
	screen_testados_linea=screen_testados_linea*porcentaje_velocidad_emulador/100;

	printf ("t estados por linea: %d\n",screen_testados_linea);

}
*/


//ajusta max_cpu_cycles segun porcentaje cpu
//desactivado de momento
void old_set_emulator_speed(void)
{

	return ;

	//Esto se hacia antes cuando la cpu no estaba sincronizada y solo habia el contador de maximo de instrucciones
	//ahora habria que cuadrar muchos contadores en base a la velocidad: total_testates, testates de border, etc....

	//if (machine_type==20) max_cpu_cycles=MAX_CPU_CYCLES_ZX80;
	//else if (machine_type==21) max_cpu_cycles=MAX_CPU_CYCLES_ZX81;
	//else max_cpu_cycles=MAX_CPU_CYCLES_SPECTRUM;


	if (porcentaje_velocidad_emulador==100) return;

	//controlar valores. por ejemplo, un porcentaje de 1, da un floating point exception en alguna parte del codigo...
	if (porcentaje_velocidad_emulador<10 || porcentaje_velocidad_emulador>1000) {
		debug_printf (VERBOSE_ERR,"Invalid value for cpu speed: %d",porcentaje_velocidad_emulador);
		return;
	}

	debug_printf (VERBOSE_INFO,"Setting cpu speed to: %d%%",porcentaje_velocidad_emulador);

	//lo hacemos multiple de 312
	//max_cpu_cycles=max_cpu_cycles/312;

	//aplicamos porcentaje
	//max_cpu_cycles=max_cpu_cycles*porcentaje_velocidad_emulador;
	//max_cpu_cycles=max_cpu_cycles/100;

	//y lo volvemos a dejar *312
	//max_cpu_cycles=max_cpu_cycles*312;

}


//asignar memoria de maquina, liberando memoria antes si conviene
void malloc_machine(int tamanyo)
{
	if (memoria_spectrum!=NULL) {
		debug_printf(VERBOSE_INFO,"Freeing previous Machine memory");
		free(memoria_spectrum);
	}

	debug_printf(VERBOSE_INFO,"Allocating %d bytes for Machine memory",tamanyo);
	memoria_spectrum=malloc(tamanyo);

	if (memoria_spectrum==NULL) {
		cpu_panic ("Error. Cannot allocate Machine memory");
	}


	//En Z88, el puntero que se usa es realmente z88_puntero_memoria
	z88_puntero_memoria=memoria_spectrum;
}

void malloc_mem_machine(void) {

	//Caso Inves. Asignamos 64 kb ram+16kb para rom

	if (MACHINE_IS_INVES) {
		malloc_machine(65536+16384);
		random_ram_inves(memoria_spectrum,65536);
	}

        else if (MACHINE_IS_SPECTRUM_16_48 || MACHINE_IS_ZX8081 || MACHINE_IS_ACE) {
                //total 64kb
                malloc_machine(65536);
                random_ram(memoria_spectrum+16384,49152);

        }

        else if (MACHINE_IS_SPECTRUM_128_P2) {

                //32 kb rom, 128 ram
                malloc_machine((32+128)*1024);
                random_ram(memoria_spectrum+32768,131072);

                int puntero=0;
                int i;
                for (i=0;i<2;i++) {
                        rom_mem_table[i]=&memoria_spectrum[puntero];
                        puntero +=16384;
                }

                for (i=0;i<8;i++) {
                        ram_mem_table[i]=&memoria_spectrum[puntero];
                        puntero +=16384;
                }

                mem_set_normal_pages_128k();

        }

	 else if (MACHINE_IS_SPECTRUM_P2A) {

                //64 kb rom, 128 ram
                malloc_machine((64+128)*1024);
                random_ram(memoria_spectrum+65536,131072);

                int puntero=0;
                int i;
                for (i=0;i<4;i++) {
                        rom_mem_table[i]=&memoria_spectrum[puntero];
                        puntero +=16384;
                }

                for (i=0;i<8;i++) {
                        ram_mem_table[i]=&memoria_spectrum[puntero];
                        puntero +=16384;
                }

                mem_set_normal_pages_p2a();

        }

	else if (MACHINE_IS_ZXUNO) {
		//16 KB rom
		//512 KB SRAM
		//1024 FLASH
		malloc_machine((ZXUNO_ROM_SIZE+ZXUNO_SRAM_SIZE+ZXUNO_SPI_SIZE)*1024);
		random_ram(memoria_spectrum,(ZXUNO_ROM_SIZE+ZXUNO_SRAM_SIZE+ZXUNO_SPI_SIZE)*1024);

                int puntero=16384; //saltamos los primeros 16kb de rom del bootloader
                int i;

		//Paginas SRAM de zxuno
                for (i=0;i<ZXUNO_SRAM_PAGES;i++) {
                        zxuno_sram_mem_table[i]=&memoria_spectrum[puntero];
                        puntero +=16384;
                }

                mem_set_normal_pages_zxuno();



	}

	  else if (MACHINE_IS_CHLOE) {

		//Necesita 32 kb rom, 256 kb ram (EX 64, DOCK 64, HOME 128)

                //32 kb rom, 256 ram
                malloc_machine((32+256)*1024);
                random_ram(memoria_spectrum+32768,256*1024);

		chloe_init_memory_tables();
		chloe_set_memory_pages();

        }

	else if (MACHINE_IS_PRISM) {

                //Necesita 4096 KB rom, 512 kb ram (EX 64, DOCK 64, HOME 128)

                //4096 kb rom, 512 ram
                malloc_machine((4096+512)*1024);

                random_ram(memoria_spectrum+(4096*1024),512*1024);


		//Y paginas VRAM
		prism_malloc_vram();

                prism_init_memory_tables();
                prism_set_memory_pages();

        }

        else if (MACHINE_IS_TBBLUE) {

		//512 KB RAM + 8 KB ROM (8 kb repetido dos veces)
                malloc_machine( (512+8*2)*1024);

                random_ram(memoria_spectrum,(512+8*2)*1024);


                tbblue_init_memory_tables();
                tbblue_set_memory_pages();

        }


          else if (MACHINE_IS_TIMEX_TS2068) {

                //Necesita 192KB de memoria (EX 64, DOCK 64, HOME 48, ROM 16)

                malloc_machine((192)*1024);
                random_ram(memoria_spectrum,192*1024);

                timex_init_memory_tables();
                timex_set_memory_pages();

		//Inicializar memoria DOCK con 255
		timex_empty_dock_space();

        }

	else if (MACHINE_IS_CPC_464) {

		//32 kb rom
		//64 kb ram

                malloc_machine(96*1024);
                random_ram(memoria_spectrum,96*1024);

                cpc_init_memory_tables();
                cpc_set_memory_pages();


        }


	else if (MACHINE_IS_Z88) {
		//Asignar 4 MB
		//z88_puntero_memoria=malloc(4*1024*1024);
		malloc_machine(4*1024*1024);
		random_ram(memoria_spectrum,4*1024*1024);
	}

	else if (MACHINE_IS_SAM) {
		//Asignar 512 kb+32 kb de rom
		//Incluso si la maquina es de 256kb, nosotros asignamos el maximo (512)
		malloc_machine((512+32)*1024);
		random_ram(memoria_spectrum,(512+32)*1024);

                sam_init_memory_tables();
                sam_set_memory_pages();
	}

	else if (MACHINE_IS_QL) {
		//Asignar 256 kb de ram
		malloc_machine(QL_MEM_LIMIT+1);
		memoria_ql=memoria_spectrum;
		//random_ram(memoria_ql,QL_MEM_LIMIT+1);
	}

	else {
		cpu_panic("Do not know how to allocate mem for active machine");
	}

}


void set_machine_params(void)
{

/*
0=Sinclair 16k
1=Sinclair 48k
2=Inves Spectrum+
3=tk90x
4=tk90xs
5=tk95
6=Sinclair 128k
7=Sinclair 128k Español
8=Amstrad +2
9=Amstrad +2 - Frances
10=Amstrad +2 - Espa�ol
11=Amstrad +2A (ROM v4.0)
12=Amstrad +2A (ROM v4.1)
13=Amstrad +2A - Espa�ol
14=ZX-Uno
15=Chloe 140SE
16=Chloe 280SE
17=Timex TS2068
18=Prism
19=TBBlue
20=Spectrum + Spanish
21=Pentagon 128
22-29 Reservado (Spectrum)
120=zx80 (old 20)
121=zx81 (old 21)
122=jupiter ace (old 22)
130=z88 (old 30)
140=amstrad cpc464 (old 40)
150=Sam Coupe (old 50)
151-59 reservado para otros sam (old 51-59)
160=QL Standard
161-179 reservado para otros QL
*/

		char mensaje_error[200];

		//defaults
		ay_chip_present.v=0;

		if (!MACHINE_IS_Z88) {
			//timer_sleep_machine=original_timer_sleep_machine=20000;
			original_timer_sleep_machine=20000;
        	        set_emulator_speed();
		}

		allow_write_rom.v=0;

		multiface_enabled.v=0;
		dandanator_enabled.v=0;
		superupgrade_enabled.v=0;
		//nota: combiene que allow_write_rom.v sea 0 al desactivar superupgrade
		//porque si estaba activo allow_write_rom.v antes, y desactivamos superupgrade,
		//al intentar desactivar allow_write, se produce segmentation fault

		//Si maquina anterior era pentagon, desactivar timing pentagon y activar contended memory
		if (last_machine_type==MACHINE_ID_PENTAGON128) {
			debug_printf(VERBOSE_DEBUG,"Disabling pentagon timing and enabling contended memory because previous machine was Pentagon");
			pentagon_timing.v=0;
			contend_enabled.v=1;
		}

		chloe_keyboard.v=0;


		input_file_keyboard_turbo.v=0;

		//Modo turbo no. No, parece que cambiarlo aqui sin mas no provoca cambios cuando se hace smartload
		//no tocarlo, dejamos que el usuario se encargue de ponerlo a 1 si quiere
		//printf ("Setting cpu speed to 1\n");
		//sleep (3);
		//cpu_turbo_speed=1;


		//cpu_core_loop=cpu_core_loop_spectrum;
		if (MACHINE_IS_SPECTRUM) {
			cpu_core_loop_active=CPU_CORE_SPECTRUM;
		}

		else if (MACHINE_IS_ZX8081) {
			cpu_core_loop_active=CPU_CORE_ZX8081;
		}

		else if (MACHINE_IS_ACE) {
			cpu_core_loop_active=CPU_CORE_ACE;
		}

		else if (MACHINE_IS_CPC) {
			cpu_core_loop_active=CPU_CORE_CPC;
		}

		else if (MACHINE_IS_SAM) {
			cpu_core_loop_active=CPU_CORE_SAM;
		}

		else if (MACHINE_IS_QL) {
			cpu_core_loop_active=CPU_CORE_QL;
		}

		else {
			cpu_core_loop_active=CPU_CORE_Z88;
		}

		debug_breakpoints_enabled.v=0;
		set_cpu_core_loop();

		//esto ya establece el cpu core. Debe estar antes de establecer peek, poke, lee_puerto, out_port
		//dado que "restaura" dichas funciones a sus valores originales... pero la primera vez, esos valores originales valen 0
		//breakpoints_disable();

		out_port=out_port_spectrum;


		//desactivar ram refresh emulation, cpu transaction log y cualquier otra funcion que altere el cpu_core, el peek_byte, etc
		cpu_transaction_log_enabled.v=0;
		machine_emulate_memory_refresh=0;



		//Valores usados en real video
		//TODO. para 128k hay 1 linea menos superior. ZX80,ZX81??
		screen_invisible_borde_superior=8;
		screen_borde_superior=56;

		screen_total_borde_inferior=56;

		screen_total_borde_izquierdo=48;
		screen_total_borde_derecho=48;
		screen_invisible_borde_derecho=96;
		screen_testados_linea=224;

		//Reseteos para ZX80/81

                //offset_zx8081_t_coordx=0;
		video_zx8081_lnctr_adjust.v=0;

		video_zx8081_estabilizador_imagen.v=1;

		disable_wrx();
                //wrx_present.v=0;
		//wrx_mueve_primera_columna.v=1;
                zx8081_vsync_sound.v=0;
		//video_zx8081_decremento_x_cuando_mayor=8;
		minimo_duracion_vsync=DEFAULT_MINIMO_DURACION_VSYNC;

		//normalmente excepto +2a
		port_from_ula=port_from_ula_48k;

		fetch_opcode=fetch_opcode_spectrum;




		//2 para ZX81 mejor
		//0 para spectrum mejor
		realtape_volumen=0;


		screen_set_parameters_slow_machines();

		audio_empty_buffer();

		if (MACHINE_IS_CHLOE) chloe_keyboard.v=1;

		//Cargar keymap de manera generica. De momento solo se usa en Z88, CPC y Chloe
		if (scr_z88_cpc_load_keymap!=NULL) scr_z88_cpc_load_keymap();


		//Quitar mensajes de footer establecidos con autoselectoptions.c
		//Desactivado. Esto provoca:
		//Al cambiar de maquina, si hay un mensaje pendiente, seguira desplazandose hasta que acabe
		//Si en cambio activasemos esta linea comentada, pasaria que al cargar un snapshot que tenga .config,
		//se estableceria el mensaje del programname del .config, y esta linea borraria el mensaje y no se veria
		//temp tape_options_set_first_message_counter=tape_options_set_second_message_counter=0;


		if (MACHINE_IS_SPECTRUM_16_48) {
			contend_read=contend_read_48k;
			contend_read_no_mreq=contend_read_no_mreq_48k;
			contend_write_no_mreq=contend_write_no_mreq_48k;

			ula_contend_port_early=ula_contend_port_early_48k;
			ula_contend_port_late=ula_contend_port_late_48k;

			//Ajustes para Inves
			if (MACHINE_IS_INVES) {
	                        screen_testados_linea=228;
        	                screen_invisible_borde_superior=7;
                	        screen_invisible_borde_derecho=104;
			}
		}

		else if (MACHINE_IS_SPECTRUM_128_P2_P2A) {
                        contend_read=contend_read_128k;
                        contend_read_no_mreq=contend_read_no_mreq_128k;
                        contend_write_no_mreq=contend_write_no_mreq_128k;

			ula_contend_port_early=ula_contend_port_early_128k;
			ula_contend_port_late=ula_contend_port_late_128k;


	                screen_testados_linea=228;
        	        screen_invisible_borde_superior=7;
                	screen_invisible_borde_derecho=104;

			contend_pages_128k_p2a=contend_pages_128k;

		        if (MACHINE_IS_SPECTRUM_P2A) {
                		port_from_ula=port_from_ula_p2a;
				contend_pages_128k_p2a=contend_pages_p2a;
			}

			if (MACHINE_IS_PENTAGON128) {
				contend_enabled.v=0;
				ula_enable_pentagon_timing_no_common();
			}


		}

                else if (MACHINE_IS_ZXUNO) {
			zxuno_set_timing_48k();
                }


		else if (MACHINE_IS_ZX8081) {

		        screen_invisible_borde_superior=16;
			screen_borde_superior=48;

                	contend_read=contend_read_zx8081;
	                contend_read_no_mreq=contend_read_no_mreq_zx8081;
        	        contend_write_no_mreq=contend_write_no_mreq_zx8081;

			ula_contend_port_early=ula_contend_port_early_zx8081;
			ula_contend_port_late=ula_contend_port_late_zx8081;


			//2 para ZX81 mejor
			//0 para spectrum mejor
			realtape_volumen=2;

		}

		else if (MACHINE_IS_ACE) {
		        screen_invisible_borde_superior=16;
			screen_borde_superior=48;

                        contend_read=contend_read_ace;
                        contend_read_no_mreq=contend_read_no_mreq_ace;
                        contend_write_no_mreq=contend_write_no_mreq_ace;

                        ula_contend_port_early=ula_contend_port_early_ace;
                        ula_contend_port_late=ula_contend_port_late_ace;


                }


	       else if (MACHINE_IS_CHLOE) {
                        contend_read=contend_read_chloe;
                        contend_read_no_mreq=contend_read_no_mreq_chloe;
                        contend_write_no_mreq=contend_write_no_mreq_chloe;

                        ula_contend_port_early=ula_contend_port_early_chloe;
                        ula_contend_port_late=ula_contend_port_late_chloe;


                }

		else if (MACHINE_IS_PRISM) {
                        contend_read=contend_read_prism;
                        contend_read_no_mreq=contend_read_no_mreq_prism;
                        contend_write_no_mreq=contend_write_no_mreq_prism;

                        ula_contend_port_early=ula_contend_port_early_prism;
                        ula_contend_port_late=ula_contend_port_late_prism;

			screen_invisible_borde_superior=45;
			screen_borde_superior=48;
			screen_total_borde_inferior=48;

			screen_total_borde_izquierdo=64;
			screen_total_borde_derecho=64;
			screen_invisible_borde_derecho=158;
			screen_testados_linea=133;
                }

                if (MACHINE_IS_TBBLUE) {
			tbblue_set_timing_48k();

		        //divmmc arranca desactivado, lo desactivamos asi para que no cambie las funciones peek/poke
			//esto ya se desactiva en set_machine
		        /*if (divmmc_enabled.v) {
		                divmmc_enabled.v=0;
		                diviface_enabled.v=0;
		        }
			*/


                }

               else if (MACHINE_IS_TIMEX_TS2068) {
                        contend_read=contend_read_timex;
                        contend_read_no_mreq=contend_read_no_mreq_timex;
                        contend_write_no_mreq=contend_write_no_mreq_timex;

                        ula_contend_port_early=ula_contend_port_early_timex;
                        ula_contend_port_late=ula_contend_port_late_timex;


                }




                else if (MACHINE_IS_Z88) {
                        contend_read=contend_read_z88;
                        contend_read_no_mreq=contend_read_no_mreq_z88;
                        contend_write_no_mreq=contend_write_no_mreq_z88;

                        ula_contend_port_early=ula_contend_port_early_z88;
                        ula_contend_port_late=ula_contend_port_late_z88;

			//timer_sleep_machine=original_timer_sleep_machine=5000;
			original_timer_sleep_machine=5000;
			set_emulator_speed();

                }


		else if (MACHINE_IS_CPC_464) {
                        contend_read=contend_read_cpc;
                        contend_read_no_mreq=contend_read_no_mreq_cpc;
                        contend_write_no_mreq=contend_write_no_mreq_cpc;

                        ula_contend_port_early=ula_contend_port_early_cpc;
                        ula_contend_port_late=ula_contend_port_late_cpc;


                }

		else if (MACHINE_IS_SAM) {
			contend_read=contend_read_sam;
                        contend_read_no_mreq=contend_read_no_mreq_sam;
                        contend_write_no_mreq=contend_write_no_mreq_sam;

                        ula_contend_port_early=ula_contend_port_early_sam;
                        ula_contend_port_late=ula_contend_port_late_sam;
		}






	switch (current_machine_type) {

		case 0:
		poke_byte=poke_byte_spectrum_16k;
		peek_byte=peek_byte_spectrum_16k;
		peek_byte_no_time=peek_byte_no_time_spectrum_16k;
		poke_byte_no_time=poke_byte_no_time_spectrum_16k;
		lee_puerto=lee_puerto_spectrum;
		break;

		case 1:
		case 20:
		poke_byte=poke_byte_spectrum_48k;
		peek_byte=peek_byte_spectrum_48k;
		peek_byte_no_time=peek_byte_no_time_spectrum_48k;
		poke_byte_no_time=poke_byte_no_time_spectrum_48k;
		lee_puerto=lee_puerto_spectrum;

		break;

                case 2:
                poke_byte=poke_byte_spectrum_inves;
                peek_byte=peek_byte_spectrum_inves;
		peek_byte_no_time=peek_byte_no_time_spectrum_inves;
		poke_byte_no_time=poke_byte_no_time_spectrum_inves;
		lee_puerto=lee_puerto_spectrum;
                break;

                case 3:
                poke_byte=poke_byte_spectrum_48k;
                peek_byte=peek_byte_spectrum_48k;
		peek_byte_no_time=peek_byte_no_time_spectrum_48k;
		poke_byte_no_time=poke_byte_no_time_spectrum_48k;
                lee_puerto=lee_puerto_spectrum;
                break;

                case 4:
                poke_byte=poke_byte_spectrum_48k;
                peek_byte=peek_byte_spectrum_48k;
		peek_byte_no_time=peek_byte_no_time_spectrum_48k;
		poke_byte_no_time=poke_byte_no_time_spectrum_48k;
                lee_puerto=lee_puerto_spectrum;
                break;

                case 5:
                poke_byte=poke_byte_spectrum_48k;
                peek_byte=peek_byte_spectrum_48k;
		peek_byte_no_time=peek_byte_no_time_spectrum_48k;
		poke_byte_no_time=poke_byte_no_time_spectrum_48k;
                lee_puerto=lee_puerto_spectrum;
                break;




                case 6:
                poke_byte=poke_byte_spectrum_128k;
                peek_byte=peek_byte_spectrum_128k;
		peek_byte_no_time=peek_byte_no_time_spectrum_128k;
		poke_byte_no_time=poke_byte_no_time_spectrum_128k;
                lee_puerto=lee_puerto_spectrum;
		ay_chip_present.v=1;
                break;

                case 7:
                poke_byte=poke_byte_spectrum_128k;
                peek_byte=peek_byte_spectrum_128k;
		peek_byte_no_time=peek_byte_no_time_spectrum_128k;
		poke_byte_no_time=poke_byte_no_time_spectrum_128k;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;




                case 8:
                poke_byte=poke_byte_spectrum_128k;
                peek_byte=peek_byte_spectrum_128k;
		peek_byte_no_time=peek_byte_no_time_spectrum_128k;
		poke_byte_no_time=poke_byte_no_time_spectrum_128k;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;

                case 9:
                poke_byte=poke_byte_spectrum_128k;
                peek_byte=peek_byte_spectrum_128k;
		peek_byte_no_time=peek_byte_no_time_spectrum_128k;
		poke_byte_no_time=poke_byte_no_time_spectrum_128k;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;

                case 10:
                poke_byte=poke_byte_spectrum_128k;
                peek_byte=peek_byte_spectrum_128k;
		peek_byte_no_time=peek_byte_no_time_spectrum_128k;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;

                case 11:
                poke_byte=poke_byte_spectrum_128kp2a;
                peek_byte=peek_byte_spectrum_128kp2a;
		peek_byte_no_time=peek_byte_no_time_spectrum_128kp2a;
		poke_byte_no_time=poke_byte_no_time_spectrum_128kp2a;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;

                case 12:
                poke_byte=poke_byte_spectrum_128kp2a;
                peek_byte=peek_byte_spectrum_128kp2a;
		peek_byte_no_time=peek_byte_no_time_spectrum_128kp2a;
		poke_byte_no_time=poke_byte_no_time_spectrum_128kp2a;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;

                case 13:
                poke_byte=poke_byte_spectrum_128kp2a;
                peek_byte=peek_byte_spectrum_128kp2a;
		peek_byte_no_time=peek_byte_no_time_spectrum_128kp2a;
		poke_byte_no_time=poke_byte_no_time_spectrum_128kp2a;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;

                case 14:
                poke_byte=poke_byte_zxuno;
                peek_byte=peek_byte_zxuno;
                peek_byte_no_time=peek_byte_no_time_zxuno;
                poke_byte_no_time=poke_byte_no_time_zxuno;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;


		case 15:
		case 16:
                poke_byte=poke_byte_chloe;
                peek_byte=peek_byte_chloe;
                peek_byte_no_time=peek_byte_no_time_chloe;
                poke_byte_no_time=poke_byte_no_time_chloe;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
		enable_rainbow();
		enable_ulaplus();
		enable_timex_video();

		//Chloe 280SE lleva turbosound
		if (MACHINE_IS_CHLOE_280SE) {
		        ay_chip_selected=0;
		        turbosound_enabled.v=1;
		}


                break;

                case 17:
                poke_byte=poke_byte_timex;
                peek_byte=peek_byte_timex;
                peek_byte_no_time=peek_byte_no_time_timex;
                poke_byte_no_time=poke_byte_no_time_timex;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                enable_rainbow();
                enable_timex_video();

                break;

		case 18:
                poke_byte=poke_byte_prism;
                peek_byte=peek_byte_prism;
                peek_byte_no_time=peek_byte_no_time_prism;
                poke_byte_no_time=poke_byte_no_time_prism;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                enable_rainbow();
                enable_ulaplus();
                enable_timex_video();

                break;

                case 19:
                poke_byte=poke_byte_tbblue;
                peek_byte=peek_byte_tbblue;
                peek_byte_no_time=peek_byte_no_time_tbblue;
                poke_byte_no_time=poke_byte_no_time_tbblue;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;

                break;


                case 21:
                poke_byte=poke_byte_spectrum_128k;
                peek_byte=peek_byte_spectrum_128k;
                peek_byte_no_time=peek_byte_no_time_spectrum_128k;
                poke_byte_no_time=poke_byte_no_time_spectrum_128k;
                lee_puerto=lee_puerto_spectrum;
                ay_chip_present.v=1;
                break;







		case 120:
                poke_byte=poke_byte_zx80;
                peek_byte=peek_byte_zx80;
		peek_byte_no_time=peek_byte_zx80_no_time;
		poke_byte_no_time=poke_byte_zx80_no_time;
		lee_puerto=lee_puerto_zx80;


		out_port=out_port_zx80;

		nmi_generator_active.v=0;
		hsync_generator_active.v=0;

		debug_printf (VERBOSE_INFO,"Emulating ZX80 with %d KB (ramtop=%d)",(ramtop_zx8081-16383)/1024,ramtop_zx8081);
		//printf ("ramtop: %d\n",ramtop_zx8081);

		screen_testados_linea=207;
                //offset_zx8081_t_estados=0;

		fetch_opcode=fetch_opcode_zx81;



		//video_zx8081_lnctr_adjust.v=0;



		break;

		case 121:
                poke_byte=poke_byte_zx80;
                peek_byte=peek_byte_zx80;
		peek_byte_no_time=peek_byte_zx80_no_time;
		poke_byte_no_time=poke_byte_zx80_no_time;

		lee_puerto=lee_puerto_zx81;

		out_port=out_port_zx81;

		nmi_generator_active.v=0;
		hsync_generator_active.v=0;

		debug_printf (VERBOSE_INFO,"Emulating ZX81 with %d KB (ramtop=%d)",(ramtop_zx8081-16383)/1024,ramtop_zx8081);
		//printf ("ramtop: %d\n",ramtop_zx8081);
		screen_testados_linea=207;

                //offset_zx8081_t_estados=0;

		fetch_opcode=fetch_opcode_zx81;


		//video_zx8081_lnctr_adjust.v=1;

		break;

		case 122:
		//Jupiter Ace
                poke_byte=poke_byte_ace;
                peek_byte=peek_byte_ace;
                peek_byte_no_time=peek_byte_ace_no_time;
                poke_byte_no_time=poke_byte_ace_no_time;

                lee_puerto=lee_puerto_ace;

                out_port=out_port_ace;

                //nmi_generator_active.v=0;
                //hsync_generator_active.v=0;

                debug_printf (VERBOSE_INFO,"Emulating Jupiter Ace with %d KB (ramtop=%d)",(ramtop_ace-16383)/1024+3,ramtop_ace);
                screen_testados_linea=208;

                //offset_zx8081_t_estados=0;

                fetch_opcode=fetch_opcode_ace;



                break;


                case 130:
                poke_byte=poke_byte_z88;
                peek_byte=peek_byte_z88;
                peek_byte_no_time=peek_byte_no_time_z88;
                poke_byte_no_time=poke_byte_no_time_z88;
                lee_puerto=lee_puerto_z88;
		out_port=out_port_z88;

		init_z88_memory_slots();

		disable_rainbow();

		//zx81
		//screen_testados_linea=207;
		//spectrum 3.5 Mhz
		//screen_testados_linea=224;

		//Z88 3,2768 MHz
		screen_testados_linea=210;




		break;


		//CPC464
                case 140:
                poke_byte=poke_byte_cpc;
                peek_byte=peek_byte_cpc;
                peek_byte_no_time=peek_byte_no_time_cpc;
                poke_byte_no_time=poke_byte_no_time_cpc;
                lee_puerto=lee_puerto_cpc;
		out_port=out_port_cpc;
                ay_chip_present.v=1;
                fetch_opcode=fetch_opcode_cpc;
		//4Mhz
		screen_testados_linea=256;

		//temp
		//screen_testados_linea=228;
                break;


		case 150:
                poke_byte=poke_byte_sam;
                peek_byte=peek_byte_sam;
                peek_byte_no_time=peek_byte_no_time_sam;
                poke_byte_no_time=poke_byte_no_time_sam;
                lee_puerto=lee_puerto_sam;
                out_port=out_port_sam;
                fetch_opcode=fetch_opcode_sam;
		screen_testados_linea=384; //6 MHZ aprox
                break;

		case MACHINE_ID_QL_STANDARD: //QL 160


		poke_byte=poke_byte_legacy_ql;
		peek_byte=peek_byte_legacy_ql;
		peek_byte_no_time=peek_byte_no_time_legacy_ql;
		poke_byte_no_time=poke_byte_no_time_legacy_ql;
		lee_puerto=lee_puerto_legacy_ql;
		out_port=out_port_legacy_ql;
		fetch_opcode=fetch_opcode_legacy_ql;


							//Hagamoslo mas lento
								screen_testados_linea=80;
		break;






		default:
			//printf ("Init Machine id %d not supported. Exiting\n",current_machine_type);
			sprintf (mensaje_error,"Init Machine id %d not supported. Exiting",current_machine_type);
			cpu_panic(mensaje_error);
		break;
	}


	if (MACHINE_IS_SPECTRUM) {
                //Activar deteccion automatica de rutina de impresion de caracteres, si conviene
                if (chardetect_detect_char_enabled.v) {
                        chardetect_init_automatic_char_detection();
                }

		//Reactivar poke de spectra
		if (spectra_enabled.v) spectra_set_poke();

	}


	debug_printf(VERBOSE_INFO,"Setting machine %s",get_machine_name(current_machine_type));

	//Recalcular algunos valores cacheados
	recalcular_get_total_ancho_rainbow();
	recalcular_get_total_alto_rainbow();



}


/*
Reabrir ventana en caso de que maquina seleccionada tenga tamanyo diferente que la anterior
TODO: Quiza se podria simplificar esto, se empezó con Z88 a spectrum y se han ido agregando,
se exponen todos los casos de maquinas con diferentes tamanyos de ventana,
pero quiza simplemente habria que ver que el tamanyo anterior fuera diferente al actual y entonces reabrir ventana
*/
void post_set_machine_no_rom_load_reopen_window(void)
{
	//si se cambia de maquina Z88 o a maquina Z88, redimensionar ventana
	if (last_machine_type!=255) {

		if ( (MACHINE_IS_Z88 && last_machine_type!=130)  || (last_machine_type==130 && !(MACHINE_IS_Z88) ) ) {
			debug_printf (VERBOSE_INFO,"Reopening window so machine has different size (changing Z88 to/from other machine)");
			debug_printf(VERBOSE_INFO,"End Screen");
			scr_end_pantalla();
			debug_printf(VERBOSE_INFO,"Creating Screen");
			scr_init_pantalla();
			return;
		}
	}


	//si se cambia de maquina CPC o a maquina CPC, redimensionar ventana

							if (last_machine_type!=255) {

											if ( (MACHINE_IS_CPC && !(last_machine_type>=140 && last_machine_type<=149) )  || (  (last_machine_type>=140 && last_machine_type<=149) && !(MACHINE_IS_CPC) ) ) {
															debug_printf (VERBOSE_INFO,"Reopening window so machine has different size (changing CPC to/from other machine)");

															debug_printf(VERBOSE_INFO,"End Screen");
															scr_end_pantalla();
															debug_printf(VERBOSE_INFO,"Creating Screen");
															scr_init_pantalla();
															return;
											}
							}

							//si se cambia de maquina SAM o a maquina SAM, redimensionar ventana

							if (last_machine_type!=255) {

											if ( (MACHINE_IS_SAM && !(last_machine_type>=150 && last_machine_type<=159) )  || (  (last_machine_type>=150 && last_machine_type<=159) && !(MACHINE_IS_SAM) ) ) {
															debug_printf (VERBOSE_INFO,"Reopening window so machine has different size (changing SAM to/from other machine)");

															debug_printf(VERBOSE_INFO,"End Screen");
															scr_end_pantalla();
															debug_printf(VERBOSE_INFO,"Creating Screen");
															scr_init_pantalla();
															return;
											}
							}

							//si se cambia de maquina QL o a maquina QL, redimensionar ventana

							if (last_machine_type!=255) {

											if ( (MACHINE_IS_QL && !(last_machine_type>=160 && last_machine_type<=179) )  || (  (last_machine_type>=160 && last_machine_type<=179) && !(MACHINE_IS_QL) ) ) {
															debug_printf (VERBOSE_INFO,"Reopening window so machine has different size (changing QL to/from other machine)");

															debug_printf(VERBOSE_INFO,"End Screen");
															scr_end_pantalla();
															debug_printf(VERBOSE_INFO,"Creating Screen");
															scr_init_pantalla();
															return;
											}
							}



							//si se cambia de maquina Prism o a maquina Prism, redimensionar ventana

							if (last_machine_type!=255) {

											if ( (MACHINE_IS_PRISM && last_machine_type!=18)   || (last_machine_type==18 && !(MACHINE_IS_PRISM)  ) ) {
															debug_printf (VERBOSE_INFO,"Reopening window so machine has different size (changing PRISM to/from other machine)");

															debug_printf(VERBOSE_INFO,"End Screen");
															scr_end_pantalla();
															debug_printf(VERBOSE_INFO,"Creating Screen");
															scr_init_pantalla();
															return;
											}
							}

							//Si se cambia de maquina Spectrum o a maquina Spectrum - afecta especialmente a spectrum
							if (last_machine_type!=255) {

											if ( (MACHINE_IS_SPECTRUM && !(last_machine_type<30) )  || (  (last_machine_type<30) && !(MACHINE_IS_SPECTRUM) ) ) {
															debug_printf (VERBOSE_INFO,"Reopening window so machine has different size (changing Spectrum to/from other machine)");
															debug_printf(VERBOSE_INFO,"End Screen");
															scr_end_pantalla();
															debug_printf(VERBOSE_INFO,"Creating Screen");
															scr_init_pantalla();
															return;
											}
							}

}

void post_set_machine_no_rom_load(void)
{

		screen_set_video_params_indices();
		inicializa_tabla_contend();

		init_rainbow();
		init_cache_putpixel();


		post_set_machine_no_rom_load_reopen_window();



		last_machine_type=current_machine_type;
		menu_init_footer();

}

void post_set_machine(char *romfile)
{

                //leer rom
                debug_printf (VERBOSE_INFO,"Loading ROM");
                rom_load(romfile);

		post_set_machine_no_rom_load();
}


void set_machine(char *romfile)
{

	//Si estaba divmmc o divide activo, desactivarlos
	//if (divmmc_enabled.v) divmmc_disable();
	//if (divide_enabled.v) divide_disable();
	if (diviface_enabled.v) diviface_disable();

                //Si se cambia de maquina zxuno a otra no zxuno, desactivar divmmc
		/*
                if (last_machine_type!=255) {
                        if (last_machine_type==14 && machine_type!=14 && divmmc_enabled.v) {
                                debug_printf (VERBOSE_INFO,"Disabling divmmc because it was enabled on ZX-Uno");
                                divmmc_disable();
                        }
                }
		*/



	set_machine_params();
	malloc_mem_machine();

	//Si hay activo divmmc o divide, reactivar. Se pone aqui porque en caso de zxuno es necesario que esten inicializadas las paginas de memoria
	//if (divmmc_enabled.v) divmmc_enable();
	//if (divide_enabled.v) divide_enable();

	post_set_machine(romfile);
}



//Punto de entrada para error al cargar rom de cualquier modelo de spectrum, dado que se lee longitud de la rom diferente de la esperada
//panic
//void rom_load_cpu_panic(char *romfilename,int leidos)
//{
//	cpu_panic("Error loading ROM");
//}


void rom_load(char *romfilename)
{
                FILE *ptr_romfile;
		int leidos;

	char mensaje_error[200];


	if (romfilename==NULL) {
		switch (current_machine_type) {
                case 0:
                romfilename="48.rom";
                break;

                case 1:
                romfilename="48.rom";
                break;

                case 2:
                romfilename="inves.rom";
                break;

                case 3:
                romfilename="tk90x.rom";
                break;

                case 4:
                romfilename="tk90xs.rom";
                break;

                case 5:
                romfilename="tk95.rom";
                break;




                case 6:
                romfilename="128.rom";
                break;

                case 7:
                romfilename="128s.rom";
                break;




                case 8:
                romfilename="p2.rom";
                break;

                case 9:
                romfilename="p2f.rom";
                break;

                case 10:
                romfilename="p2s.rom";
                break;

                case 11:
                romfilename="p2a40.rom";
                break;

                case 12:
                romfilename="p2a41.rom";
                break;

                case 13:
                romfilename="p2as.rom";
                break;


		case 14:
		romfilename="zxuno_bootloader.rom";
		break;

		case 15:
		case 16:
		romfilename="se.rom";
		break;


		case 17:
		romfilename="ts2068.rom";
		break;

		case 18:
		romfilename="prism.rom";
		break;

		case 19:
		romfilename="tbblue_loader.rom";
		break;

                case 20:
                romfilename="48es.rom";
                break;

                case 21:
                romfilename="pentagon.rom";
                break;


                case 120:
                romfilename="zx80.rom";

                break;

                case 121:

                romfilename="zx81.rom";

		break;

                case 122:

                romfilename="ace.rom";

                break;

		case 130:

		romfilename="Z88OZ47.rom";


		break;


		case 140:
		romfilename="cpc464.rom";
		break;

		case 150:
		if (atomlite_enabled.v) romfilename="atomlite.rom";
		else romfilename="samcoupe.rom";
		break;

		case MACHINE_ID_QL_STANDARD:
		romfilename="ql_js.rom";

		//romfilename="MIN189.rom";
		//romfilename="ql_jm.rom";
		break;

                        default:
                                //printf ("ROM for Machine id %d not supported. Exiting\n",machine_type);
                                sprintf (mensaje_error,"ROM for Machine id %d not supported. Exiting",current_machine_type);
				cpu_panic(mensaje_error);
                        break;



		}
	}

                //ptr_romfile=fopen(romfilename,"rb");
		open_sharedfile(romfilename,&ptr_romfile);
                if (!ptr_romfile)
                {
                        debug_printf(VERBOSE_ERR,"Unable to open rom file %s",romfilename);
			cpu_panic("Unable to open rom file");
                }

		//Caso Inves. ROM esta en el final de la memoria asignada
		if (MACHINE_IS_INVES) {
			//Inves
			leidos=fread(&memoria_spectrum[65536],1,16384,ptr_romfile);
                                if (leidos!=16384) {
				 	cpu_panic("Error loading ROM");
                                }
                }


		else if (MACHINE_IS_SPECTRUM_16_48) {
			//Spectrum 16k rom
                        	leidos=fread(memoria_spectrum,1,16384,ptr_romfile);
				if (leidos!=16384) {
				 	cpu_panic("Error loading ROM");
				}
		}

		else if (MACHINE_IS_SPECTRUM_128_P2) {
			//Spectrum 32k rom

                                leidos=fread(rom_mem_table[0],1,32768,ptr_romfile);
                                if (leidos!=32768) {
				 	cpu_panic("Error loading ROM");
	                         }

		}

		else if (MACHINE_IS_SPECTRUM_P2A) {

			//Spectrum 64k rom

                                leidos=fread(rom_mem_table[0],1,65536,ptr_romfile);
                                if (leidos!=65536) {
				 	cpu_panic("Error loading ROM");
                                 }
		}

		else if (MACHINE_IS_ZXUNO) {
			//107 bytes rom
                        //leidos=fread(memoria_spectrum,1,56,ptr_romfile);
                        //if (leidos!=56) {
                        //                cpu_panic("Error loading ROM");
                        //}

			//Max 8kb rom
                        leidos=fread(memoria_spectrum,1,8192,ptr_romfile);
			//Un minimo de rom...
                        if (leidos<1) {
                                        cpu_panic("Error loading ROM");
                        }

			debug_printf (VERBOSE_DEBUG,"Read %d bytes of rom file %s",leidos,romfilename);

			zxuno_load_spi_flash();
		}

	       else if (MACHINE_IS_CHLOE) {
                        //SE Basic IV 32k rom

                                leidos=fread(chloe_rom_mem_table[0],1,32768,ptr_romfile);
                                if (leidos!=32768) {
                                        cpu_panic("Error loading ROM");
                                 }

                }

               else if (MACHINE_IS_PRISM) {
                        //320k rom
/*
ROM page	file	size
0		48.rom					16k
1		48.rom					16k
2		48.rom					16k
3		48.rom					16k
4		se.rom					32k
6		se.rom  				32k
8		128.rom 				32k
10		128.rom 				32k
12		128.rom 				32k
14		128.rom 				32k
16		alternaterom_plus3e_mmcen3eE.rom	64k
20 end

Total 20 pages=320 Kb
*/

                                leidos=fread(prism_rom_mem_table[0],1,320*1024,ptr_romfile);
                                if (leidos!=320*1024) {
                                        cpu_panic("Error loading ROM");
                                 }


				prism_load_failsafe_rom();

                }

		else if (MACHINE_IS_TBBLUE) {
			//Carga en memoria de rom de fpga, cargado dos veces
			leidos=fread(tbblue_fpga_rom,1,8192,ptr_romfile);
			memcpy(&tbblue_fpga_rom[8192],tbblue_fpga_rom,8192);
                                if (leidos!=8192) {
                                        cpu_panic("Error loading ROM");
                                 }


                }




              else if (MACHINE_IS_TIMEX_TS2068) {

                                leidos=fread(timex_rom_mem_table[0],1,16384,ptr_romfile);
                                if (leidos!=16384) {
                                        cpu_panic("Error loading ROM");
                                 }

                                leidos=fread(timex_ex_ram_mem_table[0],1,8192,ptr_romfile);
                                if (leidos!=8192) {
                                        cpu_panic("Error loading ROM");
                                 }



                }


		else if (MACHINE_IS_ZX80) {
			//ZX80
                                leidos=fread(memoria_spectrum,1,4096,ptr_romfile);
                                if (leidos!=4096) {
					cpu_panic("Error loading ROM");
                                }
		}


		else if (MACHINE_IS_ZX81) {

			//ZX81
                                leidos=fread(memoria_spectrum,1,8192,ptr_romfile);
                                if (leidos!=8192) {
					cpu_panic("Error loading ROM");
                                }
		}

                else if (MACHINE_IS_ACE) {

                        //Jupiter Ace
                                leidos=fread(memoria_spectrum,1,8192,ptr_romfile);
                                if (leidos!=8192) {
                                        cpu_panic("Error loading ROM");
                                }
                }


		else if (MACHINE_IS_Z88) {
			//Z88
			//leer maximo 512 kb de ROM
			leidos=fread(z88_puntero_memoria,1,512*1024,ptr_romfile);
                                if (leidos<=0) {
                                        cpu_panic("Error loading ROM");
                                }

			z88_internal_rom_size=leidos-1;

		}


	      else if (MACHINE_IS_CPC_464) {
                        //32k rom

                                leidos=fread(cpc_rom_mem_table[0],1,32768,ptr_romfile);
                                if (leidos!=32768) {
                                        cpu_panic("Error loading ROM");
                                 }

                }


              else if (MACHINE_IS_SAM) {
                        //32k rom

                                leidos=fread(sam_rom_memory[0],1,32768,ptr_romfile);
                                if (leidos!=32768) {
                                        cpu_panic("Error loading ROM");
                                 }

                }

		else if (MACHINE_IS_QL) {
			//minimo 16kb,maximo 128k
			        leidos=fread(memoria_ql,1,131072,ptr_romfile);

																//Minimo 16kb
                                if (leidos<16384) {
                                        cpu_panic("Error loading ROM");
                                 }

		}


                fclose(ptr_romfile);

}



void segint_signal_handler(int sig)
{

	debug_printf (VERBOSE_INFO,"Sigint (CTRL+C) received");

        //para evitar warnings al compilar
        sig++;

//Primero de todo detener el pthread del emulador, que no queremos que siga activo el emulador con el pthread de fondo mientras
//se ejecuta el end_emulator
#ifdef USE_PTHREADS
        debug_printf (VERBOSE_INFO,"Ending main loop thread");
        pthread_cancel(thread_main_loop);
#endif


	end_emulator();


}

void segterm_signal_handler(int sig)
{

        debug_printf (VERBOSE_INFO,"Sigterm received");

        //para evitar warnings al compilar
        sig++;

//Primero de todo detener el pthread del emulador, que no queremos que siga activo el emulador con el pthread de fondo mientras
//se ejecuta el end_emulator
#ifdef USE_PTHREADS
        debug_printf (VERBOSE_INFO,"Ending main loop thread");
        pthread_cancel(thread_main_loop);
#endif


        end_emulator();


}



void segfault_signal_handler(int sig)
{
	//para evitar warnings al compilar
	sig++;

	cpu_panic("Segmentation fault");
}


/* TODO testear senyal sigbus
void sigbus_signal_handler(int sig)
{
        //para evitar warnings al compilar
        sig++;

        cpu_panic("Bus error");
}
*/


void floatingpoint_signal_handler(int sig)
{
        //para evitar warnings al compilar
        sig++;

        cpu_panic("Floating point exception");
}


void main_init_video(void)
{
		//Video init
                debug_printf (VERBOSE_INFO,"Initializing Video Driver");

                //gestion de fallback y con driver indicado

                //scr_init_pantalla=NULL;

#ifdef USE_COCOA
                add_scr_init_array("cocoa",scrcocoa_init,set_scrdriver_cocoa);
                if (!strcmp(driver_screen,"cocoa")) {
                        set_scrdriver_cocoa();
                }
#endif


#ifdef COMPILE_XWINDOWS
                add_scr_init_array("xwindows",scrxwindows_init,set_scrdriver_xwindows);
                if (!strcmp(driver_screen,"xwindows")) {
                        set_scrdriver_xwindows();
                }
#endif


#ifdef COMPILE_SDL
                add_scr_init_array("sdl",scrsdl_init,set_scrdriver_sdl);
                if (!strcmp(driver_screen,"sdl")) {
                        set_scrdriver_sdl();
                }
#endif



#ifdef COMPILE_FBDEV
		add_scr_init_array("fbdev",scrfbdev_init,set_scrdriver_fbdev);
                if (!strcmp(driver_screen,"fbdev")) {
                        set_scrdriver_fbdev();
		}
#endif

#ifdef COMPILE_CACA
                add_scr_init_array("caca",scrcaca_init,set_scrdriver_caca);
                if (!strcmp(driver_screen,"caca")) {
                        set_scrdriver_caca();
                }
#endif

#ifdef COMPILE_AA
                add_scr_init_array("aa",scraa_init,set_scrdriver_aa);
                if (!strcmp(driver_screen,"aa")) {
                        set_scrdriver_aa();
                }
#endif


#ifdef COMPILE_CURSES
                add_scr_init_array("curses",scrcurses_init,set_scrdriver_curses);

                if (!strcmp(driver_screen,"curses")) {
                        set_scrdriver_curses();
                }
#endif

#ifdef COMPILE_STDOUT
                add_scr_init_array("stdout",scrstdout_init,set_scrdriver_stdout);
                if (!strcmp(driver_screen,"stdout")) {
                        set_scrdriver_stdout();
                }
#endif

#ifdef COMPILE_SIMPLETEXT
                add_scr_init_array("simpletext",scrsimpletext_init,set_scrdriver_simpletext);
                if (!strcmp(driver_screen,"simpletext")) {
                        set_scrdriver_simpletext();
                }
#endif


                //Y finalmente null video driver
                add_scr_init_array("null",scrnull_init,set_scrdriver_null);
                if (!strcmp(driver_screen,"null")) {
                        set_scrdriver_null();
                }



                if (try_fallback_video.v==1) {
                        //probar drivers de video
                        do_fallback_video();
                }

                //no probar. Inicializar driver indicado. Si falla, fallback a null
                else {
                        if (scr_init_pantalla()) {
                                debug_printf (VERBOSE_ERR,"Error using video output driver %s. Fallback to null",driver_screen);
				set_scrdriver_null();
				scr_init_pantalla();
                        }
                }


}

void main_init_audio(void)
{
		debug_printf (VERBOSE_INFO,"Initializing Audio");
                //Audio init
                audio_init=NULL;
                audio_buffer_switch.v=0;

                interrupt_finish_sound.v=0;
                audio_playing.v=1;

                audio_buffer_one=audio_buffer_oneandtwo;
                audio_buffer_two=&audio_buffer_oneandtwo[AUDIO_BUFFER_SIZE];

                set_active_audio_buffer();

		audio_empty_buffer();

                //gestion de fallback y con driver indicado.

#ifdef COMPILE_PULSE
                add_audio_init_array("pulse",audiopulse_init,set_audiodriver_pulse);
                if (!strcmp(driver_audio,"pulse")) {
                        set_audiodriver_pulse();

                }
#endif


#ifdef COMPILE_ALSA
                add_audio_init_array("alsa",audioalsa_init,set_audiodriver_alsa);
                if (!strcmp(driver_audio,"alsa")) {
                        set_audiodriver_alsa();

                }
#endif


#ifdef COMPILE_COREAUDIO
                add_audio_init_array("coreaudio",audiocoreaudio_init,set_audiodriver_coreaudio);
                if (!strcmp(driver_audio,"coreaudio")) {
                        set_audiodriver_coreaudio();

                }
#endif


#ifdef COMPILE_SDL
                add_audio_init_array("sdl",audiosdl_init,set_audiodriver_sdl);
                if (!strcmp(driver_audio,"sdl")) {
                        set_audiodriver_sdl();

                }
#endif


#ifdef COMPILE_DSP
                add_audio_init_array("dsp",audiodsp_init,set_audiodriver_dsp);
                if (!strcmp(driver_audio,"dsp")) {
                        set_audiodriver_dsp();

                }
#endif




                //Y finalmente null audio driver
                add_audio_init_array("null",audionull_init,set_audiodriver_null);
                if (!strcmp(driver_audio,"null")) {
                        set_audiodriver_null();

                }





                if (try_fallback_audio.v==1) {
                        //probar drivers de audio
                        do_fallback_audio();
                }

                //no probar. Inicializar driver indicado. Si falla, fallback a null
                else {
                        if (audio_init()) {
				fallback_audio_null();
                        }
                }

}

char *param_custom_romfile=NULL;
z80_bit opcion_no_splash;


z80_bit command_line_zx8081_vsync_sound={0};
z80_bit command_line_wrx={0};
z80_bit command_line_spectra={0};
z80_bit command_line_timex_video={0};
z80_bit command_line_spritechip={0};
z80_bit command_line_ulaplus={0};
z80_bit command_line_chroma81={0};
z80_bit command_line_zxpand={0};
z80_bit command_line_mmc={0};
z80_bit command_line_zxmmc={0};
z80_bit command_line_divmmc={0};
z80_bit command_line_divmmc_ports={0};
z80_bit command_line_8bitide={0};

z80_bit command_line_ide={0};
z80_bit command_line_divide={0};

z80_bit command_line_dandanator={0};
z80_bit command_line_dandanator_push_button={0};
z80_bit command_line_superupgrade={0};

z80_bit command_line_set_breakpoints={0};

int command_line_vsync_minimum_lenght=0;

char *command_line_load_binary_file=NULL;
int command_line_load_binary_address;
int command_line_load_binary_length;

int command_line_chardetect_printchar_enabled=-1;





//cuantos botones-joystick a teclas definidas
int joystickkey_definidas=0;


//void parse_cmdline_options(int argc,char *argv[]) {
void parse_cmdline_options(void) {

		while (!siguiente_parametro()) {
			if (!strcmp(argv[puntero_parametro],"--help")) {
				cpu_help();
				exit(1);
			}

                        if (!strcmp(argv[puntero_parametro],"--experthelp")) {
                                cpu_help_expert();
                                exit(1);
                        }

			if (!strcmp(argv[puntero_parametro],"--helpcustomconfig")) {
				customconfig_help();
				exit(1);
			}


			if (!strcmp(argv[puntero_parametro],"--showcompileinfo")) {
                                show_compile_info();
                                exit(1);
                        }



			if (!strcmp(argv[puntero_parametro],"--debugconfigfile")) {
				//Este parametro aqui se ignora, solo se lee antes del parseo del archivo de configuracion
                        }

			else if (!strcmp(argv[puntero_parametro],"--noconfigfile")) {
                                //Este parametro aqui se ignora, solo se lee antes del parseo del archivo de configuracion
                        }

			else if (!strcmp(argv[puntero_parametro],"--saveconf-on-exit")) {
				save_configuration_file_on_exit.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--zoomx")) {
				siguiente_parametro_argumento();
				zoom_x=atoi(argv[puntero_parametro]);
			}

			else if (!strcmp(argv[puntero_parametro],"--zoomy")) {
				siguiente_parametro_argumento();
				zoom_y=atoi(argv[puntero_parametro]);
			}

			else if (!strcmp(argv[puntero_parametro],"--zoom")) {
				siguiente_parametro_argumento();
				zoom_y=zoom_x=atoi(argv[puntero_parametro]);
			}

			else if (!strcmp(argv[puntero_parametro],"--frameskip")) {
                                siguiente_parametro_argumento();
                                frameskip=atoi(argv[puntero_parametro]);
				if (frameskip>49 || frameskip<0) {
					printf ("Frameskip out of range\n");
					exit(1);
				}
    }

			else if (!strcmp(argv[puntero_parametro],"--disable-autoframeskip")) {
					autoframeskip.v=0;
				}


			else if (!strcmp(argv[puntero_parametro],"--nochangeslowparameters")) {
				no_cambio_parametros_maquinas_lentas.v=1;
			}


			else if (!strcmp(argv[puntero_parametro],"--fullscreen")) {
				ventana_fullscreen=1;
			}

                        else if (!strcmp(argv[puntero_parametro],"--verbose")) {
                                siguiente_parametro_argumento();
                                verbose_level=atoi(argv[puntero_parametro]);
				if (verbose_level<0 || verbose_level>4) {
					printf ("Invalid Verbose level\n");
					exit(1);
				}
                        }

			else if (!strcmp(argv[puntero_parametro],"--nodisableconsole")) {
				//Parametro que solo es de Windows, pero lo admitimos en cualquier sistema
				windows_no_disable_console.v=1;
			}

			/*
		        else if (!strcmp(argv[puntero_parametro],"--invespokerom")) {
                                siguiente_parametro_argumento();
                                valor_poke_rom=atoi(argv[puntero_parametro]);
                        }
			*/



                        else if (!strcmp(argv[puntero_parametro],"--cpuspeed")) {
                                siguiente_parametro_argumento();
                                porcentaje_velocidad_emulador=atoi(argv[puntero_parametro]);
                                if (porcentaje_velocidad_emulador<10 || porcentaje_velocidad_emulador>1000) {
                                        printf ("Invalid CPU percentage\n");
                                        exit(1);
                                }
                        }

			else if (!strcmp(argv[puntero_parametro],"--denyturbozxunoboot")) {
					zxuno_deny_turbo_bios_boot.v=1;
			}

                        else if (!strcmp(argv[puntero_parametro],"--zx8081mem")) {
                                siguiente_parametro_argumento();
				int valor=atoi(argv[puntero_parametro]);
				if (valor<1 || valor>16) {
					printf ("Invalid RAM value\n");
					exit(1);
				}
				set_zx8081_ramtop(valor);
                        }

			else if (!strcmp(argv[puntero_parametro],"--acemem")) {
                                siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                if (valor!=3 && valor!=19 && valor!=35) {
                                        printf ("Invalid RAM value\n");
                                        exit(1);
                                }
                                set_ace_ramtop(valor);
                        }

                        else if (!strcmp(argv[puntero_parametro],"--videozx8081")) {
                                siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                if (valor<1 || valor>16) {
                                        printf ("Invalid threshold value\n");
                                        exit(1);
                                }
				simulate_screen_zx8081.v=1;
				umbral_simulate_screen_zx8081=valor;
                        }




			else if (!strcmp(argv[puntero_parametro],"--scr")) {
				siguiente_parametro_argumento();
				//cargar pantalla
				scrfile=argv[puntero_parametro];
			}

			/*
			else if (!strcmp(argv[puntero_parametro],"--step")) {
				cpu_step_mode.v=1;
			}
			*/



			else if (!strcmp(argv[puntero_parametro],"--tape")) {
				siguiente_parametro_argumento();
				insert_tape_cmdline(argv[puntero_parametro]);
			}

			else if (!strcmp(argv[puntero_parametro],"--realtape")) {
                                siguiente_parametro_argumento();
				realtape_name=argv[puntero_parametro];
			}



                        else if (!strcmp(argv[puntero_parametro],"--snap")) {
				siguiente_parametro_argumento();
				insert_snap_cmdline(argv[puntero_parametro]);
                        }


                        else if (!strcmp(argv[puntero_parametro],"--outtape")) {
                                siguiente_parametro_argumento();
                                tape_out_file=argv[puntero_parametro];
                        }


                        else if (!strcmp(argv[puntero_parametro],"--noautoload")) {
				noautoload.v=1;
                        }


			//eprom y flash cards de z88 hacen lo mismo que quickload
			else if (!strcmp(argv[puntero_parametro],"--slotcard")) {
                                siguiente_parametro_argumento();
				//ver si extension valida
				if (
				      !util_compare_file_extension(argv[puntero_parametro],"epr")
				 ||   !util_compare_file_extension(argv[puntero_parametro],"63")
				 ||   !util_compare_file_extension(argv[puntero_parametro],"eprom")
				 ||   !util_compare_file_extension(argv[puntero_parametro],"flash")

				 ){
					quickload_inicial.v=1;
					quickload_nombre=argv[puntero_parametro];
				}
				else {
                                        printf ("Invalid extension for eprom/flash card\n");
					exit(1);
				}

                        }


                        else if (!strcmp(argv[puntero_parametro],"--vo")) {
				int drivervook=0;
				try_fallback_video.v=0;

                                siguiente_parametro_argumento();
                                driver_screen=argv[puntero_parametro];

#ifdef USE_COCOA
				if (!strcmp(driver_screen,"cocoa")) drivervook=1;
#endif

#ifdef COMPILE_XWINDOWS
				if (!strcmp(driver_screen,"xwindows")) drivervook=1;
#endif

#ifdef COMPILE_SDL
                                if (!strcmp(driver_screen,"sdl")) drivervook=1;
#endif


#ifdef COMPILE_FBDEV
				if (!strcmp(driver_screen,"fbdev")) drivervook=1;
#endif

#ifdef COMPILE_CURSES
				if (!strcmp(driver_screen,"curses")) drivervook=1;
#endif

#ifdef COMPILE_AA
				if (!strcmp(driver_screen,"aa")) drivervook=1;
#endif

#ifdef COMPILE_CACA
				if (!strcmp(driver_screen,"caca")) drivervook=1;
#endif

#ifdef COMPILE_STDOUT
				if (!strcmp(driver_screen,"stdout")) drivervook=1;
#endif

#ifdef COMPILE_SIMPLETEXT
                                if (!strcmp(driver_screen,"simpletext")) drivervook=1;
#endif


				if (!strcmp(driver_screen,"null")) drivervook=1;

				if (!drivervook) {
					printf ("Video Driver %s not supported\n",driver_screen);
					exit(1);
				}



                        }

#ifdef COMPILE_AA
                        else if (!strcmp(argv[puntero_parametro],"--aaslow")) {
                                scraa_fast=0;
                        }
#endif




                        else if (!strcmp(argv[puntero_parametro],"--ao")) {
				try_fallback_audio.v=0;
                                int driveraook=0;
                                siguiente_parametro_argumento();
                                driver_audio=argv[puntero_parametro];



#ifdef COMPILE_DSP
                                if (!strcmp(driver_audio,"dsp")) driveraook=1;
#endif

#ifdef COMPILE_SDL
                                if (!strcmp(driver_audio,"sdl")) driveraook=1;
#endif


#ifdef COMPILE_ALSA
                                if (!strcmp(driver_audio,"alsa")) driveraook=1;
#endif

#ifdef COMPILE_PULSE
                                if (!strcmp(driver_audio,"pulse")) driveraook=1;
#endif


#ifdef COMPILE_COREAUDIO
                                if (!strcmp(driver_audio,"coreaudio")) driveraook=1;
#endif

                                if (!strcmp(driver_audio,"null")) driveraook=1;

                                if (!driveraook) {
                                        printf ("Audio Driver %s not supported\n",driver_audio);
                                        exit(1);
                                }



                        }

                        else if (!strcmp(argv[puntero_parametro],"--aofile")) {
                                siguiente_parametro_argumento();
                                aofilename=argv[puntero_parametro];
                        }

                        else if (!strcmp(argv[puntero_parametro],"--vofile")) {
                                siguiente_parametro_argumento();
                                vofilename=argv[puntero_parametro];
                        }

                        else if (!strcmp(argv[puntero_parametro],"--vofilefps")) {
                                siguiente_parametro_argumento();

                                int valor=atoi(argv[puntero_parametro]);
                                if (valor==1 || valor==2 || valor==5 || valor==10 || valor==25 || valor==50) {
					vofile_fps=50/valor;
				}

				else {
				        printf ("Invalid FPS value\n");
                                        exit(1);
                                }

                        }


                        else if (!strcmp(argv[puntero_parametro],"--disableayspeech")) {
				ay_speech_enabled.v=0;
                        }

			else if (!strcmp(argv[puntero_parametro],"--disableenvelopes")) {
                                ay_envelopes_enabled.v=0;
                        }



                        else if (!strcmp(argv[puntero_parametro],"--debugregisters")) {
				debug_registers=1;
                        }

#ifdef USE_XEXT
			else if (!strcmp(argv[puntero_parametro],"--disableshm"))
				disable_shm=1;
#endif

			else if (!strcmp(argv[puntero_parametro],"--disableborder")) {
				border_enabled.v=0;
			}

			else if (!strcmp(argv[puntero_parametro],"--hidemousepointer")) {
				mouse_pointer_shown.v=0;
			}

			/*else if (!strcmp(argv[puntero_parametro],"--overlayinfo")) {
				enable_second_layer();
			}*/

			else if (!strcmp(argv[puntero_parametro],"--disablefooter")) {
				disable_footer();
			}

			else if (!strcmp(argv[puntero_parametro],"--disablemultitaskmenu")) {
                                menu_multitarea=0;
			}


			else if (!strcmp(argv[puntero_parametro],"--machine")) {
				char *machine_name;
				siguiente_parametro_argumento();
                                machine_name=argv[puntero_parametro];

				if (set_machine_type_by_name(machine_name)) {
					exit(1);
                                }

			}

			else if (!strcmp(argv[puntero_parametro],"--videofastblack")) {
				video_fast_mode_emulation.v=1;
			}


			else if (!strcmp(argv[puntero_parametro],"--zx8081vsyncsound")) {
				command_line_zx8081_vsync_sound.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--zx8081ram8K2000")) {
                                ram_in_8192.v=1;
                        }

                        else if (!strcmp(argv[puntero_parametro],"--zx8081ram16K8000")) {
                                enable_ram_in_32768();
                        }

                        else if (!strcmp(argv[puntero_parametro],"--zx8081ram16KC000")) {
                                enable_ram_in_49152();
                        }

			else if (!strcmp(argv[puntero_parametro],"--autodetectwrx")) {
                                autodetect_wrx.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--wrx")) {
                                command_line_wrx.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--vsync-minimum-length")) {
		                siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                if (valor<100 || valor>999) {
                                        printf ("Invalid vsync lenght value\n");
                                        exit(1);
                                }
                                command_line_vsync_minimum_lenght=valor;
			}

			else if (!strcmp(argv[puntero_parametro],"--chroma81")) {
                                command_line_chroma81.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--romfile")) {
                                siguiente_parametro_argumento();
                                param_custom_romfile=argv[puntero_parametro];
			}

                        else if (!strcmp(argv[puntero_parametro],"--smartloadpath")) {
                                siguiente_parametro_argumento();
				sprintf(quickload_file,"%s/",argv[puntero_parametro]);

                                quickfile=quickload_file;
                        }


			else if (!strcmp(argv[puntero_parametro],"--loadbinarypath")) {
                                siguiente_parametro_argumento();
                                sprintf(binary_file_load,"%s/",argv[puntero_parametro]);
			}

			else if (!strcmp(argv[puntero_parametro],"--zxunospifile")) {
                                siguiente_parametro_argumento();
				sprintf(zxuno_flash_spi_name,"%s",argv[puntero_parametro]);
                        }

			else if (!strcmp(argv[puntero_parametro],"--zxunospiwriteenable")) {
                                zxuno_flash_write_to_disk_enable.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--printerbitmapfile")) {
				siguiente_parametro_argumento();
                                zxprinter_enabled.v=1;
				zxprinter_bitmap_filename=argv[puntero_parametro];
				zxprinter_file_bitmap_init();
			}

			else if (!strcmp(argv[puntero_parametro],"--printertextfile")) {
                                siguiente_parametro_argumento();
                                zxprinter_enabled.v=1;
                                zxprinter_ocr_filename=argv[puntero_parametro];
                                zxprinter_file_ocr_init();
                        }

			else if (!strcmp(argv[puntero_parametro],"--redefinekey")) {
				z80_byte tecla_original, tecla_redefinida;
				siguiente_parametro_argumento();
				tecla_original=parse_string_to_number(argv[puntero_parametro]);

				siguiente_parametro_argumento();
				tecla_redefinida=parse_string_to_number(argv[puntero_parametro]);

				if (util_add_redefinir_tecla(tecla_original,tecla_redefinida)) {
					exit(1);
				}
			}


			else if (!strcmp(argv[puntero_parametro],"--autoloadsnap")) {
                        	autoload_snapshot_on_start.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--autosavesnap")) {
                                autosave_snapshot_on_exit.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--autosnappath")) {
                                siguiente_parametro_argumento();
                                sprintf(autosave_snapshot_path_buffer,"%s/",argv[puntero_parametro]);
                        }

			else if (!strcmp(argv[puntero_parametro],"--loadbinary")) {
				siguiente_parametro_argumento();
				command_line_load_binary_file=argv[puntero_parametro];
				siguiente_parametro_argumento();
				command_line_load_binary_address=parse_string_to_number(argv[puntero_parametro]);
				siguiente_parametro_argumento();
				command_line_load_binary_length=parse_string_to_number(argv[puntero_parametro]);

				//Y decimos cual ha sido ultimo archivo binario cargado
				sprintf(binary_file_load,"%s",command_line_load_binary_file);
			}


			else if (!strcmp(argv[puntero_parametro],"--disablearttext")) {
				texto_artistico.v=0;
			}


                        else if (!strcmp(argv[puntero_parametro],"--arttextthresold")) {
                                siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                if (valor<1 || valor>16) {
                                        printf ("Invalid threshold value\n");
                                        exit(1);
                                }
                                umbral_arttext=valor;
                        }


		        else if (!strcmp(argv[puntero_parametro],"--disableprintchartrap")) {
				command_line_chardetect_printchar_enabled=0;
			}

                        else if (!strcmp(argv[puntero_parametro],"--enableprintchartrap")) {
				command_line_chardetect_printchar_enabled=1;
                        }


		        else if (!strcmp(argv[puntero_parametro],"--autoredrawstdout")) {
				stdout_simpletext_automatic_redraw.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--sendansi")) {
                                screen_text_accept_ansi=1;
			}

                        else if (!strcmp(argv[puntero_parametro],"--linewidth")) {
                                siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                chardetect_line_width=valor;
                        }


		        else if (!strcmp(argv[puntero_parametro],"--automaticdetectchar")) {
				trap_char_detection_routine_number=TRAP_CHAR_DETECTION_ROUTINE_AUTOMATIC;
				chardetect_detect_char_enabled.v=1;
			}


			else if (!strcmp(argv[puntero_parametro],"--secondtrapchar")) {
				siguiente_parametro_argumento();
				int valor=atoi(argv[puntero_parametro]);
                               	chardetect_second_trap_char_dir=valor;
			}

                        else if (!strcmp(argv[puntero_parametro],"--thirdtrapchar")) {
				siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                chardetect_third_trap_char_dir=valor;
                        }

			else if (!strcmp(argv[puntero_parametro],"--linewidthwaitspace")) {
				chardetect_line_width_wait_space.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--secondtrapsum32")) {
				chardetect_second_trap_sum32.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--textspeechprogram")) {
				siguiente_parametro_argumento();
                                textspeech_filter_program=argv[puntero_parametro];
				//Si es ruta relativa, poner ruta absoluta
				if (!si_ruta_absoluta(textspeech_filter_program)) {
					//printf ("es ruta relativa\n");
					//lo metemos en el buffer de texto del menu usado para esto
					//TODO: quiza hacer esto con convert_relative_to_absolute pero esa funcion es para directorios,
					//no para directorios con archivo, por tanto quiza habria que hacer un paso intermedio separando
					//directorio de archivo
				        char directorio_actual[PATH_MAX];
				        getcwd(directorio_actual,PATH_MAX);

					sprintf (menu_buffer_textspeech_filter_program,"%s/%s",directorio_actual,textspeech_filter_program);
		                        textspeech_filter_program=menu_buffer_textspeech_filter_program;

					//printf ("ruta final: %s\n",textspeech_filter_program);

				}

				//Validar para windows que no haya espacios en la ruta
				textspeech_filter_program_check_spaces();

				//Y si es NULL es que han habido espacios, y salir del emulador
				if (textspeech_filter_program==NULL) exit(1);

			}

                        else if (!strcmp(argv[puntero_parametro],"--textspeechstopprogram")) {
                                siguiente_parametro_argumento();
                                textspeech_stop_filter_program=argv[puntero_parametro];
                                //Si es ruta relativa, poner ruta absoluta
                                if (!si_ruta_absoluta(textspeech_stop_filter_program)) {
                                        //printf ("es ruta relativa\n");
                                        //lo metemos en el buffer de texto del menu usado para esto
                                        char directorio_actual[PATH_MAX];
                                        getcwd(directorio_actual,PATH_MAX);

                                        sprintf (menu_buffer_textspeech_stop_filter_program,"%s/%s",directorio_actual,textspeech_stop_filter_program);
                                        textspeech_stop_filter_program=menu_buffer_textspeech_stop_filter_program;

                                        //printf ("ruta final: %s\n",textspeech_stop_filter_program);


                                        //Validar para windows que no haya espacios en la ruta
                                        textspeech_stop_filter_program_check_spaces();

                                        //Y si es NULL es que han habido espacios, y salir del emulador
                                        if (textspeech_stop_filter_program==NULL) exit(1);
                                }
                        }

			else if (!strcmp(argv[puntero_parametro],"--textspeechwait")) {
				textspeech_filter_program_wait.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--textspeechmenu")) {
                                textspeech_also_send_menu.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--textspeechtimeout")) {
                                siguiente_parametro_argumento();
				textspeech_timeout_no_enter=parse_string_to_number(argv[puntero_parametro]);
				if (textspeech_timeout_no_enter<0 || textspeech_timeout_no_enter>99) {
					printf ("Invalid value for textspeechtimeout\n");
					exit(1);
				}
			}

                        else if (!strcmp(argv[puntero_parametro],"--tool-sox-path")) {
                                siguiente_parametro_argumento();
                                sprintf (external_tool_sox,"%s",argv[puntero_parametro]);
			}

                        else if (!strcmp(argv[puntero_parametro],"--tool-unzip-path")) {
                                siguiente_parametro_argumento();
                                sprintf (external_tool_unzip,"%s",argv[puntero_parametro]);
			}

                        else if (!strcmp(argv[puntero_parametro],"--tool-gunzip-path")) {
                                siguiente_parametro_argumento();
                                sprintf (external_tool_gunzip,"%s",argv[puntero_parametro]);
			}

                        else if (!strcmp(argv[puntero_parametro],"--tool-tar-path")) {
                                siguiente_parametro_argumento();
                                sprintf (external_tool_tar,"%s",argv[puntero_parametro]);
			}

                        else if (!strcmp(argv[puntero_parametro],"--tool-unrar-path")) {
                                siguiente_parametro_argumento();
                                sprintf (external_tool_unrar,"%s",argv[puntero_parametro]);
			}

			else if (!strcmp(argv[puntero_parametro],"--mmc-file")) {
				siguiente_parametro_argumento();

                                //Si es ruta relativa, poner ruta absoluta
                                if (!si_ruta_absoluta(argv[puntero_parametro])) {
                                        //printf ("es ruta relativa\n");

                                        //TODO: quiza hacer esto con convert_relative_to_absolute pero esa funcion es para directorios,
                                        //no para directorios con archivo, por tanto quiza habria que hacer un paso intermedio separando
                                        //directorio de archivo
                                        char directorio_actual[PATH_MAX];
                                        getcwd(directorio_actual,PATH_MAX);

                                        sprintf (mmc_file_name,"%s/%s",directorio_actual,argv[puntero_parametro]);

                                }

				else {
					sprintf (mmc_file_name,"%s",argv[puntero_parametro]);
				}

			}

			else if (!strcmp(argv[puntero_parametro],"--enable-mmc")) {
				command_line_mmc.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--enable-divmmc-ports")) {
				command_line_divmmc_ports.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--enable-divmmc")) {
				command_line_divmmc.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--divmmc-rom")) {
				siguiente_parametro_argumento();
				strcpy(divmmc_rom_name,argv[puntero_parametro]);
			}

			else if (!strcmp(argv[puntero_parametro],"--enable-zxmmc")) {
				command_line_zxmmc.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--enable-8bit-ide")) {
                                command_line_8bitide.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--enable-zxpand")) {
				command_line_zxpand.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--zxpand-root-dir")) {
                                siguiente_parametro_argumento();

                                //Si es ruta relativa, poner ruta absoluta
                                if (!si_ruta_absoluta(argv[puntero_parametro])) {
                                        //printf ("es ruta relativa\n");
					convert_relative_to_absolute(argv[puntero_parametro],zxpand_root_dir);
                                }


				else {
	                                sprintf (zxpand_root_dir,"%s",argv[puntero_parametro]);
				}



                        }

                        else if (!strcmp(argv[puntero_parametro],"--ide-file")) {
                                siguiente_parametro_argumento();

                                //Si es ruta relativa, poner ruta absoluta
                                if (!si_ruta_absoluta(argv[puntero_parametro])) {
                                        //printf ("es ruta relativa\n");

                                        //TODO: quiza hacer esto con convert_relative_to_absolute pero esa funcion es para directorios,
                                        //no para directorios con archivo, por tanto quiza habria que hacer un paso intermedio separando
                                        //directorio de archivo
                                        char directorio_actual[PATH_MAX];
                                        getcwd(directorio_actual,PATH_MAX);

                                        sprintf (ide_file_name,"%s/%s",directorio_actual,argv[puntero_parametro]);

                                }

                                else {
                                        sprintf (ide_file_name,"%s",argv[puntero_parametro]);
                                }

                        }

                        else if (!strcmp(argv[puntero_parametro],"--enable-ide")) {
                                command_line_ide.v=1;
                        }

                        else if (!strcmp(argv[puntero_parametro],"--enable-divide")) {
                                command_line_divide.v=1;
                        }

												else if (!strcmp(argv[puntero_parametro],"--divide-rom")) {
													siguiente_parametro_argumento();
													strcpy(divide_rom_name,argv[puntero_parametro]);
												}


		        else if (!strcmp(argv[puntero_parametro],"--dandanator-rom")) {
                                siguiente_parametro_argumento();

                                //Si es ruta relativa, poner ruta absoluta
                                if (!si_ruta_absoluta(argv[puntero_parametro])) {
                                        //printf ("es ruta relativa\n");

                                        //TODO: quiza hacer esto con convert_relative_to_absolute pero esa funcion es para directorios,
                                        //no para directorios con archivo, por tanto quiza habria que hacer un paso intermedio separando
                                        //directorio de archivo
                                        char directorio_actual[PATH_MAX];
                                        getcwd(directorio_actual,PATH_MAX);

                                        sprintf (dandanator_rom_file_name,"%s/%s",directorio_actual,argv[puntero_parametro]);

                                }

                                else {
                                        sprintf (dandanator_rom_file_name,"%s",argv[puntero_parametro]);
                                }

                        }

                        else if (!strcmp(argv[puntero_parametro],"--enable-dandanator")) {
                                command_line_dandanator.v=1;
                        }

                        else if (!strcmp(argv[puntero_parametro],"--dandanator-press-button")) {
                                command_line_dandanator_push_button.v=1;
                        }

                        else if (!strcmp(argv[puntero_parametro],"--superupgrade-flash")) {
                                siguiente_parametro_argumento();

                                //Si es ruta relativa, poner ruta absoluta
                                if (!si_ruta_absoluta(argv[puntero_parametro])) {
                                        //printf ("es ruta relativa\n");

                                        //TODO: quiza hacer esto con convert_relative_to_absolute pero esa funcion es para directorios,
                                        //no para directorios con archivo, por tanto quiza habria que hacer un paso intermedio separando
                                        //directorio de archivo
                                        char directorio_actual[PATH_MAX];
                                        getcwd(directorio_actual,PATH_MAX);

                                        sprintf (superupgrade_rom_file_name,"%s/%s",directorio_actual,argv[puntero_parametro]);

                                }

                                else {
                                        sprintf (superupgrade_rom_file_name,"%s",argv[puntero_parametro]);
                                }

                        }

                        else if (!strcmp(argv[puntero_parametro],"--enable-superupgrade")) {
                                command_line_superupgrade.v=1;
                        }




#ifdef COMPILE_FBDEV
                	else if (!strcmp(argv[puntero_parametro],"--no-use-ttyfbdev")) {
				fbdev_no_uses_tty=1;
			}

                	else if (!strcmp(argv[puntero_parametro],"--no-use-ttyrawfbdev")) {
				fbdev_no_uses_ttyraw=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--use-all-res-fbdev")) {
                                fbdev_use_all_virtual_res=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--decimal-full-scale-fbdev")) {
				ventana_fullscreen=1;
                                fbdev_decimal_full_scale_fbdev=1;
			}

#ifdef EMULATE_RASPBERRY
			else if (!strcmp(argv[puntero_parametro],"--fbdev-margin-height")) {
				siguiente_parametro_argumento();
				int valor=atoi(argv[puntero_parametro]);
				if (valor<0) {
					printf ("Invalid margin height value\n");
					exit(1);
				}
				fbdev_margin_height=valor;
			}
			else if (!strcmp(argv[puntero_parametro],"--fbdev-margin-width")) {
				siguiente_parametro_argumento();
				int valor=atoi(argv[puntero_parametro]);
				if (valor<0) {
					printf ("Invalid margin width value\n");
					exit(1);
				}
				fbdev_margin_width=valor;
			}
#endif


#endif

                        else if (!strcmp(argv[puntero_parametro],"--noautoselectfileopt")) {
                                autoselect_snaptape_options.v=0;
                        }


			else if (!strcmp(argv[puntero_parametro],"--nosplash")) {
                                screen_show_splash_texts.v=0;
			}

			else if (!strcmp(argv[puntero_parametro],"--nowelcomemessage")) {
                                opcion_no_splash.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--realvideo")) {
				enable_rainbow();
			}

			else if (!strcmp(argv[puntero_parametro],"--enableulaplus")) {
				command_line_ulaplus.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--enablespectra")) {
				command_line_spectra.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--enabletimexvideo")) {
                                command_line_timex_video.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--enablezgx")) {
                                command_line_spritechip.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--disablerealbeeper")) {
				beeper_real_enabled=0;
      }

			else if (!strcmp(argv[puntero_parametro],"--disablebeeper")) {
				beeper_enabled.v=0;
			}

			else if (!strcmp(argv[puntero_parametro],"--enableturbosound")) {
                                turbosound_enabled.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--enablespecdrum")) {
																specdrum_enabled.v=1;
			}


			else if (!strcmp(argv[puntero_parametro],"--snoweffect")) {
				snow_effect_enabled.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--audiovolume")) {
                                siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);

			        if (valor>100 || valor<0) {
			                printf ("Invalid volume value\n");
					exit (1);
				}

        			audiovolume=valor;
			}

			else if (!strcmp(argv[puntero_parametro],"--ayplayer-end-exit")) {
				ay_player_exit_emulator_when_finish.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--ayplayer-end-no-repeat")) {
				ay_player_repeat_file.v=0;
			}

			else if (!strcmp(argv[puntero_parametro],"--ayplayer-inf-length")) {
				siguiente_parametro_argumento();
				int valor=atoi(argv[puntero_parametro]);
				if (valor<1 || valor>1310) {
					printf ("Invalid length value. Must be between 1 and 1310\n");
					exit(1);
				}
				ay_player_limit_infinite_tracks=valor*50;
			}

			else if (!strcmp(argv[puntero_parametro],"--ayplayer-any-length")) {
				siguiente_parametro_argumento();
				int valor=atoi(argv[puntero_parametro]);
				if (valor<1 || valor>1310) {
					printf ("Invalid length value. Must be between 1 and 1310\n");
					exit(1);
				}
				ay_player_limit_any_track=valor*50;
			}

			else if (!strcmp(argv[puntero_parametro],"--ayplayer-cpc")) {
				ay_player_cpc_mode.v=1;
			}


#ifdef COMPILE_ALSA
			else if (!strcmp(argv[puntero_parametro],"--alsaperiodsize")) {
		                 siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                if (valor!=2 && valor!=4) {
                                        printf ("Invalid Alsa Period size\n");
                                        exit(1);
                                }
                                alsa_periodsize=AUDIO_BUFFER_SIZE*valor;
			}


#ifdef USE_PTHREADS
                        else if (!strcmp(argv[puntero_parametro],"--fifoalsabuffersize")) {
                                 siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                if (valor<4 || valor>10) {
                                        printf ("Invalid Alsa Fifo Buffer size\n");
                                        exit(1);
                                }
				fifo_alsa_buffer_size=AUDIO_BUFFER_SIZE*valor;
                        }


#endif
#endif


#ifdef COMPILE_PULSE
#ifdef USE_PTHREADS
                        else if (!strcmp(argv[puntero_parametro],"--pulseperiodsize")) {
                                 siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                //if (valor!=2 && valor!=4 && valor!=1) {
                                if (valor<1 || valor>4) {
                                        printf ("Invalid Pulse Period size\n");
                                        exit(1);
                                }
                                pulse_periodsize=AUDIO_BUFFER_SIZE*valor;
                        }


                        else if (!strcmp(argv[puntero_parametro],"--fifopulsebuffersize")) {
                                 siguiente_parametro_argumento();
                                int valor=atoi(argv[puntero_parametro]);
                                if (valor<4 || valor>10) {
                                        printf ("Invalid Pulse Fifo Buffer size\n");
                                        exit(1);
                                }
                                fifo_pulse_buffer_size=AUDIO_BUFFER_SIZE*valor;
                        }


#endif
#endif


#ifdef COMPILE_SDL
			else if (!strcmp(argv[puntero_parametro],"--sdlsamplesize")) {
				siguiente_parametro_argumento();
				int valor=atoi(argv[puntero_parametro]);
				if (valor<128 || valor>2048) {
					printf ("Invalid SDL audio sample size\n");
					exit(1);
				}
				audiosdl_samples=valor;
			}

#endif


			else if (!strcmp(argv[puntero_parametro],"--simulaterealload")) {
                                tape_loading_simulate.v=1;
                        }

                        else if (!strcmp(argv[puntero_parametro],"--simulaterealloadfast")) {
                                tape_loading_simulate_fast.v=1;
                        }

                        else if (!strcmp(argv[puntero_parametro],"--blue")) {
                                screen_gray_mode |=1;
                        }

                        else if (!strcmp(argv[puntero_parametro],"--green")) {
                                screen_gray_mode |=2;
                        }

                        else if (!strcmp(argv[puntero_parametro],"--red")) {
                                screen_gray_mode |=4;
                        }

			else if (!strcmp(argv[puntero_parametro],"--inversevideo")) {
                                inverse_video.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--disabletooltips")) {
				tooltip_enabled.v=0;
			}

			else if (!strcmp(argv[puntero_parametro],"--disablemenu")) {
				menu_desactivado.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--forcevisiblehotkeys")) {
                                menu_force_writing_inverse_color.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--forceconfirmyes")) {
				force_confirm_yes.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--gui-style")) {
				siguiente_parametro_argumento();
				int i;
				for (i=0;i<ESTILOS_GUI;i++) {
					if (!strcasecmp(argv[puntero_parametro],definiciones_estilos_gui[i].nombre_estilo)) {
						estilo_gui_activo=i;
						set_charset();
						break;
					}
				}
				if (i==ESTILOS_GUI) {
					printf ("Invalid GUI style\n");
					exit(1);
				}
                        }


			else if (!strcmp(argv[puntero_parametro],"--keyboardspoolfile")) {
                                siguiente_parametro_argumento();
				//sprintf(input_file_keyboard_name_buffer,"%s",argv[puntero_parametro]);
				input_file_keyboard_name=argv[puntero_parametro];
			        input_file_keyboard_init();
			}

//Si no hay soporte de pthreads, estas opciones las permitimos pero luego no hace nada

			else if (!strcmp(argv[puntero_parametro],"--enable-remoteprotocol")) {
					remote_protocol_enabled.v=1;
		 }

		 else if (!strcmp(argv[puntero_parametro],"--remoteprotocol-port")) {
																siguiente_parametro_argumento();
																int valor=atoi(argv[puntero_parametro]);

						 if (valor>65535 || valor<1) {
										 printf ("Invalid port value\n");
				 exit (1);
			 }

						 remote_protocol_port=valor;
		 }


		 else if (!strcmp(argv[puntero_parametro],"--set-breakpoint")) {
			 siguiente_parametro_argumento();
			 int valor=atoi(argv[puntero_parametro]);
			 valor--;

			 siguiente_parametro_argumento();


			 if (valor<0 || valor>MAX_BREAKPOINTS_CONDITIONS-1) {
				 printf("Index %d out of range setting breakpoint \"%s\"\n",valor+1,argv[puntero_parametro]);
				 exit(1);
			 }

			 debug_set_breakpoint(valor,argv[puntero_parametro]);

			 command_line_set_breakpoints.v=1;

		 }


		 else if (!strcmp(argv[puntero_parametro],"--hardware-debug-ports")) {
			 hardware_debug_port.v=1;
		 }

			else if (!strcmp(argv[puntero_parametro],"--joystickemulated")) {
                                siguiente_parametro_argumento();
				if (realjoystick_set_type(argv[puntero_parametro])) {
                                        exit(1);
				}

			}


			else if (!strcmp(argv[puntero_parametro],"--disablerealjoystick")) {
				realjoystick_present.v=0;
			}

			else if (!strcmp(argv[puntero_parametro],"--joystickevent")) {
				char *text_button;
				char *text_event;

				//obtener boton
				siguiente_parametro_argumento();
				text_button=argv[puntero_parametro];

				//obtener evento
				siguiente_parametro_argumento();
				text_event=argv[puntero_parametro];

				//Y definir el evento
				if (realjoystick_set_button_event(text_button,text_event)) {
                                        exit(1);
                                }


			}

                        else if (!strcmp(argv[puntero_parametro],"--joystickkeybt")) {

				char *text_button;
                                char *text_key;

                                //obtener boton
                                siguiente_parametro_argumento();
                                text_button=argv[puntero_parametro];

                                //obtener tecla
                                siguiente_parametro_argumento();
                                text_key=argv[puntero_parametro];

				//Y definir el evento
                                if (realjoystick_set_button_key(text_button,text_key)) {
                                        exit(1);
                                }

			}


			else if (!strcmp(argv[puntero_parametro],"--joystickkeyev")) {

				char *text_event;
                                char *text_key;

                                //obtener evento
				siguiente_parametro_argumento();
				text_event=argv[puntero_parametro];

				//Y obtener tecla
				siguiente_parametro_argumento();
				text_key=argv[puntero_parametro];

	                        //Y definir el evento
                                if (realjoystick_set_event_key(text_event,text_key)) {
					exit (1);
                                }


			}

			else if (!strcmp(argv[puntero_parametro],"--clearkeylistonsmart")) {
				realjoystick_clear_keys_on_smartload.v=1;
			}

			else if (!strcmp(argv[puntero_parametro],"--cleareventlist")) {
                          	realjoystick_clear_events_array();
			}


			else if (!strcmp(argv[puntero_parametro],"--quickexit")) {
				quickexit.v=1;
			}





			//autodetectar que el parametro es un snap o cinta. Esto tiene que ser siempre el ultimo else if
			else if (quickload_valid_extension(argv[puntero_parametro])) {
			        quickload_inicial.v=1;
                		quickload_nombre=argv[puntero_parametro];
		        }

			else {

				//parametro desconocido
				printf ("Unknown parameter : %s\n\n",argv[puntero_parametro]);
				cpu_help();
				exit(1);
			}


		}

		//Fin de interpretacion de parametros

}



void emulator_main_loop(void)
{
	while (1) {
		if (menu_abierto==1) menu_inicio();

    //Bucle principal de ejecución de la cpu
    while (menu_abierto==0) {
    	cpu_core_loop();
    }


	}


}

#ifdef USE_PTHREADS
void *thread_main_loop_function(void *nada)
{
        emulator_main_loop();

	//aqui no llega nunca, lo hacemos solo para que no se queje el compilador
        nada=0;
        nada++;
	return NULL;
}
#endif


//Proceso inicial
int zesarux_main (int main_argc,char *main_argv[]) {
//int main (int main_argc,char *main_argv[]) {



//argc=main_argc;
//argv=main_argv;



	//de momento ponemos esto a null y los mensajes siempre saldran por un printf normal
	scr_messages_debug=NULL;



	printf ("ZEsarUX - ZX Second-Emulator And Released for UniX\n"
			"Copyright (C) 2013 Cesar Hernandez Bano\n\n"
			"This program comes with ABSOLUTELY NO WARRANTY\n"
			"This is free software, and you are welcome to redistribute it under certain conditions\n\n");
			printf ("Includes Musashi 3.4 - A portable Motorola M680x0 processor emulation engine.\n"
						"Copyright 1998-2002 Karl Stenerud. All rights reserved.\n\n");
			printf ("ZEsarUX Version: " EMULATOR_VERSION " Date: " EMULATOR_DATE " - " EMULATOR_EDITION_NAME "\n\n");



#ifdef SNAPSHOT_VERSION
	printf ("Build number: " BUILDNUMBER "\n");

	printf ("WARNING. This is a Snapshot version and not a stable one\n"
			 "Some features may not work, can suffer random crashes, abnormal CPU use, or lots of debug messages on console\n\n");
	sleep (3);
#endif






#ifdef DEBUG_SECOND_TRAP_STDOUT
		printf ("\n\nWARNING!!!! DEBUG_SECOND_TRAP_STDOUT enabled!!\n"
			"Enable this only when you want to find printing routines\n\n");
		sleep (3);
#endif



	//Unos cuantos valores por defecto
	cpu_step_mode.v=0;
	current_machine_type=1;
	noautoload.v=0;
	ay_speech_enabled.v=1;
	ay_envelopes_enabled.v=1;
	tapefile=NULL;
	realtape_name=NULL;
	tape_out_file=NULL;
	aofilename=NULL;
	aofile_inserted.v=0;
	vofilename=NULL;
	vofile_inserted.v=0;
	input_file_keyboard_inserted.v=0;
	input_file_keyboard_send_pause.v=1;


	scr_z88_cpc_load_keymap=NULL;



	scrfile=NULL;
	snapfile=NULL;
	modificado_border.v=1;
	border_enabled.v=1;

	scr_putpixel=NULL;

	simulate_screen_zx8081.v=0;
	keyboard_issue2.v=0;
	tape_any_flag_loading.v=0;

	video_interlaced_mode.v=0;

	tape_loadsave_inserted=0;

	//tape_load_inserted.v=0;
	//tape_save_inserted.v=0;

	menu_splash_text_active.v=0;
	opcion_no_splash.v=0;
	spec_smp_memory=NULL;

	autoselect_snaptape_options.v=1;

	tape_loading_simulate.v=0;
	tape_loading_simulate_fast.v=0;


//Valores de ZX80/81
//ZX80/81 con 16 kb
ramtop_zx8081=16383+16384;
ram_in_8192.v=0;
ram_in_32768.v=0;
ram_in_49152.v=0;
wrx_present.v=0;
zx8081_vsync_sound.v=0;
//video_zx8081_shows_vsync_on_display.v=0;
video_zx8081_estabilizador_imagen.v=1;
//video_zx8081_decremento_x_cuando_mayor=8;

//19KB (3+16)
ramtop_ace=16383+16384;

try_fallback_video.v=1;
try_fallback_audio.v=1;

video_fast_mode_emulation.v=0;

simulate_lost_vsync.v=0;


last_x_atributo=0;

snow_effect_enabled.v=0;

inverse_video.v=0;

kempston_mouse_emulation.v=0;


scr_driver_name="";
audio_driver_name="";

transaction_log_filename[0]=0;

#ifdef COMPILE_XWINDOWS
	#ifdef USE_XEXT
	#else
	disable_shm=1;
	#endif
#endif


texto_artistico.v=1;


rainbow_enabled.v=0;
autodetect_rainbow.v=1;
autodetect_wrx.v=0;

contend_enabled.v=1;

zxprinter_enabled.v=0;
zxprinter_motor.v=0;
zxprinter_power.v=0;

tooltip_enabled.v=1;

	autosave_snapshot_on_exit.v=0;
	autoload_snapshot_on_start.v=0;
	autosave_snapshot_path_buffer[0]=0;

	audio_ay_player_mem=NULL;


	clear_lista_teclas_redefinidas();

	debug_nested_cores_pokepeek_init();


	//esto va aqui, asi podemos parsear el establecer set-breakpoint desde linea de comandos
	init_breakpoints_table();


	//estos dos se inicializan para que al hacer set_emulator_speed, que se ejecuta antes de init audio,
	//si no hay driver inicializado, no llamarlos
	audio_end=NULL;
	audio_init=NULL;



	quickload_inicial.v=0;

//Establecer rutas de utilidades externas
#if defined(__APPLE__)
                sprintf (external_tool_tar,"/usr/bin/tar");
                sprintf (external_tool_gunzip,"/usr/bin/gunzip");
#endif

	realjoystick_set_default_functions();


		//por si lanzamos un cpu_panic antes de inicializar video, que esto este a NULL y podamos detectarlo para no ejecutarlo
	scr_end_pantalla=NULL;
	memoria_spectrum=NULL;


	//Primero parseamos archivo de configuracion
	debug_parse_config_file.v=0;

		//Si hay parametro de debug parse config file...
		//main_argc,char *main_argv[])
		//printf ("%d\n",main_argc);
		if (main_argc>1) {
			if (!strcmp(main_argv[1],"--debugconfigfile")) {
				debug_parse_config_file.v=1;
			}
		}


	int noconfigfile=0;

                if (main_argc>1) {
                        if (!strcmp(main_argv[1],"--noconfigfile")) {
				noconfigfile=1;
                        }
                }



	if (noconfigfile==0) {
			configfile_parse();

        	        argc=configfile_argc;
                	argv=configfile_argv;
			puntero_parametro=0;

        	        parse_cmdline_options();
	}


  //Luego parseamos parametros por linea de comandos
  argc=main_argc;
  argv=main_argv;
  puntero_parametro=0;

  parse_cmdline_options();

//Mensaje para la ZXSpectr edition (ZEsarUX 4.1)
//no se si para versiones superiores lo seguire manteniendo...

  printf ("Detected SoundBlaster at A220 I5 D1 T2 ... Just kidding ;) \n\n");

                /*
                printf ("386 Processor or higher detected\n"
                        "Using expanded memory (EMS)\n");
                */


#ifdef MINGW
        //Si no se ha pasado ningun parametro, ni parametro --nodisableconsole, sea en consola o en archivo de configuracion, liberar consola, con pausa de 2 segundos para que se vea un poco :P
        if (main_argc==1 && windows_no_disable_console.v==0) {
                sleep(2);
                printf ("Disabling text printing on this console. Specify --nodisableconsole or any other command line setting to avoid it\n");
                FreeConsole();
        }
#endif



		//guardamos zoom original. algunos drivers, como fbdev, lo modifican.
                zoom_x_original=zoom_x;
                zoom_y_original=zoom_y;


		//Pausa para leer texto de inicio, copyright, etc
		//desactivada sleep(1);

		//Inicializacion maquina


		//Init random value. Usado en AY Chip y Random ram
	init_randomize_noise_value();


	init_cpu_tables();

	//movemos esto antes, asi podemos parsear el establecer set-breakpoint desde linea de comandos
	//init_breakpoints_table();


	init_ulaplus_table();
	init_prism_palettes();
		//init_cpc_rgb_table();
	screen_init_colour_table();

	init_screen_addr_table();

	init_cpc_line_display_table();


	debug_printf (VERBOSE_INFO,"Starting emulator");




#ifdef USE_PTHREADS
		debug_printf (VERBOSE_INFO,"Using phtreads");
#else
		debug_printf (VERBOSE_INFO,"Not using phtreads");
#endif


	debug_printf (VERBOSE_INFO,"Initializing Machine");

	//Si hemos especificado una rom diferente por linea de comandos


	if (param_custom_romfile!=NULL) {
		set_machine(param_custom_romfile);
	}


	else {
		set_machine(NULL);
	}

	cold_start_cpu_registers();

	reset_cpu();

	//Algun parametro que se resetea con reset_cpu y/o set_machine y se puede haber especificado por linea de comandos
	if (command_line_zx8081_vsync_sound.v) zx8081_vsync_sound.v=1;




  //Inicializamos Video antes que el resto de cosas.
  main_init_video();

  //Activar deteccion automatica de rutina de impresion de caracteres, si conviene
	//Esto se hace tambien al inicializar cpu... Pero como al inicializar cpu aun no hemos inicializado driver video,
	//y por tanto no se sabe si hay stdout... Entonces hacemos esto justo despues de inicializar video
  //Activar deteccion automatica de rutina de impresion de caracteres, si conviene
  if (chardetect_detect_char_enabled.v) {
  	chardetect_init_automatic_char_detection();
  }


  set_putpixel_zoom();
	menu_init_footer();


	//Despues de inicializar video, llamar a esta funcion, por si hay que cambiar frameskip (especialmente en cocoa)
	screen_set_parameters_slow_machines();


	tape_init();

  tape_out_init();

	//Si hay realtape insertado
	if (realtape_name!=NULL) realtape_insert();


	main_init_audio();



	init_chip_ay();

	if (realjoystick_present.v==1) {
			if (realjoystick_init()) {
				realjoystick_present.v=0;
			}
	}

	if (aofilename!=NULL) {
			init_aofile();
	}

  if (vofilename!=NULL) {
      init_vofile();
  }



	//Load Screen
	if (scrfile!=NULL) {
		load_screen(scrfile);
	}



	scr_refresca_pantalla();

#ifdef USE_REALTIME
	//Establecemos prioridad realtime
struct sched_param sparam;

        if ((sparam.sched_priority = sched_get_priority_max(SCHED_RR))==-1) {
                debug_printf (VERBOSE_WARN,"Error setting realtime priority : %s",strerror(errno));
        }

        else if (sched_setscheduler(0, SCHED_RR, &sparam)) {
                debug_printf (VERBOSE_WARN,"Error setting realtime priority : %s",strerror(errno));
        }

	else debug_printf (VERBOSE_INFO,"Using realtime priority");
#endif



	//Capturar segmentation fault
	//desactivado normalmente en versiones snapshot
	signal(SIGSEGV, segfault_signal_handler);

	//Capturar floating point exception
	//desactivado normalmente en versiones snapshot
	signal(SIGFPE, floatingpoint_signal_handler);

  //Capturar sigbus. TODO probar en que casos salta
  //desactivado normalmente en versiones snapshot
  //signal(SIGBUS, sigbus_signal_handler);



	//Capturar segint (CTRL+C)
	signal(SIGINT, segint_signal_handler);

	//Capturar segterm
	signal(SIGTERM, segterm_signal_handler);



	//Inicio bucle principal
	reg_pc=0;
	interrupcion_maskable_generada.v=0;
	interrupcion_non_maskable_generada.v=0;
	interrupcion_timer_generada.v=0;

	z80_ejecutando_halt.v=0;

	esperando_tiempo_final_t_estados.v=0;
	framescreen_saltar=0;


	if (opcion_no_splash.v==0) set_splash_text();


	//Algun parametro que se resetea con reset_cpu y/o set_machine y se puede haber especificado por linea de comandos
	if (command_line_wrx.v) enable_wrx();

	if (command_line_load_binary_file!=NULL) {
		load_binary_file(command_line_load_binary_file,command_line_load_binary_address,command_line_load_binary_length);
	}



	if (command_line_chardetect_printchar_enabled != -1) {
		chardetect_printchar_enabled.v=command_line_chardetect_printchar_enabled;
	}

	if (command_line_spectra.v) spectra_enable();

	if (command_line_ulaplus.v) enable_ulaplus();

	if (command_line_timex_video.v) enable_timex_video();

	if (command_line_spritechip.v) spritechip_enable();


	if (command_line_chroma81.v) enable_chroma81();


	if (command_line_mmc.v) mmc_enable();

	if (command_line_divmmc_ports.v) {
		divmmc_mmc_ports_enable();
	}

	if (command_line_divmmc.v) {
		divmmc_mmc_ports_enable();
		divmmc_diviface_enable();
	}

	if (command_line_zxmmc.v) zxmmc_emulation.v=1;

	if (command_line_zxpand.v) zxpand_enable();


  if (command_line_ide.v) ide_enable();

  if (command_line_divide.v) {
		divide_ide_ports_enable();
		divide_diviface_enable();
  }


	if (command_line_8bitide.v) eight_bit_simple_ide_enable();


	//Dandanator
	if (command_line_dandanator.v) dandanator_enable();
	if (command_line_dandanator_push_button.v) dandanator_press_button();

	//Superupgrade
	if (command_line_superupgrade.v) superupgrade_enable(1);

	if (command_line_set_breakpoints.v) {
		if (debug_breakpoints_enabled.v==0) {
		        debug_breakpoints_enabled.v=1;
		        breakpoints_enable();
		}

	}


	start_timer_thread();

	gettimeofday(&z80_interrupts_timer_antes, NULL);


	//antes de cargar otros snapshots por linea de comandos, ver si hay autocarga
	//si hay autocarga, pero luego se indica otro snapshot, se cargara el del autoload pero despues el indicado...
	//y por tanto la autocarga no permanece
	if (autoload_snapshot_on_start.v) {
		autoload_snapshot();
	}


	//Ver si hay que cargar snapshot. considerar quickload
	if (quickload_inicial.v==1) {
		debug_printf(VERBOSE_INFO,"Smartloading %s",quickload_nombre);
		quickload(quickload_nombre);
	}

	else {
		debug_printf(VERBOSE_INFO,"See if we have to load snapshot...");
	  snapshot_load();
	}


	//Poner esto aqui porque se resetea al establecer parametros maquina en set_machine_params
	if (command_line_vsync_minimum_lenght) {
		minimo_duracion_vsync=command_line_vsync_minimum_lenght;
	}


	init_remote_protocol();

	//Inicio bucle de emulacion


//En SDL2 y SDL1, rutinas de refresco de pantalla se deben lanzar desde el thread principal. Por tanto:
#ifdef COMPILE_SDL
	if (!strcmp(driver_screen,"sdl")) {
		debug_printf (VERBOSE_INFO,"Calling main loop emulator on the main thread as it is required by SDL2");
		emulator_main_loop();

		//Aqui no se llega nunca pero por si acaso
		return 0;
	}
#endif

	//si no tenemos pthreads, entrar en el bucle principal tal cual
	//si hay pthreads, lanzarlo como thread aparte y quedarnos en un bucle con sleep

#ifdef USE_PTHREADS

	debug_printf (VERBOSE_INFO,"Calling main loop emulator on a thread");

                if (pthread_create( &thread_main_loop, NULL, &thread_main_loop_function, NULL) ) {
                        cpu_panic("Can not create main loop pthread");
                }
	#ifdef USE_COCOA

		//Si hay soporte COCOA, dejar solo el thread con el main loop y volver a main (de scrcocoa)

	#else
		//Bucle cerrado con sleep. El bucle main se ha lanzado como thread
		while (1) {
			timer_sleep(1000);
			//printf ("bucle con sleep\n");
		}
	#endif



#else
	debug_printf (VERBOSE_INFO,"Calling main loop emulator without threads");
	emulator_main_loop();
#endif

	//Aqui solo se llega en caso de cocoa

	return 0;

}



void end_emulator(void)
{
	debug_printf (VERBOSE_INFO,"End emulator");

	top_speed_timer.v=0;

//Si se ha llamado aqui desde otro sitio que no sea el pthread del main_loop_emulator, hay que destruir antes el pthread con:
//#ifdef USE_PTHREADS
//        debug_printf (VERBOSE_INFO,"Ending main loop thread");
//        pthread_cancel(thread_main_loop);
//#endif

	menu_abierto=0;

	if (save_configuration_file_on_exit.v) util_write_configfile();

	//end_remote_protocol(); porque si no, no se puede finalizar el emulador desde el puerto telnet
	if (!remote_calling_end_emulator.v) {
		end_remote_protocol();
	}

	reset_menu_overlay_function();

	cls_menu_overlay();

	close_aofile();
	close_vofile();
	close_zxprinter_bitmap_file();
	close_zxprinter_ocr_file();
	z88_flush_eprom_or_flash_to_disk();
	mmc_flush_flash_to_disk();
	ide_flush_flash_to_disk();


	audio_thread_finish();
	audio_playing.v=0;
	audio_end();

	//printf ("footer: %d\n",menu_footer);

	//Parece ser que el fadeout y en particular el refresco de pantalla no sienta muy bien
	//cuando se ejecuta desde remote protocol y con el driver cocoa gl. No se muy bien porque,
	//probado a poner un check de que no se refresque dos veces simultaneamente la pantalla y aun asi peta con segmentation fault
	if (!remote_calling_end_emulator.v) {
		if (!no_fadeout_exit.v) {
			scr_fadeout();
		}
	}


  scr_end_pantalla();

	//Borrar archivos de speech en Windows
	//Borrar archivo de lock
	textspeech_borrar_archivo_windows_lock_file();
	//Borrar archivo de speech
	textspeech_borrar_archivo_windows_speech_file();


	if (remote_calling_end_emulator.v) end_remote_protocol();

  exit(0);

}
