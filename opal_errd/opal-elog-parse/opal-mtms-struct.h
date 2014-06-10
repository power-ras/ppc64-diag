#ifndef _H_OPAL_MTMS_STRUCT
#define _H_OPAL_MTMS_STRUCT

#define OPAL_SYS_MODEL_LEN  8
#define OPAL_SYS_SERIAL_LEN 12

struct opal_mtms_struct {
	char model[OPAL_SYS_MODEL_LEN];
	char serial_no[OPAL_SYS_SERIAL_LEN];
} __attribute__((packed));

static inline void copy_mtms_struct(struct opal_mtms_struct *dest,
                             const struct opal_mtms_struct *src)
{
	memcpy(dest->model, src->model, OPAL_SYS_MODEL_LEN);
	memcpy(dest->serial_no, src->serial_no, OPAL_SYS_SERIAL_LEN);
}

int print_mtms_struct(const struct opal_mtms_struct mtms);

#endif /* _H_OPAL_MTMS_STRUCT */
