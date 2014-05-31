/****************************************************************************/
/* LNK.CMD                                                                  */
/*                                                                          */
/*   Usage:  clpru  <src files...> -z -o <out file> -m <map file> lnk.cmd   */
/*                                                                          */
/*   Description: This file is a sample command file that can be used       */
/*                for linking programs built with the PRU C                 */
/*                Compiler.   Use it as a guideline; you may want to change */
/*                the allocation scheme according to the size of your       */
/*                program and the memory layout of your target system.      */
/*                                                                          */
/*   Notes: (1)   You must specify the directory in which rts.lib is        */
/*                located.  Either add a "-i<directory>" line to this       */
/*                file, or use the system environment variable PRU_C_DIR to */
/*                specify a search path for libraries.                      */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
-cr                                        /* LINK USING C CONVENTIONS      */
-stack  0x0200                             /* SOFTWARE STACK SIZE           */
-heap   0x0200                             /* HEAP AREA SIZE                */
/*--args 0x100  */

/* SPECIFY THE SYSTEM MEMORY MAP */

/* memory map for am335x (8K data, 8K code) */
MEMORY
{
    PAGE 0:
       P_MEM    : org = 0x00000000   len = 0x00002000

    PAGE 1:
       D_MEM    : org = 0x00000000   len = 0x00000800
       C0       : org = 0x00020000   len = 0x00000300 CREGISTER=0
       C4       : org = 0x00026000   len = 0x00000100 CREGISTER=4
}

/* SPECIFY THE SECTIONS ALLOCATION INTO MEMORY */

SECTIONS
{
    .bss        : {} > D_MEM, PAGE 1      /* GLOBAL & STATIC VARS             */
    .data       : {} > D_MEM, PAGE 1
    .rodata     : {} > D_MEM, PAGE 1      /* CONSTANT DATA                    */
    .sysmem     : {} > D_MEM, PAGE 1      /* DYNAMIC MEMORY ALLOCATION AREA   */
    .stack      : {} > D_MEM, PAGE 1      /* SOFTWARE SYSTEM STACK            */
    .cinit      : {} > D_MEM, PAGE 1      /* INITIALIZATION TABLES            */
    .const      : {} > D_MEM, PAGE 1      /* CONSTANT DATA                    */
    .args       : {} > D_MEM, PAGE 1     
    .init_array : {} > D_MEM, PAGE 1      /* C++ CONSTRUCTOR TABLES           */
    .farbss     : {} > D_MEM, PAGE 1
    .fardata    : {} > D_MEM, PAGE 1
    .rofardata  : {} > D_MEM, PAGE 1

    .text       : {} > P_MEM, PAGE 0      /* CODE                             */
}


