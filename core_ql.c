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
#include <string.h>


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


//Numero de canal ficticio para archivos que se abran mdv o flp, para distinguirlos de los que gestiona el sistema
#define QL_ID_CANAL_INVENTADO_MICRODRIVE 1

z80_byte byte_leido_core_ql;
char buffer_disassemble[100000];

extern unsigned char ql_pc_intr;

int refresca=0;


//char ql_nombre_archivo_load[255];

//Archivo que se est abiendo, cargando, etc. TODO: no soporta abrir mas de un archivo a ala vez
char ql_full_path_load[PATH_MAX];

void ql_debug_force_breakpoint(char *message)
{
  catch_breakpoint_index=0;
  menu_breakpoint_exception.v=1;
  menu_abierto=1;
  sprintf (catch_breakpoint_message,"%s",message);
  printf ("Abrimos menu\n");
}

void core_ql_trap_one(void)
{

  //Ver pagina 173. 18.14 Trap Keys

  debug_printf (VERBOSE_PARANOID,"Trap 1. D0=%02XH D1=%02XH A0=%08XH A1=%08XH A6=%08XH PC=%05XH is : ",
    m68k_get_reg(NULL,M68K_REG_D0),m68k_get_reg(NULL,M68K_REG_D1),m68k_get_reg(NULL,M68K_REG_A0),
    m68k_get_reg(NULL,M68K_REG_A1),m68k_get_reg(NULL,M68K_REG_A6),m68k_get_reg(NULL,M68K_REG_PC));

  switch(m68k_get_reg(NULL,M68K_REG_D0)) {

      case 0x01:
        debug_printf (VERBOSE_PARANOID,"Trap 1: MT.INF");
        //ql_debug_force_breakpoint("despues MT.INF");
      break;

      case 0x10:
        debug_printf (VERBOSE_PARANOID,"Trap 1: MT.DMODE");
        //ql_debug_force_breakpoint("despues DMODE");
      break;

      case 0x11:
        debug_printf (VERBOSE_PARANOID,"Trap 1: MT.IPCOM");
      break;

      case 0x16:
        debug_printf (VERBOSE_PARANOID,"Trap 1: MT.ALBAS allocate BASIC area");
      break;

      case 0x17:
        debug_printf (VERBOSE_PARANOID,"Trap 1: MT.REBAS release BASIC area");
      break;


      default:
        debug_printf (VERBOSE_PARANOID,"Unknown trap");
      break;

    }

}

unsigned int pre_io_open_a[8];
unsigned int pre_io_open_d[8];


unsigned int pre_fs_headr_a[8];
unsigned int pre_fs_headr_d[8];

unsigned int pre_fs_load_a[8];
unsigned int pre_fs_load_d[8];

void ql_store_a_registers(unsigned int *destino, int ultimo)
{
  if (ultimo>=0) destino[0]=m68k_get_reg(NULL,M68K_REG_A0);
  if (ultimo>=1) destino[1]=m68k_get_reg(NULL,M68K_REG_A1);
  if (ultimo>=2) destino[2]=m68k_get_reg(NULL,M68K_REG_A2);
  if (ultimo>=3) destino[3]=m68k_get_reg(NULL,M68K_REG_A3);
  if (ultimo>=4) destino[4]=m68k_get_reg(NULL,M68K_REG_A4);
  if (ultimo>=5) destino[5]=m68k_get_reg(NULL,M68K_REG_A5);
  if (ultimo>=6) destino[6]=m68k_get_reg(NULL,M68K_REG_A6);
  if (ultimo>=7) destino[7]=m68k_get_reg(NULL,M68K_REG_A7);
}

