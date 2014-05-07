#ifndef _H_OPAL_PARSE_EVENT
#define _H_OPAL_PARSE_EVENT

#include "opal-event-log.h"
#include "libopalevents.h"

int parse_opal_event_log(char *buf, int buflen, struct opal_event_log_scn **log);

int parse_opal_event(char *buf, int buflen);

__attribute__ ((unused))
static struct opal_priv_hdr_scn *get_priv_hdr_scn(opal_event_log *log) {
	return (struct opal_priv_hdr_scn *) get_opal_event_log_scn(log, "PH", 0);
}

__attribute__ ((unused))
static struct opal_usr_hdr_scn *get_usr_hdr_scn(opal_event_log *log) {
	return (struct opal_usr_hdr_scn *) get_opal_event_log_scn(log, "UH", 0);
}

__attribute__ ((unused))
static struct opal_src_scn *get_src_ps_scn(opal_event_log *log) {
	return (struct opal_src_scn *) get_opal_event_log_scn(log, "PS", 0);
}

__attribute__ ((unused))
static struct opal_eh_scn *get_eh_scn(opal_event_log *log) {
	return (struct opal_eh_scn *) get_opal_event_log_scn(log, "EH", 0);
}

__attribute__ ((unused))
static struct opal_mtms_scn *get_mtms_scn(opal_event_log *log) {
	return (struct opal_mtms_scn *) get_opal_event_log_scn(log, "MT", 0);
}

__attribute__ ((unused))
static struct opal_src_scn *get_src_ss_scn(opal_event_log *log, int n) {
	return (struct opal_src_scn *) get_opal_event_log_scn(log, "SS", n);
}

__attribute__ ((unused))
static struct opal_dh_scn *get_dh_scn(opal_event_log *log) {
	return (struct opal_dh_scn *) get_opal_event_log_scn(log, "DH", 0);
}

__attribute__ ((unused))
static struct opal_sw_scn *get_sw_scn(opal_event_log *log, int n) {
	return (struct opal_sw_scn *) get_opal_event_log_scn(log, "SW", n);
}

__attribute__ ((unused))
static struct opal_lp_scn *get_lp_scn(opal_event_log *log) {
	return (struct opal_lp_scn *) get_opal_event_log_scn(log, "LP", 0);
}

__attribute__ ((unused))
static struct opal_lr_scn *get_lr_scn(opal_event_log *log) {
	return (struct opal_lr_scn *) get_opal_event_log_scn(log, "LR", 0);
}

__attribute__ ((unused))
static struct opal_ep_scn *get_ep_scn(opal_event_log *log) {
	return (struct opal_ep_scn *) get_opal_event_log_scn(log, "EP", 0);
}

__attribute__ ((unused))
static struct opal_ie_scn *get_ie_scn(opal_event_log *log) {
	return (struct opal_ie_scn *) get_opal_event_log_scn(log, "IE", 0);
}

__attribute__ ((unused))
static struct opal_mi_scn *get_mi_scn(opal_event_log *log) {
	return (struct opal_mi_scn *) get_opal_event_log_scn(log, "MI", 0);
}

__attribute__ ((unused))
static struct opal_ch_scn *get_ch_scn(opal_event_log *log) {
	return (struct opal_ch_scn *) get_opal_event_log_scn(log, "CH", 0);
}

__attribute__ ((unused))
static struct opal_ud_scn *get_ud_scn(opal_event_log *log, int n) {
	return (struct opal_ud_scn *) get_opal_event_log_scn(log, "UD", n);
}

__attribute__ ((unused))
static struct opal_ei_scn *get_ei_scn(opal_event_log *log) {
	return (struct opal_ei_scn *) get_opal_event_log_scn(log, "EI", 0);
}

__attribute__ ((unused))
static struct opal_ed_scn *get_ed_scn(opal_event_log *log, int n) {
	return (struct opal_ed_scn *) get_opal_event_log_scn(log, "ED", n);
}

#endif /* _H_OPAL_PARSE_EVENT */
