/*
 * @file opal-dump-parse.c
 *
 * Copyright (C) 2014 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <endian.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "opal-dump-parse.h"

char *dump_file;
char *opt_output_file = "Skiboot-log";

int opt_output_flag = 0;
int opt_mdst = 0;
int opt_sec_id = 0;
int opt_sec_file = 0;

/* OPAL dump section detail */
struct mdst_section section_types[] = {
	DUMP_SECTION_DESC
};

static void print_usage(char *command)
{
	printf("Usage: %s [-l | -h | -s id | -o file] <SYSDUMP>\n\n"
		"\t-l      - List all the sections\n"
		"\t-s id   - Capture log with specified section id\n"
		"\t-o file - Output file to capture the log with specified"
		" section id\n"
		"\t-h      - Print this message and exit\n",
		command);
}

/*
 * Validates section directory header computes dump header offset
 *
 * Returns:
 *   E_SUCCESS - success
 *   E_INVALID - failure
 */
static int validate_sections(char *data, int *dump_header)
{
	uint16_t offset;
	sec_dir_entry *section;

	offset = SECTION_START_OFFSET;

	do {
		section = (sec_dir_entry *)(data + offset);

		if (strncmp(section->dirlabel, SECTION_SIGNATURE, SECTION_SIGNATURE_LENGTH)) {
			fprintf(stderr, "%s signature mismatch\n",
				SECTION_SIGNATURE);
			return E_INVALID;
		}

		offset += be16toh(section->dirsize);

	} while (!(be32toh(section->flags) & SECTION_LAST));

	*dump_header = offset;

	return E_SUCCESS;
}

/*
 * Validates dump header computes hwdata section offset
 *
 * Returns:
 *   E_SUCCESS - success
 *   E_INVALID - failure
 */
static int validate_dump_header(char *data, int dump_header, int *hwdata_start)
{
	dump_hdr *hdr;

	hdr = (dump_hdr *)(data + dump_header);

	if (strncmp(hdr->type, DUMP_HDR_SIGNATURE,  DUMP_HDR_SIGNATURE_LENGTH)) {
		fprintf(stderr, "%s signature mismatch\n", DUMP_HDR_SIGNATURE);
		return E_INVALID;
	}

	*hwdata_start = dump_header + be16toh(hdr->dumphdrsize);
	return E_SUCCESS;
}

/*
 * Validates HWDATA section and compute offset to skiboot log
 *
 * Returns:
 *   E_SUCCESS - success
 *   E_INVALID - failure
 */
static int get_skiboot(char *data, int hwdata_start,
		       int *skiboot_start, uint32_t *skiboot_size)
{
	int toc_start, next_toc;
	int i;
	uint16_t toc_size;
	uint32_t toc_cnt, offset = 0, facility;
	dump_node_header *hdr;
	hw_toc_entry *toc;

	hdr = (dump_node_header *)(data + hwdata_start);

	if (be16toh(hdr->headerversion) != SUPPORTED_DUMP_HDR_VERSION) {
		fprintf(stderr, "Unsupported dump version: %d\n",
							be16toh(hdr->headerversion));
		return E_INVALID;
	}

	if (strncmp(hdr->dumplabel, HWDATA_SIGNATURE, HWDATA_SIGNATURE_LENGTH)) {
		fprintf(stderr, "%s signature mismatch\n", HWDATA_SIGNATURE);
		return E_INVALID;
	}

	toc_start = hwdata_start + be16toh(hdr->headersize);
	toc_cnt = be32toh(hdr->toccnt);
	toc_size = be16toh(hdr->tocsize);

	next_toc = toc_start;

	for (i = 0; i < toc_cnt; i++) {
		toc = (hw_toc_entry *)(data + next_toc);

		*skiboot_size = be32toh(toc->size);

		/* first 5 bits of flags is used for facility */
		facility = (be32toh(toc->flags) >> 27) & BLOCK_DATA_FACILITY;

		if (facility && (*skiboot_size > SKIBOOT_SIZE_LIMIT)) {
			*skiboot_size -= SKIBOOT_HEADER_SIZE;
			offset = be32toh(toc->offset);
			break;
		}
		next_toc += toc_size;
	}

	if (!offset) {
		fprintf(stderr, "Failed to get skiboot offset\n");
		return E_OPAL_DATA;
	}

	*skiboot_start = toc_start + (toc_size * toc_cnt) + offset + SKIBOOT_HEADER_SIZE;

	if (strncmp(&data[*skiboot_start], SKIBOOT_SIGNATURE, SKIBOOT_SIGNATURE_LENGTH)) {
		*skiboot_start += SKIBOOT_TIMESTAMP_LENGTH;

		if (strncmp(&data[*skiboot_start], SKIBOOT_SIGNATURE, SKIBOOT_SIGNATURE_LENGTH)) {
			fprintf(stderr, "%s signature mismatch\n", SKIBOOT_SIGNATURE);
			return E_INVALID;
		}
	}

	return E_SUCCESS;
}