void ql_store_d_registers(unsigned int *destino, int ultimo)
{
  if (ultimo>=0) destino[0]=m68k_get_reg(NULL,M68K_REG_D0);
  if (ultimo>=1) destino[1]=m68k_get_reg(NULL,M68K_REG_D1);
  if (ultimo>=2) destino[2]=m68k_get_reg(NULL,M68K_REG_D2);
  if (ultimo>=3) destino[3]=m68k_get_reg(NULL,M68K_REG_D3);
  if (ultimo>=4) destino[4]=m68k_get_reg(NULL,M68K_REG_D4);
  if (ultimo>=5) destino[5]=m68k_get_reg(NULL,M68K_REG_D5);
  if (ultimo>=6) destino[6]=m68k_get_reg(NULL,M68K_REG_D6);
  if (ultimo>=7) destino[7]=m68k_get_reg(NULL,M68K_REG_D7);
}



void ql_restore_a_registers(unsigned int *origen, int ultimo)
{
  if (ultimo>=0) m68k_set_reg(M68K_REG_A0,origen[0]);
  if (ultimo>=1) m68k_set_reg(M68K_REG_A1,origen[1]);
  if (ultimo>=2) m68k_set_reg(M68K_REG_A2,origen[2]);
  if (ultimo>=3) m68k_set_reg(M68K_REG_A3,origen[3]);
  if (ultimo>=4) m68k_set_reg(M68K_REG_A4,origen[4]);
  if (ultimo>=5) m68k_set_reg(M68K_REG_A5,origen[5]);
  if (ultimo>=6) m68k_set_reg(M68K_REG_A6,origen[6]);
  if (ultimo>=7) m68k_set_reg(M68K_REG_A7,origen[7]);
}


void ql_restore_d_registers(unsigned int *origen, int ultimo)
{
  if (ultimo>=0) m68k_set_reg(M68K_REG_D0,origen[0]);
  if (ultimo>=1) m68k_set_reg(M68K_REG_D1,origen[1]);
  if (ultimo>=2) m68k_set_reg(M68K_REG_D2,origen[2]);
  if (ultimo>=3) m68k_set_reg(M68K_REG_D3,origen[3]);
  if (ultimo>=4) m68k_set_reg(M68K_REG_D4,origen[4]);
  if (ultimo>=5) m68k_set_reg(M68K_REG_D5,origen[5]);
  if (ultimo>=6) m68k_set_reg(M68K_REG_D6,origen[6]);
  if (ultimo>=7) m68k_set_reg(M68K_REG_D7,origen[7]);
}


void core_ql_trap_two(void)
{

  //int reg_a0;

  //Ver pagina 173. 18.14 Trap Keys

  debug_printf (VERBOSE_PARANOID,"Trap 1. D0=%02XH A0=%08XH A1=%08XH PC=%05XH is : ",
    m68k_get_reg(NULL,M68K_REG_D0),m68k_get_reg(NULL,M68K_REG_A0),m68k_get_reg(NULL,M68K_REG_A1),m68k_get_reg(NULL,M68K_REG_PC));

  switch(m68k_get_reg(NULL,M68K_REG_D0)) {

      case 1:
        debug_printf(VERBOSE_PARANOID,"Trap 1. IO.OPEN");
        //Open a channel. IO.OPEN Guardo todos registros A y D yo internamente de D2,D3,A2,A3 para restaurarlos despues de que se hace el trap de microdrive
        ql_store_a_registers(pre_io_open_a,7);
        ql_store_d_registers(pre_io_open_d,7);
      break;

      case 2:
        debug_printf(VERBOSE_PARANOID,"Trap 1. IO.CLOSE");
      break;

      default:
        debug_printf (VERBOSE_PARANOID,"Unknown trap");
      break;

    }

}

