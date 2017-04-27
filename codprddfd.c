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

#include "contend.h"
#include "compileoptions.h"
#include "utils.h"

 

z80_byte offset_xycb;


void invalid_opcode_ddfd(char *s)
{
	//debug_printf(VERBOSE_ERR,"Invalid opcode %s. Final PC: %X. Executing opcode without preffix",s,reg_pc);

	//En este caso, retroceder 1 instruccion y volver a parsear
	reg_pc--;
	reg_r--;

	//Para que no se queje el compilador
	if (s) {}

}	


void instruccion_ddfd_0 ()
{
        invalid_opcode_ddfd("DD/FD0");
}

void instruccion_ddfd_1 ()
{
        invalid_opcode_ddfd("DD/FD1");
}

void instruccion_ddfd_2 ()
{
        invalid_opcode_ddfd("DD/FD2");
}

void instruccion_ddfd_3 ()
{
        invalid_opcode_ddfd("DD/FD3");
}

void instruccion_ddfd_4 ()
{                                                                                                                                                      
        invalid_opcode_ddfd("DD/FD4");                                                                                                                
}

void instruccion_ddfd_5 ()
{
        invalid_opcode_ddfd("DD/FD5");
}

void instruccion_ddfd_6 ()
{
        invalid_opcode_ddfd("DD/FD6");
}

void instruccion_ddfd_7 ()
{
        invalid_opcode_ddfd("DD/FD7");
}

void instruccion_ddfd_8 ()
{
        invalid_opcode_ddfd("DD/FD8");
}


void instruccion_ddfd_9 ()
{

//ADD IX,BC
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );


	z80_int reg_ixiy;

	reg_ixiy=*registro_ixiy;

	reg_ixiy=add_16bit(reg_ixiy,BC);

   	*registro_ixiy=reg_ixiy;

}

void instruccion_ddfd_10 ()
{
        invalid_opcode_ddfd("DD/FD10");
}

void instruccion_ddfd_11 ()
{
        invalid_opcode_ddfd("DD/FD11");
}

void instruccion_ddfd_12 ()
{
        invalid_opcode_ddfd("DD/FD12");
}

void instruccion_ddfd_13 ()
{
        invalid_opcode_ddfd("DD/FD13");
}

void instruccion_ddfd_14 ()
{
        invalid_opcode_ddfd("DD/FD14");
}

void instruccion_ddfd_15 ()
{
        invalid_opcode_ddfd("DD/FD15");
}

void instruccion_ddfd_16 ()
{
        invalid_opcode_ddfd("DD/FD16");
}

void instruccion_ddfd_17 ()
{
        invalid_opcode_ddfd("DD/FD17");
}

void instruccion_ddfd_18 ()
{
        invalid_opcode_ddfd("DD/FD18");
}

void instruccion_ddfd_19 ()
{
        invalid_opcode_ddfd("DD/FD19");
}

void instruccion_ddfd_20 ()
{
        invalid_opcode_ddfd("DD/FD20");
}

void instruccion_ddfd_21 ()
{
        invalid_opcode_ddfd("DD/FD21");
}

void instruccion_ddfd_22 ()
{
        invalid_opcode_ddfd("DD/FD22");
}

void instruccion_ddfd_23 ()
{
        invalid_opcode_ddfd("DD/FD23");
}

void instruccion_ddfd_24 ()
{
        invalid_opcode_ddfd("DD/FD24");
}


void instruccion_ddfd_25 ()
{

//ADD IX,DE
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );

        z80_int reg_ixiy;

        reg_ixiy=*registro_ixiy;

        reg_ixiy=add_16bit(reg_ixiy,DE);

        *registro_ixiy=reg_ixiy;


}

void instruccion_ddfd_26 ()
{
        invalid_opcode_ddfd("DD/FD26");
}

void instruccion_ddfd_27 ()
{
        invalid_opcode_ddfd("DD/FD27");
}

void instruccion_ddfd_28 ()
{
        invalid_opcode_ddfd("DD/FD28");
}

void instruccion_ddfd_29 ()
{
        invalid_opcode_ddfd("DD/FD29");
}

void instruccion_ddfd_30 ()
{
        invalid_opcode_ddfd("DD/FD30");
}

void instruccion_ddfd_31 ()
{
        invalid_opcode_ddfd("DD/FD31");
}

void instruccion_ddfd_32 ()
{
        invalid_opcode_ddfd("DD/FD32");
}


void instruccion_ddfd_33 ()
{

//LD IY,NN
	*registro_ixiy=lee_word_pc();
}

void instruccion_ddfd_34 ()
{
//LD (NN),IX
	z80_int dir;

	dir=lee_word_pc();

	poke_word(dir,*registro_ixiy);

	set_memptr(dir+1);

}

void instruccion_ddfd_35 ()
{
//INC IX
        contend_read_no_mreq( IR, 1 );
        contend_read_no_mreq( IR, 1 );


        z80_int reg;

        reg=*registro_ixiy;

        //reg=inc_16bit(reg);
	reg++;

	*registro_ixiy=reg;


}

void instruccion_ddfd_36 ()
{
//INC IXh
	z80_byte reg;
	z80_byte *p;

	p=(z80_byte *)registro_ixiy;
	p++;
	reg=*p;
        inc_8bit(reg);
	*p=reg;
}

void instruccion_ddfd_37 ()
{
//DEC IXh
        z80_byte reg;
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        reg=*p;
        dec_8bit(reg);
        *p=reg;
}

