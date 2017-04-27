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

#include "cpu.h"
#include "operaciones.h"
#include "debug.h"
#include "audio.h"
#include "tape.h"
#include "ay38912.h"
#include "mem128.h"
#include "zx8081.h"
#include "menu.h"
#include "screen.h"
#include "compileoptions.h"
#include "contend.h"
#include "joystick.h"
#include "ula.h"
#include "utils.h"
#include "printers.h"
#include "disassemble.h"
#include "scrstdout.h"
#include "ulaplus.h"
#include "zxuno.h"
#include "chardetect.h"
#include "mmc.h"
#include "divmmc.h"
#include "zxpand.h"
#include "spectra.h"
#include "spritechip.h"
#include "jupiterace.h"
#include "chloe.h"
#include "timex.h"


void (*poke_byte)(z80_int dir,z80_byte valor);
void (*poke_byte_no_time)(z80_int dir,z80_byte valor);
z80_byte (*peek_byte)(z80_int dir);
z80_byte (*peek_byte_no_time)(z80_int dir);
z80_byte (*lee_puerto)(z80_byte puerto_h,z80_byte puerto_l);
void (*out_port)(z80_int puerto,z80_byte value);
z80_byte (*fetch_opcode)(void);

z80_byte lee_puerto_teclado(z80_byte puerto_h);
z80_byte lee_puerto_spectrum_no_time(z80_byte puerto_h,z80_byte puerto_l);


//Tablas para instrucciones adc, sbc y otras
z80_byte overflow_add_table[] = { 0, 0, 0, FLAG_PV, FLAG_PV, 0, 0, 0 };

z80_byte halfcarry_add_table[] ={ 0, FLAG_H, FLAG_H, FLAG_H, 0, 0, 0, FLAG_H };
z80_byte halfcarry_sub_table[] = { 0, 0, FLAG_H, 0, FLAG_H, 0, FLAG_H, FLAG_H };

z80_byte overflow_sub_table[] = { 0, FLAG_PV, 0, 0, 0, 0, FLAG_PV, 0 };

z80_byte parity_table[256];
z80_byte sz53_table[256];
z80_byte sz53p_table[256];



#ifdef EMULATE_VISUALMEM

//A 1 indica memoria modificada
//A 0 se establece desde opcion de menu
char visualmem_buffer[65536];



void set_visualmembuffer(z80_int dir)
{
	visualmem_buffer[dir]=1;
}

void clear_visualmembuffer(z80_int dir)
{
        visualmem_buffer[dir]=0;
}



#endif


//La mayoria de veces se llama aqui desde set_flags_carry_, dado que casi todas las operaciones que tocan el carry tocan el halfcarry
//hay algunas operaciones, como inc8, que tocan el halfcarry pero no el carry
void set_flags_halfcarry_suma(z80_byte antes,z80_byte result)
{
        antes=antes & 0xF;
        result=result & 0xF;

        if (result<antes) Z80_FLAGS |=FLAG_H;
        else Z80_FLAGS &=(255-FLAG_H);
}

//La mayoria de veces se llama aqui desde set_flags_carry_, dado que casi todas las operaciones que tocan el carry tocan el halfcarry
//hay algunas operaciones, como inc8, que tocan el halfcarry pero no el carry
void set_flags_halfcarry_resta(z80_byte antes,z80_byte result)
{
        antes=antes & 0xF;
        result=result & 0xF;

        if (result>antes) Z80_FLAGS |=FLAG_H;
        else Z80_FLAGS &=(255-FLAG_H);
}


//Well as stated in chapter 3 if the result of an operation in two's complement produces a result that's signed incorrectly then there's an overflow
//o So overflow flag = carry-out flag XOR carry from bit 6 into bit 7.
/*
void set_flags_overflow_inc(z80_byte antes,z80_byte result)
{

	if (result==128) Z80_FLAGS |=FLAG_PV;
	else Z80_FLAGS &=(255-FLAG_PV);

}
*/

/*
void set_flags_overflow_dec(z80_byte antes,z80_byte result)
//Well as stated in chapter 3 if the result of an operation in two's complement produces a result that's signed incorrectly then there's an overflow
//o So overflow flag = carry-out flag XOR carry from bit 6 into bit 7.
{
	if (result==127) Z80_FLAGS |=FLAG_PV;
	else Z80_FLAGS &=(255-FLAG_PV);
}
*/

void set_flags_overflow_suma(z80_byte antes,z80_byte result)

//Siempre llamar a esta funcion despues de haber actualizado el Flag de Carry, pues lo utiliza

//Well as stated in chapter 3 if the result of an operation in two's complement produces a result that's signed incorrectly then there's an overflow
//o So overflow flag = carry-out flag XOR carry from bit 6 into bit 7.
{
	//127+127=254 ->overflow    01111111 01111111 = 11111110    NC   67=y    xor=1
	//-100-100=-200 -> overflow 10011100 10011100 = 00111000     C    67=n   xor=1

	//-2+127=125 -> no overflow 11111110 01111111 = 11111101     C    67=y   xor=0
	//127-2=125 -> no overlow   01111111 11111110 = 11111101     C    67=y   xor=0

	//10-100=-90 -> no overflow 00001010 10011100 = 10100110    NC    67=n   xor=0

	z80_byte overflow67;

        if ( (result & 127) < (antes & 127) ) overflow67=FLAG_C;
        else overflow67=0;

	if ( (Z80_FLAGS & FLAG_C ) ^ overflow67) Z80_FLAGS |=FLAG_PV;
	else Z80_FLAGS &=(255-FLAG_PV);

}

void set_flags_overflow_resta(z80_byte antes,z80_byte result)

//Siempre llamar a esta funcion despues de haber actualizado el Flag de Carry, pues lo utiliza

//Well as stated in chapter 3 if the result of an operation in two's complement produces a result that's signed incorrectly then there's an overflow
//o So overflow flag = carry-out flag XOR carry from bit 6 into bit 7.
{
        //127+127=254 ->overflow    01111111 01111111 = 11111110    NC   67=y    xor=1
        //-100-100=-200 -> overflow 10011100 10011100 = 100111000    C    67=n   xor=1

        //-2+127=125 -> no overflow 11111110 01111111 = 11111101     C    67=y   xor=0
        //127-2=125 -> no overlow   01111111 11111110 = 11111101     C    67=y   xor=0

        //10-100=-90 -> no overflow 00001010 10011100 = 10100110    NC    67=n   xor=0

        z80_byte overflow67;


        if ( (result & 127) > (antes & 127) ) overflow67=FLAG_C;
        else overflow67=0;

	if ( (Z80_FLAGS & FLAG_C ) ^ overflow67) Z80_FLAGS |=FLAG_PV;
	else Z80_FLAGS &=(255-FLAG_PV);

}


void set_flags_overflow_suma_16(z80_int antes,z80_int result)

//Siempre llamar a esta funcion despues de haber actualizado el Flag de Carry, pues lo utiliza

//Well as stated in chapter 3 if the result of an operation in two's complement produces a result that's signed incorrectly then there's an overflow
//o So overflow flag = carry-out flag XOR carry from bit 6 into bit 7.
{
        //127+127=254 ->overflow    01111111 01111111 = 11111110    NC   67=y    xor=1
        //-100-100=-200 -> overflow 10011100 10011100 = 00111000    C    67=n   xor=1

        //-2+127=125 -> no overflow 11111110 01111111 = 11111101     C    67=y   xor=0
        //127-2=125 -> no overlow   01111111 11111110 = 11111101     C    67=y   xor=0

        //10-100=-90 -> no overflow 00001010 10011100 = 10100110    NC    67=n   xor=0

        z80_byte overflow67;


        if ( (result & 32767) < (antes & 32767) ) overflow67=FLAG_C;
        else overflow67=0;

	if ( (Z80_FLAGS & FLAG_C ) ^ overflow67) Z80_FLAGS |=FLAG_PV;
	else Z80_FLAGS &=(255-FLAG_PV);


}

void set_flags_overflow_resta_16(z80_int antes,z80_int result)

//Siempre llamar a esta funcion despues de haber actualizado el Flag de Carry, pues lo utiliza

//Well as stated in chapter 3 if the result of an operation in two's complement produces a result that's signed incorrectly then there's an overflow
//o So overflow flag = carry-out flag XOR carry from bit 6 into bit 7.
{
        //127+127=254 ->overflow    01111111 01111111 = 11111110    NC   67=y    xor=1
        //-100-100=-200 -> overflow 10011100 10011100 = 00111000     C    67=n   xor=1

        //-2+127=125 -> no overflow 11111110 01111111 = 11111101     C    67=y   xor=0
        //127-2=125 -> no overlow   01111111 11111110 = 11111101     C    67=y   xor=0

        //10-100=-90 -> no overflow 00001010 10011100 = 10100110    NC    67=n   xor=0

        z80_byte overflow67;


        if ( (result & 32767) > (antes & 32767) ) overflow67=FLAG_C;
        else overflow67=0;

	if ( (Z80_FLAGS & FLAG_C ) ^ overflow67) Z80_FLAGS |=FLAG_PV;
	else Z80_FLAGS &=(255-FLAG_PV);

}



//activa flags segun instruccion in r,(c) 
void set_flags_in_reg(z80_byte value)
{
	Z80_FLAGS=( Z80_FLAGS & FLAG_C) | sz53p_table[value]; 
}

void set_flags_parity(z80_byte value)
{
	Z80_FLAGS=(Z80_FLAGS & (255-FLAG_PV) ) | parity_table[value];
}



//Parity set if even number of bits set
//Paridad si numero par de bits
z80_byte get_flags_parity(z80_byte value) 
{ 
	z80_byte result;

        result=FLAG_PV; 
        z80_byte mascara=1; 
        z80_byte bit; 
 
        for (bit=0;bit<8;bit++) { 
                if ( (value) & mascara) result = result ^ FLAG_PV; 
                mascara = mascara << 1; 
        } 

	return result;
} 



//Inicializar algunas tablas para acelerar cpu
void init_cpu_tables(void)
{
	z80_byte value=0;

	int contador=0;

	debug_printf (VERBOSE_INFO,"Initializing cpu flags tables");

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0) 


	//Tabla paridad, sz53
	for (contador=0;contador<256;contador++,value++) {
		parity_table[value]=get_flags_parity(value);
		debug_printf (VERBOSE_DEBUG,"Parity table: value: %3d (" BYTETOBINARYPATTERN ") parity: %d",value,BYTETOBINARY(value),parity_table[value]);


		sz53_table[value]=value & ( FLAG_3|FLAG_5|FLAG_S );

		if (value==0) sz53_table[value] |=FLAG_Z;

		debug_printf (VERBOSE_DEBUG,"SZ53 table: value: %3d (" BYTETOBINARYPATTERN ") flags: (" BYTETOBINARYPATTERN ") ",value,BYTETOBINARY(value),BYTETOBINARY(sz53_table[value])) ;


		sz53p_table[value]=sz53_table[value] | parity_table[value];
		debug_printf (VERBOSE_DEBUG,"SZ53P table: value: %3d (" BYTETOBINARYPATTERN ") flags: (" BYTETOBINARYPATTERN ") ",value,BYTETOBINARY(value),BYTETOBINARY(sz53p_table[value])) ;

	}
}		

void neg(void)
{
/*
        z80_byte result,antes;

        antes=0;

        result=antes-reg_a;
        reg_a=result;

        set_flags_carry_resta(antes,result);
        set_flags_overflow_resta(antes,result);
        Z80_FLAGS=(Z80_FLAGS & (255-FLAG_3-FLAG_5-FLAG_Z-FLAG_S)) | FLAG_N | sz53_table[result];

*/

        z80_byte tempneg=reg_a;
        reg_a=0;
	sub_a_reg(tempneg);
}

void poke_byte_no_time_spectrum_48k(z80_int dir,z80_byte valor)
{
        if (dir>16383) {
		memoria_spectrum[dir]=valor;

#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
	}
}

void poke_byte_spectrum_48k(z80_int dir,z80_byte valor)
{
#ifdef EMULATE_CONTEND
        if ( (dir&49152)==16384) {
                t_estados += contend_table[ t_estados ];
        }
#endif

        //Y sumamos estados normales
        t_estados += 3;


        if (dir>16383) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
		memoria_spectrum[dir]=valor;


	}
}


void poke_byte_spectrum_48k_chardetect_automatic_detect_char(z80_int dir,z80_byte valor)
{
	//poke_byte_spectrum_48k(dir,valor);
	//printf ("Original: %d\n",original_char_detect_poke_byte);
	original_char_detect_poke_byte(dir,valor);

        //Para hacer debug de aventuras de texto, ver desde donde se escribe en pantalla
        chardetect_debug_char_table_routines_poke(dir);

}


void poke_byte_no_time_spectrum_128k(z80_int dir,z80_byte valor)
{
int segmento;
z80_byte *puntero;

                segmento=dir / 16384;

        if (dir>16383) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
                dir = dir & 16383;
                puntero=memory_paged[segmento]+dir;

//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
                *puntero=valor;
        }
}


void poke_byte_spectrum_128k(z80_int dir,z80_byte valor)
{
int segmento;
z80_byte *puntero;

		segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
			//printf ("t_estados: %d\n",t_estados);
                        t_estados += contend_table[ t_estados ];
                }       
#endif          
                t_estados += 3;



        if (dir>16383) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
		dir = dir & 16383;
		puntero=memory_paged[segmento]+dir;

//		printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
		*puntero=valor;
	}
}

void poke_byte_no_time_spectrum_128kp2a(z80_int dir,z80_byte valor)
{
int segmento;
z80_byte *puntero;
                segmento=dir / 16384;

        if (dir>16383) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
                dir = dir & 16383;
                puntero=memory_paged[segmento]+dir;

//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
                *puntero=valor;
        }

        else {
                //memoria normalmente ROM. Miramos a ver si esta page RAM in ROM
                if ((puerto_8189&1)==1) {
                        //printf ("Poke en direccion normalmente ROM pero hay RAM. Dir=%d Valor=%d\n",dir,valor);
                        //segmento=0;
                        //dir = dir & 16383;
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
                        puntero=memory_paged[0]+dir;
                        *puntero=valor;
                }
        }

}


void poke_byte_spectrum_128kp2a(z80_int dir,z80_byte valor)
{
int segmento;
z80_byte *puntero;
                segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
                        t_estados += contend_table[ t_estados ];
                }       
#endif          
                t_estados += 3;


        if (dir>16383) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
                dir = dir & 16383;
                puntero=memory_paged[segmento]+dir;

//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
                *puntero=valor;
        }

	else {
		//memoria normalmente ROM. Miramos a ver si esta page RAM in ROM
		if ((puerto_8189&1)==1) {
			//printf ("Poke en direccion normalmente ROM pero hay RAM. Dir=%d Valor=%d\n",dir,valor);
                	//segmento=0;
	                //dir = dir & 16383;
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
	                puntero=memory_paged[0]+dir;
	                *puntero=valor;
		}
	}

}


//Poke en Inves afecta a los 64 kb de RAM
void poke_byte_no_time_spectrum_inves(z80_int dir,z80_byte valor)
{

#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
	memoria_spectrum[dir]=valor;
}


void poke_byte_spectrum_inves(z80_int dir,z80_byte valor)
{

#ifdef EMULATE_CONTEND
        if ( (dir&49152)==16384) {
                t_estados += contend_table[ t_estados ]; 
        }
#endif

        //Y sumamos estados normales
        t_estados += 3;

	poke_byte_no_time_spectrum_inves(dir,valor);

}

void poke_byte_no_time_spectrum_16k(z80_int dir,z80_byte valor)
{

        if (dir>16383 && dir<32768) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
		memoria_spectrum[dir]=valor;
	}
}



void poke_byte_spectrum_16k(z80_int dir,z80_byte valor)
{

#ifdef EMULATE_CONTEND
        if ( (dir&49152)==16384) {
                t_estados += contend_table[ t_estados ]; 
        }
#endif

        //Y sumamos estados normales
        t_estados += 3;



        if (dir>16383 && dir<32768) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
		memoria_spectrum[dir]=valor;
	}
}



//Quicksilva QS Sound board
void out_port_chip_ay_zx8081(z80_int puerto,z80_byte valor)
{
      if ( puerto==32766 || puerto==32767 ) {
                //printf("out Puerto sonido chip AY puerto=%d valor=%d\n",puerto,value);
                if (puerto==32767) puerto=65533;
                else if (puerto==32766) puerto=49149;
                //printf("out Puerto cambiado sonido chip AY puerto=%d valor=%d\n",puerto,value);

		//no autoactivamos chip AY, pues este metodo aqui se activa cuando se lee direccion 32766 o 32767 y esto pasa siempre
		//que se inicializa el ZX81
		//activa_ay_chip_si_conviene();
                if (ay_chip_present.v==1) out_port_ay(puerto,valor);
        }
}


void poke_byte_zx80_no_time(z80_int dir,z80_byte valor)
{

	//if (dir==49296) printf ("poke dir=%d valor=%d\n",dir,valor);

	//Modulo RAM en 49152
	if (ram_in_49152.v==1 && dir>49151) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
		memoria_spectrum[dir]=valor;
                return;
        }

	//Modulo RAM en 32768
	if (ram_in_32768.v==1 && dir>32767) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
		memoria_spectrum[dir]=valor;
                return;
        }



        //Modulo RAM en 2000H
        if (ram_in_8192.v==1 && dir>8191 && dir<16384) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
                //printf ("poke en rom 8192-16383 %d %d\n",dir,valor);
                memoria_spectrum[dir]=valor;
                return;
        }


        //chip ay
        if (dir==32766 || dir==32767) {
                //printf ("poke Chip ay dir=%d valor=%d\n",dir,valor);
                out_port_chip_ay_zx8081(dir,valor);
        }
        if (dir>ramtop_zx8081) dir = (dir&(ramtop_zx8081));

        if (dir>16383 && dir<=ramtop_zx8081) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
		memoria_spectrum[dir]=valor;
	}



}


