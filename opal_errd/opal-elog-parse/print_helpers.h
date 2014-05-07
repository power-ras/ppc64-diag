#ifndef _H_OPAL_PRINT_HELPERS
#define _H_OPAL_PRINT_HELPERS

#include <stdint.h>

#define LINE_LENGTH 81
#define TITLE_LENGTH 29
#define ARG_LENGTH (LINE_LENGTH - TITLE_LENGTH)

int print_bar(void);

int print_center(const char *output);

int print_header(const char *header);

int print_hex(const uint8_t *values, int len);

int print_line(char *entry, const char *format, ...)
   __attribute__ ((format (printf, 2, 3)));

#endif /* _H_OPAL_PRINT_HELPERS */