void ql_get_file_header(char *nombre,unsigned int destino)
{
  /*
  pagina 38. 7.0 Directory Device Drivers
  Each file is assumed to have a 64-byte header (the logical beginning of file is set to byte 64, not byte zero). This header should be formatted as follows:

  00  long        file length
  04  byte        file access key (not yet implemented - currently always zero)
  05  byte        file type
  06  8 bytes     file type-dependent information
  0E  2+36 bytes  filename
  34 long         reserved for update date (not yet implemented)
  38 long         reserved for reference date (not yet implemented)
  3c long         reserved for backup date (not yet implemented)

  The current file types allowed are: 2, which is a relocatable object file;
  1, which is an executable program;
  255 is a directory;
  and 0 which is anything else

  In the case of file type 1,the first longword of type-dependent information holds
  the default size of the data space for the program.

  Ejecutable quiere decir binario??
  Si es un basic, es tipo 0?

  */

  debug_printf(VERBOSE_PARANOID,"Returning header for file %s on address %05XH",nombre,destino);

  //Inicializamos cabecera a 0
  int i;
  for (i=0;i<64;i++) ql_writebyte(destino+i,0);

  unsigned int tamanyo=get_file_size(nombre);
  //Guardar tamanyo big endian
  ql_writebyte(destino+0,(tamanyo>>24)&255);
  ql_writebyte(destino+1,(tamanyo>>16)&255);
  ql_writebyte(destino+2,(tamanyo>>8)&255);
  ql_writebyte(destino+3,tamanyo&255);

  //Tipo
  ql_writebyte(destino+5,0); //ejecutable 1

  //Nombre. de momento me lo invento para ir rapido
  ql_writebyte(destino+0xe,0); //longitud nombre en big endian
  ql_writebyte(destino+0xf,4); //longitud nombre en big endian

  ql_writebyte(destino+0x10,'p');
  ql_writebyte(destino+0x11,'e');
  ql_writebyte(destino+0x12,'p');
  ql_writebyte(destino+0x13,'e');

  //Falta el "default size of the data space for the program" ?


}

void core_ql_trap_three(void)
{

//Ver pagina 173. 18.14 Trap Keys

  debug_printf (VERBOSE_PARANOID,"Trap 3. D0=%02XH A0=%08XH A1=%08XH PC=%05XH is : ",
    m68k_get_reg(NULL,M68K_REG_D0),m68k_get_reg(NULL,M68K_REG_A0),m68k_get_reg(NULL,M68K_REG_A1),m68k_get_reg(NULL,M68K_REG_PC));

  switch(m68k_get_reg(NULL,M68K_REG_D0)) {
    case 0x2:
      debug_printf(VERBOSE_PARANOID,"Trap 3: IO.FLINE. fetch a line of bytes");
    break;

    case 0x4:
      debug_printf (VERBOSE_PARANOID,"Trap 3: IO.EDLIN");
    break;

    case 0x7:
      debug_printf (VERBOSE_PARANOID,"Trap 3: IO.SSTRG");
    break;

    case 0x47:
      debug_printf (VERBOSE_PARANOID,"Trap 3: FS.HEADR");
      //Guardar registros
      ql_store_a_registers(pre_fs_headr_a,7);
      ql_store_d_registers(pre_fs_headr_d,7);


    break;

    case 0x48:
      debug_printf (VERBOSE_PARANOID,"Trap 3: FS.LOAD. Lenght: %d Channel: %d Address: %05XH"
          ,m68k_get_reg(NULL,M68K_REG_D2),m68k_get_reg(NULL,M68K_REG_A0),m68k_get_reg(NULL,M68K_REG_A1)  );
      //D2.L length of file. A0 channellD. A1 base address for load

      //ql_debug_force_breakpoint("En FS.LOAD");

      //Guardar registros
      ql_store_a_registers(pre_fs_load_a,7);
      ql_store_d_registers(pre_fs_load_d,7);


    break;

    default:
      debug_printf (VERBOSE_PARANOID,"Trap 3: unknown");
    break;

  }
}

