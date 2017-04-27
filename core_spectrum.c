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
#include "ula.h"
#include "utils.h"
#include "realjoystick.h"
#include "chardetect.h"
#include "diviface.h"
#include "timex.h"
#include "zxuno.h"
#include "prism.h"
#include "snap_rzx.h"

#include "scrstdout.h"

z80_byte byte_leido_core_spectrum;


//int tempcontafifty=0;

int si_siguiente_sonido(void)
{

	if (MACHINE_IS_PRISM) {
/*
Si son 525 scanlines:
525/5*3=315

0/1.6=0 mod 5=0
1/1.6=0 mod 5=1
2/1.6=1
3/1.6=1
4/1.6=2

5/1.6=3
6/1.6=3
7/1,6=4
8/1.6=4
9/1.6=5

10/1.6=6
Hacer mod 5 y escoger 0,2,4 (de cada 5 coger 3)
*/
		int resto=t_scanline%5;
		if (resto==0 || resto==2 || resto==4) return 1;
		else return 0;
	}

	return 1;
}


/*
void t_scanline_next_border(void)
{
        //resetear buffer border

	int i;
        //en principio inicializamos el primer valor con el ultimo out del border
        buffer_border[0]=out_254 & 7;

	//Siguientes valores inicializamos a 255
        for (i=1;i<BORDER_ARRAY_LENGTH;i++) buffer_border[i]=255;


}
*/

void t_scanline_next_fullborder(void)
{
        //resetear buffer border

        int i;

        //a 255
        for (i=0;i<CURRENT_FULLBORDER_ARRAY_LENGTH;i++) fullbuffer_border[i]=255;

	//Resetear buffer border para prism
	if (MACHINE_IS_PRISM) {
		for (i=0;i<CURRENT_PRISM_BORDER_BUFFER;i++) prism_ula2_border_colour_buffer[i]=65535;
	}

	//printf ("max buffer border : %d\n",i);


}




void core_spectrum_store_rainbow_current_atributes(void)
{

	//En maquina prism, no hacer esto
	if (MACHINE_IS_PRISM) return;

				//ULA dibujo de pantalla
				//last_x_atributo guarda ultima posicion (+1) antes de ejecutar el opcode
				//lo que se pretende hacer aqui es guardar los atributos donde esta leyendo la ula actualmente,
				//y desde esta lectura a la siguiente, dado que cada opcode del z80 puede tardar mas de 4 ciclos (en 4 ciclos se generan 8 pixeles)
				//Esto no es exactamente perfecto,
				//lo ideal es que cada vez que avanzase el contador de t_estados se guardase el atributo que se va a leer
				//como esto es muy costoso, hacemos que se guardan los atributos leidos desde el opcode anterior hasta este
				if (rainbow_enabled.v) {
					if (t_scanline_draw>=screen_indice_inicio_pant && t_scanline_draw<screen_indice_fin_pant) {
						int atributo_pos=(t_estados % screen_testados_linea)/4;
							z80_byte *screen_atributo=get_base_mem_pantalla_attributes();
							z80_byte *screen_pixel=get_base_mem_pantalla();

							//printf ("atributos: %p pixeles: %p\n",screen_atributo,screen_pixel);

							int dir_atributo;

							if (timex_si_modo_8x1()) {
								dir_atributo=screen_addr_table[(t_scanline_draw-screen_indice_inicio_pant)*32];
							}

							else {
								dir_atributo=(t_scanline_draw-screen_indice_inicio_pant)/8;
								dir_atributo *=32;
							}


                                                        int dir_pixel=screen_addr_table[(t_scanline_draw-screen_indice_inicio_pant)*32];

						//si hay cambio de linea, empezamos con el 0
						//puede parecer que al cambiar de linea nos perdemos el ultimo atributo de pantalla hasta el primero de la linea
						//siguiente, pero eso es imposible, dado que eso sucede desde el ciclo 128 hasta el 228 (zona de borde)
						// y no hay ningun opcode que tarde 100 ciclos
						if (last_x_atributo>atributo_pos) last_x_atributo=0;
							dir_atributo += last_x_atributo;
							dir_pixel +=last_x_atributo;

						//Puntero al buffer de atributos
						z80_byte *puntero_buffer;
						puntero_buffer=&scanline_buffer[last_x_atributo*2];

						for (;last_x_atributo<=atributo_pos;last_x_atributo++) {
							//printf ("last_x_atributo: %d atributo_pos: %d\n",last_x_atributo,atributo_pos);
							last_ula_pixel=screen_pixel[dir_pixel];
							last_ula_attribute=screen_atributo[dir_atributo];

							//realmente en este array guardamos tambien atributo cuando estamos en la zona de border,
							//nos da igual, lo hacemos por no complicarlo
							//debemos tambien suponer que atributo_pos nunca sera mayor o igual que ATRIBUTOS_ARRAY_LENGTH
							//esto debe ser asi dado que atributo_pos=(t_estados % screen_testados_linea)/4;

							*puntero_buffer++=last_ula_pixel;
							*puntero_buffer++=last_ula_attribute;
							dir_atributo++;
							dir_pixel++;
						}

						//Siguiente posicion se queda en last_x_atributo

						//printf ("fin lectura attr linea %d\n",t_scanline_draw);
					}

					//Para el bus idle le decimos que estamos en zona de border superior o inferior y por tanto retornamos 255
					else {
						last_ula_attribute=255;
						last_ula_pixel=255;
					}
				}
}

