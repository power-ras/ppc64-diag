#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "opal-ed-scn.h"
#include "parse_helpers.h"
#include "opal-event-data.h"
#include "print_helpers.h"

int parse_ed_scn(struct opal_ed_scn **r_ed,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_ed_scn *ed;
	struct opal_ed_scn *edbuf = (struct opal_ed_scn *)buf;

	if (check_buflen(buflen, sizeof(struct opal_ed_scn), __func__) < 0 ||
	    check_buflen(buflen, hdr->length, __func__) < 0 ||
	    check_buflen(hdr->length, sizeof(struct opal_ed_scn), __func__) < 0)
		return -EINVAL;
	*r_ed = malloc(hdr->length);
	if (!*r_ed)
		return -ENOMEM;
	ed = *r_ed;

	ed->v6hdr = *hdr;
	ed->creator_id = edbuf->creator_id;
	memcpy(ed->user_data, edbuf->user_data, hdr->length - 12);

	return 0;
}

int print_ed_scn(const struct opal_ed_scn *ed)
{
	print_header("Extended User Defined Data");
	print_opal_v6_hdr(ed->v6hdr);
	print_line("Created by", "%s", get_creator_name(ed->creator_id));
	print_hex(ed->user_data, ed->v6hdr.length - 12);
	print_bar();
	return 0;
}
