#ifndef _H_OPAL_DATETIME
#define _H_OPAL_DATETIME

#include <inttypes.h>

/* This comes in BCD for some reason, we convert it in parsing */
struct opal_datetime {
	uint16_t year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  hour;
	uint8_t  minutes;
	uint8_t  seconds;
	uint8_t  hundredths;
} __attribute__((packed));

struct opal_datetime parse_opal_datetime(const struct opal_datetime in);

#endif /* _H_OPAL_DATETIME */
