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

#include <stdio.h>
#include <string.h>

#include "zxuno.h"
#include "cpu.h"
#include "debug.h"
#include "contend.h"
#include "mem128.h"
#include "utils.h"
#include "ula.h"
#include "screen.h"
#include "menu.h"
#include "divmmc.h"
#include "ay38912.h"
#include "ulaplus.h"


z80_byte last_port_FC3B=0;

//Primer registro, masterconf, bit 0 a 1 (modo bootm)
z80_byte zxuno_ports[256]={1};

//Primer registro, masterconf, bit 0 a 0 (no modo bootm)
//z80_byte zxuno_ports[256]={0};

//Emulacion de SPI Flash de Winbond W25X40BV
//http://zxuno.speccy.org/ficheros/spi_datasheet.pdf
//Datos de escritura al bus spi
z80_byte zxuno_spi_bus[8];
z80_byte zxuno_spi_bus_index=0;

int last_spi_write_address;
int last_spi_read_address;
z80_byte next_spi_read_byte;

/*Registro de estado de la spi flash

Bit 7: Status register protect (non-volatile)
Bit 6: Reserved
Bit 5: Top/Bottom write protect (non-volatile)
Bit 4,3,2: Block protect bits (non-volatile)
Bit 1: Write enable latch
Bit 0: Erase or write in progress

*/

z80_byte zxuno_spi_status_register=0;


//Denegar modo turbo en boot de bios
z80_bit zxuno_deny_turbo_bios_boot={0};


//Para radas offset registro 41h. Indica a 1 que se va a escribir byte alto de registro 41
z80_bit zxuno_radasoffset_high_byte={0};

//Este es el registro 41 h pero realmente es de 14 bits,
//cuando escribes en ese registro, lo primero que se escribe son los 8 bits menos significativos
//y luego, los 6 más significativos (en la segunda escritura, los dos bits más significativos se descartan)
z80_int zxuno_radasoffset=0;


//Direcciones donde estan cada pagina de ram. 512kb ram. 32 paginas
//con bootm=0, se comporta como spectrum +2a, y,
//rams 0-7 de +2a corresponden a las primeras 8 paginas de esta tabla
//roms 0-3 de +2a corresponden a las siguientes 4 paginas de esta tabla
z80_byte *zxuno_sram_mem_table[ZXUNO_SRAM_PAGES];

//Direcciones actuales mapeadas, con modo bootm a 1
z80_byte *zxuno_bootm_memory_paged[4];

//Direcciones actuales mapeadas, con modo bootm a 0
z80_byte *zxuno_no_bootm_memory_paged[4];


//Direcciones actuales mapeadas, modo nuevo sin tener que distinguir entre bootm a 0 o 1 
z80_byte *zxuno_memory_paged_new[4];


int zxuno_flash_must_flush_to_disk=0;


//Si las escrituras de la spi flash luego se hacen flush a disco
z80_bit zxuno_flash_write_to_disk_enable={0};


//Nombre de la flash. Si "", nombre y ruta por defecto
char zxuno_flash_spi_name[PATH_MAX]="";


//Paginas mapeadas en cada zona de RAM, en modo bootm=1. Se solamente usa en menu debug y breakpoints, no para el core de emulacion
//Si numero pagina >=128, numero pagina=numero pagina-128 y se trata de ROM. Si no, es RAM
z80_byte zxuno_debug_paginas_memoria_mapeadas_bootm[4];

//Aviso de operacion de flash spi en footer
int zxuno_flash_operating_counter=0;

/*
Para registro de COREID:
$FF	COREID	Lectura	Cada operación de lectura proporciona el siguiente carácter ASCII de la cadena que contiene la revisión actual del core del ZX-Uno. Cuando la cadena termina,lecturas posteriores emiten bytes con el valor 0 (al menos se emite uno de ellos) hasta que vuelve a comenzar la cadena. Este puntero vuelve a 0 automáticamente tras un reset o una escritura en el registro $FC3B. Los caracteres entregados que forman parte de la cadena son ASCII estándar imprimibles (códigos 32-127). Cualquier otro valor es indicativo de que este registro no está operativo.
*/
int zxuno_core_id_indice=0;
//char *zxuno_core_id_message="ZEsarUX Z80 Spectrum core";
char *zxuno_core_id_message="Z22-14072016";


void zxuno_spi_set_write_enable(void)
{
	zxuno_spi_status_register |=ZXUNO_SPI_WEL;
}

void zxuno_spi_clear_write_enable(void)
{
        //quitamos write enable de la spi flash zxuno
        zxuno_spi_status_register &=(255-ZXUNO_SPI_WEL);
}

int zxuno_spi_is_write_enabled(void)
{
	if (zxuno_spi_status_register & ZXUNO_SPI_WEL) return 1;
	return 0;
}

void zxuno_footer_print_flash_operating(void)
{
	if (zxuno_flash_operating_counter) {
		//color inverso
		menu_putstring_footer(WINDOW_FOOTER_ELEMENT_X_FLASH,1," FLASH ",WINDOW_FOOTER_PAPER,WINDOW_FOOTER_INK);
	}
}

void zxuno_footer_flash_operating(void)
{

	//Si ya esta activo, no volver a escribirlo. Porque ademas el menu_putstring_footer consumiria mucha cpu
	if (!zxuno_flash_operating_counter) {

		zxuno_flash_operating_counter=2;
		zxuno_footer_print_flash_operating();

	}
}

void delete_zxuno_flash_text(void)
{
	menu_putstring_footer(WINDOW_FOOTER_ELEMENT_X_FLASH,1,"       ",WINDOW_FOOTER_INK,WINDOW_FOOTER_PAPER);
}

