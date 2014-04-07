/*
 * @file opal_errd_parse.c
 * Copyright (C) 2014 IBM Corporation
 */

#include <stdio.h>
#include <stdlib.h>
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
#include <syslog.h>
#include <sys/types.h>

#include "libopalevents.h"

#define DEFAULT_opt_platform_dir "/var/log/opal-elog"
char *opt_platform_dir = DEFAULT_opt_platform_dir;
char *opt_platform_file;
int opt_display_file = 0;
int opt_display_all = 0;

#define ELOG_COMMIT_TIME_OFFSET	0x10
#define ELOG_CREATOR_ID_OFFSET	0x18
#define ELOG_ID_OFFSET		0x2c
#define ELOG_SEVERITY_OFFSET	0x3a
#define ELOG_ACTION_OFFSET	0x42
#define ELOG_SRC_OFFSET		0x78
#define OPAL_ERROR_LOG_MAX	16384
#define ELOG_ACTION_FLAG	0xa8000000

#define ELOG_SRC_SIZE		8
#define OPAL_ERROR_LOG_MAX      16384
#define ELOG_ACTION_FLAG        0xa8000000

#define ELOG_MIN_READ_OFFSET	ELOG_SRC_OFFSET + ELOG_SRC_SIZE

/* Severity of the log */
#define OPAL_INFORMATION_LOG    0x00
#define OPAL_RECOVERABLE_LOG    0x10
#define OPAL_PREDICTIVE_LOG     0x20
#define OPAL_UNRECOVERABLE_LOG  0x40

static int platform_log_fd = -1;

void print_usage(char *command)
{
	printf("Usage: %s { -d  <entryid> | -a | -l | -s | -h } [ -p dir | -f file]\n"
		"\t-d: Display error log entry details\n"
	        "\t-a: Display all error log entry details\n"
		"\t-l: list all error logs\n"
		"\t-s: list all call home logs\n"
	        "\t-p  dir: use dir as platform log directory (default %s)\n"
	        "\t-f  file: use individual file as platform log\n"
	        "\t-h: print the usage\n", command, DEFAULT_opt_platform_dir);
}

static int file_filter(const struct dirent *d)
{
        struct stat sbuf;
        if (d->d_type == DT_DIR)
           return 0;
        if (d->d_type == DT_REG)
           return 1;
        if (stat(d->d_name,&sbuf))
           return 0;
        if (S_ISDIR(sbuf.st_mode))
           return 0;
        if (d->d_type == DT_UNKNOWN)
           return 1;

        return 0;
}

int read_elog(char path[], void *buf, size_t count){

        struct stat sbuf;
        size_t bufsz;
        ssize_t readsz = 0;
        ssize_t sz = 0;
        int ret = 0;

        if (stat(path, &sbuf) == -1){
            fprintf(stderr, "Error accessing %s\n",path);
            return -1;
        }
        bufsz = sbuf.st_size;

        platform_log_fd = open(path, O_RDONLY);
        if (platform_log_fd <= 0) {
                fprintf(stderr, "Could not open error log file : %s (%s).\n "
                       "Skipping....\n", path, strerror(errno));
                goto out;
        }

        sz = 0;
        do {
            readsz = read(platform_log_fd, (char *)buf, count);
            if (readsz < 0) {
                    fprintf(stderr, "Read Platform log failed\n");
                    ret = -1;
                    goto out;
            }
            sz += readsz;
        } while(sz != bufsz);
        ret = sz;

out:
        close(platform_log_fd);
        return ret;

}

void print_elog_summary(char *buffer, int bufsz, uint32_t service_flag)
{
        char *parse;
        uint32_t logid;
        char src[ELOG_SRC_SIZE + 1];
        char severity;
        uint32_t action;
        struct opal_datetime date_time_in, date_time_out;
        uint8_t creator_id;

        logid = be32toh(*(uint32_t*)(buffer+ELOG_ID_OFFSET));
        memcpy(src, (buffer + ELOG_SRC_OFFSET), sizeof(src));
        src[ELOG_SRC_SIZE] = '\0';
        severity = buffer[ELOG_SEVERITY_OFFSET];
        action = be32toh(*(uint32_t *)(buffer + ELOG_ACTION_OFFSET));
        switch (severity) {
        case OPAL_INFORMATION_LOG:
                parse = "Informational Event";
                break;
        case OPAL_RECOVERABLE_LOG:
                parse = "Recoverable Error";
                break;
        case OPAL_PREDICTIVE_LOG:
                parse = "Predictive Error";
                break;
        case OPAL_UNRECOVERABLE_LOG:
                parse = "Unrecoverable Error";
                break;
        default:
                parse = "NONE";
                break;
        }

        date_time_in = *(const struct opal_datetime *)(buffer + ELOG_COMMIT_TIME_OFFSET);
        date_time_out = parse_opal_datetime(date_time_in);
        creator_id = buffer[ELOG_CREATOR_ID_OFFSET];
        if (service_flag != 1)
                        printf("|0x%08X %8.8s %4u-%02u-%02u %02u:%02u:%02u  %-17.17s %-19.19s|\n",
                                logid, src,
                                date_time_out.year, date_time_out.month,
                                date_time_out.day, date_time_out.hour,
                                date_time_out.minutes, date_time_out.seconds,
                                get_creator_name(creator_id), parse);
                else if ((action == ELOG_ACTION_FLAG) && (service_flag == 1))
                        /* list only service action logs */
                        printf("|0x%08X %8.8s %4u-%02u-%02u %02u:%02u:%02u  %-17.17s %-19.19s|\n",
                                logid, src,
                                date_time_out.year, date_time_out.month,
                                date_time_out.day, date_time_out.hour,
                                date_time_out.minutes, date_time_out.seconds,
                                get_creator_name(creator_id), parse);

}

