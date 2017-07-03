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

#ifndef SCMP_H
#define SCMP_H

#include "compileoptions.h"

typedef unsigned char SCMP8;
typedef char INT8;
typedef unsigned short SCMP16;
typedef short INT16;
typedef unsigned int SCMP32;
typedef int INT32;

union SCMP_PAIR
{


#ifdef WORDS_BIGENDIAN
        struct { SCMP8 h3,h2,h,l; } b;
        struct { INT8 h3,h2,h,l; } sb;
        struct { SCMP16 h,l; } w;
        struct { INT16 h,l; } sw;
#else
        struct { SCMP8 l,h,h2,h3; } b;
        struct { SCMP16 l,h; } w;
        struct { INT8 l,h,h2,h3; } sb;
        struct { INT16 l,h; } sw;


#endif

        SCMP32 d;
        INT32 sd;
};

extern union SCMP_PAIR    scmp_m_PC;
extern union SCMP_PAIR    scmp_m_P1;
extern union SCMP_PAIR    scmp_m_P2;
extern union SCMP_PAIR    scmp_m_P3;
extern SCMP8   scmp_m_AC;
extern SCMP8   scmp_m_ER;
extern SCMP8   scmp_m_SR;


extern void scmp_run_opcode(void);
extern void scmp_reset();

extern int scmp_CPU_DISASSEMBLE( int pc , unsigned char op, unsigned char arg, char *buffer);

extern void scmp_get_flags_letters(unsigned char f,char *buffer);

#endif
