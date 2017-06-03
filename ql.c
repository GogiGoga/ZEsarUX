#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>

#include "ql.h"
#include "m68k.h"
#include "debug.h"
#include "utils.h"
#include "menu.h"
#include "operaciones.h"


#if defined(__APPLE__)
        #include <sys/syslimits.h>
#endif

//extern unsigned char puerto_49150;


char ql_mdv1_root_dir[PATH_MAX]="";
char ql_mdv2_root_dir[PATH_MAX]="";
char ql_flp1_root_dir[PATH_MAX]="";


unsigned char *memoria_ql;
unsigned char ql_mc_stat;

unsigned char ql_pc_intr;

int ql_microdrive_floppy_emulation=0;


#define QL_STATUS_IPC_IDLE 0
#define QL_STATUS_IPC_WRITING 1

unsigned char ql_ipc_last_write_value=0;

//Ultimo comando recibido
unsigned char ql_ipc_last_command=0;

//Alterna bit ready o no leyendo el serial bit de ipc
int ql_ipc_reading_bit_ready=0;

//Ultimo parametro de comando recibido
unsigned char ql_ipc_last_command_parameter;

int ql_ipc_last_write_bits_enviados=0;
int ql_estado_ipc=QL_STATUS_IPC_IDLE;
int ql_ipc_bytes_received=0;

unsigned char ql_ipc_last_nibble_to_read[32];
int ql_ipc_last_nibble_to_read_mascara=8;
int ql_ipc_last_nibble_to_read_index=0;
int ql_ipc_last_nibble_to_read_length=1;


unsigned char temp_pcintr;

//Valores que me invento para gestionar pulsaciones de teclas no ascii
#define QL_KEYCODE_F1 256
#define QL_KEYCODE_F2 257
#define QL_KEYCODE_UP 258
#define QL_KEYCODE_DOWN 259
#define QL_KEYCODE_LEFT 260
#define QL_KEYCODE_RIGHT 261


//ql_keyboard_table[0] identifica a fila 7 F4     F1      5     F2     F3     F5      4      7
//...
//ql_keyboard_table[7] identifica a fila 0 Shift   Ctrl    Alt      x      v      /      n      ,

//Bits se cuentan desde la izquierda:

// 1      2      4     8      16     32      64    128
//F4     F1      5     F2     F3     F5      4      7

z80_byte ql_keyboard_table[8]={
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255
};

//adicionales
int ql_pressed_backspace=0;

//Nota: esta matrix de teclas y su numeracion de cada fila está documentada erroneamente en la info del QL de manera ascendente (de 0 a 7),
//mientras que lo correcto, cuando se habla de filas en resultado de comandos ipc, es descendente, tal y como está a continuación:

// ================================== matrix ============================
//        0      1      2      3      4      5      6      7
//  +-------------------------------------------------------
// 7|    F4     F1      5     F2     F3     F5      4      7     ql_keyboard_table[0]
// 6|   Ret   Left     Up    Esc  Right      \  Space   Down     ql_keyboard_table[1]
// 5|     ]      z      .      c      b  Pound      m      '     ql_keyboard_table[2]
// 4|     [   Caps      k      s      f      =      g      ;     ql_keyboard_table[3]
// 3|     l      3      h      1      a      p      d      j     ql_keyboard_table[4]
// 2|     9      w      i    Tab      r      -      y      o     ql_keyboard_table[5]
// 1|     8      2      6      q      e      0      t      u     ql_keyboard_table[6]
// 0| Shift   Ctrl    Alt      x      v      /      n      ,     ql_keyboard_table[7]


//Retorna fila y columna para una tecla pulsada mirando ql_keyboard_table. No se puede retornar por ahora mas de una tecla a la vez
//Se excluye shift, ctrl y alt de la respuesta
//Retorna fila -1 y columna -1 si ninguna tecla pulsada

void ql_return_columna_fila_puertos(int *columna,int *fila)
{

	int c,f;
	c=-1;
	f=-1;

	int i;
	int rotacion;

	z80_byte valor_puerto;
	int salir=0;
	for (i=0;i<8 && salir==0;i++){
		valor_puerto=ql_keyboard_table[i];
		//Si shift ctrl y alt quitarlos
		if (i==7) valor_puerto |=1+2+4;

		//Ver si hay alguna tecla pulsada

		for (rotacion=0;rotacion<8 && salir==0;rotacion++) {
			if ((valor_puerto&1)==0) {
				c=rotacion;
				f=7-i;
				salir=1;
				//printf ("c: %d f: %d\n",c,f);
			}
			else {
				valor_puerto=valor_puerto>>1;
			}
		}
	}

	*columna=c;
	*fila=f;
}

