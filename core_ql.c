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

#include <time.h>
#include <sys/time.h>
#include <errno.h>


#include "cpu.h"
#include "debug.h"
#include "tape.h"
#include "audio.h"
#include "screen.h"
#include "ay38912.h"
#include "operaciones.h"
#include "snap.h"
#include "timer.h"
#include "menu.h"
#include "compileoptions.h"
#include "contend.h"
#include "utils.h"
#include "realjoystick.h"
#include "chardetect.h"
#include "m68k.h"
#include "ula.h"




z80_byte byte_leido_core_ql;
char buffer_disassemble[100000];

extern unsigned char ql_pc_intr;

int refresca=0;

void core_ql_trap_one(void)
{

  switch(m68k_get_reg(NULL,M68K_REG_D0)) {

      /*case 1:

      break;*/

      default:
        debug_printf (VERBOSE_PARANOID,"Trap 1. D0=%02XH",m68k_get_reg(NULL,M68K_REG_D0));
      break;

    }

}

void core_ql_trap_two(void)
{

  //int reg_a0;

  switch(m68k_get_reg(NULL,M68K_REG_D0)) {

      /*case 1:
        //Open a channel
        reg_a0=m68k_get_reg(NULL,M68K_REG_A0);
        printf ("Trap 2. D0=1 open a channel. Name Address: %06XH\n",reg_a0);
        //En A0 , direccion. primero word con longitud texto, luego texto
        int longitud_nombre=peek_byte_z80_moto(reg_a0)*256+peek_byte_z80_moto(reg_a0+1);
        printf ("Longitud nombre canal: %d\n",longitud_nombre);
      break;*/

      default:
      debug_printf (VERBOSE_PARANOID,"Trap 2. D0=%02XH",m68k_get_reg(NULL,M68K_REG_D0));
      break;

    }

}

void core_ql_trap_three(void)
{

  debug_printf (VERBOSE_PARANOID,"Trap 3. D0=%02XH A0=%08XH is : ",m68k_get_reg(NULL,M68K_REG_D0),m68k_get_reg(NULL,M68K_REG_A0));

  switch(m68k_get_reg(NULL,M68K_REG_D0)) {
    case 0x4:
      debug_printf (VERBOSE_PARANOID,"Trap 3: IO.EDLIN");
    break;

    case 0x7:
      debug_printf (VERBOSE_PARANOID,"Trap 3: IO.SSTRG");
    break;

    case 0x47:
      debug_printf (VERBOSE_PARANOID,"Trap 3: FS.HEADR");
    break;

    case 0x48:
      debug_printf (VERBOSE_PARANOID,"Trap 3: FS.LOAD");
      sleep(5);
    break;

    default:
      debug_printf (VERBOSE_PARANOID,"Trap 3: unknown");
    break;

  }
}