void instruccion_ddfd_38 ()
{
//LD IXh,N
        z80_byte reg;
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        reg=lee_byte_pc();
        *p=reg;

}

void instruccion_ddfd_39 ()
{
        invalid_opcode_ddfd("DD/FD39");
}

void instruccion_ddfd_40 ()
{
        invalid_opcode_ddfd("DD/FD40");
}


void instruccion_ddfd_41 ()
{
//coddd41:                        ;ADD IX,IX
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );


        z80_int reg_ixiy;

        reg_ixiy=*registro_ixiy;

        reg_ixiy=add_16bit(reg_ixiy,reg_ixiy);

        *registro_ixiy=reg_ixiy;


}

void instruccion_ddfd_42 ()
{
//coddd42:                        ;LD IX,(NN)

        z80_int dir;
        dir=lee_word_pc();

	*registro_ixiy=peek_word(dir);

	set_memptr(dir+1);

}

void instruccion_ddfd_43 ()
{
//DEC IX
        contend_read_no_mreq( IR, 1 );
        contend_read_no_mreq( IR, 1 );

        z80_int reg;

        reg=*registro_ixiy;

        //reg=dec_16bit(reg);
	reg--;

        *registro_ixiy=reg;

}

void instruccion_ddfd_44 ()
{
//INC IXl

        z80_byte reg;
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        reg=*p;
        inc_8bit(reg);
        *p=reg;


}

void instruccion_ddfd_45 ()
{
//DEC IXl

        z80_byte reg;
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        reg=*p;
        dec_8bit(reg);
        *p=reg;


}

void instruccion_ddfd_46 ()
{
//LD IXl,N

        z80_byte reg;
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        reg=lee_byte_pc();
        *p=reg;

}

void instruccion_ddfd_47 ()
{
        invalid_opcode_ddfd("DD/FD47");
}

void instruccion_ddfd_48 ()
{
        invalid_opcode_ddfd("DD/FD48");
}

void instruccion_ddfd_49 ()
{
        invalid_opcode_ddfd("DD/FD49");
}

void instruccion_ddfd_50 ()
{
        invalid_opcode_ddfd("DD/FD50");
}

void instruccion_ddfd_51 ()
{
        invalid_opcode_ddfd("DD/FD51");
}

void instruccion_ddfd_52 ()
{
//INC (IX+d)

        z80_byte valor_leido,desp;
        z80_int desp16,puntero;

	desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
	reg_pc++;

        desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );

        inc_8bit(valor_leido);
        poke_byte(puntero,valor_leido);

}

void instruccion_ddfd_53 ()
{
//DEC (IX+d)
        z80_byte valor_leido,desp;
        z80_int desp16,puntero;

        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;

        desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;

        valor_leido=peek_byte(puntero);
	contend_read_no_mreq( puntero, 1 );

        //valor_leido=dec_8bit(valor_leido);
        dec_8bit(valor_leido);
        poke_byte(puntero,valor_leido);

}

void instruccion_ddfd_54 ()
{
//LD (IX+d),N
        z80_byte valor_leido,desp;
        z80_int desp16,puntero;
        
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 ); 
	reg_pc++;

        desp16=desp8_to_16(desp);
        puntero=*registro_ixiy + desp16;
        
        valor_leido=lee_byte_pc();
        poke_byte(puntero,valor_leido);

}

void instruccion_ddfd_55 ()
{
        invalid_opcode_ddfd("DD/FD55");
}

void instruccion_ddfd_56 ()
{
        invalid_opcode_ddfd("DD/FD56");
}

void instruccion_ddfd_57 ()
{
//coddd57:                        ;ADD IX,SP
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );


        z80_int reg_ixiy;

        reg_ixiy=*registro_ixiy;

        reg_ixiy=add_16bit(reg_ixiy,reg_sp);

        *registro_ixiy=reg_ixiy;


}

void instruccion_ddfd_58 ()
{
        invalid_opcode_ddfd("DD/FD58");
}

void instruccion_ddfd_59 ()
{
        invalid_opcode_ddfd("DD/FD59");
}

void instruccion_ddfd_60 ()
{
        invalid_opcode_ddfd("DD/FD60");
}

void instruccion_ddfd_61 ()
{
        invalid_opcode_ddfd("DD/FD61");
}

void instruccion_ddfd_62 ()
{
        invalid_opcode_ddfd("DD/FD62");
}

void instruccion_ddfd_63 ()
{
        invalid_opcode_ddfd("DD/FD63");
}

void instruccion_ddfd_64 ()
{
        invalid_opcode_ddfd("DD/FD64");
}

void instruccion_ddfd_65 ()
{
        invalid_opcode_ddfd("DD/FD65");
}

void instruccion_ddfd_66 ()
{
        invalid_opcode_ddfd("DD/FD66");
}

void instruccion_ddfd_67 ()
{
        invalid_opcode_ddfd("DD/FD67");
}

void instruccion_ddfd_68 ()
{
//LD B,IXh

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        reg_b=*p;

}

void instruccion_ddfd_69 ()
{
//LD B,IXl

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        reg_b=*p;

}

void instruccion_ddfd_70 ()
{
//LD B,(IX+d)

        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	reg_pc++;
	reg_b = peek_byte_desp(*registro_ixiy,desp);


}

void instruccion_ddfd_71 ()
{
        invalid_opcode_ddfd("DD/FD71");
}

void instruccion_ddfd_72 ()
{
        invalid_opcode_ddfd("DD/FD72");
}

