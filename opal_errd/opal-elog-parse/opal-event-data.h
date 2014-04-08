#ifndef _H_OPAL_EVENTS_DATA
#define _H_OPAL_EVENTS_DATA

#include <stdint.h>

#include "libopalevents.h"

struct generic_desc{
	uint8_t id;
	const char *desc;
};

const char *get_event_desc(uint8_t id);

const char *get_subsystem_name(uint8_t id);

const char *get_severity_desc(uint8_t id);

const char *get_creator_name(uint8_t id);

const char *get_event_scope(uint8_t id);

const char *get_elog_desc(uint8_t id);

const char *get_fru_priority_desc(uint8_t id);

const char *get_fru_component_desc(uint8_t id);

const char *get_ep_event_desc(int id);
#endif /* _H_OPAL_EVENTS_DATA */