void mem_set_normal_pages_zxuno(void)
{

	//En modo bootm a 1.

	//Los 16kb de rom del zxuno
	zxuno_bootm_memory_paged[0]=memoria_spectrum;

	//Paginas 5,2
	zxuno_bootm_memory_paged[1]=zxuno_sram_mem_table[5];
	zxuno_bootm_memory_paged[2]=zxuno_sram_mem_table[2];

	//Y pagina 0
	zxuno_bootm_memory_paged[3]=zxuno_sram_mem_table[0];




	//En modo bootm a 0
	//La pagina sram 8 (la primera rom del spectrum emulado)
	zxuno_no_bootm_memory_paged[0]=zxuno_sram_mem_table[8];

	//Paginas 5,2
	zxuno_no_bootm_memory_paged[1]=zxuno_sram_mem_table[5];
	zxuno_no_bootm_memory_paged[2]=zxuno_sram_mem_table[2];

	//Y pagina 0
	zxuno_no_bootm_memory_paged[3]=zxuno_sram_mem_table[0];


	//TODO
	contend_pages_actual[0]=0;
	contend_pages_actual[1]=contend_pages_128k_p2a[5];
	contend_pages_actual[2]=contend_pages_128k_p2a[2];
	contend_pages_actual[3]=contend_pages_128k_p2a[0];

	zxuno_debug_paginas_memoria_mapeadas_bootm[0]=128+0;
	zxuno_debug_paginas_memoria_mapeadas_bootm[1]=5;
	zxuno_debug_paginas_memoria_mapeadas_bootm[2]=2;
	zxuno_debug_paginas_memoria_mapeadas_bootm[3]=0;

	debug_paginas_memoria_mapeadas[0]=128+0;
	debug_paginas_memoria_mapeadas[1]=5;
	debug_paginas_memoria_mapeadas[2]=2;
	debug_paginas_memoria_mapeadas[3]=0;


}



void hard_reset_cpu_zxuno(void)
{
	//quitamos write enable de la spi flash zxuno
	zxuno_spi_status_register &=(255-ZXUNO_SPI_WEL);



	//resetear todos los bits de masterconf y dejar solo bootm
	zxuno_ports[0]=1;

	zxuno_ports[0x0C]=255;
	zxuno_ports[0x0D]=1;
	zxuno_ports[0x0E]=0;
	zxuno_ports[0x0F]=0;
	zxuno_ports[0x40]=0;

	zxuno_radasoffset_high_byte.v=0;
	zxuno_radasoffset=0;

	zxuno_ports[0x42]=0;

	//Y sincronizar parametros
        zxuno_set_emulator_setting_diven();
	zxuno_set_emulator_setting_disd();
        zxuno_set_emulator_setting_i2kb();
        zxuno_set_emulator_setting_timing();
        zxuno_set_emulator_setting_contend();
	zxuno_set_emulator_setting_devcontrol_diay();
	zxuno_set_emulator_setting_scandblctrl();
	zxuno_set_emulator_setting_ditimex();
	zxuno_set_emulator_setting_diulaplus();

	reset_cpu();

}


//Rutina mapeo pagina zxuno con modo bootm=1
void zxuno_page_ram(z80_byte bank)
{
	zxuno_bootm_memory_paged[3]=zxuno_sram_mem_table[bank];

	//contend_pages_actual[3]=contend_pages_128k_p2a[ramentra];
	zxuno_debug_paginas_memoria_mapeadas_bootm[3]=bank;
}

//Escribir 1 byte en la memoria spi
void zxuno_spi_page_program(int address,z80_byte valor_a_escribir)
{
	//Solo 1 MB de la spi
	int indice_spi=address & ZXUNO_SPI_SIZE_BYTES_MASK;

	memoria_spectrum[ (ZXUNO_ROM_SIZE+ZXUNO_SRAM_SIZE)*1024 + indice_spi ]=valor_a_escribir;

	zxuno_flash_must_flush_to_disk=1;

}

