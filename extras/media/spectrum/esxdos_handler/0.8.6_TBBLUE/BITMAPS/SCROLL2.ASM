	device zxspectrum48

KEYAND_CAPS		equ %00000001
KEYAND_Z		equ	%00000010
KEYAND_X		equ	%00000100
KEYAND_C		equ	%00001000
KEYAND_V		equ	%00010000
KEYAND_A		equ	%00000001
KEYAND_S		equ	%00000010
KEYAND_D		equ	%00000100
KEYAND_F		equ	%00001000
KEYAND_G		equ	%00010000
KEYAND_Q		equ %00000001
KEYAND_W		equ	%00000010
KEYAND_E		equ	%00000100
KEYAND_R		equ	%00001000
KEYAND_T		equ	%00010000
KEYAND_1		equ %00000001
KEYAND_2		equ	%00000010
KEYAND_3		equ	%00000100
KEYAND_4		equ	%00001000
KEYAND_5		equ	%00010000
KEYAND_0		equ %00000001
KEYAND_9		equ	%00000010
KEYAND_8		equ	%00000100
KEYAND_7		equ	%00001000
KEYAND_6		equ	%00010000
KEYAND_P		equ %00000001
KEYAND_O		equ	%00000010
KEYAND_I		equ	%00000100
KEYAND_U		equ	%00001000
KEYAND_Y		equ	%00010000
KEYAND_ENTER	equ %00000001
KEYAND_L		equ	%00000010
KEYAND_K		equ	%00000100
KEYAND_J		equ	%00001000
KEYAND_H		equ	%00010000
KEYAND_SPACE	equ %00000001
KEYAND_SYM		equ	%00000010
KEYAND_M		equ	%00000100
KEYAND_N		equ	%00001000
KEYAND_B		equ	%00010000

KEY_CAPS	equ 7
KEY_Z		equ	7
KEY_X		equ	7
KEY_C		equ	7
KEY_V		equ	7
KEY_A		equ	6
KEY_S		equ	6
KEY_D		equ	6
KEY_F		equ	6
KEY_G		equ	6
KEY_Q		equ 5
KEY_W		equ	5
KEY_E		equ	5
KEY_R		equ	5
KEY_T		equ	5
KEY_1		equ 4
KEY_2		equ	4
KEY_3		equ	4
KEY_4		equ	4
KEY_5		equ	4
KEY_0		equ 3
KEY_9		equ	3
KEY_8		equ	3
KEY_7		equ	3
KEY_6		equ	3
KEY_P		equ 2
KEY_O		equ	2
KEY_I		equ	2
KEY_U		equ	2
KEY_Y		equ	2
KEY_ENTER	equ 1
KEY_L		equ	1
KEY_K		equ	1
KEY_J		equ	1
KEY_H		equ	1
KEY_SPACE	equ 0
KEY_SYM		equ	0
KEY_M		equ	0
KEY_N		equ	0
KEY_B		equ	0

;-------------------------------
	
	org	$8000
start
	call scroller

finish
	ld	bc,4667:xor a:out (c),a:ret

;-------------------------------

oldkeys	db	0,0,0,0,0,0,0,0	; 0 0 1 1
newkeys	db	0,0,0,0,0,0,0,0 ; 0 1 0 1
debkeys	db	0,0,0,0,0,0,0,0 ; 0 1 0 0

scankeys	ld	a,%01111111:ld hl,oldkeys:ld de,newkeys:ld bc,debkeys
.lp	push af:ld a,(de):ld (hl),a:pop af
	push af:in a,(254):cpl:ld (de),a:and (hl):ex de,hl:xor (hl):ex de,hl:ld (bc),a:pop af
	inc	 hl:inc de:inc bc:rrca:jr c,.lp:ret

pausekey call waitnokey
waitkey xor a:in a,(254):cpl:and $1f:jr z,waitkey:ret
waitnokey xor a:in a,(254):cpl:and $1f:jr nz,waitnokey:ret

;-------------------------------

gameframe db	0
scrx	dw	0
scry	dw	0
mapx	dw	0
mapy	dw	0

scroller
	ld	bc,$243b:ld a,21:out (c),a:ld bc,$253b:ld a,3:out (c),a
editlp
	call scankeys
	ld	hl,gameframe:inc (hl)
	halt
;point to scrool X reg
	ld bc,$243b:ld a,23:out (c),a:ld bc,$253b:ld a,(scry):out (c),a	

	xor a:out (254),a
	ld a,(newkeys+KEY_Q):and KEYAND_Q:jr z,.nup:ld hl,(mapy):dec hl:ld (mapy),hl
	ld	a,(scry):dec a:cp 255:jr c,.nu1:ld a,191
.nu1
	ld	(scry),a:call horizstrip
.nup
	ld a,(newkeys+KEY_A):and KEYAND_A:jr z,.ndn:ld hl,(mapy):inc hl:ld (mapy),hl:ld de,191:add hl,de
	ld	a,(scry):inc a:cp 192:jr c,.nd1:xor a
.nd1
	ld	(scry),a:call horizstrip	
.ndn
	ld	a,1:out(254),a
	ld a,(newkeys+KEY_O):and KEYAND_O:jr z,.nlt:ld a,(scrx):dec a:ld (scrx),a:ld hl,(mapx):dec hl:ld (mapx),hl:call vertstrip
.nlt
	ld	a,2:out(254),a
	ld a,(newkeys+KEY_P):and KEYAND_P:jr z,.nrt:ld a,(scrx):inc a:ld (scrx),a:ld hl,(mapx):inc hl:ld (mapx),hl:inc h:dec hl:call vertstrip
.nrt
	ld	a,7:out (254),a
	
	ld	a,(debkeys+KEY_SPACE):and KEYAND_SPACE:jp z,editlp

	ret

;at the moment it's not scrolling a map so I'm not affecting opposite axis offsets for when drawing strips, will do that once horizontal scroll is fixed

vertstrip
	ld bc,$243b:ld a,22:out (c),a:ld bc,$253b:ld a,(scrx):out (c),a	
;	ld	a,h:add a,a:add a,a:add a,a:add a,a:add a,l:ld e,a
;	ld bc,4667:ld a,$03:out (c),a:ld b,64:ld h,0
;.l1	ld (hl),e:inc h:djnz .l1
;	ld bc,4667:ld a,$43:out (c),a:ld b,64:ld h,0
;.l2	ld (hl),e:inc h:djnz .l2
;	ld bc,4667:ld a,$83:out (c),a:ld b,64:ld h,0
;.l3	ld (hl),e:inc h:djnz .l3
	ld	bc,4667:ld a,2:out (c),a
	ret

horizstrip
	ld bc,$243b:ld a,23:out (c),a:ld bc,$253b:ld a,(scry):out (c),a	
;	ld	a,h:add a,a:add a,a:add a,a:add a,a:add a,l:ld e,a
;	ld	a,l:and $c0:or 3:ld bc,4667:out (c),a:ld b,0:ld a,l:and 63:ld h,a
;.l1	ld (hl),e:inc l:djnz .l1
	ld	bc,4667:ld a,2:out (c),a
	ret
	
;-------------------------------

	savesna "scroller.sna",start

;-------------------------------
