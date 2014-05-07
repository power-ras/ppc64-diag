#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "opal-event-log.h"

opal_event_log *create_opal_event_log(int n) {
	opal_event_log *log = (struct opal_event_log_scn *)
		malloc(sizeof(struct opal_event_log_scn) * (n + 1));
	if (!log)
		return NULL;

	log[n].id[0] = '\0';
	log[n].id[1] = '\0';
	log[n].scn = NULL;

	return log;
}


int add_opal_event_log_scn(opal_event_log *log, const char *id, void *scn, int n) {
	if(!log || n < 0)
		return -EINVAL;

	memcpy(log[n].id, id, 2);
	log[n].scn = scn;

	return 0;
}

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

struct opal_event_log_scn *
get_nth_opal_event_log_scn(opal_event_log *log, int n)
{
	if (n < 0)
		return NULL;

	int i = 0;
	while (has_more_elements(log[i])) {
		if(n-- == 0)
			return log + i;
		i++;
	}

	return NULL;
}