//Operacion de escritura a la flash spi
void zxuno_write_spi(z80_byte value)
{
	//Indicar actividad en la flash
	zxuno_footer_flash_operating();

	zxuno_spi_bus[zxuno_spi_bus_index]=value;
	if (zxuno_spi_bus_index<7) zxuno_spi_bus_index++;

	switch (zxuno_spi_bus[0]) {

		case 0x03:
			//Si instruccion es read data (3), mostrar direccion 24 bits
			//Ver si se han recibido los 24 bits
			if (zxuno_spi_bus_index==4) {
				last_spi_read_address=zxuno_spi_bus[1]*65536+zxuno_spi_bus[2]*256+zxuno_spi_bus[3];
				debug_printf (VERBOSE_DEBUG,"Write SPI. SPI Read data command, received start Address: 0x%06x",
					      last_spi_read_address);


				//Primer byte devuelve valor enviado aqui, no byte flash. O sea, el primer byte la bios lo descarta
				next_spi_read_byte=value;

				//zxuno_footer_flash_operating();
			}

			break;

		case 0x02:
			//Instruccion es page program (write)

			//Comprobacion de Write enable y puesta a 0
			if (zxuno_spi_bus_index==1) {
				if (!zxuno_spi_is_write_enabled() ) {
					debug_printf (VERBOSE_INFO,"Write SPI Page Program command but Write Enable Latch (WEL) is not enabled");
					//reseteamos indice
					zxuno_spi_bus_index=0;
				}
				//desactivamos write enable. Siguiente instruccion debera habilitarla
				zxuno_spi_clear_write_enable();
			}

			//Se ha recibido el ultimo de los 24 bits que indica direccion
			if (zxuno_spi_bus_index==4) {
				last_spi_write_address=zxuno_spi_bus[1]*65536+zxuno_spi_bus[2]*256+zxuno_spi_bus[3];
				debug_printf (VERBOSE_DEBUG,"Write SPI. SPI Page Program command, received start Adress: 0x%06x",last_spi_write_address);

			}

			//Se estan recibiendo los datos a escribir
			if (zxuno_spi_bus_index==5) {
				z80_byte valor_a_escribir=zxuno_spi_bus[4];
				debug_printf (VERBOSE_PARANOID,"Write SPI. SPI Page Program command, writing at Adress: 0x%06x Value: 0x%02x",
					last_spi_write_address,valor_a_escribir);

				//escribimos el valor
				zxuno_spi_page_program(last_spi_write_address,valor_a_escribir);

				last_spi_write_address++;

				//decrementamos el indice para que siempre se guarde el dato en posicion 4
				zxuno_spi_bus_index--;

				//zxuno_footer_flash_operating();
			}

			break;

		case 0x04:
			//Instruccion es write disable
			debug_printf (VERBOSE_DEBUG,"Write SPI. SPI Write Disable command.");
			zxuno_spi_clear_write_enable();
			break;

		case 0x06:
			//Instruccion es write enable
			debug_printf (VERBOSE_DEBUG,"Write SPI. SPI Write Enable command.");
			zxuno_spi_set_write_enable();
			break;

		case 0x20:
			//Instruccion es sector erase
			//Comprobacion de Write enable y puesta a 0
			if (zxuno_spi_bus_index==1) {
				if (!zxuno_spi_is_write_enabled() ) {
					debug_printf (VERBOSE_INFO,"Write SPI Sector Erase command but Write Enable Latch (WEL) is not enabled");
					//reseteamos indice
					zxuno_spi_bus_index=0;
				}
				//desactivamos write enable. Siguiente instruccion debera habilitarla
				zxuno_spi_clear_write_enable();
			}



			//Ver si se han recibido los 24 bits
			if (zxuno_spi_bus_index==4) {
				int sector_erase_address=zxuno_spi_bus[1]*65536+zxuno_spi_bus[2]*256+zxuno_spi_bus[3];
				debug_printf (VERBOSE_DEBUG,"Write SPI. SPI Sector Erase (4KB) command, received Address: 0x%06x",
					      sector_erase_address);

				//Hacemos esa direccion multiple de 4 kb
				sector_erase_address &=(0xFFFFFF-0xFFF);

				//Y mascara a tamanyo SPI
				sector_erase_address &= ZXUNO_SPI_SIZE_BYTES_MASK;
                                debug_printf (VERBOSE_DEBUG,"Write SPI. SPI Sector Erase (4KB) command, effective Address: 0x%06x",
                                              sector_erase_address);

				//Y borrar (poner a 255)
				int len;
				for (len=0;len<4096;len++) {
				        memoria_spectrum[ (ZXUNO_ROM_SIZE+ZXUNO_SRAM_SIZE)*1024 + sector_erase_address ]=255;
					debug_printf (VERBOSE_PARANOID,"Sector Erase in progress. Address: 0x%06x",sector_erase_address);
					sector_erase_address++;
				}

			        zxuno_flash_must_flush_to_disk=1;

			}

			//zxuno_footer_flash_operating();
			break;

		case 0x05:
			//Instruccion es read status register
			if (zxuno_spi_bus[0]==0x5) {
				debug_printf (VERBOSE_DEBUG,"Write SPI. SPI Read Status Register. ");
			}
			break;

		default:
			debug_printf (VERBOSE_DEBUG,"Write SPI. Command 0x%02X not implemented yet",zxuno_spi_bus[0]);
			break;

	}

}

//Operacion de lectura de la flash spi
z80_byte zxuno_read_spi(void)
{

	//Indicar actividad en la flash
	zxuno_footer_flash_operating();

	//debug_printf (VERBOSE_DEBUG,"Read SPI");
	z80_byte valor_leido;

	//Ver que instruccion se habia mandado a la spi
	switch (zxuno_spi_bus[0]) {


		case 0x03:
			//Lectura de datos
			valor_leido=next_spi_read_byte;


			//siguiente valor

			//Solo 1 MB de la spi
			int indice_spi=last_spi_read_address & ZXUNO_SPI_SIZE_BYTES_MASK;

			next_spi_read_byte = memoria_spectrum[ (ZXUNO_ROM_SIZE+ZXUNO_SRAM_SIZE)*1024 + indice_spi ];

			debug_printf (VERBOSE_PARANOID,"Read SPI. Read Data command. Address: 0x%06X Next Value: 0x%02X",
				      indice_spi,next_spi_read_byte);

			last_spi_read_address++;


			return valor_leido;
			break;

		case 0x05:
			//Read Status Register

			debug_printf (VERBOSE_DEBUG,"Read SPI. Read Status Register. Value: 0x%02X",zxuno_spi_status_register);
			return zxuno_spi_status_register;
			break;

		default:
			debug_printf (VERBOSE_DEBUG,"Read SPI. Command 0x%02X not implemented",zxuno_spi_bus[0]);
			return 0;
			break;

	}


}


//Lectura de puerto ZX-Uno
z80_byte zxuno_read_port(z80_int puerto)
{

	if (puerto==0xFC3B) {
		//Leer valor indice
		//printf ("In Port %x ZX-Uno read, value %d, PC after=%x\n",puerto,last_port_FC3B,reg_pc);
		return last_port_FC3B;
	}

	if (puerto==0xFD3B) {
		//Leer contenido de uno de los 256 posibles registros
		//printf ("In Port %x ZX-Uno read, register %d, value %d, PC after=%x\n",puerto,last_port_FC3B,zxuno_ports[last_port_FC3B],reg_pc);

		if (last_port_FC3B==2) {

			//Si esta bit LOCK, no permitir lectura de SPI
			if ((zxuno_ports[0]&128)==128) {
				debug_printf (VERBOSE_DEBUG,"LOCK bit set. Not allowed FLASHSPI read");
				return 255;
			}
			//printf ("Leyendo FLASHSPI\n");
			//devolver byte de la flash spi

			return zxuno_read_spi();


		}

		//Registro de Core ID
		else if (last_port_FC3B==0xFF) {
			//Retorna Core ID
			int len=strlen(zxuno_core_id_message);
			if (zxuno_core_id_indice==len) {
				zxuno_core_id_indice=0;
				return 0;
			}
			else {
				return zxuno_core_id_message[zxuno_core_id_indice++];
			}
		}

		return zxuno_ports[last_port_FC3B];
	}

	//Sera uno de los dos puertos... pero para que no se queje el compilador, hacemos return 0
	return 0;

}