//Dado una ruta de QL tipo mdv1_programa , retorna mdv1 y programa por separados
void ql_split_path_device_name(char *ql_path, char *ql_device, char *ql_file)
{
  //Buscar hasta _
  int i;
  for (i=0;ql_path[i] && ql_path[i]!='_';i++);
  if (ql_path[i]==0) {
    //No encontrado
    ql_device[0]=0;
    ql_file[0]=0;
  }

  //Copiar desde inicio hasta aqui en ql_device
  int iorig=i;
  //Vamos del final hacia atras
  ql_device[i]=0;
  i--;
  char c;
  for (;i>=0;i--) {
    c=letra_minuscula(ql_path[i]);
    ql_device[i]=c;
  }

  //Restauramos indice y vamos de ahi+1 al final. Si nombre de archivo contiene un _, sustituir por .
  //Y pasar a minusculas todo
  i=iorig+1;
  int destino=0;

  for (;ql_path[i];i++,destino++) {
    c=letra_minuscula(ql_path[i]);
    if (c=='_') c='.';
    ql_file[destino]=c;
  }

  ql_file[destino]=0;

  debug_printf(VERBOSE_DEBUG,"Source path: %s Device: %s File: %s",ql_path,ql_device,ql_file);
}

//Dado un device y un nombre de archivo, retorna ruta a archivo en filesystem final
//Retorna 1 si error
int ql_return_full_path(char *device, char *file, char *fullpath)
{
  char *sourcepath;

  if (!strcasecmp(device,"mdv1")) sourcepath=ql_mdv1_root_dir;
  else if (!strcasecmp(device,"mdv2")) sourcepath=ql_mdv2_root_dir;
  else if (!strcasecmp(device,"flp1")) sourcepath=ql_flp1_root_dir;
  else return 1;

  if (sourcepath[0])  sprintf(fullpath,"%s/%s",sourcepath,file);
  else sprintf(fullpath,"%s",file); //Ruta definida como vacia


  return 0;
}

