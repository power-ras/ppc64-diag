#include <inttypes.h>

uint16_t from_bcd16(uint16_t bcd)
{
	return (bcd & 0x000f) +
	       ((bcd & 0x00f0) >> 4) * 10 +
	       ((bcd & 0x0f00) >> 8) * 100 +
	       ((bcd & 0xf000) >> 12) * 1000;
}

uint8_t from_bcd8(uint8_t bcd)
{
	return (bcd & 0x0f) + ((bcd & 0xf0) >> 4) * 10;
}