//Escritura de puerto ZX-Uno
void zxuno_write_port(z80_int puerto, z80_byte value)
{
	if (puerto==0xFC3B) {
		//printf ("Out Port %x ZX-Uno written with value %x, PC after=%x\n",puerto,value,reg_pc);
		last_port_FC3B=value;
		zxuno_core_id_indice=0;
	}
	if (puerto==0xFD3B) {

		z80_byte anterior_masterconf=zxuno_ports[0];
		z80_byte anterior_devcontrol=zxuno_ports[0x0E];
		z80_byte anterior_devctrl2=zxuno_ports[0x0F];
		z80_byte anterior_scandblctrl=zxuno_ports[0x0B];
		//Si se va a tocar masterconf, ver cambios en bits, como por ejemplo I2KB

		//El indice al registro viene por last_port_FC3B
		//printf ("Out Port %x ZX-Uno written, index: %d with value %x, PC after=%x\n",puerto,last_port_FC3B,value,reg_pc);
		//Si Lock esta a 1, en masterconf, mascara de bits y avisar
		if (last_port_FC3B==0 && (zxuno_ports[0]&128)==128 ) {
			debug_printf (VERBOSE_DEBUG,"MASTERCONF lock bit set to 1");

			//Solo se pueden modificar bits 6,5,4,3
			//Hacemos mascara del valor nuevo quitando bits 7,0,1,2 (que son los que no se tocan)
			value=value&(255-128-1-2-4);

			z80_byte valor_puerto=zxuno_ports[0];

			//Valor actual de masterconf conservamos bits 7,0,1,2
			valor_puerto = valor_puerto & (128+1+2+4);

			//Y or con lo que teniamos
			valor_puerto |= value;

			value = valor_puerto;
		}


		zxuno_ports[last_port_FC3B]=value;

		//Que registro se ha tocado?
		switch (last_port_FC3B) {
			case 0:
				//Si 0, masterconf
				debug_printf (VERBOSE_DEBUG,"Write Masterconf. BOOTM=%d",value&1);

				//ver cambio en bit diven
				 z80_byte diven=zxuno_ports[0]&2;
                                if ( (anterior_masterconf&2) != diven) {
                                        zxuno_set_emulator_setting_diven();
                                }

				//ver cambios en bits, por ejemplo I2KB
				z80_byte i2kb=zxuno_ports[0]&8;
				if ( (anterior_masterconf&8) != i2kb) {
					zxuno_set_emulator_setting_i2kb();
				}


				//ver cambio en bits timings
				z80_byte timing=zxuno_ports[0]&(16+64);
				if ( (anterior_masterconf&(16+64)) != timing) {
					zxuno_set_emulator_setting_timing();
				}

				//ver cambio en bit contended
				z80_byte contend=zxuno_ports[0]&32;
				if ( (anterior_masterconf&32) != contend) {
					zxuno_set_emulator_setting_contend();
				}



				break;

			case 1:
				debug_printf (VERBOSE_DEBUG,"Write Mastermapper. Bank=%d",value&31);

				//Si esta en modo ejecucion, no hacer nada
				if ( (zxuno_ports[0] &1)==0) {
					debug_printf (VERBOSE_DEBUG,"Write Mastermapper but zxuno is not on boot mode, so it has no effect");
				}

				else {

					//Paginar 512kb de zxuno
					z80_byte bank=value&31;
					zxuno_page_ram(bank);

				}

				break;

			case 2:
				//debug_printf (VERBOSE_DEBUG,"Write SPI. Value: 0x%02x",value);

				//Si esta bit LOCK, no permitir escritura en SPI
				if ((zxuno_ports[0]&128)==128) {
					debug_printf (VERBOSE_DEBUG,"LOCK bit set. Not allowed FLASHSPI command");
				}
				else zxuno_write_spi(value);


				break;

			case 3:
				debug_printf (VERBOSE_DEBUG,"Write FLASHCS. Value: %d %s",value,(value&1 ? "not selected" : "selected") );
				//Si esta bit LOCK, no permitir escritura en SPI
				if ((zxuno_ports[0]&128)==128) {
					debug_printf (VERBOSE_DEBUG,"LOCK bit set. Not allowed FLASHCS command");
				}
				else {
					if ( (value&1)==0) {
						zxuno_spi_bus_index=0;
					}
					//Indicar actividad en la flash
					zxuno_footer_flash_operating();
				}

				break;

			case 0x0B:
				//Si 0x0B, SCANDBLCTRL
				debug_printf (VERBOSE_DEBUG,"Write SCANDBLCTRL. Value=%d",value);

				z80_byte scn=zxuno_ports[0x0B];
				if (anterior_scandblctrl!=scn) zxuno_set_emulator_setting_scandblctrl();
			break;


                        case 0x0E:
                                //Si 0x0E, DEVCONTROL
                                debug_printf (VERBOSE_DEBUG,"Write Devcontrol. Value=%d",value);


                                //ver cambio en bit diay
                                 z80_byte diay=zxuno_ports[0x0E]&1;
                                if ( (anterior_devcontrol&1) != diay) {
					zxuno_set_emulator_setting_devcontrol_diay();
                                }

				//ver cambio en bit ditay
				z80_byte ditay=zxuno_ports[0x0E]&2;
				if ( (anterior_devcontrol&2) != ditay) {
                                        zxuno_set_emulator_setting_devcontrol_ditay();
                                }

		                  //ver cambio en bit disd
                                 z80_byte disd=zxuno_ports[0x0E]&128;
                                if ( (anterior_devcontrol&128) != disd) {
                                        zxuno_set_emulator_setting_disd();
                                }


			break;

			case 0x0F:
				//Si 0x0F, DEVCTRL2
				debug_printf (VERBOSE_DEBUG,"Write DEVCTRL2. Value=%d",value);
				/*bit 2 DIRADAS: a 1 para deshabilitar el modo radastaniano. Tenga en cuenta que si el modo radastaniano no se deshabilita, pero se deshabilita la ULAplus, al intentar usar el modo radastaniano, el datapath usado en la ULA no será el esperado y el comportamiento de la pantalla en este caso no está documentado.
				DITIMEX: a 1 para deshabilitar los modos de pantalla compatible Timex. Cualquier escritura al puerto $FF es por tanto ignorada. Si la MMU del Timex está habilitada, una lectura al puerto $FF devolverá 0.
				DIULAPLUS: a 1 para deshabilitar la ULApluis. Cualquier escritura a los puertos de ULAplus se ignora. Las lecturas a dichos puertos devuelven el valor del bus flotante. No obstante tenga en cuenta que el mecanismo de contención para este puerto sigue funcionando aunque esté deshabilitado.
				*/

				//TODO: poder desactivar modo radastan

				z80_byte ditimex=zxuno_ports[0x0F]&2;
				if ( (anterior_devctrl2 & 2) !=ditimex) {
					zxuno_set_emulator_setting_ditimex();
				}

				z80_byte diulaplus=zxuno_ports[0x0F]&1;
				if ( (anterior_devctrl2 & 1) !=diulaplus) {
					zxuno_set_emulator_setting_diulaplus();
				}
			break;



			case 0x40:
                                ulaplus_set_extended_mode(value);

			break;

			case 0x41:
				//radasoffset
				if (zxuno_radasoffset_high_byte.v) {
						zxuno_radasoffset &=255;
						zxuno_radasoffset |=(value<<8);
					}
				else {
					zxuno_radasoffset &= 0xFF00;
					zxuno_radasoffset |=value;
				}

				zxuno_radasoffset_high_byte.v ^=1;

			break;

		}
	}


}