void poke_byte_zx80(z80_int dir,z80_byte valor)
{

	t_estados +=3;
	poke_byte_zx80_no_time(dir,valor);

}


/*
Gestionar direcciones shadow de RAM desde 2000H hasta 3FFFH

2000H-23FFH shadow de 2400H                 
2400H-27FFH 1K Ram

2800H-2BFFH Shadow de 2C00H
2C00H-2FFFH 1K Ram 

3000H-33FFH Shadow de 3C00H
3400H-37FFH Shadow de 3C00H
3800H-3BFFH Shadow de 3C00H
3C00H-3FFFH 1K Ram

Y tambien validar que no se vaya mas alla de la ramtop

*/

z80_int jupiterace_adjust_memory_pointer(z80_int dir)
{
	if (dir>=0x2000 && dir<=0x23ff) {
		dir |=0x0400;
	}

	else if (dir>=0x2800 && dir<=0x2bff) {
		dir |=0x0400;
	}

	else if (dir>=0x3000 && dir<=0x3bff) {
		dir |=0x0C00;
	}


	//Casos ramtop:
	//3  KB. ramtop=16383=3FFH=00111111 11111111
	//19 KB. ramtop=32767=7FFH=01111111 11111111
	//35 KB. ramtop=49151=BFFH=10111111 11111111

	//Si hay 35 KB y se accede a direccion, por ejemplo, 65535, se convierte en: FFFF AND BFFH = BFFH
	//Si se accede a 49152 es: C000H AND BFFH = 8000H = 32768

        else {
		if (dir>ramtop_ace) dir = (dir&(ramtop_ace));
	}

	return dir;
}


void poke_byte_ace_no_time(z80_int dir,z80_byte valor)
{


	dir=jupiterace_adjust_memory_pointer(dir);


        if (dir>8191 && dir<=ramtop_ace) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif

                memoria_spectrum[dir]=valor;
        }


}


void poke_byte_ace(z80_int dir,z80_byte valor)
{

        t_estados +=3;
        poke_byte_ace_no_time(dir,valor);

}


z80_byte peek_byte_zx80_no_time(z80_int dir)
{
	//Modulo RAM en 49152
	if (ram_in_49152.v==1 && dir>49151) {
		return memoria_spectrum[dir];
	}

	//Modulo RAM en 32768
	if (ram_in_32768.v==1 && dir>32767) {
                return memoria_spectrum[dir];
        }


        if (dir>ramtop_zx8081) {
                dir = (dir&(ramtop_zx8081));
        }

	//Si ZXpand
	if (zxpand_enabled.v) {
		if (dir<8192) {
			if (zxpand_overlay_rom.v) return zxpand_memory_pointer[dir];
		}
	
		return memoria_spectrum[dir];
	}
        else return memoria_spectrum[dir];

	//return memoria_spectrum[dir];
}



z80_byte peek_byte_zx80(z80_int dir)
{

	t_estados +=3;
	
	return peek_byte_zx80_no_time(dir);

}       

z80_byte peek_byte_ace_no_time(z80_int dir)
{
	dir=jupiterace_adjust_memory_pointer(dir);
        return memoria_spectrum[dir];

}


z80_byte peek_byte_ace(z80_int dir)
{

        t_estados +=3;

        return peek_byte_ace_no_time(dir);

}


z80_byte fetch_opcode_spectrum(void)
{
	return peek_byte_no_time (reg_pc);
}

z80_byte fetch_opcode_ace(void)
{
        return peek_byte_no_time (reg_pc);
}



z80_byte fetch_opcode_zx81(void)
{

	z80_byte op;

        op=peek_byte_zx80_no_time(reg_pc&0x7fff);

	if( (reg_pc&0x8000) ) {


		//se esta ejecutando la zona de pantalla

                z80_byte caracter;
		if (!(op&64)) {
			caracter=op;
			op=0;
		}

		else {
			caracter=0;
			//Otros caracteres no validos (y el HALT) generan video display a 0
		
			return op;

		}



		//Si no esta el modo real zx8081, no hacer esto
		if (rainbow_enabled.v==1) {
extern int se_ha_escrito_en_linea;
			se_ha_escrito_en_linea=1;

			
			z80_byte sprite;
			int x;

			int t_estados_en_linea=t_estados % screen_testados_linea;


	        	        //poner caracter en pantalla de video highmem
		                z80_bit caracter_inverse;
		                z80_int direccion_sprite;
		                

        		        if (caracter&128) {
		                        caracter_inverse.v=1;
		                        caracter=caracter&127;
	        	        }
		                else caracter_inverse.v=0;

				//printf ("fetch caracter: %d\n",caracter);
				int y=t_scanline_draw;

                                //TODO
                                y -=LINEAS_SUP_NO_USABLES;
                                //para evitar las lineas superiores 
                                //TODO. cuadrar esto con valores de borde invisible superior

				//Posible modo wrx, y excluir valores de I usados en chr$128 y udg
				if (reg_i>=33 && reg_i!=0x31 && reg_i!=0x30 && wrx_present.v==0 && autodetect_wrx.v) {
					//posible modo wrx
					debug_printf(VERBOSE_INFO,"Autoenabling wrx so the program seems to need it (I register>32). Also enable 8K RAM in 2000H");
					enable_wrx();

					//algunos juegos requieren que este ram pack este presente antes de activar wrx... sino no funcionara
					//pero igualmente, por si acaso, lo activamos aqui
					ram_in_8192.v=1;

				}


				//Modos WRX
				if (wrx_present.v==1 && reg_i>=32 ) {

					//printf ("reg_i en zona WRX\n");

                			direccion_sprite=(reg_i<<8) | (reg_r_bit7 & 128) | ((reg_r) & 127);

					if (video_zx8081_estabilizador_imagen.v==0) {
						x=(t_estados_en_linea)*2;
					}
					else {
						//Estabilizador de imagen, para que no "tiemble"
						x=(video_zx8081_caracter_en_linea_actual-8)*8; //+offset_zx8081_t_coordx;
					}

					//printf ("direccion_sprite: %d\n",direccion_sprite);
                                        sprite=memoria_spectrum[direccion_sprite];

                                        if (caracter_inverse.v) sprite=sprite^255;

        			}


				else {

					//chr$128
                                        if (reg_i==0x31) {
                                           if (caracter_inverse.v) {
                                             //El bit de inverse es para acceder a los 64 caracteres siguientes
                                             caracter=caracter | 64;
                                             //Pero sigue indicando inverse
                                             //caracter_inverse.v=0;

                                           }
                                        }


					//TODO. Parche. Ajustar linea y linecntr. Para que no "salte"
        		                //z80_byte cdflag=memoria_spectrum[0x403B];

					//Quiza se deberia hacer esto solo cuando reg_i apunta a zona de ROM
					//pero resulta que Manic miner y otros usan reg_i apuntando a ROM.. y por tanto aplicaria
					//el ajuste para slow y se ve mal
		                        //if ( machine_type==21 && (cdflag & 128) && video_zx8081_slow_adjust.v==1 && reg_i == 0x1e) {
		                        //if ( machine_type==21 && (cdflag & 128) && video_zx8081_slow_adjust.v==1) {

		                        if ( video_zx8081_lnctr_adjust.v==1) {
						direccion_sprite=((reg_i&254)*256)+caracter*8+( (video_zx8081_linecntr-1) & 7);
					}

					else {	
						direccion_sprite=((reg_i&254)*256)+caracter*8+(video_zx8081_linecntr & 7);
					}



					if (video_zx8081_estabilizador_imagen.v==0) {
                                                x=t_estados_en_linea*2-24;
extern int temp_contador_hsync;
						//x=(temp_contador_hsync%207)*2;

						//x=(video_zx8081_caracter_en_linea_actual+6)*8;

						//if (x>256) {
						//	printf ("---%d\n",x);
						//	sleep(3);
						//}
					}

					else {
						//Estabilizador de imagen, para que no "tiemble"
                                        	//x=(video_zx8081_caracter_en_linea_actual+6)*8; //+offset_zx8081_t_coordx;
                                        	x=(video_zx8081_caracter_en_linea_actual-8)*8; //+offset_zx8081_t_coordx;
					}


					//printf ("direccion_sprite: %d\n",direccion_sprite);
                                        sprite=memoria_spectrum[direccion_sprite];
					//aunque este en modo zxpand, la tabla de caracteres siempre sale de la rom principal
					//por eso hacemos sprite=memoria_spectrum[direccion_sprite]; y zxpand rom esta en otro puntero de memoria
					//sprite=peek_byte_zx80_no_time(direccion_sprite);

                                        if (caracter_inverse.v) sprite=sprite^255;

				}

                                //ajustar para determinados juegos
                                x=x+offset_zx8081_t_coordx;



				if (border_enabled.v==0) {
					y=y-screen_borde_superior;
					x=x-screen_total_borde_izquierdo;
				}


				//printf ("fetch y: %d x: %d\n",y,video_zx8081_caracter_en_linea_actual);
				int totalancho=get_total_ancho_rainbow();

                                //valores negativos vienen por la derecha
                                if (x<0) {
                                   x=totalancho-screen_total_borde_derecho-screen_total_borde_izquierdo+x;
                                }

                                //valores mayores por la derecha
                                if (x>=totalancho ) {
                                      //x=x-totalancho+screen_total_borde_derecho+screen_total_borde_izquierdo-video_zx8081_decremento_x_cuando_mayor;
                                      x=x-totalancho+screen_total_borde_derecho+screen_total_borde_izquierdo;
                                 }


				if (y>=0 && y<get_total_alto_rainbow() ) {

						if (x>=0 && x<totalancho )  {

						        //si linea no coincide con entrelazado, volvemos
							if (if_store_scanline_interlace(y) ) {
								screen_store_scanline_char_zx8081(x,y,sprite,caracter,caracter_inverse.v);
							}


						 }

				}


			//video_zx8081_caracter_en_linea_actual++;
		}

		
		//Si no modo real video
		else {
			//Intentar autodetectar si hay que activar realvideo
			if (autodetect_rainbow.v) {
				if (MACHINE_IS_ZX80) {
					//ZX80
					if (reg_i!=0x0e) {
						debug_printf(VERBOSE_INFO,"Autoenabling realvideo so the program seems to need it (I register on ZX80 != 0x0e)");
						enable_rainbow();
					}
				}
				if (MACHINE_IS_ZX81) {
					//ZX81
					if (reg_i!=0x1e) {
						debug_printf(VERBOSE_INFO,"Autoenabling realvideo so the program seems to need it (I register on ZX81 != 0x1e)");
						enable_rainbow();
					}
				}
			}
			
		}
	}

	return op;

}

//Si dir>16383, leemos RAM. Sino, leemos ROM siempre que la ram oculta este oculta
z80_byte peek_byte_no_time_spectrum_inves(z80_int dir)
{
	if (dir<16384) {
		if (inves_show_hidden_ram.v==0) {
			return memoria_spectrum[65536+dir];
		}
		else return memoria_spectrum[dir];
	}
        else return memoria_spectrum[dir];
}



z80_byte peek_byte_spectrum_inves(z80_int dir)
{
#ifdef EMULATE_CONTEND
        if ( (dir&49152)==16384) {
                //printf ("%d\n",t_estados);
                t_estados += contend_table[ t_estados ];
        }
#endif

        t_estados +=3;


	return peek_byte_no_time_spectrum_inves(dir);

}



z80_byte peek_byte_no_time_spectrum_48k(z80_int dir)
{
        return memoria_spectrum[dir];
}       


z80_byte peek_byte_spectrum_48k(z80_int dir)
{
#ifdef EMULATE_CONTEND
        if ( (dir&49152)==16384) {
		//printf ("%d\n",t_estados);
                t_estados += contend_table[ t_estados ];
        }
#endif

        t_estados +=3;


#ifdef DEBUG_SECOND_TRAP_STDOUT

	//Para hacer debug de aventuras de texto, investigar desde donde se estan leyendo las tablas de caracteres
	//hay que definir este DEBUG_SECOND_TRAP_STDOUT manualmente en compileoptions.h despues de ejecutar el configure
	scr_stdout_debug_char_table_routines_peek(dir);

#endif



	return memoria_spectrum[dir];

}


//Funcion usada en la emulacion de refresco de RAM
z80_byte peek_byte_spectrum_48k_refresh_memory(z80_int dir)
{
	        //emulacion refresco memoria superior en 48kb
                //si es memoria alta y ha llegado a la mitad del maximo, memoria ram inestable y retornamos valores indeterminados
                if (machine_emulate_memory_refresh_counter>=MAX_EMULATE_MEMORY_REFRESH_LIMIT) {
                        if (dir>32767) {
				//Alteramos el valor enviandolo a 255
				z80_byte c=memoria_spectrum[dir];
				if (c<255) {
					c++;
					memoria_spectrum[dir]=c;
                                	debug_printf (VERBOSE_DEBUG,"RAM is bad, altering byte value at address: %d",dir);
				}
				

                                //valor ligeramente aleatorio->invertimos valor y metemos bit 1 a 1
                                //printf ("devolvemos valor ram sin refrescar dir: %d\n",dir);
                                //return (memoria_spectrum[dir] ^ 255) | 1;
                        }       
                }       

	return peek_byte_spectrum_48k(dir);
}

//Puntero a la funcion original
z80_byte (*peek_byte_no_ram_refresh)(z80_int dir);


void set_peek_byte_function_ram_refresh(void)
{

        debug_printf(VERBOSE_INFO,"Enabling RAM refresh on peek_byte");

        peek_byte_no_ram_refresh=peek_byte;
        peek_byte=peek_byte_spectrum_48k_refresh_memory;

}

void reset_peek_byte_function_ram_refresh(void)
{
	debug_printf(VERBOSE_INFO,"Setting normal peek_byte");
        peek_byte=peek_byte_no_ram_refresh;
}


z80_byte peek_byte_no_time_spectrum_128k(z80_int dir)
{

	int segmento;
	z80_byte *puntero;
	segmento=dir / 16384;


	dir = dir & 16383;
	puntero=memory_paged[segmento]+dir;
//		printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
	return *puntero;
}


z80_byte peek_byte_spectrum_128k(z80_int dir)
{

	int segmento;
	z80_byte *puntero;
	segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
                        t_estados += contend_table[ t_estados ];
                }
#endif
                t_estados += 3;



	dir = dir & 16383;
	puntero=memory_paged[segmento]+dir;
//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
	return *puntero;
}




z80_byte peek_byte_no_time_spectrum_128kp2a(z80_int dir)
{
	int segmento;
	z80_byte *puntero;
	segmento=dir / 16384;

	dir = dir & 16383;
	puntero=memory_paged[segmento]+dir;
//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
	return *puntero;

}

z80_byte peek_byte_spectrum_128kp2a(z80_int dir)
{
	int segmento;
	z80_byte *puntero;
	segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
                        t_estados += contend_table[ t_estados ];
                }
#endif
                t_estados += 3;


	dir = dir & 16383;
	puntero=memory_paged[segmento]+dir;
//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
	return *puntero;

}



z80_byte peek_byte_no_time_spectrum_16k(z80_int dir)
{

        if (dir<32768) return memoria_spectrum[dir];
	else return 255;
}       

z80_byte peek_byte_spectrum_16k(z80_int dir)
{

#ifdef EMULATE_CONTEND
        if ( (dir&49152)==16384) {
                t_estados += contend_table[ t_estados ];
        }
#endif

        t_estados += 3;


        if (dir<32768) return memoria_spectrum[dir];
        else return 255;
}



void poke_byte_no_time_zxuno(z80_int dir,z80_byte valor)
{
	int segmento;
	z80_byte *puntero;
	segmento=dir / 16384;

	//Modo BOOTM

	if ( (zxuno_ports[0]&1)==1) {
		//Si no es rom
		if (dir>16383) {
			dir = dir & 16383;
		
			puntero=zxuno_bootm_memory_paged[segmento]+dir;
			*puntero=valor;
		}
	}

	else {
		//Modo no bootm. como un +2a

		if (dir>16383) {
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
			dir = dir & 16383;
			puntero=zxuno_no_bootm_memory_paged[segmento]+dir;

	//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
			*puntero=valor;
		}

		else {
			//memoria normalmente ROM. Miramos a ver si esta page RAM in ROM
			if ((puerto_8189&1)==1) {
				//printf ("Poke en direccion normalmente ROM pero hay RAM. Dir=%d Valor=%d\n",dir,valor);
#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
				puntero=zxuno_no_bootm_memory_paged[0]+dir;
				*puntero=valor;
			}
		}

	}

}

void poke_byte_zxuno(z80_int dir,z80_byte valor)
{
int segmento;
                segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
                        t_estados += contend_table[ t_estados ];
                }
#endif
                t_estados += 3;

	poke_byte_no_time_zxuno(dir,valor);
}



z80_byte peek_byte_no_time_zxuno(z80_int dir)
{
        int segmento;
        z80_byte *puntero;
        segmento=dir / 16384;
	dir = dir & 16383;

	//Modo BOOTM

	if ( (zxuno_ports[0]&1)==1) {
		puntero=zxuno_bootm_memory_paged[segmento]+dir;
//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
		return *puntero;

	}

	else {
		
		puntero=zxuno_no_bootm_memory_paged[segmento]+dir;
//              printf ("segmento: %d dir: %d puntero: %p\n",segmento,dir,puntero);
		return *puntero;
	}

}

z80_byte peek_byte_zxuno(z80_int dir)
{
        int segmento;
        segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
                        t_estados += contend_table[ t_estados ];
                }
#endif
                t_estados += 3;

	return peek_byte_no_time_zxuno(dir);

}



