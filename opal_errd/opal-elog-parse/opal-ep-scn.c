#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "opal-ep-scn.h"
#include "opal-event-data.h"
#include "print_helpers.h"

int parse_ep_scn(struct opal_ep_scn **r_ep,
                 const struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_ep_scn *bufep = (struct opal_ep_scn *)buf;
	struct opal_ep_scn *ep;

	if (buflen < sizeof(struct opal_ep_scn)) {
		fprintf(stderr, "%s: corrupted, expected length >= %lu, got %u\n",
		        __func__,
		        sizeof(struct opal_ep_scn), buflen);
		return -EINVAL;
	}

	*r_ep = malloc(sizeof(struct opal_ep_scn));
	if(!*r_ep)
		return -ENOMEM;
	ep = *r_ep;

	ep->v6hdr = *hdr;

	ep->value = bufep->value;
	ep->modifier = bufep->modifier;
	ep->ext_modifier = be16toh(bufep->ext_modifier);
	ep->reason = be32toh(bufep->reason);

	return 0;
}

int print_ep_scn(const struct opal_ep_scn *ep)
{
	print_header("EPOW");
	print_opal_v6_hdr(ep->v6hdr);
	print_line("Sensor Value", "0x%x", ep->value >> OPAL_EP_VALUE_SHIFT);
	print_line("EPOW Action", "0x%x", ep->value & OPAL_EP_ACTION_BITS);
	print_line("EPOW Event", "0x%x", ep->modifier >> OPAL_EP_EVENT_SHIFT);
	if ((ep->value >> OPAL_EP_VALUE_SHIFT) == OPAL_EP_VALUE_SET) {
		print_line("EPOW Event Modifier", "%s",
		           get_ep_event_desc(ep->modifier & OPAL_EP_EVENT_BITS));
		if (ep->v6hdr.version == OPAL_EP_HDR_V) {
			if (ep->ext_modifier == 0x00)
				print_line("EPOW Ext Modifier", "System wide shutdown");
			else if (ep->ext_modifier == 0x01)
				print_line("EPOW Ext Modifier", "Parition specific shutdown");
		}
	}
	print_line("Platform reason code", "0x%x", ep->reason);

	return 0;
}