//Rutinas de puertos paginacion zxuno pero cuando bootm=0, o sea, como plus2a
void zxuno_mem_page_rom_p2a(void)
{

	//asignar rom
	//z80_byte rom_entra=((puerto_32765>>4)&1) + ((puerto_8189>>1)&2);

	z80_byte rom1f=(puerto_8189>>1)&2;
	z80_byte rom7f=(puerto_32765>>4)&1;

	z80_byte dirom1f=((zxuno_ports[0x0E]>>4)^255)&2; //Tiene que ir al bit 1
	z80_byte dirom7f=((zxuno_ports[0x0E]>>4)^255)&1; //Tiene que ir al bit 0

	z80_byte rom_entra=(rom1f&dirom1f)+(rom7f&dirom7f);


	//En la tabla zxuno_sram_mem_table hay que saltar las 8 primeras, que son las 8 rams del modo 128k
	zxuno_no_bootm_memory_paged[0]=zxuno_sram_mem_table[rom_entra+8];


	//printf ("Entra rom: %d\n",rom_entra);

	contend_pages_actual[0]=0;
	debug_paginas_memoria_mapeadas[0]=128+rom_entra;
}

//Rutinas de puertos paginacion zxuno pero cuando bootm=0, o sea, como plus2a
void zxuno_mem_page_ram_p2a(void)
{
	z80_byte ramentra=puerto_32765&7;
	//asignar ram
	zxuno_no_bootm_memory_paged[3]=zxuno_sram_mem_table[ramentra];

	contend_pages_actual[3]=contend_pages_128k_p2a[ramentra];
	debug_paginas_memoria_mapeadas[3]=ramentra;
}

//Rutinas de puertos paginacion zxuno pero cuando bootm=0, o sea, como plus2a
void zxuno_mem_page_ram_rom(void)
{
	z80_byte page_type;

	page_type=(puerto_8189 >>1) & 3;

	switch (page_type) {
		case 0:
			debug_printf (VERBOSE_DEBUG,"Pages 0,1,2,3");
			zxuno_no_bootm_memory_paged[0]=zxuno_sram_mem_table[0];
			zxuno_no_bootm_memory_paged[1]=zxuno_sram_mem_table[1];
			zxuno_no_bootm_memory_paged[2]=zxuno_sram_mem_table[2];
			zxuno_no_bootm_memory_paged[3]=zxuno_sram_mem_table[3];

			contend_pages_actual[0]=contend_pages_128k_p2a[0];
			contend_pages_actual[1]=contend_pages_128k_p2a[1];
			contend_pages_actual[2]=contend_pages_128k_p2a[2];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];

			debug_paginas_memoria_mapeadas[0]=0;
			debug_paginas_memoria_mapeadas[1]=1;
			debug_paginas_memoria_mapeadas[2]=2;
			debug_paginas_memoria_mapeadas[3]=3;

			break;

		case 1:
			debug_printf (VERBOSE_DEBUG,"Pages 4,5,6,7");
			zxuno_no_bootm_memory_paged[0]=zxuno_sram_mem_table[4];
			zxuno_no_bootm_memory_paged[1]=zxuno_sram_mem_table[5];
			zxuno_no_bootm_memory_paged[2]=zxuno_sram_mem_table[6];
			zxuno_no_bootm_memory_paged[3]=zxuno_sram_mem_table[7];


			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[5];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[7];

			debug_paginas_memoria_mapeadas[0]=4;
			debug_paginas_memoria_mapeadas[1]=5;
			debug_paginas_memoria_mapeadas[2]=6;
			debug_paginas_memoria_mapeadas[3]=7;



			break;

		case 2:
			debug_printf (VERBOSE_DEBUG,"Pages 4,5,6,3");
			zxuno_no_bootm_memory_paged[0]=zxuno_sram_mem_table[4];
			zxuno_no_bootm_memory_paged[1]=zxuno_sram_mem_table[5];
			zxuno_no_bootm_memory_paged[2]=zxuno_sram_mem_table[6];
			zxuno_no_bootm_memory_paged[3]=zxuno_sram_mem_table[3];

			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[5];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];

			debug_paginas_memoria_mapeadas[0]=4;
			debug_paginas_memoria_mapeadas[1]=5;
			debug_paginas_memoria_mapeadas[2]=6;
			debug_paginas_memoria_mapeadas[3]=3;


			break;

		case 3:
			debug_printf (VERBOSE_DEBUG,"Pages 4,7,6,3");
			zxuno_no_bootm_memory_paged[0]=zxuno_sram_mem_table[4];
			zxuno_no_bootm_memory_paged[1]=zxuno_sram_mem_table[7];
			zxuno_no_bootm_memory_paged[2]=zxuno_sram_mem_table[6];
			zxuno_no_bootm_memory_paged[3]=zxuno_sram_mem_table[3];

			contend_pages_actual[0]=contend_pages_128k_p2a[4];
			contend_pages_actual[1]=contend_pages_128k_p2a[7];
			contend_pages_actual[2]=contend_pages_128k_p2a[6];
			contend_pages_actual[3]=contend_pages_128k_p2a[3];

			debug_paginas_memoria_mapeadas[0]=4;
			debug_paginas_memoria_mapeadas[1]=7;
			debug_paginas_memoria_mapeadas[2]=6;
			debug_paginas_memoria_mapeadas[3]=3;


			break;

	}
}