//Dice si la ruta que se le ha pasado corresponde a un mdv1_, o mdv2_, o flp1_
int ql_si_ruta_mdv_flp(char *texto)
{
  char *encontrado;

  char *buscar_mdv1="mdv1_";
  encontrado=util_strcasestr(texto, buscar_mdv1);
  if (encontrado) return 1;

  char *buscar_mdv2="mdv2_";
  encontrado=util_strcasestr(texto, buscar_mdv2);
  if (encontrado) return 1;

  char *buscar_flp1="flp1_";
  encontrado=util_strcasestr(texto, buscar_flp1);
  if (encontrado) return 1;

  return 0;
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
    //TODO: saltar esta llamada de manera mas elegante
    /*if ( get_pc_register()==0x04B4C) {
      debug_printf(VERBOSE_DEBUG,"QL Trap ROM: Skipping MDV1 boot");
      m68k_set_reg(M68K_REG_PC,0x04B50);
    }*/


    /* Lectura de tecla */
    if (get_pc_register()==0x02D40) {
      //Decir que hay una tecla pulsada
      //Temp solo cuando se pulsa enter
      //if ((puerto_49150&1)==0) {
      if (ql_pulsado_tecla()) {
        //debug_printf(VERBOSE_DEBUG,"QL Trap ROM: Tell one key pressed");
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
        //debug_printf(VERBOSE_DEBUG,"QL Trap ROM: Tell 1 key pressed");
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

  //Nota: lo normal seria que no hagamos este trap a no ser que se habilite emulacion de ql_microdrive_floppy_emulation.
  //Pero, si lo hacemos asi, si no habilitamos emulacion de micro&floppy, al pasar del menu de inicio (F1,F2) buscara el archivo BOOT, y como no salta el
  //trap, se queda bloqueado

    if (get_pc_register()==0x032B4 && m68k_get_reg(NULL,M68K_REG_D0)==1) {
      //en A0
      char ql_nombre_archivo_load[255];
      int reg_a0=m68k_get_reg(NULL,M68K_REG_A0);
      int longitud_nombre=peek_byte_z80_moto(reg_a0)*256+peek_byte_z80_moto(reg_a0+1);
      reg_a0 +=2;
      debug_printf (VERBOSE_PARANOID,"Lenght channel name: %d",longitud_nombre);

      char c;
      int i=0;
      for (;longitud_nombre;longitud_nombre--) {
        c=peek_byte_z80_moto(reg_a0++);
        ql_nombre_archivo_load[i++]=c;
        //printf ("%c",c);
      }
      //printf ("\n");
      ql_nombre_archivo_load[i++]=0;
      debug_printf (VERBOSE_PARANOID,"Channel name: %s",ql_nombre_archivo_load);
      //sleep(1);

      //Hacer que si es mdv1_ ... volver

      //A7=0002846EH
      //A7=0002847AH
      //Incrementar A7 en 12
      //set-register pc=5eh. apunta a un rte
      if (ql_si_ruta_mdv_flp(ql_nombre_archivo_load)) {
      //char *buscar="mdv";
      //char *encontrado;
      //encontrado=util_strcasestr(ql_nombre_archivo_load, buscar);
      //if (encontrado) {
        debug_printf (VERBOSE_PARANOID,"Returning from trap without opening anything because file is mdv1, mdv2 or flp1");

        //ql_debug_force_breakpoint("En IO.OPEN");

/*
069CC movea.l A1, A0                              |L069CC MOVEA.L A1,A0
069CE move.w  (A6,A1.l), -(A7)                    |       MOVE.W  $00(A6,A1.L),-(A7)
069D2 trap    #$4                                 |       TRAP    #$04
>069D4 trap    #$2                                 |       TRAP    #$02
069D6 moveq   #$3, D3                             |       MOVEQ   #$03,D3
069D8 add.w   (A7)+, D3                           |       ADD.W   (A7)+,D3
069DA bclr    #$0, D3                             |       BCLR    #$00,D3
069DE add.l   D3, ($58,A6)                        |       ADD.L   D3,$0058(A6)

Es ese trap 2 el que se llama al hacer lbytes mdv...

Y entra asi:
command@cpu-step> run
Running until a breakpoint, menu opening or other event
PC: 069D4 SP: 3FFC0 USP: 3FFC0 SR: 0000 :

A0: 00000D88 A1: 00000D88 A2: 00006906 A3: 00000668 A4: 00000012 A5: 00000670 A6: 0003F068 A7: 0003FFC0 D0: 00000001 D1: FFFFFFFF D2: 00000058 D3: 00000001 D4: 00000001 D5: 00000000 D6: 00000000 D7: 00000000
069D4 trap    #$2

*/

        //D2,D3,A2,A3 se tienen que preservar, segun dice el trap.
        //Segun la info general de los traps, tambien se deben guardar de D4 a D7 y A4 a A6. Directamente guardo todos los D y A excepto A7

        ql_restore_d_registers(pre_io_open_d,7);
        ql_restore_a_registers(pre_io_open_a,6);
        //ql_restore_a_registers(pre_io_open_a,7);



        //Volver de ese trap
        m68k_set_reg(M68K_REG_PC,0x5e);
        //Ajustar stack para volver
        int reg_a7=m68k_get_reg(NULL,M68K_REG_A7);
        reg_a7 +=12;
        m68k_set_reg(M68K_REG_A7,reg_a7);



        //Metemos channel id (A0) inventado
        m68k_set_reg(M68K_REG_A0,QL_ID_CANAL_INVENTADO_MICRODRIVE);

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


        //No error.
        m68k_set_reg(M68K_REG_D0,0);

        char ql_io_open_device[PATH_MAX];
        char ql_io_open_file[PATH_MAX];

        ql_split_path_device_name(ql_nombre_archivo_load,ql_io_open_device,ql_io_open_file);

        ql_return_full_path(ql_io_open_device,ql_io_open_file,ql_full_path_load);

        if (!si_existe_archivo(ql_full_path_load)) {
          debug_printf(VERBOSE_PARANOID,"File %s not found",ql_full_path_load);
          //Retornar Not found (NF)
          m68k_set_reg(M68K_REG_D0,-7);
        }

        //D1= Job ID. TODO. Parece que da error "error in expression" porque no se asigna un job id valido?
        //Parece que D1 entra con -1, que quiere decir "the channel will be associated with the current job"
        //m68k_set_reg(M68K_REG_D1,0); //Valor de D1 inventado. Da igual, tambien fallara
        /*

        */

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

    //Quiza Trap 3 FS.HEADR acaba saltando a 0337C move.l  A0, D7
    if (get_pc_register()==0x0337C && m68k_get_reg(NULL,M68K_REG_D0)==0x47 && ql_microdrive_floppy_emulation) {
        debug_printf (VERBOSE_PARANOID,"FS.HEADR. Channel ID=%d",m68k_get_reg(NULL,M68K_REG_A0) );

        //Si canal es el mio ficticio 100
        if (m68k_get_reg(NULL,M68K_REG_A0)==QL_ID_CANAL_INVENTADO_MICRODRIVE) {
          //Devolver cabecera. Se supone que el sistema operativo debe asignar espacio para la cabecera? Posiblemente si.
          //Forzamos meter cabecera en espacio de memoria de pantalla a ver que pasa
          ql_get_file_header(ql_full_path_load,m68k_get_reg(NULL,M68K_REG_A1));
          //ql_get_file_header(ql_nombre_archivo_load,131072); //131072=pantalla

          ql_restore_d_registers(pre_fs_headr_d,7);
          ql_restore_a_registers(pre_fs_headr_a,6);

          //Volver de ese trap
          m68k_set_reg(M68K_REG_PC,0x5e);
          unsigned int reg_a7=m68k_get_reg(NULL,M68K_REG_A7);
          reg_a7 +=12;
          m68k_set_reg(M68K_REG_A7,reg_a7);

          //No error.
          m68k_set_reg(M68K_REG_D0,0);

          //D1.W length of header read. A1 top of read buffer
          m68k_set_reg(M68K_REG_D1,64);

          //Decimos que A1 es A1 top of read buffer
          unsigned int reg_a1=m68k_get_reg(NULL,M68K_REG_A1);
          reg_a1 +=64;
          m68k_set_reg(M68K_REG_A1,reg_a1);


          //Le decimos en A1 que la cabecera esta en la memoria de pantalla
          //m68k_set_reg(M68K_REG_A1,131072);

        }
    }



    //FS.LOAD
    if (get_pc_register()==0x0337C && m68k_get_reg(NULL,M68K_REG_D0)==0x48 && ql_microdrive_floppy_emulation) {
        debug_printf (VERBOSE_PARANOID,"FS.LOAD. Channel ID=%d",m68k_get_reg(NULL,M68K_REG_A0) );

        //Si canal es el mio ficticio 100
        if (m68k_get_reg(NULL,M68K_REG_A0)==QL_ID_CANAL_INVENTADO_MICRODRIVE) {

          ql_restore_d_registers(pre_fs_load_d,7);
          ql_restore_a_registers(pre_fs_load_a,6);

          unsigned int longitud=m68k_get_reg(NULL,M68K_REG_D2);


            debug_printf (VERBOSE_PARANOID,"Loading file %s at address %05XH with lenght: %d",ql_full_path_load,m68k_get_reg(NULL,M68K_REG_A1),longitud);
            //void load_binary_file(char *binary_file_load,int valor_leido_direccion,int valor_leido_longitud)

            //longitud la saco del propio archivo, ya que no me llega bien de momento pues no retornaba bien fs.headr
            //int longitud=get_file_size(ql_nombre_archivo_load);
            load_binary_file(ql_full_path_load,m68k_get_reg(NULL,M68K_REG_A1),longitud);



          //Volver de ese trap
          m68k_set_reg(M68K_REG_PC,0x5e);
          unsigned int reg_a7=m68k_get_reg(NULL,M68K_REG_A7);
          reg_a7 +=12;
          m68k_set_reg(M68K_REG_A7,reg_a7);

          //No error.
          m68k_set_reg(M68K_REG_D0,0);

          //m68k_set_reg(M68K_REG_D0,-7);


          //Decimos que A1 es A1 top address after load
          unsigned int reg_a1=m68k_get_reg(NULL,M68K_REG_A1);
          reg_a1 +=longitud;
          m68k_set_reg(M68K_REG_A1,reg_a1);


          //Le decimos en A1 que la cabecera esta en la memoria de pantalla
          //m68k_set_reg(M68K_REG_A1,131072);

        }
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