//Retorna caracter ascii segun tecla pulsada en ql_keyboard_table
//Miramos segun tabla de tecla a puertos (ql_tabla_teclado_letras)
int ql_return_ascii_key_pressed(void)
{
	//temp
	//if ((ql_keyboard_table[4]&1)==0) return 'l';
	//if ((ql_keyboard_table[4]&16)==0) return 'a';

	int letra;
	int indice=0;

	for (letra='a';letra<='z';letra++) {
		if ((*ql_tabla_teclado_letras[indice].puerto & ql_tabla_teclado_letras[indice].mascara)==0) {
			//printf ("letra: %c\n",letra);
			return letra;
		}

		indice++;
	}

	indice=0;
	for (letra='0';letra<='9';letra++) {
		if ((*ql_tabla_teclado_numeros[indice].puerto & ql_tabla_teclado_numeros[indice].mascara)==0) {
			//printf ("numero: %c\n",letra);
			return letra;
		}

		indice++;
	}

	//Otras teclas
	//Enter
	if ((ql_keyboard_table[1]&1)==0) return 10;

	//Punto
	if ((ql_keyboard_table[2]&4)==0) return '.';

	//Coma
	if ((ql_keyboard_table[7]&128)==0) return ',';

	//Espacio
	if ((ql_keyboard_table[1]&64)==0) return 32;


	//F1
	if ((ql_keyboard_table[0]&2)==0) return QL_KEYCODE_F1;

	//F2
	if ((ql_keyboard_table[0]&8)==0) return QL_KEYCODE_F2;

// 1|   Ret   Left     Up    Esc  Right      \  Space   Down
	if ((ql_keyboard_table[1]&4)==0) return QL_KEYCODE_UP;

	if ((ql_keyboard_table[1]&128)==0) return QL_KEYCODE_DOWN;

	if ((ql_keyboard_table[1]&2)==0) return QL_KEYCODE_LEFT;

	if ((ql_keyboard_table[1]&16)==0) return QL_KEYCODE_RIGHT;


	return 0;
}

void ql_ipc_reset(void)
{
	ql_ipc_last_write_value=0;
	ql_ipc_last_write_bits_enviados=0;
	ql_estado_ipc=QL_STATUS_IPC_IDLE;
	ql_ipc_last_command=0;
	ql_ipc_bytes_received=0;
	ql_ipc_last_nibble_to_read_mascara=8;
	ql_ipc_last_nibble_to_read_index=0;
	ql_ipc_last_nibble_to_read_length=1;
	ql_ipc_reading_bit_ready=0;
}

void ql_debug_port(unsigned int Address)
{
	return;
	switch (Address) {
		case 0x18000:
		printf ("	PC_CLOCK		Real-time clock (Long word)\n\n");
		break;

		case 0x18002:
		printf ("	PC_TCTRL		Transmit control register\n\n");
		break;

		case 0x18003:
		printf ("	PC_IPCWR		IPC port - write only\n\n");
		break;

		case 0x18020:
		printf ("	PC_IPCRD		IPC port - read only\n");
		printf ("	PC_MCTRL		Microdrive control register - write only\n\n");
		break;

		case 0x18021:
		printf ("	PC_INTR		Interrupt register\n\n");
		//usleep(20000);
		break;

		case 0x18022:
		printf ("	PC_TDATA		Transmit register - write only\n");
		printf ("	PC_TRAK1		Read microdrive track 1\n\n");
		break;

		case 0x18023:
		printf ("	PC_TRAK2		Read microdrive track 2\n\n");
		break;

		case 0x18063:
		printf ("	MC_STAT		Master chip status register\n\n");
		break;

		default:
		printf ("Unknown i/o port %08XH\n",Address);
		break;
	}
}





//unsigned char temp_read_ipc;
unsigned char ql_read_ipc(void)
{


//Temporal
//temp_read_ipc ^=64;

	unsigned char valor_retorno=0;

	//printf ("Valor temporal reading ipc: %d\n",ql_ipc_last_nibble_to_read[0]);
/*
        ql_ipc_last_nibble_to_read_index;
        ql_ipc_last_nibble_to_read_length;
*/

	//Ir alternando valor retornado
	if (ql_ipc_reading_bit_ready==0) {
		ql_ipc_reading_bit_ready=1;
		return 0;
	}

	//else ql_ipc_reading_bit_ready=0;



	if (ql_ipc_last_nibble_to_read[ql_ipc_last_nibble_to_read_index]&ql_ipc_last_nibble_to_read_mascara) valor_retorno |=128; //Valor viene en bit 7


	//Solo mostrar este debug si hay tecla pulsada

	//if (ql_pulsado_tecla() )printf ("Returning ipc: %XH. Index nibble: %d mask: %d\n",valor_retorno,ql_ipc_last_nibble_to_read_index,ql_ipc_last_nibble_to_read_mascara);

	if (ql_ipc_last_nibble_to_read_mascara!=1) ql_ipc_last_nibble_to_read_mascara=ql_ipc_last_nibble_to_read_mascara>>1;
	else {

		//Siguiente byte
		ql_ipc_last_nibble_to_read_mascara=8;
		ql_ipc_last_nibble_to_read_index++;
		if (ql_ipc_last_nibble_to_read_index>=ql_ipc_last_nibble_to_read_length) ql_ipc_last_nibble_to_read_index=0; //Si llega al final, dar la vuelta
		//if (ql_ipc_last_nibble_to_read_index>=ql_ipc_last_nibble_to_read_length) ql_ipc_last_nibble_to_read_index=ql_ipc_last_nibble_to_read_length; //dejarlo al final
	}
	//Para no perder nunca el valor. Rotamos mascara


	//if (ql_ipc_last_nibble_to_read_index>1) sleep(2);

	//if (valor_retorno) sleep(1);

	return valor_retorno;
	//return 0;  //De momento eso



	/*
	* Receiving data from the IPC is done by writing %1110 to pc_ipcwr for each bit
* of the data, once again waiting for bit 6 at pc_ipcrd to go to zero, and
* then reading bit 7 there as the data bit. The data is received msb first.
*/
}