void instruccion_ddfd_73 ()
{
        invalid_opcode_ddfd("DD/FD73");
}

void instruccion_ddfd_74 ()
{
        invalid_opcode_ddfd("DD/FD74");
}

void instruccion_ddfd_75 ()
{
        invalid_opcode_ddfd("DD/FD75");
}

void instruccion_ddfd_76 ()
{
//LD C,IXh

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        reg_c=*p;

}

void instruccion_ddfd_77 ()
{
//LD C,IXl

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        reg_c=*p;

}

void instruccion_ddfd_78 ()
{
//LD C,(IX+d)

        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
        reg_pc++;
        reg_c = peek_byte_desp(*registro_ixiy,desp);


}

void instruccion_ddfd_79 ()
{
        invalid_opcode_ddfd("DD/FD79");
}

void instruccion_ddfd_80 ()
{
        invalid_opcode_ddfd("DD/FD80");
}

void instruccion_ddfd_81 ()
{
        invalid_opcode_ddfd("DD/FD81");
}

void instruccion_ddfd_82 ()
{
        invalid_opcode_ddfd("DD/FD82");
}

void instruccion_ddfd_83 ()
{
        invalid_opcode_ddfd("DD/FD83");
}


void instruccion_ddfd_84 ()
{
//LD D,IXh

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        reg_d=*p;

}

void instruccion_ddfd_85 ()
{
//LD D,IXl

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        reg_d=*p;

}



void instruccion_ddfd_86 ()
{
//LD D,(IX+d)

        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
        reg_pc++;
        reg_d = peek_byte_desp(*registro_ixiy,desp);


}

void instruccion_ddfd_87 ()
{
        invalid_opcode_ddfd("DD/FD87");
}

void instruccion_ddfd_88 ()
{
        invalid_opcode_ddfd("DD/FD88");
}

void instruccion_ddfd_89 ()
{
        invalid_opcode_ddfd("DD/FD89");
}

void instruccion_ddfd_90 ()
{
        invalid_opcode_ddfd("DD/FD90");
}

void instruccion_ddfd_91 ()
{
        invalid_opcode_ddfd("DD/FD91");
}

void instruccion_ddfd_92 ()
{
//LD E,IXh

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        reg_e=*p;

}

void instruccion_ddfd_93 ()
{
//LD E,IXl

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        reg_e=*p;

}


void instruccion_ddfd_94 ()
{
//LD E,(IX+d)

        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
        reg_pc++;
        reg_e = peek_byte_desp(*registro_ixiy,desp);


}

void instruccion_ddfd_95 ()
{
        invalid_opcode_ddfd("DD/FD95");
}



void instruccion_ddfd_96 ()
{
//LD IXh,B
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        *p=reg_b;
}

void instruccion_ddfd_97 ()
{
//LD IXh,C
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        *p=reg_c;
}

void instruccion_ddfd_98 ()
{
//LD IXh,D
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        *p=reg_d;
}

void instruccion_ddfd_99 ()
{
//LD IXh,E
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        *p=reg_e;
}


void instruccion_ddfd_100 ()
{
//LD IXh,IXh
}

void instruccion_ddfd_101 ()
{
//LD IXh,IXl
        z80_byte *q;

        q=(z80_byte *)registro_ixiy;

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        *p=*q;
}


void instruccion_ddfd_102 ()
{
//LD H,(IX+d)

        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
        reg_pc++;
        reg_h = peek_byte_desp(*registro_ixiy,desp);


}

void instruccion_ddfd_103 ()
{
//LD IXh,A
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        *p=reg_a;


}

void instruccion_ddfd_104 ()
{
//LD IXl,B
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        *p=reg_b;
}

void instruccion_ddfd_105 ()
{
//LD IXl,C
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        *p=reg_c;
}

void instruccion_ddfd_106 ()
{
//LD IXl,D
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        *p=reg_d;
}

void instruccion_ddfd_107 ()
{
//LD IXl,E
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        *p=reg_e;
}


void instruccion_ddfd_108 ()
{
//LD IXl,IXh
        z80_byte *q;

        q=(z80_byte *)registro_ixiy;
        q++;


        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        *p=*q;


}

void instruccion_ddfd_109 ()
{
//LD IXl,IXl
}

void instruccion_ddfd_110 ()
{
//LD L,(IX+d)

        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
        reg_pc++;
        reg_l = peek_byte_desp(*registro_ixiy,desp);


}

void instruccion_ddfd_111 ()
{
//LD IXl,A
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        *p=reg_a;


}

void instruccion_ddfd_112 ()
{
//LD (IX+d),B

        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
        reg_pc++;

        poke_byte_desp(*registro_ixiy,desp,reg_b);

}

void instruccion_ddfd_113 ()
{
//LD (IX+d),C
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 	
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;

        poke_byte_desp(*registro_ixiy,desp,reg_c);

}

void instruccion_ddfd_114 ()
{
//LD (IX+d),D
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;

        poke_byte_desp(*registro_ixiy,desp,reg_d);

}

void instruccion_ddfd_115 ()
{
//LD (IX+d),E
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;

        poke_byte_desp(*registro_ixiy,desp,reg_e);

}

void instruccion_ddfd_116 ()
{
//LD (IX+d),H
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;

        poke_byte_desp(*registro_ixiy,desp,reg_h);

}

void instruccion_ddfd_117 ()
{
//LD (IX+d),L
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;

        poke_byte_desp(*registro_ixiy,desp,reg_l);

}

