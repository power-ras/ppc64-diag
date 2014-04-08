#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "print_helpers.h"
#include "libopalevents.h"
#include "opal-event-data.h"

int print_opal_v6_hdr(struct opal_v6_hdr hdr) {
   print_line("Section Version", "%d (%c%c)", hdr.version,
               hdr.id[0], hdr.id[1]);
   print_line("Sub-section type", "0x%x", hdr.subtype);
   print_line("Section Length", "0x%x", hdr.length);
   print_line("Component ID", "%x", hdr.component_id);
   return 0;
}

int print_usr_hdr_action(struct opal_usr_hdr_scn *usrhdr)
{
   char *entry = "Action Flags";

   if (usrhdr->action & OPAL_UH_ACTION_REPORT_EXTERNALLY) {
      if (usrhdr->action & OPAL_UH_ACTION_HMC_ONLY)
         print_line(entry,"Report to HMC");
      else
         print_line(entry, "Report to Operating System");
      entry = "";
   }

   if (usrhdr->action & OPAL_UH_ACTION_SERVICE) {
      print_line(entry, "Service Action Required");
      if (usrhdr->action & OPAL_UH_ACTION_CALL_HOME)
         print_line("","HMC Call Home");
      if(usrhdr->action & OPAL_UH_ACTION_TERMINATION)
         print_line("","Termination Error");
      entry = "";
   }

   if (usrhdr->action & OPAL_UH_ACTION_ISO_INCOMPLETE) {
      print_line(entry, "Error isolation incomplete");
      print_line("","Further analysis required");
      entry = "";
   }

   if(entry[0] != '\0') {
      print_line(entry, "Unknown action flag (0x%08x)", usrhdr->action);
   }

   return 0;
}

int print_usr_hdr_event_data(struct opal_usr_hdr_scn *usrhdr)
{
	print_line("Event Scope", "%s", get_event_scope(usrhdr->event_data));

   print_line("Event Severity", "%s", get_severity_desc(usrhdr->event_severity));

	print_line("Event Type", "%s", get_event_desc(usrhdr->event_type));

   return 0;
}

int print_usr_hdr_subsystem_id(struct opal_usr_hdr_scn *usrhdr)
{
   print_header("User Header");
   print_opal_v6_hdr(usrhdr->v6hdr);
   print_line("Subsystem", "%s", get_subsystem_name(usrhdr->subsystem_id));

   return 0;
}

int print_src_refcode(struct opal_src_scn *src)
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

int print_mt_data(struct opal_mt_struct mt)
{
   char model[OPAL_SYS_MODEL_LEN+1];
   char serial_no[OPAL_SYS_SERIAL_LEN+1];

   memcpy(model, mt.model, OPAL_SYS_MODEL_LEN);
   model[OPAL_SYS_MODEL_LEN] = '\0';
   memcpy(serial_no, mt.serial_no, OPAL_SYS_SERIAL_LEN);
   model[OPAL_SYS_SERIAL_LEN] = '\0';

   print_line("Machine Type Model", "%s", model);
   print_line("Serial Number", "%s", serial_no);

   return 0;
}

int print_mt_scn(struct opal_mtms_scn *mtms)
{

   print_header("Machine Type/Model & Serial Number");
   print_opal_v6_hdr(mtms->v6hdr);
   print_mt_data(mtms->mt);
   return 0;
}

int print_fru_id_scn(struct opal_fru_id_sub_scn id)
{
   print_center(" ");
   print_center(get_fru_priority_desc(id.hdr.flags & 0xF0));
   if (id.hdr.flags & OPAL_FRU_ID_PART)
      print_line("Part Number", "%s", id.part);

   if (id.hdr.flags & OPAL_FRU_ID_PROC)
      print_line("Procedure ID", "%s", id.part);

   if (id.hdr.flags & OPAL_FRU_ID_CCIN)
      print_line("CCIN", "%c%c%c%c", id.ccin[0], id.ccin[1],
            id.ccin[2], id.ccin[3]);

   if (id.hdr.flags & OPAL_FRU_ID_SERIAL) {
      char tmp[OPAL_FRU_ID_SERIAL_MAX+1];
      memcpy(tmp, id.serial, OPAL_FRU_ID_SERIAL);
      tmp[OPAL_FRU_ID_SERIAL] = '\0';
      print_line("Serial Number", "%s", tmp);
   }

   return 0;
}

int print_fru_pe_scn(struct opal_fru_pe_sub_scn pe)
{
   print_mt_data(pe.mtms);
   print_line("PCE", "%s", pe.pce);
   return 0;
}

int print_fru_mr_scn(struct opal_fru_mr_sub_scn mr)
{

   int total_mru = mr.hdr.flags & 0x0F;
   int i;
   for (i = 0; i < total_mru; i++) {
      print_line("MRU ID", "0x%x", mr.mru[i].id);

      print_line("Priority", "%s", get_fru_priority_desc(mr.mru[i].priority));
   }
   return 0;
}