int ql_pulsado_tecla(void)
{

	if (menu_abierto) return 0;

	//Si backspace
	if (ql_pressed_backspace) return 1;

	z80_byte acumulado;

	acumulado=255;

	int i;
	for (i=0;i<8;i++) acumulado &=ql_keyboard_table[i];

	if (acumulado==255) return 0;
	return 1;
	/*

	acumulado=menu_da_todas_teclas();


					//Hay tecla pulsada
					if ( (acumulado & MENU_PUERTO_TECLADO_NINGUNA) !=MENU_PUERTO_TECLADO_NINGUNA ) {
						return 1;
					}
	return 0;*/
}

//unsigned char temp_stat_cmd;
//unsigned char temp_contador_tecla_pulsada;

//int temp_columna=0;



//Para gestionar repeticiones
int ql_mantenido_pulsada_tecla=0;
int ql_mantenido_pulsada_tecla_timer=0;

/*
// ================================== matrix ============================
//        0      1      2      3      4      5      6      7
//  +-------------------------------------------------------
// 7|    F4     F1      5     F2     F3     F5      4      7
// 6|   Ret   Left     Up    Esc  Right      \  Space   Down
// 5|     ]      z      .      c      b  Pound      m      '
// 4|     [   Caps      k      s      f      =      g      ;
// 3|     l      3      h      1      a      p      d      j
// 2|     9      w      i    Tab      r      -      y      o
// 1|     8      2      6      q      e      0      t      u
// 0| Shift   Ctrl    Alt      x      v      /      n      ,
*/

struct x_tabla_columna_fila
{
        int columna;
        int fila;
};

struct x_tabla_columna_fila ql_col_fil_numeros[]={
	{5,1}, //0
	{3,3},
	{1,1}, //2
	{1,3},
	{6,7},  //4
	{2,7},
	{2,1},  //6
	{7,7},
	{0,1}, //8
	{0,2}
};


/*
// ================================== matrix ============================
//        0      1      2      3      4      5      6      7
//  +-------------------------------------------------------
// 7|    F4     F1      5     F2     F3     F5      4      7
// 6|   Ret   Left     Up    Esc  Right      \  Space   Down
// 5|     ]      z      .      c      b  Pound      m      '
// 4|     [   Caps      k      s      f      =      g      ;
// 3|     l      3      h      1      a      p      d      j
// 2|     9      w      i    Tab      r      -      y      o
// 1|     8      2      6      q      e      0      t      u
// 0| Shift   Ctrl    Alt      x      v      /      n      ,
*/
struct x_tabla_columna_fila ql_col_fil_letras[]={
	{4,3}, //A
	{4,5},
	{3,5},
	{6,3},
	{4,1}, //E
	{4,4},
	{6,4},
	{2,3}, //H
	{2,2},
	{7,3},
	{2,4}, //K
	{0,3},
	{6,5},
	{6,0}, //N
	{7,2},
	{5,3},
	{3,1}, //Q
	{4,2},
	{3,4},
	{6,1}, //T
	{7,1},
	{4,0},
	{1,2}, //W
	{3,0},
	{6,2},
	{1,5} //Z
};


//Returna fila y columna para una tecla dada.
void ql_return_fila_columna_tecla(int tecla,int *columna,int *fila)
{
	int c;
	int f;
	int indice;

	//Por defecto
	c=-1;
	f=-1;

	if (tecla>='0' && tecla<='9') {
		indice=tecla-'0';
		c=ql_col_fil_numeros[indice].columna;
		f=ql_col_fil_numeros[indice].fila;
	}

	else if (tecla>='a' && tecla<='z') {
		indice=tecla-'a';
		c=ql_col_fil_letras[indice].columna;
		f=ql_col_fil_letras[indice].fila;
	}

	else if (tecla==32) {
		c=6;
		f=6;
	}

	else if (tecla==10) {
		c=0;
		f=6;
	}

	else if (tecla==QL_KEYCODE_F1) {
		c=1;
		f=7;
	}

	else if (tecla==QL_KEYCODE_F2) {
		c=3;
		f=7;
	}

	else if (tecla=='.') {
		c=2;
		f=5;
	}

	else if (tecla==',') {
		c=7;
		f=0;
	}

	//        0      1      2      3      4      5      6      7
	//  +-------------------------------------------------------
	// 6|   Ret   Left     Up    Esc  Right      \  Space   Down

	else if (tecla==QL_KEYCODE_UP) {
		c=2;
		f=6;
	}

	else if (tecla==QL_KEYCODE_DOWN) {
		c=7;
		f=6;
	}


	else if (tecla==QL_KEYCODE_LEFT) {
		c=1;
		f=6;
	}


	else if (tecla==QL_KEYCODE_RIGHT) {
		c=4;
		f=6;
	}


	*columna=c;
	*fila=f;
}

/*
89l6ihverantyd


*/