void instruccion_ddfd_118 ()
{
        invalid_opcode_ddfd("DD/FD118");
}



void instruccion_ddfd_119 ()
{
//LD (IX+d),A
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;

        poke_byte_desp(*registro_ixiy,desp,reg_a);


}

void instruccion_ddfd_120 ()
{
        invalid_opcode_ddfd("DD/FD120");
}

void instruccion_ddfd_121 ()
{
        invalid_opcode_ddfd("DD/FD121");
}

void instruccion_ddfd_122 ()
{
        invalid_opcode_ddfd("DD/FD122");
}

void instruccion_ddfd_123 ()
{
        invalid_opcode_ddfd("DD/FD123");
}


void instruccion_ddfd_124 ()
{
//LD A,IXh

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        reg_a=*p;

}


void instruccion_ddfd_125 ()
{
//LD A,IXl

        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        reg_a=*p;

}

void instruccion_ddfd_126 ()
{
//LD A,(IX+d)

        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
        reg_pc++;
        reg_a = peek_byte_desp(*registro_ixiy,desp);


}

void instruccion_ddfd_127 ()
{
        invalid_opcode_ddfd("DD/FD127");
}

void instruccion_ddfd_128 ()
{
        invalid_opcode_ddfd("DD/FD128");
}

void instruccion_ddfd_129 ()
{
        invalid_opcode_ddfd("DD/FD129");
}

void instruccion_ddfd_130 ()
{
        invalid_opcode_ddfd("DD/FD130");
}

void instruccion_ddfd_131 ()
{
        invalid_opcode_ddfd("DD/FD131");
}

void instruccion_ddfd_132()
{
//ADD A,IXh
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        add_a_reg(*p);
}

void instruccion_ddfd_133()
{
//ADD A,IXl
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        add_a_reg(*p);
}



void instruccion_ddfd_134 ()
{
//ADD A,(IX+d)
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;
	add_a_reg( peek_byte_desp(*registro_ixiy,desp));

}

void instruccion_ddfd_135 ()
{
        invalid_opcode_ddfd("DD/FD135");
}

void instruccion_ddfd_136 ()
{
        invalid_opcode_ddfd("DD/FD136");
}

void instruccion_ddfd_137 ()
{
        invalid_opcode_ddfd("DD/FD137");
}

void instruccion_ddfd_138 ()
{
        invalid_opcode_ddfd("DD/FD138");
}

void instruccion_ddfd_139 ()
{
        invalid_opcode_ddfd("DD/FD139");
}


void instruccion_ddfd_140()
{
//ADC A,IXh
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        adc_a_reg(*p);
}

void instruccion_ddfd_141()
{
//ADC A,IXl
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        adc_a_reg(*p);
}

void instruccion_ddfd_142 ()
{
//ADC A,(IX+d)
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;
        adc_a_reg( peek_byte_desp(*registro_ixiy,desp));

}

void instruccion_ddfd_143 ()
{
        invalid_opcode_ddfd("DD/FD143");
}

void instruccion_ddfd_144 ()
{
        invalid_opcode_ddfd("DD/FD144");
}

void instruccion_ddfd_145 ()
{
        invalid_opcode_ddfd("DD/FD145");
}

void instruccion_ddfd_146 ()
{
        invalid_opcode_ddfd("DD/FD146");
}

void instruccion_ddfd_147 ()
{
        invalid_opcode_ddfd("DD/FD147");
}

void instruccion_ddfd_148()
{
//SUB IXh
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        sub_a_reg(*p);
}

void instruccion_ddfd_149()
{
//SUB IXl
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        sub_a_reg(*p);
}

void instruccion_ddfd_150 ()
{
//SUB (IX+d)
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;
        sub_a_reg( peek_byte_desp(*registro_ixiy,desp));

}

void instruccion_ddfd_151 ()
{
        invalid_opcode_ddfd("DD/FD151");
}

void instruccion_ddfd_152 ()
{
        invalid_opcode_ddfd("DD/FD152");
}

void instruccion_ddfd_153 ()
{
        invalid_opcode_ddfd("DD/FD153");
}

void instruccion_ddfd_154 ()
{
        invalid_opcode_ddfd("DD/FD154");
}

void instruccion_ddfd_155 ()
{
        invalid_opcode_ddfd("DD/FD155");
}

void instruccion_ddfd_156()
{
//SBC A,IXh
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        sbc_a_reg(*p);
}

void instruccion_ddfd_157()
{
//SBC A,IXl
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        sbc_a_reg(*p);
}

void instruccion_ddfd_158 ()
{
//SBC A,(IX+d)
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;
        sbc_a_reg( peek_byte_desp(*registro_ixiy,desp));

}

void instruccion_ddfd_159 ()
{
        invalid_opcode_ddfd("DD/FD159");
}

void instruccion_ddfd_160 ()
{
        invalid_opcode_ddfd("DD/FD160");
}

void instruccion_ddfd_161 ()
{
        invalid_opcode_ddfd("DD/FD161");
}

void instruccion_ddfd_162 ()
{
        invalid_opcode_ddfd("DD/FD162");
}

void instruccion_ddfd_163 ()
{
        invalid_opcode_ddfd("DD/FD163");
}


void instruccion_ddfd_164()
{
//AND IXh
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        and_reg(*p);
}

void instruccion_ddfd_165()
{
//AND IXl
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        and_reg(*p);
}

void instruccion_ddfd_166 ()
{
//AND (IX+d)
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;
        and_reg( peek_byte_desp(*registro_ixiy,desp));

}

