/*
 *
 * PRU Remoteproc LED WS28xx Firmware
 *
 * Based on an original work of Pantelis Antoniou
 *
 * Copyright 2013, Matt Ranostay. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY MATT RANOSTAY ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATT RANOSTAY OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Matt Ranostay.
 *
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <strings.h>

/* pull in PRU0 functions */
#define PRU0

#include <debug.h>
#include <linux_types.h>
#include <pru_defs.h>
#include <pt.h>
#include <ws28xx.h>
#include <virtio_ids.h>
#include <prucomm.h>
#include <remoteproc.h>
#include <syscall.h>
#include <prurproc.h>
#include <pru_vring.h>

struct pru_vring tx_ring;
struct pru_vring rx_ring;

static struct pt pt_event;
static struct pt pt_prompt;
static struct pt pt_tx;

/*
 * Globals
 */ 

static u32 *SHARED_MEM = (u32 *) DPRAM_SHARED;
#define PRU_LED_DATA 0x1000

#define RX_SIZE 32
#define RX_SIZE_MASK (RX_SIZE - 1)
#define TX_SIZE 64
#define TX_SIZE_MASK (TX_SIZE - 1)

static u8 rx_in;
static u8 rx_out;
static u8 rx_cnt;
static char rx_buf[RX_SIZE];

static u8 tx_in;
static u8 tx_out;
static u8 tx_cnt;
static char tx_buf[TX_SIZE];

/* might yield, make sure _ch is static */
#define RX_IN(_ch) \
	do { \
		/* yield while there's no space */ \
		PT_YIELD_UNTIL(pt, rx_cnt < RX_SIZE); \
		rx_buf[rx_in++ & RX_SIZE_MASK] = (_ch); \
		rx_cnt++; \
	} while (0)

/* waits until input (bah - no gcc makes me a sad panda) */
#define RX_OUT(_ch) \
	do { \
		PT_WAIT_UNTIL(pt, rx_cnt > 0); \
	 	rx_cnt--; \
		(_ch) = rx_buf[rx_out++ & RX_SIZE_MASK]; \
	} while (0)

/* might yield, make sure _ch is static */
#define TX_IN(_ch) \
	do { \
		/* yield while there's no space */ \
		PT_YIELD_UNTIL(pt, tx_cnt < TX_SIZE); \
		tx_buf[tx_in++ & TX_SIZE_MASK] = (_ch); \
		tx_cnt++; \
	} while (0)

/* waits until input (bah - no gcc makes me a sad panda) */
#define TX_OUT(_ch) \
	do { \
		PT_WAIT_UNTIL(pt, tx_cnt > 0); \
	 	tx_cnt--; \
		(_ch) = tx_buf[tx_out++ & TX_SIZE_MASK]; \
	} while (0)

/* non blocking version */
#define TX_OUT_NB(_ch) \
	do { \
	 	tx_cnt--; \
		(_ch) = tx_buf[tx_out++ & TX_SIZE_MASK]; \
	} while (0)

static inline void blank_slots(u8 universe)
{
	int i;
	int offset = universe;

	for (i = 0; i < MAX_SLOTS; i++) {
		SHARED_MEM[offset] = 0;
		offset += MAX_UNIVERSES;
	}
}

static inline void write_data(u8 universe, u8 slot_num, u32 data)
{
	SHARED_MEM[universe + (MAX_UNIVERSES * slot_num)] = data;
}


