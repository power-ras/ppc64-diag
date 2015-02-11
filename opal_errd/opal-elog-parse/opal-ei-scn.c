#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "opal-ei-scn.h"
#include "parse_helpers.h"
#include "print_helpers.h"

int parse_ei_scn(struct opal_ei_scn **r_ei,
                 struct opal_v6_hdr *hdr,
                 const char *buf, int buflen)
{
	struct opal_ei_scn *ei;
	struct opal_ei_scn *eibuf = (struct opal_ei_scn *)buf;

	if (check_buflen(buflen, sizeof(struct opal_ei_scn), __func__) < 0 ||
		 check_buflen(hdr->length, sizeof(struct opal_ei_scn), __func__) < 0)
		return -EINVAL;

	*r_ei = malloc(hdr->length);
	if (!*r_ei)
		return -ENOMEM;

	ei = *r_ei;

	ei->v6hdr = *hdr;
	ei->g_timestamp = be64toh(eibuf->g_timestamp);
	ei->genesis.corrosion = be32toh(eibuf->genesis.corrosion);
	ei->genesis.temperature = be16toh(eibuf->genesis.temperature);
	ei->genesis.rate = be16toh(eibuf->genesis.rate);
	ei->status = eibuf->status;
	ei->user_data_scn = eibuf->user_data_scn;
	ei->read_count = be16toh(eibuf->read_count);
	if (check_buflen(hdr->length, sizeof(struct opal_ei_scn) +
		 (ei->read_count * sizeof(struct opal_ei_env_scn)),
		 __func__) < 0 ||
		 check_buflen(buflen, sizeof(struct opal_ei_scn) +
		 (ei->read_count * sizeof(struct opal_ei_env_scn)),
		 __func__)) {
		free(ei);
		return -EINVAL;
	}

	int i;
	for (i = 0; i < ei->read_count; i++) {
		ei->readings[i].corrosion = be32toh(eibuf->readings[i].corrosion);
		ei->readings[i].temperature = be16toh(eibuf->readings[i].temperature);
		ei->readings[i].rate = be16toh(eibuf->readings[i].rate);
	}

	return 0;
}

static int print_ei_env_scn(const struct opal_ei_env_scn *ei_env)
{
	print_line("Avg Norm corrosion", "0x%08x", ei_env->corrosion);
	print_line("Avg Norm temp", "0x%04x", ei_env->temperature);
	print_line("Corrosion rate", "0x%04x", ei_env->rate);

	return 0;
}


int print_ei_scn(const struct opal_ei_scn *ei)
{
	print_header("Environmental Information");
	print_opal_v6_hdr(ei->v6hdr);
	print_center("Genesis Readings");
	print_line("Timetamp", "0x%016lx", ei->g_timestamp);
	print_ei_env_scn(&(ei->genesis));

	print_center(" ");
	if (ei->status == CORROSION_RATE_NORM)
		print_line("Corrosion Rate Status", "Normal");
	else if (ei->status == CORROSION_RATE_ABOVE)
		print_line("Corrosion Rate Status", "Above Normal");
	else
		print_line("Corrosion Rate Status", "Unknown");

	print_line("User Data Section", "%s",
	           ei->user_data_scn ? "Present" : "Absent");

	print_line("Sensor Reading Count", "0x%04x", ei->read_count);
	int i;
	for(i = 0; i < ei->read_count; i++)
		print_ei_env_scn(ei->readings + i);

	return 0;
}