void instruccion_ddfd_167 ()
{
        invalid_opcode_ddfd("DD/FD167");
}

void instruccion_ddfd_168 ()
{
        invalid_opcode_ddfd("DD/FD168");
}

void instruccion_ddfd_169 ()
{
        invalid_opcode_ddfd("DD/FD169");
}

void instruccion_ddfd_170 ()
{
        invalid_opcode_ddfd("DD/FD170");
}

void instruccion_ddfd_171 ()
{
        invalid_opcode_ddfd("DD/FD171");
}

void instruccion_ddfd_172()
{
//XOR IXh
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        xor_reg(*p);
}

void instruccion_ddfd_173()
{
//XOR IXl
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        xor_reg(*p);
}




void instruccion_ddfd_174 ()
{
//XOR (IX+d)
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;
        xor_reg( peek_byte_desp(*registro_ixiy,desp));

}

void instruccion_ddfd_175 ()
{
        invalid_opcode_ddfd("DD/FD175");
}

void instruccion_ddfd_176 ()
{
        invalid_opcode_ddfd("DD/FD176");
}

void instruccion_ddfd_177 ()
{
        invalid_opcode_ddfd("DD/FD177");
}

void instruccion_ddfd_178 ()
{
        invalid_opcode_ddfd("DD/FD178");
}

void instruccion_ddfd_179 ()
{
        invalid_opcode_ddfd("DD/FD179");
}

void instruccion_ddfd_180()
{
//OR IXh
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        or_reg(*p);
}

void instruccion_ddfd_181()
{
//OR IXl
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        or_reg(*p);
}


void instruccion_ddfd_182 ()
{
//OR (IX+d)
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;
        or_reg( peek_byte_desp(*registro_ixiy,desp));

}

void instruccion_ddfd_183 ()
{
        invalid_opcode_ddfd("DD/FD183");
}

void instruccion_ddfd_184 ()
{
        invalid_opcode_ddfd("DD/FD184");
}

void instruccion_ddfd_185 ()
{
        invalid_opcode_ddfd("DD/FD185");
}

void instruccion_ddfd_186 ()
{
        invalid_opcode_ddfd("DD/FD186");
}

void instruccion_ddfd_187 ()
{
        invalid_opcode_ddfd("DD/FD187");
}


void instruccion_ddfd_188()
{
//CP IXh
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        p++;
        cp_reg(*p);
}

void instruccion_ddfd_189()
{
//CP IXl
        z80_byte *p;

        p=(z80_byte *)registro_ixiy;
        cp_reg(*p);
}

void instruccion_ddfd_190 ()
{
//CP (IX+d)
        z80_byte desp;
        desp=peek_byte(reg_pc);
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 ); 
	contend_read_no_mreq( reg_pc, 1 );
        contend_read_no_mreq( reg_pc, 1 );
        reg_pc++;
        cp_reg( peek_byte_desp(*registro_ixiy,desp));

}

void instruccion_ddfd_191 ()
{
        invalid_opcode_ddfd("DD/FD191");
}

void instruccion_ddfd_192 ()
{
        invalid_opcode_ddfd("DD/FD192");
}

void instruccion_ddfd_193 ()
{
        invalid_opcode_ddfd("DD/FD193");
}

void instruccion_ddfd_194 ()
{
        invalid_opcode_ddfd("DD/FD194");
}

void instruccion_ddfd_195 ()
{
        invalid_opcode_ddfd("DD/FD195");
}

void instruccion_ddfd_196 ()
{
        invalid_opcode_ddfd("DD/FD196");
}

void instruccion_ddfd_197 ()
{
        invalid_opcode_ddfd("DD/FD197");
}

void instruccion_ddfd_198 ()
{
        invalid_opcode_ddfd("DD/FD198");
}

void instruccion_ddfd_199 ()
{
        invalid_opcode_ddfd("DD/FD199");
}

void instruccion_ddfd_200 ()
{
        invalid_opcode_ddfd("DD/FD200");
}

void instruccion_ddfd_201 ()
{
        invalid_opcode_ddfd("DD/FD201");
}

void instruccion_ddfd_202 ()
{
        invalid_opcode_ddfd("DD/FD202");
}


//Variable solo usada en instruccion_ddfd_203. La dejamos aqui para no tenerla que asignar siempre en el stack
z80_byte pref_ddfd_203_opcode_leido;