//bucle principal de ejecucion de la cpu de spectrum
void cpu_core_loop_spectrum(void)
{

		debug_get_t_stados_parcial_post();
		debug_get_t_stados_parcial_pre();

		timer_check_interrupt();


//#ifdef COMPILE_STDOUT
//		if (screen_stdout_driver) scr_stdout_printchar();
//#endif
//
//#ifdef COMPILE_SIMPLETEXT
//                if (screen_simpletext_driver) scr_simpletext_printchar();
//#endif
		if (chardetect_detect_char_enabled.v) chardetect_detect_char();
		if (chardetect_printchar_enabled.v) chardetect_printchar();


		//Gestionar autoload
		gestionar_autoload_spectrum();


		if (tap_load_detect()) {
        	                //si estamos en pausa, no hacer nada
                	        if (!tape_pause) {
					audio_playing.v=0;

					draw_tape_text();

					tap_load();
					all_interlace_scr_refresca_pantalla();

					//printf ("refresco pantalla\n");
					//audio_playing.v=1;
					timer_reset();
				}

				else {
					core_spectrum_store_rainbow_current_atributes();
					//generamos nada. como si fuera un NOP

					contend_read( reg_pc, 4 );


				}
		}

		else  if (tap_save_detect()) {
	               	        audio_playing.v=0;

				draw_tape_text();

                        	tap_save();
	                        //audio_playing.v=1;
				timer_reset();
               	}


		else {
			if (esperando_tiempo_final_t_estados.v==0) {

				core_spectrum_store_rainbow_current_atributes();



#ifdef DEBUG_SECOND_TRAP_STDOUT

        //Para poder debugar rutina que imprima texto. Util para aventuras conversacionales
        //hay que definir este DEBUG_SECOND_TRAP_STDOUT manualmente en compileoptions.h despues de ejecutar el configure

	scr_stdout_debug_print_char_routine();

#endif


				//Modo normal
				if (diviface_enabled.v==0) {

        	                        contend_read( reg_pc, 4 );
					byte_leido_core_spectrum=fetch_opcode();

				}


				//Modo con diviface activado
				else {
					diviface_pre_opcode_fetch();
					contend_read( reg_pc, 4 );
					byte_leido_core_spectrum=fetch_opcode();
					diviface_post_opcode_fetch();
				}


#ifdef EMULATE_CPU_STATS
				util_stats_increment_counter(stats_codsinpr,byte_leido_core_spectrum);
#endif

                                reg_pc++;

				reg_r++;

				rzx_in_fetch_counter_til_next_int_counter++;



	                	codsinpr[byte_leido_core_spectrum]  () ;

				//printf ("t_estados:%d\n",t_estados);

				/*if (rzx_reproduciendo && rzx_in_fetch_counter_til_next_int) {
					if (rzx_in_fetch_counter_til_next_int_counter>=rzx_in_fetch_counter_til_next_int) {
						//Forzar final de frame
						//t_estados=screen_testados_total;
						printf ("Forzar final de frame\n");
						rzx_next_frame_recording();
					}
				}*/


				//Soporte interrupciones raster zxuno
				if (MACHINE_IS_ZXUNO) zxuno_handle_raster_interrupts();




                        }
                }



		//ejecutar esto al final de cada una de las scanlines (312)
		//esto implica que al final del frame de pantalla habremos enviado 312 bytes de sonido


		//A final de cada scanline
		if ( (t_estados/screen_testados_linea)>t_scanline  ) {
			//printf ("%d\n",t_estados);
			//if (t_estados>69000) printf ("t_scanline casi final: %d\n",t_scanline);

			if (si_siguiente_sonido() ) {

				audio_valor_enviar_sonido=0;

				audio_valor_enviar_sonido +=da_output_ay();


				if (beeper_enabled.v) {
					if (beeper_real_enabled==0) {
						audio_valor_enviar_sonido += value_beeper;
					}

					else {
						audio_valor_enviar_sonido += get_value_beeper_sum_array();
						beeper_new_line();
					}
				}

				if (specdrum_enabled.v) {
					specdrum_mix();
				}


				if (realtape_inserted.v && realtape_playing.v) {
					realtape_get_byte();
					if (realtape_loading_sound.v) {
                        	        audio_valor_enviar_sonido /=2;
                                	audio_valor_enviar_sonido += realtape_last_value/2;
					}
				}

				//Ajustar volumen
				if (audiovolume!=100) {
					audio_valor_enviar_sonido=audio_adjust_volume(audio_valor_enviar_sonido);
				}

				audio_buffer[audio_buffer_indice]=audio_valor_enviar_sonido;


				//temporal
				//printf ("%02X ",audio_valor_enviar_sonido);


				if (audio_buffer_indice<AUDIO_BUFFER_SIZE-1) audio_buffer_indice++;
				//else printf ("Overflow audio buffer: %d \n",audio_buffer_indice);


				ay_chip_siguiente_ciclo();

			}



			//final de linea


			//copiamos contenido linea y border a buffer rainbow
			if (rainbow_enabled.v==1) {
				screen_store_scanline_rainbow_solo_border();
				screen_store_scanline_rainbow_solo_display();

				//t_scanline_next_border();

			}

			t_scanline_next_line();

			//se supone que hemos ejecutado todas las instrucciones posibles de toda la pantalla. refrescar pantalla y
			//esperar para ver si se ha generado una interrupcion 1/50

                        if (t_estados>=screen_testados_total) {

				if (rainbow_enabled.v==1) t_scanline_next_fullborder();

		                t_scanline=0;

		                //printf ("final scan lines. total: %d\n",screen_scanlines);
		                if (MACHINE_IS_INVES) {
		                        //Inves
		                        t_scanline_draw=screen_indice_inicio_pant;
		                        //printf ("reset inves a inicio pant : %d\n",t_scanline_draw);
		                }

		                else {
                		        //printf ("reset no inves\n");
					set_t_scanline_draw_zero();

		                }


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

				//Para paperboy, thelosttapesofalbion0 y otros que hacen letras en el border, para que no se desplacen en diagonal
				//t_estados=0;
				//->paperboy queda fijo. thelosttapesofalbion0 no se desplaza, sino que tiembla si no forzamos esto


				//Final de instrucciones ejecutadas en un frame de pantalla
				if (iff1.v==1) {
					interrupcion_maskable_generada.v=1;

					//En Timex, ver bit 6 de puerto FF
					if ( MACHINE_IS_TIMEX_TS2068 && ( timex_port_ff & 64) ) interrupcion_maskable_generada.v=0;

					//En ZXuno, ver bit disvint
                                	if (MACHINE_IS_ZXUNO) {

	                                        if (zxuno_ports[0x0d] & 4) {
        	                                        //interrupciones normales deshabilitadas
                	                                //printf ("interrupciones normales deshabilitadas\n");
							//Pero siempre que no se haya disparado una maskable generada por raster

							if (zxuno_disparada_raster.v==0) interrupcion_maskable_generada.v=0;
                        	                }
                                	}


				}


				cpu_loop_refresca_pantalla();

				vofile_send_frame(rainbow_buffer);


				siguiente_frame_pantalla();


				if (debug_registers) scr_debug_registers();

	  	                contador_parpadeo--;
                        	//printf ("Parpadeo: %d estado: %d\n",contador_parpadeo,estado_parpadeo.v);
	                        if (!contador_parpadeo) {
        	                        contador_parpadeo=16;
                	                estado_parpadeo.v ^=1;
	                        }


				if (!interrupcion_timer_generada.v) {
					//Llegado a final de frame pero aun no ha llegado interrupcion de timer. Esperemos...
					//printf ("no demasiado\n");
					esperando_tiempo_final_t_estados.v=1;
				}

				else {
					//Llegado a final de frame y ya ha llegado interrupcion de timer. No esperamos.... Hemos tardado demasiado
					//printf ("demasiado\n");
					esperando_tiempo_final_t_estados.v=0;
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
			//printf ("Generada interrupcion timer\n");
                        interrupcion_timer_generada.v=0;
                        esperando_tiempo_final_t_estados.v=0;
			interlaced_numero_frame++;
			//printf ("%d\n",interlaced_numero_frame);
                }


		//Interrupcion de cpu. gestion im0/1/2. Esto se hace al final de cada frame en spectrum o al cambio de bit6 de R en zx80/81
		if (interrupcion_maskable_generada.v || interrupcion_non_maskable_generada.v) {

			//printf ("Generada interrupcion Z80\n");

			//if (interrupcion_non_maskable_generada.v) printf ("generada nmi\n");

                        //if (interrupts.v==1) {   //esto ya no se mira. si se ha producido interrupcion es porque estaba en ei o es una NMI
                        //ver si esta en HALT
                        if (z80_ejecutando_halt.v) {
                                        z80_ejecutando_halt.v=0;
                                        reg_pc++;
                        }

			if (1==1) {

					if (interrupcion_non_maskable_generada.v) {
						debug_anota_retorno_step_nmi();
						//printf ("generada nmi\n");
                                                interrupcion_non_maskable_generada.v=0;


                                                //NMI wait 14 estados
                                                t_estados += 14;


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
                                                t_estados += 6;

                                                //Total NMI: NMI WAIT 14 estados + NMI CALL 12 estados
                                                reg_pc= 0x66;

                                                //temp

                                                t_estados -=15;

						//Prueba
						//Al recibir nmi tiene que poner paginacion normal. Luego ya saltara por autotrap de diviface
						if (diviface_enabled.v) {
							//diviface_paginacion_manual_activa.v=0;
							diviface_control_register&=(255-128);
							diviface_paginacion_automatica_activa.v=0;
						}



					}

					if (1==1) {
					//else {


					//justo despues de EI no debe generar interrupcion
					//e interrupcion nmi tiene prioridad
						if (interrupcion_maskable_generada.v && byte_leido_core_spectrum!=251) {
						debug_anota_retorno_step_maskable();
						//Tratar interrupciones maskable
						interrupcion_maskable_generada.v=0;

						//Aunque parece que rzx deberia saltar aqui al siguiente frame, lo hacemos solo cuando es necesario (cuando las lecturas en un frame exceden el frame)
						//if (rzx_reproduciendo) {
							//rzx_next_frame_recording();
						//}

						z80_byte reg_pc_h,reg_pc_l;
                                                reg_pc_h=value_16_to_8h(reg_pc);
                                                reg_pc_l=value_16_to_8l(reg_pc);

                                                poke_byte(--reg_sp,reg_pc_h);
                                                poke_byte(--reg_sp,reg_pc_l);

						reg_r++;

						//Caso Inves. Hacer poke (I*256+R) con 255
						if (MACHINE_IS_INVES) {
							//z80_byte reg_r_total=(reg_r&127) | (reg_r_bit7 &128);

							//Se usan solo los 7 bits bajos del registro R
							z80_byte reg_r_total=(reg_r&127);

							z80_int dir=reg_i*256+reg_r_total;

							poke_byte_no_time(dir,255);
						}


						//desactivar interrupciones al generar una
						iff1.v=0;
						//Modelos spectrum

						if (im_mode==0 || im_mode==1) {
							reg_pc=56;
							t_estados += 7;
						}
						else {
						//IM 2.

						        z80_int temp_i;
                                                        z80_byte dir_l,dir_h;
                                                        temp_i=reg_i*256+255;
                                                        dir_l=peek_byte(temp_i++);
                                                        dir_h=peek_byte(temp_i);
                                                        reg_pc=value_8_to_16(dir_h,dir_l);
                                                        t_estados += 7;


							//Para mejorar demos ula128 y scroll2017
							//Pero esto hace empeorar la demo ulatest3.tap
							if (ula_im2_slow.v) t_estados++;
						}

					}
				}


			}

                }

}