//Rutinas de puertos paginacion zxuno pero cuando bootm=0, o sea, como plus2a
void zxuno_p2a_write_page_port(z80_int puerto, z80_byte value)
{
	//Puerto tipicamente 32765
	// the hardware will respond only to those port addresses with bit 1 reset, bit 14 set and bit 15 reset (as opposed to just bits 1 and 15 reset on the 128K/+2).
	if ( (puerto & 49154) == 16384 ) {
		//ver si paginacion desactivada
		if (puerto_32765 & 32) return;
		puerto_32765=value;


		//si modo de paginacion ram en rom, volver
		if ((puerto_8189&1)==1) {
			//printf ("Ram in ROM enabled. So RAM paging change with 32765 not allowed. out value:%d\n",value);
			//Livingstone supongo II hace esto, continuamente cambia de Screen 5/7 y ademas cambia
			//formato de paginas de 4,5,6,3 a 4,7,6,3 tambien continuamente
			return;
		}

		//64 kb rom, 128 ram

		//Paginacion desactivada por puertos de zxuno DEVCONTROL. DI7FFD
		if (zxuno_ports[0x0E]&4) return;

		//asignar ram
		zxuno_mem_page_ram_p2a();

		//asignar rom
		zxuno_mem_page_rom_p2a();

		return;
	}

	//Puerto tipicamente 8189

	// the hardware will respond to all port addresses with bit 1 reset, bit 12 set and bits 13, 14 and 15 reset).
	if ( (puerto & 61442 )== 4096) {
		//ver si paginacion desactivada
		if (puerto_32765 & 32) return;

		//Paginacion desactivada por puertos de zxuno DEVCONTROL. DI7FFD
		if (zxuno_ports[0x0E]&4) return;

		//Paginacion desactivada por puertos de zxuno DEVCONTROL. DI1FFD
		if (zxuno_ports[0x0E]&8) return;

		//Modo paginacion especial RAM en ROM
		if (value & 1) {
			puerto_8189=value;
			debug_printf (VERBOSE_DEBUG,"Paging RAM in ROM");
			zxuno_mem_page_ram_rom();

			return;
		}

		else {
			//valor de puerto indica paginado normal
			//miramos si se vuelve de paginado especial RAM in ROM
			if ((puerto_8189&1)==1) {
				debug_printf (VERBOSE_DEBUG,"Going back from paging RAM in ROM");
				//zxuno_mem_set_normal_pages_p2a();
				mem_set_normal_pages_zxuno();
				//asignar ram
				zxuno_mem_page_ram_p2a();
			}
			puerto_8189=value;

			//asignar rom
			zxuno_mem_page_rom_p2a();

			//printf ("temp. paging rom value: %d\n",value);

			return;
		}
	}
}

void zxuno_load_spi_flash(void)
{

	FILE *ptr_flashfile;
	int leidos=0;

	if (zxuno_flash_spi_name[0]==0) {
		open_sharedfile(ZXUNO_SPI_FLASH_NAME,&ptr_flashfile);
	}
	else {
		debug_printf (VERBOSE_INFO,"Opening ZX-Uno Custom Flash File %s",zxuno_flash_spi_name);
		ptr_flashfile=fopen(zxuno_flash_spi_name,"rb");
	}


	if (ptr_flashfile!=NULL) {

		//la flash esta despues de los primeros 16kb rom y 512kb sram
		leidos=fread(&memoria_spectrum[ (ZXUNO_ROM_SIZE+ZXUNO_SRAM_SIZE)*1024 ],1,ZXUNO_SPI_SIZE*1024,ptr_flashfile);
		fclose(ptr_flashfile);

	}



	if (leidos!=ZXUNO_SPI_SIZE*1024 || ptr_flashfile==NULL) {
		debug_printf (VERBOSE_ERR,"Error reading ZX-Uno SPI Flash");
		//cpu_panic("Error loading ZX-Uno SPI Flash");
	}


}


void zxuno_set_timing_128k(void)
{
	contend_read=contend_read_128k;
	contend_read_no_mreq=contend_read_no_mreq_128k;
	contend_write_no_mreq=contend_write_no_mreq_128k;

	ula_contend_port_early=ula_contend_port_early_128k;
	ula_contend_port_late=ula_contend_port_late_128k;


	screen_testados_linea=228;
	screen_invisible_borde_superior=7;
	screen_invisible_borde_derecho=104;

	port_from_ula=port_from_ula_p2a;
	contend_pages_128k_p2a=contend_pages_p2a;

}


void zxuno_set_timing_48k(void)
{
	contend_read=contend_read_48k;
	contend_read_no_mreq=contend_read_no_mreq_48k;
	contend_write_no_mreq=contend_write_no_mreq_48k;

	ula_contend_port_early=ula_contend_port_early_48k;
	ula_contend_port_late=ula_contend_port_late_48k;

	screen_testados_linea=224;
	screen_invisible_borde_superior=8;
	screen_invisible_borde_derecho=96;

	port_from_ula=port_from_ula_48k;

	//esto no se usara...
	contend_pages_128k_p2a=contend_pages_128k;

}