//Mete en valores ipc de vuelta segun teclas pulsadas
/*Desde la rom, cuando se genera una PC_INTR, se pone a leer de ipc lo siguiente y luego ya llama a write_ipc comando 8:


Read PC_INTR pulsado tecla
Returning ipc: 0H. Index nibble: 0 mask: 1
Returning ipc: 0H. Index nibble: 0 mask: 8
Returning ipc: 80H. Index nibble: 0 mask: 4
Write ipc command 1 pressed key
Returning ipc: 80H. Index nibble: 0 mask: 8
Returning ipc: 80H. Index nibble: 0 mask: 4
Returning ipc: 80H. Index nibble: 0 mask: 2
Returning ipc: 80H. Index nibble: 0 mask: 1

Returning ipc: 80H. Index nibble: 0 mask: 8
Returning ipc: 80H. Index nibble: 0 mask: 4
Returning ipc: 80H. Index nibble: 0 mask: 2
Returning ipc: 80H. Index nibble: 0 mask: 1

Returning ipc: 80H. Index nibble: 0 mask: 8
Returning ipc: 80H. Index nibble: 0 mask: 4
Returning ipc: 80H. Index nibble: 0 mask: 2
Returning ipc: 80H. Index nibble: 0 mask: 1

Returning ipc: 80H. Index nibble: 0 mask: 8
Returning ipc: 80H. Index nibble: 0 mask: 4
Returning ipc: 80H. Index nibble: 0 mask: 2
Returning ipc: 80H. Index nibble: 0 mask: 1

Returning ipc: 80H. Index nibble: 0 mask: 8
QL Trap ROM: Tell one key pressed
Returning ipc: 80H. Index nibble: 0 mask: 4
Returning ipc: 80H. Index nibble: 0 mask: 2
Returning ipc: 80H. Index nibble: 0 mask: 1


--Aparentemente estas lecturas anteriores no afectan al valor de la tecla

Lectura de teclado comando. PC=00002F8EH
letra: a
no repeticion


*/


//int temp_flag_reves=0;
void ql_ipc_write_ipc_teclado(void)
{
	/*
	* This returns one nibble, plus up to 7 nibble/byte pairs:
	* first nibble, ms bit: set if final last keydef is still held
	* first nibble, ls 3 bits: count of keydefs to follow.
	* then, for each of the 0..7 keydefs:
	* nibble, bits are 3210=lsca: lost keys (last set only), shift, ctrl and alt.
	* byte, bits are 76543210=00colrow: column and row as keyrow table.
	* There is a version of the IPC used on the thor that will also return keydef
	* values for a keypad. This needs looking up
	*/

	/*Devolveremos una tecla. Esto es:
	* first nibble, ms bit: set if final last keydef is still held
	* first nibble, ls 3 bits: count of keydefs to follow.
	valor 1
	//nibble, bits are 3210=lsca: lost keys (last set only), shift, ctrl and alt.
	valor 0
	//byte, bits are 76543210=00colrow: column and row as keyrow table
// ================================== matrix ============================
//        0      1      2      3      4      5      6      7
//  +-------------------------------------------------------
// 7|    F4     F1      5     F2     F3     F5      4      7
// 6|   Ret   Left     Up    Esc  Right      \  Space   Down
// 5|     ]      z      .      c      b  Pound      m      '
// 4|     [   Caps      k      s      f      =      g      ;
// 3|     l      3      h      1      a      p      d      j
// 2|     9      w      i    Tab      r      -      y      o
// 1|     8      2      6      q      e      0      t      u
// 0| Shift   Ctrl    Alt      x      v      /      n      ,


	//F1 es columna 1, row 0
	valor es 7654=00co=0000=0
	valor es lrow=1000 =8
	col=001 row=000
	00 001 000

	*/


	int columna;
	int fila;


int i;
	//Si tecla no pulsada
	//if ((puerto_49150&1)) {
	if (!ql_pulsado_tecla()) {
		for (i=0;i<ql_ipc_last_nibble_to_read_length;i++) ql_ipc_last_nibble_to_read[i]=0;
	}


ql_return_columna_fila_puertos(&columna,&fila);
int tecla_shift=0;
int tecla_control=0;
int tecla_alt=0;


if ( (columna>=0 && fila>=0) || ql_pressed_backspace) {
	if (ql_mantenido_pulsada_tecla==0 || (ql_mantenido_pulsada_tecla==1 && ql_mantenido_pulsada_tecla_timer>=50) )  {
		if (ql_mantenido_pulsada_tecla==0) {
			ql_mantenido_pulsada_tecla=1;
			ql_mantenido_pulsada_tecla_timer=0;
		}


		if (ql_pressed_backspace) {
			//CTRL + flecha izquierda
			tecla_control=1;
			// 6|   Ret   Left     Up    Esc  Right      \  Space   Down
			fila=6;
			columna=1;

		}

		//printf ("------fila %d columna %d\n",fila,columna);
		z80_byte byte_tecla=((fila&7)<<3) | (columna&7);



		ql_ipc_last_nibble_to_read[2]=(byte_tecla>>4)&15;
		ql_ipc_last_nibble_to_read[3]=(byte_tecla&15);

		if ((ql_keyboard_table[7]&1)==0) tecla_shift=1;
		if ((ql_keyboard_table[7]&2)==0) tecla_control=1;
		if ((ql_keyboard_table[7]&4)==0) tecla_alt=1;


		ql_ipc_last_nibble_to_read[1]=0;  //lsca
		if (tecla_shift) ql_ipc_last_nibble_to_read[1] |=4;
		if (tecla_control) ql_ipc_last_nibble_to_read[1] |=2;
		if (tecla_alt) ql_ipc_last_nibble_to_read[1] |=1;


	}
	else {
		//debug_printf (VERBOSE_PARANOID,"Repeating key");
		ql_ipc_last_nibble_to_read[0]=ql_ipc_last_nibble_to_read[1]=ql_ipc_last_nibble_to_read[2]=ql_ipc_last_nibble_to_read[3]=ql_ipc_last_nibble_to_read[4]=ql_ipc_last_nibble_to_read[5]=0;
	}
}
else {
	//debug_printf (VERBOSE_PARANOID,"Unknown key");
	ql_mantenido_pulsada_tecla=0;
	ql_ipc_last_nibble_to_read[0]=ql_ipc_last_nibble_to_read[1]=ql_ipc_last_nibble_to_read[2]=ql_ipc_last_nibble_to_read[3]=ql_ipc_last_nibble_to_read[4]=0;
}


				ql_ipc_last_nibble_to_read_mascara=8;
				ql_ipc_last_nibble_to_read_index=0;
				ql_ipc_last_nibble_to_read_length=5; //5;

					//printf ("Ultimo pc_intr: %d\n",temp_pcintr);

				for (i=0;i<ql_ipc_last_nibble_to_read_length;i++) {
					//debug_printf (VERBOSE_PARANOID,"Return IPC values:[%d] = %02XH",i,ql_ipc_last_nibble_to_read[i]);
				}

}



