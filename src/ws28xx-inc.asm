;*
;* Various assembly utils
;*

	;*
	;* Constants
	;*

	.asg C0, CONST_PRUSSINTC
	.asg C2, CONST_PRUCFG
	.asg 36, SICR_OFFSET

	.asg 0x10000, CONST_SHARED_MEM

	.asg 17, PRU0_PRU1_INTERRUPT
	.asg 18, PRU1_PRU0_INTERRUPT

	.asg 255, MAX_SLOTS
	.asg 12,  MAX_UNIVERSES

	.asg R1.b0, UNIVERSE_COUNT
	.asg R1.b1, SLOT_COUNT
	.asg R1.b2, BIT_COUNT

	;*
	;* Spin around and kill time. 5 nanoseconds per cycle.
	;* 


DELAY_CYCLES	.macro NR

	LDI32 R14, NR
	SUB R14, R14, 5		;* function entry + overhead
	LSR R14, R14, 1		;* loop is by two
$1:
	SUB R14, R14, 1

	;* allow 32-bits of counting
	QBNE $1, R14.b0, 0
	QBNE $1, R14.b1, 0
	QBNE $1, R14.b2, 0
	QBNE $1, R14.b3, 0

	.endm


ASSEMBLE_DATA	.macro
	;* 13 - 27 clock cycles

	;* R14 has the assembled value to pipe out R30
	LDI32 R14, 0

	QBBC $B0, R18, 0
	OR R14.b0, R14.b0, 1<<0
$B0:
	QBBC $B1, R19, 0
	OR R14.b0, R14.b0, 1<<1
$B1:
	QBBC $B2, R20, 0
	OR R14.b0, R14.b0, 1<<2
$B2:
	QBBC $B3, R21, 0
	OR R14.b0, R14.b0, 1<<3
$B3:
	QBBC $B4, R22, 0
	OR R14.b0, R14.b0, 1<<4
$B4:
	QBBC $B5, R23, 0
	OR R14.b0, R14.b0, 1<<5
$B5:
	QBBC $B6, R24, 0
	OR R14.b0, R14.b0, 1<<6
$B6:
	QBBC $B7, R25, 0
	OR R14.b0, R14.b0, 1<<7
$B7:
	QBBC $B8, R26, 0
	OR R14.b1, R14.b1, 1<<0
$B8:
	QBBC $B9, R27, 0
	OR R14.b1, R14.b1, 1<<1
$B9:
	QBBC $B10, R28,0
	OR R14.b1, R14.b1, 1<<2
$B10:
	QBBC $B11, R29,0 
	OR R14.b1, R14.b1, 1<<3
$B11:

	.endm



SHIFT_DATA	.macro
	;* 12 clock ticks

	;* first 8 bits
	LSL R18, R18, 1
	LSL R19, R19, 1
	LSL R20, R20, 1
	LSL R21, R21, 1
	LSL R22, R22, 1
	LSL R23, R23, 1
	LSL R24, R24, 1
	LSL R25, R25, 1
	
	;* second 4 bits
	LSL R26, R26, 1
	LSL R27, R27, 1
	LSL R28, R28, 1
	LSL R29, R29, 1

	.endm