static int handle_downcall(u32 id, u32 arg0, u32 arg1, u32 arg2,
		u32 arg3, u32 arg4)
{
	int max_slots = SHARED_MEM[CONFIG_MAX_SLOTS];

	switch (id) {
		case DC_LED_BLANK:
			blank_slots(arg0);

			break;
		case DC_LED_WRITE:
			write_data(arg0, arg1, arg2);

			break;
		case DC_LED_WRITE_BURST: {
			int i = 0;
			u32 *data = (u32 *) PRU_LED_DATA;

			for (i = 0; i < max_slots; i++) {
				write_data(arg0, i, *data++);
			}
			break;
		}
		case DC_LED_LATCH:
			SIGNAL_EVENT(SYSEV_THIS_PRU_TO_OTHER_PRU);

			break;
		default:
			sc_printf("bad downcall with id %d", id);
			/* error */
			return -EINVAL;
	}

	/* Allow kernel binding to know of slot count  */
	*((u32 *) PRU_LED_DATA) = max_slots;

	return 0;
}


static int event_thread(struct pt *pt)
{
	static struct pru_vring_elem pvre;
	static u16 idx, count;
	static u32 rx_len, len;
	struct vring_desc *vrd;
	static char *ptr;

	PT_BEGIN(pt);

	for (;;) {
		/* wait until we get the indication */
		PT_WAIT_UNTIL(pt,
			/* pru_signal() && */
			(PINTC_SRSR0 & SYSEV_THIS_PRU_INCOMING_MASK) != 0);

		/* downcall from the host */
		if (PINTC_SRSR0 & BIT(SYSEV_ARM_TO_THIS_PRU)) {
			PINTC_SICR = SYSEV_ARM_TO_THIS_PRU;

			PT_WAIT_UNTIL(pt, !(PINTC_SRSR0 & BIT(SYSEV_THIS_PRU_TO_OTHER_PRU)));
			sc_downcall(handle_downcall);
		}

		if (PINTC_SRSR0 & BIT(SYSEV_VR_ARM_TO_THIS_PRU)) {
			PINTC_SICR = SYSEV_VR_ARM_TO_THIS_PRU;

			while (pru_vring_pop(&rx_ring, &pvre)) {

				/* process the incoming buffers (??? not used) */
				if ((count = pvre.in_num) > 0) {
					idx = pvre.in_idx;
					while (count-- > 0) {
						vrd = pru_vring_desc(&rx_ring, idx);
						ptr = pa_to_da(vrd->addr);
						idx++;
					}
				}

				rx_len = 0;

				/* process the outgoing buffers (this is our RX) */
				if (pvre.out_num > 0) {

					idx = pvre.out_idx;
					count = pvre.out_num;
					while (count-- > 0) {
						vrd = pru_vring_desc(&rx_ring, idx);
						ptr = pa_to_da(vrd->addr);
						len = vrd->len;

						/* put it in the rx buffer (can yield) */
						while (len-- > 0)
							RX_IN(*ptr++);

						rx_len += vrd->len;

						idx++;
					}
				}

				pru_vring_push(&rx_ring, &pvre, rx_len);
				PT_WAIT_UNTIL(pt, !(PINTC_SRSR0 & BIT(SYSEV_VR_THIS_PRU_TO_ARM)));

				/* VRING PRU -> ARM */
				SIGNAL_EVENT(SYSEV_VR_THIS_PRU_TO_ARM);
			}
		}

		if (PINTC_SRSR0 & BIT(SYSEV_OTHER_PRU_TO_THIS_PRU)) {
			PINTC_SICR = SYSEV_OTHER_PRU_TO_THIS_PRU;
		}
	}

	/* get around warning */
	PT_YIELD(pt);

	PT_END(pt);
}


/* context for console I/O */
#define CONSOLE_LINE_MAX	80

struct console_cxt {
	struct pt pt;
	char *buf;
	int size;
	int max_size;
	u8 flags;
#define S_WRITEMODE	0x01
#define S_CRLF		0x02
#define S_LINEMODE	0x04
#define S_ECHO		0x08
#define S_READLINE	0x10
};
/* pt must always! me the first member */
#define to_console_cxt(_pt)	((struct console_cxt *)(void *)(_pt))

