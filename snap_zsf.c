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
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cpu.h"
#include "debug.h"
#include "operaciones.h"
#include "zx8081.h"
#include "mem128.h"
#include "ay38912.h"
#include "compileoptions.h"
#include "tape_smp.h"
#include "audio.h"
#include "screen.h"
#include "menu.h"
#include "tape.h"
#include "snap.h"
#include "snap_z81.h"
#include "snap_zx8081.h"
#include "utils.h"
#include "ula.h"
#include "joystick.h"
#include "realjoystick.h"
#include "z88.h"
#include "chardetect.h"
#include "jupiterace.h"
#include "cpc.h"
#include "timex.h"
#include "zxuno.h"
#include "ulaplus.h"
#include "chloe.h"
#include "prism.h"
#include "diviface.h"
#include "snap_rzx.h"


#include "autoselectoptions.h"

#if defined(__APPLE__)
        #include <sys/syslimits.h>
#endif

#define ZSF_NOOP_ID 0
#define ZSF_Z80_REGS_ID 1
#define ZSF_MOTO_REGS_ID 2
#define ZSF_RAMBLOCK 3

/*
Format ZSF:
* All numbers are LSB

Every block is defined with a header:

2 bytes - 16 bit: block ID
4 bytes - 32 bit: block Lenght
After these 6 bytes, the data for the block comes.



-Block ID 0: ZSF_NOOP
No operation. Block lenght 0

-Block ID 1: ZSF_Z80_REGS
Z80 CPU Registers

-Block ID 2: ZSF_MOTO_REGS
Motorola CPU Registers

-Block ID 3: ZSF_RAMBLOCK
A ram binary block
Byte Fields:
0: Flags. Currently: bit 0: if compressed
1,2: Block start
3,4: Block lenght
5 and next bytes: data bytes

*/


int zsf_write_block(FILE *ptr_zsf_file, z80_byte *source,z80_int block_id, unsigned int lenght)
{
  z80_byte block_header[6];
  block_header[0]=value_16_to_8l(block_id);
  block_header[1]=value_16_to_8h(block_id);

  block_header[2]=(lenght)      & 0xFF;
  block_header[3]=(lenght>>8)   & 0xFF;
  block_header[4]=(lenght>>16)  & 0xFF;
  block_header[5]=(lenght>>24)  & 0xFF;

  //Write header
  fwrite(block_header, 1, 6, ptr_zsf_file);

  //Write data block
  fwrite(source, 1, lenght, ptr_zsf_file);

  return 0;

}


void load_zsf_snapshot(char *filename)
{

  FILE *ptr_zsf_file;

  ptr_zsf_file=fopen(filename,"rb");
  if (!ptr_zsf_file) {
          debug_printf (VERBOSE_ERR,"Error reading snapshot file %s",filename);
          return;
  }

  z80_byte block_header[6];

  //Read blocks
  while (!feof(ptr_zsf_file)) {
    //Read header block
    unsigned int leidos=fread(block_header,1,6,ptr_zsf_file);
    if (leidos!=6) {
      debug_printf(VERBOSE_ERR,"Error reading snapshot file");
      return;
    }

    z80_int block_id;
    block_id=value_8_to_16(block_header[1],block_header[0]);
    unsigned int block_lenght=block_header[2]+(block_header[3]*256)+(block_header[4]*65536)+(block_header[5]*16777216);

    debug_printf (VERBOSE_INFO,"Block id: %u Lenght: %u",block_id,block_lenght);

    z80_byte *block_data=malloc(block_lenght);

    if (block_data==NULL) {
      debug_printf(VERBOSE_ERR,"Error allocation memory reading ZSF file");
      return;
    }

    //Read block data
    leidos=fread(block_data,1,block_lenght,ptr_zsf_file);
    if (leidos!=block_lenght) {
      debug_printf(VERBOSE_ERR,"Error reading snapshot file");
      return;
    }

    //switch for everu possible block id
    switch(block_id)
    {
      default:
        debug_printf(VERBOSE_ERR,"Unknown ZSF Block ID: %u. Continue anyway",block_id);
      break;

    }


    free(block_data);

  }

  fclose(ptr_zsf_file);


}


void save_zsf_snapshot(char *filename)
{

  FILE *ptr_zsf_file;

  //Save header File
  ptr_zsf_file=fopen(filename,"wb");
  if (!ptr_zsf_file) {
          debug_printf (VERBOSE_ERR,"Error writing snapshot file %s",filename);
          return;
  }

  //Test. Save 48kb block
  //Allocate 5+48kb bytes
  z80_byte *ramblock=malloc(49152+5);
  if (ramblock==NULL) {
    debug_printf (VERBOSE_ERR,"Error allocating memory");
    return;
  }

  /*
  0: Flags. Currently: bit 0: if compressed
  1,2: Block start
  3,4: Block lenght
  5 and next bytes: data bytes
  */

  ramblock[0]=0;
  ramblock[1]=value_16_to_8l(16384);
  ramblock[2]=value_16_to_8h(16384);
  ramblock[3]=value_16_to_8l(49152);
  ramblock[4]=value_16_to_8h(49152);

  //Copy spectrum memory to ramblock
  int i;
  for (i=0;i<49152;i++) ramblock[5+i]=peek_byte_no_time(16384+i);

  //Store block to file
  zsf_write_block(ptr_zsf_file, ramblock,ZSF_RAMBLOCK, 49152+5);

  free(ramblock);


  fclose(ptr_zsf_file);

}
