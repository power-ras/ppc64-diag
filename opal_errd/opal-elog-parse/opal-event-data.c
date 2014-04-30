#include <stdint.h>

struct generic_desc{
	uint8_t id;
	const char *desc;
};

#define MAX_EVENT sizeof(usr_hdr_event_type)/sizeof(struct generic_desc)
#define MAX_SEV      sizeof(usr_hdr_severity)/sizeof(struct generic_desc)
#define MAX_CREATORS    sizeof(prv_hdr_creator_id)/sizeof(struct generic_desc)
#define MAX_SUBSYSTEMS     sizeof(usr_hdr_subsystem_id)/sizeof(struct generic_desc)
#define MAX_EVENT_SCOPE sizeof(usr_hdr_event_scope)/sizeof(struct generic_desc)
#define MAX_FRU_PRIORITY sizeof(fru_scn_priority)/sizeof(struct generic_desc)
#define MAX_FRU_ID_COMPONENT sizeof(fru_id_scn_component)/sizeof(struct generic_desc)
#define MAX_EP_EVENT sizeof(ep_event)/sizeof(struct generic_desc)
#define MAX_LR_RES sizeof(lr_res)/sizeof(struct generic_desc)
#define MAX_IE_TYPE sizeof(ie_type)/sizeof(struct generic_desc)
#define MAX_IE_SCOPE sizeof(ie_scope)/sizeof(struct generic_desc)
#define MAX_IE_SUBTYPE sizeof(ie_subtype)/sizeof(struct generic_desc)
#define MAX_DH_TYPE sizeof(dh_type)/sizeof(struct generic_desc)

#define EVENT_TYPE \
   {0x00, "Not applicable."}, \
   {0x01, "Miscellaneous, informational only."},\
   {0x02, "Tracing event"}, \
   {0x08, "Dump notification."},\
   {0x10, "Previously reported error has been corrected by system."},\
   {0x20, "System resources manually deconfigured by user."}, \
   {0x21, "System resources deconfigured by system due to prior error event." }, \
   {0x22, "Resource deallocation event notification."}, \
   {0x30, "Customer environmental problem has returned to normal."}, \
   {0x40, "Concurrent maintenance event."}, \
   {0x60, "Capacity upgrade event."}, \
   {0x70, "Resource sparing event."}, \
   {0x80, "Dynamic reconfiguration event."}, \
   {0xD0, "Normal system/platform shutdown or powered off."}, \
   {0xE0, "Platform powered off by user without normal shutdown."}

#define SEVERITY_TYPE \
   {0x00, "Informational Event"}, \
   {0x10, "Recoverable Error"}, \
   {0x20, "Predictive Error"}, \
   {0x21, "Predicting degraded performance."}, \
   {0x22, "Predicting fault may be corrected after platform re-IPL."}, \
   {0x23, "Predicting fault may be corrected after IPL, degraded performance"}, \
   {0x24, "Predicting loss of redundancy"}, \
   {0x40, "Unrecoverable Error"}, \
   {0x41, "Error bypassed with degraded performance"}, \
   {0x44, "Error bypassed with loss of redundancy"}, \
   {0x45, "Error bypassed with loss of redundancy and performance"}, \
   {0x48, "Error bypassed with loss of function"}, \
   {0x50, "Critical Error"}, \
   {0x51, "Critical error system termination"}, \
   {0x52, "Critical error failure likely or imminent"}, \
   {0x53, "Critical error partition(s) terminal"}, \
   {0x54, "Critical error partition(s) failure likely or imminent"}, \
   {0x60, "Error on diagnostic test"}, \
   {0x61, "Error on diagnostic test, resource may produce incorrect results"}, \
   {0x70, "Symptom"}, \
   {0x71, "Symptom recovered"}, \
   {0x72, "Symptom predictive"}, \
   {0x74, "Symptom unrecoverable"}, \
   {0x75, "Symptom critical"}, \
   {0x76, "Symptom diagnosis error"}

#define CREATORS \
   {'C', "Hypervisor"}, \
   {'E', "Service Processor"}, \
   {'H', "PHYP"}, \
   {'W', "Power Control"}, \
   {'L', "Partition FW"}, \
   {'S', "SLIC"}, \
   {'B', "Hostboot"}, \
   {'T', "OCC"}, \
   {'M', "I/O Drawer"}, \
   {'K', "OPAL"}, \
   {'P', "POWERNV"}

