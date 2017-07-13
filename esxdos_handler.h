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

#define ESXDOS_RST8_FA_READ      0x01

#define ESXDOS_RST8_FA_WRITE      0x04
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


#endif
