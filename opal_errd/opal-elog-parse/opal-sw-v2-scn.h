#ifndef _H_OPAL_SW_V2_SCN
#define _H_OPAL_SW_V2_SCN

#include <inttypes.h>

struct opal_sw_v2_scn {
	uint32_t rc;
	uint16_t file_id;
	uint16_t location_id;
	uint32_t object_id;
} __attribute__((packed));

#endif /* _H_OPAL_SW_V2_SCN */