/*
 * Writes log to the file
 * Captures the contents from the specified offset to a file
 */
static int write_log(char path[], int flag,
		     char *data, int skiboot_start, int size)
{
	int fd;
	int rc = E_FILE, sz, ret;
	char dump_path[PATH_MAX];
	char dump_suffix[DUMP_FILE_SUFFIX_SIZE];

	strcpy(dump_path, path);

	if (!flag) {
		strncpy(dump_suffix,
			&data[offsetof(dump_file_hdr, fname) + DUMP_FILE_PREFIX_SIZE],
			DUMP_FILE_SUFFIX_SIZE);
		strcat(dump_path, dump_suffix);
	}

	fd = open(dump_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);

	if (fd == -1) {
		fprintf(stderr, "Could not write to output file "
			"\"%s\", %s.\n", dump_path, strerror(errno));
		goto err;
	}

	sz = write(fd, data + skiboot_start, size);

	if (sz != size) {
		fprintf(stderr, "Could not write to output file "
			"\"%s\", %s.\n", dump_path, strerror(errno));
		goto err;
	}

	ret = fsync(fd);
	if (ret == -1) {
		fprintf(stderr, "Failed to sync elog output file: "
                       "\"%s\", %s.\n", opt_output_file, strerror(errno));
		goto err;
	}

	printf("Captured log to file %s\n", dump_path);
	rc = E_SUCCESS;
err:
	if (fd != -1)
		close(fd);

	return rc;
}

/*
 * Validates mdst table Computes offset of mdst table
 *
 * Returns:
 *   E_SUCCESS    - success
 *   E_OPAL_TABLE - failure
 */
static int get_mdst_table_offset(char *data, int hwdata_start,
				 int *mdst_start, int *mdst_cnt)
{
	uint32_t toc_size;
	int sec_start, toc_start;
	uint32_t toc_cnt;
	int i, next_toc, found = 0;
	dump_node_header *hdr;
	sys_toc_entry *toc;

	sec_start = hwdata_start;
	hdr = (dump_node_header *) (data + sec_start);

	/* Look for SYSDATA section */
	while (strncmp(&data[sec_start], DATA_SECTION, DATA_SECTION_LENGTH)) {
		sec_start += be32toh(hdr->dumpsize);
		hdr = (dump_node_header *) (data + sec_start);
	}

	toc_start = sec_start + be16toh(hdr->headersize);
	toc_cnt = be32toh(hdr->toccnt);
	toc_size = be16toh(hdr-> tocsize);

	next_toc = toc_start;

	for (i = 0; i < toc_cnt; i++) {
		toc = (sys_toc_entry *)(data + next_toc);

		if (be32toh(toc->id) == MDST_TABLE_ID) {
			found = 1;
			break;
		}

		next_toc += toc_size;
	}

	if (!found) {
		fprintf(stderr, "MDST table not found\n");
		return E_OPAL_TABLE;
	}

	*mdst_start = toc_start + (toc_size * toc_cnt) + be32toh(toc->offset);
	*mdst_cnt = be32toh(toc->size) / sizeof(mdst_table);

	return E_SUCCESS;
}

static inline char *parse_section(int type)
{
	int i;

	for (i = 0; i < sizeof(section_types) / sizeof(section_types[0]); i++) {
		if (type == section_types[i].id)
			return section_types[i].desc;
	}

	return section_types[0].desc;
}

/*
 * Captures the content of the specified section to a file.
 *
 * Returns:
 */
static int print_section(char *data, int mdst_start,
			 int mdst_count, int skiboot_start)
{
	int i, rc, next, skiboot_offset, found = 0;
	uint32_t type, size, total_size = 0;
	mdst_table *mdst_t;

	for (i = 0, next = mdst_start; i < mdst_count; i++, next += sizeof(mdst_table)) {
		mdst_t = (mdst_table *)(data + next);

		type = be32toh(mdst_t->type);
		size = be32toh(mdst_t->size);

		if (type == opt_sec_id) {
			found = 1;
			break;
		}

		total_size += size;
	}

	if (!found) {
		fprintf(stderr, "Section id %d is invalid\n", opt_sec_id);
		return E_SECTION;
	}

	skiboot_offset = skiboot_start + total_size;
	if (!opt_output_flag)
		opt_output_file = parse_section(type);

	rc = write_log(opt_output_file,
		       opt_output_flag, data, skiboot_offset, size);
	return rc;
}

/* Lists the sections from the mdst table
 *
 * In case of a invalid section type returns E_SECTION.
 */
