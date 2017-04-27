#include <stdio.h>
#include <stdlib.h>

#include "68000_config.h"
#include "ql.h"

//compilar con : rm prueba_ql ; gcc 68000.c 68000_debug.c op68kadd.c op68karith.c op68ksub.c op68klogop.c op68kmisc.c op68kmove.c op68kshift.c 68000_additional.c ql.c prueba_ql.c -o prueba_ql

extern unsigned long   pc;


extern unsigned long   reg[16], *areg;         // areg later points to reg[8]
extern unsigned long   usp, ssp;
extern unsigned long   dfc, sfc, vbr;

extern void    disass(char *buf, uint16 *inst_stream);

char texto_disassemble[1000];


void loadrom(void)
{
        FILE *ptr_configfile;
        ptr_configfile=fopen("ql_js.rom","rb");
        //ptr_configfile=fopen("ql_jm.rom","rb");

        if (!ptr_configfile) {
                printf("Unable to open rom file\n");
                return ;
        }

        int leidos=fread(memoria_ql,1,49152,ptr_configfile);


        fclose(ptr_configfile);

}

int main()
{

	int refresca=0;

	memoria_ql=malloc(QL_MEM_LIMIT+1);

	//Inicializar a 0
	int i;
	for (i=0;i<QL_MEM_LIMIT+1;i++) memoria_ql[i]=0;

	loadrom();

pc=0;

HWReset();
		//print_regs();

	while (1==1) {
		//print_regs();
		//disassemble();
		CPURun(1);
		//sleep(1);

		
		refresca++;
		if (refresca==100000) {
			dump_screen();
			print_regs();
			disassemble();
		}

		if (refresca>100000) {
			disassemble();
			if (refresca==100010) refresca=0;
		}
		
	}

	return 0;
}