#define SUBSYSTEMS \
   {0x00, "Unknown"}, \
   {0x10, "Processor subsystem"}, \
   {0x11, "Processor FRU"}, \
   {0x12, "Processor chip including internal cache"}, \
   {0x13, "Processor unit (CPU)"}, \
   {0x14, "Processor/system bus controller & interface"}, \
   {0x20, "Memory subsystem"}, \
   {0x21, "Memory controller"}, \
   {0x22, "Memory bus interface including SMI"}, \
   {0x23, "Memory DIMM"}, \
   {0x24, "Memory card/FRU"}, \
   {0x25, "External cache"}, \
   {0x30, "I/O subsystem"}, \
   {0x31, "I/O hub RIO"}, \
   {0x32, "I/O bridge, general (PHB, PCI/PCI, PCI/ISA, EADS, etc.)"}, \
   {0x33, "I/O bus interface"}, \
   {0x34, "I/O processor"}, \
   {0x35, "I/O hub others (SMA, Torrent, etc.)"}, \
   {0x36, "RIO loop and associated RIO hub"}, \
   {0x37, "RIO loop and associated RIO bridge"}, \
   {0x38, "PHB"}, \
   {0x39, "EADS/EADS-X global"}, \
   {0x3a, "EADS/EADS-X slot"}, \
   {0x3b, "InfiniBand hub"}, \
   {0x3c, "Infiniband bridge"}, \
   {0x40, "I/O adapter - I/O deivce - I/O peripheral"}, \
   {0x41, "I/O adapter - communication"}, \
   {0x46, "I/O device"}, \
   {0x47, "I/O device - DASD"}, \
   {0x4c, "I/O peripheral"}, \
   {0x4d, "I/O perpheral - local workstation"}, \
   {0x4e, "Storage mezzanine expansion subsystem"}, \
   {0x50, "CEC hardware"}, \
   {0x51, "CEC hardware - service processor A"}, \
   {0x52, "CEC hardware - service processor B"}, \
   {0x53, "CEC hardware - node controller"}, \
   {0x54, "Reserved for CEC hardware"}, \
   {0x55, "CEC hardware - VPD device and interface (smart chip and I2C device)"}, \
   {0x56, "CEC hardware - I2C devices and interface (non VPD)"}, \
   {0x57, "CEC hardware - CEC chip interface (JTAG, FSI, etc.)"}, \
   {0x57, "CEC hardware - CEC chip interface (JTAG, FSI, etc.)"}, \
   {0x58, "CEC hardware - clock & control"}, \
   {0x59, "CEC hardware - Op. panel"}, \
   {0x5a, "CEC hardware - time of day hardware including its battery"}, \
   {0x5b, "CEC hardware - storage/memory device (NVRAM, Flash, SP DRAM, etc.)"}, \
   {0x5c, "CEC hardware - Service processor-Hypervisor hardware interface (PSI, PCI, etc.)"}, \
   {0x5d, "CEC hardware - Service network"}, \
   {0x5e, "CEC hardware - Service processor-Hostboot hardware interface (FSI Mailbox)"}, \
   {0x60, "Power/Cooling subsystem & control"}, \
   {0x61, "Power supply"}, \
   {0x62, "Power control hardware"}, \
   {0x63, "Fan, air moving devices"}, \
   {0x64, "DPSS"}, \
   {0x70, "Others"}, \
   {0x71, "Hypervisor subsystem & hardware (excluding code)"}, \
   {0x72, "Test tool"}, \
   {0x73, "Removable media"}, \
   {0x74, "Multiple subsystems"}, \
   {0x75, "Not applicable (unknown, invalid value, etc.)"}, \
   {0x76, "Reserved"}, \
   {0x77, "CMM A"}, \
   {0x78, "CMM B"}, \
   {0x7a, "Connection Monitoring - Hypervisor lost communication with service processor"}, \
   {0x7b, "Connection Monitoring - Service processor lost communication with hypervisor"}, \
   {0x7c, "Connection Monitoring - Service processor lost communcation with hypervisor"}, \
   {0x7e, "Connection Monitoring - Hypervisor lost communication with logical partition"}, \
   {0x7e, "Connection Monitoring - Hypervisor lost communication with BPA"}, \
   {0x7f, "Connection Monitoring - Hypervisor lost communication with another hypervisor"}, \
   {0x80, "Platform firmware"}, \
   {0x81, "Service processor firmware"}, \
   {0x82, "Hypervisor firmware"}, \
   {0x83, "Partition firmware"}, \
   {0x84, "SLIC firmware"}, \
   {0x85, "SPCN firmware"}, \
   {0x86, "Bulk power formware side A"}, \
   {0x87, "Hypervisor code/firmware"}, \
   {0x88, "Bulk power firmware side B"}, \
   {0x89, "Virtual service processor firmware (VSP)"}, \
   {0x8a, "Hostboot"}, \
   {0x8b, "OCC"}, \
   {0x90, "Software"}, \
   {0x91, "Operating system software"}, \
   {0x92, "XPF software"}, \
   {0x93, "Application software"}, \
   {0xa0, "External environment"}, \
   {0xa1, "Input power source (AC)"}, \
   {0xa2, "Room ambient temperature"}, \
   {0xa3, "User error"}, \
   {0xa4, "Unknown"}, \
   {0xb0, "Unknown"}, \
   {0xc0, "Unknown"}, \
   {0xd0, "Unknown"}, \
   {0xe0, "Unknown"}, \
   {0xf0, "Unknown"}

