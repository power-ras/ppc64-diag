#ifndef _H_OPAL_EVENTS
#define _H_OPAL_EVENTS

#include <stdio.h>
#include <inttypes.h>

#include "opal-datetime.h"
#include "opal-v6-hdr.h"
#include "opal-priv-hdr-scn.h"
#include "opal-usr-scn.h"
#include "opal-src-scn.h"
#include "opal-mtms-scn.h"
#include "opal-lr-scn.h"
#include "opal-eh-scn.h"
#include "opal-ep-scn.h"
#include "opal-sw-scn.h"
#include "opal-ud-scn.h"
#include "opal-hm-scn.h"
#include "opal-ch-scn.h"
#include "opal-lp-scn.h"
#include "opal-ie-scn.h"
#include "opal-mi-scn.h"
#include "opal-ei-scn.h"
#include "opal-ed-scn.h"
#include "opal-dh-scn.h"

/* Header ID,
 * Required? (1 = yes, 2 = only with error),
 * Position (0 = no specific pos),
 * Max amount (0 = no max)
 */
struct header_id{
  const char *id;
  int req;
  int pos;
  int max;
};

#define HEADER_ORDER_MAX sizeof(elog_hdr_id)/sizeof(struct header_id)

#define HEADER_NOT_REQ 0x0
#define HEADER_REQ 0x1
#define HEADER_REQ_W_ERROR 0x2

#define HEADER_ORDER \
  {"PH", 1, 1, 1}, \
  {"UH", 1, 2, 1}, \
  {"PS", 2, 3, 1}, \
  {"EH", 1, 0, 1}, \
  {"MT", 2, 0, 1}, \
  {"SS", 0, 0, -1}, \
  {"DH", 0, 0, 1}, \
  {"SW", 0, 0, -1}, \
  {"LP", 0, 0 ,1}, \
  {"LR", 0, 0, 1}, \
  {"HM", 0, 0, 1}, \
  {"EP", 0, 0, 1}, \
  {"IE", 0, 0, 1}, \
  {"MI", 0, 0, 1}, \
  {"CH", 0, 0, 1}, \
  {"UD", 0, 0, -1}, \
  {"EI", 0, 0, 1}, \
  {"ED", 0, 0, -1}

#endif