z80_byte *chloe_return_segment_memory(z80_int dir)
{
	int segmento;
	z80_byte *puntero;

	segmento=dir/8192;
	puntero=chloe_memory_paged[segmento];
	return puntero;
}
	

void poke_byte_no_time_chloe(z80_int dir,z80_byte valor)
{

#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
		z80_byte *puntero;
		puntero=chloe_return_segment_memory(dir);

		//proteccion rom
		////Constantes definidas en CHLOE_MEMORY_TYPE_ROM, _HOME, _DOCK, _EX
		//z80_byte chloe_type_memory_paged[8];
		if (dir<16384) {
			//Segmento 0 o 1 
			int segmento=dir/8192;
			if (chloe_type_memory_paged[segmento]==CHLOE_MEMORY_TYPE_ROM) return;
		}

		dir = dir & 8191;

		puntero=puntero+dir;
		*puntero=valor;
        
}

void poke_byte_chloe(z80_int dir,z80_byte valor)
{
int segmento;
                segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
                        t_estados += contend_table[ t_estados ];

                }
#endif
                t_estados += 3;

        poke_byte_no_time_chloe(dir,valor);
}



z80_byte peek_byte_no_time_chloe(z80_int dir)
{
		z80_byte *puntero;
		puntero=chloe_return_segment_memory(dir);

		dir = dir & 8191;

		puntero=puntero+dir;
		return *puntero;
}


z80_byte peek_byte_chloe(z80_int dir)
{
        int segmento;
        segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
                        t_estados += contend_table[ t_estados ];
                }
#endif
                t_estados += 3;

        return peek_byte_no_time_chloe(dir);

}


z80_byte *timex_return_segment_memory(z80_int dir)
{
        int segmento;
        z80_byte *puntero;

        segmento=dir/8192;
        puntero=timex_memory_paged[segmento];
        return puntero;
}
        

void poke_byte_no_time_timex(z80_int dir,z80_byte valor)
{

#ifdef EMULATE_VISUALMEM

set_visualmembuffer(dir);

#endif
                z80_byte *puntero;
                puntero=timex_return_segment_memory(dir);

                //proteccion rom
                ////Constantes definidas en TIMEX_MEMORY_TYPE_ROM, _HOME, _DOCK, _EX
                //z80_byte timex_type_memory_paged[8];
                if (dir<16384) {
                        //Segmento 0 o 1 
                        int segmento=dir/8192;
			if (timex_type_memory_paged[segmento]!=TIMEX_MEMORY_TYPE_HOME) return;
                }

                dir = dir & 8191;

                puntero=puntero+dir;
                *puntero=valor;
        
}


void poke_byte_timex(z80_int dir,z80_byte valor)
{

//TODO. tabla contend de timex la hacemos que funcione como spectrum 48k: dir<32768, contend
//pese a que haya ex o dock mapeado
#ifdef EMULATE_CONTEND
        if ( (dir&49152)==16384) {
                t_estados += contend_table[ t_estados ];
        }
#endif

                t_estados += 3;

        poke_byte_no_time_timex(dir,valor);
}



z80_byte peek_byte_no_time_timex(z80_int dir)
{
                z80_byte *puntero;
                puntero=timex_return_segment_memory(dir);

                dir = dir & 8191;

                puntero=puntero+dir;
                return *puntero;
}


z80_byte peek_byte_timex(z80_int dir)
{
        int segmento;
        segmento=dir / 16384;

#ifdef EMULATE_CONTEND
                if (contend_pages_actual[segmento]) {
                        t_estados += contend_table[ t_estados ];
                }
#endif
                t_estados += 3;

        return peek_byte_no_time_timex(dir);

}













z80_int peek_word(z80_int dir)
{
        z80_byte h,l;

        l=peek_byte(dir);
        h=peek_byte(dir+1);

        return (h<<8) | l;
}

z80_int peek_word_no_time(z80_int dir)
{
        z80_byte h,l;

        l=peek_byte_no_time(dir);
        h=peek_byte_no_time(dir+1);

        return (h<<8) | l;
}



z80_int lee_word_pc(void)
{
        z80_int valor;

        valor=peek_word(reg_pc);
        reg_pc +=2;
        return valor;

	//este reg_pc+=2 es incorrecto, no funciona
	//return peek_word(reg_pc+=2);
}

void poke_word(z80_int dir,z80_int valor)
{
        poke_byte(dir,valor & 0xFF);
        poke_byte(dir+1, (valor >> 8) & 0xFF );
}



z80_byte peek_byte_desp(z80_int dir,z80_byte desp)
{


//RETURN (dir+desp)

        z80_int desp16,puntero;

        desp16=desp8_to_16(desp);

        puntero=dir + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif

        return peek_byte(puntero);

}

void poke_byte_desp(z80_int dir,z80_byte desp,z80_byte valor)
{
        z80_int desp16,puntero;

        desp16=desp8_to_16(desp);

        puntero=dir + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif

	poke_byte(puntero,valor);
}






       







z80_int sbc_16bit(z80_int reg, z80_int value)
{

	z80_int result;
        z80_byte h;
        int result_32bit;

        set_memptr(reg+1);



        result_32bit=reg-value-( Z80_FLAGS & FLAG_C );

        z80_int lookup =      ( (  (reg) & 0x8800 ) >> 11 ) | \
                            ( (  (value) & 0x8800 ) >> 10 ) | \
                            ( ( result_32bit & 0x8800 ) >>  9 );  \


        result=result_32bit & 65535;
        h=value_16_to_8h(result);
        set_undocumented_flags_bits(h);


        if (result_32bit & 0x10000) Z80_FLAGS |=FLAG_C;
        else Z80_FLAGS &=(255-FLAG_C);

        set_flags_zero_sign_16(result);

	Z80_FLAGS=(Z80_FLAGS & (255-FLAG_H-FLAG_PV)) | halfcarry_sub_table[lookup&0x07] | overflow_sub_table[lookup >> 4] | FLAG_N;

	return result;


}





z80_int add_16bit(z80_int reg, z80_int value)
{
        z80_int result,antes;
        z80_byte h;

	set_memptr(reg+1);


        antes=reg;
        result=reg;
        
        result +=value;

	z80_int lookup = ( (       (reg) & 0x0800 ) >> 11 ) | \
                            ( (  (value) & 0x0800 ) >> 10 ) | \
                            ( (   result & 0x0800 ) >>  9 );  \


        h=value_16_to_8h(result);
        set_undocumented_flags_bits(h);

        set_flags_carry_16_suma(antes,result);


	Z80_FLAGS=(Z80_FLAGS & (255-FLAG_H-FLAG_N)) | halfcarry_add_table[lookup];


        return result;
}

z80_int adc_16bit(z80_int reg, z80_int value)
{

        z80_int result;
        z80_byte h;
	int result_32bit;

	set_memptr(reg+1);


        result_32bit=reg+value+( Z80_FLAGS & FLAG_C );

	z80_int lookup =      ( (  (reg) & 0x8800 ) >> 11 ) | \
                            ( (  (value) & 0x8800 ) >> 10 ) | \
                            ( ( result_32bit & 0x8800 ) >>  9 );  \


	result=result_32bit & 65535;
        h=value_16_to_8h(result);
        set_undocumented_flags_bits(h);


        if (result_32bit & 0x10000) Z80_FLAGS |=FLAG_C;
        else Z80_FLAGS &=(255-FLAG_C);

        set_flags_zero_sign_16(result);

        Z80_FLAGS=(Z80_FLAGS & (255-FLAG_H-FLAG_PV-FLAG_N)) | halfcarry_add_table[lookup&0x07] | overflow_add_table[lookup >> 4];


        return result;


}

/*z80_int inc_16bit(z80_int reg)
{
//INC 16 bit register
	reg++;
	return reg;
}

z80_int dec_16bit(z80_int reg)
{
//DEC 16 bit register
	reg--;
	return reg;
}
*/

/*

z80_int inc_16bit(z80_int reg)
{
//INC 16 bit register

	z80_byte h;

        reg++;
        
        h=value_16_to_8h(reg);
        set_undocumented_flags_bits(h);

	return reg;

}


z80_int dec_16bit(z80_int reg)
{
//DEC 16 bit register
	z80_byte h;

        reg--;

        h=value_16_to_8h(reg);
        set_undocumented_flags_bits(h);

        return reg;

}
*/

z80_int desp8_to_16(z80_byte desp)
{

	z80_int desp16;

        if (desp>127) {
		desp=256-desp;
                desp16=-desp;
	}
        else
                desp16=desp;

	return desp16;

}



z80_int pop_valor()
{
	z80_int valor;

        valor=peek_word(reg_sp);
        reg_sp +=2;
        return valor;

}


z80_byte rlc_valor_comun(z80_byte value)
{
        if (value & 128) Z80_FLAGS |=FLAG_C;
        else Z80_FLAGS &=(255-FLAG_C);

        value=value << 1;
        value |= ( Z80_FLAGS & FLAG_C );

        set_undocumented_flags_bits(value);
	Z80_FLAGS &=(255-FLAG_N-FLAG_H);

        return value;

}

z80_byte rlc_valor(z80_byte value)
{
	value=rlc_valor_comun(value);
	set_flags_zero_sign(value);
	set_flags_parity(value);
	return value;
}


void rlc_reg(z80_byte *reg)
{
        z80_byte value=*reg;

        *reg=rlc_valor(value);

}

void rlca(void)
{
        //RLCA no afecta a S, Z ni P
        reg_a=rlc_valor_comun(reg_a);
}



//rutina comun a rl y rla
z80_byte rl_valor_comun(z80_byte value)
{
        z80_bit flag_C_antes;
        flag_C_antes.v=( Z80_FLAGS & FLAG_C );
        
        if (value & 128) Z80_FLAGS |=FLAG_C;
        else Z80_FLAGS &=(255-FLAG_C);
        
        value=value << 1;
        value |= flag_C_antes.v;
        
        set_undocumented_flags_bits(value);
        
        Z80_FLAGS &=(255-FLAG_N-FLAG_H);
        
        return value;

}

z80_byte rl_valor(z80_byte value)
{
	value=rl_valor_comun(value);
	set_flags_zero_sign(value);
	set_flags_parity(value);
        return value;

}

void rl_reg(z80_byte *reg)
{
        z80_byte value=*reg;

        *reg=rl_valor(value);

}


void rla(void)
{
	//RLA no afecta a S, Z ni P
        reg_a=rl_valor_comun(reg_a);
}



z80_byte rr_valor_comun(z80_byte value)
{
        z80_bit flag_C_antes;
        flag_C_antes.v=( Z80_FLAGS & FLAG_C );

        if (value & 1) Z80_FLAGS |=FLAG_C;
        else Z80_FLAGS &=(255-FLAG_C);

        value=value >> 1;
        value |= (flag_C_antes.v*128);

        set_undocumented_flags_bits(value);
	Z80_FLAGS &=(255-FLAG_N-FLAG_H);

        return value;

}

z80_byte rr_valor(z80_byte value)
{
	value=rr_valor_comun(value);
        set_flags_zero_sign(value);
	set_flags_parity(value);
	return value;
}


void rr_reg(z80_byte *reg)
{
        z80_byte value=*reg;

        *reg=rr_valor(value);
        
}     


void rra(void)
{
        //RRA no afecta a S, Z ni P
        reg_a=rr_valor_comun(reg_a);

}

z80_byte rrc_valor_comun(z80_byte value)
{
        if (value & 1) Z80_FLAGS |=FLAG_C;
        else Z80_FLAGS &=(255-FLAG_C);

        value=value >> 1;
        value |= (( Z80_FLAGS & FLAG_C )*128);

        set_undocumented_flags_bits(value);
	Z80_FLAGS &=(255-FLAG_N-FLAG_H);

        return value;

}

z80_byte rrc_valor(z80_byte value)
{
	value=rrc_valor_comun(value);
	set_flags_zero_sign(value);
	set_flags_parity(value);
	return value;
}

void rrc_reg(z80_byte *reg)
{
        z80_byte value=*reg;

        *reg=rrc_valor(value);

}



void rrca(void)
{
        //RRCA no afecta a S, Z ni P
        reg_a=rrc_valor_comun(reg_a);
}





z80_byte sla_valor(z80_byte value)
{

	Z80_FLAGS=0;

        if (value & 128) Z80_FLAGS |=FLAG_C;
        
        value=value << 1;

	Z80_FLAGS |=sz53p_table[value];
        
        return value;
        
}      

void sla_reg(z80_byte *reg)
{

        z80_byte value=*reg;

        *reg=sla_valor(value);

}

z80_byte sra_valor(z80_byte value)
{
        z80_byte value7=value & 128;

	Z80_FLAGS=0;

        if (value & 1) Z80_FLAGS |=FLAG_C;

        value=value >> 1;
        value |= value7;

	Z80_FLAGS |=sz53p_table[value];

        return value;

}

void sra_reg(z80_byte *reg)
{
        z80_byte value=*reg;

        *reg=sra_valor(value);

}

z80_byte srl_valor(z80_byte value)
{

	Z80_FLAGS=0;

        if (value & 1) Z80_FLAGS |=FLAG_C;

        value=value >> 1;

	Z80_FLAGS |=sz53p_table[value];

        return value;

}

void srl_reg(z80_byte *reg)
{
        z80_byte value=*reg;

        *reg=srl_valor(value);

}

z80_byte sls_valor(z80_byte value)
{

	Z80_FLAGS=0;

        if (value & 128) Z80_FLAGS |=FLAG_C;
        
        value=(value << 1) | 1;

	Z80_FLAGS |=sz53p_table[value];

        return value;
        
}

void sls_reg(z80_byte *reg)
{

        z80_byte value=*reg;
        
        *reg=sls_valor(value);

}   


void add_a_reg(z80_byte value)
{

        z80_byte result,antes;

        result=reg_a;
        antes=reg_a;

        result +=value;
        reg_a=result;


        set_flags_carry_suma(antes,result);
        set_flags_overflow_suma(antes,result);
        Z80_FLAGS=(Z80_FLAGS & (255-FLAG_N-FLAG_Z-FLAG_S-FLAG_5-FLAG_3)) | sz53_table[result];


}


void adc_a_reg(z80_byte value)
{

	//printf ("flag_C antes: %d ",( Z80_FLAGS & FLAG_C ));

        z80_byte result,lookup;
	z80_int result_16bit;


        result_16bit=reg_a;
        result_16bit=result_16bit+value+( Z80_FLAGS & FLAG_C );

	lookup = ( (       reg_a & 0x88 ) >> 3 ) | 
                     ( ( (value) & 0x88 ) >> 2 ) | 
                ( ( result_16bit & 0x88 ) >> 1 );  


	result=value_16_to_8l(result_16bit);
        reg_a=result;


	Z80_FLAGS = ( result_16bit & 0x100 ? FLAG_C : 0 );

	Z80_FLAGS=Z80_FLAGS | halfcarry_add_table[lookup & 0x07] | overflow_add_table[lookup >> 4] | sz53_table[result];

	//printf ("antes: %d value: %d result: %d flag_c: %d flag_pv: %d\n", antes, value, result, ( Z80_FLAGS & FLAG_C ),flag_PV.v); 

}


//Devuelve el resultado de A-value sin alterar A
z80_byte sub_value(z80_byte value)
{
        z80_byte result,antes;

        result=reg_a;
        antes=reg_a;

        result -=value;

        set_flags_carry_resta(antes,result); 
        set_flags_overflow_resta(antes,result); 
        Z80_FLAGS=(Z80_FLAGS & (255-FLAG_Z-FLAG_S-FLAG_5-FLAG_3)) | FLAG_N | sz53_table[result]; 

	return result;


}

void sub_a_reg(z80_byte value)
{
	reg_a=sub_value(value);

}

void sbc_a_reg(z80_byte value)
{
        z80_byte result,lookup;
        z80_int result_16bit;

        result_16bit=reg_a;
	result_16bit=result_16bit-value-( Z80_FLAGS & FLAG_C );

	
        lookup = ( (       reg_a & 0x88 ) >> 3 ) |
                     ( ( (value) & 0x88 ) >> 2 ) |
                ( ( result_16bit & 0x88 ) >> 1 );

	result=value_16_to_8l(result_16bit);
        reg_a=result;


	Z80_FLAGS = ( result_16bit & 0x100 ? FLAG_C : 0 );

        Z80_FLAGS=Z80_FLAGS | halfcarry_sub_table[lookup & 0x07] | overflow_sub_table[lookup >> 4] | FLAG_N | sz53_table[result]; 


}
      



void rl_ixiy_desp_reg(z80_byte desp,z80_byte *registro)
{
        z80_byte valor_leido;
        z80_int desp16,puntero;

	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
	set_memptr(puntero);
#endif

        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = rl_valor(valor_leido);
        poke_byte(puntero,valor_leido);

        if (registro!=0) *registro=valor_leido;
}

void rr_ixiy_desp_reg(z80_byte desp,z80_byte *registro)
{
        z80_byte valor_leido;
        z80_int desp16,puntero;

	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif


        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = rr_valor(valor_leido);
        poke_byte(puntero,valor_leido);

        if (registro!=0) *registro=valor_leido;
}



void rlc_ixiy_desp_reg(z80_byte desp,z80_byte *registro)
{
	z80_byte valor_leido;
	z80_int desp16,puntero;

	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif


        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = rlc_valor(valor_leido);
        poke_byte(puntero,valor_leido);

        if (registro!=0) *registro=valor_leido;
}

void rrc_ixiy_desp_reg(z80_byte desp,z80_byte *registro)
{
        z80_byte valor_leido;
        z80_int desp16,puntero;

	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif


        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = rrc_valor(valor_leido);
        poke_byte(puntero,valor_leido);

        if (registro!=0) *registro=valor_leido;
}

void sla_ixiy_desp_reg(z80_byte desp,z80_byte *registro)
{
        z80_byte valor_leido;
        z80_int desp16,puntero;


	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif


        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = sla_valor(valor_leido);
        poke_byte(puntero,valor_leido);

        if (registro!=0) *registro=valor_leido;
}

