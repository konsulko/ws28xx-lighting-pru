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

	.asg 170, MAX_SLOTS
	.asg 14,  MAX_UNIVERSES

	.asg R1.b0, UNIVERSE_COUNT
	.asg R1.b1, SLOT_COUNT
	.asg R1.b2, BIT_COUNT

	;*
	;* Spin around and kill time. 5 nanoseconds per cycle.
	;* 


DELAY_CYCLES	.macro NR

	LDI R14, NR
	SUB R14, R14, 3		;* function entry + overhead
	LSR R14, R14, 1		;* loop is by two
$1:
	SUB R14, R14, 1
	QBNE $1, R14, 0

	.endm

LATCH_DATA	.macro
	LDI R15, 200 ;* 50 uS
$1:
	DELAY_CYCLES 255

	SUB R15, R15, 1
	QBNE $1, R15, 0

	.endm


ASSEMBLE_DATA	.macro
	;* 15 - 29 clock cycles

	;* R14 has the assembled value to pipe out R30
	LDI R14, 0


	QBBC $1, R16, 0
	SET R14, R14, 0
$1:
	QBBC $2, R17, 0
	SET R14, R14, 1
$2:
	QBBC $3, R18, 0
	SET R14, R14, 2
$3:
	QBBC $4, R19, 0
	SET R14, R14, 3
$4:
	QBBC $5, R20, 0
	SET R14, R14, 4
$5:
	QBBC $6, R21, 0
	SET R14, R14, 5
$6:
	QBBC $7, R22, 0
	SET R14, R14, 6
$7:
	QBBC $8, R23, 0
	SET R14, R14, 7
$8:
	QBBC $9, R24, 0
	SET R14, R14, 8
$9:
	QBBC $10,R25, 0
	SET R14, R14, 9
$10:
	QBBC $11,R26, 0
	SET R14, R14, 10
$11:
	QBBC $12,R27, 0
	SET R14, R14, 11
$12:
	QBBC $13,R28, 0
	SET R14, R14, 12
$13:
	QBBC $14,R29, 0 
	SET R14, R14, 13
$14:

	.endm



SHIFT_DATA	.macro
	;* 14 clock ticks

	;* first 8 bits
	LSR R16, R16, 1
	LSR R17, R17, 1
	LSR R18, R18, 1
	LSR R19, R19, 1
	LSR R20, R20, 1
	LSR R21, R21, 1
	LSR R22, R22, 1
	LSR R23, R23, 1
	
	;* second 6 bits
	LSR R24, R24, 1
	LSR R25, R25, 1
	LSR R26, R26, 1
	LSR R27, R27, 1
	LSR R28, R28, 1
	LSR R29, R29, 1

	.endm