void instruccion_ddfd_203 ()
{

	z80_byte *registro;
	z80_byte numerobit;

//Prefijo DD o FD + CB

	        contend_read(reg_pc, 3);
		offset_xycb=peek_byte_no_time(reg_pc);
		reg_pc++;
		contend_read( reg_pc, 3 );
		pref_ddfd_203_opcode_leido=peek_byte_no_time(reg_pc);

#ifdef EMULATE_CPU_STATS
//Ver si es DD o FD
		//printf ("%x ix %x iy %x\n",registro_ixiy,&reg_ix,&reg_iy);
		if (registro_ixiy==&reg_ix) {
			util_stats_increment_counter(stats_codprddcb,pref_ddfd_203_opcode_leido);
		}
                else {
			util_stats_increment_counter(stats_codprfdcb,pref_ddfd_203_opcode_leido);
		}
#endif


                contend_read_no_mreq( reg_pc, 1 ); 
		contend_read_no_mreq( reg_pc, 1 );
                reg_pc++;


		switch (pref_ddfd_203_opcode_leido & 192) {
			case 64:
                                //printf ("aquibit ");
                                registro=devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7);
                                numerobit=(pref_ddfd_203_opcode_leido >> 3) & 7;
                                bit_bit_ixiy_desp_reg(numerobit,offset_xycb);
			break;

			case 128:
				//printf ("aquires ");
				registro=devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7);	
				numerobit=(pref_ddfd_203_opcode_leido >> 3) & 7;
				res_bit_ixiy_desp_reg(numerobit,offset_xycb,registro);
			break;
			case 192:
		                //printf ("aquiset ");
		                registro=devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7);
		                numerobit=(pref_ddfd_203_opcode_leido >> 3) & 7;
		                set_bit_ixiy_desp_reg(numerobit,offset_xycb,registro);
			break;

			default:
				switch(pref_ddfd_203_opcode_leido & 56) {

					case 0:
						rlc_ixiy_desp_reg(offset_xycb,devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7) );
					break;

					case 8:
						rrc_ixiy_desp_reg(offset_xycb,devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7) );
					break;

					case 16:
						rl_ixiy_desp_reg(offset_xycb,devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7) );
					break;

					case 24:
						rr_ixiy_desp_reg(offset_xycb,devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7) );
					break;

					case 32:
						sla_ixiy_desp_reg(offset_xycb,devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7) );
					break;

					case 40:
						sra_ixiy_desp_reg(offset_xycb,devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7) );
					break;

					case 48:
						sls_ixiy_desp_reg(offset_xycb,devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7) );
					break;

					case 56:
						srl_ixiy_desp_reg(offset_xycb,devuelve_reg_offset(pref_ddfd_203_opcode_leido & 7) );
					break;


					default:
					//Aqui no deberia llegar nunca
					cpu_panic ("Opcode DD/FD + CB. Invalid mask instruction");
					break;
				}
			break;

		}
		

}

void instruccion_ddfd_204 ()
{
        invalid_opcode_ddfd("DD/FD204");
}

void instruccion_ddfd_205 ()
{
        invalid_opcode_ddfd("DD/FD205");
}

void instruccion_ddfd_206 ()
{
        invalid_opcode_ddfd("DD/FD206");
}

void instruccion_ddfd_207 ()
{
        invalid_opcode_ddfd("DD/FD207");
}

void instruccion_ddfd_208 ()
{
        invalid_opcode_ddfd("DD/FD208");
}

void instruccion_ddfd_209 ()
{
        invalid_opcode_ddfd("DD/FD209");
}

void instruccion_ddfd_210 ()
{
        invalid_opcode_ddfd("DD/FD210");
}

void instruccion_ddfd_211 ()
{
        invalid_opcode_ddfd("DD/FD211");
}

void instruccion_ddfd_212 ()
{
        invalid_opcode_ddfd("DD/FD212");
}

void instruccion_ddfd_213 ()
{
        invalid_opcode_ddfd("DD/FD213");
}

void instruccion_ddfd_214 ()
{
        invalid_opcode_ddfd("DD/FD214");
}

void instruccion_ddfd_215 ()
{
        invalid_opcode_ddfd("DD/FD215");
}

void instruccion_ddfd_216 ()
{
        invalid_opcode_ddfd("DD/FD216");
}

void instruccion_ddfd_217 ()
{
        invalid_opcode_ddfd("DD/FD217");
}

void instruccion_ddfd_218 ()
{
        invalid_opcode_ddfd("DD/FD218");
}

void instruccion_ddfd_219 ()
{
        invalid_opcode_ddfd("DD/FD219");
}

void instruccion_ddfd_220 ()
{
        invalid_opcode_ddfd("DD/FD220");
}

void instruccion_ddfd_221 ()
{
        invalid_opcode_ddfd("DD/FD221");
}

void instruccion_ddfd_222 ()
{
        invalid_opcode_ddfd("DD/FD222");
}

void instruccion_ddfd_223 ()
{
        invalid_opcode_ddfd("DD/FD223");
}

void instruccion_ddfd_224 ()
{
        invalid_opcode_ddfd("DD/FD224");
}

void instruccion_ddfd_225 ()
{
//POP IX
	*registro_ixiy=pop_valor();
}

void instruccion_ddfd_226 ()
{
	invalid_opcode_ddfd("DD/FD226");
}

void instruccion_ddfd_227 ()
{
//EX (SP),IX

        z80_int valor;


        valor=peek_word(reg_sp);
	contend_read_no_mreq( reg_sp + 1, 1 );
        poke_word(reg_sp,*registro_ixiy);
	contend_write_no_mreq( reg_sp, 1 ); 
	contend_write_no_mreq( reg_sp, 1 );

	*registro_ixiy=valor;
	set_memptr(valor);


}

void instruccion_ddfd_228 ()
{
	invalid_opcode_ddfd("DD/FD228");
}

void instruccion_ddfd_229 ()
{
//PUSH IX
	contend_read_no_mreq( IR, 1 );
	push_valor(*registro_ixiy);
}

void instruccion_ddfd_230 ()
{
	invalid_opcode_ddfd("DD/FD230");
}

void instruccion_ddfd_231 ()
{
	invalid_opcode_ddfd("DD/FD231");
}

void instruccion_ddfd_232 ()
{
	invalid_opcode_ddfd("DD/FD232");
}

void instruccion_ddfd_233 ()
{
//JP (IX)
        reg_pc=*registro_ixiy;


}

void instruccion_ddfd_234 ()
{
	invalid_opcode_ddfd("DD/FD234");
}

