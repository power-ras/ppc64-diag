/**
 * @file prrn.c
 *
 * Copyright (C) IBM Corporation
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <librtas.h>
#include "rtas_errd.h"

struct pmap_struct {
	struct pmap_struct	*next;
	uint32_t 		phandle;
	uint32_t		drc_index;
	char			*name;
};

struct drconf_cell {
	uint64_t 	base_addr;
	uint32_t 	drc_index;
	uint32_t 	reserved;
	uint32_t 	aa_index;
	uint32_t 	flags;
};

static struct pmap_struct *plist;
static int prrn_log_fd = -1;

#define OFDT_BASE	"/proc/device-tree"
#define OFDTPATH	"/proc/ppc64/ofdt"
#define PRRN_HOTPLUG	"/etc/ppc64-diag/prrn_hotplug"
#define DRCONF_PATH	"/proc/device-tree/ibm,dynamic-reconfiguration-memory/ibm,dynamic-memory"

static int write_prrn_log(const char *buf, int len)
{
	if (prrn_log_fd)
		return write(prrn_log_fd, buf, len);

	return -1;
}

static void open_prrn_log(void)
{
	time_t now;
	int len;
	char buf[128];

	prrn_log_fd = open("/var/log/prrn_log", O_CREAT | O_WRONLY | O_TRUNC,
			   S_IRUSR | S_IWUSR);
	if (prrn_log_fd == -1) {
		dbg("Could not open PRRN log file");
		return;
	}

	now = time(0);
	len = sprintf(buf, "# PRRN Event Recieved: %s", ctime(&now));
	write_prrn_log(buf, len);
}

static void close_prrn_log(void)
{
	if (prrn_log_fd)
		close(prrn_log_fd);
}

uint32_t get_drc_index(const char *path)
{
	char drc_path[256];
	FILE *fp;
	uint32_t drc_index;
	int rc;

	sprintf(drc_path, "%s/%s", path, "ibm,my-drc-index");
	fp = fopen(drc_path, "r");
	if (!fp)
		return 0;

	rc = fread(&drc_index, sizeof(drc_index), 1, fp);
	if (rc != 1)
		drc_index = 0;

	fclose(fp);
	return drc_index;
}

/**
 * add_phandle_to_list
 *
 * @param name
 * @param phandle
 */
static void add_phandle_to_list(char *name, uint32_t phandle,
				uint32_t drc_index)
{
	struct pmap_struct *pm;

	pm = malloc(sizeof(struct pmap_struct));
	memset(pm, 0, sizeof(struct pmap_struct));

	pm->name = malloc(strlen(name) + 1);
	sprintf(pm->name, "%s", name);

	pm->phandle = phandle;
	pm->drc_index = drc_index;
	pm->next = plist;
	plist = pm;
}

/**
 * phandle_to_name
 *
 * @param ph
 * @returns
 */
static struct pmap_struct *phandle_to_pms(uint32_t phandle)
{
	struct pmap_struct *pms = plist;

	while (pms && pms->phandle != phandle)
		pms = pms->next;

	return pms;
}

/**
 * add_std_phandles
 *
 * @param parent
 * @param p
 * @returns
 */
static int add_std_phandles(char *parent, char *p)
{
	DIR *d;
	struct dirent *de;
	char path[PATH_MAX];
	uint32_t phandle;
	char *pend;
	FILE *fd;

	strcpy(path, parent);
	if (p) {
		strcat(path, "/");
		strcat(path, p);
	}
	pend = path + strlen(path);

	d = opendir(path);
	if (!d) {
		perror(path);
		return -1;
	}

	while ((de = readdir(d))) {
		if ((de->d_type == DT_DIR) &&
		    (strcmp(de->d_name, ".")) &&
		    (strcmp(de->d_name, "..")))
			add_std_phandles(path, de->d_name);
	}

	strcpy(pend, "/ibm,phandle");
	fd = fopen(path, "r");
	if (fd) {
		uint32_t drc_index;

		if (fread(&phandle, sizeof(phandle), 1, fd) != 1) {
			dbg("Error reading phandle data!");
			return -1;
		}
		*pend = '\0';

		drc_index = get_drc_index(path);
		add_phandle_to_list(path + strlen(OFDT_BASE), phandle,
				    drc_index);
		fclose(fd);
	}

	closedir(d);
	return 0;
}

/**
 * add_drconf_phandles
 *
 * @returns
 */
