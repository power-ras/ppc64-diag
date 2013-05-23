/**
 * @file dchrp_frus.h
 *
 * Copyright (C) 2005, 2008 IBM Corporation
 */

#ifndef _H_DCHRP_FRUS
#define _H_DCHRP_FRUS

#include "ela_msg.h"

/*
 * In the following #defines, x is the probability (confidence) that this
 * callout represents the actual failure
 */
#define FRU_MEMORY_CONTROLLER(x)	{ x, "", " ", 1}
#define FRU_PROCESSOR(x)		{ x, "procx", " ", 127}
#define FRU_L2CACHE(x)			{ x, "L2Cache0", " ", 33}
#define FRU_MISSING_L2(x)		{ x, "L2Cache0", " ", 33}
#define FRU_HOST_BRIDGE(x)		{ x, "", " ", 2} 
#define FRU_SYSTEM_BUS_CONNECTOR(x)	{ x, "", " ", 192}
#define FRU_MEMORY_MODULE(x)		{ x, "Memory module", " ", 4}
#define FRU_MEMORY_CARD(x)		{ x, "", " ", 5}
#define FRU_MEZZANINE_BUS(x)		{ x, "", " ", 7}
#define FRU_PCI_DEVICE(x)		{ x, "", " ", 3}
#define	FRU_PCI_BUS(x)			{ x, "", " ", 192}
#define	FRU_ISA_DEVICE(x)		{ x, "", " ", 8}
#define	FRU_ISA_BRIDGE(x)		{ x, "", " ", 9}
#define	FRU_ISA_BUS(x)			{ x, "", " ", 10}
#define FRU_MEZZANINE_BUS_ARBITER(x)	{ x, "", " ", 11}
#define FRU_SERVICE_PROCESSOR(x)	{ x, "", " ", 12}
#define FRU_SP_SYSTEM_INTERFACE(x)	{ x, "", " ", 13}
#define FRU_SP_PRIMARY_BUS(x)		{ x, "", " ", 14}
#define FRU_SP_SECONDARY_BUS(x)		{ x, "", " ", 15}
#define FRU_VPD_MODULE(x)		{ x, "", " ", 16}
#define FRU_POWER_CONTROLLER(x)		{ x, "", " ", 18}
#define FRU_FAN_SENSOR(x)		{ x, "Fan Sensor", " ", 19}
#define FRU_THERMAL_SENSOR(x)		{ x, "Sensor", " ", 20}
#define FRU_VOLTAGE_SENSOR(x)		{ x, "Sensor", " ", 21}
#define FRU_POWER_SENSOR(x)		{ x, "sensor", " ", 179}
#define FRU_SERIAL_PORT_CONTROLLER(x)	{ x, "", " ", 22}
#define FRU_NVRAM(x)			{ x, "", " ", 23}
#define FRU_RTC_TOD(x)			{ x, "", " ", 24}
#define FRU_JTAG(x)			{ x, "", " ", 25}
#define FRU_SOFTWARE(x)			{ x, "", " ", 26}
#define FRU_HARDWARE(x)			{ x, "", " ", 27}
#define FRU_FAN(x)			{ x, "Fan", " ", 28}
#define FRU_POWER_SUPPLY(x)		{ x, "", " ", 29}
#define FRU_BATTERY(x)			{ x, "", " ", 30}
#define FRU_IO_EXPANSION_UNIT(x)	{ x, "", " ", 128}
#define FRU_IO_EXPANSION_BUS(x)		{ x, "", " ", 129}
#define FRU_34_POWER_SUPPLY(x)		{ x, "", " ", 130}
#define FRU_14_POWER_SUPPLY(x)		{ x, "", " ", 131}
#define FRU_RIO_BRIDGE(x)		{ x, "", " ", 185}
#define FRU_RTASFRU(x)			{ x, "", " ", 192}
#define FRU_OPERATOR_PANEL(x)		{ x, "", " ", 213}
#define FRU_IO_PLANAR(x)		{ x, "", " ", 31}

/*
 * If there are two entries for a given failure, the first should be
 * used in the case of an actual failure, and the second should be used
 * in the case of a predictive failure (i.e. the repair can be deferred)
 *
 * Some of the io??? failures have three entries.
 */

struct event_description_pre_v6 cpu610[] = 	/* CPU internal error */
{	
	{ "", ERRD1, 0x651, 0x610, MSGCPUB12b0, 
	    {
		FRU_PROCESSOR(100)
	    }
	},
	{ "", ERRD1, 0x652, 0x610, DEFER_MSGCPUB12b0, 
	    {
		FRU_PROCESSOR(100)
	    }
	}
};

struct event_description_pre_v6 cpu611[] =  	/* CPU internal cache error */
{	
	{ "", ERRD1, 0x651, 0x611, MSGCPUB12b1,
	    {
		FRU_PROCESSOR(100)
	    }
	},
	{ "", ERRD1, 0x652, 0x611, DEFER_MSGCPUB12b1,
	    {
		FRU_PROCESSOR(100)
	    }
	}
};

struct event_description_pre_v6 cpu612[] =
				/* L2 cache parity or mult-bit ecc error */
{	
	{ "", ERRD1, 0x651, 0x612, MSGCPUB12b2, 
	    {
		FRU_L2CACHE(100)
	    }
	},
	{ "", ERRD1, 0x652, 0x612, DEFER_MSGCPUB12b2, 
	    {
		FRU_L2CACHE(100)
	    }
	}
};

struct event_description_pre_v6 cpu613[] = /* L2 cache ECC single-bit error */
{
	{ "", ERRD1, 0x651, 0x613, MSGCPUB12b3,
	    {
		FRU_L2CACHE(100)
	    }
	},
	{ "", ERRD1, 0x652, 0x613, DEFER_MSGCPUB12b3,
	    {
		FRU_L2CACHE(100)
	    }
	}
};

struct event_description_pre_v6 cpu614[] =
				/* Time-out error waiting for mem controller */
{
	{ "", ERRD1, 0x651, 0x614, MSGCPUB12b4, 
	    {
		FRU_MEMORY_CONTROLLER(100)
	    }
	}
};

