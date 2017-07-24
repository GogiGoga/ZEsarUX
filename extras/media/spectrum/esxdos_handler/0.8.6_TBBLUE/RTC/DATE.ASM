;
; TBBlue / ZX Spectrum Next project
; Copyright (c) 2015 - Fabio Belavenuto & Victor Trucco
;
; All rights reserved
;
; Redistribution and use in source and synthezised forms, with or without
; modification, are permitted provided that the following conditions are met:
;
; Redistributions of source code must retain the above copyright notice,
; this list of conditions and the following disclaimer.
;
; Redistributions in synthesized form must reproduce the above copyright
; notice, this list of conditions and the following disclaimer in the
; documentation and/or other materials provided with the distribution.
;
; Neither the name of the author nor the names of other contributors may
; be used to endorse or promote products derived from this software without
; specific prior written permission.
;
; THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
; THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;
; You are responsible for any legal issues arising from your use of this code.
;
;-------------------------------------------------------------------------------
;
; .DATE for ESXDOS
;
; - Return the date saved in DS1307
; - Save the date in DS1307
;
;

	DEVICE   ZXSPECTRUM48

	output "DATE"

	org 0x2000

	
PORT		equ 0x3B
PORT_CLOCK 	equ 0x10 ;0x103b
PORT_DATA 	equ 0x11 ;0x113b

  
MAIN:
	ld a,h
	or l
	jr z,READ_DATE  ;if we dont have parameters it a read command


WRITE_DATE:

	;HL point to parameters
	CALL CONVERT_DIGITS
	jr c,end_error; return to basic with error message
	LD (DATE),a ; store in the table
	
	inc HL ; separator
	
	inc HL
	CALL CONVERT_DIGITS
	jr c,end_error; return to basic with error message
	LD (MON),a ; store in the table
	
	inc HL ;separator
	inc HL ; 2
	inc HL ; 0
	
	inc HL ; 
	CALL CONVERT_DIGITS
	jr c,end_error; return to basic with error message
	LD (YEA),a ; store in the table

	
	;---------------------------------------------------
	; Talk to DS1307 
	call START_SEQUENCE
	
	ld l,0xD0 
	call SEND_DATA
	
	ld l,0x04  ;start to send at reg 0x04 (date) 
	call SEND_DATA
	
	;---------------------------------------------------

	ld hl, (DATE)
	call SEND_DATA
	
	ld hl, (MON)
	call SEND_DATA

	ld hl, (YEA)
	call SEND_DATA

	;STOP_SEQUENCE
	CALL SDA0
	CALL SCL1
	CALL SDA1
	
	;-------------------------------------------------

end:
	;it´s ok, lets show the current date
	JR READ_DATE
	ret
	
	;return to basic with an error message
end_error:
	LD HL, MsgUsage
	CALL PrintMsg	
	ret
	
	
	
CONVERT_DIGITS:
	LD a,(HL)
	;test ascii for 0 to 9
	CP 48
	jr C,CHAR_ERROR 
	
	CP 58
	jr NC,CHAR_ERROR
	
	or a; clear the carry
	
	sub 48 ; convert asc to number

	;first digit in upper bits
	SLA A
	SLA A
	SLA A
	SLA A

	LD b,a ; store in b
	
	; next parameter
	inc HL
	LD a,(HL)
	
	;test ascii for 0 to 9
	CP 48
	jr C,CHAR_ERROR 
	
	CP 58
	jr NC,CHAR_ERROR
	
	sub 48 ; convert asc to number

	and 0x0f ; get just the lower bits
	or b ;combine with first digit
	
	or a; clear the carry
	ret
	
	
CHAR_ERROR:
	
	scf ; set the carry
	ret
	
	
	
READ_DATE:					
	;---------------------------------------------------
	; Talk to DS1307 and request the first reg
	call START_SEQUENCE
	
	ld l,0xD0 
	call SEND_DATA
	
	ld l,0x00 
	call SEND_DATA
	
	call START_SEQUENCE
	
	ld l,0xD1
	call SEND_DATA
	;---------------------------------------------------
	
	;point to the first reg in table
	LD HL,SEC
	
	;there are 7 regs to read
	LD e, 7
	
loop_read:
	call READ

	;point to next reg
	inc l	
	
	;dec number of regs
	dec e
	jr z, end_read
	
	;if don´t finish, send as ACK and loop
	call SEND_ACK
	jr loop_read

	;we just finished to read the I2C, send a NACK and STOP
