/**
 * @file rtas_errd.h
 * @brief Main header for rtas_errd
 *
 * Copyright (C) 2004 IBM Corporation.
 */

#ifndef _RTAS_ERRD_H
#define _RTAS_ERRD_H

#include <signal.h>
#include <librtasevent.h>
#include <servicelog-1/servicelog.h>
#include "fru_prev6.h"
#include "config.h"

extern char *platform_log;
extern char *messages_log;
extern char *proc_error_log1;
extern char *proc_error_log2;
extern char *rtas_errd_log;
extern char *rtas_errd_log0;
extern char *test_file;

#ifdef DEBUG
extern char *scenario_file;
extern int testing_finished;
extern int no_drmgr;
/**
 * @def RTAS_ERRD_ARGS 
 * @brief DEBUG args for rtas_errd
 */
#define RTAS_ERRD_ARGS		"c:de:f:l:m:p:s:"
#else
/**
 * @def RTAS_ERRD_ARGS
 * @brief standard args for rtas_errd
 */
#define RTAS_ERRD_ARGS		"d"
#endif

extern int platform_log_fd;
extern int debug;
extern char *scanlog;

extern struct servicelog *slog;

#define RTAS_ERROR_LOG_MAX 	4096
#define ADDL_TEXT_MAX		256

#ifndef MAX
#define MAX(x,y)	((x) > (y) ? (x) : (y))
#endif

#define TRUE	1
#define FALSE	0

/* Structure containing error log entries for a device. */
struct errdata  {
	unsigned 	sequence;	/* sequence # of entry */
	unsigned 	time_stamp;	/* entry timestamp */
	unsigned 	err_id;    	/* error id code */
};

struct diag_vpd {
	char *ds;  /* displayable message */
	char *yl;  /* location code */
	char *fn;  /* fru part number */
	char *sn;  /* fru serial number */
	char *se;  /* enclosure serial number */
	char *tm;  /* enclosure model number */
};

/**
 * @struct event
 * @brief struct to track and handle RTAS events in rtas_errd.
 *
 * NOTE: The first two items in this struct (seq_num and event_buf)
 * must remain as the first two items and in the same order.  Otherwise
 * reading events from /proc will break.  The read from /proc returns
 * the event number followed by the actual event itself.
 */
struct event {
	int			seq_num;   /**< Number assigned by kernel */
	char			event_buf[RTAS_ERROR_LOG_MAX];
	int			length;    /**< RTAS event length (bytes) */
	struct rtas_event_hdr	*rtas_hdr; /**< RTAS event header */
					/**< data read in from proc_erro log */
	unsigned int		flags;	/**< rtas_Event flags */
	char			*loc_codes;
	char			addl_text[ADDL_TEXT_MAX];
	struct errdata		errdata;
	struct diag_vpd		diag_vpd;
	struct rtas_event	*rtas_event;
	struct sl_event		*sl_entry;
};

/* flags for struct event */
#define RE_SCANLOG_AVAIL	0x00000001
#define RE_SERVICEABLE		0x00000002
#define RE_PLATDUMP_AVAIL	0x00000004
#define RE_PREDICTIVE		0x00000008

#define RE_HMC_TAGGED		0x40000000
#define RE_ALREADY_REPORTED	0x20000000
#define RE_RECOVERED_ERROR	0x10000000

extern char *epow_status_file;

/* files.c */
#define dbg(_f, _a...)	_dbg("%s(): "_f, __FUNCTION__, ##_a)
void log_msg(struct event *, char *, ...);
void cfg_log(char *, ...);
int init_files(void);
void close_files(void);
void _dbg(const char *, ...);
int print_rtas_event(struct event *);
int platform_log_write(char *, ...);
void update_epow_status_file(int);
int read_proc_error_log(char *, int);

/* dump.c */
void check_scanlog_dump(void);
void check_platform_dump(struct event *);

/* eeh.c */
void check_eeh(struct event *);

/* guard.c */
void handle_resource_dealloc(struct event *);

/* rtas_errd.c */
int handle_rtas_event(struct event *);

/* update.c */
void update_rtas_msgs(void);

/* ela.c */
int process_pre_v6(struct event *);
int get_error_fmt(struct event *);

/* v6ela.c */
int process_v6(struct event *);

/* diag_support.c */
char *get_dt_status(char *);
char *diag_get_fru_pn(struct event *, char *);
void free_diag_vpd(struct event *);

/* menugoal.c */
int menugoal(struct event *, char *);

/* epow.c */
void epow_timer_handler(int, siginfo_t, void *);
void check_epow(struct event *);

/* servicelog.c */
time_t get_event_date(struct event *event);
int servicelog_sev(int rtas_sev);
void add_callout(struct event *event, char pri, int type, char *proc,
		 char *loc, char *pn, char *sn, char *ccin);
void log_event(struct event *);

/* signal.c */
void sighup_handler(int, siginfo_t, void *);
void restore_sigchld_default(void);
void setup_sigchld_handler(void);

/* prrn.c */
void handle_prrn_event(struct event *);

#endif /* _RTAS_ERRD_H */