void ql_ipc_write_ipc_read_keyrow(int row)
{

/*
kbdr_cmd equ    9       keyboard direct read
* kbdr_cmd requires one nibble which selects the row to be read.
* The top bit of this is ignored (at least on standard IPC's...).
* It responds with a byte whose bits indicate which of the up to eight keys on
* the specified row of the keyrow table are held down. */

//De momento nada

	//z80_byte temp_resultado=0;

	//if (ql_pulsado_tecla()) temp_resultado++;
	z80_byte resultado_row;

	resultado_row=ql_keyboard_table[row&7] ^ 255;
	//Bit a 1 para cada tecla pulsada
	//row numerando de 0 a 7
	/*
	// ================================== matrix ============================
	//        0      1      2      3      4      5      6      7
	//  +-------------------------------------------------------
	// |    F4     F1      5     F2     F3     F5      4      7
	// |   Ret   Left     Up    Esc  Right      \  Space   Down
	// |     ]      z      .      c      b  Pound      m      '
	// |     [   Caps      k      s      f      =      g      ;
	// |     l      3      h      1      a      p      d      j
	// |     9      w      i    Tab      r      -      y      o
	// |     8      2      6      q      e      0      t      u
	// | Shift   Ctrl    Alt      x      v      /      n      ,
	Por ejemplo, para leer si se pulsa Space, tenemos que leer row 1, y ver luego si bit 6 está a 1 (40H)
	*/

	debug_printf (VERBOSE_PARANOID,"Reading ipc command 9: read keyrow. row %d returning %02XH",row,resultado_row);

		ql_ipc_last_nibble_to_read[0]=(resultado_row>>4)&15;
		ql_ipc_last_nibble_to_read[1]=resultado_row&15;
			ql_ipc_last_nibble_to_read_mascara=8;
			ql_ipc_last_nibble_to_read_index=0;
			ql_ipc_last_nibble_to_read_length=2;


}