void sra_ixiy_desp_reg(z80_byte desp,z80_byte *registro)
{
        z80_byte valor_leido;
        z80_int desp16,puntero;

	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif

        
        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = sra_valor(valor_leido);
        poke_byte(puntero,valor_leido);
        
        if (registro!=0) *registro=valor_leido;
}  

void srl_ixiy_desp_reg(z80_byte desp,z80_byte *registro)
{
        z80_byte valor_leido;
        z80_int desp16,puntero;

	desp16=desp8_to_16(desp);	
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif

        
        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = srl_valor(valor_leido);
        poke_byte(puntero,valor_leido);
        
        if (registro!=0) *registro=valor_leido;
}

void sls_ixiy_desp_reg(z80_byte desp,z80_byte *registro)
{
        z80_byte valor_leido;
        z80_int desp16,puntero;

	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif

        
        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = sls_valor(valor_leido);
        poke_byte(puntero,valor_leido);
        
        if (registro!=0) *registro=valor_leido;
}


void res_bit_ixiy_desp_reg(z80_byte numerobit, z80_byte desp, z80_byte *registro)
{

	z80_byte valor_and,valor_leido;
	z80_int desp16,puntero;

	//printf ("res bit=%d desp=%d reg=%x   \n",numerobit,desp,registro);

	valor_and=1;

	if (numerobit) valor_and = valor_and << numerobit;

	//cambiar 0 por 1
	valor_and = valor_and ^ 0xFF;

	desp16=desp8_to_16(desp);
	puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif


	valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
	valor_leido = valor_leido & valor_and;
	poke_byte(puntero,valor_leido);

	
	if (registro!=0) *registro=valor_leido;
	//printf (" valor_and = %d valor_final = %d \n",valor_and,valor_leido); 
	
}

void set_bit_ixiy_desp_reg(z80_byte numerobit, z80_byte desp, z80_byte *registro)
{

        z80_byte valor_or,valor_leido;
        z80_int desp16,puntero;

        //printf ("set bit=%d desp=%d reg=%x   \n",numerobit,desp,registro);

        valor_or=1;

        if (numerobit) valor_or = valor_or << numerobit;

	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif


        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );
        valor_leido = valor_leido | valor_or;
        poke_byte(puntero,valor_leido);

        if (registro!=0) *registro=valor_leido;
        //printf (" valor_set = %d valor_final = %d \n",valor_or,valor_leido);

}

//void bit_bit_ixiy_desp_reg(z80_byte numerobit, z80_byte desp, z80_byte *registro)
void bit_bit_ixiy_desp_reg(z80_byte numerobit, z80_byte desp)
{

        z80_byte valor_and,valor_leido;
        z80_int desp16,puntero;

        //printf ("bit bit=%d desp=%d reg=%x   \n",numerobit,desp,registro);

        valor_and=1;

        if (numerobit) valor_and = valor_and << numerobit;

	desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

#ifdef EMULATE_MEMPTR
        set_memptr(puntero);
#endif


        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );

//                                        ;Tambien hay que poner el flag P/V con el mismo valor que
//                                        ;coge el flag Z, y el flag S debe tener el valor del bit 7


	Z80_FLAGS=(Z80_FLAGS & (255-FLAG_N-FLAG_Z-FLAG_PV-FLAG_S)) | FLAG_H;

	if (!(valor_leido & valor_and) ) {
		Z80_FLAGS |=FLAG_Z|FLAG_PV;
	}

        if (numerobit==7 && (valor_leido & 128) ) Z80_FLAGS |=FLAG_S;

        //En teoria el BIT no tiene este comportamiento de poder asignar el resultado a un registro
        //if (registro!=0) *registro=valor_leido;

        //printf (" valor_bit = %d valor_final = %d \n",valor_or,valor_leido);

        set_undocumented_flags_bits(value_16_to_8h(puntero));

}




z80_byte *devuelve_reg_offset(z80_byte valor)
{

	//printf ("devuelve_reg de: %d\n",valor);
	switch (valor) {
		case 0:
			return &reg_b;
		;;
		case 1:
			return &reg_c;
		;;
		case 2:
			return &reg_d;
		;;
		case 3:
			return &reg_e;
		;;
		case 4:
			return &reg_h;
		;;
		case 5:
			return &reg_l;
		;;
		case 6:
			return 0;
		;;
		case 7:
			return &reg_a;
		;;

		default:
			cpu_panic("Critical Error devuelve_reg_offset valor>7");
			//aqui no deberia llegar nunca
			return 0;
		break;
	}

}



void rl_cb_reg(z80_byte *registro)
{
        z80_byte valor_leido;

	if (registro==0) {
		//(HL)
		valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
		valor_leido=rl_valor(valor_leido);
        	poke_byte(HL,valor_leido);
	}

	else rl_reg(registro);

}

void rr_cb_reg(z80_byte *registro)
{
        z80_byte valor_leido;

        if (registro==0) {
                //(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
                valor_leido=rr_valor(valor_leido);
                poke_byte(HL,valor_leido);
        }

        else rr_reg(registro);
}



void rlc_cb_reg(z80_byte *registro)
{
        z80_byte valor_leido;

        if (registro==0) {
                //(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
                valor_leido=rlc_valor(valor_leido);
                poke_byte(HL,valor_leido);
        }

        else rlc_reg(registro);
}

void rrc_cb_reg(z80_byte *registro)
{
        z80_byte valor_leido;

        if (registro==0) {
                //(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
                valor_leido=rrc_valor(valor_leido);
                poke_byte(HL,valor_leido);
        }

        else rrc_reg(registro);
}

void sla_cb_reg(z80_byte *registro)
{
        z80_byte valor_leido;

        if (registro==0) {
                //(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
                valor_leido=sla_valor(valor_leido);
                poke_byte(HL,valor_leido);
        }

        else sla_reg(registro);
}

void sra_cb_reg(z80_byte *registro)
{
        z80_byte valor_leido;

        if (registro==0) {
                //(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
                valor_leido=sra_valor(valor_leido);
                poke_byte(HL,valor_leido);
        }

        else sra_reg(registro);
}  

void srl_cb_reg(z80_byte *registro)
{
        z80_byte valor_leido;

        if (registro==0) {
                //(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
                valor_leido=srl_valor(valor_leido);
                poke_byte(HL,valor_leido);
        }

        else srl_reg(registro);

}

void sls_cb_reg(z80_byte *registro)
{
        z80_byte valor_leido;

        if (registro==0) {
                //(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
                valor_leido=sls_valor(valor_leido);
                poke_byte(HL,valor_leido);
        }

        else sls_reg(registro);
}


void res_bit_cb_reg(z80_byte numerobit, z80_byte *registro)
{

	z80_byte valor_and,valor_leido;

	//printf ("res bit=%d reg=%x  \n",numerobit,registro);

	valor_and=1;

	if (numerobit) valor_and = valor_and << numerobit;

	//cambiar 0 por 1
	valor_and = valor_and ^ 0xFF;

	if (registro==0) {
		//(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
		valor_leido=valor_leido & valor_and;
                poke_byte(HL,valor_leido);
	}	
	else {
		valor_leido = (*registro) & valor_and;
		*registro = valor_leido;
	}

	//printf (" valor_and = %d valor_final = %d \n",valor_and,valor_leido); 
	
}

void set_bit_cb_reg(z80_byte numerobit, z80_byte *registro)
{

        z80_byte valor_or,valor_leido;
        
	//printf ("set bit=%d reg=%x  \n",numerobit,registro);
        
        valor_or=1;
        
        if (numerobit) valor_or = valor_or << numerobit;
        
        if (registro==0) {
                //(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
                valor_leido = valor_leido | valor_or;
                poke_byte(HL,valor_leido);
        }       
        else {
        
                valor_leido = (*registro) | valor_or;
                *registro = valor_leido;
        }
        
        //printf (" valor_set = %d valor_final = %d \n",valor_or,valor_leido);

}

void bit_bit_cb_reg(z80_byte numerobit, z80_byte *registro)
{

        z80_byte valor_or,valor_leido;

	//printf ("bit bit=%d reg=%x ",numerobit,registro);

        valor_or=1;

        if (numerobit) valor_or = valor_or << numerobit;

	if (registro==0) {
		//(HL)
                valor_leido=peek_byte(HL);
		contend_read_no_mreq( HL, 1 );
		set_undocumented_flags_bits_memptr();
		
	}
	else {
		valor_leido=*registro;
	        if (numerobit==5 && (valor_leido & 32 )) Z80_FLAGS |=FLAG_5;
	        else Z80_FLAGS &=(255-FLAG_5);

        	if (numerobit==3 && (valor_leido & 8 )) Z80_FLAGS |=FLAG_3;
	        else Z80_FLAGS &=(255-FLAG_3);

	}

//                                        ;Tambien hay que poner el flag P/V con el mismo valor que
//                                        ;coge el flag Z, y el flag S debe tener el valor del bit 7


	Z80_FLAGS=(Z80_FLAGS & (255-FLAG_N-FLAG_Z-FLAG_PV-FLAG_S)) | FLAG_H;

	if (!(valor_leido & valor_or) ) {
		Z80_FLAGS |=FLAG_Z|FLAG_PV;
	}

	if (numerobit==7 && (valor_leido & 128) ) Z80_FLAGS |=FLAG_S;

        //printf (" valor_bit = %d valor_final = %d \n",valor_or,valor_leido);


}


//;                    Bits:  4    3    2    1    0     ;desplazamiento puerto
//puerto_65278   db    255  ; V    C    X    Z    Sh    ;0
//puerto_65022   db    255  ; G    F    D    S    A     ;1
//puerto_64510    db              255  ; T    R    E    W    Q     ;2
//puerto_63486    db              255  ; 5    4    3    2    1     ;3
//puerto_61438    db              255  ; 6    7    8    9    0     ;4
//puerto_57342    db              255  ; Y    U    I    O    P     ;5
//puerto_49150    db              255  ; H                J         K      L    Enter ;6
//puerto_32766    db              255  ; B    N    M    Simb Space ;7

z80_byte lee_puerto_zx80_no_time(z80_byte puerto_h,z80_byte puerto_l)
{

	z80_byte valor;

	//xx1D Zebra Joystick                          - - - F R L D U   (0=Pressed)
	if ( puerto_l==0x1d) {
		 if (joystick_emulation==JOYSTICK_ZEBRA) {
			z80_byte valor_joystick=255;
			//si estamos con menu abierto, no retornar nada
			if (menu_abierto==1) return valor_joystick;

			//z80_byte puerto_especial_joystick=0; //Fire Up Down Left Right

			if ((puerto_especial_joystick&1)) valor_joystick &=(255-8);
			if ((puerto_especial_joystick&2)) valor_joystick &=(255-4);
			if ((puerto_especial_joystick&4)) valor_joystick &=(255-2);
			if ((puerto_especial_joystick&8)) valor_joystick &=(255-1);
			if ((puerto_especial_joystick&16)) valor_joystick &=(255-16);

                        return valor_joystick;
                }
		return 255;
	}

	//xxDF Mikro-Gen Digital Joystick (Port DFh) - - - F L R D U   (0=Pressed)

        else if ( puerto_l==0xdf) {
                 if (joystick_emulation==JOYSTICK_MIKROGEN) {
                        z80_byte valor_joystick=255;
                        //si estamos con menu abierto, no retornar nada
                        if (menu_abierto==1) return valor_joystick;

                        //z80_byte puerto_especial_joystick=0; //Fire Up Down Left Right

                        if ((puerto_especial_joystick&1)) valor_joystick &=(255-4);
                        if ((puerto_especial_joystick&2)) valor_joystick &=(255-8);
                        if ((puerto_especial_joystick&4)) valor_joystick &=(255-2);
                        if ((puerto_especial_joystick&8)) valor_joystick &=(255-1);
                        if ((puerto_especial_joystick&16)) valor_joystick &=(255-16);

                        return valor_joystick;
                }
                return 255;
        }

	//zx printer
        else if (puerto_l==0xFB) {
                if (zxprinter_enabled.v==1) {
                        return zxprinter_get_port();
                }


                else return 255;
        }

	//zxpand
	else if (puerto_l==0x07 && zxpand_enabled.v) {
        	//printf ("In Port ZXpand 0x%X asked, PC after=0x%x\n",puerto_l+256*puerto_h,reg_pc);
		z80_byte valor_retorno=zxpand_read(puerto_h);
		//printf ("Returning value 0x%X\n",valor_retorno);
		return valor_retorno;
	}


	//Puerto con A0 cero
	else if ( (puerto_l&1)==0) {


                if (nmi_generator_active.v==0) {

			//printf ("lee puerto. t_estados=%d t_scanline_draw_timeout: %d\n",t_estados,t_scanline_draw_timeout);

			//si es inicio vsync, guardar tiempo inicial

			if (video_zx8081_linecntr_enabled.v==1) {


				//solo admitir inicio vsync hacia el final de pantalla o hacia principio
				//if (t_scanline_draw_timeout>MINIMA_LINEA_ADMITIDO_VSYNC || t_scanline_draw_timeout<=3) {
				if (t_scanline_draw_timeout>MINIMA_LINEA_ADMITIDO_VSYNC) {
					inicio_pulso_vsync_t_estados=t_estados;
					//printf ("admitido inicio pulso vsync: t_estados: %d linea: %d\n",inicio_pulso_vsync_t_estados,t_scanline_draw_timeout);

					//video_zx8081_linecntr=0;
					//video_zx8081_linecntr_enabled.v=0;
				}

				else {
					//printf ("no se admite inicio pulso vsync : t_estados: %d linea: %d\n",inicio_pulso_vsync_t_estados,t_scanline_draw_timeout);
				}

			}


			video_zx8081_linecntr=1;
			video_zx8081_linecntr_enabled.v=0;


                        //printf("Disabling the HSYNC generator t_scanline_draw=%d\n",t_scanline_draw);
                        hsync_generator_active.v=0;
                        modificado_border.v=1;


                	//y ponemos a low la salida del altavoz
	                bit_salida_sonido_zx8081.v=0;

			set_value_beeper_on_array(da_amplitud_speaker_zx8081() );


			if (zx8081_vsync_sound.v==1) {
				//solo resetea contador de silencio cuando esta activo el vsync sound - beeper
				silence_detection_counter=0;
				beeper_silence_detection_counter=0;
			}


			video_zx8081_ula_video_output=255;


			//ejecutado_zona_pantalla.v=0;

		}

                //Teclado

		//probamos a enviar lo mismo que con teclado de spectrum
		valor=lee_puerto_teclado(puerto_h);

/*
  Bit  Expl.
  0-4  Keyboard column bits (0=Pressed)
  5    Not used             (1)
  6    Display Refresh Rate (0=60Hz, 1=50Hz)
  7    Cassette input       (0=Normal, 1=Pulse)
*/
		//decimos que no hay pulso de carga. 50 Hz refresco

		valor = (valor & 31) | (32+64);

		//valor &= (255-128);
		

		//decimos que hay pulso de carga, alternado
		//if (reg_b!=0) temp_cinta_zx81=128;
		//else temp_cinta_zx81=0;
		//valor |=temp_cinta_zx81;

		//printf ("valor: %d\n",valor);


                if (realtape_inserted.v && realtape_playing.v) {
			if (realtape_last_value>=realtape_volumen) {
                                valor=valor|128;
                                //printf ("1 ");
                        }
                        else {
                                valor=(valor & (255-128));
                                //printf ("0 ");
                        }
                }


		return valor;


	}

	


	//chroma 81
	if (puerto_l==0xef && puerto_h==0x7f) {

		//autoactivar
		if (chroma81.v==0 && autodetect_chroma81.v) {
			debug_printf (VERBOSE_INFO,"Autoenabling Chroma81");
			enable_chroma81();
		}

		if (chroma81.v) {
			//printf ("in port chroma 81\n");
			//bit 5: 0=Colour modes available
			return 0;
		}
	}	




	//Cualquier otro puerto
	//debug_printf (VERBOSE_DEBUG,"In Port %x unknown asked, PC after=0x%x",puerto_l+256*puerto_h,reg_pc);
	return 255;
}

z80_byte lee_puerto_ace_no_time(z80_byte puerto_h,z80_byte puerto_l)
{


        //Puerto ULA, cualquier puerto par
        
        if ((puerto_l & 1)==0) {

		//Any read from this port toggles the speaker "off".
		                        //y ponemos a low la salida del altavoz
                        bit_salida_sonido_ace.v=0;

                        set_value_beeper_on_array(da_amplitud_speaker_ace() );

				//No alteramos detector silencio aqui. Se hace en out port, dado que
				//la funcion de lectura se llama siempre (al leer teclado) y provocaria que nunca
				//se activase el silencio
                                //silence_detection_counter=0;
                                //beeper_silence_detection_counter=0;

                z80_byte valor;
                valor=lee_puerto_teclado(puerto_h);

		valor = (valor & 31);

/*
Port FEh Read (or any Read with A0=0)
  0-4  Keyboard Bits
  5    Cassette Input (EAR/LOAD)
  6-7  Not used
*/
                


                if (realtape_inserted.v && realtape_playing.v) {
                        if (realtape_last_value>=realtape_volumen) {
                                valor=valor|32;
                                //printf ("1 ");
                        }
                        else {
                                valor=(valor & (255-32));
                                //printf ("0 ");
                        }
                }

		return valor;

        }

	//Soundbox
        if (puerto_l==0xFF) {
                        activa_ay_chip_si_conviene();
                        if (ay_chip_present.v==1) return in_port_ay(0xFF);
        }



        //debug_printf (VERBOSE_DEBUG,"In Port %x unknown asked, PC after=0x%x",puerto_l+256*puerto_h,reg_pc);
        return 255;


}

