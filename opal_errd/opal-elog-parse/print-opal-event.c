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

int print_src_refcode(const struct opal_src_scn *src)
{
   char primary_refcode_display[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN+1];

   memcpy(primary_refcode_display, src->primary_refcode,
             OPAL_SRC_SCN_PRIMARY_REFCODE_LEN);
   primary_refcode_display[OPAL_SRC_SCN_PRIMARY_REFCODE_LEN] = '\0';

   print_line("Primary Reference Code", "%s", primary_refcode_display);
   print_line("Hex Words 2 - 5", "0x%08x 0x%08x 0x%08x 0x%08x",
             src->ext_refcode2, src->ext_refcode3,
             src->ext_refcode4, src->ext_refcode5);
   print_line("Hex Words 6 - 9", "0x%08x 0x%08x 0x%08x 0x%08x",
             src->ext_refcode6, src->ext_refcode7,
             src->ext_refcode8, src->ext_refcode9);
   return 0;
}

int print_fru_id_scn(const struct opal_fru_id_sub_scn id)
{
   if (id.hdr.flags & OPAL_FRU_ID_PART)
      print_line("Part Number", "%s", id.part);

   if (id.hdr.flags & OPAL_FRU_ID_PROC)
      print_line("Procedure Number", "%s", id.part);

   if (id.hdr.flags & OPAL_FRU_ID_CCIN)
      print_line("CCIN", "%c%c%c%c", id.ccin[0], id.ccin[1],
            id.ccin[2], id.ccin[3]);

   if (id.hdr.flags & OPAL_FRU_ID_SERIAL) {
      char tmp[OPAL_FRU_ID_SERIAL_MAX+1];
      memset(tmp, 0, OPAL_FRU_ID_SERIAL_MAX+1);
      memcpy(tmp, id.serial, OPAL_FRU_ID_SERIAL_MAX);
      print_line("Serial Number", "%s", tmp);
   }

   return 0;
}

int print_fru_pe_scn(const struct opal_fru_pe_sub_scn pe)
{
   print_mtms_struct(pe.mtms);
   print_line("PCE", "%s", pe.pce);
   return 0;
}

int print_fru_mr_scn(const struct opal_fru_mr_sub_scn mr)
{

   int total_mru = mr.hdr.flags & 0x0F;
   int i;
   for (i = 0; i < total_mru; i++) {
      print_line("MRU ID", "0x%x", mr.mru[i].id);

      print_line("Priority", "%s", get_fru_priority_desc(mr.mru[i].priority));
   }
   return 0;
}

int print_fru_scn(const struct opal_fru_scn fru)
{
	/* FIXME This printing was to roughly confirm the correctness
	 * of the parsing. Improve here.
	 */
	print_center(" ");
	if (fru.type & OPAL_FRU_ID_SUB)
		print_center(get_fru_component_desc(fru.id.hdr.flags & 0xF0));

	print_line("Priority", "%s", get_fru_priority_desc(fru.priority));
	if (fru.location_code[0] != '\0')
		print_line("Location Code","%s", fru.location_code);

	if (fru.type & OPAL_FRU_ID_SUB)
		print_fru_id_scn(fru.id);

	if(fru.type & OPAL_FRU_PE_SUB)
		print_fru_pe_scn(fru.pe);

	if(fru.type & OPAL_FRU_MR_SUB)
		print_fru_mr_scn(fru.mr);

	return 0;
}

int print_opal_src_scn(const struct opal_src_scn *src)
{
   if (src->v6hdr.id[0] == 'P')
      print_header("Primary System Reference Code");
   else
      print_header("Secondary System Reference Code");

   print_opal_v6_hdr(src->v6hdr);
   print_line("SRC Format", "0x%x", src->flags);
   print_line("SRC Version", "0x%x", src->version);
   print_line("Valid Word Count", "0x%x", src->wordcount);
   print_line("SRC Length", "%x", src->srclength);
   print_src_refcode(src);
   if(src->fru_count) {
      print_center(" ");
      print_center("Callout Section");
      print_center(" ");
      /* Hardcode this to look like FSP, not what what they want here... */
      print_line("Additional Sections", "Disabled");
      print_line("Callout Count", "%d", src->fru_count);
      int i;
      for (i = 0; i < src->fru_count; i++)
         print_fru_scn(src->fru[i]);
   }
   print_center(" ");
   return 0;
}