void instruccion_ddfd_235 ()
{
//coddd235:               ;EX DE,IX

	z80_int reg_temp_de,reg_temp_de_orig;

	reg_temp_de_orig=DE;

	reg_temp_de=*registro_ixiy;
	DE=reg_temp_de;

	*registro_ixiy=reg_temp_de_orig;


}

void instruccion_ddfd_236 ()
{
        invalid_opcode_ddfd("DD/FD236");
}

void instruccion_ddfd_237 ()
{
        invalid_opcode_ddfd("DD/FD237");
}

void instruccion_ddfd_238 ()
{
        invalid_opcode_ddfd("DD/FD238");
}

void instruccion_ddfd_239 ()
{
        invalid_opcode_ddfd("DD/FD239");
}

void instruccion_ddfd_240 ()
{
        invalid_opcode_ddfd("DD/FD240");
}

void instruccion_ddfd_241 ()
{
        invalid_opcode_ddfd("DD/FD241");
}

void instruccion_ddfd_242 ()
{
        invalid_opcode_ddfd("DD/FD242");
}

void instruccion_ddfd_243 ()
{
        invalid_opcode_ddfd("DD/FD243");
}

void instruccion_ddfd_244 ()
{
        invalid_opcode_ddfd("DD/FD244");
}

void instruccion_ddfd_245 ()
{
        invalid_opcode_ddfd("DD/FD245");
}

void instruccion_ddfd_246 ()
{
        invalid_opcode_ddfd("DD/FD246");
}

void instruccion_ddfd_247 ()
{
        invalid_opcode_ddfd("DD/FD247");
}

void instruccion_ddfd_248 ()
{
        invalid_opcode_ddfd("DD/FD248");
}

void instruccion_ddfd_249 ()
{
//coddd249:               ;LD SP,IX
      contend_read_no_mreq( IR, 1 );
      contend_read_no_mreq( IR, 1 );

	reg_sp=*registro_ixiy;
}

void instruccion_ddfd_250 ()
{
        invalid_opcode_ddfd("DD/FD250");
}

void instruccion_ddfd_251 ()
{
        invalid_opcode_ddfd("DD/FD251");
}

void instruccion_ddfd_252 ()
{
        invalid_opcode_ddfd("DD/FD252");
}

void instruccion_ddfd_253 ()
{
        invalid_opcode_ddfd("DD/FD253");
}

void instruccion_ddfd_254 ()
{
        invalid_opcode_ddfd("DD/FD254");
}

void instruccion_ddfd_255 ()
{
        invalid_opcode_ddfd("DD/FD255");
}




