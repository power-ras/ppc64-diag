#ifndef _H_OPAL_SW_V1_SCN
#define _H_OPAL_SW_V1_SCN

#include <inttypes.h>

struct opal_sw_v1_scn {
	uint32_t rc;
	uint32_t line_num;
	uint32_t object_id;
	uint8_t id_length;
	char file_id[0]; /* Variable length, NULL terminated, padded to 4 bytes */
} __attribute__((packed));

#endif /* _H_OPAL_SW_V1_SCN */
