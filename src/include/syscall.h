#ifndef SYSCALL_H
#define SYSCALL_H

#include "stdlib.h"

#include "linux_types.h"

static inline void *pa_to_da(u32 pa)
{
	/* we don't support physical addresses in GPMC */
	if (pa < 0x00080000)
		return NULL;

	return (void *)pa;
}

#define SC_HALT			0	/* halt system */
#define SC_PUTC			1	/* output a single char */
#define SC_EXIT			2	/* exit (with a value) */
#define SC_PUTS			3	/* output a null terminated string */

#define SC_GET_CFG	4	/* get configuration */
#define  SC_GET_CFG_VRING_NR   0 /* get vring number */
#define  SC_GET_CFG_VRING_INFO 1	/* get info about the vring */
#define  SC_GET_CFG_RESOURCE_TABLE 2	/* get the resource table */

#define SC_DOWNCALL_READY	254	/* host requested a downcall, ack it, and execute */
#define SC_DOWNCALL_DONE	255	/* call is performed, inform the host */

/* in syscall.asm */
extern int syscall(u32 nr);
extern int syscall1(u32 nr, u32 arg0);
extern int syscall2(u32 nr, u32 arg0, u32 arg1);
extern int syscall3(u32 nr, u32 arg0, u32 arg1, u32 arg2);

static inline void sc_halt(void)
{
	syscall(SC_HALT);
}

static inline void sc_putc(char c)
{
	syscall1(SC_PUTC, (u32)c);
}

static inline void sc_exit(int code)
{
	syscall1(SC_EXIT, (u32)code);
}

static inline void sc_puts(const char *str)
{
	syscall1(SC_PUTS, (u32)str);
}

static inline int sc_get_cfg(int cfg, int nr, void *ptr)
{
	return syscall3(SC_GET_CFG, (u32)cfg, (u32)nr,
			(u32)ptr);
}

static inline int sc_get_cfg_vring_nr(void)
{
	return sc_get_cfg(SC_GET_CFG_VRING_NR, 0, NULL);
}

struct vring_info {
	u32 paddr;
	u32 num;
	u32 align;
	u32 pad;
};

static inline int sc_get_cfg_vring_info(int idx, struct vring_info *vrinfo)
{
	return sc_get_cfg(SC_GET_CFG_VRING_INFO, idx, vrinfo);
}

struct resource_table;

static inline struct resource_table *sc_get_cfg_resource_table(void)
{
	return (void *)sc_get_cfg(SC_GET_CFG_RESOURCE_TABLE, 0, NULL);
}

/* downcall is handled by assembly */

extern void sc_downcall(int (*handler)(u32 nr, u32 arg0, u32 arg1,
			u32 arg2, u32 arg3, u32 arg4));

#endif
