/*
 * Copyright (C) 2012, 2015 IBM Corporation
 * See 'COPYRIGHT' for License of this code.
 */

#ifndef _ENCL_LED_H
#define _ENCL_LED_H

#define LED_SAME	-1
#define LED_OFF		0
#define LED_ON		1

extern const char *progname;

extern int bluehawk_list_leds(const char *, const char *, int);
extern int bluehawk_set_led(const char *, const char *, int, int, int);

#endif	/* _ENCL_LED_H */
