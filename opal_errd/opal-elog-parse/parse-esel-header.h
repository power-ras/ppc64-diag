#ifndef _H_PARSE_ESEL_HEADER
#define _H_PARSE_ESEL_HEADER

#include <inttypes.h>

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

int parse_esel_header(const char* buf);
int print_esel_header(const char* buf);

#endif /* _H_PARSE_ESEL_HEADER */
