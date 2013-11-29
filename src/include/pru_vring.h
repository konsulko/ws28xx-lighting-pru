#ifndef PRU_VRING_H
#define PRU_VRING_H

#include "debug.h"

#include "remoteproc.h"
#include "virtio_ring.h"

struct pru_vring {
	struct vring vr;
	const char *name;
	u32 align;
	u32 notifyid;

	u16 num_mask;
	u16 last_avail;
	u16 last_avail_orig;

	u16 last_used_idx;

	u16 * volatile avail_idx_p;
	u16 * volatile used_idx_p;
	u16 * volatile avail_ring_p;
};

struct pru_vring_elem {

	u32 out_len;	/* sum length of out descs */
	u16 out_num;	/* number of out descs */
	u16 out_idx;	/* index of first out desc */

	u32 in_len;	/* sum length on in descs */
	u16 in_num;	/* number of in descs */
	u16 in_idx;	/* index of first in desc */

	u16 idx;	/* first idx */
};

static inline u16 pru_vring_avail_idx(struct pru_vring *pvr)
{
	u16 val;

	val = *pvr->avail_idx_p;
	return val;
}

static inline u16 pru_vring_used_idx(struct pru_vring *pvr)
{
	return *pvr->used_idx_p;
}

static inline void pru_vring_used_idx_set(struct pru_vring *pvr, u16 idx)
{
	*pvr->used_idx_p = idx;
}

static inline u16 pru_vring_avail_ring(struct pru_vring *pvr, u16 i)
{
	BUG_ON(i >= pvr->vr.num);
	return pvr->avail_ring_p[i];
}

static inline struct vring_desc *pru_vring_desc(struct pru_vring *pvr, u16 idx)
{
	return &pvr->vr.desc[idx & pvr->num_mask];
}

/* if > num something is very very wrong */
static inline u16 pru_vring_num_heads(struct pru_vring *pvr, u16 idx)
{
	u16 num_heads = pru_vring_avail_idx(pvr) - idx;

	BUG_ON(num_heads > pvr->vr.num);
	return num_heads;
}

static inline u16 pru_vring_get_head(struct pru_vring *pvr, u16 idx)
{
	u16 head = pru_vring_avail_ring(pvr, idx & pvr->num_mask);

	BUG_ON(head >= pvr->vr.num);
	return head;
}

static inline u8
pru_vring_buf_is_avail(struct pru_vring *pvr)
{
	return pru_vring_num_heads(pvr, pvr->last_avail) != 0;
}

static inline struct vring_desc *
pru_vring_get_next_avail_desc(struct pru_vring *pvr)
{
	return pru_vring_desc(pvr, pru_vring_get_head(pvr, pvr->last_avail++));
}

static inline void pru_vring_push_one(struct pru_vring *pvr, u32 len)
{
	u16 old, idx;
	struct vring_used_elem *vru;

	/* first fill */
	old = pru_vring_used_idx(pvr);
	idx = old & pvr->num_mask;

	vru = &pvr->vr.used->ring[idx];
	vru->id = idx & pvr->num_mask;
	vru->len = len;

	/* now update */
	pru_vring_used_idx_set(pvr, ++old);
}

void pru_vring_init(struct pru_vring *pvr, const char *name,
		const struct fw_rsc_vdev_vring *rsc_vring);
void pru_vring_elem_init(struct pru_vring *pvr, struct pru_vring_elem *pvre);
int pru_vring_pop(struct pru_vring *pvr, struct pru_vring_elem *pvre);
void pru_vring_push(struct pru_vring *pvr, const struct pru_vring_elem *pvre,
		u32 len);

#ifdef DEBUG
void dump_vring(const char *name, struct vring *vring, unsigned int align);
#else
#define dump_vring(name, vring, align) \
	do { } while(0)
#endif

#endif