z80_byte lee_puerto_ace(z80_byte puerto_h,z80_byte puerto_l)
{
  z80_int port=value_8_to_16(puerto_h,puerto_l);
  ula_contend_port_early( port );
  ula_contend_port_late( port );
  z80_byte valor = lee_puerto_ace_no_time( puerto_h, puerto_l );

  t_estados++;

  return valor;

}


z80_byte lee_puerto_zx81(z80_byte puerto_h,z80_byte puerto_l)
{

	return lee_puerto_zx80(puerto_h,puerto_l);
}


z80_byte lee_puerto_zx80(z80_byte puerto_h,z80_byte puerto_l)
{
  z80_int port=value_8_to_16(puerto_h,puerto_l);
  ula_contend_port_early( port );
  ula_contend_port_late( port );
  z80_byte valor = lee_puerto_zx80_no_time( puerto_h, puerto_l );

  t_estados++;

  return valor;

}

z80_byte envia_load_pp_spectrum(z80_byte puerto_h);

z80_byte envia_jload_pp_spectrum(z80_byte puerto_h)
{

//#define DURA_TECLA 20
//#define DURA_SILENCIO 15
#define DURA_TECLA 30
#define DURA_SILENCIO 22

#define SEQUENCE_ENTER1_MIN DURA_SILENCIO
#define SEQUENCE_ENTER1_MAX SEQUENCE_ENTER1_MIN+DURA_TECLA

#define SEQUENCE_J_MIN SEQUENCE_ENTER1_MAX+DURA_SILENCIO
#define SEQUENCE_J_MAX SEQUENCE_J_MIN+DURA_TECLA

#define SEQUENCE_SYM_MIN SEQUENCE_J_MAX+DURA_SILENCIO

#define SEQUENCE_P1_MIN SEQUENCE_SYM_MIN+DURA_SILENCIO
#define SEQUENCE_P1_MAX SEQUENCE_P1_MIN+DURA_TECLA

#define SEQUENCE_P2_MIN SEQUENCE_P1_MAX+DURA_SILENCIO*2
#define SEQUENCE_P2_MAX SEQUENCE_P2_MIN+DURA_TECLA

#define SEQUENCE_SYM_MAX SEQUENCE_P2_MAX+DURA_SILENCIO

#define SEQUENCE_ENTER2_MIN SEQUENCE_SYM_MAX+DURA_SILENCIO
#define SEQUENCE_ENTER2_MAX SEQUENCE_ENTER2_MIN+DURA_TECLA

			if (autoload_spectrum_loadpp_mode==2) {
				//L O A D "" para spectrum 128k spanish
				return envia_load_pp_spectrum(puerto_h);
				
			}


                        if (initial_tap_sequence>SEQUENCE_ENTER1_MIN && initial_tap_sequence<SEQUENCE_ENTER1_MAX && puerto_h==191)  {
                                return 255-1; //ENTER
                        }

			//Si es modo 128k (solo enter) no enviar todo el load pp, solo el primer enter
			if (initial_tap_sequence>SEQUENCE_J_MIN && autoload_spectrum_loadpp_mode==0) {
				initial_tap_load.v=0;
				return 255;
			}


                        if (initial_tap_sequence>SEQUENCE_J_MIN && initial_tap_sequence<SEQUENCE_J_MAX && puerto_h==191)  {
                                return 255-8; //J
                        }

                        if (initial_tap_sequence>SEQUENCE_SYM_MIN && initial_tap_sequence<SEQUENCE_SYM_MAX && puerto_h==127) {
                                return 255-2; //sym
                        }

                        if (initial_tap_sequence>SEQUENCE_P1_MIN && initial_tap_sequence<SEQUENCE_P1_MAX && puerto_h==223) {
                                return 255-1; //P
                        }

                        if (initial_tap_sequence>SEQUENCE_P2_MIN && initial_tap_sequence<SEQUENCE_P2_MAX && puerto_h==223) {
                                return 255-1; //P
                        }

                        if (initial_tap_sequence>SEQUENCE_ENTER2_MIN && initial_tap_sequence<SEQUENCE_ENTER2_MAX && puerto_h==191)  {
                                return 255-1; //ENTER
                        }


                        if (initial_tap_sequence<SEQUENCE_ENTER2_MAX) initial_tap_sequence++;
			else initial_tap_load.v=0;

			return 255;
/*
//puerto_57342    db              255  ; Y    U    I    O    P     ;5
//puerto_49150    db              255  ; H    J    K    L    Enter ;6
//puerto_32766    db              255  ; B    N    M    Simb Space ;7
*/


//0=nada
//1 esperando a llegar a main-1 (48k) -> 0x12a9 y se enviara J
//10 se dejara pulsado sym. se enviara p
//20 se enviara p
//30 se libera sym
//40 se enviara ENTER


}


//Enviar L O A D " " para spectrum 128k spanish
z80_byte envia_load_pp_spectrum(z80_byte puerto_h)
{

//#define DURA2_TECLA 20
//#define DURA2_SILENCIO 15
#define DURA2_TECLA 30
#define DURA2_SILENCIO 22

#define SEQUENCE2_ENTER1_MIN DURA2_SILENCIO
#define SEQUENCE2_ENTER1_MAX SEQUENCE2_ENTER1_MIN+DURA2_TECLA*2

#define SEQUENCE2_L_MIN SEQUENCE2_ENTER1_MAX+DURA2_SILENCIO
#define SEQUENCE2_L_MAX SEQUENCE2_L_MIN+DURA2_TECLA

#define SEQUENCE2_O_MIN SEQUENCE2_L_MAX+DURA2_SILENCIO
#define SEQUENCE2_O_MAX SEQUENCE2_O_MIN+DURA2_TECLA

#define SEQUENCE2_A_MIN SEQUENCE2_O_MAX+DURA2_SILENCIO
#define SEQUENCE2_A_MAX SEQUENCE2_A_MIN+DURA2_TECLA

#define SEQUENCE2_D_MIN SEQUENCE2_A_MAX+DURA2_SILENCIO
#define SEQUENCE2_D_MAX SEQUENCE2_D_MIN+DURA2_TECLA


#define SEQUENCE2_SYM_MIN SEQUENCE2_D_MAX+DURA2_SILENCIO

#define SEQUENCE2_P1_MIN SEQUENCE2_SYM_MIN+DURA2_SILENCIO
#define SEQUENCE2_P1_MAX SEQUENCE2_P1_MIN+DURA2_TECLA

#define SEQUENCE2_P2_MIN SEQUENCE2_P1_MAX+DURA2_SILENCIO*2
#define SEQUENCE2_P2_MAX SEQUENCE2_P2_MIN+DURA2_TECLA

#define SEQUENCE2_SYM_MAX SEQUENCE2_P2_MAX+DURA2_SILENCIO

#define SEQUENCE2_ENTER2_MIN SEQUENCE2_SYM_MAX+DURA2_SILENCIO
#define SEQUENCE2_ENTER2_MAX SEQUENCE2_ENTER2_MIN+DURA2_TECLA



                        if (initial_tap_sequence>SEQUENCE2_ENTER1_MIN && initial_tap_sequence<SEQUENCE2_ENTER1_MAX && puerto_h==191)  {
                                return 255-1; //ENTER
                        }


                        if (initial_tap_sequence>SEQUENCE2_L_MIN && initial_tap_sequence<SEQUENCE2_L_MAX && puerto_h==191)  {
                                return 255-2; //L
                        }

                        if (initial_tap_sequence>SEQUENCE2_O_MIN && initial_tap_sequence<SEQUENCE2_O_MAX && puerto_h==223)  {
                                return 255-2; //O
                        }

                        if (initial_tap_sequence>SEQUENCE2_A_MIN && initial_tap_sequence<SEQUENCE2_A_MAX && puerto_h==253)  {
                                return 255-1; //A
                        }

                        if (initial_tap_sequence>SEQUENCE2_D_MIN && initial_tap_sequence<SEQUENCE2_D_MAX && puerto_h==253)  {
                                return 255-4; //D
                        }

                        if (initial_tap_sequence>SEQUENCE2_SYM_MIN && initial_tap_sequence<SEQUENCE2_SYM_MAX && puerto_h==127) {
                                return 255-2; //sym
                        }

                        if (initial_tap_sequence>SEQUENCE2_P1_MIN && initial_tap_sequence<SEQUENCE2_P1_MAX && puerto_h==223) {
                                return 255-1; //P
                        }

                        if (initial_tap_sequence>SEQUENCE2_P2_MIN && initial_tap_sequence<SEQUENCE2_P2_MAX && puerto_h==223) {
                                return 255-1; //P
                        }

                        if (initial_tap_sequence>SEQUENCE2_ENTER2_MIN && initial_tap_sequence<SEQUENCE2_ENTER2_MAX && puerto_h==191)  {
                                return 255-1; //ENTER
                        }


                        if (initial_tap_sequence<SEQUENCE2_ENTER2_MAX) initial_tap_sequence++;
                        else initial_tap_load.v=0;

                        return 255;
/*
//puerto_57342    db              255  ; Y    U    I    O    P     ;5
//puerto_49150    db              255  ; H    J    K    L    Enter ;6
//puerto_32766    db              255  ; B    N    M    Simb Space ;7
//puerto_65022    db              255  ; G    F    D    S    A     ;1
//puerto_57342    db              255  ; Y    U    I    O    P     ;5
//puerto_49150    db              255  ; H    J    K    L    Enter ;6

*/


}



z80_byte envia_load_pp_zx80(z80_byte puerto_h)
{

#define DURA_ZX80_TECLA 60 
#define DURA_ZX80_SILENCIO 44

#define SEQUENCE_ZX80_ENTER1_MIN DURA_ZX80_SILENCIO
#define SEQUENCE_ZX80_ENTER1_MAX SEQUENCE_ZX80_ENTER1_MIN+DURA_ZX80_TECLA

#define SEQUENCE_ZX80_W_MIN SEQUENCE_ZX80_ENTER1_MAX+DURA_ZX80_SILENCIO
#define SEQUENCE_ZX80_W_MAX SEQUENCE_ZX80_W_MIN+DURA_ZX80_TECLA


#define SEQUENCE_ZX80_ENTER2_MIN SEQUENCE_ZX80_W_MAX+DURA_ZX80_SILENCIO
#define SEQUENCE_ZX80_ENTER2_MAX SEQUENCE_ZX80_ENTER2_MIN+DURA_ZX80_TECLA
                        
                        //printf ("initial_tap_sequence zx80: %d\n",initial_tap_sequence);
                        
                        if (initial_tap_sequence>SEQUENCE_ZX80_ENTER1_MIN && initial_tap_sequence<SEQUENCE_ZX80_ENTER1_MAX && puerto_h==191)  {
                                //no enviar enter inicial como en spectrum
                                //return 255-1; //ENTER
                        }
                        
                        if (initial_tap_sequence>SEQUENCE_ZX80_W_MIN && initial_tap_sequence<SEQUENCE_ZX80_W_MAX && puerto_h==251)  {
                                return 255-2; //W
                        }
                        
                        
                        if (initial_tap_sequence>SEQUENCE_ZX80_ENTER2_MIN && initial_tap_sequence<SEQUENCE_ZX80_ENTER2_MAX && puerto_h==191)  {
                                return 255-1; //ENTER
                        }       
                        

                        if (initial_tap_sequence<SEQUENCE_ZX80_ENTER2_MAX) initial_tap_sequence++;
                        else initial_tap_load.v=0;
                        
                        return 255;
/*                      
//puerto_57342    db              255  ; Y    U    I    O    P     ;5
//puerto_49150    db              255  ; H    J    K    L    Enter ;6
//puerto_32766    db              255  ; B    N    M    Simb Space ;7
//puerto_65278    db              255  ; V    C    X    Z    Sh    ;0

//puerto_64510    db              255  ; T    R    E    W    Q     ;2

*/


}


z80_byte envia_load_pp_zx81(z80_byte puerto_h)
{

#define DURA_ZX81_TECLA 60
#define DURA_ZX81_SILENCIO 44

#define SEQUENCE_ZX81_ENTER1_MIN DURA_ZX81_SILENCIO
#define SEQUENCE_ZX81_ENTER1_MAX SEQUENCE_ZX81_ENTER1_MIN+DURA_ZX81_TECLA

#define SEQUENCE_ZX81_J_MIN SEQUENCE_ZX81_ENTER1_MAX+DURA_ZX81_SILENCIO
#define SEQUENCE_ZX81_J_MAX SEQUENCE_ZX81_J_MIN+DURA_ZX81_TECLA

#define SEQUENCE_ZX81_SYM_MIN SEQUENCE_ZX81_J_MAX+DURA_ZX81_SILENCIO

#define SEQUENCE_ZX81_P1_MIN SEQUENCE_ZX81_SYM_MIN+DURA_ZX81_SILENCIO
#define SEQUENCE_ZX81_P1_MAX SEQUENCE_ZX81_P1_MIN+DURA_ZX81_TECLA

#define SEQUENCE_ZX81_P2_MIN SEQUENCE_ZX81_P1_MAX+DURA_ZX81_SILENCIO*2
#define SEQUENCE_ZX81_P2_MAX SEQUENCE_ZX81_P2_MIN+DURA_ZX81_TECLA

#define SEQUENCE_ZX81_SYM_MAX SEQUENCE_ZX81_P2_MAX+DURA_ZX81_SILENCIO

#define SEQUENCE_ZX81_ENTER2_MIN SEQUENCE_ZX81_SYM_MAX+DURA_ZX81_SILENCIO
#define SEQUENCE_ZX81_ENTER2_MAX SEQUENCE_ZX81_ENTER2_MIN+DURA_ZX81_TECLA

                        //printf ("initial_tap_sequence zx81: %d\n",initial_tap_sequence);

                        if (initial_tap_sequence>SEQUENCE_ZX81_ENTER1_MIN && initial_tap_sequence<SEQUENCE_ZX81_ENTER1_MAX && puerto_h==191)  {
				//no enviar enter inicial como en spectrum
                                //return 255-1; //ENTER
                        }

                        if (initial_tap_sequence>SEQUENCE_ZX81_J_MIN && initial_tap_sequence<SEQUENCE_ZX81_J_MAX && puerto_h==191)  {
                                return 255-8; //J
                        }

                        if (initial_tap_sequence>SEQUENCE_ZX81_SYM_MIN && initial_tap_sequence<SEQUENCE_ZX81_SYM_MAX && puerto_h==254) {
                                return 255-1; //shift
                        }

                        if (initial_tap_sequence>SEQUENCE_ZX81_P1_MIN && initial_tap_sequence<SEQUENCE_ZX81_P1_MAX && puerto_h==223) {
                                return 255-1; //P
                        }

                        if (initial_tap_sequence>SEQUENCE_ZX81_P2_MIN && initial_tap_sequence<SEQUENCE_ZX81_P2_MAX && puerto_h==223) {
                                return 255-1; //P
                        }

                        if (initial_tap_sequence>SEQUENCE_ZX81_ENTER2_MIN && initial_tap_sequence<SEQUENCE_ZX81_ENTER2_MAX && puerto_h==191)  {
                                return 255-1; //ENTER
                        }


                        if (initial_tap_sequence<SEQUENCE_ZX81_ENTER2_MAX) initial_tap_sequence++;
                        else initial_tap_load.v=0;

                        return 255;
/*
//puerto_57342    db              255  ; Y    U    I    O    P     ;5
//puerto_49150    db              255  ; H    J    K    L    Enter ;6
//puerto_32766    db              255  ; B    N    M    Simb Space ;7
//puerto_65278    db    	  255  ; V    C    X    Z    Sh    ;0

*/


}



z80_byte lee_puerto_spectrum(z80_byte puerto_h,z80_byte puerto_l)
{
  z80_int port=value_8_to_16(puerto_h,puerto_l);
  ula_contend_port_early( port );
  ula_contend_port_late( port );
  z80_byte valor = lee_puerto_spectrum_no_time( puerto_h, puerto_l );

  t_estados++;

  return valor;

}


z80_byte idle_bus_port_atribute(void)
{

	//Retorna el byte que lee la ULA
	//de momento solo retornar el ultimo atributo, teniendo en cuenta que el rainbow debe estar habilitado
	//TODO: retornar tambien byte de pixeles
	//Si no esta habilitado rainbow, retornara algun valor aleatorio fijo


	//Si no esta habilitado rainbow y hay que detectar rainbow, habilitar rainbow, siempre que no este en ROM
	if (rainbow_enabled.v==0) {
		if (autodetect_rainbow.v) {
			if (reg_pc>=16384) {
				debug_printf(VERBOSE_INFO,"Autoenabling realvideo so the program seems to need it (Idle bus port reading on Spectrum)");
				enable_rainbow();
			}
		}

	}

	return last_ula_attribute;

}


z80_byte idle_bus_port(z80_int puerto)
{

	//debug_printf(VERBOSE_DEBUG,"Idle bus port reading: %x",puerto);

	//Caso Inves, siempre 255
	if (machine_type==2) return 255;

	//Caso 48k, 128k, zxuno. Retornar atributo de pantalla
	if (MACHINE_IS_SPECTRUM_16_48 || MACHINE_IS_SPECTRUM_128_P2 || MACHINE_IS_ZXUNO) return idle_bus_port_atribute();

	//Caso +2A
	/*
When the paging is disabled (bit 5 of port 32765 set to 1), we will
always read 255; when it is not, we will have the byte that the ULA reads
(with bit 0 set) if the port number has the following mask:
0000XXXXXXXXXX01b, I mean, starting from port 1, stepping 4, until port 4093
(1,5,9,13,..., 4093)

//valor:
0000000000000001b=1

//mascara:
1111000000000011b=61443

	*/
	if (MACHINE_IS_SPECTRUM_P2A) {
		//Paginacion deshabilitada
		if ( (puerto_32765&32) == 32) return 255;

		if ( (puerto&61443) == 1) return ( idle_bus_port_atribute() | 1 );
			
	}


	//Resto de casos o maquinas, retornamos 255
	return 255;

}