//bucle principal de ejecucion de la cpu de jupiter ace
void cpu_core_loop_ql(void)
{

                debug_get_t_stados_parcial_post();
                debug_get_t_stados_parcial_pre();


		timer_check_interrupt();

//#ifdef COMPILE_STDOUT
//              if (screen_stdout_driver) scr_stdout_printchar();
//#endif
//
//#ifdef COMPILE_SIMPLETEXT
//                if (screen_simpletext_driver) scr_simpletext_printchar();
//#endif
                if (chardetect_detect_char_enabled.v) chardetect_detect_char();
                if (chardetect_printchar_enabled.v) chardetect_printchar();



    //Traps de intercepcion de teclado
    //F1 y F2 iniciales
    /*
    Para parar y simular F1:

sb 2 pc=4AF6H
set-register pc=4AF8H  (saltamos el trap 3)
set-register D1=E8H    (devolvemos tecla f1)
(o para simular F2: set-register D1=ECH)
  */

    //Si pulsado F1 o F2
    //Trap ya no hace falta. Esto era necesario cuando la lectura de teclado no funcionaba y esta era la unica manera de simular F1 o F2
    /*
    if ( get_pc_register()==0x4af6 &&
        ((ql_keyboard_table[0]&2)==0 || (ql_keyboard_table[0]&8)==0)
        )     {


          debug_printf(VERBOSE_DEBUG,"QL Trap ROM: Read F1 or F2");

      //Saltar el trap 3
      m68k_set_reg(M68K_REG_PC,0x4AF8);

      //Si F1
      if ((ql_keyboard_table[0]&2)==0) m68k_set_reg(M68K_REG_D1,0xE8);
      //Pues sera F2
      else m68k_set_reg(M68K_REG_D1,0xEC);

    }
*/



    //Saltar otro trap que hace mdv1 boot
    /*
    04B4C trap    #$2
04B4E tst.l   D0
04B50 rts


TRAP #2 D0=$1
Open a channel
IO.OPEN


A0=00004BE4 :

L04BE4 DC.W    $0009
       DC.B    'MDV1_BOOT'
       DC.B    $00


saltamos ese trap
set-register pc=04B50h

    */

    if ( get_pc_register()==0x04B4C) {
      debug_printf(VERBOSE_DEBUG,"QL Trap ROM: Skipping MDV1 boot");
      m68k_set_reg(M68K_REG_PC,0x04B50);
    }


    /* Lectura de tecla */
    if (get_pc_register()==0x02D40) {
      //Decir que hay una tecla pulsada
      //Temp solo cuando se pulsa enter
      //if ((puerto_49150&1)==0) {
      if (ql_pulsado_tecla()) {
        debug_printf(VERBOSE_DEBUG,"QL Trap ROM: Tell one key pressed");
        m68k_set_reg(M68K_REG_D7,0x01);
      }
      else {
        //printf ("no tecla pulsada\n");
        ql_mantenido_pulsada_tecla=0;
      }
    }

    //Decir 1 tecla pulsada
    if (get_pc_register()==0x02E6A) {
      //if ((puerto_49150&1)==0) {
      if (ql_pulsado_tecla()) {
        debug_printf(VERBOSE_DEBUG,"QL Trap ROM: Tell 1 key pressed");
        m68k_set_reg(M68K_REG_D1,1);
        //L02E6A MOVE.B  D1,D5         * d1=d5=d4 : number of bytes in buffer
      }
      else {
        m68k_set_reg(M68K_REG_D1,0); //0 teclas pulsadas
      }
    }


    if (get_pc_register()==0x0031e) {
      core_ql_trap_one();
    }


//00324 bsr     336
//Interceptar trap 2
/*
trap 2 salta a:
00324 bsr     336
*/
  if (get_pc_register()==0x00324) {
    core_ql_trap_two();
  }


    //Interceptar trap 2, con d0=1, cuando ya sabemos la direccion
    //IO.OPEN
/*
command@cpu-step> cs
PC: 032B4 SP: 2846E USP: 3FFC0 SR: 2000 :  S         A0: 0003FDEE A1: 0003EE00 A2: 00006906 A3: 00000670 A4: 00000012 A5: 0002846E A6: 00028000 A7: 0002846E D0: 00000001 D1: FFFFFFFF D2: 00000058 D3: 00000001 D4: 00000001 D5: 00000000 D6: 00000000 D7: 0000007F
032B4 subq.b  #1, D0

*/
    if (get_pc_register()==0x032B4) {
      //en A0
      char nombre_archivo[255];
      int reg_a0=m68k_get_reg(NULL,M68K_REG_A0);
      int longitud_nombre=peek_byte_z80_moto(reg_a0)*256+peek_byte_z80_moto(reg_a0+1);
      reg_a0 +=2;
      debug_printf (VERBOSE_PARANOID,"Lenght channel name: %d",longitud_nombre);

      char c;
      int i=0;
      for (;longitud_nombre;longitud_nombre--) {
        c=peek_byte_z80_moto(reg_a0++);
        nombre_archivo[i++]=c;
        //printf ("%c",c);
      }
      //printf ("\n");
      nombre_archivo[i++]=0;
      debug_printf (VERBOSE_PARANOID,"Channel name: %s",nombre_archivo);
      //sleep(1);

      //Hacer que si es mdv1_ ... volver

      //A7=0002846EH
      //A7=0002847AH
      //Incrementar A7 en 12
      //set-register pc=5eh. apunta a un rte
      char *buscar="mdv";
      char *encontrado;
      encontrado=util_strcasestr(nombre_archivo, buscar);
      if (encontrado) {
        debug_printf (VERBOSE_PARANOID,"Returning from trap without opening anything");
        m68k_set_reg(M68K_REG_PC,0x5e);
        int reg_a7=m68k_get_reg(NULL,M68K_REG_A7);
        reg_a7 +=12;
        m68k_set_reg(M68K_REG_A7,reg_a7);


        //Metemos channel id (A0) inventado
        m68k_set_reg(M68K_REG_A0,100);

        //Como decir no error
        /*
        pg 11 qltm.pdf
        When the TRAP operation is complete, control is returned to the program at the location following the TRAP instruction,
        with an error key in all 32 bits of D0. This key is set to zero if the operation has been completed successfully,
        and is set to a negative number for any of the system-defined errors (see section 17.1 for a list of the meanings
        of the possible error codes). The key may also be set to a positive number, in which case that number is a pointer
        to an error string, relative to address $8000. The string is in the usual Qdos form of a word giving the length of
        the string, followed by the characters.
        */

        //No error
        m68k_set_reg(M68K_REG_D0,0);

        //D1= Job ID. TODO. Parece que da error "error in expression" porque no se asigna un job id valido?
        //Parece que D1 vuelve con -1
        m68k_set_reg(M68K_REG_D1,0); //Valor de D1 inventado
        /*

        */

        //Parece que no sirve de mucho, despues de hacer esto se queda en un bucle de:
        //Trap 3. D0=04H A0=00000000H: IO.EDLIN
        //Trap 3. D0=07H A0=00000000H: IO.SSTRG
        //Eso tanto con dir mdv1_ o con lbytes mdv1_archivo,xxxx

        //Hacer saltar el menu como si fuese breakpoint debugger
        //Tener en cuenta que la accion del breakpoint 0 sea nula, sino no se abriria el menu
        catch_breakpoint_index=0;
        menu_breakpoint_exception.v=1;
        menu_abierto=1;
        sprintf (catch_breakpoint_message,"Opened file %s",nombre_archivo);
        printf ("Abrimos menu\n");

        //sleep(5);
      }

    }

    //Interceptar trap 3
    /*
    trap 3 salta a:
    0032A bsr     336
    */
    if (get_pc_register()==0x0032a) {
      core_ql_trap_three();

    }



		if (0==1) { }




		else {
			if (esperando_tiempo_final_t_estados.v==0) {



#ifdef EMULATE_CPU_STATS
                                util_stats_increment_counter(stats_codsinpr,byte_leido_core_ql);
#endif



        /*char buffer_disassemble[1000];
        unsigned int pc_ql=100;

        // Access the internals of the CPU
        pc_ql=m68k_get_reg(NULL, M68K_REG_PC);

        int longitud=m68k_disassemble(buffer_disassemble, pc_ql, M68K_CPU_TYPE_68000);
        printf ("%08XH %d %s\n",pc_ql,longitud,buffer_disassemble);
        */


	//intento de detectar cuando se llama a rutina rom de mt.ipcom
	//unsigned int pc_ql=get_ql_pc();
	//if (pc_ql==0x0872) exit(1);

	//Ejecutar opcode
                // Values to execute determine the interleave rate.
                // Smaller values allow for more accurate interleaving with multiple
                // devices/CPUs but is more processor intensive.
                // 100000 is usually a good value to start at, then work from there.

                // Note that I am not emulating the correct clock speed!
                m68k_execute(1);


		/* Disasemble one instruction at pc and store in str_buff */
//unsigned int m68k_disassemble(char* str_buff, unsigned int pc, unsigned int cpu_type)
//unsigned int m68k_get_reg(void* context, m68k_register_t regnum)
//                case M68K_REG_PC:       return MASK_OUT_ABOVE_32(cpu->pc);
//		unsigned int registropcql=m68k_get_reg(NULL,M68K_REG_PC);
//		printf ("Registro PC: %08XH\n",registropcql);
//		m68k_disassemble(buffer_disassemble,registropcql,M68K_CPU_TYPE_68000);


				//Simplemente incrementamos los t-estados un valor inventado, aunque luego al final parece ser parecido a la realidad
				t_estados+=4;


                        }
                }



		//Esto representa final de scanline

		//normalmente
		if ( (t_estados/screen_testados_linea)>t_scanline  ) {

			t_scanline++;



                        //Envio sonido

                        audio_valor_enviar_sonido=0;

                        audio_valor_enviar_sonido +=da_output_ay();




                        if (realtape_inserted.v && realtape_playing.v) {
                                realtape_get_byte();
                                //audio_valor_enviar_sonido += realtape_last_value;
				if (realtape_loading_sound.v) {
				audio_valor_enviar_sonido /=2;
                                audio_valor_enviar_sonido += realtape_last_value/2;
				}
                        }

                        //Ajustar volumen
                        if (audiovolume!=100) {
                                audio_valor_enviar_sonido=audio_adjust_volume(audio_valor_enviar_sonido);
                        }


			//printf ("sonido: %d\n",audio_valor_enviar_sonido);

                        audio_buffer[audio_buffer_indice]=audio_valor_enviar_sonido;


                        if (audio_buffer_indice<AUDIO_BUFFER_SIZE-1) audio_buffer_indice++;
                        //else printf ("Overflow audio buffer: %d \n",audio_buffer_indice);


                        ay_chip_siguiente_ciclo();

			//se supone que hemos ejecutado todas las instrucciones posibles de toda la pantalla. refrescar pantalla y
			//esperar para ver si se ha generado una interrupcion 1/50

			//Final de frame

			if (t_estados>=screen_testados_total) {

				t_scanline=0;

                                //Parche para maquinas que no generan 312 lineas, porque si enviamos menos sonido se escuchara un click al final
                                //Es necesario que cada frame de pantalla contenga 312 bytes de sonido
				//Igualmente en la rutina de envio_audio se vuelve a comprobar que todo el sonido a enviar
				//este completo; esto es necesario para Z88

                                int linea_estados=t_estados/screen_testados_linea;

                                while (linea_estados<312) {

                                        audio_buffer[audio_buffer_indice]=audio_valor_enviar_sonido;
                                        if (audio_buffer_indice<AUDIO_BUFFER_SIZE-1) audio_buffer_indice++;
                                        linea_estados++;
                                }


				t_estados -=screen_testados_total;

				cpu_loop_refresca_pantalla();


				vofile_send_frame(rainbow_buffer);

				siguiente_frame_pantalla();

        contador_parpadeo--;
            //printf ("Parpadeo: %d estado: %d\n",contador_parpadeo,estado_parpadeo.v);
            if (!contador_parpadeo) {
                    contador_parpadeo=20; //TODO no se si esta es la frecuencia normal de parpadeo
                    estado_parpadeo.v ^=1;
            }


				if (debug_registers) scr_debug_registers();


				if (!interrupcion_timer_generada.v) {
					//Llegado a final de frame pero aun no ha llegado interrupcion de timer. Esperemos...
					esperando_tiempo_final_t_estados.v=1;
				}

				else {
					//Llegado a final de frame y ya ha llegado interrupcion de timer. No esperamos.... Hemos tardado demasiado
					//printf ("demasiado\n");
					esperando_tiempo_final_t_estados.v=0;
				}


			//Prueba generar interrupcion
			//printf ("Generar interrupcion\n");
			 //m68k_set_irq(0); de 0 a 6 parece no tener efecto. poniendo 7 no pasa el test de ram
			/*m68k_set_irq(1);
			m68k_set_irq(2);
			m68k_set_irq(3);
			m68k_set_irq(4);
			m68k_set_irq(5);
			m68k_set_irq(6);*/


/*
* read addresses
pc_intr equ     $18021  bits 4..0 set as pending level 2 interrupts
*/

      //Sirve para algo esto????
			ql_pc_intr |=31;
			m68k_set_irq(2);
			//Esto acaba generando llamadas a leer PC_INTR		Interrupt register


                                //Final de instrucciones ejecutadas en un frame de pantalla
                                if (iff1.v==1) {
                                        interrupcion_maskable_generada.v=1;
                                }



			}



		}

		if (esperando_tiempo_final_t_estados.v) {
			timer_pause_waiting_end_frame();
		}


              //Interrupcion de 1/50s. mapa teclas activas y joystick
                if (interrupcion_fifty_generada.v) {
                        interrupcion_fifty_generada.v=0;

                        //y de momento actualizamos tablas de teclado segun tecla leida
                        //printf ("Actualizamos tablas teclado %d ", temp_veces_actualiza_teclas++);
                       scr_actualiza_tablas_teclado();


                       //lectura de joystick
                       realjoystick_main();

                        //printf ("temp conta fifty: %d\n",tempcontafifty++);
                }


                //Interrupcion de procesador y marca final de frame
                if (interrupcion_timer_generada.v) {
                        interrupcion_timer_generada.v=0;
                        esperando_tiempo_final_t_estados.v=0;
                        interlaced_numero_frame++;
                        //printf ("%d\n",interlaced_numero_frame);
                }



		//Interrupcion de cpu. gestion im0/1/2.
		if (interrupcion_maskable_generada.v || interrupcion_non_maskable_generada.v) {



                        //ver si esta en HALT
                        if (z80_ejecutando_halt.v) {
                                        z80_ejecutando_halt.v=0;
                                        reg_pc++;

                        }

			if (1==1) {

					if (interrupcion_non_maskable_generada.v) {
						debug_anota_retorno_step_nmi();
						interrupcion_non_maskable_generada.v=0;



					}


					if (1==1) {


					//justo despues de EI no debe generar interrupcion
					//e interrupcion nmi tiene prioridad
						if (interrupcion_maskable_generada.v && byte_leido_core_ql!=251) {
						debug_anota_retorno_step_maskable();
						//Tratar interrupciones maskable




					}
				}


			}

  }


}
