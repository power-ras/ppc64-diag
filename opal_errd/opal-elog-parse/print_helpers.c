#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "print_helpers.h"

int print_bar(void)
{
	int i;
	putchar('|');
	for(i = 0; i < LINE_LENGTH-3; i++)
		putchar('-');
	printf("|\n");
	return 0;
}

int print_center(const char *output)
{
   int len = strlen(output);
   int i;

   putchar('|');

   i = 0;
   while (i < (LINE_LENGTH - 3 - len)/2) {
      putchar(' ');
      i++;
   }

   printf("%s", output);

   i = 0;
   while(i < (LINE_LENGTH - 3 - len) / 2) {
      putchar(' ');
      i++;
   }

   if ((LINE_LENGTH - 2 - len) % 2 == 0)
      putchar(' ');

   printf("|\n");

   return 0;
}

int print_header(const char *header)
{
   print_center(header);

   print_bar();

   return 0;
}

int print_line(char *entry, const char *format, ...)
{
   va_list args;
   int written;
   int title;
   char buf[ARG_LENGTH];

   if(strlen(entry) > LINE_LENGTH - 6)
      entry[LINE_LENGTH - 6] = '\0';

   title = printf("| %-25s: ", entry);
   if (title < TITLE_LENGTH)
      return title;

   va_start(args, format);
   written = vsnprintf(buf, LINE_LENGTH - title - 1, format, args);
   va_end(args);
   if (written < LINE_LENGTH - title && written >= 0) {
      /* Didn't overflow the 80 character per line limit */
      written = title;
      written += printf("%s", buf);
      while (written < LINE_LENGTH - 2) {
         putchar(' ');
         written++;
      }
      written += printf("|\n");
   } else if (written > 0) {
      /* Did overflow the 80 character per line limit, print the rest on
      * the next line(s)
      */
      char *remainder = malloc((written + 1) * sizeof(char));
      va_start(args, format);
      vsnprintf(remainder, written + 1, format, args);
      va_end(args);
      written = title;
      written += printf("%s|\n",buf);
      int i = ARG_LENGTH - 2;
      written += printf("|%*c: ",TITLE_LENGTH-3,' ');
      while (remainder[i] != '\0') {
         putchar(remainder[i]);
         i++;
         written++;
         if ((written + 2) % (LINE_LENGTH) == 0) {
            /* Swallow leading spaces */
            while(remainder[i] == ' ') {
               i++;
            }
            /* Only print this goodness if there is another line */
            if(remainder[i] != '\0') {
               written += printf("|\n");
               written += printf("|%*c: ",TITLE_LENGTH-3,' ');
            }
         }
      }

      while ((written + 2) % LINE_LENGTH) {
         putchar(' ');
         written++;
      }
      written += printf("|\n");
      free(remainder);
   }

   return written;
}

int print_hex(const uint8_t *values, int len) {
	int written = 0;
	int i = 0;
	int j = 0;
	int lines = 0;
	/* Going to run into problems if we don't have an integer number of words */
	if (len % sizeof(uint32_t))
		return 0;

	while (i < len) {
		if (written % LINE_LENGTH == 0) {
			written = printf("|   %08x    ", i);
			lines++;
		}

		for (j = 0; j < len - i && j < 16; j += 4)
			written += printf("%08x  ", be32toh(*((uint32_t *) (values + j + i))));

		/* padd with spaces before printing the chars*/
		while (written < LINE_LENGTH - 20) {
			putchar(' ');
			written++;
		}

		for (j = 0; j < len - i && j < 16; j++)
			written += printf("%c", isprint(values[j + i]) ? values[j + i] : '.');

		i += j;
		while ((written + 2) % LINE_LENGTH != 0) {
			putchar(' ');
			written++;
		}
		written += printf("|\n");
	}

	return lines * LINE_LENGTH;
}
