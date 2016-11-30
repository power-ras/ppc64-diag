/**
 * @file hotplug.c
 *
 * Copyright (C) 2013 IBM Corporation
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#include <librtas.h>
#include "rtas_errd.h"

#define DRMGR_PROGRAM           "/usr/sbin/drmgr"
#define DRMGR_PROGRAM_NOPATH    "drmgr"

void handle_hotplug_event(struct event *re)
{
        struct rtas_event_hdr *rtas_hdr = re->rtas_hdr;
        struct rtas_hotplug_scn *hotplug;
        pid_t child;
        int status;
        char drc_index[11];
	char count[4];
        char *drmgr_args[] = { DRMGR_PROGRAM_NOPATH, "-c", NULL, NULL, NULL,
                        NULL, NULL, "-d4", "-V", NULL};

        /* Retrieve Hotplug section */
        if (rtas_hdr->version >= 6) {
	        hotplug = rtas_get_hotplug_scn(re->rtas_event);

                /* Build drmgr argument list */
                dbg("Build drmgr command\n");

                switch (hotplug->type) {
                        case RTAS_HP_TYPE_PCI:
                                drmgr_args[2] = "pci";
                                drmgr_args[6] = "-n";
                                break;
			case RTAS_HP_TYPE_CPU:
				drmgr_args[2] = "cpu";
				break;
			case RTAS_HP_TYPE_MEMORY:
				drmgr_args[2] = "mem";
                                break;
			case RTAS_HP_TYPE_PHB:
				drmgr_args[2] = "phb";
                                break;
                        default:
                                dbg("Unknown or unsupported hotplug type %d\n",
					hotplug->type);
                                return;
                }

                switch (hotplug->action) {
                        case RTAS_HP_ACTION_ADD:
                                drmgr_args[3] = "-a";
                                break;
                        case RTAS_HP_ACTION_REMOVE:
                                drmgr_args[3] = "-r";
                                break;
                        default:
                                dbg("Unknown hotplug action %d\n", hotplug->action);
                                return;
                }

                switch (hotplug->identifier) {
                        case RTAS_HP_ID_DRC_INDEX:
                                drmgr_args[4] = "-s";
                                snprintf(drc_index, 11, "%#x", hotplug->u1.drc_index);
                                drmgr_args[5] = drc_index;
                                break;
			case RTAS_HP_ID_DRC_COUNT:
				drmgr_args[4] = "-q";
				snprintf(count, 4, "%u", hotplug->u1.count);
				drmgr_args[5] = count;
				break;
                        default:
                                dbg("Unknown or unsupported hotplug identifier %d\n",
					hotplug->identifier);
                                return;
                }

                dbg("run: %s %s %s %s %s %s %s\n", drmgr_args[0],
                        drmgr_args[1], drmgr_args[2], drmgr_args[3],
                        drmgr_args[4], drmgr_args[5], drmgr_args[6]);

#ifdef DEBUG
		if(no_drmgr)
			return;
#endif
                /* invoke drmgr */
                dbg("Invoke drmgr command\n");

                child = fork();
                if (child == -1) {
                        log_msg(NULL, "%s cannot be run to handle a hotplug event, %s",
                                DRMGR_PROGRAM, strerror(errno));
                        return;
                } else if (child == 0) {
                        /* child process */
                        execv(DRMGR_PROGRAM, drmgr_args);

                        /* shouldn't get here */
                        log_msg(NULL, "Couldn not exec %s in response to hotplug event, %s",
                                DRMGR_PROGRAM, strerror(errno));
                        exit(1);
                }

                child = waitpid(child, &status, 0);

                dbg("drmgr call exited with %d\n", WEXITSTATUS(status));
        }
}
