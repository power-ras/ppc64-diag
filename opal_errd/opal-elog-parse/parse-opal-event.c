#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include "libopalevents.h"
#include "print-opal-event.h"
#include "opal-event-data.h"
#include "parse-opal-event.h"
#include "parse_helpers.h"
#include "parse-esel-header.h"

int header_id_lookup(struct header_id *elog_hdr_id, int size, char *id) {
	int i;
	for (i = 0; i < size; i++)
		if (strncmp(id,elog_hdr_id[i].id,2) == 0)
			return i;
	return -1;
}

int parse_opal_event_log(char *buf, int buflen, struct opal_event_log_scn **r_log)
{
	struct header_id elog_hdr_id[] = {
				HEADER_ORDER
	};

	int rc;
	struct opal_v6_hdr hdr;
	struct opal_priv_hdr_scn *ph;
	int header_pos;
	struct header_id *hdr_data;
	char *start = buf;
	int nrsections = 0;
	int is_error = 0;
	int i;
	opal_event_log *log = NULL;
	int log_pos = 0;

	*r_log = NULL;

	if (parse_esel_header(buf)) {
		print_esel_header(buf);
		buf += sizeof(struct esel_header);
	}

	while (buflen) {
		rc = parse_section_header(&hdr, buf, buflen);
		if (rc < 0) {
			break;
		}

		header_pos = header_id_lookup(elog_hdr_id, HEADER_ORDER_MAX, hdr.id);
		if (header_pos == -1) {
				printf("Unknown section header: %c%c at %lu:\n",
						hdr.id[0], hdr.id[1], buf-start);
				printf("Length: %u (incl 8 byte header)\n", hdr.length);
				printf("Hex:\n");
				for (i = 8; i < hdr.length; i++) {
					printf("0x%02x ", *(buf+i));
					if (i % 16)
						printf("\n");
				}
				printf("Text (. = unprintable):\n");
				for (i = 8; i < hdr.length; i++) {
					printf("%c",
							(isgraph(*(buf+i)) | isspace(*(buf+i))) ?
							*(buf+i) : '.');
				}
		}

		hdr_data = &elog_hdr_id[header_pos];

		nrsections++;

		if (hdr_data->pos != 0 && hdr_data->pos != nrsections &&
				((hdr_data->req & HEADER_REQ) ||
				((hdr_data->req & HEADER_REQ_W_ERROR) && is_error))) {
				fprintf(stderr, "ERROR %s: Section number %d should be "
						"%s, instead is 0x%02x%02x (%c%c)\n",
						__func__, nrsections, hdr_data->id,
						hdr.id[0], hdr.id[1], hdr.id[0], hdr.id[1]);
			rc = -1;
			break;
		}

		if (hdr_data->max == 0) {
			fprintf(stderr, "ERROR %s: Section %s has already appeared the "
					"required times and should not be seen again\n", __func__,
					hdr_data->id);
		} else if (hdr_data->max > 0) {
			hdr_data->max--;
		}

		if (strncmp(hdr.id, "PH", 2) == 0) {
			if (parse_priv_hdr_scn(&ph, &hdr, buf, buflen) == 0) {
				log = create_opal_event_log(ph->scn_count);
				if (!log) {
					free(ph);
					fprintf(stderr, "ERROR %s: Could not allocate internal log buffer\n",
							__func__);
					return -ENOMEM;
				}
				add_opal_event_log_scn(log, "PH", ph, log_pos++);
			} else {
				/* We didn't parse the private header and therefore couldn't malloc
				 * the log array, must stop
				 */
				fprintf(stderr, "ERROR %s: Unable to parse private header section"
						" cannot continue\n", __func__);
				return -EINVAL;
			}
		} else if (strncmp(hdr.id, "UH", 2) == 0) {
			struct opal_usr_hdr_scn *usr;
			if (parse_usr_hdr_scn(&usr, &hdr, buf, buflen,
					      &is_error) == 0) {
				add_opal_event_log_scn(log, "UH", usr, log_pos++);
			}
		} else if (strncmp(hdr.id, "PS", 2) == 0) {
			struct opal_src_scn *src;
			if (parse_src_scn(&src, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "PS", src, log_pos++);
			}
		} else if (strncmp(hdr.id, "EH", 2) == 0) {
			struct opal_eh_scn *eh;
			if (parse_eh_scn(&eh, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "EH", eh, log_pos++);
			}
		} else if (strncmp(hdr.id, "MT", 2) == 0) {
			struct opal_mtms_scn *mtms;
			if (parse_mtms_scn(&mtms, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "MT", mtms, log_pos++);
			}
		} else if (strncmp(hdr.id, "SS", 2) == 0) {
			struct opal_src_scn *src;
			if (parse_src_scn(&src, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "SS", src, log_pos++);
			}
		} else if (strncmp(hdr.id, "DH", 2) == 0) {
			struct opal_dh_scn *dh;
			if (parse_dh_scn(&dh, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "DH", dh, log_pos++);
			}
		} else if (strncmp(hdr.id, "SW", 2) == 0) {
			struct opal_sw_scn *sw;
			if (parse_sw_scn(&sw, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "SW", sw, log_pos++);
			}
		} else if (strncmp(hdr.id, "LP", 2) == 0) {
			struct opal_lp_scn *lp;
			if (parse_lp_scn(&lp, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "LP", lp, log_pos++);
			}
		} else if (strncmp(hdr.id, "LR", 2) == 0) {
			struct opal_lr_scn *lr;
			if (parse_lr_scn(&lr, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "LR", lr, log_pos++);
			}
		} else if (strncmp(hdr.id, "HM", 2) == 0) {
			struct opal_hm_scn *hm;
			if (parse_hm_scn(&hm, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "HM", hm, log_pos++);
			}
		} else if (strncmp(hdr.id, "EP", 2) == 0) {
			struct opal_ep_scn *ep;
			if (parse_ep_scn(&ep, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "EP", ep, log_pos++);
			}
		} else if (strncmp(hdr.id, "IE", 2) == 0) {
			struct opal_ie_scn *ie;
			if (parse_ie_scn(&ie, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "IE", ie, log_pos++);
			}
		} else if (strncmp(hdr.id, "MI", 2) == 0) {
			struct opal_mi_scn *mi;
			if (parse_mi_scn(&mi, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "MI", mi, log_pos++);
			}
		} else if (strncmp(hdr.id, "CH", 2) == 0) {
			struct opal_ch_scn *ch;
			if (parse_ch_scn(&ch, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "CH", ch, log_pos++);
			}
		} else if (strncmp(hdr.id, "UD", 2) == 0) {
			struct opal_ud_scn *ud;
			if (parse_ud_scn(&ud, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "UD", ud, log_pos++);
			}
		} else if (strncmp(hdr.id, "EI", 2) == 0) {
			struct opal_ei_scn *ei;
			if (parse_ei_scn(&ei, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "EI", ei, log_pos++);
			}
		} else if (strncmp(hdr.id, "ED", 2) == 0) {
			struct opal_ed_scn *ed;
			if (parse_ed_scn(&ed, &hdr, buf, buflen) == 0) {
				add_opal_event_log_scn(log, "ED", ed, log_pos++);
			}
		}

		buf += hdr.length;

		if (nrsections == ph->scn_count)
			break;
	}
	if(log) {
		/* we could get here but have failed to parse sections of have an
	 	 * unexpectady truncated buffer pad log with NULLS
	 	 */
		char nulStr2[2] = {'\0','\0'};
		for(i = log_pos; i < ph->scn_count; i++) {
			add_opal_event_log_scn(log, nulStr2, NULL, i);
		}
		*r_log = log;
	}

	for (i = 0; i < HEADER_ORDER_MAX; i++) {
		if (((elog_hdr_id[i].req & HEADER_REQ) ||
					((elog_hdr_id[i].req & HEADER_REQ_W_ERROR) && is_error))
				&& elog_hdr_id[i].max != 0) {
			fprintf(stderr,"ERROR %s: Truncated error log, expected section %s"
					" not found\n", __func__, elog_hdr_id[i].id);
			rc = -EINVAL;
		}
	}

	return rc;
}

/* parse all required sections of the log */
int parse_opal_event(char *buf, int buflen)
{
	int rc;
	opal_event_log *log = NULL;

	rc = parse_opal_event_log(buf, buflen, &log);

	if (log) {
		print_opal_event_log(log);
		free_opal_event_log(log);
	}

	return rc;
}