void ql_write_ipc(unsigned char Data)
{
	/*
	* Commands and data are sent msb first, by writing a byte containg %11x0 to
	* location pc_ipcwr ($18023), where the "x" is one data bit. Bit 6 at location
	* pc_ipcrd ($18020) is then examined, waiting for it to go zero to indicate
	* that the bit has been received by the IPC.
	*/
	/*
	* Receiving data from the IPC is done by writing %1110 to pc_ipcwr for each bit
* of the data, once again waiting for bit 6 at pc_ipcrd to go to zero, and
* then reading bit 7 there as the data bit. The data is received msb first.
	*/

	//Si dato tiene formato: 11x0  (8+4+1)
	if ((Data&13)!=12) return;

	int bitdato=(Data>>1)&1;
	//printf ("Escribiendo bit ipc: %d\n",bitdato);
	ql_ipc_last_write_value=ql_ipc_last_write_value<<1;
	ql_ipc_last_write_value |=bitdato;
	ql_ipc_last_write_bits_enviados++;
	if (ql_ipc_last_write_bits_enviados==4) {
			switch (ql_estado_ipc) {
			  case QL_STATUS_IPC_IDLE:
					ql_ipc_last_command=ql_ipc_last_write_value&15;
					//printf ("Resultante ipc command: %d (%XH)\n",ql_ipc_last_command,ql_ipc_last_command); //Se generan 4 bits cada vez
					ql_ipc_last_write_bits_enviados=0;

					//Actuar segun comando
					switch (ql_ipc_last_command)
					{


						case 0:
						//*rset_cmd equ    0       resets the IPC software
							ql_ipc_reset();
						break;

						case 1:
/*
stat_cmd equ    1       report input status
* returns a byte, the bits of which are:
ipc..kb equ     0       set if data available in keyboard buffer, or key held
ipc..so equ     1       set if sound is still being generated
*               2       set if kbd shift setting has changed, with key held
*               3       set if key held down
ipc..s1 equ     4       set if input is pending from RS232 channel 1
ipc..s2 equ     5       set if input is pending from RS232 channel 2
ipc..wp equ     6       return state of p26, currently not connected
*               7       was set if serial transfer was being zapped, now zero
*/

							ql_ipc_last_nibble_to_read[0]=0; //ipc..kb equ     0       set if data available in keyboard buffer, or key held


							//Decir tecla pulsada

							//temp
							//temp_stat_cmd++;
							//ql_ipc_last_nibble_to_read[0]=temp_stat_cmd;
							ql_ipc_last_nibble_to_read[0]=15; //Devolver valor entre 8 y 15 implica que acabara leyendo el teclado
						        ql_ipc_last_nibble_to_read_mascara=8;
						        ql_ipc_last_nibble_to_read_index=0;
						        ql_ipc_last_nibble_to_read_length=1;


							//Si tecla no pulsada
							if (!ql_pulsado_tecla()) ql_ipc_last_nibble_to_read[0]=4;

							else {
								//debug_printf (VERBOSE_DEBUG,"Write ipc command 1: Report input status pressed key");
							}
							//if ((puerto_49150&1)) ql_ipc_last_nibble_to_read[0]=4;

							//printf ("Valor a retornar: %d\n",ql_ipc_last_nibble_to_read[0]&15);

							//sleep(1);

						break;

						case 8:

							//debug_printf (VERBOSE_DEBUG,"Write ipc command 8: Read key. PC=%08XH",get_pc_register() );

							ql_ipc_write_ipc_teclado();


						break;

						case 9:
							//debug_printf (VERBOSE_ERR,"Write ipc command 9: Reading keyrow. Not implemented");
							/*
							kbdr_cmd equ    9       keyboard direct read
* kbdr_cmd requires one nibble which selects the row to be read.
* The top bit of this is ignored (at least on standard IPC's...).
* It responds with a byte whose bits indicate which of the up to eight keys on
* the specified row of the keyrow table are held down. */

							ql_estado_ipc=QL_STATUS_IPC_WRITING;


						break;

						case 10:
						/*
						inso_cmd equ    10      initiate sound process
* This requires no less than 64 bits of data. it starts sound generation.
* Note that the 16 bit values below need to have their ls 8 bits sent first!
* 8 bits pitch 1
* 8 bits pitch 2
* 16 bits interval between steps
* 16 bits duration (0=forever)
* 4 bits signed step in pitch
* 4 bits wrap
* 4 bits randomness (none unless msb is set)
* 4 bits fuziness (none unless msb is set)
*/
							debug_printf (VERBOSE_PARANOID,"ipc command 10 inso_cmd initiate sound process");
							//ql_estado_ipc=QL_STATUS_IPC_WRITING; Mejor lo desactivo porque si no se queda en estado writing y no sale de ahi
						break;

						//baud_cmd
						case 13:
						/*
						baud_cmd equ    13      change baud rate
* This expects one nibble, of which the 3 lsbs select the baud rate for both
* serial channels. The msb is ignored. Values 7 down to zero correspond to baud
* rates 75/300/600/1200/2400/4800/9600/19200.
* The actual clock rate is supplied from the PC to the IPC, but this command is
* also needed in the IPC for timing out transfers!
						*/
						ql_estado_ipc=QL_STATUS_IPC_WRITING;
						break;


						case 15:
							//El que se acaba enviando para leer del puerto ipc. No hacer nada
						break;

						default:
							//debug_printf (VERBOSE_ERR,"Write ipc command %d. Not implemented",ql_ipc_last_command);
						break;
					}
				break;


			case QL_STATUS_IPC_WRITING:
				ql_ipc_last_command_parameter=ql_ipc_last_write_value&15;
				//printf ("Parametro recibido de ultimo comando %d: %d\n",ql_ipc_last_command,ql_ipc_last_command_parameter);
				//Segun ultimo comando
					switch (ql_ipc_last_command) {
						case 9:
						/*
						kbdr_cmd equ    9       keyboard direct read
* kbdr_cmd requires one nibble which selects the row to be read.
* The top bit of this is ignored (at least on standard IPC's...).
* It responds with a byte whose bits indicate which of the up to eight keys on
* the specified row of the keyrow table are held down. */
//ql_ipc_write_ipc_read_keyrow();
							//printf ("Parametro recibido de ultimo comando read keyrow %d: %d\n",ql_ipc_last_command,ql_ipc_last_command_parameter);
							ql_estado_ipc=QL_STATUS_IPC_IDLE;
							ql_ipc_last_write_bits_enviados=0;
							ql_ipc_write_ipc_read_keyrow(ql_ipc_last_command_parameter);


						break;


						case 10:
							debug_printf (VERBOSE_PARANOID,"parameter sound %d: %d",ql_ipc_bytes_received,ql_ipc_last_command_parameter);
							ql_ipc_bytes_received++;
							if (ql_ipc_bytes_received>=8) {
								debug_printf (VERBOSE_PARANOID,"End receiving ipc parameters");
								ql_estado_ipc=QL_STATUS_IPC_IDLE;
								ql_ipc_last_write_bits_enviados=0;
								ql_ipc_bytes_received=0;
							}
						break;

						case 13:
							ql_estado_ipc=QL_STATUS_IPC_IDLE;
							//Fin de parametros de comando. Establecer baud rate y dejar status a idle de nuevo para que la siguiente escritura se interprete como comando inicial
						break;
					}
			break;
			}
			//sleep(2);
	}


}




