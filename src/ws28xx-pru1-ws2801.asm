;* PRU1 Application
;*

	.include "ws28xx-inc.asm"

	.sect "text:main"
	.global main

main:
	;* enable master OCP
	LBCO &R0, CONST_PRUCFG, 4, 4
	CLR R0, R0, 4
	SBCO &R0, CONST_PRUCFG, 4, 4

	;* assure we have blank shared memory
	.asg R2,    OFFSET_REG
	.asg R16,   DATA_REGS

	LDI32 R0, CONST_SHARED_MEM
	LDI UNIVERSE_COUNT, 0 ; universe counter
$M1:
	LDI SLOT_COUNT, 0     ; slot counter
	JMP $M3
$M2:
	ADD SLOT_COUNT, SLOT_COUNT, 1
$M3:
	;* zero memory
	LDI32 R2, 0
	SBBO &R2, R0, 0, 4

	;* next slot
	ADD R0, R0, 4
	QBNE $M2, SLOT_COUNT, MAX_SLOTS

	;* next universe
	ADD UNIVERSE_COUNT, UNIVERSE_COUNT, 1
	QBNE $M2, UNIVERSE_COUNT, MAX_UNIVERSES

	;* default to max slots to 255
	LDI32 OFFSET_REG, CONST_MAX_SLOTS
	LDI MAX_SLOTS, 255
	SBBO &MAX_SLOTS, OFFSET_REG, 0, 1

	;* Blank the LED strips now
	JMP $M6

	;* Latch Data Out
	;*
$M4:
	;* Clear interrupt
	LDI R4.w2, 0
	LDI R4.w0, PRU0_PRU1_INTERRUPT 
	SBCO &R4, CONST_PRUSSINTC, SICR_OFFSET, 4 

	;* 250 uS delay
	LATCH_DATA
	LATCH_DATA
	LATCH_DATA
	LATCH_DATA
	LATCH_DATA

	;* 250 uS delay
	LATCH_DATA
	LATCH_DATA
	LATCH_DATA
	LATCH_DATA
	LATCH_DATA

$M5:
	;* Spin till we get an update interrupt from PRU0	
	QBBC $M5, R31, 31

	LDI32 OFFSET_REG, CONST_MAX_SLOTS
	LBBO &MAX_SLOTS, OFFSET_REG, 0, 1
$M6:	
	LDI SLOT_COUNT, 0     ; slot counter
	LDI32 OFFSET_REG, CONST_SHARED_MEM

	; be sure we do a zero count
	JMP $M8
$M7:
	ADD SLOT_COUNT, SLOT_COUNT, 1
$M8:
	; use 14 registers, one for each universe
	LBBO &DATA_REGS, OFFSET_REG, 0, 4 * MAX_UNIVERSES

	LDI BIT_COUNT, 0      ; bit counter
$M9:
	;* Assemble data for output
	ASSEMBLE_DATA
	CLR R14.w0, R14.w0, 11
	MOV R30.w0, R14.w0

	DELAY_CYCLES 70 ; delay 0.40 uS - 10 clocks

	SET R30.w0, R30.w0, 11
	DELAY_CYCLES 63 ; delay 0.40 uS - 17 clocks

	;* shift everything right
	SHIFT_DATA

	ADD BIT_COUNT, BIT_COUNT, 1
	QBNE $M9, BIT_COUNT, 24

	;* next slot
	ADD OFFSET_REG, OFFSET_REG, 4 * MAX_UNIVERSES
	QBNE $M7, SLOT_COUNT, MAX_SLOTS

	JMP $M4
