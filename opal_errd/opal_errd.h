/**
 * @file	opal_errd.h
 * @brief	Main header for opal_errd
 *
 * Copyright (C) 2014 IBM Corporation.
 */
#ifndef _OPAL_ERRD_H
#define _OPAL_ERRD_H

#define SYSFS_ELOG	"/sys/firmware/opal/opal_elog"
#define SYSFS_ELOG_ACK	"/sys/firmware/opal/opal_elog_ack"
#define PLATFORM_LOG	"/var/log/platform"

#define EXTRACT_OPAL_DUMP_CMD	"/usr/sbin/extract_opal_dump"

/*
 * As per PEL v6 (defined in PAPR spec) fixed offset for
 * error log information.
 */
#define OPAL_ERROR_LOG_MAX	16384
#define ELOG_ID_SIZE		4
#define ELOG_SRC_SIZE		8

#define ELOG_ID_OFFESET		0x2c
#define ELOG_DATE_OFFSET	0x8
#define ELOG_TIME_OFFSET	0xc
#define ELOG_SRC_OFFSET		0x78
#define ELOG_SEVERITY_OFFSET	0x3a
#define ELOG_SUBSYSTEM_OFFSET	0x38
#define ELOG_ACTION_OFFSET	0x42

/* Severity of the log */
#define OPAL_INFORMATION_LOG	0x00
#define OPAL_RECOVERABLE_LOG	0x10
#define OPAL_PREDICTIVE_LOG	0x20
#define OPAL_UNRECOVERABLE_LOG	0x40

#define ELOG_ACTION_FLAG	0xa8000000

#endif /* _OPAL_ERRD_H */