unsigned char ql_lee_puerto(unsigned int Address)
{



	unsigned char valor_retorno=0;

	//temporal
	//return 255;

	switch (Address) {

		case 	0x18020:
			//printf ("Leyendo IPC. PC=%06XH\n",get_pc_register());
			//temp
			//return 0;
			return ql_read_ipc();

		break;


		case 0x18021:
			//printf ("Read PC_INTR		Interrupt register. Value: %02XH\n\n",ql_pc_intr);


                        //temp solo al pulsar enter
                        ////puerto_49150    db              255  ; H                J         K      L    Enter ;6
                        //if ((puerto_49150&1)==0) {
/*
* read addresses
pc_intr equ     $18021  bits 4..0 set as pending level 2 interrupts
pc.intre equ    1<<4    external interrupt register
pc.intrf equ    1<<3    frame interrupt register
pc.intrt equ    1<<2    transmit interrupt register
pc.intri equ    1<<1    interface interrupt register
pc.intrg equ    1<<0    gap interrupt register
*/

/*
Esto se lee en la rom asi:
L00352 MOVEM.L D7/A5-A6,-(A7)       *#: ext2int entry
XL00352 EQU L00352
       MOVEA.L A7,A5
       MOVEA.L #$00028000,A6
       MOVE.B  $00018021,D7
       LSR.B   #1,D7
       BCS     L02ABC
       LSR.B   #1,D7
       BCS     L02CCC               * 8049 interrupt

	Por tanto la 8049 interrupt se interpreta cuando bit 1 activo
*/

			ql_pc_intr=0;
			//if ((puerto_49150&1)==0) ql_pc_intr |=2;
			if (ql_pulsado_tecla() ) {
				//debug_printf (VERBOSE_DEBUG,"Read PC_INTR pressed key");
				ql_pc_intr |=2;
			}
			//printf ("------------Retornando %d\n",ql_pc_intr);
		//	return ql_pc_intr;
			return 134; //Con pruebas, acabo viendo que retornar este valor acaba provocando lectura de teclado

			temp_pcintr++;
			//printf ("------------Retornando %d\n",temp_pcintr);
			return temp_pcintr;
			//if ((puerto_49150&1)==0) ql_pc_intr |=31;
			if (ql_pulsado_tecla() ) ql_pc_intr |=31;
			//usleep(5000000);
			//sleep(1);
			return ql_pc_intr;
			//return 31;

			//}

			//else return 255;
			//return 255;
		break;

	}


	return valor_retorno;
}

void ql_out_port(unsigned int Address, unsigned char Data)
{

	int anterior_video_mode;

	switch (Address) {

  	case    0x18003:
			//printf ("Escribiendo IPC. Valor: %02XH PC=%06XH\n",Data,get_pc_register() );
			ql_write_ipc(Data);

		break;

		case 0x18020:
			//printf ("Writing on 18020H - Microdrive control register - write. Data: %02XH\n",Data);
		break;

		case 0x18021:
		  //printf ("Escribiendo pc_intr. Valor: %02XH\n",Data);
/*
*pc_intr equ    $18021  7..5 masks and 4..0 to clear interrupt
*/

//Es una mascara??
			ql_pc_intr=ql_pc_intr&(Data^255);

			ql_pc_intr=Data;

			//sleep(5);
		break;

		case 0x18063:
			//MC_STAT		Master chip status register
			anterior_video_mode=(ql_mc_stat>>3)&1;

			ql_mc_stat=Data;

			int video_mode=(ql_mc_stat>>3)&1;

			if (video_mode!=anterior_video_mode) {
				//0=512x256
				//1=256x256
				/*
					0 = 4 colour (mode 4) =512x256
				  1 = 8 colour (mode 8) =256x256
				*/
				if (video_mode==0) screen_print_splash_text(10,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,"Setting mode 4 512x256");
				else screen_print_splash_text(10,ESTILO_GUI_TINTA_NORMAL,ESTILO_GUI_PAPEL_NORMAL,"Setting mode 8 256x256");
			}

		break;
	}

}





void ql_writebyte(unsigned int Address, unsigned char Data)
{
	Address&=QL_MEM_LIMIT;

	if (Address>=0x18000 && Address<=0x1BFFF) {

		if (Address==0x18003) {
                //printf ("writing i/o %X value %X\n",Address,Data);
								ql_ipc_reading_bit_ready=0;
								ql_debug_port(Address);
								//sleep(1);
		}
		else {
			ql_debug_port(Address);
		}

		ql_out_port(Address,Data);


#ifdef EMULATE_VISUALMEM
		//Escribimos en visualmem a partir de direccion 18000H
		set_visualmembuffer(Address);

#endif

    return; //Espacio i/o
  }

	if (Address<0x18000 || Address>QL_MEM_LIMIT) return;
  //memoria_ql[Address&0xfffff]=Data;

  unsigned char valor=Data;
	//memoria_ql[Address&0xfffff]=valor;
  memoria_ql[Address]=valor;

#ifdef EMULATE_VISUALMEM
	//printf ("addr: %d\n",Address);
	//Escribimos en visualmem a partir de direccion 18000H
	set_visualmembuffer(Address);

#endif

}

unsigned char ql_readbyte(unsigned int Address)
{
	Address&=QL_MEM_LIMIT;

	if (Address>=0x18000 && Address<=0x1BFFF) {

		if (Address==0x18020) {
	    //printf ("Reading i/o %X\n",Address);
			ql_debug_port(Address);
			//sleep(1);
		}


		else {
			ql_debug_port(Address);
		}


		unsigned char valor=ql_lee_puerto(Address);
		//printf ("return value: %02XH\n",valor);
		return valor;
	}

	//if (Address==0x00028000) printf ("Leyendo parte despues del boot\n");

  if (Address>QL_MEM_LIMIT) return(0);

	//unsigned char valor=memoria_ql[Address&0xfffff];
	unsigned char valor=memoria_ql[Address];
	return valor;
}

unsigned char ql_readbyte_no_ports(unsigned int Address)
{
	Address&=QL_MEM_LIMIT;
	unsigned char valor=memoria_ql[Address];
	return valor;

}

