#include "opal-esel-parse.h"

bool is_esel_header(const char *buf)
{
	const struct esel_header* bufhdr = (struct esel_header*)buf;

	if (bufhdr->record_type == ESEL_RECORD_TYPE &&
	    bufhdr->signature == ESEL_SIGNATURE)
		return 1;

	return 0;
}