#define EVENT_SCOPE \
   {0x1, "Single partition"}, \
   {0x2, "Multiple partitions"}, \
   {0x3, "Single platform"}, \
   {0x4, "Possibly multiple platforms"}

#define FRU_PRIORITY \
   {'L', "Lowest priority replacement"}, /*Default Value */ \
   {'H', "Mandatory, replace all with this type as a unit"}, \
   {'M', "Medium Priority"}, \
   {'A', "Medium Priority group A"}, \
   {'B', "Medium Priority group B"}, \
   {'C', "Medium Priority group C"},

#define FRU_ID_COMPONENT \
   {0x00, "Reserved"}, \
   {0x10, "Hardware FRU"}, \
   {0x20, "Code FRU"}, \
   {0x30, "Configuration error"}, \
   {0x40, "Maintenance Procedure required"}, \
   {0x90, "External FRU"}, \
   {0xa0, "External code FRU"}, \
   {0xb0, "Tool FRU"}, \
   {0xc0, "Symbolic FRU"}, \
   {0xe0, "Symbolic FRU with trusted location code"}, \
   {0xf0, "Reserved"}

#define EP_EVENT \
	{0x01, "Normal system shutdown with no additional delay"}, \
	{0x02, "Loss of utility power, system is running on UPS/batter"}, \
	{0x03, "Loss of system critical functions, system should be shutdown"}, \
	{0x04, "Ambient temperature too high"}

#define LR_RES \
	{0x10, "Processor"}, \
	{0x11, "Shared Processor"}, \
	{0x40, "Memory Page"}, \
	{0x41, "Memory LMB"}

#define IE_TYPE \
	{0x01, "Error Detected"}, \
	{0x02, "Error Recovered"}, \
	{0x03, "Event"}, \
	{0x04, "RPC Pass Through"}

#define IE_SCOPE \
	{0x00, "Not Applicable"}, \
	{0x36, "RIO Hub"}, \
	{0x37, "RIO Bridge"}, \
	{0x38, "PHB"}, \
	{0x39, "EADS global"}, \
	{0x3A, "Slot"}

#define IE_SUBTYPE \
	{0x01, "Rebalance request"}, \
	{0x03, "Node online"}, \
	{0x04, "Node offline"}, \
	{0x05, "Change platform max size"}

#define DH_TYPE \
	{0x01, "FSP Dump"}, \
	{0x02, "Platform System Dump"}, \
	{0x03, "Shared Memory Adapter Dump"}, \
	{0x04, "Power Subsystem Dump"}, \
	{0x05, "Platform Event Log Entry Dump"}, \
	{0x06, "Partition Initiated Resource Dump"}, \
	{0x07, "System Firmware Dump"},

struct generic_desc usr_hdr_event_type[] =  {
	   EVENT_TYPE
};

struct generic_desc usr_hdr_severity[] = {
	   SEVERITY_TYPE
};

struct generic_desc usr_hdr_subsystem_id[] = {
	   SUBSYSTEMS
};

struct generic_desc prv_hdr_creator_id[] = {
	   CREATORS
};

struct generic_desc usr_hdr_event_scope[] = {
	   EVENT_SCOPE
};