static int add_drconf_phandles()
{
	FILE *fd;
	int *membuf;
	int i, rc, entries;
	struct stat sbuf;
	struct drconf_cell *mem;

	/* For PRRN Events the LMBs in dynamic-reconfiguration-memory that
	 * will get updated have their drc-index reported instead of the
	 * phandle in the list of phandles to update reported by
	 * rtas_update_nodes(). So we need to build a list of the possible
	 * LMBs.
	 */
	rc = stat(DRCONF_PATH, &sbuf);
	if (rc)
		return 0;

	fd = fopen(DRCONF_PATH, "r");
	if (!fd) {
		/* This is fine, older systems don't have this property */
		return 0;
	}

	membuf = malloc(sbuf.st_size);
	if (!membuf)
		return -1;

	fread(membuf, sbuf.st_size, 1, fd);
	fclose(fd);

	entries = membuf[0];
	mem = (struct drconf_cell *)&membuf[1];

	for (i = 0; i < entries; i++) {
		/* See comment above about rtas reporting drc_indexes. */
		add_phandle_to_list("LMB", mem->drc_index, mem->drc_index);
		mem++; /* trust your compiler */
	}

	free(membuf);
	return 0;
}

/**
 * add_phandles
 *
 * @returns
 */
static int add_phandles()
{
	int rc;

	rc = add_std_phandles(OFDT_BASE, NULL);
	if (rc)
		return rc;

	return add_drconf_phandles();
}

/**
 * do_update
 *
 * @param cmd
 * @param len
 * @returns 0 on success, !0 otherwise
 */
static int do_update(char *cmd, int len)
{
	int rc;
	int i, fd;

	fd = open(OFDTPATH, O_WRONLY);
	if (fd <= 0) {
		dbg("Failed to open %s: %s", OFDTPATH, strerror(errno));
		rc = errno;
		return rc;
	}

	if ((rc = write(fd, cmd, len)) != len)
		dbg("Error writing to ofdt file! rc %d errno %d", rc, errno);

	close(fd);

	/* The reamining code only formats the cmd buffer to make it
	 * human readable when printed via dbg().
	 */
	for (i = 0; i < len; i++) {
		if (!isprint(cmd[i]))
			cmd[i] = '.';
		if (isspace(cmd[i]))
			cmd[i] = ' ';
	}
	cmd[len-1] = 0x00;

	dbg("<%s>", cmd);
	return rc;
}

/**
 * update_properties
 *
 * @param phandle
 * @returns 0 on success, !0 otherwise
 */
static int update_properties(uint32_t phandle)
{
	int rc;
	char cmd[1024];
	char *longcmd = NULL;
	char *newcmd;
	int cmdlen = 0;
	int proplen = 0;
	unsigned int wa[1024];
	unsigned int *op;
	unsigned int nprop;
	unsigned int vd;
	int lenpos = 0;
	char *pname;
	unsigned int i;
	int more = 0;
	struct pmap_struct *pms = phandle_to_pms(phandle);

	memset(wa, 0x00, 16);
	wa[0] = phandle;

	do {
		dbg("about to call rtas_update_properties.\nphandle: %8.8x,"
		    " node: %s\nwork area: %8.8x %8.8x %8.8x %8.8x",
		    phandle, pms->name, wa[0], wa[1], wa[2], wa[3]);

		rc = rtas_update_properties((char *)wa, 1);
		if (rc && rc != 1) {
			dbg("Error %d from rtas_update_properties()", rc);
			return 1;
		}

		dbg("successful rtas_update_properties (more %d)", rc);

		op = wa+4;
		nprop = *op++;

		/* Should just be on property to update, the affinity. Except
		 * for reconfig memory, that is a single property for
		 * all possible LMBs on the system.
		 */

		/* Layout of the workarea returned from rtas, see PAPR for
		 * detailed explanation:
		 * u32	phandle of node
		 * u32	state variable
		 * u64	reserved
		 * u32	number of updated properties
		 * 		null terminated property name
		 *		vd - u32 len of property
		 *		actual property
		 */
		for (i = 0; i < nprop; i++) {
			pname = (char *)op;
			op = (unsigned int *)(pname + strlen(pname) + 1);
			vd = *op++;

			switch (vd) {
			    case 0x00000000:
				dbg("%s - name only property %s", pms->name,
				    pname);
				break;

			    case 0x80000000:
				dbg("%s - delete property %s", pms->name,
				    pname);
				sprintf(cmd,"remove_property %u %s",
					phandle, pname);
				do_update(cmd, strlen(cmd));
				break;

			    default:
				if (vd & 0x80000000) {
					dbg("partial property!");
					/* twos compliment of length */
					vd = ~vd + 1;
					more = 1;
				} else {
					more = 0;
				}

				dbg("%s - updating property %s length %d",
				    pms->name, pname, vd);

				/* See if we have a partially completed
				 * command
				 */
				if (longcmd) {
					newcmd = malloc(cmdlen + vd);
					memcpy(newcmd, longcmd, cmdlen);
					free(longcmd);
					longcmd = newcmd;
				} else {
					longcmd = malloc(vd+128);
					/* Build the command with a length
					 * of six zeros
					 */
					lenpos = sprintf(longcmd,
							 "update_property %u "
							 "%s ", phandle,
							 pname);
					strcat(longcmd, "000000 ");
					cmdlen = strlen(longcmd);
				}

				memcpy(longcmd + cmdlen, op, vd);
				cmdlen += vd;
				proplen += vd;

				if (!more) {
				        /* Now update the length to its actual
					 * value  and do a hideous fixup of
					 * the new trailing null
					 */
					sprintf(longcmd+lenpos,"%06d",proplen);
					longcmd[lenpos+6] = ' ';

					do_update(longcmd, cmdlen);
					free(longcmd);
					longcmd = NULL;
					cmdlen = 0;
					proplen = 0;
				}

				op = (unsigned int *)(((char *)op) + vd);
			}
		}
	} while (rc == 1);

	return 0;
}

