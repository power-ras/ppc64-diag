/**
 * @file dchrp.h
 *
 * Copyright (C) 2005 IBM Corporation
 */

#ifndef _H_DCHRP
#define _H_DCHRP

/*
 * Error log identifiers that are built as follows:
 *
 * bit number:
 *	3322222222221111111111
 *      10987654321098765432109876543210
 *      --------------------------------
 *      0000
 *	    ----
 *           |	--------
 *           |     |
 *           |     |	--------
 *           |     |       |	--------
 *	     |	   |	   |	   |
 *	     |	   |	   |	   if cpu, mem, post or io format 
 *           |     |       |       	extended log byte 12 
 *           |     |       |       if epow format 
 *           |     |       |       	extended log byte 15 
 *	     |     |       |       if sp format 
 *           |     |       |            extended log byte 16
 *	     |	   |	   |	   
 *	     |	   |	   if mem, post or io format 
 *	     |	   |	   	extended log byte 13
 *	     |     |       if sp format 
 *           |     |            extended log byte 17
 *	     |     |       if epow format
 *           |     |            extended log byte 16
 *	     |	   |	  	   
 *	     |     if sp format 
 *	     |	   	extended log byte 18
 *	     |     if epow format
 *           |          extended log byte 17
 *	     |
 *	     extended log byte 2, bits 4:7
 *
 *	
 * and each ID is labeled xxxByybz where xxx= format,yy=byte #,z=bit # 
 *     and Byybz is repeated as often as necessary
 *
 */
#define CPUB12b0	0x01000080
#define CPUB12b1	0x01000040
#define CPUB12b2	0x01000020
#define CPUB12b3	0x01000010
#define CPUB12b4	0x01000008
#define CPUB12b5	0x01000004
#define CPUB12b6	0x01000002
#define CPUB12b7	0x01000001
#define CPUALLZERO	0x01000000
#define MEMB12b0	0x02000080
#define MEMB12b1	0x02000040
#define MEMB12b2	0x02000020
#define MEMB12b3	0x02000010
#define MEMB12b4	0x02000008
#define MEMB12b4B13b3	0x02001008
#define MEMB12b5	0x02000004
#define MEMB12b6	0x02000002
#define MEMB12b7	0x02000001
#define MEMB13b0	0x02008000
#define MEMB13b1	0x02004000
#define MEMB13b2	0x02002000
#define MEMB13b3	0x02001000
#define MEMB13b4	0x02000800
#define MEMALLZERO	0x02000000
#define IOB12b0		0x03000080
#define IOB12b1		0x03000040
#define IOB12b2		0x03000020
#define IOB12b3		0x03000010
#define IOB12b3B13b2	0x03002010
#define IOB12b4		0x03000008
#define IOB12b5		0x03000004
#define IOB12b6		0x03000002
#define IOB12b7		0x03000001
#define IOB12b5B13b0	0x03008004
#define IOB12b6B13b0	0x03008002
#define IOB12b7B13b0	0x03008001
#define IOB12b5B13b1	0x03004004
#define IOB12b6B13b1	0x03004002
#define IOB12b7B13b1	0x03004001
#define IOB12b5B13b2	0x03002004
#define IOB12b6B13b2	0x03002002
#define IOB12b7B13b2	0x03002001
#define IOB12b6B13b3	0x03001002
#define IOB13b4		0x03000800
#define IOB13b5		0x03000400
#define IOB13b6		0x03000200
#define IOB13b7		0x03000100
#define IOALLZERO       0x03000000
#define POSTALLZERO	0x04000000
#define POSTB12b0	0x04000080
#define POSTB12b1 	0x04000040
#define POSTB12b2	0x04000020
#define POSTB12b3	0x04000010
#define POSTB12b4	0x04000008
#define POSTB12b5	0x04000004
#define POSTB12b6	0x04000002
#define POSTB12b7	0x04000001
#define POSTB13b0	0x04008000
#define POSTB13b1	0x04004000
#define POSTB13b2	0x04002000
#define POSTB13b3	0x04001000
#define POSTB13b4	0x04000800
#define POSTB13b5	0x04000400
#define POSTB13b7	0x04000100
#define EPOWB1501	0x05000001
#define EPOWB1501B16b2b4	0x05002801
#define EPOWB1502	0x05000002
#define EPOWB1502B16b4	0x05000802
#define EPOWB1503	0x05000003
#define EPOWB1503B16b23	0x05003003
#define EPOWB1503B16b3	0x05001003
#define EPOWB1504	0x05000004
#define EPOWB1505	0x05000005
#define EPOWB1505B16b1B17b1	0x05404005
#define EPOWB1505B16b1B17b2	0x05204005
#define EPOWB1505B16b1B17b3	0x05104005
#define EPOWB1502B16b4B17b2	0x05200802
#define EPOWB1502B16b1b4B17b2	0x05204802
#define EPOWB1507	0x05000007
#define SPB16b0		0x0D000080
#define SPB16b1		0x0D000040
#define SPB16b2		0x0D000020
#define SPB16b3		0x0D000010
#define SPB16b4		0x0D000008
#define SPB16b5		0x0D000004
#define SPB16b6		0x0D000002
#define SPB16b7		0x0D000001
#define SPB17b0		0x0D008000
#define SPB17b1		0x0D004000
#define SPB17b2		0x0D002000
#define SPB17b3		0x0D001000
#define SPB17b4		0x0D000800
#define SPB17b5		0x0D000400
#define SPB17b6		0x0D000200
#define SPB17b7		0x0D000100
#define SPB18b0		0x0D800000
#define SPB18b1		0x0D400000
#define SPB18b2		0x0D200000
#define SPB18b3		0x0D100000
#define SPB18b4		0x0D080000
#define SPB18b5		0x0D040000
#define SPB18b6		0x0D020000
#define SPB18b7		0x0D010000
#define EPOWLOGB16b0	0x05008000
#define LOGB16b4	0x00000800	

