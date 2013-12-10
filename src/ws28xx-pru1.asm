;*
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
$M2:
	;* zero memory
	LDI32 R2, 0
	SBBO &R2, R0, 0, 4

	;* next slot
	ADD SLOT_COUNT, SLOT_COUNT, 1
	ADD R0, R0, 4
	QBNE $M2, SLOT_COUNT, MAX_SLOTS

	;* next universe
	ADD UNIVERSE_COUNT, UNIVERSE_COUNT, 1
	QBNE $M1, UNIVERSE_COUNT, MAX_UNIVERSES

	;* Blank the LED strips now
	JMP $M5
	
	;* Latch Data Out
	;*
$M3:
	;* Clear interrupt
	LDI R4.w2, 0
	LDI R4.w0, PRU0_PRU1_INTERRUPT 
	SBCO &R4, CONST_PRUSSINTC, SICR_OFFSET, 4 

	LATCH_DATA

$M4:
	;* Spin till we get an update interrupt from PRU0	
	QBBC $M4, R31, 31
$M5:	
	LDI SLOT_COUNT, 0     ; slot counter
	LDI32 OFFSET_REG, CONST_SHARED_MEM
$M6:
	; use 14 registers, one for each universe
	LBBO &DATA_REGS, OFFSET_REG, 0, 4 * MAX_UNIVERSES

	LDI BIT_COUNT, 0      ; bit counter
$M7:
	;* Logic Low  -> high 0.40 uS -> low 0.85 uS (1.25 uS)
	;* Logic High -> high 0.80 uS -> low 0.45 uS (1.25 uS)
	;*

	;* start for logic high + low
	LDI R30.w0, 0x0FFF
	DELAY_CYCLES 53 ; delay 0.40 uS - 27 clocks

	;* Assemble data for output
	ASSEMBLE_DATA

	MOV R30.w0, R14.w0
	DELAY_CYCLES 70 ; delay 0.40 uS - 10 clocks

	;* shift everything right
	SHIFT_DATA

	LDI R30.w0, 0x0000
	DELAY_CYCLES 90 ; delay 0.45 uS

	ADD BIT_COUNT, BIT_COUNT, 1
	QBNE $M7, BIT_COUNT, 24

	;* next slot
	ADD SLOT_COUNT, SLOT_COUNT, 1
	ADD OFFSET_REG, OFFSET_REG, 4 * MAX_UNIVERSES
	QBNE $M6, SLOT_COUNT, MAX_SLOTS

	JMP $M3