static int console_thread(struct pt *pt)
{
	struct console_cxt *c = to_console_cxt(pt);	/* always called */
	static char ch;

	PT_BEGIN(pt);
	if (c->flags & S_WRITEMODE) {
		c->size = 0;
		while (c->size < c->max_size) {
			ch = *c->buf;
			if ((c->flags & S_LINEMODE) && ch == '\0')
				goto out;
			c->size++;
			c->buf++;
			/* '\n' -> '\r\n' */
			if ((c->flags & S_CRLF) && ch == '\n')
				TX_IN('\r');
			TX_IN(ch);
		}

	} else {
		c->size = 0;
		for (;;) {
rx_again:
			if ((c->flags & S_READLINE) == 0 &&
					c->size >= c->max_size)
				goto out;

			RX_OUT(ch);

			/* only support backspace (or del) */
			if ((c->flags & S_READLINE) &&
					(ch == '\b' || ch == 0x7f)) {
				if (c->size > 0) {
					c->size--;
					c->buf--;
					TX_IN('\b');
					TX_IN(' ');
					TX_IN('\b');
				}
				goto rx_again;
			}

			if ((c->flags & S_LINEMODE) &&
					(ch == '\r' || ch == '\n')) {
				if (c->size < c->max_size)
					*c->buf = '\0';
				goto out;
			}

			if ((c->flags & S_ECHO))
				TX_IN(ch);

			/* stop at max */
			if ((c->flags & S_READLINE) && c->size >= c->max_size)
				goto rx_again;

			*c->buf++ = ch;
			c->size++;
		}
	}
out:
	PT_YIELD(pt);

	PT_END(pt);
}

/* context unions (only one active at the time) */
static struct console_cxt console_cxt;

#define c_puts(_str) \
	do { \
		console_cxt.buf = (void *)(_str); \
		console_cxt.size = 0; \
		console_cxt.max_size = strlen(console_cxt.buf); \
		console_cxt.flags = S_CRLF | S_LINEMODE | S_WRITEMODE; \
		PT_SPAWN(pt, &console_cxt.pt, console_thread(&console_cxt.pt)); \
	} while(0)

#define c_write(_str, _sz) \
	do { \
		console_cxt.buf = (void *)(_str); \
		console_cxt.size = 0; \
		console_cxt.max_size = (_sz); \
		console_cxt.flags = S_CRLF | S_WRITEMODE; \
		PT_SPAWN(pt, &console_cxt.pt, console_thread(&console_cxt.pt)); \
	} while(0)

#define c_getc(_ch) \
	do { \
		console_cxt.buf = (void *) &(_ch); \
		console_cxt.size = 0; \
		console_cxt.max_size = 1; \
		console_cxt.flags = S_CRLF | S_LINEMODE; \
		PT_SPAWN(pt, &console_cxt.pt, console_thread(&console_cxt.pt)); \
	} while(0)

#define c_gets(_buf, _max) \
	do { \
		console_cxt.buf = (void *)(_buf); \
		console_cxt.size = 0; \
		console_cxt.max_size = _max; \
		console_cxt.flags = S_CRLF | S_LINEMODE; \
		PT_SPAWN(pt, &console_cxt.pt, console_thread(&console_cxt.pt)); \
	} while(0)

#define c_read(_buf, _sz) \
	do { \
		console_cxt.buf = (void *)(_buf); \
		console_cxt.size = 0; \
		console_cxt.max_size = (_sz); \
		console_cxt.flags = S_CRLF; \
		PT_SPAWN(pt, &console_cxt.pt, console_thread(&console_cxt.pt)); \
	} while(0)

#define c_readline(_buf, _sz) \
	do { \
		console_cxt.buf = (void *)(_buf); \
		console_cxt.size = 0; \
		console_cxt.max_size = (_sz); \
		console_cxt.flags = S_CRLF | S_LINEMODE | S_ECHO | S_READLINE; \
		PT_SPAWN(pt, &console_cxt.pt, console_thread(&console_cxt.pt)); \
		(_sz) = console_cxt.size; \
	} while(0)

