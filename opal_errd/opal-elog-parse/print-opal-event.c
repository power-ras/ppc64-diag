#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "print_helpers.h"
#include "libopalevents.h"
#include "opal-event-data.h"
#include "opal-event-log.h"
#include "print-opal-event.h"

int print_opal_event_log(opal_event_log *log)
{
	if (!log)
		return -1;

	int i = 0;
	while(has_more_elements(log[i])) {
		if ((strncmp(log[i].id, "PH", 2) == 0)) {
			print_opal_priv_hdr_scn((struct opal_priv_hdr_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "UH", 2) == 0) {
			print_opal_usr_hdr_scn((struct opal_usr_hdr_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "PS", 2) == 0) {
			print_opal_src_scn((struct opal_src_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "EH", 2) == 0) {
			print_eh_scn((struct opal_eh_scn *) log[i].scn);
      } else if (strncmp(log[i].id, "MT", 2) == 0) {
			print_mtms_scn((struct opal_mtms_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "SS", 2) == 0) {
			print_opal_src_scn((struct opal_src_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "DH", 2) == 0) {
			print_dh_scn((struct opal_dh_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "SW", 2) == 0) {
			print_sw_scn((struct opal_sw_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "LP", 2) == 0) {
			print_lp_scn((struct opal_lp_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "LR", 2) == 0) {
			print_lr_scn((struct opal_lr_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "HM", 2) == 0) {
			print_hm_scn((struct opal_hm_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "EP", 2) == 0) {
			print_ep_scn((struct opal_ep_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "IE", 2) == 0) {
			print_ie_scn((struct opal_ie_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "MI", 2) == 0) {
			print_mi_scn((struct opal_mi_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "CH", 2) == 0) {
			print_ch_scn((struct opal_ch_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "UD", 2) == 0) {
			print_ud_scn((struct opal_ud_scn *) log[i].scn);
		} else if (strncmp(log[i].id, "ED", 2) == 0) {
			print_ed_scn((struct opal_ed_scn *) log[i].scn);
      } else {
			fprintf(stderr, "ERROR: %s malformed opal-event-log structure"
					"unknown log section type %c%c", __func__,
					log[i].id[0], log[i].id[1]);
			return -EINVAL;
		}
		i++;
	}
	return 0;
}