end_read:	
	call SEND_NACK
	
	;STOP_SEQUENCE:
	CALL SDA0
	CALL SCL1
	CALL SDA1

;-----------------------------------------------------------	

	;get the date
	LD HL, DATE
	LD a,(HL)
	call NUMBER_TO_ASC
	ld a,b
	LD (day_txt),a
	ld a,c
	LD (day_txt + 1),a
	
	
	
	;get the month
	inc HL
	LD a,(HL)
	call NUMBER_TO_ASC
	ld a,b
	LD (mon_txt),a
	ld a,c
	LD (mon_txt + 1),a

	;get the year
	inc HL
	LD a,(HL)
	call NUMBER_TO_ASC
	ld a,b
	LD (yea_txt),a
	ld a,c
	LD (yea_txt + 1),a

	
	ld hl,MsgDate
	CALL PrintMsg
	
	

	ret
NUMBER_TO_ASC:
	LD a,(HL)
	
	; get just the upper bits
	SRA A
	SRA A
	SRA A
	SRA A 
	add 48 ;convert number to ASCII
	LD b,a
	
	;now the lower bits
	LD a,(HL)
	and 0x0f ;just the lower bits
	add 48 ;convert number to ASCII
	LD c,a
	
	ret
	
LOAD_PREPARE_AND_MULT:
	ld a,(HL)
	and 0x7F ; clear the bit 7 
PREPARE_AND_MULT:
	SRA a
	SRA a
	SRA a
	SRA a
	CALL X10
	ld b,a
	ld a,(HL)
	and 0x0F
	add a,b
	
	ret
	
SEND_DATA:
	; 8 bits
	ld h,8
SEND_DATA_LOOP:	
	
	;next bit
	RLC L
	
	ld a,L
	CALL SDA
	
	call PULSE_CLOCK
	
	dec h
	jr nz, SEND_DATA_LOOP
	
		
WAIT_ACK
	;free the line to wait for the ACK
	CALL SDA1
	call PULSE_CLOCK

	ret

READ:
	;free the data line
	CALL SDA1
	
	; lets read 8 bits
	ld D,8	
READ_LOOP:

	;next bit
	rlc (hl)
	
	;clock is high
	CALL SCL1	
	
	;read the bit
	ld b,PORT_DATA
	in a,(c)
	
	;is it 1?
	and 1
	
	jr nz, set_bit
	res 0,(hl)
	jr end_set
	
set_bit:
	set 0,(hl)
	
end_set:

	
	;clock is low
	CALL SCL0
	
	
	dec d
	
	;go to next bit
	jr nz, READ_LOOP
	
	;finish the byte read
	ret
	
SEND_NACK:
	ld a,1
	jr SEND_ACK_NACK
	
SEND_ACK:	
	xor a  ;a=0	
	
SEND_ACK_NACK:
	
	CALL SDA
	
	call PULSE_CLOCK
	
;	free the data line
	CALL SDA1

	ret


START_SEQUENCE:	

	;high in both i2c pins, before begin
	ld a,1
	ld c, PORT
	CALL SCL 
	CALL SDA
	
	;high to low when clock is high
	CALL SDA0
	
	;low the clock to start sending data
	CALL SCL
	
	ret
	
SDA0:
	xor a 
	jr SDA
	
SDA1: 
	ld a,1

SDA: 
	ld b,PORT_DATA
	 OUT (c), a
	 ret

SCL0:
	xor a 
	jr SCL
SCL1:
	ld a,1
SCL:
	ld b,PORT_CLOCK
	OUT (c), a
	ret
	
PULSE_CLOCK:
	CALL SCL1
	CALL SCL0
	ret
	
; input A, output A = A * 10
X10
	ld b,a
	add a,a
	add a,a
	add a,a
	add a,b
	add a,b
	ret

PrintMsg         
	ld a,(hl)
	or a
	ret z
	rst 10h
	inc hl
	jr PrintMsg
	
MsgDate db "The date is "
day_txt db "  "
slash1  db "/"
mon_txt db "  "
slash2  db "/20"
yea_txt db "  "
endmsg  db 13,13,0

MsgUsage db "usage: ",13
	     db "date <ENTER>",13
	     db "show current date",13,13
	     db "date DD/MM/YYYY <ENTER>",13
	     db "set the date",13,13
	     db "Date must be greater",13
	     db "than 01/01/2000 !",13,13,0
					
SEC		defb 0		
MIN		defb 0	
HOU		defb 0	
DAY		defb 0		 
DATE	defb 0	
MON		defb 0	
YEA		defb 0	
	