void (*codprddfd[]) ()   = { 
instruccion_ddfd_0,
instruccion_ddfd_1,
instruccion_ddfd_2,
instruccion_ddfd_3,
instruccion_ddfd_4,
instruccion_ddfd_5,
instruccion_ddfd_6,
instruccion_ddfd_7,
instruccion_ddfd_8,
instruccion_ddfd_9,
instruccion_ddfd_10,
instruccion_ddfd_11,
instruccion_ddfd_12,
instruccion_ddfd_13,
instruccion_ddfd_14,
instruccion_ddfd_15,
instruccion_ddfd_16,
instruccion_ddfd_17,
instruccion_ddfd_18,
instruccion_ddfd_19,
instruccion_ddfd_20,
instruccion_ddfd_21,
instruccion_ddfd_22,
instruccion_ddfd_23,
instruccion_ddfd_24,
instruccion_ddfd_25,
instruccion_ddfd_26,
instruccion_ddfd_27,
instruccion_ddfd_28,
instruccion_ddfd_29,
instruccion_ddfd_30,
instruccion_ddfd_31,
instruccion_ddfd_32,
instruccion_ddfd_33,
instruccion_ddfd_34,
instruccion_ddfd_35,
instruccion_ddfd_36,
instruccion_ddfd_37,
instruccion_ddfd_38,
instruccion_ddfd_39,
instruccion_ddfd_40,
instruccion_ddfd_41,
instruccion_ddfd_42,
instruccion_ddfd_43,
instruccion_ddfd_44,
instruccion_ddfd_45,
instruccion_ddfd_46,
instruccion_ddfd_47,
instruccion_ddfd_48,
instruccion_ddfd_49,
instruccion_ddfd_50,
instruccion_ddfd_51,
instruccion_ddfd_52,
instruccion_ddfd_53,
instruccion_ddfd_54,
instruccion_ddfd_55,
instruccion_ddfd_56,
instruccion_ddfd_57,
instruccion_ddfd_58,
instruccion_ddfd_59,
instruccion_ddfd_60,
instruccion_ddfd_61,
instruccion_ddfd_62,
instruccion_ddfd_63,
instruccion_ddfd_64,
instruccion_ddfd_65,
instruccion_ddfd_66,
instruccion_ddfd_67,
instruccion_ddfd_68,
instruccion_ddfd_69,
instruccion_ddfd_70,
instruccion_ddfd_71,
instruccion_ddfd_72,
instruccion_ddfd_73,
instruccion_ddfd_74,
instruccion_ddfd_75,
instruccion_ddfd_76,
instruccion_ddfd_77,
instruccion_ddfd_78,
instruccion_ddfd_79,
instruccion_ddfd_80,
instruccion_ddfd_81,
instruccion_ddfd_82,
instruccion_ddfd_83,
instruccion_ddfd_84,
instruccion_ddfd_85,
instruccion_ddfd_86,
instruccion_ddfd_87,
instruccion_ddfd_88,
instruccion_ddfd_89,
instruccion_ddfd_90,
instruccion_ddfd_91,
instruccion_ddfd_92,
instruccion_ddfd_93,
instruccion_ddfd_94,
instruccion_ddfd_95,
instruccion_ddfd_96,
instruccion_ddfd_97,
instruccion_ddfd_98,
instruccion_ddfd_99,
instruccion_ddfd_100,
instruccion_ddfd_101,
instruccion_ddfd_102,
instruccion_ddfd_103,
instruccion_ddfd_104,
instruccion_ddfd_105,
instruccion_ddfd_106,
instruccion_ddfd_107,
instruccion_ddfd_108,
instruccion_ddfd_109,
instruccion_ddfd_110,
instruccion_ddfd_111,
instruccion_ddfd_112,
instruccion_ddfd_113,
instruccion_ddfd_114,
instruccion_ddfd_115,
instruccion_ddfd_116,
instruccion_ddfd_117,
instruccion_ddfd_118,
instruccion_ddfd_119,
instruccion_ddfd_120,
instruccion_ddfd_121,
instruccion_ddfd_122,
instruccion_ddfd_123,
instruccion_ddfd_124,
instruccion_ddfd_125,
instruccion_ddfd_126,
instruccion_ddfd_127,
instruccion_ddfd_128,
instruccion_ddfd_129,
instruccion_ddfd_130,
instruccion_ddfd_131,
instruccion_ddfd_132,
instruccion_ddfd_133,
instruccion_ddfd_134,
instruccion_ddfd_135,
instruccion_ddfd_136,
instruccion_ddfd_137,
instruccion_ddfd_138,
instruccion_ddfd_139,
instruccion_ddfd_140,
instruccion_ddfd_141,
instruccion_ddfd_142,
instruccion_ddfd_143,
instruccion_ddfd_144,
instruccion_ddfd_145,
instruccion_ddfd_146,
instruccion_ddfd_147,
instruccion_ddfd_148,
instruccion_ddfd_149,
instruccion_ddfd_150,
instruccion_ddfd_151,
instruccion_ddfd_152,
instruccion_ddfd_153,
instruccion_ddfd_154,
instruccion_ddfd_155,
instruccion_ddfd_156,
instruccion_ddfd_157,
instruccion_ddfd_158,
instruccion_ddfd_159,
instruccion_ddfd_160,
instruccion_ddfd_161,
instruccion_ddfd_162,
instruccion_ddfd_163,
instruccion_ddfd_164,
instruccion_ddfd_165,
instruccion_ddfd_166,
instruccion_ddfd_167,
instruccion_ddfd_168,
instruccion_ddfd_169,
instruccion_ddfd_170,
instruccion_ddfd_171,
instruccion_ddfd_172,
instruccion_ddfd_173,
instruccion_ddfd_174,
instruccion_ddfd_175,
instruccion_ddfd_176,
instruccion_ddfd_177,
instruccion_ddfd_178,
instruccion_ddfd_179,
instruccion_ddfd_180,
instruccion_ddfd_181,
instruccion_ddfd_182,
instruccion_ddfd_183,
instruccion_ddfd_184,
instruccion_ddfd_185,
instruccion_ddfd_186,
instruccion_ddfd_187,
instruccion_ddfd_188,
instruccion_ddfd_189,
instruccion_ddfd_190,
instruccion_ddfd_191,
instruccion_ddfd_192,
instruccion_ddfd_193,
instruccion_ddfd_194,
instruccion_ddfd_195,
instruccion_ddfd_196,
instruccion_ddfd_197,
instruccion_ddfd_198,
instruccion_ddfd_199,
instruccion_ddfd_200,
instruccion_ddfd_201,
instruccion_ddfd_202,
instruccion_ddfd_203,
instruccion_ddfd_204,
instruccion_ddfd_205,
instruccion_ddfd_206,
instruccion_ddfd_207,
instruccion_ddfd_208,
instruccion_ddfd_209,
instruccion_ddfd_210,
instruccion_ddfd_211,
instruccion_ddfd_212,
instruccion_ddfd_213,
instruccion_ddfd_214,
instruccion_ddfd_215,
instruccion_ddfd_216,
instruccion_ddfd_217,
instruccion_ddfd_218,
instruccion_ddfd_219,
instruccion_ddfd_220,
instruccion_ddfd_221,
instruccion_ddfd_222,
instruccion_ddfd_223,
instruccion_ddfd_224,
instruccion_ddfd_225,
instruccion_ddfd_226,
instruccion_ddfd_227,
instruccion_ddfd_228,
instruccion_ddfd_229,
instruccion_ddfd_230,
instruccion_ddfd_231,
instruccion_ddfd_232,
instruccion_ddfd_233,
instruccion_ddfd_234,
instruccion_ddfd_235,
instruccion_ddfd_236,
instruccion_ddfd_237,
instruccion_ddfd_238,
instruccion_ddfd_239,
instruccion_ddfd_240,
instruccion_ddfd_241,
instruccion_ddfd_242,
instruccion_ddfd_243,
instruccion_ddfd_244,
instruccion_ddfd_245,
instruccion_ddfd_246,
instruccion_ddfd_247,
instruccion_ddfd_248,
instruccion_ddfd_249,
instruccion_ddfd_250,
instruccion_ddfd_251,
instruccion_ddfd_252,
instruccion_ddfd_253,
instruccion_ddfd_254,
instruccion_ddfd_255

};
