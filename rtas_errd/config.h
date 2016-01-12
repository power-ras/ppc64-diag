/**
 * @file config.h
 * @brief Header for ppc64-diag config file
 *
 * Copyright (C) 2004 IBM Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
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
