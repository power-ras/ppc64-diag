/**
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef LP_DIAG_H
#define LP_DIAG_H

#include <servicelog-1/servicelog.h>

/* Definations for indicator operating mode */
#define GUIDING_LIGHT_MODE	0x01
#define LIGHT_PATH_MODE		0x02

/* Definations for RTAS indicator call token values */
#define IDENT_INDICATOR         9007
#define ATTN_INDICATOR          9006
#define INDICATOR_TYPE(x)       (((x) == IDENT_INDICATOR) ? "identification" \
							  : "attention")

/* Defination for type of indicator call */
#define DYNAMIC_INDICATOR       0xFFFFFFFF

/* Definations for indicator type */
#define TYPE_ALL		0
#define TYPE_RTAS               1
#define TYPE_SES                2
#define TYPE_OS			3
#define TYPE_EOL		4

/* Indicator state */
#define INDICATOR_SAME		-1
#define INDICATOR_ON		1
#define INDICATOR_OFF		0

/* Buffer size */
#define BUF_SIZE		4096
#define LP_ERROR_LOG_MAX	4096

/* Debug defination */
#define dbg(_f, _a...)		_dbg("%s(): "_f, __FUNCTION__, ##_a)

/* Log file size */
#define KILOBYTE		1024
#define LP_ERRD_LOGSZ		(KILOBYTE * KILOBYTE)

/* External variables */
extern uint32_t		operating_mode;
extern char		*program_name;
extern char		*lp_event_log_file;
extern int		lp_event_log_fd;
extern char		*lp_error_log_file;
extern int		lp_error_log_fd;

#define	DEV_LENGTH		64
#define	VPD_LENGTH		128
#define	LOCATION_LENGTH		128

/* device vpd data */
struct dev_vpd {
	char	dev[DEV_LENGTH];
	char	location[LOCATION_LENGTH];
	char	mtm[VPD_LENGTH];
	char	sn[VPD_LENGTH];
	char	pn[VPD_LENGTH];
	char	fru[VPD_LENGTH];
	char	ds[VPD_LENGTH];

	struct	dev_vpd *next;
};

/* Indicator list -
 *
 * Note :
 *	The order of the fields in loc_code is important!
 *	First three fields must be first, and in this order.
 *	These fields are used for RTAS-controlled indicators.
 *	Fields after this point can be reordered.
 */
struct loc_code {
	uint32_t	length;	/* includes null terminator (RTAS) */
	char		code[LOCATION_LENGTH];	/* location code */
	uint32_t	index;	/* RTAS index, if RTAS indicator */

	uint32_t	type;	/* one of TYPE_ defines */
	int		state;	/* identify indicator status */
	char		dev[DEV_LENGTH]; /* device name to interact with*/

	char		devname[DEV_LENGTH];	/* like sda, sdb etc */
	char		mtm[VPD_LENGTH];	/* Model */
	char		sn[VPD_LENGTH];		/* Serial Number */
	char		pn[VPD_LENGTH];		/* Part Number */
	char		fru[VPD_LENGTH];	/* FRU number */
	char		ds[VPD_LENGTH];		/* Display name */

	struct		loc_code *next;
};


/* files.c */
int init_files(void);
void close_files(void);
void _dbg(const char *, ...);
void log_msg(const char *, ...);
int indicator_log_write(const char *, ...);

/* indicator.c */
int check_operating_mode(void);
int get_indicator_list(int, struct loc_code **);
int get_indicator_state(int , struct loc_code *, int *);
int set_indicator_state(int , struct loc_code *, int);
void get_all_indicator_state(int , struct loc_code *);
void set_all_indicator_state(int , struct loc_code *, int);
int enable_check_log_indicator(void);
int disable_check_log_indicator(void);
void fill_indicators_vpd(struct loc_code *);
void free_indicator_list(struct loc_code *);

int device_supported(const char *, const char *);
int enclosure_supported(const char *);
int truncate_loc_code(char *);
struct loc_code *get_indicator_for_loc_code(struct loc_code *, const char *);
int get_loc_code_for_dev(const char *, char *, int);

/* servicelog.c */
int get_service_event(int, struct sl_event **);
int get_all_open_service_event(struct sl_event **);
int get_repair_event(int, struct sl_event **);
int print_service_event(struct sl_event *);

#endif  /* LP_DIAG_H */
