#include <stdio.h>
#include <string.h>

#include "opal-mtms-struct.h"
#include "print_helpers.h"

int print_mtms_struct(const struct opal_mtms_struct mtms)
{
	char model[OPAL_SYS_MODEL_LEN+1];
	char serial_no[OPAL_SYS_SERIAL_LEN+1];

	memcpy(model, mtms.model, OPAL_SYS_MODEL_LEN);
	model[OPAL_SYS_MODEL_LEN] = '\0';
	memcpy(serial_no, mtms.serial_no, OPAL_SYS_SERIAL_LEN);
	serial_no[OPAL_SYS_SERIAL_LEN] = '\0';

	print_line("Machine Type Model", "%s", model);
	print_line("Serial Number", "%s", serial_no);

	return 0;
}
