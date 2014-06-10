#include <endian.h>

#include "opal-datetime.h"
#include "parse_helpers.h"

struct opal_datetime parse_opal_datetime(const struct opal_datetime in)
{
	struct opal_datetime out;

	out.year       = from_bcd16(be16toh(in.year));
	out.month      = from_bcd8(in.month);
	out.day        = from_bcd8(in.day);
	out.hour       = from_bcd8(in.hour);
	out.minutes    = from_bcd8(in.minutes);
	out.seconds    = from_bcd8(in.seconds);
	out.hundredths = from_bcd8(in.hundredths);

	return out;
}