void zxuno_change_timing(int timing)
{
	if (timing==0) zxuno_set_timing_48k();
	else if (timing==1) zxuno_set_timing_128k();
	else if (timing==2) ula_enable_pentagon_timing();

	screen_set_video_params_indices();
	inicializa_tabla_contend();

}


void zxuno_set_emulator_setting_i2kb(void)
{

	z80_byte i2kb=zxuno_ports[0]&8;

	debug_printf (VERBOSE_INFO,"Apply MASTERCONF.I2KB change: issue%d",(i2kb ? 2 : 3) );
	if (i2kb) keyboard_issue2.v=1;
	else keyboard_issue2.v=0;
}


void zxuno_set_emulator_setting_timing(void)
{
	/*MASTERCONF:
	LOCK	MODE1	DISCONT	MODE0	I2KB	DISNMI	DIVEN	BOOTM
	MODE1,MODE0: especifica el modo de timing de la ULA para acomodarse a diferentes modelos de Spectrum. 00 = ULA ZX Spectrum 48K PAL, 01 = ZX Spectrum 128K/+2 gris, 10 = Pentagon 128, 11 = Reservado.
	*/


	z80_byte timing;
	timing=(zxuno_ports[0]&16)>>4;
	timing|=(zxuno_ports[0]&64)>>5;

	if (timing==0) debug_printf (VERBOSE_INFO,"Apply MASTERCONF.TIMING change: 48k");
	else if (timing==1) debug_printf (VERBOSE_INFO,"Apply MASTERCONF.TIMING change: 128k");
	else if (timing==2) debug_printf (VERBOSE_INFO,"Apply MASTERCONF.TIMING change: Pentagon");
	else debug_printf (VERBOSE_INFO,"Apply MASTERCONF.TIMING unknown");
	zxuno_change_timing(timing);

}

void zxuno_set_emulator_setting_contend(void)
{
	z80_byte contend=zxuno_ports[0]&32;
	debug_printf (VERBOSE_INFO,"Apply MASTERCONF.DISCONT change: %s",(contend ? "disabled" : "enabled") );
	if (contend) contend_enabled.v=0;
	else contend_enabled.v=1;
	inicializa_tabla_contend();
}

void zxuno_set_emulator_setting_diven(void)
{
	z80_byte diven=zxuno_ports[0]&2;
	debug_printf (VERBOSE_INFO,"Apply MASTERCONF.DIVEN change: %s",(diven ? "enabled" : "disabled") );
        if (diven) divmmc_diviface_enable();
	else divmmc_diviface_disable();
}


void zxuno_set_emulator_setting_disd(void)
{
        z80_byte disd=zxuno_ports[0x0E]&128;
        debug_printf (VERBOSE_INFO,"Apply DEVCONTROL.DISD change: %s",(disd ? "enabled" : "disabled") );
        if (disd) divmmc_mmc_ports_disable();
        else divmmc_mmc_ports_enable();
}


void zxuno_set_emulator_setting_ditimex(void)
{
        z80_byte ditimex=zxuno_ports[0x0F]&2;
        debug_printf (VERBOSE_INFO,"Apply DEVCTRL2.DITIMEX change: %s",(ditimex ? "disabled" : "enabled") );
        if (ditimex) disable_timex_video();
        else enable_timex_video();
}

void zxuno_set_emulator_setting_diulaplus(void)
{
        z80_byte diulaplus=zxuno_ports[0x0F]&1;
        debug_printf (VERBOSE_INFO,"Apply DEVCTRL2.DIULAPLUS change: %s",(diulaplus ? "disabled" : "enabled") );
        if (diulaplus) disable_ulaplus();
        else enable_ulaplus();
}




//El resto de bits son leidos directamente por el core, actuando en consecuencia
void zxuno_set_emulator_setting_devcontrol_diay(void)
{
	z80_byte diay=zxuno_ports[0x0E]&1;
	debug_printf (VERBOSE_INFO,"Apply DEVCONTROL.DIAY change: %s",(diay ? "disabled" : "enabled") );

	if (diay) ay_chip_present.v=0;
	else ay_chip_present.v=1;
}


void zxuno_set_emulator_setting_devcontrol_ditay(void)
{
        z80_byte ditay=zxuno_ports[0x0E]&2;
        debug_printf (VERBOSE_INFO,"Apply DEVCONTROL.DITAY change: %s",(ditay ? "disabled" : "enabled") );

        if (ditay) set_total_ay_chips(1);
        else set_total_ay_chips(2);

}






//Ajustar modo turbo. Resto de bits de ese registro no usados
void zxuno_set_emulator_setting_scandblctrl(void)
{
	z80_byte scn=(zxuno_ports[0x0B]>>6)&3;

/* Limitar cuando se activa modo turbo. Dado que si se activa en la bios se satura mucho el emulador, no dejo hacerlo ni en arranque ni en bios
turbo modo 8 en pc=50
turbo modo 8 en pc=312
turbo modo 8 en pc=50
turbo modo 8 en pc=312
*/

	int t;

	if (scn==0) t=1;
	else if (scn==1) t=2;
	else if (scn==2) t=4;
	else t=8;

	debug_printf (VERBOSE_INFO,"Set zxuno turbo mode %d with pc=%d",t,reg_pc);

	if (zxuno_deny_turbo_bios_boot.v) {

		if (t>1) {
			if (reg_pc==50 || reg_pc==312) {
				debug_printf (VERBOSE_INFO,"Not changing cpu speed on zxuno bios. We dont want to use too much real cpu for this!");
				return;
			}
		}

	}

	cpu_turbo_speed=t;

	cpu_set_turbo_speed();
}



