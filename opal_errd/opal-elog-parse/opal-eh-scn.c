#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "opal-eh-scn.h"
#include "opal-mtms-struct.h"
#include "print_helpers.h"

int parse_eh_scn(struct opal_eh_scn **r_eh,
                 const struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_eh_scn *eh;
	struct opal_eh_scn *bufeh = (struct opal_eh_scn*)buf;

	if (hdr->length < sizeof(*eh)) {
		fprintf(stderr, "%s: corruped input data, expected length >= %lu, "
				"got %u", __func__, sizeof(*eh), hdr->length);
		return -EINVAL;
	}

	eh = malloc(hdr->length);
	if (!eh)
		return -ENOMEM;

	if (buflen < sizeof(struct opal_eh_scn)) {
		fprintf(stderr, "%s: corrupted input buffer, expected length >= %lu, "
				"got %u\n", __func__,  sizeof(struct opal_eh_scn), buflen);
		free(eh);
		return -EINVAL;
	}

	eh->v6hdr = *hdr;
	copy_mtms_struct(&(eh->mtms), &(bufeh->mtms));

	strncpy(eh->opal_release_version, bufeh->opal_release_version, OPAL_VER_LEN);
	strncpy(eh->opal_subsys_version, bufeh->opal_subsys_version, OPAL_VER_LEN);

	eh->event_ref_datetime = parse_opal_datetime(bufeh->event_ref_datetime);

	eh->opal_symid_len = bufeh->opal_symid_len;
	/* Best to have strlen walk random memory rather than overflow eh->opalsymid */
	if (eh->opal_symid_len && hdr->length < sizeof(struct opal_eh_scn) + strlen(bufeh->opalsymid)) {
		fprintf(stderr, "%s: corrupted EH section, opalsymid is larger than header"
		        " specified length %lu > %u", __func__,
		        sizeof(struct opal_eh_scn) + strlen(bufeh->opalsymid), hdr->length);
		free(eh);
		return -EINVAL;
	}
	strncpy(eh->opalsymid, bufeh->opalsymid, eh->opal_symid_len);

	*r_eh = eh;
	return 0;
}

int print_eh_scn(const struct opal_eh_scn *eh)
{
	print_header("Extended User Header");
	print_opal_v6_hdr(eh->v6hdr);
	print_mtms_struct(eh->mtms);
	print_line("FW Released Ver", "%s",
	          eh->opal_release_version);
	print_line("FW SubSys Version", "%s",
	          eh->opal_subsys_version);
	print_line("Common Ref Time (UTC)",
	           "%4u-%02u-%02u | %02u:%02u:%02u",
	           eh->event_ref_datetime.year,
	           eh->event_ref_datetime.month,
	           eh->event_ref_datetime.day,
	           eh->event_ref_datetime.hour,
	           eh->event_ref_datetime.minutes,
	           eh->event_ref_datetime.seconds);
	print_line("Symptom Id Len", "%d", eh->opal_symid_len);
	if (eh->opal_symid_len)
		print_line("Symptom Id", "%s", eh->opalsymid);

	return 0;
}