int print_fru_scn(struct opal_fru_scn fru)
{
   /* FIXME This printing was to roughly confirm the correctness
    * of the parsing. Improve here.
    */
   print_line("Priority", "%s", get_fru_priority_desc(fru.priority));
   print_line("Location Code","%s", fru.location_code);
   if (fru.type & OPAL_FRU_ID_SUB) {
      print_fru_id_scn(fru.id);
   }

   if(fru.type & OPAL_FRU_PE_SUB) {
      print_fru_pe_scn(fru.pe);
   }

   if(fru.type & OPAL_FRU_MR_SUB) {
      print_fru_mr_scn(fru.mr);
   }

   return 0;
}

int print_opal_src_scn(struct opal_src_scn *src)
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

int print_opal_usr_hdr_scn(struct opal_usr_hdr_scn *usrhdr)
{
   print_usr_hdr_subsystem_id(usrhdr);
   print_usr_hdr_event_data(usrhdr);
   print_usr_hdr_action(usrhdr);
   return 0;
}

int print_opal_priv_hdr_scn(struct opal_priv_hdr_scn *privhdr)
{
   print_header("Private Header");
   print_opal_v6_hdr(privhdr->v6hdr);
   print_line("Created at", "%4u-%02u-%02u | %02u:%02u:%02u",
             privhdr->create_datetime.year,
             privhdr->create_datetime.month,
             privhdr->create_datetime.day,
             privhdr->create_datetime.hour,
             privhdr->create_datetime.minutes,
             privhdr->create_datetime.seconds);
   print_line("Committed at", "%4u-%02u-%02u | %02u:%02u:%02u",
             privhdr->commit_datetime.year,
             privhdr->commit_datetime.month,
             privhdr->commit_datetime.day,
             privhdr->commit_datetime.hour,
             privhdr->commit_datetime.minutes,
             privhdr->commit_datetime.seconds);
   print_line("Created by", "%s", get_creator_name(privhdr->creator_id));
   print_line("Creator Sub Id", "0x%x (%u), 0x%x (%u)",
         privhdr->creator_subid_hi,
         privhdr->creator_subid_hi,
         privhdr->creator_subid_lo,
         privhdr->creator_subid_lo);
   print_line("Platform Log Id", "0x%x", privhdr->plid);
   print_line("Entry ID", "0x%x", privhdr->log_entry_id);
   print_line("Section Count","%u",privhdr->scn_count);
   return 0;
}

int print_eh_scn(struct opal_eh_scn *eh)
{
   char model[OPAL_SYS_MODEL_LEN+1];
   char serial_no[OPAL_SYS_SERIAL_LEN+1];

   memcpy(model, eh->mt.model, OPAL_SYS_MODEL_LEN);
   model[OPAL_SYS_MODEL_LEN] = '\0';
   memcpy(serial_no, eh->mt.serial_no, OPAL_SYS_SERIAL_LEN);
   model[OPAL_SYS_SERIAL_LEN] = '\0';

   print_header("Extended User Header");
   print_line("Version", "%d (%c%c)", eh->v6hdr.version,
             eh->v6hdr.id[0], eh->v6hdr.id[1]);
   print_line("Sub-section type", "%d", eh->v6hdr.subtype);
   print_line("Component ID", "%x", eh->v6hdr.component_id);
   print_line("Reporting Machine Type", "%s", model);
   print_line("Reporting Serial Number", "%s", serial_no);
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

int print_ch_scn(struct opal_ch_scn *ch)
{
   print_header("Call Home Log Comment");
   print_opal_v6_hdr(ch->v6hdr);
   print_line("Call Home Comment", "%s", ch->comment);

   return 0;
}

int print_ud_scn(struct opal_ud_scn *ud)
{
   print_header("User Defined Data");
   print_opal_v6_hdr(ud->v6hdr);
   int i = 0;
   int written = 0;
   /*FIXME this data should be parsable if documentation appears/exists
    * In the mean time, just dump it in hex
    */
   print_line("User data hex","length %d",ud->v6hdr.length - 8);
   while(i < (ud->v6hdr.length - 8)) {
      if(written % LINE_LENGTH == 0)
         written = printf("| ");

         written += printf("%.2x",ud->data[i]);

      if((written + 2) % LINE_LENGTH == 0) {
         written += printf("|\n");
      } else {
         putchar(' ');
         written++;
      }

      i++;
   }
   while ((written + 2) % LINE_LENGTH) {
      putchar(' ');
      written++;
   }
   printf("|\n");
   return 0;
}

int print_hm_scn(struct opal_hm_scn *hm)
{
	print_header("HMC ID");

	print_opal_v6_hdr(hm->v6hdr);
	print_mt_data(hm->mt);

	return 0;
}