//Teclado en Spectrum y Jupiter Ace es casi igual excepto en la fila inferior de teclas
void jupiter_ace_adapta_teclas_fila_inferior(z80_byte *valor_puerto_65278,z80_byte *valor_puerto_32766)
{
//Spectrum:
//puerto_65278    db              255  ; V    C    X    Z    Sh    ;0
//puerto_32766    db              255  ; B    N    M    Simb Space ;7


//Jupiter Ace
//puerto_65278    db              255  ; C    X    Z    Simb Sh
//puerto_32766    db              255  ; V    B    N    M    Space

	z80_byte p65278,p32766;

	//Obtener cada tecla por separado en formato teclado spectrum y luego convertirlo a teclas jupiterace
	z80_byte tecla_sh,tecla_z,tecla_x,tecla_c,tecla_v,tecla_space,tecla_simb,tecla_m,tecla_n,tecla_b;

	tecla_sh=(puerto_65278&1 ? 1 :0 );
	tecla_z=(puerto_65278&2 ? 1 :0 );
	tecla_x=(puerto_65278&4 ? 1 :0 );
	tecla_c=(puerto_65278&8 ? 1 :0 );
	tecla_v=(puerto_65278&16 ? 1 :0 );

	tecla_space=(puerto_32766&1 ? 1 :0 );
	tecla_simb=(puerto_32766&2 ? 1 :0 );
	tecla_m=(puerto_32766&4 ? 1 :0 );
	tecla_n=(puerto_32766&8 ? 1 :0 );
	tecla_b=(puerto_32766&16 ? 1 :0 );

	//Pasarlas a puertos de jupiter ace
	p65278=tecla_sh | (tecla_simb<<1) | (tecla_z<<2) | (tecla_x<<3) | (tecla_c<<4);
	p32766=tecla_space | (tecla_m<<1) | (tecla_n<<2) | (tecla_b<<3) | (tecla_v<<4);


	*valor_puerto_65278=p65278;
	*valor_puerto_32766=p32766;


}

z80_byte jupiter_ace_retorna_puerto_65278(void)
{
	z80_byte a,b;

	jupiter_ace_adapta_teclas_fila_inferior(&a,&b);

	return a;
}

z80_byte jupiter_ace_retorna_puerto_32766(void)
{
        z80_byte a,b;

        jupiter_ace_adapta_teclas_fila_inferior(&a,&b);

        return b;
}

//comun para spectrum y zx80/81
z80_byte lee_puerto_teclado(z80_byte puerto_h)
{

			z80_byte acumulado;

                if (initial_tap_load.v==1 && initial_tap_sequence) {
			if (MACHINE_IS_SPECTRUM) {
	                        return envia_jload_pp_spectrum(puerto_h);
			}
			else {
				if (MACHINE_IS_ZX80) return envia_load_pp_zx80(puerto_h);
				else return envia_load_pp_zx81(puerto_h);
			}
                }

                //puerto teclado

                //si estamos en el menu, no devolver tecla
                if (menu_abierto==1) return 255;


		//Si esta spool file activo, generar siguiente tecla
		if (input_file_keyboard_inserted.v==1) {
			if (input_file_keyboard_turbo.v==0) {
				input_file_keyboard_get_key();
			}

			else {
				//en modo turbo enviamos cualquier tecla pero la rom realmente lee de la direccion 23560
				ascii_to_keyboard_port(' ');
			}
		}
		


                        acumulado=255;

                        //A zero in one of the five lowest bits means that the corresponding key is pressed. If more than one address line is made low, the result is the logical AND of all single inputs, so a zero in a bit means that at least one of the appropriate keys is pressed. For example, only if each of the five lowest bits of the result from reading from Port 00FE (for instance by XOR A/IN A,(FE)) is one, no key is pressed

                        if ((puerto_h & 1) == 0)   {
				if (MACHINE_IS_ACE) {
					acumulado &=jupiter_ace_retorna_puerto_65278();
				}
				else acumulado &=puerto_65278;

				//Si hay alguna tecla del joystick cursor pulsada, enviar tambien shift
//z80_byte puerto_65278=255; //    db    255  ; V    C    X    Z    Sh    ;0
				if (joystick_emulation==JOYSTICK_CURSOR_WITH_SHIFT && puerto_especial_joystick!=0) {
					acumulado &=(255-1);
				}
			}
                        if ((puerto_h & 2) == 0)   {
				acumulado &=puerto_65022;

                                //OPQASPACE Joystick
                                if (joystick_emulation==JOYSTICK_OPQA_SPACE) {
                                        if ((puerto_especial_joystick&4)) acumulado &=(255-1);
                                }

			}
                        if ((puerto_h & 4) == 0)   {
				acumulado &=puerto_64510;

                                //OPQASPACE Joystick
                                if (joystick_emulation==JOYSTICK_OPQA_SPACE) {
                                        if ((puerto_especial_joystick&8)) acumulado &=(255-1);
                                }

			}




//z80_byte puerto_especial_joystick=0; //Fire Up Down Left Right

			//Para cursor, sinclair joystick
//z80_byte puerto_63486=255; //    db              255  ; 5    4    3    2    1     ;3
                        if ((puerto_h & 8) == 0)   {
				acumulado &=puerto_63486;
				//sinclair 2 joystick
				if (joystick_emulation==JOYSTICK_SINCLAIR_2) {
					if ((puerto_especial_joystick&1)) acumulado &=(255-2);
					if ((puerto_especial_joystick&2)) acumulado &=(255-1);
					if ((puerto_especial_joystick&4)) acumulado &=(255-4);
					if ((puerto_especial_joystick&8)) acumulado &=(255-8);
					if ((puerto_especial_joystick&16)) acumulado &=(255-16);
				}

				//cursor joystick 5 iz 8 der 6 abajo 7 arriba 0 fire
				if (joystick_emulation==JOYSTICK_CURSOR || joystick_emulation==JOYSTICK_CURSOR_WITH_SHIFT) {
					if ((puerto_especial_joystick&2)) acumulado &=(255-16);
				}

		                //gunstick
                                if (gunstick_emulation==GUNSTICK_SINCLAIR_2) {
                                        if (mouse_left!=0) {

                                                acumulado &=(255-1);

                                                if (gunstick_view_white()) acumulado &=(255-4);


                                        }
                                }

			}

//z80_byte puerto_61438=255; //    db              255  ; 6    7    8    9    0     ;4
                        if ((puerto_h & 16) == 0)  {
				acumulado &=puerto_61438;

				//sinclair 1 joystick
				if (joystick_emulation==JOYSTICK_SINCLAIR_1) {
					if ((puerto_especial_joystick&1)) acumulado &=(255-8);
					if ((puerto_especial_joystick&2)) acumulado &=(255-16);
					if ((puerto_especial_joystick&4)) acumulado &=(255-4);
					if ((puerto_especial_joystick&8)) acumulado &=(255-2);
					if ((puerto_especial_joystick&16)) acumulado &=(255-1);
				}
				//cursor joystick 5 iz 8 der 6 abajo 7 arriba 0 fire
                                if (joystick_emulation==JOYSTICK_CURSOR  || joystick_emulation==JOYSTICK_CURSOR_WITH_SHIFT) {
                                        if ((puerto_especial_joystick&1)) acumulado &=(255-4);
                                        if ((puerto_especial_joystick&4)) acumulado &=(255-16);
                                        if ((puerto_especial_joystick&8)) acumulado &=(255-8);
                                        if ((puerto_especial_joystick&16)) acumulado &=(255-1);
                                } 


				//gunstick
				if (gunstick_emulation==GUNSTICK_SINCLAIR_1) {
					if (mouse_left!=0) {

						acumulado &=(255-1);

						if (gunstick_view_white()) acumulado &=(255-4);

						
					}
				}
			}

                        if ((puerto_h & 32) == 0)  {
				acumulado &=puerto_57342;

                                //OPQASPACE Joystick
                                if (joystick_emulation==JOYSTICK_OPQA_SPACE) {
                                        if ((puerto_especial_joystick&1)) acumulado &=(255-1);
                                        if ((puerto_especial_joystick&2)) acumulado &=(255-2);
                                }


			}
                        if ((puerto_h & 64) == 0)  acumulado &=puerto_49150;
                        if ((puerto_h & 128) == 0) {
				if (MACHINE_IS_ACE) {
					acumulado &=jupiter_ace_retorna_puerto_32766();
				}
				else acumulado &=puerto_32766;

				//OPQASPACE Joystick
				if (joystick_emulation==JOYSTICK_OPQA_SPACE) {
					if ((puerto_especial_joystick&16)) acumulado &=(255-1);	
				}

			}

                        return acumulado;


}


//Devuelve valor para el puerto de kempston (joystick o gunstick kempston)
z80_byte get_kempston_value(void)
{
                        
                        z80_byte acumulado=0;
                        
                        //si estamos con menu abierto, no retornar nada
                        if (menu_abierto==1) return 0;
                        
                        if (joystick_emulation==JOYSTICK_KEMPSTON) {
                                //mapeo de ese puerto especial es igual que kempston
                                acumulado=puerto_especial_joystick;
                        }
                       
                       //gunstick
                       if (gunstick_emulation==GUNSTICK_KEMPSTON) {
                                
                                if (mouse_left!=0) {
                                    
                                    acumulado |=16;
                                    
                                    if (gunstick_view_white()) acumulado |=4;
                                
                                }
                        }
                        
                        return acumulado;
}

z80_byte lee_puerto_spectrum_ula(z80_byte puerto_h)
{

                z80_byte valor;
                valor=lee_puerto_teclado(puerto_h);
                //teclado issue 2 o 3

                //issue 3
                if (keyboard_issue2.v==0) valor=(valor & (255-64));

                //issue 2
                else valor=valor|64;


                if (realtape_inserted.v && realtape_playing.v) {
                        if (realtape_last_value>=realtape_volumen) {
                                valor=valor|64;
                                //printf ("1 ");
                        }
                        else {
                                valor=(valor & (255-64));
                                //printf ("0 ");
                        }
                }

                        //Miramos si estamos en una rutina de carga de cinta
                        //Y esta cinta standard

                        if (standard_to_real_tape_fallback.v) {

                                //Por defecto detectar real tape
                                int detectar=1;

                                //Si ya hay una cinta insertada y en play, no hacerlo
                                if (realtape_inserted.v && realtape_playing.v) detectar=0;

                                //Si no hay cinta standard, no hacerlo
                                if (tapefile==0) detectar=0;

                                //Si no esta cinta standard insertada, no hacerlo
                                if ((tape_loadsave_inserted & TAPE_LOAD_INSERTED)==0) detectar=0;

                                if (detectar) tape_detectar_realtape();

                        }

                return valor;


}

//Devuelve valor puerto para maquinas Spectrum
z80_byte lee_puerto_spectrum_no_time(z80_byte puerto_h,z80_byte puerto_l)
{

	//extern z80_byte in_port_ay(z80_int puerto);
	//65533 o 49149
	//FFFDh (65533), BFFDh (49149)

	z80_int puerto=value_8_to_16(puerto_h,puerto_l);
	
	if (puerto_l==0xFD) {
		if (puerto_h==0xFF) {
			activa_ay_chip_si_conviene();
			if (ay_chip_present.v==1) return in_port_ay(puerto_h);
		}
		
		if (puerto_h==0x2F) {
			//Puerto Disco +3
			debug_printf (VERBOSE_DEBUG,"In port 0x2ffd - +3 Disk. Not implemented yet");
			return 255;
		}
		
	}

	//Fuller audio box. Interfiere con MMC
	if (zxmmc_emulation.v==0 && (puerto_l==0x3f && puerto_h==0)) {

		activa_ay_chip_si_conviene();
		if (ay_chip_present.v==1) {

			return in_port_ay(0xFF);

		}

	}
	
	
	//Fuller Joystick
	//The Fuller Audio Box included a joystick interface. Results were obtained by reading from port 0x7f in the form F---RLDU, with active bits low.
        if (puerto_l==0x7f) {
                if (joystick_emulation==JOYSTICK_FULLER) {
			z80_byte valor_joystick=255;
			//si estamos con menu abierto, no retornar nada
			if (menu_abierto==1) return valor_joystick;


			if ((puerto_especial_joystick&1)) valor_joystick &=(255-8);
			if ((puerto_especial_joystick&2)) valor_joystick &=(255-4);
			if ((puerto_especial_joystick&4)) valor_joystick &=(255-2);
			if ((puerto_especial_joystick&8)) valor_joystick &=(255-1);
			if ((puerto_especial_joystick&16)) valor_joystick &=(255-128);

                        return valor_joystick;
                }

        }

	//zx printer
	if (puerto_l==0xFB) {
                if (zxprinter_enabled.v==1) {
			return zxprinter_get_port();
                }
		

        }

	//ulaplus
	if (ulaplus_presente.v && puerto_l==0x3B && puerto_h==0xFF) {
		return ulaplus_last_send_FF3B;
	}


	//Puerto DF parece leerse desde juego Bestial Warrior en el menu, aunque dentro del juego no se lee
	//Quiza hace referencia a la Stack Light Rifle

	//Dejarlo comentado dado que entra en conflicto con kempston mouse

	/*

	if ( puerto_l==0xdf) {
		//gunstick. 
//Stack Light Rifle (Stack Computer Services Ltd) (1983)
//connects to expansion port, accessed by reading from Port DFh:

//  bit1       = Trigger Button (0=Pressed, 1=Released)
//  bit4       = Light Sensor   (0=Light, 1=No Light)
//  other bits = Must be "1"    (the programs use compare FDh to test if bit1=0)

               if (gunstick_emulation==GUNSTICK_PORT_DF) {

                        z80_byte acumulado=255;
                        if (mouse_left!=0) {

                            acumulado &=(255-2);

                            if (gunstick_view_white()) acumulado &=(255-16);


                        }

			printf ("gunstick acumulado: %d\n",acumulado);

                        return acumulado;
                }

		//puerto kempston tambien es DF en kempston mouse
		//else return idle_bus_port(puerto_l+256*puerto_h);

	}

	*/

	//kempston mouse
	if ( kempston_mouse_emulation.v  &&  (puerto_l&32) == 0  &&  ( (puerto_h&7)==3 || (puerto_h&7)==7 || (puerto_h&2)==2 ) ) {
		//printf ("kempston mouse. port 0x%x%x\n",puerto_h,puerto_l);

//IN 64479 - return X axis (0-255)
//IN 65503 - return Y axis (0-255)
//IN 64223 - return button status
//D0 - right button
//D1 - left button
//D2-D7 - not used

//From diagram ports

//X-Axis  = port 64479 xxxxx011 xx0xxxxx
//Y-Axis  = port 65503 xxxxx111 xx0xxxxx
//BUTTONS = port 64223 xxxxxx10 xx0xxxxx

		z80_byte acumulado=0;

		if ((puerto_h&7)==3) {
			//X-Axis
			acumulado=kempston_mouse_x;
		}

                if ((puerto_h&7)==7) {
                        //Y-Axis
			acumulado=kempston_mouse_y;
                }

                if ((puerto_h&3)==2) {
                        //Buttons
			acumulado=255;

			//left button
			if (mouse_left) acumulado &=(255-2);
                        //right button
                        if (mouse_right) acumulado &=(255-1);
                }

		//printf ("devolvemos valor: %d\n",acumulado);	
		return acumulado;
		
	}

	//If you read from a port that activates both the keyboard and a joystick port (e.g. Kempston), the joystick takes priority.

        //kempston joystick en Inves
	//En Inves, Si A5=0 solamente
        if ( (puerto_l & 32) ==0 && (MACHINE_IS_INVES) ) {

                if (joystick_emulation==JOYSTICK_KEMPSTON || gunstick_emulation==GUNSTICK_KEMPSTON) {
			return get_kempston_value();
                }

        }

        //kempston joystick para maquinas no Inves
        //Si A5=A6=A7=0 y A0=1, kempston joystick
        if ( (puerto_l & (1+32+64+128)) == 1 && !(MACHINE_IS_INVES) ) {

                if (joystick_emulation==JOYSTICK_KEMPSTON || gunstick_emulation==GUNSTICK_KEMPSTON) {
			return get_kempston_value();
                }

        }

        //ZXUNO
        if ( MACHINE_IS_ZXUNO && (puerto==0xFC3B  || puerto==0xFD3B)) {
		return zxuno_read_port(puerto);
        }


        //Puertos ZXMMC. Interfiere con Fuller Audio Box
        if (zxmmc_emulation.v && (puerto_l==0x1f || puerto_l==0x3f) ) {
              //printf ("Puerto ZXMMC Read: 0x%02x\n",puerto_l);
		if (puerto_l==0x3f) {
			z80_byte valor_leido=mmc_read();
			//printf ("Valor leido: %d\n",valor_leido);
			return valor_leido;
		}

		return 255;

        }


	//Puertos DIVMMC
	if (divmmc_enabled.v && (puerto_l==0xe7 || puerto_l==0xeb) ) {
		//printf ("Puerto DIVMMC Read: 0x%02x\n",puerto_l);

	        //Si en ZXUNO y DIVEN desactivado.
		//Aunque cuando se toca el bit DIVEN de ZX-Uno se sincroniza divmmc_enable,
		//pero por si acaso... por si se activa manualmente desde menu del emulador
		//el divmmc pero zxuno espera que este deshabilitado, como en la bios
	        if (MACHINE_IS_ZXUNO_DIVEN_DISABLED) return 255;

                if (puerto_l==0xeb) {
                        z80_byte valor_leido=mmc_read();
                        //printf ("Valor leido: %d\n",valor_leido);
                        return valor_leido;
                }

                return 255;

        }

	//Puerto Spectra
	if (spectra_enabled.v && puerto==0x7FDF) return spectra_read();

	//Sprite Chip
	if (spritechip_enabled.v && (puerto==SPRITECHIP_COMMAND_PORT || puerto==SPRITECHIP_DATA_PORT) ) return spritechip_read(puerto);




        //Puerto ULA, cualquier puerto par. En un Spectrum normal, esto va al final
	//En un Inves, deberia ir al principio, pues el inves hace un AND con el valor de los perifericos que retornan valor en el puerto leido
        if ( (puerto_l & 1)==0 && !(MACHINE_IS_CHLOE) && !(MACHINE_IS_TIMEX_TS2068)	) {

		return lee_puerto_spectrum_ula(puerto_h);
        
        }


        //Puerto 254 solamente, para chloe y timex
        if ( puerto_l==254 && (MACHINE_IS_CHLOE || MACHINE_IS_TIMEX_TS2068) ) {
		return lee_puerto_spectrum_ula(puerto_h);
        }




        //Puerto Timex Video. 8 bit bajo a ff
        if (timex_video_emulation.v && puerto_l==0xFF) {
		return timex_video_last_out;
        }

	//Puerto Timex Paginacion
        if (puerto_l==0xf4 && (MACHINE_IS_CHLOE || MACHINE_IS_TIMEX_TS2068) ) {
		return timex_port_f4;

        }


	//Puerto Chip AY para Timex y Chloe
        if (puerto_l==0xF6 && (MACHINE_IS_CHLOE || MACHINE_IS_TIMEX_TS2068) ) {
                        activa_ay_chip_si_conviene();
                        if (ay_chip_present.v==1) return in_port_ay(0xFF);
	}


	//debug_printf (VERBOSE_DEBUG,"In Port %x unknown asked, PC after=0x%x",puerto_l+256*puerto_h,reg_pc);
	return idle_bus_port(puerto_l+256*puerto_h);

}





