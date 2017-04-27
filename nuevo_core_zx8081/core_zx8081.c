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
#include "zx8081.h"
#include "timer.h"
#include "menu.h"
#include "compileoptions.h"
#include "contend.h"
#include "snap_zx8081.h"
#include "utils.h"
#include "realjoystick.h"
#include "chardetect.h"



void init_zx8081_scanline_y(int y)
{

//inicializamos valores, para border o fast mode. TODO. esto es una aproximacion
	if (border_enabled.v==0) y=y-screen_borde_superior;
        if (y>=0 && y<get_total_alto_rainbow() ) {
		//if (if_store_scanline_interlace(y)) {


	                int x;
        	        //en principio zona blanca (border) -> 0
	                z80_byte sprite;
        	        sprite=video_zx8081_ula_video_output;


                	for (x=0;x<get_total_ancho_rainbow();x+=8) screen_store_scanline_char_zx8081_border_scanline(x,y,sprite);
		//}
        }


}


void init_zx8081_scanline_x_longitud(int x,int longitud)
{

int y=t_scanline_draw-LINEAS_SUP_NO_USABLES;
//inicializamos valores, para border o fast mode. TODO. esto es una aproximacion
        if (border_enabled.v==0) return;
        if (y>=0 && y<get_total_alto_rainbow() ) {
                //if (if_store_scanline_interlace(y)) {


                        //en principio zona blanca (border) -> 0
                        z80_byte sprite;
                        sprite=video_zx8081_ula_video_output;

			int final=x+longitud;

                        for (;x<final;x+=8) screen_store_scanline_char_zx8081_border_scanline(x,y,sprite);
                //}
        }


}



void init_zx8081_scanline(void)
{


int y=t_scanline_draw-LINEAS_SUP_NO_USABLES;
//para evitar las lineas superiores
//TODO. cuadrar esto con valores de borde invisible superior

if (y>=0) init_zx8081_scanline_y(y);


}


z80_byte byte_leido_core_zx8081;

int temp_contador_hsync=0;
int temp_ajuste_contador_hsync=0;

int temp_pendiente_init_scanline=0;

int antes_t_estados;

int pendiente_refrescar_pantalla=1;


int se_ha_escrito_en_linea=0;

int dif_testados=0;

int x_anterior=0;

