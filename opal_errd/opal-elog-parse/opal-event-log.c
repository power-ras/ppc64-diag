#include <stdlib.h>
#include <string.h>
#include "opal-event-log.h"

int has_more_elements(struct opal_event_log_scn log_scn) {
   return !(log_scn.id[0] == '\0' && log_scn.id[1] == '\0' && log_scn.scn == NULL);
}

void *get_opal_event_log_scn(opal_event_log *log, const char *id, int n) {
   if (n < 0)
      return NULL;

   int i = 0;
   while (has_more_elements(log[i])) {
      if (!strncmp(log[i].id, id, 2) && n-- == 0)
         return log[i].scn;
      i++;
   }

   return NULL;
}
