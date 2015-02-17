#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opal-dh-scn.h"
#include "parse_helpers.h"
#include "opal-event-data.h"
#include "print_helpers.h"

int parse_dh_scn(struct opal_dh_scn **r_dh,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_dh_scn *dhbuf = (struct opal_dh_scn *)buf;
	struct opal_dh_scn *dh;

	if (check_buflen(buflen, sizeof(struct opal_dh_scn) - DH_DUMP_STR_MAX,
	    __func__) < 0)
		return -EINVAL;

	*r_dh = malloc(sizeof(struct opal_dh_scn));
	if(!*r_dh)
		return -ENOMEM;
	dh = *r_dh;

	dh->v6hdr = *hdr;
	dh->dump_id = be32toh(dhbuf->dump_id);
	dh->flags = dhbuf->flags;
	dh->length_dump_os = dhbuf->length_dump_os;
	dh->dump_size = be64toh(dhbuf->dump_size);
	if (dh->flags & DH_FLAG_DUMP_HEX) {
		if (check_buflen(buflen, sizeof(struct opal_dh_scn) + sizeof(uint32_t),
		    __func__) < 0) {
			free(dh);
			return -EINVAL;
		}
		dh->shared.dump_hex = be32toh(dh->shared.dump_hex);
	} else { /* therefore it is in ascii */
		if (check_buflen(buflen, sizeof(struct opal_dh_scn) + dh->length_dump_os,
		    __func__) < 0) {
			free(dh);
			return -EINVAL;
		}
		memcpy(dh->shared.dump_str, dhbuf->shared.dump_str, dh->length_dump_os);
	}
	return 0;
}

int print_dh_scn(const struct opal_dh_scn *dh)
{
	print_header("Dump Locator");
	print_opal_v6_hdr(dh->v6hdr);
	print_line("Dump Type", "%s", get_dh_type_desc(dh->v6hdr.subtype));
	print_line("Dump Identifier", "0x%08x", dh->dump_id);
	print_line("Dump Flags", "0x%02x", dh->flags);
	print_line("OS Dump Length", "0x%02x", dh->length_dump_os);
	print_line("Dump Size", "0x%016lx", dh->dump_size);
	if (dh->flags & DH_FLAG_DUMP_HEX)
		print_line("OS Assigned Dump ID", "0x%08x", dh->shared.dump_hex);
	else /* therefore ascii */
		print_line("OS Assigned Dump File", "%s", dh->shared.dump_str);

	print_bar();
	return 0;
}