void cpi_cpd_common(void)
{

	z80_byte antes,result;
 
	antes=reg_a;
        result=reg_a-peek_byte(HL);

        contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
        contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
        contend_read_no_mreq( HL, 1 );



        set_undocumented_flags_bits(result);
        set_flags_zero_sign(result);
	set_flags_halfcarry_resta(antes,result);
        Z80_FLAGS |=FLAG_N;


        BC--;

        if (!BC) Z80_FLAGS &=(255-FLAG_PV);
        else Z80_FLAGS |=FLAG_PV;

	if ((Z80_FLAGS & FLAG_H)) result--;

        if (result & 8 ) Z80_FLAGS |=FLAG_3;
        else Z80_FLAGS &=(255-FLAG_3);

        if (result & 2 ) Z80_FLAGS |=FLAG_5;
        else Z80_FLAGS &=(255-FLAG_5);


}

void out_port_ace_no_time(z80_int puerto,z80_byte value)
{
	//de momento nada

        z80_byte puerto_l=puerto&0xFF;
        //z80_byte puerto_h=(puerto>>8)&0xFF;

       //Puerto ULA, cualquier puerto par

        if ((puerto_l & 1)==0) {

		//Any write to this port toggles the speaker "on".
                                        //y ponemos a high la salida del altavoz
                        bit_salida_sonido_ace.v=1;

                        set_value_beeper_on_array(da_amplitud_speaker_ace() );

                                silence_detection_counter=0;
                                beeper_silence_detection_counter=0;


			//bit_salida_sonido_ace.v=value&1;
			//set_value_beeper_on_array(da_amplitud_speaker_ace() );
			//printf ("Out port ACE ula value: %d\n",bit_salida_sonido_ace.v);


	}


        //Soundbox 
        if (puerto_l==0xFD) {
                        activa_ay_chip_si_conviene();
                        if (ay_chip_present.v==1) out_port_ay(65533,value);
                }

        if (puerto_l==0xFF) {
                        activa_ay_chip_si_conviene();
                        if (ay_chip_present.v==1) out_port_ay(49149,value);
        }

	
}


void out_port_ace(z80_int puerto,z80_byte value)
{
  ula_contend_port_early( puerto );
  out_port_ace_no_time(puerto,value);
  ula_contend_port_late( puerto ); t_estados++;
}

void out_port_zx80_no_time(z80_int puerto,z80_byte value)
{

	//Esto solo sirve para mostrar en menu debug i/o ports
	zx8081_last_port_write_value=value;

	z80_byte puerto_l=puerto&0xFF;
	z80_byte puerto_h=(puerto>>8)&0xFF;

	//Bi-Pak ZON-X81 Sound
		if (puerto_l==0xDF || puerto_l==0xCF) {
			activa_ay_chip_si_conviene();
			if (ay_chip_present.v==1) out_port_ay(65533,value);
		}

		if (puerto_l==0x0F || puerto_l==0x1F) {
			activa_ay_chip_si_conviene();
			if (ay_chip_present.v==1) out_port_ay(49149,value);
		}



        //zx printer
        if (puerto_l==0xFB) {
                if (zxprinter_enabled.v==1) {
                        zxprinter_write_port(value);

                }
        }

	//chroma
	if (puerto==0x7FEF && chroma81.v) {
		chroma81_port_7FEF=value;
		//if (chroma81_port_7FEF&16) debug_printf (VERBOSE_INFO,"Chroma 81 mode 1");
		//else debug_printf (VERBOSE_INFO,"Chroma 81 mode 0");
		debug_printf (VERBOSE_INFO,"Setting Chroma 81 parameters: Border: %d, Mode: %s, Enable: %s",
			chroma81_port_7FEF&15,(chroma81_port_7FEF&16 ? "1 - Attribute file" : "0 - Character code"),
			(chroma81_port_7FEF&32 ? "Yes" : "No") );
	}


        //zxpand
        if (puerto_l==0x07 && zxpand_enabled.v) {
		//z80_byte letra=value;
		//if (letra>=128) letra -=128;
		//if (letra<64) letra=da_codigo_zx81_no_artistic(letra);
		//else letra='?';
		
                //printf ("Out Port ZXpand 0x%X value : 0x%0x (%c), PC after=0x%x\n",puerto,value,letra,reg_pc);
		zxpand_write(puerto_h,value);
        }



	//Cualquier puerto generara vsync


	//printf ("Sending vsync with hsync_generator_active : %d video_zx8081_ula_video_output: %d\n",hsync_generator_active.v,video_zx8081_ula_video_output);


        //printf("Enabling the HSYNC generator t_scanline_draw=%d\n",t_scanline_draw);

        hsync_generator_active.v=1;
        modificado_border.v=1;


	//reseteamos contador de deteccion de modo fast-pantalla negra. Para modo no-realvideo
	video_fast_mode_next_frame_black=0;


	//y ponemos a high la salida del altavoz
	bit_salida_sonido_zx8081.v=1;

	set_value_beeper_on_array(da_amplitud_speaker_zx8081() );


	if (zx8081_vsync_sound.v==1) {
		//solo resetea contador de silencio cuando esta activo el vsync sound - beeper
		silence_detection_counter=0;
		beeper_silence_detection_counter=0;
	}



        //Calcular cuanto ha tardado el vsync
        int longitud_pulso_vsync;

        if (t_estados>inicio_pulso_vsync_t_estados) longitud_pulso_vsync=t_estados-inicio_pulso_vsync_t_estados;

        //contador de t_estados ha dado la vuelta. estamos al reves
        else longitud_pulso_vsync=screen_testados_total-inicio_pulso_vsync_t_estados+t_estados;

        //printf ("escribe puerto. final vsync  t_estados=%d. diferencia: %d t_scanline_draw: %d t_scanline_draw_timeout: %d\n",t_estados,longitud_pulso_vsync,t_scanline_draw,t_scanline_draw_timeout);


	if (video_zx8081_linecntr_enabled.v==0) {
        	printf ("escribe puerto. final vsync  t_estados=%d. diferencia: %d t_scanline_draw: %d t_scanline_draw_timeout: %d\n",t_estados,longitud_pulso_vsync,t_scanline_draw,t_scanline_draw_timeout);
		if (longitud_pulso_vsync >= minimo_duracion_vsync) {
			//if (t_scanline_draw_timeout>MINIMA_LINEA_ADMITIDO_VSYNC || t_scanline_draw_timeout<=3) {
			if (t_scanline_draw_timeout>MINIMA_LINEA_ADMITIDO_VSYNC) {
				printf ("admitido final pulso vsync\n");

		                if (!simulate_lost_vsync.v) {



					if (zx8081_detect_vsync_sound.v) {
						//printf ("vsync total de zx8081 t_estados: %d\n",t_estados);
						if (zx8081_detect_vsync_sound_counter>0) zx8081_detect_vsync_sound_counter--;

					}

					generar_zx8081_vsync();
					vsync_per_second++;
				}


			}
		}

		else {
			printf ("no admitimos pulso vsync, duracion: %d\n",longitud_pulso_vsync);
		}

	}


	video_zx8081_linecntr_enabled.v=1;

	//modo slow ?¿
	/*
	if (nmi_generator_active.v==1) {
		video_zx8081_lnctr_adjust.v=1;
	}
	else {
		video_zx8081_lnctr_adjust.v=0;
	}
	*/

 	video_zx8081_ula_video_output=0;

	//Prueba para no tener que usar ajuste lnctr
	//con esto: modo zx80 lnctr desactivado y zx81 activado y todos juegos se ven bien. PERO modo fast zx81 se ve mal
	//video_zx8081_linecntr=0;


	//prueba if (nmi_generator_active.v==1) video_zx8081_linecntr=0;


}
void out_port_zx81_no_time(z80_int puerto,z80_byte value)
{


	if ((puerto&0xFF)==0xfd) {
		//debug_printf (VERBOSE_DEBUG,"Disabling NMI generator\n");
		nmi_generator_active.v=0;
		//return;
	}

        if ((puerto&0xFF)==0xfe) {
		//debug_printf (VERBOSE_DEBUG,"Enabling NMI generator\n");
                nmi_generator_active.v=1;
		//return;
        }


	//TODO
	//Con el luego wrx/nucinv16.p esto NO da imagen estable
	//para imagen estable, los if de antes no deben finalizar con return, es decir,
	//se debe activar/desactivar nmi y luego lanzar la sentencia out_port_zx80_no_time(puerto,value);
	//para wrx/nucinv16.p se hace el truco de bajar el timeout de vsync
	out_port_zx80_no_time(puerto,value);



}


//Extracted from Fuse emulator
void set_value_beeper (int v)
{
  static int beeper_ampl[] = { 0, AMPLITUD_TAPE, AMPLITUD_BEEPER,
                               AMPLITUD_BEEPER+AMPLITUD_TAPE };
/*
  if( tape_is_playing() ) {
    // Timex machines have no loading noise 
    if( !settings_current.sound_load || machine_current->timex ) on = on & 0x02;
  } else {
    // ULA book says that MIC only isn't enough to drive the speaker as output voltage is below the 1.4v threshold 
    if( on == 1 ) on = 0;
  }
*/

  //Si estamos en rutina de SAVE, enviar sonido teniendo en cuenta como referencia el 0 y de ahi hacia arriba o abajo
  //siempre que el parametro de filtro lo tengamos activo en el menu

  //valores de PC en hacer out 254:
  //rutina save : 1244, 1262, 1270, 1310. oscila solo bit MIC
  //rutina load: 1374
  //rutina beep: 995
  if (reg_pc>1200 && reg_pc<1350 && output_beep_filter_on_rom_save.v) {
	//Estamos en save. 
	value_beeper=( v ? AMPLITUD_TAPE*2 : -AMPLITUD_TAPE*2);
  }

  else {


  //Por defecto el sonido se genera en negativo y de ahi oscila

  //temp normal en fuse 
  value_beeper = -beeper_ampl[3] + beeper_ampl[v]*2;

  }

  //temp prueba para que sonido en grabacion no sea negativo
  //value_beeper = beeper_ampl[v]*2;

}

z80_byte get_border_colour_from_out(void)
{
z80_byte color_border;

                                        if (spectra_enabled.v) {
                                                if (spectra_display_mode_register&16) {
                                                        //enhanced border
                                                        //spectra_display_mode_register&4 ->extra colours
                                                        if (spectra_display_mode_register&4) {
                                                                //extra colours
                                                                
                                                                color_border=
                                                                        (out_254&32  ? 1 : 0) +
                                                                        (out_254&1   ? 2 : 0) +
                                                                        (out_254&64  ? 4 : 0) +
                                                                        (out_254&2   ? 8 : 0) +
                                                                        (out_254&128 ? 16 : 0) +
                                                                        (out_254&4   ? 32 : 0) ;

                                                                

                                                        }
                                                        else {
                                                                
                                                                color_border=out_254&7;
                                                                //basic colours
                                                                //flash?
                                                                if (out_254 & 128) {
                                                                        if (estado_parpadeo.v) {
                                                                                //parpadeo a negro o blanco
                                                                                if (out_254 & 32) color_border=7;
                                                                                else color_border=0;
                                                                        }
                                                                }
                                                                //brillo?
                                                                if (out_254 & 64) color_border+=8;
                                                                
                                                        }
                                                }
                                                else {
                                                        //standard border
                                                        color_border=out_254 & 7;
                                                }
              }

                                        //no es color spectra
                                        else {
                                                color_border=out_254 & 7;
                                        }


	return color_border;
}



void out_port_spectrum_border(z80_int puerto,z80_byte value)
{

	//Guardamos temporalmente el valor anterior para compararlo con el actual
	//en el metodo de autodeteccion de border real video
	z80_byte anterior_out_254=out_254;

            modificado_border.v=1;
                silence_detection_counter=0;
		beeper_silence_detection_counter=0;

		out_254_original_value=value;

		//printf ("valor mic y ear: %d\n",value&(8+16));
		//printf ("pc: %d\n",reg_pc);

                if (MACHINE_IS_INVES) {
                        //Inves
                        //printf ("Inves");
                        //AND con valor de la memoria RAM. Este AND en Inves solo aplica a puertos de la ULA

			//temp
			//if (puerto>31*256+254) 
			//	printf ("puerto: %d (%XH) valor %d (%XH)\n", puerto,puerto,value,value);

			value=value & memoria_spectrum[puerto];
                        out_254=value;

                        //y xor con bits 3 y 4
                        z80_byte bit3,bit4;
                        bit3=value & 8;
                        bit4=value & 16;
                        bit4=bit4 >> 1;

                        bit3= (bit3 ^ bit4) & 8;

                        if ( (bit3 & 8) != (ultimo_altavoz & 8) ) {
                                //valor diferente. conmutar altavoz
                                //bit_salida_sonido.v ^=1;
				set_value_beeper((!!(bit3 & 8) << 1));
                        }
                        ultimo_altavoz=bit3;


                }
                else {
                        out_254=value;

			/*
                        if ( (value & 24) != (ultimo_altavoz & 24) ) {
                                //valor diferente. conmutar altavoz
                                bit_salida_sonido.v ^=1;
                        }
                        ultimo_altavoz=value;
			*/

			//Extracted from Fuse emulator
			set_value_beeper( (!!(value & 0x10) << 1) + ( (!(value & 0x8))  ) );
                }


                if (rainbow_enabled.v) {
                        //printf ("%d\n",t_estados);
                        int i;

			i=t_estados;
                        //printf ("t_estados %d screen_testados_linea %d bord: %d\n",t_estados,screen_testados_linea,i);

			//Este i>=0 no haria falta en teoria
			//pero ocurre a veces que justo al activar rainbow, t_estados_linea_actual tiene un valor descontrolado
                        if (i>=0 && i<FULLBORDER_ARRAY_LENGTH) {

				//Ajuste valor de indice a multiple de 4 (lo normal en la ULA)
				//O sea, quitamos los 2 bits inferiores
				i=i&(0xFFFFFFFF-3);

				//En Inves. Snow in border
				if (MACHINE_IS_INVES) {
					//Enviamos color real, sin mascara a la ROM
					fullbuffer_border[i]=out_254_original_value & 7;

					//E inmediatamente despues, color con mascara
					i++;

					//Si nos salimos del array, volver atras y sobreescribir color real
					if (i==FULLBORDER_ARRAY_LENGTH) i--;

					//Finalmente escribimos valor con mascara a la ROM
					fullbuffer_border[i]=out_254 & 7;
				}

				else {
					fullbuffer_border[i]=get_border_colour_from_out();
				}
				//printf ("cambio border i=%d color: %d\n",i,out_254 & 7);
			}
                        //else printf ("Valor de border scanline fuera de rango: %d\n",i);
                }


		else {
			//Realvideo desactivado. Si esta autodeteccion de realvideo, hacer lo siguiente
			if (autodetect_rainbow.v) {
				//if (reg_pc>=16384) {
					//Ver si hay cambio de border
					if ( (anterior_out_254&7) != (out_254&7) ) {
						//printf ("cambio de border\n");
						detect_rainbow_border_changes_in_frame++;
					}
				//}
			}
		}

		set_value_beeper_on_array(value_beeper);

}