static char *parse_u32(char *str, u32 *valp)
{
	u32 result;
	u32 tval;
	char c;

	while (*str == ' ')
		str++;

	result = 0;
	for (;;) {
		c = *str;
		tval = (u32)(c - '0');
		if (tval >= 10)
			break;
		result *= 10;
		result += tval;
		str++;
	}
	*valp = result;
	return str;
}


static char *parse_hex(char *str, u32 *valp)
{
	u32 result;
	u32 tval;
	char c;

	while (*str == ' ' || *str == '.')
		str++;

	result = 0;
	for (;;) {
		c = *str;
		tval = (u32)(c - '0');
		if (tval >= 10) {
			tval = (u32)(c - 'a');
			if (tval >= 6)
				break;
			tval += 10;
		}
		result *= 16;
		result += tval;
		str++;
	}
	
	*valp = result;
	return str;
}


static char *parse_u24(char *p, u32 *valp)
{
	u32 result = 0;
	u32 tval;

	p = parse_hex(p, &tval);
	result |= tval << 0;

	p = parse_hex(p, &tval);
	result |= tval << 8;

	p = parse_hex(p, &tval);
	result |= tval << 16;

	*valp = result;
	return p;
}



static int prompt_thread(struct pt *pt)
{
	static char ch1;
	static char *pp;
	static char linebuf[80];
	char buf[8];
	char *p;
	static int linesz;
	static u8 current_universe;
	u32 val;

	PT_BEGIN(pt);

	for (;;) {
again:
		sprintf((char *) &buf, "PRU#%d> ", current_universe);
		c_puts(buf);
		linesz = sizeof(linebuf);
		c_readline(linebuf, linesz);
		c_puts("\n");
		if (linesz == 0)
			goto again;

		ch1 = linebuf[0];
		pp = "";

		PT_WAIT_UNTIL(pt, !(PINTC_SRSR0 & BIT(SYSEV_THIS_PRU_TO_OTHER_PRU)));

		if (ch1 == '?') {
			c_puts("Help\n"
		 		" s <universe>              "
				"select universe 0-13\n"
		  		" b                         "
				"blanks slots 1-170\n"
				" m <val>                   "
				"max number of slots per universe 0-169\n"
		  		" w <num> <v1>.<v2>.<v3>    "
				"write 24-bit GRB value to slot number\n"
		  		" l                         "
				"latch data out the PRU1\n");
		} else if (ch1 == 's') {
			p = parse_u32(linebuf + 1, &val);
			if (val > MAX_UNIVERSES - 1) {
				pp = "*BAD*\n";
			} else { 
				current_universe = val;
			}
		} else if (ch1 == 'b') {
			blank_slots(current_universe);
		} else if (ch1 == 'm') {
			p = parse_u32(linebuf + 1, &val);
			
			if (val > MAX_SLOTS || val == 0) {
				pp = "*BAD\n";
			} else {
				SHARED_MEM[CONFIG_MAX_SLOTS] = val;
				*((u32 *) PRU_LED_DATA) = val;
			}
		} else if (ch1 == 'w') {
			p = parse_u32(linebuf + 1, &val);

			if (val > MAX_SLOTS) {
				pp = "*BAD*\n";
			} else {
				u32 rgb_data;
				p = parse_u24(p, &rgb_data);
				write_data(current_universe, val, rgb_data);
			}
		} else if (ch1 == 'l') {
			/*
			 * Tell PRU1 to update the LED strings
	 		*/

			SIGNAL_EVENT(SYSEV_THIS_PRU_TO_OTHER_PRU);
		} else {
			pp = "*BAD*\n";
		}

		c_puts(pp);
	}

	PT_YIELD(pt);

	PT_END(pt);
}

