/**
 * @file extract_platdump.c
 * @brief Command to extract a platform dump and copy it to the filesystem
 *
 * Copyright (C) 2007 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include <librtas.h>
#include <sys/stat.h>
#include "rtas_errd.h"

#define DUMP_HDR_PREFIX_OFFSET	0x16	/* prefix size in dump header */
#define DUMP_HDR_FNAME_OFFSET	0x18	/* suggested filename in dump header */
#define DUMP_MAX_FNAME_LEN	40
#define TOKEN_PLATDUMP_MAXSIZE	32
#define DUMP_BUF_SZ		4096

int flag_v = 0;

static struct option long_options[] = {
	{"help",		no_argument,		NULL, 'h'},
	{"verbose",		no_argument,		NULL, 'v'},
	{0,0,0,0}
};

/**
 * msg
 * @brief Print message if verbose flag is set
 *
 * @param fmt format string a la  printf()
 * @param ... additional args a la printf()
 */
void
msg(char *fmt, ...)
{
	va_list ap;

	if (flag_v) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fprintf(stderr, "\n");
	}

	return;
}

/**
 * print_usage
 * @brief Print the usage message for this command
 *
 * @param name the name of this executable
 */ 
static void
print_usage(const char *name) {
	printf("Usage: %s [-h] [-v] <dump_tag>\n"
		"\t-h: print this help message\n"
		"\t-v: verbose output\n"
		"\t<dump_tag>: the tag of the dump(s) to extract, in hex\n",
		name);
	return;
}

/**
 * handle_platform_dump_error
 * @brief Interpret librtas return codes
 * 
 * Interpret a return code from librtas for any librtas specific messages.
 * 
 * @param e return code from librtas
 * @param err_buf buffer to write librtas message to
 * @param sz size of the err_buf parameter
 */
static void 
handle_platform_dump_error(int e, char *err_buf, int sz) 
{
	char *err = "Library error during ibm,platform-dump call: ";

	switch (e) {
	    case RTAS_KERNEL_INT: /* no kernel interface to firmware */
		snprintf(err_buf, sz, "%s%s", err,
			 "No kernel interface to firmware");
		break;

	    case RTAS_KERNEL_IMP: /* no kernel implementation of function */
		snprintf(err_buf, sz, "%s%s", err,
			 "No kernel implementation of function");
		break;

	    case RTAS_PERM: 	 /* non-root caller */
		snprintf(err_buf, sz, "%s%s", err,
			 "Permission denied (non-root caller)");
		break;

	    case RTAS_NO_MEM:	 /* out of heap memory */
		snprintf(err_buf, sz, "%s%s", err, "Out of heap memory");
		break;

	    case RTAS_NO_LOWMEM: /* kernel out of low memory */
		snprintf(err_buf, sz, "%s%s", err, "Kernel out of low memory");
		break;

	    case RTAS_FREE_ERR:	 /* attempt to free nonexistant rmo buffer */
		snprintf(err_buf, sz, "%s%s", err,
			 "Attempt to free nonexistant rmo buffer");
		break;

	    case RTAS_IO_ASSERT:  /* unexpected I/O error */
		snprintf(err_buf, sz, "%s%s", err, "Unexpected I/O error");
		break;

 	    case RTAS_UNKNOWN_OP: /* no firmware implementation of function */
		snprintf(err_buf, sz, "%s%s", err,
			 "No firmware implementation of function");
		break;

	    case -1:		  /* hardware error */
		snprintf(err_buf, sz, "%s%s", err,
			 "RTAS call returned a hardware error");
		break;

	    case -9002:	/* platform has decided to deliver dump elsewhere */
		snprintf(err_buf, sz, "%s%s", err, "Not authorized (platform "
			 "will deliver dump elsewhere)");
		break;

	    default:
		snprintf(err_buf, sz, "%sUnknown error (%d)", err, e);
		break;
	}
}
/**
 * remove_old_dumpfiles
 * @brief if needed, remove any old dumpfiles
 *
 * Users can specify the number of old dumpfiles they wish to save
 * via the config file PlatformDumpMax entry.  This routine will search
 * through and removeany dump files of the specified type if the count 
 * exceeds the maximum value.
 *
 * @param dumpname suggested filename of the platform dump
 * @param prefix_size length of the prefix
 */