/* parse error log entry passed by user */
int elogdisplayentry(uint32_t eid)
{
	uint32_t logid;
	int ret = 0;
	char buffer[OPAL_ERROR_LOG_MAX];
        struct dirent **filelist;
        int nfiles;
        ssize_t sz = 0;
        int i;

        if(opt_display_file){
            sz = read_elog(opt_platform_file, (char *)buffer, OPAL_ERROR_LOG_MAX);
            if(sz < 0) {
                return -1;
            /* Make sure we read minimum data needed in this function */
            } else if (sz < (ELOG_ID_OFFSET + sizeof(logid))){
                       fprintf(stderr, "Partially read elog, cannot parse\n");
                       return -1;
            }

            logid = be32toh(*(uint32_t*)(buffer+ELOG_ID_OFFSET));
            if (opt_display_all || logid == eid) {
                    ret = parse_opal_event(buffer, sz);
            }

        } else {
            nfiles = scandir(opt_platform_dir, &filelist,
                             file_filter, alphasort);
            if (nfiles < 0){
                fprintf(stderr, "Error accessing directory: %s\n",opt_platform_dir);
                return -1;
            }
            if (nfiles == 0){
                fprintf(stderr,"0 files found in directory: %s\n",opt_platform_dir);
                return -1;
            }

            for (i = 0; i < nfiles; i++){

                chdir(opt_platform_dir);
                sz = read_elog(filelist[i]->d_name, (char *)buffer, OPAL_ERROR_LOG_MAX);
                if(sz < 0) {
                    return -1;
                /* Make sure we read minimum data needed in this function */
                } else if (sz < (ELOG_ID_OFFSET + sizeof(logid))){
                           fprintf(stderr, "Partially read elog, cannot parse\n");
                           continue;
                }

                if(sz > 0){
                    logid = be32toh(*(uint32_t*)(buffer+ELOG_ID_OFFSET));
                    if (opt_display_all || logid == eid) {
                            ret = parse_opal_event(buffer, sz);
                            if (!opt_display_all)
                                    break;
                    }
                }
                free(filelist[i]);

            }
            free(filelist);
        }

	return ret;

}

/* list all the error logs */
int eloglist(uint32_t service_flag)
{
	int ret = 0;
	char buffer[OPAL_ERROR_LOG_MAX];
        struct dirent **filelist;
        int nfiles;
        ssize_t sz = 0;
        int i;

	printf("|------------------------------------------------------------------------------|\n");
	printf("|ID         SRC      Date       Time      Creator           Event Severity     |\n");
	printf("|------------------------------------------------------------------------------|\n");

        if(opt_display_file){
            sz = read_elog(opt_platform_file, (char *)buffer, OPAL_ERROR_LOG_MAX);
            if (sz <= 0) {
                return -1;
            } else if (sz < ELOG_MIN_READ_OFFSET) {
                fprintf(stderr, "Partially read elog, cannot parse\n");
                return -1;
            }

            print_elog_summary(buffer, sz, service_flag);

        } else {
            nfiles = scandir(opt_platform_dir, &filelist,
                             file_filter, alphasort);

            if (nfiles < 0){
                fprintf(stderr,"Error accessing directory: %s\n",opt_platform_dir);
                return -1;
            }
            if (nfiles == 0){
                fprintf(stderr,"0 files found in directory: %s\n",opt_platform_dir);
                return -1;
            }

            for (i = 0; i < nfiles; i++){

                chdir(opt_platform_dir);
                sz = read_elog(filelist[i]->d_name, (char *)buffer, OPAL_ERROR_LOG_MAX);
                if (sz <= 0){
                    continue;
                } else if (sz < ELOG_MIN_READ_OFFSET) {
                    fprintf(stderr, "Partially read elog, cannot parse\n");
                    continue;
                }

                print_elog_summary(buffer, sz, service_flag);
                free(filelist[i]);
            }

        free(filelist);

        }
        if(!ret){
            printf("|------------------------------------------------------------------------------|\n");
        }

	return ret;
}

int main(int argc, char *argv[])
{
	uint32_t eid = 0;
	int opt = 0, ret = 0;
	int arg_cnt = 0;
	char do_operation = '\0';
	const char *eid_opt;

	while ((opt = getopt(argc, argv, "ad:lshf:p:")) != -1) {
		switch (opt) {
		case 'd':
			eid_opt = optarg;
			/* fallthrough */
		case 'l':
		case 's':
			arg_cnt++;
			do_operation = opt;
			break;
		case 'a':
			arg_cnt++;
			opt_display_all = 1;
			do_operation = opt;
			break;
		case 'f':
                        opt_platform_file = optarg;
                        opt_display_file = 1;
			break;
                case 'p':
			opt_platform_dir = optarg;
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
		fprintf(stderr, "No parameters are specified\n");
		print_usage(argv[0]);
		ret = -1;
	}

	if (arg_cnt > 1) {
		fprintf(stderr, "Only one operation (-d | -a | -l | -s) "
			"can be selected at any one time.\n");
		print_usage(argv[0]);
		return -1;
	}
	switch (do_operation) {
	case 'l':
		ret = eloglist(0);
		break;
	case 'd':
		eid = strtoul(eid_opt, 0, 0);
		/* fallthrough */
	case 'a':
		ret = elogdisplayentry(eid);
		break;
	case 's':
		ret = eloglist(1);
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}
