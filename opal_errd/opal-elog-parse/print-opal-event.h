#ifndef _H_OPAL_PRINT_EVENT
#define _H_OPAL_PRINT_EVENT

#include "libopalevents.h"
#include "opal-event-data.h"

int print_opal_v6_hdr(struct opal_v6_hdr hdr);

int print_usr_hdr_action(struct opal_usr_hdr_scn *usrhdr);

int get_field_desc(
		struct generic_desc *data,
		int length,
		int id,
		int default_id,
		int default_index);

int print_usr_hdr_event_data(struct opal_usr_hdr_scn *usrhdr);

int print_usr_hdr_subsystem_id(struct opal_usr_hdr_scn *usrhdr);

int print_src_refcode(struct opal_src_scn *src);

int print_mt_data(struct opal_mt_struct mt);

int print_mt_scn(struct opal_mtms_scn *mtms);

int print_fru_id_scn(struct opal_fru_id_sub_scn id);

int print_fru_pe_scn(struct opal_fru_pe_sub_scn pe);

int print_fru_mr_scn(struct opal_fru_mr_sub_scn mr);

int print_fru_scn(struct opal_fru_scn fru);

int print_opal_src_scn(struct opal_src_scn *src);

int print_opal_usr_hdr_scn(struct opal_usr_hdr_scn *usrhdr);

int print_opal_priv_hdr_scn(struct opal_priv_hdr_scn *privhdr);

int print_eh_scn(struct opal_eh_scn *eh);

int print_ch_scn(struct opal_ch_scn *ch);

int print_ud_scn(struct opal_ud_scn *ud);

int print_hm_scn(struct opal_hm_scn *hm);

int print_ep_scn(struct opal_ep_scn *ep);

int print_sw_scn(struct opal_sw_scn *sw);

int print_lp_scn(struct opal_lp_scn *lp);

#endif /* _H_OPAL_PRINT_EVENT */
