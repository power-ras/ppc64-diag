/**
 * @file config.h
 * @brief Header for ppc64-diag config file
 *
 * Copyright (C) 2004 IBM Corporation.
 */

#ifndef _CONFIG_H
#define _CONFIG_H

extern char *config_file; 

/**
 * struct ppc64_diag_config
 * @brief structure to store ppc64-diag configuration variables
 */
struct ppc64_diag_config {
	unsigned int		flags;
	int			min_processors;
	int			min_entitled_capacity;
	char			scanlog_dump_path[1024];
	char			platform_dump_path[1024];
	int			restart_policy;
	void			(*log_msg)(char *, ...);
};

#define RE_CFG_RECEIVED_SIGHUP	0x00000001
#define RE_CFG_RECFG_SAFE	0x00000002

extern struct ppc64_diag_config d_cfg;

/* config.c */
int diag_cfg(int, void (*log_msg)(char *, ...));

#endif /* _CONFIG_H */
