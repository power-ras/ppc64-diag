#ifndef _H_OPAL_EVENT_LOG
#define _H_OPAL_EVENT_LOG

struct opal_event_log_scn {
   char id[2];
   void *scn;
};

typedef struct opal_event_log_scn opal_event_log;

opal_event_log *create_opal_event_log(int n);

int add_opal_event_log_scn(opal_event_log *log, const char *id, void *scn, int n);

int has_more_elements(struct opal_event_log_scn log_scn);

void *get_opal_event_log_scn(opal_event_log *log, const char *id, int n);

void *get_opal_event_log_scn(opal_event_log *log, const char *id, int n);


#endif /* _H_OPAL_EVENT_LOG */
