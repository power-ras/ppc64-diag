#ifndef _H_OPAL_PRINT_EVENT
#define _H_OPAL_PRINT_EVENT

#include "libopalevents.h"
#include "opal-event-data.h"
#include "opal-event-log.h"

int print_opal_event_log(opal_event_log *log);

int print_opal_v6_hdr(const struct opal_v6_hdr hdr);

int print_usr_hdr_action(const struct opal_usr_hdr_scn *usrhdr);

int get_field_desc(
		struct generic_desc *data,
		int length,
		int id,
		int default_id,
		int default_index);

int print_usr_hdr_event_data(const struct opal_usr_hdr_scn *usrhdr);

int print_usr_hdr_subsystem_id(const struct opal_usr_hdr_scn *usrhdr);

int print_src_refcode(const struct opal_src_scn *src);

int print_mt_data(const struct opal_mt_struct mt);

int print_mt_scn(const struct opal_mtms_scn *mtms);

int print_fru_id_scn(const struct opal_fru_id_sub_scn id);

int print_fru_pe_scn(const struct opal_fru_pe_sub_scn pe);

int print_fru_mr_scn(const struct opal_fru_mr_sub_scn mr);

int print_fru_scn(const struct opal_fru_scn fru);

int print_opal_src_scn(const struct opal_src_scn *src);

int print_opal_usr_hdr_scn(const struct opal_usr_hdr_scn *usrhdr);

int print_opal_priv_hdr_scn(const struct opal_priv_hdr_scn *privhdr);

int print_eh_scn(const struct opal_eh_scn *eh);

int print_ch_scn(const struct opal_ch_scn *ch);

int print_ud_scn(const struct opal_ud_scn *ud);

int print_hm_scn(const struct opal_hm_scn *hm);

int print_ep_scn(const struct opal_ep_scn *ep);

int print_sw_scn(const struct opal_sw_scn *sw);

int print_lp_scn(const struct opal_lp_scn *lp);

int print_lr_scn(const struct opal_lr_scn *lr);

int print_ie_scn(const struct opal_ie_scn *ie);

int print_mi_scn(const struct opal_mi_scn *mi);

int print_ei_scn(const struct opal_ei_scn *ei);

int print_ed_scn(const struct opal_ed_scn *ed);

int print_dh_scn(const struct opal_dh_scn *dh);

#endif /* _H_OPAL_PRINT_EVENT */
