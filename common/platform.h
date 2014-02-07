/**
 * Copyright (C) 2014 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef PLATFORM_H
#define PLARFORM_H

#define PLATFORM_FILE	"/proc/cpuinfo"

enum {
	PLATFORM_UNKNOWN = 0,
	PLATFORM_POWERKVM,
	PLATFORM_POWERKVM_GUEST,
	PLATFORM_PSERIES_LPAR,
	/* Add new platforms here */
	PLATFORM_MAX,
};

extern const char *__platform_name[];

extern int get_platform(void);

static inline const char * __power_platform_name(int platform)
{
	if (platform > PLATFORM_UNKNOWN && platform < PLATFORM_MAX)
		return __platform_name[platform];

	return __platform_name[PLATFORM_UNKNOWN];
}
#endif