static void list_section(char *data, int mdst_start, int mdst_count)
{
	int i, next;
	uint32_t size, type;
	mdst_table *mdst_t;

	printf("|---------------------------------------------------------|\n");
	printf("|ID              SECTION                              SIZE|\n");
	printf("|---------------------------------------------------------|\n");


	for (i = 0, next = mdst_start; i < mdst_count; i++, next += sizeof(mdst_table)) {
		mdst_t = (mdst_table *)(data + next);

		type = be32toh(mdst_t->type);
		size = be32toh(mdst_t->size);

		printf("|%-d\t\t%-18s\t\t%10d|\n", type, parse_section(type), size);
	}
	printf("|---------------------------------------------------------|\n");
	printf("List completed\n");
}

/*
 * parses the SYSDUMP file, captures the opal log to a file.
 */
static int parse_dump(void)
{
	int dump_fd;
	int dump_header;
	int mdst_cnt, mdst_start;
	int rc = E_SUCCESS;
	int hwdata_start, skiboot_start;
	uint32_t skiboot_size;
	struct stat dump_sbuf;
	char *dump_map = NULL, *data = NULL;
	dump_file_hdr *hdr;

	if ((stat(dump_file, &dump_sbuf)) < 0) {
		fprintf(stderr, "Could not get status of dump configuration"
			" file \"%s\", %s.\n", dump_file, strerror(errno));
		return E_FILE;
	}

	if (dump_sbuf.st_size == 0)
		return E_FILE;

	if ((dump_fd = open(dump_file, O_RDONLY)) < 0) {
		fprintf(stderr, "Could not open the dump file "
			"\"%s\", %s.\n", dump_file, strerror(errno));
		return E_FILE;
	}

	if ((dump_map = mmap(0, dump_sbuf.st_size, PROT_READ, MAP_PRIVATE,
				dump_fd, 0)) == (char *)-1) {
		fprintf(stderr, "Could not map dump file "
				  "\"%s\", %s.\n", dump_file, strerror(errno));
		close(dump_fd);
		return E_FILE;
	}

	data = dump_map;
	hdr = (dump_file_hdr *) (data);

	/* Validate file signature */
	if (strncmp(hdr->label, DUMP_FILE_SIGNATURE, DUMP_FILE_SIGNATURE_LENGTH)) {
		fprintf(stderr, "Not a valid system dump\n");
		rc = E_INVALID;
		goto out;
	}

	/* Validate file name */
	if (strncmp(hdr->fname,  DUMP_FILE_PREFIX, DUMP_FILE_PREFIX_SIZE)) {
		fprintf(stderr, "Not a valid system dump\n");
		rc = E_INVALID;
		goto out;
	}

	/* Validate sections */
	if ((rc = validate_sections(data, &dump_header)) < 0)
		goto out;

	/* Validate Dump header */
	if ((rc = validate_dump_header(data, dump_header, &hwdata_start)) < 0)
		goto out;

	/* Get skiboot offset */
	if ((rc = get_skiboot(data, hwdata_start, &skiboot_start, &skiboot_size)) < 0)
		goto out;

	if (opt_mdst || opt_sec_file) {
		/* Retrieve MDST structure */
		rc = get_mdst_table_offset(data, hwdata_start, &mdst_start, &mdst_cnt);
		if (rc < 0)
			goto out;

		if (opt_sec_file)
			rc = print_section(data, mdst_start, mdst_cnt, skiboot_start);
		else
			list_section(data, mdst_start, mdst_cnt);
	} else
		/* copy skiboot log contents to the file */
		write_log(opt_output_file, opt_output_flag, data, skiboot_start, skiboot_size);

out:
	close(dump_fd);
	munmap(dump_map, dump_sbuf.st_size);
	return rc;
}

int main(int argc, char *argv[])
{
	int opt = 0;

	while ((opt = getopt(argc, argv, "lh:s:o:")) != -1) {
		switch (opt) {
		case 'l':
			opt_mdst = 1;
			break;
		case 's':
			opt_sec_id = atoi(optarg);
			opt_sec_file = 1;
			break;
		case 'o':
			opt_output_flag = 1;
			opt_output_file = optarg;
			break;
		case 'h':
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
		default:
			print_usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (argc == 1) {
		fprintf(stderr, "No parameters specified\n\n");
		print_usage(argv[0]);
		return E_USAGE;
	}

	/* Options l & s are mutually exclusive */
	if (opt_mdst && opt_sec_file) {
		fprintf(stderr, "Only one operation can be performed at a time "
				"(-l | -s)\n\n");
		print_usage(argv[0]);
		return E_USAGE;
	}

	if (opt_mdst && opt_output_flag) {
		fprintf(stderr, "The -l and -o options cannot be used "
			"together\n\n");
		print_usage(argv[0]);
		return E_USAGE;
	}

	if (optind >= argc) {
		fprintf(stderr, "Expected SYSDUMP file\n\n");
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	dump_file = argv[optind];

	if (optind + 1 < argc) {
		fprintf(stderr, "Too many arguments\n\n");
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	return parse_dump();
}