void zxuno_flush_flash_to_disk(void)
{
	if (!MACHINE_IS_ZXUNO) return;


	if (zxuno_flash_must_flush_to_disk==0) {
		debug_printf (VERBOSE_DEBUG,"Trying to flush SPI FLASH to disk but no changes made");
		return;
	}


	if (zxuno_flash_write_to_disk_enable.v==0) {
		debug_printf (VERBOSE_DEBUG,"Trying to flush SPI FLASH to file but write disabled");
		return;
	}

	debug_printf (VERBOSE_INFO,"Flushing ZX-Uno FLASH to disk");


	FILE *ptr_spiflashfile;

	if (zxuno_flash_spi_name[0]==0) {
		open_sharedfile_write(ZXUNO_SPI_FLASH_NAME,&ptr_spiflashfile);
	}
	else {
		debug_printf (VERBOSE_INFO,"Opening ZX-Uno Custom Flash File %s",zxuno_flash_spi_name);
		ptr_spiflashfile=fopen(zxuno_flash_spi_name,"wb");
	}


	int escritos=0;
	int size;
	size=ZXUNO_SPI_SIZE*1024;


	if (ptr_spiflashfile!=NULL) {
		z80_byte *puntero;
		puntero=&memoria_spectrum[ (ZXUNO_ROM_SIZE+ZXUNO_SRAM_SIZE)*1024 ];

                //Justo antes del fwrite se pone flush a 0, porque si mientras esta el fwrite entra alguna operacion de escritura,
                //metera flush a 1
		zxuno_flash_must_flush_to_disk=0;
		escritos=fwrite(puntero,1,size,ptr_spiflashfile);

		fclose(ptr_spiflashfile);


	}

	//printf ("ptr_spiflashfile: %d\n",ptr_spiflashfile);
	//printf ("escritos: %d\n",escritos);

	if (escritos!=size || ptr_spiflashfile==NULL) {
		debug_printf (VERBOSE_ERR,"Error writing to SPI Flash file. Disabling write file operations");
		zxuno_flash_write_to_disk_enable.v=0;
	}

}



//z80_bit zxuno_disparada_raster={0};
				//Soporte interrupciones raster zxuno

				/*

void zxuno_handle_raster_interrupts()
{


//$0D	RASTERCTRL	Lectura/Escritura	Registro de control y estado de la interrupción ráster. Se definen los siguientes bits.
//INT	0	0	0	0	DISVINT	ENARINT	LINE8
//INT: este bit sólo está disponible en lectura. Vale 1 durante 32 ciclos de reloj a partir del momento en que se dispara la interrupción ráster. Este bit está disponible aunque el procesador tenga las interrupciones deshabilitadas. No está disponible si el bit ENARINT vale 0.
//DISVINT: a 1 para deshabilitar las interrupciones enmascarables por retrazo vertical (las originales de la ULA). Tras un reset, este bit vale 0.
//ENARINT: a 1 para habilitar las interrupciones enmascarables por línea ráster. Tras un reset, este bit vale 0.
//LINE8: guarda el bit 8 del valor de RASTERLINE, para poder definir cualquier valor entre 0 y 511, aunque en la práctica, el mayor valor está limitado por el número de líneas generadas por la ULA (311 en modo 48K, 310 en modo 128K, 319 en modo Pentagon). Si se establece un número de línea superior al límite, la interrupción ráster no se producirá.


					if (iff1.v==1 && (zxuno_ports[0x0d] & 2) ) {
						//interrupciones raster habilitadas
						//printf ("interrupciones raster habilitadas en %d\n",zxuno_ports[0x0c] + (256 * (zxuno_ports[0x0d]&1) ));


						//Ver si estamos entre estado 128 y 128+32
						int estados_en_linea=t_estados & screen_testados_linea;

						if (estados_en_linea>=128 && estados_en_linea<128+32) {
							//Si no se ha disparado la interrupcion
							if (zxuno_disparada_raster.v==0) {
								//Comprobar la linea definida
								//El contador de lineas considera que la línea 0 es la primera línea de paper, la linea 192 por tanto es la primera línea de borde inferior.
								// El último valor del contador es 311 si estamos en un 48K, 310 si estamos en 128K, o 319 si estamos en Pentagon, y coincidiría con la última línea del borde superior.
								//se dispara justo al comenzar el borde derecho de la línea anterior a aquella que has seleccionado
								int linea_raster=zxuno_ports[0x0c] + (256 * (zxuno_ports[0x0d]&1) );

								int disparada_raster=0;


								//se dispara en linea antes... ?
								//if (linea_raster>0) linea_raster--;
								//else {
								//	linea_raster=screen_scanlines-1;
								//}


								//es zona de vsync y borde superior
								//Aqui el contador raster tiene valor (192+56 en adelante)
								//contador de scanlines del core, entre 0 y screen_indice_inicio_pant ,
								if (t_scanline<screen_indice_inicio_pant) {
									if (t_scanline==linea_raster-192-screen_total_borde_inferior) disparada_raster=1;
								}

								//Esto es zona de paper o borde inferior
								//Aqui el contador raster tiene valor 0 .. <(192+56)
								//contador de scanlines del core, entre screen_indice_inicio_pant y screen_testados_total
								else {
									if (t_scanline-screen_indice_inicio_pant==linea_raster) disparada_raster=1;
								}

								if (disparada_raster) {
									//Disparar interrupcion
									zxuno_disparada_raster.v=1;
									interrupcion_maskable_generada.v=1;

									//printf ("Generando interrupcion raster en scanline %d, raster: %d , estados en linea: %d, t_estados %d\n",
									//	t_scanline,linea_raster+1,estados_en_linea,t_estados);

									//Activar bit INT
									zxuno_ports[0x0d] |=128;
								}

								else {
									//Resetear bit INT
									//zxuno_ports[0x0d] &=(255-128);
								}
							}
						}

						//Cualquier otra zona de t_estados, meter a 0
						else {
							zxuno_disparada_raster.v=0;
							//Resetear bit INT
							zxuno_ports[0x0d] &=(255-128);
						}

					}



}
*/
