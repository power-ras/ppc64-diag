#ifndef _H_OPAL_ESEL_PARSE
#define _H_OPAL_ESEL_PARSE

#include <inttypes.h>
#include <stdbool.h>

#define ESEL_RECORD_TYPE	0xDF
#define ESEL_SIGNATURE		0xAA

struct esel_header {
	uint16_t id;
	uint8_t record_type;
	uint32_t timestamp;
	uint16_t genid;
	uint8_t evmrev;
	uint8_t sensor_type;
	uint8_t sensor_num;
	uint8_t dir_type;
	uint8_t signature;
	uint8_t reserved[2];
} __attribute__((packed));

bool is_esel_header(const char *buf);

#endif /* _H_OPAL_ESEL_PARSE */