struct event_description_pre_v6 cpu615[] = /* Time-out error waiting for I/O */
{
	{ "", ERRD1, 0x651, 0x615, MSGCPUB12b5, 
	    {
		FRU_HOST_BRIDGE(100)	
	    }
	}
};

struct event_description_pre_v6 cpu619[] = /* Unknown error in CPU Error log */
{
	{ "", ERRD1, 0x651, 0x619, MSGCPUALLZERO, 
	    {
		{ 0, "", "", 36},
		{ 0, "", "", 36},
		{ 0, "", "", 36},
		{ 0, "", "", 36}
	    }
	}
};

struct event_description_pre_v6 cpu710[] =
				/* Address/Data parity error on Processor bus */
{
	{ "", ERRD1, 0x651, 0x710, MSGCPUB12b6, 
	    {
		FRU_RTASFRU(60), 
		FRU_SYSTEM_BUS_CONNECTOR(40) 	
	    }
	}
};

struct event_description_pre_v6 cpu711[] =
				/* Address/Data parity error on Processor bus */
{
	{ "", ERRD1, 0x651, 0x711, MSGCPUB12b6, 
	    {
		FRU_RTASFRU(40), 
		FRU_RTASFRU(35), 
		FRU_SYSTEM_BUS_CONNECTOR(25)
	    } 
	}
};

struct event_description_pre_v6 cpu712[] =
				/* Address/Data parity error on Processor bus */
{
	{ "", ERRD1, 0x651, 0x712, MSGCPUB12b6, 
	    {
		FRU_RTASFRU(40), 
		FRU_RTASFRU(25), 
		FRU_RTASFRU(25), 
		FRU_SYSTEM_BUS_CONNECTOR(10)
	    }
	}
};

struct event_description_pre_v6 cpu713[] = /* Transfer error on Processor bus */
{
	{ "", ERRD1, 0x651, 0x713, MSGCPUB12b7, 
	    {
		FRU_RTASFRU(60), 
		FRU_SYSTEM_BUS_CONNECTOR(40) 	
	    }
	}
};

struct event_description_pre_v6 cpu714[] = /* Transfer error on Processor bus */
{
	{ "", ERRD1, 0x651, 0x714, MSGCPUB12b7, 
	    {
		FRU_RTASFRU(40), 
		FRU_RTASFRU(35), 
		FRU_SYSTEM_BUS_CONNECTOR(25)
	    }
	}
};

struct event_description_pre_v6 cpu715[] = /* Transfer error on Processor bus */
{
	{ "", ERRD1, 0x651, 0x715, MSGCPUB12b7, 
	    {
		FRU_RTASFRU(40), 
		FRU_RTASFRU(25), 
		FRU_RTASFRU(25), 
		FRU_SYSTEM_BUS_CONNECTOR(10)
	    }
	}
};

struct event_description_pre_v6 mem720[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x720, MSGMEMB12b0, 
	    {
		FRU_MEMORY_MODULE(85),
		FRU_MEMORY_CARD(10),
		FRU_MEMORY_CONTROLLER(5)
	    }
	}
};

struct event_description_pre_v6 mem721[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x721, MSGMEMB12b0, 
	    {
		FRU_MEMORY_MODULE(85),
		FRU_MEMORY_CARD(10),
		FRU_MEMORY_CONTROLLER(5)
	    }
	}
};

struct event_description_pre_v6 mem780[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x780, MSGMEMB12b0, 
	    {
		FRU_MEMORY_MODULE(85),
		FRU_MEMORY_CARD(10),
		FRU_MEMORY_CONTROLLER(5)
	    }
	}
};

struct event_description_pre_v6 mem781[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x781, MSGMEMB12b0, 
	    {
		FRU_MEMORY_MODULE(85),
		FRU_MEMORY_CARD(10),
		FRU_MEMORY_CONTROLLER(5)
	    }
	}
};