/**
 * do_node_update
 *
 * Not all nodes can be updated via a DLPAR operation, only those with
 * a valid drc-index. For nodes without a drc-index we just update the
 * prrn log file indicating that the node was updated in te device tree
 * but that we cannot do a DLPAR operation.

 * @param type
 * @param pms
 */
static void do_node_update(struct pmap_struct *pms, const char *type)
{
	char buf[128];
	int len;

	if (pms->drc_index == 0)
		len = sprintf(buf, "# %s %x - %s (%x)\n", type,
			      pms->drc_index, pms->name, pms->phandle);
	else
		len = sprintf(buf, "%s %x\n", type, pms->drc_index);

	write_prrn_log(buf, len);

	dbg("Updating property for %s (%08x)", pms->name, pms->phandle);
	update_properties(pms->phandle);
}

/**
 * update_nodes
 *
 * @param op
 * @param n
 */
static void update_nodes(unsigned int *op, unsigned int n)
{
	int i, len;
	uint32_t phandle;
	char buf[128];
	struct pmap_struct *pms;

	for (i = 0; i < n; i++) {
		phandle = *op++;
		dbg("Updating node with phandle %08x", phandle);

		pms = phandle_to_pms(phandle);
		if (!pms)
			continue;

		if (!strcmp(pms->name, "LMB")) {
			len = sprintf(buf, "mem %x\n", pms->drc_index);
			write_prrn_log(buf, len);
		} else if (!strncmp(pms->name, "/memory@", 8)) {
			do_node_update(pms, "mem");
		} else {
			do_node_update(pms, "cpu");
		}
	}
}

/**
 * devtree_update
 *
 */
static void devtree_update(uint scope)
{
	int rc;
	unsigned int wa[1024];
	unsigned int *op;

	dbg("Updating device_tree");
	if (add_phandles())
		return;

	/* First 16 bytes of work area must be initialized to zero */
	memset(wa, 0x00, 16);

	do {
		rc = rtas_update_nodes((char *)wa, -scope);
		if (rc && rc != 1) {
			dbg("Error %d from rtas_update_nodes()", rc);
			return;
		}

		op = wa+4;

		while (*op & 0xFF000000) {
			switch (*op & 0xFF000000) {
			    case 0x01000000:
				dbg("Received unsupported node deletion "
				    "request, trying to continue");
				break;

			    case 0x02000000:
				update_nodes(op+1, *op & 0x00FFFFFF);
				break;

			    case 0x03000000:
				dbg("Received unsupported node addition "
				    "request, trying to continue");
				break;

			    case 0x00000000:
				/* End */
				break;

			    default:
				dbg("Unknown update_nodes op %8.8x", *op);
			}
			op += 1 + (*op & 0x00FFFFFF);
		}
	} while (rc == 1);

	dbg("Finished devtree update");
}

void handle_prrn_event(struct event *re)
{
	pid_t pid;
	uint scope = re->rtas_hdr->ext_log_length;

	open_prrn_log();
	devtree_update(scope);
	close_prrn_log();

	/* Kick off script to do required hotplug add/remove */
	pid = fork();
	if (pid == -1) {
		dbg("Could not exec prrn hotplug script!");
		return;
	}

	if (pid == 0) { /* Child */
		dbg("Kicking off %s script", PRRN_HOTPLUG);
		execl(PRRN_HOTPLUG, "prrn_hotplug", NULL);
		/* Should not get here */
		dbg("Failed PRRN Hotplug exec: %s", strerror(errno));
		exit(0);
	}

	/* Nothing to do for the parent, just return */
}