//bucle principal de ejecucion de la cpu de zx80/81
void cpu_core_loop_zx8081(void)
{
  



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



		//Autoload
		//Si hay cinta insertada
		if (  (tape_loadsave_inserted & TAPE_LOAD_INSERTED)!=0 || (realtape_inserted.v==1)

		) {
			//Y si hay que hacer autoload
			if (initial_tap_load.v==1 && initial_tap_sequence==0) {

				//Para zx80
		                if (MACHINE_IS_ZX80 && reg_pc==0x0283) {
        		                debug_printf (VERBOSE_INFO,"Autoload tape");
                		        initial_tap_sequence=1;
		                }


				//Para zx81
                		if (MACHINE_IS_ZX81 && reg_pc==0x0487) {
	                	        debug_printf (VERBOSE_INFO,"Autoload tape");
        	                	initial_tap_sequence=1;
	                	}
			}
		}
		
		
		//Interceptar rutina de carga

			if (new_tap_load_detect_zx8081()) {
				audio_playing.v=0;

				draw_tape_text();

				if (MACHINE_IS_ZX80) new_tape_load_zx80();	
				else new_tape_load_zx81();

	                        //audio_playing.v=1;
        	                timer_reset();

			}

			else	if (new_tap_save_detect_zx8081()) {
        	                	audio_playing.v=0;

					draw_tape_text();

	        	                if (MACHINE_IS_ZX80) new_tape_save_zx80();
        	        	        else new_tape_save_zx81();

                	        	//audio_playing.v=1;
	                        	timer_reset();

        	        }

		else {
			if (esperando_tiempo_final_t_estados.v==0) {




				//incrementar contador hsync
				//int antes_t_estados=t_estados;
				antes_t_estados=t_estados;

				byte_leido_core_zx8081=fetch_opcode();

				contend_read( reg_pc, 4 );

#ifdef EMULATE_CPU_STATS
                                util_stats_increment_counter(stats_codsinpr,byte_leido_core_zx8081);
#endif

				
				
				reg_pc++;

				reg_r_antes_zx8081=reg_r;
				
				reg_r++;



	                	codsinpr[byte_leido_core_zx8081]  () ;

				dif_testados=t_estados-antes_t_estados;
				
				temp_contador_hsync +=dif_testados;


				//Nueva manera de generar hsync
				if (temp_contador_hsync>=207) {
					//printf ("generamos hsync. t_estados=%d\n",t_estados);


					//Si esta pendiente refrescar pantalla
					if (pendiente_refrescar_pantalla) {
						pendiente_refrescar_pantalla=0;
		                                cpu_loop_refresca_pantalla();
                		                //printf ("refresca pantalla con t_scanline_draw %d\n",t_scanline_draw);
                                		vofile_send_frame(rainbow_buffer);
		                                siguiente_frame_pantalla();

					}

					generar_zx8081_horiz_sync();
					//se_ha_escrito_en_linea=0;
					temp_contador_hsync -=207;

					temp_pendiente_init_scanline=1;
					printf ("pendiente init_zx8081_scanline con t_scanline_draw %d\n",t_scanline_draw);

				   	//if (rainbow_enabled.v==1) {
	                                //	init_zx8081_scanline();
		                        //}



				}

				if (iff1.v==1) {	
	
					//solo cuando cambia de 1 a 0
					if ( (reg_r_antes_zx8081 & 64)==64 && (reg_r & 64)==0 ) {

						interrupcion_maskable_generada.v=1;

						 //printf ("reset contador hsync debido a interrupcion con t_estados=%d\n",t_estados);
						temp_contador_hsync=0;
						video_zx8081_caracter_en_linea_actual=0;
						
					}
				}


                        }
                }

		if (rainbow_enabled.v) {

				int ancho=dif_testados*2;
				int x=( (video_zx8081_caracter_en_linea_actual-8) )*8;

				if (!se_ha_escrito_en_linea) {

					if (x>0) init_zx8081_scanline_x_longitud(x,ancho);
				}

				se_ha_escrito_en_linea=0;


			video_zx8081_caracter_en_linea_actual+=(ancho)/8;
		}





		//Esto representa final de scanline

		//normalmente
		if ( (t_estados/screen_testados_linea)>t_scanline  ) {

			t_scanline++;

			//generar_zx8081_horiz_sync();


                        //Envio sonido

                        audio_valor_enviar_sonido=0;

                        audio_valor_enviar_sonido +=da_output_ay();



                        if (zx8081_vsync_sound.v==1) {

	                        if (beeper_real_enabled==0) {
        	                        audio_valor_enviar_sonido += da_amplitud_speaker_zx8081();
                	        }

                        	else {
                                	audio_valor_enviar_sonido += get_value_beeper_sum_array();
	                                beeper_new_line();
        	                }


                        }

			else {
				//Si no hay vsync sound, beeper sound off, forzamos que hay silencio de beepr
				beeper_silence_detection_counter=SILENCE_DETECTION_MAX;
			}


                        if (realtape_inserted.v && realtape_playing.v) {
                                realtape_get_byte();
                                //audio_valor_enviar_sonido += realtape_last_value;
				audio_valor_enviar_sonido /=2;
                                audio_valor_enviar_sonido += realtape_last_value/2;
                        }

                        //Ajustar volumen
                        if (audiovolume!=100) {
                                audio_valor_enviar_sonido=audio_adjust_volume(audio_valor_enviar_sonido);
                        }


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


				pendiente_refrescar_pantalla=1;

				//cpu_loop_refresca_pantalla();
				//printf ("refresca pantalla con t_scanline_draw %d\n",t_scanline_draw);
				//vofile_send_frame(rainbow_buffer);
				//siguiente_frame_pantalla();


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

				//para el detector de vsync sound
				if (zx8081_detect_vsync_sound.v) {
					if (zx8081_detect_vsync_sound_counter==0) {
						//caso normal con vsync
                                                //hay vsync el suficiente tiempo. desactivar sonido
                                                        //printf ("hay vsync el suficiente tiempo. desactivar sonido\n");
                                                        zx8081_vsync_sound.v=0;

					}

					else {
						//no hay vsync. por tanto hay sonido
						//printf ("no hay vsync completo. hay sonido. contador: %d\n",zx8081_detect_vsync_sound_counter);
						zx8081_vsync_sound.v=1;
	
					}
					if (zx8081_detect_vsync_sound_counter<ZX8081_DETECT_VSYNC_SOUND_COUNTER_MAX) zx8081_detect_vsync_sound_counter++;

				}

			
			}


			
			//Inicializar siguiente linea. Esto es importante que este aqui despues de 
			//una posible actualizacion de pantalla, para que no se vea la linea blanca inicializada
			if (rainbow_enabled.v==1) {
                                //temp desactivado init_zx8081_scanline();
                        }
		}		



		if (esperando_tiempo_final_t_estados.v) {
			timer_pause_waiting_end_frame();
		}



		//Interrupcion de 1/50s. Actualizar mapa teclas activas, joystick
                if (interrupcion_timer_generada.v) {

			interlaced_numero_frame++;
                        interrupcion_timer_generada.v=0;
			esperando_tiempo_final_t_estados.v=0;


                        //y de momento actualizamos tablas de teclado segun tecla leida
			//printf ("Actualizamos tablas teclado %d ", temp_veces_actualiza_teclas++);
                       scr_actualiza_tablas_teclado();

                       //lectura de joystick
                       realjoystick_main();


		}

		//Interrupcion de cpu. gestion im0/1/2. Esto se hace al cambio de bit6 de R en zx80/81
		if (interrupcion_maskable_generada.v || interrupcion_non_maskable_generada.v) {

                        //ver si esta en HALT
                        if (z80_ejecutando_halt.v) {
                                        z80_ejecutando_halt.v=0;
                                        reg_pc++;

                        }

			if (1==1) {

					if (interrupcion_non_maskable_generada.v) {
						interrupcion_non_maskable_generada.v=0;


						//NMI wait 14 estados
						t_estados=t_estados + 14+temp_ajuste_contador_hsync;


                                                z80_byte reg_pc_h,reg_pc_l;
                                                reg_pc_h=value_16_to_8h(reg_pc);
                                                reg_pc_l=value_16_to_8l(reg_pc);

						//3 estados	
                                                poke_byte(--reg_sp,reg_pc_h);
						//3 estados
                                                poke_byte(--reg_sp,reg_pc_l);


						reg_r++;
						iff1.v=0;
						//printf ("Calling NMI with pc=0x%x\n",reg_pc);

						//Otros 6 estados
						//t_estados += 6;
						t_estados +=3;

						//Total NMI: NMI WAIT 14 estados + NMI CALL 12 estados
						reg_pc= 0x66;

						//temp

						//t_estados -=15;


						temp_contador_hsync=temp_contador_hsync+14+6+temp_ajuste_contador_hsync;

						temp_contador_hsync +=3;

						
					}

					//else {
					if (1==1) {

						
					//justo despues de EI no debe generar interrupcion
					//e interrupcion nmi tiene prioridad
						if (interrupcion_maskable_generada.v && byte_leido_core_zx8081!=251) {
						//Tratar interrupciones maskable


                                                //INT wait 10 estados. Valor de pruebas
                                                t_estados=t_estados+10-6;

						interrupcion_maskable_generada.v=0;

						z80_byte reg_pc_h,reg_pc_l;
						reg_pc_h=value_16_to_8h(reg_pc);
						reg_pc_l=value_16_to_8l(reg_pc);

						poke_byte(--reg_sp,reg_pc_h);
						poke_byte(--reg_sp,reg_pc_l);
						
						reg_r++;

						//Desactivar interrupciones despues de interrupcion maskable
						iff1.v=0;


						temp_contador_hsync=temp_contador_hsync+10+6-6;

						//IM0/1
						if (im_mode==0 || im_mode==1) {
							//printf ("Calling zx80/81 interrupt address 56 :%d \r",temp_veces_interrupt_zx80++);
                                                        reg_pc=56;
                                                        //oficial: 
							t_estados += 7;

							//t_estados -=6;

							temp_contador_hsync=temp_contador_hsync+7;


						}
						else {
						//IM 2.

							z80_int temp_i;
							z80_byte dir_l,dir_h;	
							temp_i=reg_i*256+255;
							dir_l=peek_byte(temp_i++);
							dir_h=peek_byte(temp_i);
							reg_pc=value_8_to_16(dir_h,dir_l);
							t_estados += 7 ;


							temp_contador_hsync=temp_contador_hsync+6+7;

						}

					}
				}


			}

                }


}