void out_port_spectrum_no_time(z80_int puerto,z80_byte value)
{

        //Los OUTS los capturan los diferentes interfaces que haya conectados, por tanto no hacer return en ninguno, para que se vayan comprobando
        //uno despues de otro
	z80_byte puerto_l=puerto&255;

        //super wonder boy usa puerto 1fe
        //paperboy usa puertos xxfe
	//Puerto 254 realmente es cualquier puerto par
       	if ( (puerto & 1 )==0 && !(MACHINE_IS_CHLOE) && !(MACHINE_IS_TIMEX_TS2068) ) {
		
		out_port_spectrum_border(puerto,value);

        }


	//Puerto 254 solamente, para chloe y timex
	if ( puerto_l==254 && (MACHINE_IS_CHLOE || MACHINE_IS_TIMEX_TS2068) ) {
		out_port_spectrum_border(puerto,value);

        }


//ay chip
/*
El circuito de sonido contiene dieciseis registros; para seleccionarlos, primero se escribe
el numero de registro en la puerta de escritura de direcciones, FFFDh (65533), y despues
lee el valor del registro (en la misma direccion) o se escribe en la direccion de escritura
de registros de datos, BFFDh (49149). Una vez seleccionado un registro, se puede realizar
cualquier numero de operaciones de lectura o escritura de datos. S~1o habr~ que volver
escribir en la puerta de escritura de direcciones cuando se necesite seleccio~ar otro registro.
La frecuencia de reloj basica de este circuito es 1.7734 MHz (con precision del 0.01~~o).
*/
	//el comportamiento de los puertos ay es con mascaras AND... esto se ve asi en juegos como chase hq
/*
Peripheral: 128K AY Register.
Port: 11-- ---- ---- --0- 

Peripheral: 128K AY (Data).
Port: 10-- ---- ---- --0- 

49154=1100000000000010

*/

	//Puertos Chip AY
	if ( ((puerto & 49154) == 49152) || ((puerto & 49154) == 32768) ) {
		if ((puerto & 49154) == 49152) puerto=65533;
		else if ((puerto & 49154) == 32768) puerto=49149;

		activa_ay_chip_si_conviene();
		if (ay_chip_present.v==1) out_port_ay(puerto,value);
		//return;
	}

	//Puertos de Paginacion
	if (MACHINE_IS_SPECTRUM_128_P2)
	{
		//Para 128k
		//Puerto tipicamente 32765
		//The additional memory features of the 128K/+2 are controlled to by writes to port 0x7ffd. As normal on Sinclair hardware, the port address is in fact only partially decoded and the hardware will respond to any port address with bits 1 and 15 reset. However, 0x7ffd should be used if at all possible to avoid conflicts with other hardware.

		if ( (puerto & 32770) == 0 ) {

			//printf ("paginacion pc: %d\n",reg_pc);
			//printf ("puerto_32765: %d enviando valor: %d\n",puerto_32765,value);

			//ver si paginacion desactivada
			//if (puerto_32765 & 32) return;

			if ((puerto_32765 & 32)==0) {

				puerto_32765=value;
				//Paginar RAM y ROM
                	        //32 kb rom, 128 ram

	                        //asignar ram
        	                mem_page_ram_128k();

                	        //asignar rom
                        	mem_page_rom_128k();

			}

                        //return;

                }
	}

	if (MACHINE_IS_SPECTRUM_P2A) 
	{
		//Para +2A


		//Puerto tipicamente 32765
		// the hardware will respond only to those port addresses with bit 1 reset, bit 14 set and bit 15 reset (as opposed to just bits 1 and 15 reset on the 128K/+2).
	        if ( (puerto & 49154) == 16384 ) {
			mem128_p2a_write_page_port(puerto,value);

			//return;
		}

		//Puerto tipicamente 8189

		// the hardware will respond to all port addresses with bit 1 reset, bit 12 set and bits 13, 14 and 15 reset).
		if ( (puerto & 61442 )== 4096) {
			mem128_p2a_write_page_port(puerto,value);
			//return;
		}
	}		


	//Puertos paginacion +2A pero en ZXUNO con BOOTM disabled
        if (MACHINE_IS_ZXUNO_BOOTM_DISABLED)
        {


        	        //Puerto tipicamente 32765
	                // the hardware will respond only to those port addresses with bit 1 reset, bit 14 set and bit 15 reset (as opposed to just bits 1 and 15 reset on the 128K/+2).
        	        if ( (puerto & 49154) == 16384 ) {
                	        zxuno_p2a_write_page_port(puerto,value);

	                        //return;
        	        }

	                //Puerto tipicamente 8189

        	        // the hardware will respond to all port addresses with bit 1 reset, bit 12 set and bits 13, 14 and 15 reset).
	                if ( (puerto & 61442 )== 4096) {
        	                zxuno_p2a_write_page_port(puerto,value);
	                        //return;
        	        }


        }               

	//Puertos paginacion +2A pero en ZXUNO con BOOTM enabled. No hacen ningun efecto
        if (MACHINE_IS_ZXUNO_BOOTM_ENABLED)
        {


                        //Puerto tipicamente 32765
                        // the hardware will respond only to those port addresses with bit 1 reset, bit 14 set and bit 15 reset (as opposed to just bits 1 and 15 reset on the 128K/+2).
                        if ( (puerto & 49154) == 16384 ) {
                                //zxuno_p2a_write_page_port(puerto,value);
				//printf ("Paginacion 32765 con bootm activo\n");

                                //return;
                        }

                        //Puerto tipicamente 8189

                        // the hardware will respond to all port addresses with bit 1 reset, bit 12 set and bits 13, 14 and 15 reset).
                        if ( (puerto & 61442 )== 4096) {
                                //zxuno_p2a_write_page_port(puerto,value);
				//printf ("Paginacion 8189 con bootm activo\n");
                                //return;
                        }


        }


	//Puerto paginacion 32765 para Chloe
	if (MACHINE_IS_CHLOE) {
			//Puerto tipicamente 32765
                        // the hardware will respond only to those port addresses with bit 1 reset, bit 14 set and bit 15 reset (as opposed to just bits 1 and 15 reset on the 128K/+2).
                        if ( (puerto & 49154) == 16384 ) {
				puerto_32765=value;
				chloe_set_memory_pages();
                        }
	}


	//Fuller audio box
	//The sound board works on port numbers 0x3f and 0x5f. Port 0x3f is used to select the active AY register and to 
	//receive data from the AY-3-8912, while port 0x5f is used for sending data
	//Interfiere con MMC
	if (zxmmc_emulation.v==0 && ( puerto==0x3f || puerto==0x5f) ) {
		activa_ay_chip_si_conviene();
		if (ay_chip_present.v==1) {
			//printf("out Puerto sonido chip AY puerto=%d valor=%d\n",puerto,value);
                	if (puerto==0x3f) puerto=65533;
	                else if (puerto==0x5f) puerto=49149;
        	        //printf("out Puerto cambiado sonido chip AY puerto=%d valor=%d\n",puerto,value);

                	out_port_ay(puerto,value);
			//return;
		}
	}

	//zx printer
	if ((puerto&0xFF)==0xFB) {
		if (zxprinter_enabled.v==1) {
			zxprinter_write_port(value);

			//return;
		}
	}

	//Puerto para ocultar/ver ram baja del inves
	if (MACHINE_IS_INVES && puerto==0x02F1) {
		//Significado valores enviados a puerto:
		// 7 6 5 4 3 2 1 0
		// X X X X X X X H  :
		// X: no usado
		// H: Mostrar RAM oculta o no: 1: no mostrar (situacion por defecto). 0 mostrar->lecturas de 0...16383 vendran de la RAM
		if ((value &1)==0) {
			inves_show_hidden_ram.v=1;
			debug_printf (VERBOSE_DEBUG,"Show Inves Low RAM");
		}
		else {
			inves_show_hidden_ram.v=0;
			debug_printf (VERBOSE_DEBUG,"Hide Inves Low RAM (normal situation)");
		}

		//return;
	}

        //UlaPlus
        if (ulaplus_presente.v && (puerto&0xFF)==0x3b) {
		z80_byte puerto_h=puerto>>8;

		//register port
                if (puerto_h==0xbf) {
			/*

I/O ports

ULAplus is controlled by two ports.

0xBF3B is the register port (write only)

The byte output will be interpreted as follows:

Bits 0-5: Select the register sub-group
Bits 6-7: Select the register group. Two groups are currently available:

00 - palette group
When this group is selected, the sub-group determines the entry in the palette table (0-63).

01 - mode group

The sub-group is (optionally) used to mirror the video functionality of Timex port #FF as follows:

Bits 0-2: Screen mode. 000=screen 0, 001=screen 1, 010=hi-colour, 110=hi-res (bank 5)
100=screen 0, 101=screen 1, 011=hi-colour, 111=hi-res (bank 7)
Bits 3-5: Sets the screen colour in hi-res mode.
000 - Black on White 100 - Green on Magenta
001 - Blue on Yellow 101 - Cyan on Red
010 - Red on Cyan 110 - Yellow on Blue
011 - Magenta on Green 111 - White on Black

0xFF3B is the data port (read/write)

When the palette group is selected, the byte written will describe the color.

When the mode group is selected, the byte output will be interpreted as follows:

Bit 0: ULAplus palette on (1) / off (0)
Bit 1: (optional) greyscale: on (1) / off (0) (same as turning the color off on the television)

Implementations that support the Timex video modes use the #FF register as the primary means to set the video mode, as per the Timex machines. It is left to the individual implementations to determine if reading the port returns the previous write or the floating bus.


			*/
                        ulaplus_last_send_BF3B=value;
                }

		//data port
		if (puerto_h==0xff) {
			ulaplus_last_send_FF3B=value;
			if ( (ulaplus_last_send_BF3B&(64+128))==0) {
				//establecer color
				ulaplus_change_palette_colour((ulaplus_last_send_BF3B&63),value);
				
			}

			if ( (ulaplus_last_send_BF3B&(64+128))==64) {
                                //establecer modo
				ulaplus_set_mode(value);

				/*

				z80_byte ulaplus_mode_anterior=ulaplus_mode;

				ulaplus_mode=(value&63);
				
				switch (ulaplus_mode) {
					case 0:
						debug_printf (VERBOSE_DEBUG,"Disabling ULAplus (mode 0)");
						ulaplus_enabled.v=0;
                                                if (ulaplus_mode!=ulaplus_mode_anterior) {
                                                        screen_print_splash_text(10,0,7+8,"Disabling ULAplus (mode 0)");
                                                }

					break;

					case 1:
						ulaplus_enabled.v=1;
						debug_printf (VERBOSE_DEBUG,"Enabling ULAplus mode 1. RGB");
						if (ulaplus_mode!=ulaplus_mode_anterior) {
							screen_print_splash_text(10,0,7+8,"Enabling ULAplus mode 1. RGB");
						}
					break;

					case 3:
						ulaplus_enabled.v=1;
						debug_printf (VERBOSE_DEBUG,"Enabling ULAplus linear mode 3. Radastan. 128x96");
                                                if (ulaplus_mode!=ulaplus_mode_anterior) {
                                                        screen_print_splash_text(10,0,7+8,"Enabling ULAplus linear mode 3. Radastan. 128x96");
                                                }

					break;

                                        case 5:
                                                ulaplus_enabled.v=1;
                                                debug_printf (VERBOSE_DEBUG,"Enabling ULAplus linear mode 5. 256x96");
                                                if (ulaplus_mode!=ulaplus_mode_anterior) {
                                                        screen_print_splash_text(10,0,7+8,"Enabling ULAplus linear mode 5. 256x96");
                                                }

                                        break;  

                                        case 7:
                                                ulaplus_enabled.v=1;
                                                debug_printf (VERBOSE_DEBUG,"Enabling ULAplus linear mode 7. 128x192");
                                                if (ulaplus_mode!=ulaplus_mode_anterior) {
                                                        screen_print_splash_text(10,0,7+8,"Enabling ULAplus linear mode 7. 128x192");
                                                }

                                        break;  

					case 9:
                                                ulaplus_enabled.v=1;
                                                debug_printf (VERBOSE_DEBUG,"Enabling ULAplus linear mode 9. 256x192");
                                                if (ulaplus_mode!=ulaplus_mode_anterior) {
                                                        screen_print_splash_text(10,0,7+8,"Enabling ULAplus linear mode 9. 256x192");
                                                }

                                        break;



					default:
						debug_printf (VERBOSE_DEBUG,"Unknown ulaplus mode %d",ulaplus_mode);
                                        break;  

				}

				*/
				
			}
		}
        }

	//ZXUNO
	if (MACHINE_IS_ZXUNO && (puerto==0xFC3B  || puerto==0xFD3B)) {
		zxuno_write_port(puerto,value);
		//return;
	}


	//Puertos ZXMMC. Interfiere con Fuller Audio Box
	if (zxmmc_emulation.v && (puerto_l==0x1f || puerto_l==0x3f)) {
		//printf ("Puerto ZXMMC Write: 0x%02x valor: 0x%02X\n",puerto_l,value);
		if (puerto_l==0x1f) mmc_cs(value);
		if (puerto_l==0x3f) mmc_write(value);
	}

        //Puertos DIVMMC. El de MMC 
        if (divmmc_enabled.v && (puerto_l==0xe7 || puerto_l==0xeb) ) {
		//printf ("Puerto DIVMMC Write: 0x%02x valor: 0x%02X\n",puerto_l,value);
        	//Si en ZXUNO y DIVEN desactivado
                //Aunque cuando se toca el bit DIVEN de ZX-Uno se sincroniza divmmc_enable,
                //pero por si acaso... por si se activa manualmente desde menu del emulador
                //el divmmc pero zxuno espera que este deshabilitado, como en la bios
	        //if (MACHINE_IS_ZXUNO_DIVEN_DISABLED) return;
		if (!MACHINE_IS_ZXUNO_DIVEN_DISABLED) {

	                if (puerto_l==0xe7) {
				//Parece que F6 es la tarjeta 0 en divmmc
				if (value==0xF6) value=0xFE;
				mmc_cs(value);
			}
        	        if (puerto_l==0xeb) mmc_write(value);
		}
        }

	//Puertos DIVMMC. El de Paginacion
	if (divmmc_enabled.v && puerto_l==0xe3) {
	        //Si en ZXUNO y DIVEN desactivado
                //Aunque cuando se toca el bit DIVEN de ZX-Uno se sincroniza divmmc_enable,
                //pero por si acaso... por si se activa manualmente desde menu del emulador
                //el divmmc pero zxuno espera que este deshabilitado, como en la bios
        	//if (MACHINE_IS_ZXUNO_DIVEN_DISABLED) return;
        	if (!MACHINE_IS_ZXUNO_DIVEN_DISABLED) {

			//printf ("Puerto control paginacion DIVMMC Write: 0x%02x valor: 0x%02X\n",puerto_l,value);
			divmmc_write_control_register(value);
		}
	}


        //Puerto Spectra
        if (spectra_enabled.v && puerto==0x7FDF) spectra_write(value);

	//Sprite Chip
	if (spritechip_enabled.v && (puerto==SPRITECHIP_COMMAND_PORT || puerto==SPRITECHIP_DATA_PORT) ) spritechip_write(puerto,value);



	//Puerto Timex Video. 8 bit bajo a ff
	if (timex_video_emulation.v && puerto_l==0xFF) {

		if ( (timex_video_last_out&7)!=(value&7)) {
	                char mensaje[200];

			if ((value&7)==0) sprintf (mensaje,"Setting Timex Video Mode 0 (standard screen 0)");
			else if ((value&7)==1) sprintf (mensaje,"Setting Timex Video Mode 1 (standard screen 1)");
			else if ((value&7)==2) sprintf (mensaje,"Setting Timex Video Mode 2 (hires colour 8x1)");
			else if ((value&7)==6) sprintf (mensaje,"Timex Video Mode 6 (512x192 monochrome) not supported yet. Reducing to 256x192");
                        else sprintf (mensaje,"Setting Unknown Timex Video Mode %d",value);

                	screen_print_splash_text(10,0,7+8,mensaje);
		}


		timex_video_last_out=value;

		if (MACHINE_IS_CHLOE_280SE) chloe_set_memory_pages();
		if (MACHINE_IS_TIMEX_TS2068) timex_set_memory_pages();

	}


	//Puerto Timex Paginacion
	if (puerto_l==0xf4 && (MACHINE_IS_CHLOE || MACHINE_IS_TIMEX_TS2068) ) {
		timex_port_f4=value;
		if (MACHINE_IS_CHLOE_280SE) chloe_set_memory_pages();
		if (MACHINE_IS_TIMEX_TS2068) timex_set_memory_pages();

        }


	//Puertos AY para Timex y Chloe
	if (puerto_l==0xf5 || puerto_l==0xf6) {
		if (MACHINE_IS_CHLOE || MACHINE_IS_TIMEX_TS2068) {
                	if (puerto_l==0xf5) puerto=65533;
			else puerto=49149;

               		activa_ay_chip_si_conviene();
	                if (ay_chip_present.v==1) out_port_ay(puerto,value);
		}
        }



	//debug_printf (VERBOSE_DEBUG,"Out Port %x unknown written with value %x, PC after=0x%x",puerto,value,reg_pc);
}





void out_port_spectrum(z80_int puerto,z80_byte value)
{
  ula_contend_port_early( puerto );
  out_port_spectrum_no_time(puerto,value);
  ula_contend_port_late( puerto ); t_estados++;
}


void out_port_zx80(z80_int puerto,z80_byte value)
{
  ula_contend_port_early( puerto );
  out_port_zx80_no_time(puerto,value);
  ula_contend_port_late( puerto ); t_estados++;
}

void out_port_zx81(z80_int puerto,z80_byte value)
{
  ula_contend_port_early( puerto );
  out_port_zx81_no_time(puerto,value);
  ula_contend_port_late( puerto ); t_estados++;
}





//Pokea parte de la ram oculta del inves con valor
void poke_inves_rom(z80_byte value)
{
        z80_int dir;

	/*
        Bit   7   6   5   4   3   2   1   0
            +-------------------------------+
            |   |   |   | E | M |   Border  |
            +-------------------------------+

	Bits y valores usados normalmente al enviar sonido son 5 bits inferiores (0...31)
	Pero por ejemplo el juego Hysteria (A year after mix) usa todos los bits.... con el resultado que los puertos usados
	van desde el 0 hasta el 65534! Entonces las direcciones AND que usa son toda la RAM!
	
	//31*256+254=8190
	*/

	//Pokeamos toda la ram baja aunque en la mayoria de juegos (no el hysteria) seria suficiente con ir desde 0 hasta 8190
        for (dir=0;dir<16384;dir++) {
                poke_byte_no_time(dir,value);
        }
}