struct event_description_pre_v6 mem782[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x782, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem783[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x783, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem784[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x784, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem785[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x785, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem786[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x786, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem787[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x787, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem788[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x788, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem789[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x789, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem78A[] = 	/* Uncorrectable Memory Error */
{
	{ "", ERRD1, 0x651, 0x78A, MSGMEMB12b0, 
	    {
		FRU_MEMORY_CARD(90),
		FRU_MEMORY_CONTROLLER(10)
	    }
	}
};

struct event_description_pre_v6 mem78B[] =    /* Uncorrectable Memory Error */
{
        { "", ERRD1, 0x651, 0x78B, MSGMEMB12b0,
            {
               FRU_MEMORY_CARD(90),
               FRU_MEMORY_CONTROLLER(10)
            }
        }
};

struct event_description_pre_v6 mem78C[] =    /* Uncorrectable Memory Error */
{
        { "", ERRD1, 0x651, 0x78C, MSGMEMB12b0,
            {
               FRU_MEMORY_CARD(90),
               FRU_MEMORY_CONTROLLER(10)
            }
        }
};

struct event_description_pre_v6 mem620[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x620, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem621[] = /* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x621, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem622[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x622, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x622, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem623[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x623, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x623, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem624[] =
				/* Memory Controller internal error */
{
	{ "", ERRD1, 0x651, 0x624, MSGMEMB12b3, 
	    {
		FRU_MEMORY_CONTROLLER(100) 
	    }
	}
};

struct event_description_pre_v6 mem625[] =
				/* Memory Address (Bad address to memory) */
{
	{ "", ERRD1, 0x651, 0x625, MSGMEMB12b4, 
	    {
		FRU_MEMORY_CONTROLLER(100) 
	    }
	}
};

struct event_description_pre_v6 mem626[] =
				/* Memory Data error (Bad data to memory) */
{
	{ "", ERRD1, 0x651, 0x626, MSGMEMB12b5, 
	    {
		FRU_MEMORY_CONTROLLER(100) 
	    }
	}
};

struct event_description_pre_v6 mem627[] = 	/* Memory time-out error */
{
	{ "", ERRD1, 0x651, 0x627, MSGMEMB12b7, 
	    {
		FRU_MEMORY_CONTROLLER(100) 
	    }
	}
};

struct event_description_pre_v6 mem628[] = 	/* Processor time-out error */
{
	{ "", ERRD1, 0x651, 0x628, MSGMEMB13b1, 
	    {
		FRU_PROCESSOR(100) 
	    }
	}
};

struct event_description_pre_v6 mem629[] =
				/* Unknown error detected by mem. controller */
{
	{ "", ERRD1, 0x651, 0x629, MSGMEMALLZERO, 
	    {
		{ 0, "", "", 36},
		{ 0, "", "", 36},
		{ 0, "", "", 36},
		{ 0, "", "", 36}
	    }
	}
};

struct event_description_pre_v6 io630[] =
				/* I/O Expansion Bus Parity Error */
{
	{ "", ERRD1, 0x651, 0x630, MSGIOB13b4, 
	    {
		FRU_IO_EXPANSION_UNIT(100) 
	    }
	},
	{ "", ERRD1, 0x652, 0x630, DEFER_MSGIOB13b4, 
	    {
		FRU_IO_EXPANSION_UNIT(100) 
	    }
	}
};

struct event_description_pre_v6 io631[] =
				/* I/O Expansion Bus Time-out Error */
{
	{ "", ERRD1, 0x651, 0x631, MSGIOB13b5, 
	    {
		FRU_IO_EXPANSION_UNIT(100) 
	    }
	},
	{ "", ERRD1, 0x652, 0x631, DEFER_MSGIOB13b5, 
	    {
		FRU_IO_EXPANSION_UNIT(100) 
	    }
	}
};

struct event_description_pre_v6 io632[] =
				/* I/O Expansion Bus Connect Failure */ 
{ 
	{ "", ERRD1, 0x651, 0x632, MSGIOB13b6, 
	    {
		FRU_IO_EXPANSION_BUS(70), 
		FRU_IO_EXPANSION_UNIT(20), 
		FRU_RIO_BRIDGE(10)
	    }
	},
	{ "", ERRD1, 0x652, 0x632, DEFER_MSGIOB13b6, 
	    {
		FRU_IO_EXPANSION_BUS(70), 
		FRU_IO_EXPANSION_UNIT(20), 
		FRU_RIO_BRIDGE(10)
	    }
	}
};

struct event_description_pre_v6 io633[] =
				/* I/O Expansion Unit not in operationg state.*/
{
	{ "", ERRD1, 0x651, 0x633, MSGIOB13b7, 
	    {
		FRU_IO_EXPANSION_UNIT(70), 
		FRU_IO_EXPANSION_BUS(30) 
	    }
	},
	{ "", ERRD1, 0x652, 0x633, DEFER_MSGIOB13b7, 
	    {
		FRU_IO_EXPANSION_UNIT(70), 
		FRU_IO_EXPANSION_BUS(30) 
	    }
	}
};

struct event_description_pre_v6 io634[] =
				/* Int Error, bridge conn. to i/o exp. bus */ 
{
	{ "", ERRD1, 0x651, 0x634, MSGIOB12b3, 
	    {
		FRU_IO_EXPANSION_UNIT(100) 
	    }
	},
	{ "", ERRD1, 0x652, 0x634, DEFER_MSGIOB12b3, 
	    {
		FRU_IO_EXPANSION_UNIT(100) 
	    }
	}
};

struct event_description_pre_v6 mem650[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x650, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem651[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x651, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem652[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x652, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem653[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x653, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem654[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x654, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem655[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x655, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem656[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x656, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem657[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x657, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem658[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x658, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem659[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x659, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem65A[] =	/* ECC correctable error */
{
	{ "", ERRD1, 0x651, 0x65A, MSGMEMB12b1, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem65B[] =    /* ECC correctable error */
{
        { "", ERRD1, 0x651, 0x65B, MSGMEMB12b1,
            {
                FRU_MEMORY_MODULE(100), /* Pct to be adjusted later */
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0)
            }
        }
};

struct event_description_pre_v6 mem65C[] =    /* ECC correctable error */
{
        { "", ERRD1, 0x651, 0x65C, MSGMEMB12b1,
            {
                FRU_MEMORY_MODULE(100), /* Pct to be adjusted later */
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0)
            }
        }
};

struct event_description_pre_v6 mem660[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x660, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x660, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem661[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x661, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x661, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem662[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x662, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x662, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem663[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x663, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x663, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem664[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x664, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x664, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem665[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x665, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x665, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem666[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x666, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x666, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem667[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x667, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x667, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem668[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x668, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x668, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem669[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x669, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x669, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem66A[] =
				/* Correctable error threshold exceeded */
{
	{ "", ERRD1, 0x651, 0x66A, MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	},
	{ "", ERRD1, 0x652, 0x66A, DEFER_MSGMEMB12b2, 
	    {
		FRU_MEMORY_MODULE(100),	/* Pct to be adjusted later */
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0),
		FRU_MEMORY_MODULE(0)
	    }
	}
};

struct event_description_pre_v6 mem66B[] =
				/* Correctable error threshold exceeded */
{
        { "", ERRD1, 0x651, 0x66B, MSGMEMB12b2,
            {
                FRU_MEMORY_MODULE(100), /* Pct to be adjusted later */
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0)
            }
        },
        { "", ERRD1, 0x652, 0x66B, DEFER_MSGMEMB12b2,
            {
                FRU_MEMORY_MODULE(100), /* Pct to be adjusted later */
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0)
            }
        }
};

struct event_description_pre_v6 mem66C[] =
				/* Correctable error threshold exceeded */
{
        { "", ERRD1, 0x651, 0x66C, MSGMEMB12b2,
            {
                FRU_MEMORY_MODULE(100), /* Pct to be adjusted later */
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0)
            }
        },
        { "", ERRD1, 0x652, 0x66C, DEFER_MSGMEMB12b2,
            {
                FRU_MEMORY_MODULE(100), /* Pct to be adjusted later */
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0),
                FRU_MEMORY_MODULE(0)
            }
        }
};

struct event_description_pre_v6 memtest670[] =	/* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x670, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(90),
		FRU_MEMORY_CARD(10)
	    }
	}
};

struct event_description_pre_v6 memtest671[] =	/* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x671, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(90),
		FRU_MEMORY_CARD(10)
	    }
	}
};

struct event_description_pre_v6 memtest672[] =	/* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x672, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(100)
	    }
	}
};

struct event_description_pre_v6 memtest673[] =	/* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x673, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(100)
	    }
	}
};

struct event_description_pre_v6 memtest674[] =	/* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x674, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(100)
	    }
	}
};

struct event_description_pre_v6 memtest675[] =	/* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x675, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(100)
	    }
	}
};

struct event_description_pre_v6 memtest676[] =	/* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x676, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(100)
	    }
	}
};

struct event_description_pre_v6 memtest677[] =	/* Missing or bad memory */ 
{
	{ "", ERRD1, 0x651, 0x677, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(100)
	    }
	}
};

struct event_description_pre_v6 memtest678[] =	/* Missing or bad memory */ 
{
	{ "", ERRD1, 0x651, 0x678, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(100)
	    }
	}
};

struct event_description_pre_v6 memtest679[] =	/* Missing or bad memory */ 
{
	{ "", ERRD1, 0x651, 0x679, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(90),
		FRU_MEMORY_CARD(10)
	    }
	}
};

struct event_description_pre_v6 memtest67A[] =	/* Missing or bad memory */ 
{
	{ "", ERRD1, 0x651, 0x67A, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(90),
		FRU_MEMORY_CARD(10)
	    }
	}
};

struct event_description_pre_v6 mem67B[] =    /* Failed memory module */
{
        { "", ERRD1, 0x651, 0x67B, MSG_MISSING_MEM,
            {
               FRU_MEMORY_MODULE(90),
               FRU_MEMORY_CARD(10)
            }
        }
};

struct event_description_pre_v6 mem67C[] =    /* Failed memory module */
{
        { "", ERRD1, 0x651, 0x67C, MSG_MISSING_MEM,
            {
               FRU_MEMORY_MODULE(90),
               FRU_MEMORY_CARD(10)
            }
        }
};

struct event_description_pre_v6 mem722[] = 	/* Processor Bus parity error */
{
	{ "", ERRD1, 0x651, 0x722, MSGMEMB13b0, 
	    {
		FRU_PROCESSOR(40),
		FRU_SYSTEM_BUS_CONNECTOR(35),
		FRU_MEMORY_CONTROLLER(25) 
	    }
	}
};

struct event_description_pre_v6 mem723[] = /* Processor bus Transfer error */
{
	{ "", ERRD1, 0x651, 0x723, MSGMEMB13b2, 
	    {
		FRU_PROCESSOR(40),
		FRU_SYSTEM_BUS_CONNECTOR(35),
		FRU_MEMORY_CONTROLLER(25) 
	    }
	}
};

struct event_description_pre_v6 mem724[] = /* I/O Host Bridge time-out error */
{
	{ "", ERRD1, 0x651, 0x724, MSGMEMB13b3, 
	    {
		FRU_HOST_BRIDGE(40), 
		FRU_MEZZANINE_BUS(35),
		FRU_MEMORY_CONTROLLER(25) 
	    }
	}
};

struct event_description_pre_v6 mem725[] =
				/* I/O Host Bridge address/data parity error */
{
	{ "", ERRD1, 0x651, 0x725, MSGMEMB13b4, 
	    {
		FRU_HOST_BRIDGE(40), 
		FRU_MEZZANINE_BUS(35),
		FRU_MEMORY_CONTROLLER(25) 
	    }
	}
};

struct event_description_pre_v6 mem726[] =
				/* I/O Host Bridge and Bad Memory Address */
{
	{ "", ERRD1, 0x651, 0x726, MSGMEMB12b4B13b3, 
	}
};

struct event_description_pre_v6 io833[] =	
{
	{ "", ERRD1, 0x833, 0x0, MSGIOB12b0, /* I/O Bus Address Parity Error */
	    {
		{ 40, "", " ", 192},	
			/* the fru name will be inserted, as will the */
			/* reason code, which will be the led of the device */
		FRU_PCI_DEVICE(35), 
		FRU_PCI_BUS(25)
	    }
	},
	{ "", ERRD1, 0x833, 0x0, MSGIOB12b1, /* I/O Bus Data Parity Error */
	    {
		{ 40, "", " ", 192},	
			/* the fru name will be inserted, as will the */
			/* reason code, which will be the led of the device */
		FRU_PCI_DEVICE(35),
		FRU_PCI_BUS(25)
	    }
	},
	{ "", ERRD1, 0x833, 0x0, MSGIOB12b2, /* I/O Time-out error */
	    {
		{ 40, "", " ", 192},	
			/* the fru name will be inserted, as will the */
			/* reason code, which will be the led of the device */
		FRU_PCI_DEVICE(35),
		FRU_PCI_BUS(25)
	    }
	}
};

#define SN9CC	0x9aa		/* 9CC was assigned, so internally I must */
				/* use 9AA. The DIMB should read 9CC-xxx  */

struct event_description_pre_v6 io9CC[] =	
{
	{ "", ERRD1, SN9CC, 0x0, MSGIOB12b0, /* I/O Bus Address Parity Error */
	    {
		{ 60, "", " ", 192},	
			/* the fru name will be inserted, as will the */
			/* reason code, which will be the led of the device */
		FRU_PCI_BUS(40)
	    }
	},
	{ "", ERRD1, SN9CC, 0x0, MSGIOB12b1, /* I/O Bus Data Parity Error */
	    {
		{ 60, "", " ", 192},	
			/* the fru name will be inserted, as will the */
			/* reason code, which will be the led of the device */
		FRU_PCI_BUS(40)
	    }
	},
	{ "", ERRD1, SN9CC, 0x0, MSGIOB12b2, /* I/O Time-out error */
	    {
		{ 60, "", " ", 192},	
			/* the fru name will be inserted, as will the */
			/* reason code, which will be the led of the device */
		FRU_PCI_BUS(40)
	    }
	}
};

struct event_description_pre_v6 iobusonly[] =	
{
	{ "", ERRD1, SN9CC, 0x0, MSGIOB12b0, /* I/O Bus Address Parity Error */
	    {
		FRU_PCI_BUS(100)
	    }
	},
	{ "", ERRD1, SN9CC, 0x0, MSGIOB12b1, /* I/O Bus Data Parity Error */
	    {
		FRU_PCI_BUS(100)
	    }
	},
	{ "", ERRD1, SN9CC, 0x0, MSGIOB12b2, /* I/O Time-out error */
	    {
		FRU_PCI_BUS(100)
	    }
	}
};

struct event_description_pre_v6 io832[] =	/* I/O Device Internal Error */
{
	{ "", ERRD1, 0x832, 0x0, MSGIOB12b3,
	    {
		{ 100, "", " ", 192}	
			/* the fru name will be inserted, as will the */
			/* reason code, which will be the led of the device */
	    }
	}
};

struct event_description_pre_v6 io639[] =
				/* Unknown error detected by I/O device */
{
	{ "", ERRD1, 0x651, 0x639, MSGIOALLZERO,
	    {
		{ 0, "", "", 36},
		{ 0, "", "", 36},
		{ 0, "", "", 36},
		{ 0, "", "", 36}
	    }
	} 
}; 

struct event_description_pre_v6 io730[] = 	/* I/O Error on non-PCI bus */
{
	{ "", ERRD1, 0x651, 0x730, MSGIOB12b4,
	    {
		FRU_ISA_BUS(100) 
	    }
	} 
};

struct event_description_pre_v6 io731[] =
				/* Mezzanine/Processor Bus Addr. Parity Error */
{
	{ "", ERRD1, 0x651, 0x731, MSGIOB12b5,
	    {
		FRU_MEZZANINE_BUS(60),
		FRU_HOST_BRIDGE(40)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x731, DEFER_MSGIOB12b5,
	    {
		FRU_MEZZANINE_BUS(60),
		FRU_HOST_BRIDGE(40)
	    }
	} 
};

struct event_description_pre_v6 io732[] =
				/* Mezzanine/Processor Bus Data Parity Error */
{
	{ "", ERRD1, 0x651, 0x732, MSGIOB12b6,
	    {
		FRU_MEZZANINE_BUS(60),
		FRU_HOST_BRIDGE(40)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x732, DEFER_MSGIOB12b6,
	    {
		FRU_MEZZANINE_BUS(60),
		FRU_HOST_BRIDGE(40)
	    }
	} 
};

struct event_description_pre_v6 io733[] =
				/* Mezzanine/Processor Bus Addr. Parity Error */
{
	{ "", ERRD1, 0x651, 0x733, MSGIOB12b5,
	    {
		FRU_MEMORY_CONTROLLER(40),
		FRU_MEZZANINE_BUS(35),
		FRU_HOST_BRIDGE(25)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x733, DEFER_MSGIOB12b5,
	    {
		FRU_MEMORY_CONTROLLER(40),
		FRU_MEZZANINE_BUS(35),
		FRU_HOST_BRIDGE(25)
	    }
	} 
};

struct event_description_pre_v6 io734[] =
				/* Mezzanine/Processor Bus Data Parity Error */
{
	{ "", ERRD1, 0x651, 0x734, MSGIOB12b6,
	    {
		FRU_MEMORY_CONTROLLER(40),
		FRU_MEZZANINE_BUS(35),
		FRU_HOST_BRIDGE(25)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x734, DEFER_MSGIOB12b6,
	    {
		FRU_MEMORY_CONTROLLER(40),
		FRU_MEZZANINE_BUS(35),
		FRU_HOST_BRIDGE(25)
	    }
	} 
};

struct event_description_pre_v6 io735[] =
				/* Mezzanine/Processor Bus Time-out Error */
{
	{ "", ERRD1, 0x651, 0x735, MSGIOB12b7,
	    {
		FRU_MEZZANINE_BUS_ARBITER(60),
		FRU_HOST_BRIDGE(40)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x735, DEFER_MSGIOB12b7,
	    {
		FRU_MEZZANINE_BUS_ARBITER(60),
		FRU_HOST_BRIDGE(40)
	    }
	} 
};

struct event_description_pre_v6 io736[] =
				/* Mezzanine/Processor Bus Time-out Error */
{
	{ "", ERRD1, 0x651, 0x736, MSGIOB12b7,
	    {
		FRU_MEZZANINE_BUS_ARBITER(40),
		FRU_HOST_BRIDGE(35),
		FRU_MEMORY_CONTROLLER(25)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x736, DEFER_MSGIOB12b7,
	    {
		FRU_MEZZANINE_BUS_ARBITER(40),
		FRU_HOST_BRIDGE(35),
		FRU_MEMORY_CONTROLLER(25)
	    }
	} 
};

struct event_description_pre_v6 sp740[] =
				/* T.O. on communication response from SP */
{
	{ "", ERRD1, 0x651, 0x740, MSGSPB16b0,
	    {
		FRU_SERVICE_PROCESSOR(60),
		FRU_SP_SYSTEM_INTERFACE(40)
	    }
	} 
};

struct event_description_pre_v6 sp640[] = /* I/O (I2C) general bus error */
{
	{ "", ERRD1, 0x651, 0x640, MSGSPB16b1,
	    {
		FRU_SP_PRIMARY_BUS(100)
	    }
	} 
};

struct event_description_pre_v6 sp641[] =
				/* Secondary I/O (I2C) general bus error */
{
	{ "", ERRD1, 0x651, 0x641, MSGSPB16b2,
	    {
		FRU_SP_SECONDARY_BUS(100) 
	    }
	} 
};

struct event_description_pre_v6 sp642[] = 
				/* Internal Service Processor memory error */
{
	{ "", ERRD1, 0x651, 0x642, MSGSPB16b3,
	    {
		FRU_SERVICE_PROCESSOR(100)
	    }
	} 
};

struct event_description_pre_v6 sp741[] =
				/* SP error accessing special registers */
{
	{ "", ERRD1, 0x651, 0x741, MSGSPB16b4,
	    {
		FRU_SERVICE_PROCESSOR(60),
		FRU_SP_SYSTEM_INTERFACE(40)
	    }
	} 
};

struct event_description_pre_v6 sp742[] =
				/* SP reports unknown communication error */
{
	{ "", ERRD1, 0x651, 0x742, MSGSPB16b5,
	    {
		FRU_SERVICE_PROCESSOR(60),
		FRU_SP_SYSTEM_INTERFACE(40)
	    }
	} 
};

struct event_description_pre_v6 sp643[] =
				/* Internal Service Processor firmware error */
{
	{ "", ERRD1, 0x651, 0x643, MSGSPB16b6,
	    {
		FRU_SERVICE_PROCESSOR(100)
	    }
	} 
};

struct event_description_pre_v6 sp644[] = /* Other internal SP hardware error */
{
	{ "", ERRD1, 0x651, 0x644, MSGSPB16b7,
	    {
		FRU_SERVICE_PROCESSOR(100)
	    }
	} 
};

struct event_description_pre_v6 sp743[] = /* SP error accessing VPD EEPROM */
{
	{ "", ERRD1, 0x651, 0x743, MSGSPB17b0,
	    {
		FRU_VPD_MODULE(75),
		FRU_SP_PRIMARY_BUS(25)
	    }
	} 
};

struct event_description_pre_v6 sp744[] =
				/* SP error accessing Operator Panel */
{
	{ "", ERRD1, 0x651, 0x744, MSGSPB17b1,
	    {
		FRU_OPERATOR_PANEL(75), 
		FRU_SP_PRIMARY_BUS(25)
	    }
	} 
};

struct event_description_pre_v6 sp745[] =
				/* SP error accessing Power Controller */
{
	{ "", ERRD1, 0x651, 0x745, MSGSPB17b2,
	    {
		FRU_POWER_CONTROLLER(75), 
		FRU_SP_PRIMARY_BUS(25)
	    }
	} 
};

struct event_description_pre_v6 sp746[] =
					/* SP error accessing Fan Sensor */
{
	{ "", ERRD1, 0x651, 0x746, MSGSPB17b3,
	    {
		FRU_FAN_SENSOR(75), 
		FRU_SP_PRIMARY_BUS(25)
	    }
	} 
};

struct event_description_pre_v6 sp747[] =
					/* SP error accessing Thermal Sensor */
{
	{ "", ERRD1, 0x651, 0x747, MSGSPB17b4,
	    {
		FRU_THERMAL_SENSOR(75), 
		FRU_SP_PRIMARY_BUS(25)
	    }
	} 
};

struct event_description_pre_v6 sp748[] =
					/* SP error accessing Voltage Sensor */
{
	{ "", ERRD1, 0x651, 0x748, MSGSPB17b5,
	    {
		FRU_VOLTAGE_SENSOR(75), 
		FRU_SP_PRIMARY_BUS(25)
	    }
	} 
};

struct event_description_pre_v6 sp749[] = /* SP error accessing serial port */
{
	{ "", ERRD1, 0x651, 0x749, MSGSPB18b0,
	    {
		FRU_SERIAL_PORT_CONTROLLER(75), 
		FRU_SP_SYSTEM_INTERFACE(25)
	    }
	} 
};

struct event_description_pre_v6 sp750[] =	/* SP error accessing NVRAM */
{
	{ "", ERRD1, 0x651, 0x750, MSGSPB18b1,
	    {
		FRU_NVRAM(75), 
		FRU_SP_SYSTEM_INTERFACE(25)
	    }
	} 
};

struct event_description_pre_v6 sp751[] = /* SP error accessing RTC/TOD clock */
{
	{ "", ERRD1, 0x651, 0x751, MSGSPB18b2,
	    {
		FRU_RTC_TOD(75), 
		FRU_SP_SYSTEM_INTERFACE(25)
	    }
	} 
};

struct event_description_pre_v6 sp752[] =
				/* SP error accessing JTAG/COP controller */
{
	{ "", ERRD1, 0x651, 0x752, MSGSPB18b3,
	    {
		FRU_JTAG(75), 
		FRU_SP_SYSTEM_INTERFACE(25)
	    }
	} 
};

struct event_description_pre_v6 sp753[] = /* SP detect error with tod battery */
{
	{ "", ERRD1, 0x651, 0x753, MSGSPB18b4,
	    {
		FRU_BATTERY(75), 
		FRU_SP_SYSTEM_INTERFACE(25)
	    }
	} 
};

struct event_description_pre_v6 sp754[] =  /* SP detect SPCN link failure */
{
	{ "", ERRD1, 0x651, 0x754, MSGSPB19b0,
	} 
};

struct event_description_pre_v6 sp760[] =
				/* SP reboot system due to surveillance T.O. */
{
	{ "", ERRD1, 0x651, 0x760, MSGSPB18b7,
	    {
		FRU_SOFTWARE(60), 
		FRU_HARDWARE(40)
	    }
	} 
};

struct event_description_pre_v6 io770[] =
				/* Mezzanine/Processor Bus Addr. Parity Error */
{
	{ "", ERRD1, 0x651, 0x770, MSGIOB12b5,
	    {
		FRU_MEZZANINE_BUS(40),
		FRU_HOST_BRIDGE(35),
		FRU_IO_EXPANSION_BUS(25)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x770, DEFER_MSGIOB12b5,
	    {
		FRU_MEZZANINE_BUS(40),
		FRU_HOST_BRIDGE(35),
		FRU_IO_EXPANSION_BUS(25)
	    }
	} 
};

struct event_description_pre_v6 io771[] =
				/* Mezzanine/Processor Bus Data Parity Error */
{
	{ "", ERRD1, 0x651, 0x771, MSGIOB12b6,
	    {
		FRU_MEZZANINE_BUS(40),
		FRU_HOST_BRIDGE(35),
		FRU_IO_EXPANSION_BUS(25)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x771, DEFER_MSGIOB12b6,
	    {
		FRU_MEZZANINE_BUS(40),
		FRU_HOST_BRIDGE(35),
		FRU_IO_EXPANSION_BUS(25)
	    }
	} 
};

struct event_description_pre_v6 io772[] =
				/* Mezzanine/Processor Bus Time-out Error */
{
	{ "", ERRD1, 0x651, 0x772, MSGIOB12b7,
	    {
		FRU_MEZZANINE_BUS(40),
		FRU_HOST_BRIDGE(35),
		FRU_MEZZANINE_BUS_ARBITER(25)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x772, DEFER_MSGIOB12b7,
	    {
		FRU_MEZZANINE_BUS(40),
		FRU_HOST_BRIDGE(35),
		FRU_MEZZANINE_BUS_ARBITER(25)
	    }
	} 
};

struct event_description_pre_v6 io773[] =
				/* Err detected by i/o exp. bus controller */
{
	{ "", ERRD1, 0x651, 0x773, MSGIOB12b6,
	    {
		FRU_RIO_BRIDGE(100)
	    }
	}, 
	{ "", ERRD1, 0x652, 0x773, DEFER_MSGIOB12b6,
	    {
		FRU_RIO_BRIDGE(100)
	    }
	} 
};

struct event_description_pre_v6 epow800[] =
				/* Fan is turning slower than expected */
{
	{ "", ERRD1, 0x651, 0x800, MSGXEPOW1n11,
	    {
		FRU_FAN(60), 
		FRU_FAN_SENSOR(40) 
	    }
	} 
};

struct event_description_pre_v6 epow801[] =	/* Fan stop was detected */
{
	{ "", ERRD1, 0x651, 0x801, MSGXEPOW1n64,
	    {
		FRU_FAN(60), 
		FRU_FAN_SENSOR(40) 
	    }
	} 
};

struct event_description_pre_v6 epow802[] =	/* Fan failure */
{
	{ "", ERRD1, 0x651, 0x802, MSGEPOWB16b2C12
	}
};

struct event_description_pre_v6 epow809[] =
				/* Power fault due to unspecified cause */
{
	{ "", ERRD1, 0x651, 0x809, MSGEPOWB17b0C12
	} 
};

struct event_description_pre_v6 epow810[] =
				/* Over voltage condition was detected */
{
	{ "", ERRD1, 0x651, 0x810, MSGXEPOW2n32,
	    {
		FRU_POWER_SUPPLY(60), 
		FRU_VOLTAGE_SENSOR(40) 
	    }
	} 
};

struct event_description_pre_v6 epow811[] =
			/* Under voltage condition was detected */
{
	{ "", ERRD1, 0x651, 0x811, MSGXEPOW2n52,
	    {
		FRU_POWER_SUPPLY(60), 
		FRU_VOLTAGE_SENSOR(40) 
	    }
	} 
};

struct event_description_pre_v6 epow812[] =
			/* System shutdown due to loss of power */
{
	{ "", ERRD1, 0x651, 0x812, MSGEPOWB1505,
	    {
		FRU_POWER_SUPPLY(100) 
	    }
	} 
};

struct event_description_pre_v6 epow813[] =
			/* System shutdown due to loss of power to site*/
{
	{ "", ERRD1, 0x651, 0x813, MSGEPOWPCI111
	} 
};

struct event_description_pre_v6 epow814[] =
			/* System shutdown due to loss of power to CEC */
{
	{ "", ERRD1, 0x651, 0x814, MSGEPOWPCI100
	} 
};

struct event_description_pre_v6 epow815[] = 
			  /* System shutdown due to loss of power to I/O Rack */
{
	{ "", ERRD1, 0x651, 0x815, MSGEPOWPCI011,
	    {
		FRU_34_POWER_SUPPLY(50),
		FRU_14_POWER_SUPPLY(50) 
	    }
	} 
};

struct event_description_pre_v6 epow816[] =
				/* Power fault due to internal power supply */
{
	{ "", ERRD1, 0x651, 0x816, MSGEPOWPCI001,
	    {
		FRU_34_POWER_SUPPLY(100) 
	    }
	} 
};

struct event_description_pre_v6 epow817[] =
				/* Power fault due to internal power supply */
{
	{ "", ERRD1, 0x651, 0x817, MSGEPOWPCI010, 
	    {
		FRU_14_POWER_SUPPLY(100) 
	    }
	} 
};

struct event_description_pre_v6 epow818[] =
				/* Power fault due to power-off request */
{
	{ "", ERRD1, 0x651, 0x818, MSGEPOWB16b1B17b3
	} 
};

struct event_description_pre_v6 epow819[] =
				/* Power fault due to internal power supply */
{ 
	{ "", ERRD1, 0x651, 0x819, MSGEPOWPCI001,
	    {
		FRU_RTASFRU(100) 
	    }
	} 
};

struct event_description_pre_v6 epow819red[] =
				/* Power fault due to internal redundant PS */
{ 
	{ "", ERRD1, 0x652, 0x819, MSGEPOWB17b2RED
	} 
};

struct event_description_pre_v6 epow820[] =
				/* Over temperature condition was detected */
{
	{ "", ERRD1, 0x651, 0x820, MSGXEPOW3n21,
	    {
		FRU_THERMAL_SENSOR(100) 
	    }
	} 
};

struct event_description_pre_v6 epow821[] =
				/* System shutdown due to over max temp. */
{
	{ "", ERRD1, 0x651, 0x821, MSGXEPOW3n73,
	    {
		FRU_THERMAL_SENSOR(100) 
	    }
	} 
};

struct event_description_pre_v6 epow822[] =
			/* System shutdown due to thermal and fan fail */
{
	{ "", ERRD1, 0x651, 0x822, MSGEPOWB16b23
	} 
};

struct event_description_pre_v6 epow823[] =
				/* System shutdown due to fan failure */
{
	{ "", ERRD1, 0x651, 0x823, MSGEPOWB16b2C37
	} 
};

struct event_description_pre_v6 epow824[] =
			/* System shutdown:Power fault-unspecified cause*/
{
	{ "", ERRD1, 0x651, 0x824, MSGEPOWB17b0C37
	} 
};

struct event_description_pre_v6 epow652810[] =
					/* Loss of redundant power supply. */
{
	{ "", ERRD1, 0x652, 0x810, MSGEPOW1502B16b4
	} 
};

struct event_description_pre_v6 epow652820[] =	/* Loss of redundant cec blower. */
{
	{ "", ERRD1, 0x652, 0x820, MSGEPOW1501B16b4
	} 
};

struct event_description_pre_v6 post[] =	/* POST 8 digit code */
{
	{ "", ERRD1, -2, 0x0, MSGPOSTALL, /* -2 = DAVars has 8 digit hex code */
	    {
		{ 0, "", "", 36},
		{ 0, "", "", 36},
		{ 0, "", "", 36},
		{ 0, "", "", 36}
	    }
	}
};

struct event_description_pre_v6 postfw[] =
					/* POST 8 digit code that is fw error*/
{
	{ "", ERRD1, -2, 0x0, MSGPOSTALL, /* -2 = DAVars has 8 digit hex code */
	    {
		{ 50, " ", " ",  35},
		{ 50, " ", " ",  34}
	    }
	}
};

struct event_description_pre_v6 memtest600[] =
					/* Uncorrectable/unsupported memory */
{
	{ "", ERRD1, 0x651, 0x600, MSG_UNSUPPORTED_MEM,
	    {
		FRU_MEMORY_MODULE(100) 
	    }
	}, 
	{ "", ERRD1, 0x652, 0x600, DEFER_MSG_UNSUPPORTED_MEM,
	    {
		FRU_MEMORY_MODULE(100) 
	    }
	} 
};

struct event_description_pre_v6 memtest601[] = /* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x601, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(100) 
	    }
	} 
};

struct event_description_pre_v6 memtest602[] = /* Missing or bad memory */ 
{
	{ "", ERRD1, 0x651, 0x602, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(90), 
		FRU_MEMORY_CARD(10)
	    }
	} 
};

struct event_description_pre_v6 memtest603[] = /* Missing or bad memory */
{
	{ "", ERRD1, 0x651, 0x603, MSG_MISSING_MEM, 
	    {
		FRU_MEMORY_MODULE(90), 
		FRU_MEMORY_CARD(10)
	    }
	} 
};

struct event_description_pre_v6 l2test608[] = /* Bad L2 Cache */
{
	{ "", ERRD1, 0x651, 0x608, MSG_BAD_L2, 
	    {
		FRU_L2CACHE(100) 
	    }
	} 
};

struct event_description_pre_v6 l2test609[] = /* Missing L2 Cache */
{
	{ "", ERRD1, 0x651, 0x609, MSG_MISSING_L2, 
	    {
		FRU_MISSING_L2(100) 
	    }
	} 
};

/* 
 * Source Number to match dresid and DIMB 
 */
struct event_description_pre_v6 optest140[] = /* Op panel display test failed */
{
	{ "", ERRD1, 0x651, 0x140, MSG_OP_PANEL_FAIL, 
	    {
		FRU_OPERATOR_PANEL(95),
		FRU_IO_PLANAR(5) 
	    }
	} 
};

/*
 * Event that is filled with ref. codes (in conf. field) and location
 * codes. The SRN will be formatted differently because the conf.
 * value is greater than 100.
 */
struct event_description_pre_v6 cec_src[] = /* SRC and ref. codes from CEC */
{
	{ "", ERRD1, 0x651, 0, "",
	    {
		{ 0, "", "", 0},
		{ 0, "", "", 0},
		{ 0, "", "", 0},
		{ 0, "", "", 0}
	    }
	}
};
 
struct event_description_pre_v6 fan_epow[] = /* SRN for fan epow */
{
        { "", ERRD1, 0x651, 0, "",
            {
		FRU_FAN(60),
		FRU_FAN_SENSOR(40)
            }
        }
};

struct event_description_pre_v6 volt_epow[] = /* SRN for voltage sensor epow */
{
        { "", ERRD1, 0x651, 0, "",
            {
		FRU_POWER_SUPPLY(60),
		FRU_VOLTAGE_SENSOR(40)
            }
        }
};

struct event_description_pre_v6 therm_epow[] = /* SRN for thermal sensor epow */
{
        { "", ERRD1, 0x651, 0, "",
            {
		FRU_THERMAL_SENSOR(100)
            }
        }
};

struct event_description_pre_v6 pow_epow[] = /* SRN for power epow */
{
        { "", ERRD1, 0x651, 0, "",
            {
		FRU_POWER_SUPPLY(60),
                FRU_POWER_SENSOR(40)
            }
        }
};

struct event_description_pre_v6 unknown_epow[] = /* SRN for unknown epow */
{
        { "", ERRD1, 0x651, 0, "",
            {
                { 100, "", "", 180}
            }
        }
};

/* Version 3 error descriptions */

struct event_description_pre_v6 platform_error[] =
					/* SRN for platform specific error */
{
        { "", ERRD1, 0x651, 0x900, MSG_PLATFORM_ERROR,
            {
		{ 0, "", "", 0},
		{ 0, "", "", 0},
		{ 0, "", "", 0},
		{ 0, "", "", 0}
            }
        }
};

#define SN_V3ELA	0xA00		/* source number for V3 ELA */

struct event_description_pre_v6 v3_errdscr[] = /* SRN for V3 error logs */
{
        { "", ERRD1, SN_V3ELA, 0, "",
            {
		{ 0, "", "", 0},
		{ 0, "", "", 0},
		{ 0, "", "", 0},
		{ 0, "", "", 0}
            }
        }
};


struct event_description_pre_v6 dt_errdscr[] =
				/* SRN for deconfig resources in device tree */
{
        { "", ERRD1, 0xA10, 0x200, FAIL_BY_PLATFORM, 
	    {
		{ 100, "", "", 0},
            }
        }
};

struct event_description_pre_v6 bypass_errdscr[] =
				/* SRN for deconfig resource from error log */
{
        { "", ERRD1, 0xA10, 0x100, MSG_BYPASS, 
	    {
		{ 100, "", "", 0},
            }
        }
};

#endif