void ql_writebyte_no_ports(unsigned int Address,unsigned char valor)
{
	Address&=QL_MEM_LIMIT;
	memoria_ql[Address]=valor;

}

//extern void    disass(char *buf, uint16 *inst_stream);

char texto_disassemble[1000];

/* Usado en prueba_ql.c
void dump_screen(void)
{
        printf ("Inicio pantalla\n");

        unsigned char *mem;

        mem=&memoria_ql[128*1024];

        int x,y;

        for (y=0;y<256;y++) {
                for (x=0;x<128;x++) {
                        printf ("%02X",*mem);
                        mem++;
                }
                printf ("\n");
        }

        printf ("\n");
}

*/


void disassemble(void)
{

/*
        uint8 *mem;

        int i;
        mem=&memoria_ql[M68K_REG_PC & QL_MEM_LIMIT];
        printf ("%02X%02X%02X%02X%02X%02X%02X%02X ",mem[0],mem[1],mem[2],mem[3],mem[4],mem[5],mem[6],mem[7]);

        uint16 buffer[16];

        uint16 temp;
        for (i=0;i<16;i++) {
                temp=(*mem)*256;
                mem++;
                temp |=(*mem);
                mem++;
                buffer[i]=temp;
        }

        disass(texto_disassemble,buffer);

        printf ("%08XH %s\n",pc,texto_disassemble);
*/
}




void print_regs(void)
{
/*
                printf ("PC: %08X usp: %08X ssp: %08X ",pc,usp,ssp);
        int i;

        for (i=0;i<8;i++) {
                printf ("D%d: %08X ",i,reg[i]);
        }
        for (;i<16;i++) {
                printf ("A%d: %08X ",i-8,reg[i]);
        }



        printf ("\n");
*/
}





unsigned int GetMemB(unsigned int address)
{
        return(ql_readbyte(address));
}
/* Fetch word, address may not be word-aligned */
unsigned int  GetMemW(unsigned int address)
{
#ifdef CHKADDRESSERR
    if (address & 0x1) ExceptionGroup0(ADDRESSERR, address, 1);
#endif
        return((ql_readbyte(address)<<8)|ql_readbyte(address+1));
}
/* Fetch dword, address may not be dword-aligned */
unsigned int GetMemL(unsigned int address)
{
#ifdef CHKADDRESSERR
    if (address & 0x1) ExceptionGroup0(ADDRESSERR, address, 1);
#endif
        return((GetMemW(address)<<16) | GetMemW(address+2));
}
/* Write byte to address */
void SetMemB (unsigned int address, unsigned int value)
{
        ql_writebyte(address,value);
}
/* Write word, address may not be word-aligned */
void SetMemW(unsigned int address, unsigned int value)
{
#ifdef CHKADDRESSERR
if (address & 0x1) ExceptionGroup0(ADDRESSERR, address, 0);
#endif
        ql_writebyte(address,(value>>8)&255);
        ql_writebyte(address+1, (value&255));
}
/* Write dword, address may not be dword-aligned */
void SetMemL(unsigned int address, unsigned int value)
{
#ifdef CHKADDRESSERR
    if (address & 0x1) ExceptionGroup0(ADDRESSERR, address, 0);
#endif
        SetMemW(address, (value>>16)&65535);
        SetMemW(address+2, (value&65535));
}


unsigned int m68k_read_disassembler_16 (unsigned int address)
{
	return GetMemW(address);
}


unsigned int m68k_read_disassembler_32 (unsigned int address)
{
	return GetMemL(address);
}




//Funciones legacy solo para interceptar posibles llamadas a poke, peek etc en caso de motorola
//la mayoria de estas vienen del menu, lo ideal es que en el menu se usen peek_byte_z80_moto , etc

void poke_byte_legacy_ql(z80_int dir GCC_UNUSED,z80_byte valor GCC_UNUSED)
{
	debug_printf(VERBOSE_ERR,"Calling poke_byte function on a QL machine. TODO fix it!");
}
void poke_byte_no_time_legacy_ql(z80_int dir GCC_UNUSED,z80_byte valor GCC_UNUSED)
{
	debug_printf(VERBOSE_ERR,"Calling poke_byte_no_time function on a QL machine. TODO fix it!");
}
z80_byte peek_byte_legacy_ql(z80_int dir GCC_UNUSED)
{
	debug_printf(VERBOSE_ERR,"Calling peek_byte function on a QL machine. TODO fix it!");
	return 0;
}
z80_byte peek_byte_no_time_legacy_ql(z80_int dir GCC_UNUSED)
{
	//debug_printf(VERBOSE_ERR,"Calling peek_byte_no_time function on a QL machine. TODO fix it!");
	return 0;
}
z80_byte lee_puerto_legacy_ql(z80_byte h GCC_UNUSED,z80_byte l GCC_UNUSED)
{
	debug_printf(VERBOSE_ERR,"Calling lee_puerto function on a QL machine. TODO fix it!");
	return 0;
}
void out_port_legacy_ql(z80_int puerto GCC_UNUSED,z80_byte value GCC_UNUSED)
{
	debug_printf(VERBOSE_ERR,"Calling out_port function on a QL machine. TODO fix it!");
}
z80_byte fetch_opcode_legacy_ql(void)
{
	debug_printf(VERBOSE_ERR,"Calling fetch_opcode function on a QL machine. TODO fix it!");
	return 0;
}
