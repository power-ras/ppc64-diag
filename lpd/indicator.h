/**
 * Copyright (C) 2012 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef INDICATOR_H
#define INDICATOR_H

#include <limits.h>
#include <linux/types.h>
#include <stdint.h>

/* Definations for indicator operating mode */
#define LED_MODE_GUIDING_LIGHT	0x01
#define LED_MODE_LIGHT_PATH	0x02

/* Definations for indicator type */
#define LED_TYPE_IDENT		0x00
#define LED_TYPE_FAULT		0x01
#define LED_TYPE_ATTN		0x02

/* Indicator type description */
#define LED_DESC_IDENT		"identify"
#define LED_DESC_FAULT		"fault"
#define LED_DESC_ATTN		"attention"

/* Definations for indicator type */
#define TYPE_ALL		0
#define TYPE_RTAS               1
#define TYPE_SES                2
#define TYPE_OS			3
#define TYPE_EOL		4

/* Indicator state */
#define LED_STATE_SAME		-1
#define LED_STATE_ON		1
#define LED_STATE_OFF		0

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

#define	DEV_LENGTH		PATH_MAX
#define	VPD_LENGTH		128
#define	LOCATION_LENGTH		80

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
 *	Fields after this point can be reordered. The 'length'
 *	field must be big endian at all times irrespective of
 *	the native platform.
 */
struct loc_code {
	__be32		length;	/* includes null terminator (RTAS) */
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

/* Each platform provides a set of hooks for LED operation. */
struct platform {
	const char	*name;

	/* Check LED operating mode (Guiding Light/Light Path) */
	int	(*get_indicator_mode)(void);

	/* Get location code list for given led type */
	int	(*get_indicator_list)(int led_type,
				      struct loc_code **list);

	/* Get LED state of given led_type for given location code */
	int	(*get_indicator_state)(int led_type,
				       struct loc_code *loc, int *state);

	/* Update LED state of given led_type for given location code */
	int	(*set_indicator_state)(int led_type,
				       struct loc_code *loc, int new_value);
};

#define DECLARE_PLATFORM(name)\
	const struct platform name ##_platform

extern struct platform platform;
extern struct platform rtas_platform;

/* files.c */
extern void _dbg(const char *, ...);
extern void log_msg(const char *, ...);
extern int indicator_log_write(const char *, ...);
extern int init_files(void);
extern void close_files(void);

/* indicator.c */
extern const char *get_indicator_desc(int );
extern int get_indicator_type(const char *);
extern int is_enclosure_loc_code(struct loc_code *);
extern int truncate_loc_code(char *);
extern struct loc_code *get_indicator_for_loc_code(struct loc_code *,
						   const char *);
extern int probe_indicator(void);
extern int get_indicator_mode(void);
extern int get_indicator_list(int, struct loc_code **);
extern int get_indicator_state(int , struct loc_code *, int *);
extern int set_indicator_state(int , struct loc_code *, int);
extern void get_all_indicator_state(int , struct loc_code *);
extern void set_all_indicator_state(int , struct loc_code *, int);
extern int enable_check_log_indicator(void);
extern int disable_check_log_indicator(void);
extern void free_indicator_list(struct loc_code *);

#endif  /* INDICATOR_H */