static struct fw_rsc_vdev *
resource_get_rsc_vdev(struct resource_table *res, int id, int idx)
{
	struct fw_rsc_hdr *rsc_hdr;
	struct fw_rsc_vdev *rsc_vdev;
	int i, j;

	j = 0;
	for (i = 0; i < res->num; i++) {
		rsc_hdr = (void *)((char *)res + res->offset[i]);
		if (rsc_hdr->type != RSC_VDEV)
			continue;
		rsc_vdev = (struct fw_rsc_vdev *)&rsc_hdr->data[0];
		if (id >= 0 && id != rsc_vdev->id)
			continue;
		if (j == idx)
			return rsc_vdev;
		j++;
	}

	return NULL;
}

static void resource_setup(void)
{
	struct resource_table *res;
	struct fw_rsc_vdev *rsc_vdev;
#ifdef DEBUG
	int i;
	struct fw_rsc_vdev_vring *rsc_vring;
#endif

	res = sc_get_cfg_resource_table();
	BUG_ON(res == NULL);

	/* get first RPROC_SERIAL VDEV resource */
	rsc_vdev = resource_get_rsc_vdev(res, VIRTIO_ID_RPROC_SERIAL, 0);
	BUG_ON(rsc_vdev == NULL);

	BUG_ON(rsc_vdev->num_of_vrings < 2);
#ifdef DEBUG
	for (i = 0, rsc_vring = rsc_vdev->vring; i < 2; i++, rsc_vring++) {
		sc_printf("VR#%d: da=0x%x align=0x%x num=0x%x notifyid=0x%x",
				i, rsc_vring->da, rsc_vring->align,
				rsc_vring->num, rsc_vring->notifyid);
	}
#endif

	pru_vring_init(&tx_ring, "tx", &rsc_vdev->vring[0]);
	pru_vring_init(&rx_ring, "rx", &rsc_vdev->vring[1]);
}

static int tx_thread(struct pt *pt)
{
	struct vring_desc *vrd = NULL;
	u32 chunk, i;
	char *ptr, ch;

	PT_BEGIN(pt);

	for (;;) {

		/* wait until we get the indication (and there's a buffer) */
		PT_WAIT_UNTIL(pt, tx_cnt && pru_vring_buf_is_avail(&tx_ring));

		vrd = pru_vring_get_next_avail_desc(&tx_ring);

		/* we don't support VRING_DESC_F_INDIRECT */
		BUG_ON(vrd->flags & VRING_DESC_F_INDIRECT);

		chunk = tx_cnt;
		if (chunk > vrd->len)
			chunk = vrd->len;

		ptr = pa_to_da(vrd->addr);

		for (i = 0; i < chunk; i++) {
			ch = tx_buf[tx_out++ & TX_SIZE_MASK];
			*ptr++ = ch;
		}
		tx_cnt -= chunk;
		vrd->len = chunk;
		vrd->flags &= ~VRING_DESC_F_NEXT;	/* last */

		pru_vring_push_one(&tx_ring, chunk);

		PT_WAIT_UNTIL(pt, !(PINTC_SRSR0 & BIT(SYSEV_VR_THIS_PRU_TO_ARM)));
		SIGNAL_EVENT(SYSEV_VR_THIS_PRU_TO_ARM);
	}

	PT_YIELD(pt);

	PT_END(pt);
}

int main(int argc, char *argv[])
{
	/* enable OCP master port */
	PRUCFG_SYSCFG &= ~SYSCFG_STANDBY_INIT;
	sc_printf("PRU0: Starting Lighting Firmware");

	PT_INIT(&pt_event);
	PT_INIT(&pt_prompt);
	PT_INIT(&pt_tx);

	resource_setup();
	rx_in = rx_out = rx_cnt = 0;
	tx_in = tx_out = tx_cnt = 0;

	for (;;) {
		event_thread(&pt_event);
		tx_thread(&pt_tx);
		prompt_thread(&pt_prompt);
	}
}
