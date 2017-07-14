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

#ifndef ESXDOS_HANDLER_H
#define ESXDOS_HANDLER_H


#define ESXDOS_RST8_FA_READ 0x01
// $01  ld bc,nn        read access


#define ESXDOS_RST8_FA_WRITE  0x02
// $02  ld (bc), a      write access


#define ESXDOS_RST8_FA_OPEN_EX  0x00
// $00  nop                     open if exists, else
//                                      error

#define ESXDOS_RST8_FA_OPEN_AL  0x08
// $08  ex af,af'       open if exists, else
 //                                      create

#define ESXDOS_RST8_FA_CREATE_NEW 0x04
// $04  inc b           create if does not
 //                                      exist, else error

//Este es suma de los anteriores
//#define ESXDOS_RST8_FA_CREATE_AL  0x0C
// $0c  inc c           create if does not
 //                                      exist, else open and
 //                                      truncate

#define ESXDOS_RST8_FA_USE_HEADER  0x40
// $40  ld b,b          use plus3dos header
//                                      (passed in de)



#define ESXDOS_RST8_FA_OVERWRITE   0x0c
#define ESXDOS_RST8_FA_CLOSE      0x00

#define ESXDOS_RST8_DISK_READ 0x81
#define ESXDOS_RST8_M_GETSETDRV   0x89
#define ESXDOS_RST8_F_OPEN      0x9a
#define ESXDOS_RST8_F_CLOSE      0x9b
#define ESXDOS_RST8_F_READ      0x9d


#define ESXDOS_RST8_F_OPENDIR 0xa3
#define ESXDOS_RST8_F_READDIR 0xa4

#define ESXDOS_RST8_F_GETCWD 0xa8
#define ESXDOS_RST8_F_CHDIR 0xa9

extern z80_bit esxdos_handler_enabled;
extern void esxdos_handler_run(void);
extern char esxdos_handler_root_dir[];
extern char esxdos_handler_cwd[];


#endif