#define EPOWLOG		0x05000000
#define SPLOG		0x0D000000

/*
 * Bits analyzed in Version 2 EPOW logs.
 */
#define EPOWB16b0	0x00008000
#define EPOWB16b1	0x00004000
#define EPOWB16b2	0x00002000
#define EPOWB16b3	0x00001000
#define EPOWB16b2b3	0x00003000
#define EPOWB16b4	0x00000800
#define EPOWB17b0	0x00800000
#define EPOWB17b1	0x00400000
#define EPOWB17b2	0x00200000
#define EPOWB17b3	0x00100000
#define EPOWB17b4	0x00080000

/* 
 * Extened EPOW codes, found after the location codes. The sensor nibble 
 * is ignored.
 */
#define XEPOW1n11	0x1011
#define XEPOW1n64	0x1064
#define XEPOW2n32	0x2032
#define XEPOW2n52	0x2052
#define XEPOW3n21	0x3021
#define XEPOW3n73	0x3073

#define IGNORE_SENSOR_MASK 0xF0FF

/*
 * PCI EPOW Register values for bits 13:15
 */
#define PCIEPOW111	0x00070000
#define PCIEPOW100	0x00040000
#define PCIEPOW011	0x00030000
#define PCIEPOW010	0x00020000
#define PCIEPOW001	0x00010000

#define PCIEPOWMASK	0x00070000


#define CRITHI 13
#define CRITLO 9
#define WARNHI 12
#define WARNLO 10	
#define NORMAL 11
#define GS_SUCCESS 0

#define THERM 3
#define POWER 9004
#define VOLT 9002
#define FAN 9001

/* 
 * Offsets into the chrp error log
 */
#define I_EXTENDED      8       /* index to extended error log */
#define I_BYTE0         0 + I_EXTENDED  /* contains predicative error bit */
#define I_BYTE1         1 + I_EXTENDED  /* has platform specific error bit */
#define I_FORMAT        2 + I_EXTENDED  /* to log format indicator */
#define I_BYTE3         3 + I_EXTENDED  /* has modifier bits */
#define I_BYTE12        12+ I_EXTENDED  /* to bytes describing error, where */
#define I_BYTE13        13+ I_EXTENDED  /* most formats use bytes 12 & 13 */
#define I_BYTE15        15+ I_EXTENDED  /* for epow errors */
#define I_BYTE16        16+ I_EXTENDED  /* for sp errors */
#define I_BYTE17        17+ I_EXTENDED  /* for sp errors */
#define I_BYTE18        18+ I_EXTENDED  /* for sp errors */
#define I_BYTE19        19+ I_EXTENDED  /* for sp errors */
#define I_BYTE24        24+ I_EXTENDED  /* for repair pending bit */
#define I_BYTE28        28+ I_EXTENDED  /* for sp errors */
#define I_CPU           13+ I_EXTENDED  /* to for physical cpu number */
#define I_POSTCODE      26+ I_EXTENDED  /* to post error code */
#define I_FWREV         30+ I_EXTENDED  /* to firmware revision level */
#define I_IBM           40+ I_EXTENDED  /* to IBM id for location codes */
#define I_TOKEN         20+I_EXTENDED
#define I_INDEX         24+I_EXTENDED
#define I_STATUS        32+I_EXTENDED

#define IGNORE_SENSOR_MASK      0xF0FF  /* for extended epow, a 2 byte value */
#define SENSOR_MASK             0x0F00
#define EXT_EPOW_REG_ID         0x3031  /* ascii for 01 */
#define SRC_REG_ID_02           0x3032  /* Ascii for 02 */
#define PCI_EPOW_REG_ID         0x3033
#define SRC_REG_ID_04           0x3034  /* Ascii for 04 */

/* Structure to describe i/o device from i/o detected chrp log */
struct device_ela {
        int status;             /* 0=no data;1=bus only;2=device ok */
        char bus;
        char devfunc;
        short deviceid;
        short vendorid;
        char revisionid;
        char slot;
        char name[NAMESIZE];
        char busname[NAMESIZE];
        char loc[LOCSIZE];
        short led;
};
#define DEVICE_NONE     0
#define DEVICE_BUS      1
#define DEVICE_OK       2

#define FIRST_LOC       1       /* mode for first location code */
#define NEXT_LOC        2       /* mode for next location code */
#define FIRST_REG       1       /* mode for first register data */
#define NEXT_REG        2       /* mode for next register data */

#define LOC_HIDE_CHAR	'>'	/* special meaning in location code buffer */

/* Return codes from get_cpu_frus */
#define RC_INVALID	0
#define RC_PLANAR       1
#define RC_PLANAR_CPU   2
#define RC_PLANAR_2CPU	3

#define MAXREFCODES     5
#define REFCODE_REASON_CUST     0x880

/*
 * The following name is a flag for the diag controller to format
 * the error description into a "special" SRN, i.e. containing refcodes
 */
#define REFCODE_FNAME   "REF-CODE"

#define MAX_MENUGOAL_SIZE	2000

#endif
