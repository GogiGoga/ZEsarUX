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

#include <stdio.h>


#include "ula.h"
#include "cpu.h"
#include "debug.h"
#include "screen.h"
#include "contend.h"
#include "menu.h"
#include "multiface.h"

//¿Como? Pues creando un puerto de debug, que todo lo que se escriba vaya a parar a la consola, así de sencillo, haces OUT 55555, 65 y aparece una A en la consola
//que ha lanzado ZesarUX. Alternativamente se podría poner algun puerto más, que lo escrito se interprete como un número,
//por ejemplo que si haces OUT 55556,65 no escriba una A, sino 65.
//http://www.zxuno.com/forum/viewtopic.php?f=39&t=611
z80_bit hardware_debug_port={0};

//#define ZESARUX_ZXI_PORT_REGISTER 0xCF3B
//#define ZESARIX_ZXI_PORT_DATA     0xDF3B

z80_byte zesarux_zxi_last_register=0;

z80_byte zesarux_zxi_registers_array[256];

//ultimo valor enviado al border, tal cual
z80_byte out_254_original_value;

//ultimo valor enviado al border, teniendo en cuenta mascara de inves
z80_byte out_254;

//a 1 indica teclado issue2 (bit 6 a 1). Sino, issue 3
z80_bit keyboard_issue2;

//ultimo atributo leido por la ULA
z80_byte last_ula_attribute=255;
//ultimo byte de pixel leido por la ULA
z80_byte last_ula_pixel=255;


z80_byte contador_parpadeo=16;
z80_bit estado_parpadeo;


z80_bit ula_late_timings={0};

z80_bit ula_im2_slow={0};

z80_bit pentagon_timing={0};


//Poder desactivar paginado de rom y ram
z80_bit ula_disabled_ram_paging={0};
z80_bit ula_disabled_rom_paging={0};

void ula_pentagon_timing_common(void)
{

        //Recalcular algunos valores cacheados
        recalcular_get_total_ancho_rainbow();
        recalcular_get_total_alto_rainbow();

        screen_set_video_params_indices();
        inicializa_tabla_contend();

        init_rainbow();
        init_cache_putpixel();

	//Parchecillo temporal, dado que el footer se desplaza una linea de caracteres hacia abajo al activar pentagon
	menu_init_footer();

}

//Activa timings de pentagon pero NO activa contended memory. Se hace para compatibilidad con funciones de zuxno
void ula_disable_pentagon_timing(void)
{
                pentagon_timing.v=0;

                set_machine_params();

	ula_pentagon_timing_common();
}


//Activa timings de pentagon pero NO desactiva contended memory. Se hace para compatibilidad con funciones de zuxno
void ula_enable_pentagon_timing_no_common(void)
{
	pentagon_timing.v=1;

                        screen_invisible_borde_superior=16;
                        screen_borde_superior=64;
                        screen_total_borde_inferior=48;

                        //los timings son realmente estos pero entonces necesitariamos mas tamanyo de ventana de ancho
                        /*screen_total_borde_izquierdo=64;
                        screen_total_borde_derecho=64;
                        screen_invisible_borde_derecho=64;*/

                        //dejamos estos que es el tamanyo normal
                        screen_total_borde_izquierdo=48;
                        screen_total_borde_derecho=48;
                        screen_invisible_borde_derecho=96;

                        screen_testados_linea=224;


}


void ula_enable_pentagon_timing(void)
{

	ula_enable_pentagon_timing_no_common();
	 ula_pentagon_timing_common();
}


z80_byte zesarux_zxi_read_last_register(void)
{
  return zesarux_zxi_last_register;
}

void zesarux_zxi_write_last_register(z80_byte value)
{
  zesarux_zxi_last_register=value;
}

void zesarux_zxi_write_register_value(z80_byte value)
{

  switch (zesarux_zxi_last_register) {
    case 0:
      //Bit 0.
      if (MACHINE_IS_INVES) {  //Solo lanzamos linea de debug, no hacemos accion, ese bit se leerá donde corresponda de la funcion peek de inves
        if ((value &1)==1) {
			    debug_printf (VERBOSE_DEBUG,"Show Inves Low RAM");
		    }
		    else {
			    debug_printf (VERBOSE_DEBUG,"Hide Inves Low RAM (normal situation)");
		    }
      }

    break;

    case 1:
      //HARDWARE_DEBUG_ASCII
        printf ("%c",(value>=32 && value<=127 ? value : '?')  );
        fflush(stdout);
    break;

    case 2:
      //HARDWARE_DEBUG_NUMBER
        printf ("%d",value);
			  fflush(stdout);
    break;

    case 3:
	/*
* Reg 3: ZEsarUX control register
Bit 0: Set to 1 to exit emulator
Bit 1-7: Unused


	*/
	if (value&1) {
		debug_printf(VERBOSE_INFO,"Exiting emulator because of a ZEsarUX ZXI port exit emulator operation");
		end_emulator();
	}
    break;
  }

  zesarux_zxi_registers_array[zesarux_zxi_last_register]=value;

}


z80_byte zesarux_zxi_read_register_value(void)
{
  return zesarux_zxi_registers_array[zesarux_zxi_last_register];
}



void generate_nmi(void)
{
	interrupcion_non_maskable_generada.v=1;
	if (multiface_enabled.v) {
		multiface_map_memory();
    multiface_lockout=0;
	}
}