void
remove_old_dumpfiles(char *dumpname, int prefix_size)
{
	struct dirent	*entry;
	DIR		*dir;
	char		pathname[DUMP_MAX_FNAME_LEN + 40];
	
	dir = opendir(d_cfg.platform_dump_path);
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, "..") == 0)
			continue;
		if (strcmp(entry->d_name, ".") == 0)
			continue;

		if (strncmp(dumpname, entry->d_name, prefix_size) == 0) {
			sprintf(pathname, "%s%s", d_cfg.platform_dump_path, 
				entry->d_name);

			msg("Deleting file %s", pathname);
			if (unlink(pathname) < 0) {
				msg("Could not delete file \"%s\" "
					"to make room for incoming platform "
					"dump \"%s\" (%s).  The new dump will "
					"be saved anyways.", pathname,
					dumpname, strerror(errno));
			}
		}
	}

	closedir(dir);
}

/**
 * extract_platform_dump
 * @brief Extract a platform dump with a given tag to the filesystem
 *
 * @param dump_tag tag of the platform dump to extract
 * @return 0 on success, 1 on failure
 */
int
extract_platform_dump(uint64_t dump_tag)
{
	uint64_t seq=0, seq_next, bytes;
	uint16_t prefix_size = 7;
	char	*dump_buf = NULL;
	char	filename[DUMP_MAX_FNAME_LEN + 1];
	char	pathname[DUMP_MAX_FNAME_LEN + 40];
	char	dump_err[RTAS_ERROR_LOG_MAX];
	char	dumpid[5];
	int	out=-1, rc, librtas_rc, dump_complete=0, ret=0;

	msg("Dump tag: 0x%016LX", dump_tag);

	dump_buf = malloc(DUMP_BUF_SZ);
	if (dump_buf == NULL) {
		msg("Could not allocate buffer to retrieve dump: %s",
			strerror(errno));
		ret = 1;
		goto platdump_error_out;
	}

	msg("Calling rtas_platform_dump, seq 0x%016LX", seq);
	librtas_rc = rtas_platform_dump(dump_tag, 0, dump_buf, DUMP_BUF_SZ, 
					&seq_next, &bytes);
	if (librtas_rc == 0) {
		dump_complete = 1;
	}
	if (librtas_rc < 0) {
		handle_platform_dump_error(librtas_rc, dump_err, 1024);
		msg("%s\nThe platform dump header could not be "
			"retrieved from firmware", dump_err);
		ret = 1;
		goto platdump_error_out;
	}

	seq = seq_next;

	/* 
	 * Retreive the prefix size and suggested filename for the dump
	 * from the dump header 
	 */
	if (bytes >= DUMP_HDR_PREFIX_OFFSET + sizeof(uint16_t)) {
		prefix_size = *(uint16_t *)(dump_buf + DUMP_HDR_PREFIX_OFFSET);
	}

	if (bytes >= DUMP_HDR_FNAME_OFFSET + DUMP_MAX_FNAME_LEN) {
		strncpy(filename, dump_buf + DUMP_HDR_FNAME_OFFSET,
			DUMP_MAX_FNAME_LEN);
	} 
	else {
		strcpy(filename, "dumpid.");
		strcat(filename, dumpid);
	}

	filename[DUMP_MAX_FNAME_LEN - 1] = '\0';
	msg("Suggested filename: %s, prefix size: %d", filename, prefix_size);

	/* Create the platform dump directory if necessary */
	if (mkdir(d_cfg.platform_dump_path, 
		  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0) {
		if (errno != EEXIST) {
			msg("Could not create %s: %s.\nThe platform dump "
				"could not be copied to the platform dump "
				"directory", d_cfg.platform_dump_path,
				strerror(errno));
			ret = 1;
			goto platdump_error_out;
		}
	}

	/*
	 * Before writing the new dump out, we need to see if any older
	 * dumps need to be removed first
	 */
	remove_old_dumpfiles(filename, prefix_size);

	/* Copy the dump off to the filesystem */
	pathname[0] = '\0';
	strcpy(pathname, d_cfg.platform_dump_path);
	strcat(pathname, filename);
	msg("Dump path/filename: %s", pathname);
	out = creat(pathname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (out <= 0) {
		msg("Could not open %s for writing: %s.\nThe platform dump "
			"could not be retrieved", pathname, strerror(errno));
		ret = 1;
		goto platdump_error_out;
	}

	rc = write(out, dump_buf, (size_t)bytes);
	if (rc < 0) {
		msg("Could not write to %s: %s.\nThe platform dump "
			"could not be retrieved", pathname, strerror(errno));
		ret = 1;
		goto platdump_error_out;
	}

	while (dump_complete == 0) {
		msg("Calling rtas_platform_dump, seq 0x%016LX", seq);
		librtas_rc = rtas_platform_dump(dump_tag, seq, dump_buf,
						DUMP_BUF_SZ, &seq_next, &bytes);
		if (librtas_rc < 0) {
			handle_platform_dump_error(librtas_rc, dump_err, 1024);
			msg("%s\nThe platform dump could not be "
				"retrieved from firmware", dump_err);
			ret = 1;
			goto platdump_error_out;
		}
		if (librtas_rc == 0) {
			dump_complete = 1;
		}

		seq = seq_next;

		rc = write(out, dump_buf, (size_t)bytes);
		if (rc < 0) {
			msg("Could not write to %s: %s\nThe platform "
				"dump could not be retrieved", pathname,
				strerror(errno));
			ret = 1;
			goto platdump_error_out;
		}
	}

	/* 
	 * Got the dump; signal the platform that it is okay for
	 * them to delete/invalidate their copy 
	 */
	msg("Sigalling that the dump has been retrieved, seq 0x%016LX", seq);
	librtas_rc = rtas_platform_dump(dump_tag, seq, NULL, 0, 
					&seq_next, &bytes);
	if (librtas_rc < 0) {
		handle_platform_dump_error(librtas_rc, dump_err, 1024);
		msg("%s\nThe platform could not be notified to "
			"delete its copy of a platform dump", dump_err);
	}

	/* rtas_errd depends on this line being printed */
	printf("%s\n", filename);

platdump_error_out:
	if (out != -1)
		close(out);
	if (dump_buf != NULL)
		free(dump_buf);

	return ret;
}

int
main(int argc, char *argv[])
{
	int option_index, rc, fail=0;
	uint64_t dump_tag;

	for (;;) {
		option_index = 0;
		rc = getopt_long(argc, argv, "hv", long_options,
				&option_index);

		if (rc == -1)
			break;

		switch (rc) {
		case 'h':
			print_usage(argv[0]);
			return 0;
		case 'v':
			flag_v = 1;
			break;
		case '?':
			print_usage(argv[0]);
			return -1;
			break;
		default:
			printf("huh?\n");
			break;
		}
	}

	if (optind < argc) {
		/* Parse ppc64-diag config file */
		rc = diag_cfg(0, &msg);
		if (rc) {
			fprintf(stderr, "Could not parse configuration file "
				"%s\n", config_file);
			return -2;
		}

		while (optind < argc) {
			dump_tag = strtoll(argv[optind++], NULL, 16);
			fail += extract_platform_dump(dump_tag);
		}
	}
	else {
		fprintf(stderr, "No dump tag specified\n");
		print_usage(argv[0]);
		return -1;
	}

	return fail;
}