struct generic_desc fru_scn_priority[] = {
	   FRU_PRIORITY
};

struct generic_desc fru_id_scn_component[] = {
	   FRU_ID_COMPONENT
};

struct generic_desc ep_event[] = {
		EP_EVENT
};

struct generic_desc lr_res[] = {
	LR_RES
};

struct generic_desc ie_type[] = {
	IE_TYPE
};

struct generic_desc ie_scope[] = {
	IE_SCOPE
};

struct generic_desc ie_subtype[] = {
	IE_SUBTYPE
};

struct generic_desc dh_type[] = {
	DH_TYPE
};

int get_field_desc(struct generic_desc *data, uint8_t size, uint8_t id, uint8_t default_id)
{
   uint8_t i;
   int to_print = -1;
   for (i = 0; i < size; i++) {
      if (id == data[i].id)
         return i;
      else if(default_id == data[i].id)
         to_print = i;
   }
   return to_print;
}

const char *get_event_desc(uint8_t id)
{
	int to_print = get_field_desc(usr_hdr_event_type, MAX_EVENT, id, id);
	if (to_print != -1)
		return usr_hdr_event_type[to_print].desc;
	return "Unknown";
}

const char *get_subsystem_name(uint8_t id)
{
	int to_print = get_field_desc(usr_hdr_subsystem_id, MAX_SUBSYSTEMS, id, id & 0xF0);
	if (to_print != -1)
		return usr_hdr_subsystem_id[to_print].desc;
	return usr_hdr_subsystem_id[0].desc; /* Default value */
}

const char *get_severity_desc(uint8_t id)
{
	int to_print = get_field_desc(usr_hdr_severity, MAX_SEV, id, id & 0xF0);
	if (to_print != -1)
		return usr_hdr_severity[to_print].desc;
	return usr_hdr_severity[0].desc; /* Default value */
}

const char *get_creator_name(uint8_t id)
{
	int to_print = get_field_desc(prv_hdr_creator_id, MAX_CREATORS, id, id);
	if (to_print != -1)
		return prv_hdr_creator_id[to_print].desc;
	return "Unknown";
}

const char *get_event_scope(uint8_t id)
{
	int to_print = get_field_desc(usr_hdr_event_scope, MAX_EVENT_SCOPE, id, id);
	if (to_print != -1)
		return usr_hdr_event_scope[to_print].desc;
	return "Unknown";
}

const char *get_fru_component_desc(uint8_t id)
{
	int to_print = get_field_desc(fru_id_scn_component, MAX_FRU_ID_COMPONENT, id, 0);
	if (to_print != -1)
	return fru_id_scn_component[to_print].desc;
		return "Unknown";
}

const char *get_fru_priority_desc(uint8_t id)
{
	int to_print = get_field_desc(fru_scn_priority, MAX_FRU_PRIORITY, id, 'L');
	if (to_print != -1)
		return fru_scn_priority[to_print].desc;
	return "Unknown";
}

const char *get_ep_event_desc(uint8_t id)
{
	int to_print = get_field_desc(ep_event, MAX_EP_EVENT, id, id);
	if (to_print != -1)
		return ep_event[to_print].desc;
	return "Unknown";
}

const char *get_lr_res_desc(uint8_t id)
{
	int to_print = get_field_desc(lr_res, MAX_LR_RES, id, id);
	if (to_print != -1)
		return lr_res[to_print].desc;
	return "Unknown";
}

const char *get_ie_type_desc(uint8_t id)
{
	int to_print = get_field_desc(ie_type, MAX_IE_TYPE, id, id);
	if (to_print != -1)
		return ie_type[to_print].desc;
	return "Unknown";
}

const char *get_ie_scope_desc(uint8_t id)
{
	int to_print = get_field_desc(ie_scope, MAX_IE_SCOPE, id, id);
	if (to_print != -1)
		return ie_scope[to_print].desc;
	return "Unknown";
}

const char *get_ie_subtype_desc(uint8_t id)
{
	int to_print = get_field_desc(ie_subtype, MAX_IE_SUBTYPE, id, id);
	if (to_print != -1)
		return ie_subtype[to_print].desc;
	return "Unknown";
}

const char *get_dh_type_desc(uint8_t id)
{
	int to_print = get_field_desc(dh_type, MAX_DH_TYPE, id, id);
	if (to_print != -1)
		return dh_type[to_print].desc;
	return "Unknown";
}
