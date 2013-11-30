#
# Makefile for an example remote proc PRU program
# It is assumed that the PRU compiler environment has been set
#

.PHONY: all clean

all: ws28xx-pru0.stripped ws28xx-pru0.lst ws28xx-pru1.stripped ws28xx-pru1.lst

CC=clpru
LD=lnkpru
AR=arpru
OBJDUMP=dispru
STRIP=strippru

# -v3				PRU version 3
# --c99 			C99 support
# --gcc 			Enable GCC extensions
# -O3				Optimization level maximum
# --printf_support=minimal 	Minimal printf
# -ppd				Generate dependencies *.pp
# -ppa				Continue after generating deps
# -DDEBUG			Enable debug
# CFLAGS= -v3 --c99 --gcc -O3 --printf_support=minimal -ppd -ppa -DDEBUG 
CFLAGS= -v3 --c99 --gcc -O3 -I ./include --printf_support=minimal -ppd -ppa --symdebug:none

# -cr 				Link using RAM auto init model (loader assisted)
# -x				Reread libs until no unresolved symbols found
LDFLAGS=-cr --diag_warning=225 -llnk-am33xx.cmd -x

STRIPFLAGS=-p

OBJS0:=ws28xx-pru0.obj syscall0.obj debug.obj pru_vring.obj
OBJS1:=ws28xx-pru1.obj

%.obj: %.c
	$(CC) $(CFLAGS) -c $<

%.obj: %.asm
	$(CC) $(CFLAGS) -c $<

ws28xx-pru0: $(OBJS0)
	$(CC) $(CFLAGS) $^ -z $(LDFLAGS) -o $@

ws28xx-pru1: $(OBJS1)
	$(CC) $(CFLAGS) $^ -z $(LDFLAGS) -o $@

ws28xx-pru0.stripped: ws28xx-pru0
	$(STRIP) $(STRIPFLAGS) $< -o $@

ws28xx-pru1.stripped: ws28xx-pru1
	$(STRIP) $(STRIPFLAGS) $< -o $@

ws28xx-pru0.lst: ws28xx-pru0
	$(OBJDUMP) -1 $< > $@

ws28xx-pru1.lst: ws28xx-pru1
	$(OBJDUMP) -1 $< > $@

clean:
	rm -f *.obj *.lst *.out *.stripped \
	ws28xx-pru0 ws28xx-pru1 tags

distclean: clean
	rm -f *.pp

# include any generated deps
-include $(patsubst %.obj,%.pp,$(OBJS))
