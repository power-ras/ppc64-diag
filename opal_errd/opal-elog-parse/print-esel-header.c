#include <stdio.h>
#include <endian.h>

#include "opal-esel-parse.h"

#include "print_helpers.h"

int print_esel_header(const char *buf)
{
	const struct esel_header* bufhdr = (struct esel_header*)buf;

	print_bar();

	print_header("eSEL Header");

	print_line("ID", "%x", le16toh(bufhdr->id));
	print_line("Record Type", "0x%x", bufhdr->record_type);
	print_line("Timestamp", "%u", le32toh(bufhdr->timestamp));
	print_line("GENID", "0x%x", le16toh(bufhdr->genid));
	print_line("EvMRev", "0x%x", bufhdr->evmrev);
	print_line("Sensor Type", "0x%x", bufhdr->sensor_type);
	print_line("Sensor No.", "0x%x", bufhdr->sensor_num);
	print_line("Dir Type", "0x%x", bufhdr->dir_type);
	print_line("Signature", "0x%x", bufhdr->signature);

	return 0;
}
