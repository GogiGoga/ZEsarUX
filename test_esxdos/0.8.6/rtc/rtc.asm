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
; RTC.SYS for ESXDOS
;
; Thanks to VELESOFT for the help.
;
; Max size for this compiled ASM is 256 bytes!!!
;
; OUTPUT
; reg BC is Date 
;		year - 1980 (7 bits) + month (4 bits) + day (5 bits)
;
; reg DE is Time
;	hours (5 bits) + minutes (6 bits) + seconds/2 (5 bits)
;
;
; ds1307 serial I2C RTC 
;  	11010001 = 0xD0 = read
;	11010000 = 0xD1 = write
;
; SCL port at 0x103B
; SDA port at 0x113B
;

	DEVICE   ZXSPECTRUM48

	output "rtc.sys"

	org 0x2700

PORT		equ 0x3B
PORT_CLOCK 	equ 0x10
PORT_DATA 	equ 0x11

  
START:
	
	; save AF and HL
	; BC and DE will contain our date and time
	push af
	push hl
	
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

	
	;-------------------------------------------------
	;prepare the bytes to ESXDOS
	
	;OK, but commented out to save space
	;prepare SECONDS
;	LD HL,SEC
	
;	CALL LOAD_PREPARE_AND_MULT
	
;	srl a ;seconds / 2
	
;	ld e,a ; save the SECONDS first 5 bits in E
	
	;debug sec
;	LD(HL),a
	
	;prepare MINUTES
;	inc HL
	
;	CALL LOAD_PREPARE_AND_MULT
	
	;debug min
;	LD(HL),a
	
	; 3 MSB bits fom minutes in D
;	ld d,a
;	srl d
;	srl d
;	srl d
;
	; 3 LBS from minutes
;	sla a
;	sla a
;	sla a
;	sla a
;	sla a
;	or e ; combine with SECONDS
;	ld e,a ; save the 3 LSB minute bits in E
	
	;prepare HOURS
;	inc HL

	;-------------------------------------------
	LD HL, DATE
	
	call LOAD_PREPARE_AND_MULT
	
;	 reg BC is Date 
;		year - 1980 (7 bits) + month (4 bits) + day (5 bits)

	ld c,a ; save day in c
	
	;debug day
	;LD(HL),a
	
	;prepare MONTH
	inc HL
	
	CALL LOAD_PREPARE_AND_MULT
	
	;debug month
	;LD(HL),a
	
	; MSB bit from month in d
	ld d,a
	srl d
	srl d
	srl d
	
	; 3 LBS from month
	sla a
	sla a
	sla a
	sla a
	sla a
	or c ; combine with day
	LD C,A ;store
	
	;prepare YEAR
	inc HL
	
	ld a,(HL)
	CALL PREPARE_AND_MULT
	
	;debug year
	;LD(HL),a
	
	;now we have the year in A. format 00-99 (2000 to 2099) 
	add a,20 ;(current year - 1980)
	sla a ;get 7 LSB
	or d ; and combine with MONTH
	LD B,A; STORE the result in B
	
END:	

	push bc

	ld bc, 0x0d03
	in a,(c)
	res 0,a
	out (c),a
	
	pop bc
	
	;4951 = 17/10/2016


	;recover HL and AF
	pop hl
	pop af
	
; return without error
; the Carry flag is clearead
	or a ;clear the carry

;return with error
; the carry flag is set
;	scf

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
		;8 bits
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

	;free the line to wait the ACK
	CALL SDA1
	
	call PULSE_CLOCK
		
	ret

READ:
;	free the data line
	CALL SDA1
	
; lets read 8 bits
	ld D,8	
READ_LOOP:

	;position for next bit
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
	;high in both i2c, before begin
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
	
SEC		defb 0		
MIN		defb 0	
HOU		defb 0	
DAY		defb 0		 
DATE	defb 0	
MON		defb 0	
YEA		defb 0	


	