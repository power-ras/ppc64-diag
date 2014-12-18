/*
 * @file opal-dump-parse.h
 *
 * Copyright (C) 2014 IBM Corporation
 */

#ifndef __OPAL_DUMP_PARSE_H
#define __OPAL_DUMP_PARSE_H

#include <stdint.h>
#include <linux/types.h>

/* Error codes */
#define E_SUCCESS	0
#define E_USAGE		-1
#define E_INVALID	-2
#define E_FILE		-3
#define E_OPAL_TABLE	-4
#define E_OPAL_DATA	-5
#define E_SECTION	-6

/* MDST section defination
 *
 * Note:
 *   Any change in OPAL dump MDST structure (adding/removing section)
 *   requires update to this table
 */
#define DUMP_SECTION_DESC	\
	{0, "Unknown"}, \
	{1, "Opal-log"}, \
	{2, "HostBoot-Runtime-log"}, \
	{128, "printk"}

#define MDST_SECTION_DESC_LEN	128

struct mdst_section
{
	int	id;
	char	desc[MDST_SECTION_DESC_LEN];
};

/* MDST structure */
typedef struct _mdsttable
{
	__be64	addr;
	__be32	type; /* DUMP_SECTION_* */
	__be32	size;
} mdst_table;

/*
 * Following section defines dump header structure used to
 * retrieve OPAL dump.
 *
 * Note that we only define structures/macros required to
 * validate/retrieve OPAL dump section.
 */

/* Skiboot signature */
#define SKIBOOT_SIGNATURE              "SkiBoot"
#define SKIBOOT_SIGNATURE_LENGTH        7	/* Excluding NULL */

/* skiboot header size (size of metadata)
 * skiboot offset + SKIBOOT_HEADER_SIZE gives actual skiboot start address
 */
#define SKIBOOT_HEADER_SIZE             0x28


/* File directory structure */
#define DUMP_FILE_SIGNATURE               "FILE"
#define DUMP_FILE_SIGNATURE_LENGTH        4	/* Excluding NULL */

/* SYSDUMP name */
#define DUMP_FILE_PREFIX                "SYSDUMP"
#define DUMP_FILE_PREFIX_SIZE           7	/* Excluding NULL */
#define DUMP_FILE_SUFFIX_SIZE           33

typedef struct _dump_file_hdr
{
	char	label[8];
	__be16	dirsize;
	char	reserved[10];
	__be16	type;
	__be16	prefixsize;
	char	fname[40];
} dump_file_hdr;

/* Section directory signature */
#define SECTION_SIGNATURE               "SECTION"
#define SECTION_SIGNATURE_LENGTH        7             /* Excluding NULL */

#define SECTION_START_OFFSET            0x40
/*
 * Facility ID for "get block" section. OPAL logs TOC
 * is available under this block.
 */
#define BLOCK_DATA_FACILITY             0x10
#define SECTION_LAST                    0x00000001
typedef struct _secdirentry
{
	char	dirlabel[8];
	__be16	dirsize;
	__be16	priority;
	__be32	reserved1;
	__be32	flags;
	__be16	type;
	__be16	reserved2;
	__be64	secsize;
	char	secname[8];
} sec_dir_entry;

/* Dump header signature */
#define DUMP_HDR_SIGNATURE              "SYS DUMP"
#define DUMP_HDR_SIGNATURE_LENGTH       8

typedef struct _dumphdr
{
	char	type[8];
	__be64	timestamp;
	__be32	id;
	__be16	version;
	__be16	dumphdrsize;
	__be64	dumpsize;
	char	panel_data[32];
	char	sysname[32];
	char	sysserial[7];
	char	creator;
	__be32	eid;
	__be16	file_hdr_size;
	__be16	dump_src_size;
	char	dump_src[320];
	char	replaceable_serial[12];
} dump_hdr;

/* Dump data header */
#define	DATA_SECTION		"SYSDATA"
#define	DATA_SECTION_LENGTH	7
#define HWDATA_SIGNATURE	"HWDATA"
#define HWDATA_SIGNATURE_LENGTH	6

/* Current P8 has headerversion 2, check for the same */
#define SUPPORTED_DUMP_HDR_VERSION 2

typedef struct _nodalheader
{
	char	dumplabel[8];	/* "HWDATA"  "SPDATA"  or "SYSDATA" */
	__be16	headersize;	/* size of this struct is 48 bytes */
	__be16	headerversion;	/* 2 for P8 */
	__be32	headerflags;	/* barely used; LSB is 1 for SYSDATA section */
	__be32	dumpsize;	/* section size: header + toc + data */
	__be16	dumpversion;	/* set to 0x10 */
	__be16	dumpnodeid;	/* node number in range 0..7 */
	__be32	datasize;	/* data (only) size */
	__be32	toccnt;		/* count of TOC entries in this section */
	__be16	tocsize;
	__be16	homVersion;
	char	reserved[12];
} dump_node_header;

/* We have assumed any size greater than 0x100000 in HWDATA section
 * is the size of skiboot log
 */
#define SKIBOOT_SIZE_LIMIT              0x100000

/* HWDAATA TOC entry foramt */
typedef struct _hwtocentry
{
	__be32	id;
	__be32	cfam;
	__be64	hash;
	__be32	offset;
	__be32	flags;
	__be32	size;
} hw_toc_entry;

/* MDST table ID inside SYSDATA TOC section define by service process */
#define MDST_TABLE_ID                   0xf00c0001

/* SYSDATA TOC entry format */
typedef struct _systocentry
{
	__be32	id;     /* Generic ID */
	__be32	offset; /* Offset of Stored Data */
	__be32	size;   /* Size of Stored Data */
} sys_toc_entry;

#endif  /* __OPAL_DUMP_PARSE_H */
