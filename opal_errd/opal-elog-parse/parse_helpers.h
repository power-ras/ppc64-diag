#ifndef _H_PARSE_HELPERS
#define _H_PARSE_HELPERS

#include <inttypes.h>

uint16_t from_bcd16(uint16_t bcd);

uint8_t from_bcd8(uint8_t bcd);

int check_buflen(int buflen, int min_length, const char *func);

#endif /* _H_PARSE_HELPERS */
