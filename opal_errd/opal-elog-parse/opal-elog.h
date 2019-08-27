#ifndef _H_OPAL_ELOG
#define _H_OPAL_ELOG

/*
 * As per PEL v6 (defined in PAPR spec) fixed offset for
 * error log information.
 */
#define ELOG_SRC_SIZE           8

#define ELOG_DATE_OFFSET        0x8
#define ELOG_COMMIT_TIME_OFFSET 0x10
#define ELOG_TIME_OFFSET        0xc
#define ELOG_CREATOR_ID_OFFSET  0x18
#define ELOG_ID_OFFSET          0x2c
#define ELOG_SEVERITY_OFFSET    0x3a
#define ELOG_SUBSYSTEM_OFFSET   0x38
#define ELOG_ACTION_OFFSET      0x42
#define ELOG_SRC_OFFSET         0x78
#define ELOG_MIN_READ_OFFSET    ELOG_SRC_OFFSET + ELOG_SRC_SIZE

/* Severity of the log */
#define OPAL_INFORMATION_LOG    0x00
#define OPAL_RECOVERABLE_LOG    0x10
#define OPAL_PREDICTIVE_LOG     0x20
#define OPAL_UNRECOVERABLE_LOG  0x40
#define OPAL_CRITICAL_LOG       0x50
#define OPAL_DIAGNOSTICS_LOG    0x60
#define OPAL_SYMPTOM_LOG        0x70

#define ELOG_ACTION_FLAG_SERVICE        0x8000
#define ELOG_ACTION_FLAG_CALL_HOME      0x0800

#define OPAL_ERROR_LOG_MAX      16384
#define ELOG_BUF_MAX            OPAL_ERROR_LOG_MAX * 10

#endif  /* _H_OPAL_ELOG */
