#ifndef _H_OPAL_LR_SCN
#define _H_OPAL_LR_SCN

#include "opal-v6-hdr.h"

#define LR_RES_TYPE_PROC 0x10
#define LR_RES_TYPE_SHARED_PROC 0x11
#define LR_RES_TYPE_MEMORY_PAGE 0x40
#define LR_RES_TYPE_MEMORY_LMB 0x41

struct opal_lr_scn {
	struct opal_v6_hdr v6hdr;
	uint8_t res_type;
	uint8_t reserved;
	uint16_t capacity;
	uint32_t shared;
	uint32_t memory_addr;
} __attribute__((packed));

int parse_lr_scn(struct opal_lr_scn **r_lr,
                 struct opal_v6_hdr *hdr, const char *buf, int buflen);

int print_lr_scn(const struct opal_lr_scn *lr);

#endif /* _H_OPAL_LR_SCN */
