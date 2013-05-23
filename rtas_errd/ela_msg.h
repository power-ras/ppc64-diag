/**
 * @file ela_msg.h
 *
 * Copyright (C) 2005 IBM Corporation
 */

#ifndef _H_RPA_ELA_MSG
#define _H_RPA_ELA_MSG

#define F_POWER_SUPPLY "Power Supply"

#define MSGCPUB12b0 "CPU internal error"

#define MSGCPUB12b5 "Time-out error waiting for I/O"

#define MSGCPUALLZERO "An error has been detected by the CPU. The error " \
    "log indicates the following physical FRU location(s) as the probable "\
    "cause(s)."

#define MSGMEMB12b0 "Uncorrectable Memory Error"
#define MSGMEMB12b1 "ECC correctable error"

#define MSGMEMB12b2 "Correctable error threshold exceeded"
#define MSGMEMB12b5 "Memory Data error (Bad data going to memory)"
#define MSGMEMB12b6 "Memory bus/switch internal error"
#define MSGMEMB12b7 "Memory time-out error"
#define MSGMEMB13b3 "I/O Host Bridge time-out error"
#define MSGMEMB13b4 "I/O Host Bridge address/data parity error"

#define MSGMEMALLZERO "An error has been detected by the memory controller. " \
    "The error log indicates the following physical FRU location(s) as the " \
    "probable cause(s)."

#define MSGIOB12b0 "I/O Bus Address Parity Error"
#define MSGIOB12b1 "I/O Bus Data Parity Error"
#define MSGIOB12b4 "I/O Error on ISA bus"

#define MSGIOB12b5 "Intermediate or System Bus Address Parity Error"
#define MSGIOB12b6 "Intermediate or System Bus Data Parity Error"
#define MSGIOB12b7 "Intermediate or System Bus Time-out Error"

#define MSGIOALLZERO "An error has been detected by the I/O. The error " \
    "log indicates the following physical FRU location(s) as the probable " \
    "cause(s)."

#define MSGSPB16b0 "Time-out on communication response from Service Processor" 
#define MSGSPB16b1 "I/O general bus error"
#define MSGSPB16b2 "Secondary I/O general bus error"

#define MSGSPB16b3 "Internal Service Processor memory error"
#define MSGSPB16b4 "Service Processor error accessing special registers"
#define MSGSPB16b5 "Service Processor reports unknown communication error" 
#define MSGSPB16b7 "Other internal Service Processor hardware error"
#define MSGSPB17b0 "Service Processor error accessing Vital Product Data EEPROM"
#define MSGSPB17b1 "Service Processor error accessing Operator Panel"
#define MSGSPB17b2 "Service Processor error accessing Power Controller"
#define MSGSPB17b3 "Service Processor error accessing Fan Sensor"
#define MSGSPB17b4 "Service Processor error accessing Thermal Sensor"
#define MSGSPB17b5 "Service Processor error accessing Voltage Sensor"
#define MSGSPB18b0 "Service Processor error accessing serial port"
#define MSGSPB18b2 "Service Processor error accessing " \
    "Real-Time Clock/Time-of-day clock"

#define MSGSPB18b4 "Service Processor detects loss of voltage from the battery"

#define MSGXEPOW1n11 "Fan is turning slower than expected"
#define MSGXEPOW1n64 "Fan stop was detected"
#define MSGXEPOW2n32 "Over voltage condition was detected"
#define MSGXEPOW2n52 "Under voltage condition was detected"
#define MSGXEPOW3n21 "Over temperature condition was detected"
#define MSGXEPOW3n73 "System shutdown due to over maximum temperature condition"

#define MSGEPOWB1505 "System shutdown due to: 1) Loss of AC power,\n "    \
     "2) Power button was pushed without proper system\n" \
     "shutdown, 3) Power supply failure." 

#define MSG_UNSUPPORTED_MEM "Uncorrectable memory or unsupported memory"
#define MSG_MISSING_MEM "Missing or bad memory" 
#define MSG_BAD_L2 "Bad L2 Cache"
#define MSG_MISSING_L2 "Missing L2 cache" 

#define MSGMENUG102 "651102 "\
     "A non-critical system cooling problem has been detected.\n" \
     "The system temperature appears to be above the expected range.\n"\
     "Check for elevated room temperatures, restricted air flow\n"\
     "around the system or any system covers that are opened.\n"\
     "If no problem is found, contact your service provider."

#define MSGIOB13b4 "I/O Expansion Bus Parity Error."
#define MSGIOB13b5 "I/O Expansion Bus Time-out Error."
#define MSGIOB13b6 "I/O Expansion Bus Connection Failure."
#define MSGIOB13b7 "I/O Expansion Unit not in an operating state."

#define MSGEPOWPCI111 "System shutdown due to loss of AC Power to the site."

#define MSGEPOWPCI100 "CEC Rack shutdown due to: 1) Loss of AC power " \
    "to the CEC Rack,\n" \
    "2) Open or disconnected SPCN cable between racks\n" \
    "3) AC module, Bulk power, regulator or SPCN card failure."

#define MSGEPOWPCI011 "I/O Rack shutdown due to: 1) Loss of AC power " \
    "to the I/O Rack,\n" \
    "2) Open or disconnected SPCN cable between racks,\n" \
    "3) Power supply failure."

#define MSGEPOWPCI001 "Power fault due to internal power supply failure."
#define MSGEPOWPCI010 "Power fault due to internal power supply failure."

#define MSGEPOWB16b1B17b3 "Power fault due to manual activation of " \
    "power-off op-panel request."

#define MSGEPOWB16b23 "System shutdown due to over temperature condition " \
    "and fan failure. Use the physical FRU location(s) as the probable " \
    "cause(s)."

#define MSGEPOW1502B16b4 "Non-critical power problem, loss of redundant " \
    "power supply. Use the physical FRU location(s) as the probable cause(s)."

#define MSGEPOW1501B16b4 "Non-critical cooling problem, loss of redundant " \
    "fan. Use the physical FRU location(s) as the probable cause(s)."

#define MSGCRIT_FAN "Sensor indicates a fan has failed."
#define MSGCRIT_VOLT "Sensor indicates a voltage is outside the normal range."
#define MSGCRIT_THERM "Sensor indicates an abnormally high internal temperature."
#define MSGCRIT_POWER "Sensor indicates a power supply has failed."
#define MSGCRIT_UNK "Sensor indicates a FRU has failed."
#define MSGWARN_FAN "Sensor indicates a fan is turning too slowly."
#define MSGRED_FAN "Sensor detected a redundant fan failure."
#define MSGRED_POWER "Sensor detected a redundant power supply failure."
#define MSGRED_UNK "Sensor detected a redundant FRU failure."

#define F_EPOW_ONE "%1$s %2$d"
#define F_VOLT_SEN_2 "Volt Sensor"
#define F_THERM_SEN "Temp sensor"
#define F_POW_SEN "PS sensor"

#define MSGREFCODE_CUST "" \
"The CEC or SPCN reported an error. Report the SRN and the following reference and physical location codes to your service provider."

#define DEFER_MSGREFCODE_CUST ""\
"The CEC or SPCN reported a non-critical error. Report the SRN and the following reference and physical location codes to your service provider."

#define MSGMENUG150 "651150 ANALYZING SYSTEM ERROR LOG\n" \
    "A loss of power was detected by one or more\n" \
    "devices with the following physical location codes:\n\n" \
    "%s\n\n" \
    "Check for the following:\n" \
    "  1. Loose or disconnected power source connections.\n" \
    "  2. Loss of site power.\n" \
    "  3. For multiple enclosure systems, loose or\n" \
    "     disconnected power and signal connections\n" \
    "     between enclosures.\n\n" \
    "If the cause cannot be determined from the list above,\n" \
    "then replace the FRUs identified by the physical\n" \
    "locations given above. Refer to the System Service Guide."

#define MSGMENUG151 "651151 ANALYZING SYSTEM ERROR LOG\n "\
    "A loss of power was detected.\n\n" \
    "Check for the following:\n" \
    "  1. Loose or disconnected power source connections.\n" \
    "  2. Loss of site power.\n" \
    "  3. For multiple enclosure systems, loose or\n" \
    "     disconnected power and/or signal connections\n" \
    "     between enclosures."

#define MSGMENUG152 "651152 ANALYZING SYSTEM ERROR LOG\n" \
    "Environmental sensor analysis is reporting an unknown\n" \
    "status, with the following physical location codes.\n\n%s\n\n" \
    "Run the Display System Environmental Sensors Service Aid."

#define MSGMENUG153 "651153 ANALYZING SYSTEM ERROR LOG\n" \
    "Environmental sensor analysis is reporting an unknown\n" \
    "status. Run the Display System Environmental Sensors Service Aid."

#define MSGMENUG156 "651156 ANALYZING SYSTEM ERROR LOG\n" \
    "A non-critical environmental and power warning was logged.\n" \
    "The following physical location codes were reported:\n\n%s"

#define MSGMENUG157 "651157 ANALYZING SYSTEM ERROR LOG\n" \
    "A non-critical environmental and power warning was logged."

#define MSGMENUG158 "651158 ANALYZING SYSTEM ERROR LOG\n "\
    "A critical environmental and power warning was logged.\n" \
    "The following physical location codes were reported:\n" \
    "\n%s\n\n%s\n\nThe system will shutdown in 10 minutes."

#define MSGMENUG159 "651159 ANALYZING SYSTEM ERROR LOG\n" \
    "A critical environmental and power warning was logged.\n" \
    "\n%s\n\nThe system will shutdown in 10 minutes."

#define MSGMENUG160 "651160 ANALYZING SYSTEM ERROR LOG\n" \
    "A system shutdown occured due to a critical \n" \
    "environmental and power warning.\n" \
    "The following physical location codes were reported:\n\n%s"

#define MSGMENUG161 "651161 ANALYZING SYSTEM ERROR LOG\n" \
    "A system shutdown occured due to a critical \n" \
    "environmental and power warning."

#define MSGEPOWB17b0C12 "Power fault due to unspecified cause."
#define MSGEPOWB17b0C37 "System shutdown due to power fault with unspecified cause."
#define MSGEPOWB16b2C12 "Fan failure. Use the physical FRU location(s) as the probable cause(s)."
#define MSGEPOWB16b2C37 "System shutdown due to fan failure. Use the physical FRU location(s) as the probable cause(s)."
#define MSGEPOWB17b2RED "Power fault due to internal redundant power supply failure."

#define MSG_OP_PANEL_FAIL "Display character test failed."
#define F_OP_PANEL "character display logic"

#define MSGMENUG162 "651162 ANALYZING SYSTEM ERROR LOG\n" \
    "An error was logged on %s" \
    "that indicates the Power-off button was pressed \n" \
    "without performing a proper shutdown.\n" \
    "To remove this error from the error log, execute\n" \
    "the following command from the command line.\n\n" \
    "  errclear -l %d 0"

#define DEFER_MSG_UNSUPPORTED_MEM ""\
    "A non-critical error has been detected: "\
    "Uncorrectable memory or unsupported memory."

#define DEFER_MSGCPUB12b0 "A non-critical error has been detected: "\
    "CPU internal error."

#define DEFER_MSGMEMB12b0 "A non-critical error has been detected: "\
    "Uncorrectable Memory Error."

#define DEFER_MSGMEMB12b1 "A non-critical error has been detected: "\
    "ECC correctable error."

#define DEFER_MSGMEMB12b2 "A non-critical error has been detected: "\
    "Correctable error threshold exceeded."

#define DEFER_MSGIOB13b4 "A non-critical error has been detected: "\
    "I/O Expansion Bus Parity Error."

#define DEFER_MSGIOB13b5 "A non-critical error has been detected: "\
    "I/O Expansion Bus Time-out Error."

#define DEFER_MSGIOB13b6 "A non-critical error has been detected: "\
    "I/O Expansion Bus Connection Failure."

#define DEFER_MSGIOB13b7 "A non-critical error has been detected: "\
    "I/O Expansion Unit not in an operating state."

#define DEFER_MSGIOB12b5 "A non-critical error has been detected: "\
    "Intermediate or System Bus Address Parity Error."

#define DEFER_MSGIOB12b6 "A non-critical error has been detected: "\
    "Intermediate or System Bus Data Parity Error."

#define DEFER_MSGIOB12b7 "A non-critical error has been detected: "\
    "Intermediate or System Bus Time-out Error."

#define MSGPOSTALL "Refer to the Error Code to FRU Index in the system service guide."

#define MSGCPUB12b1 "CPU internal cache or cache controller error"

#define DEFER_MSGCPUB12b1 "A non-critical error has been detected: "\
    "CPU internal cache or cache controller error."

#define MSGCPUB12b2 "External cache parity or multi-bit ECC error"

#define DEFER_MSGCPUB12b2 "A non-critical error has been detected: "\
    "External cache parity or multi-bit ECC error."

#define MSGCPUB12b3 "External cache ECC single-bit error"

#define DEFER_MSGCPUB12b3 "A non-critical error has been detected: "\
    "External cache ECC single-bit error."

#define MSGIOB12b3 "I/O bridge/device internal error"

#define DEFER_MSGIOB12b3 "A non-critical error has been detected: "\
    "I/O bridge/device internal error"

#define MSG_PLATFORM_ERROR "Platform specific error. Refer to the System " \
    "Service Guide for this system unit."

#define DEFER_MSGALLZERO "A non-critical error has been detected. The error " \
    "log indicates the following physical FRU location(s) as the probable " \
    "cause(s)."

#define MSGCPUB12b4 "System bus time-out error"

#define DEFER_MSGCPUB12b4 "A non-critical error has been detected: " \
    "System bus time-out error."

#define DEFER_MSGCPUB12b5 "A non-critical error has been detected: " \
    "Time-out error waiting for I/O."

#define MSGCPUB12b6 "System bus parity error"

#define DEFER_MSGCPUB12b6 "A non-critical error has been detected: "\
    "System bus parity error."

#define MSGCPUB12b7 "System bus protocol/transfer error"

#define DEFER_MSGCPUB12b7 "A non-critical error has been detected: "\
    "System bus protocol/transfer error."

#define MSGMEMB12b3 "Memory Controller subsystem internal error"

#define DEFER_MSGMEMB12b3 "A non-critical error has been detected: "\
    "Memory Controller subsystem internal error."

#define MSGMEMB12b4 "Memory Address Error (Invalid address or access attempt)"

#define DEFER_MSGMEMB12b4 "A non-critical error has been detected: " \
    "Memory Address (Bad address going to memory)."

#define DEFER_MSGMEMB12b5 "A non-critical error has been detected: "\
    "Memory Data error (Bad data going to memory)."

#define DEFER_MSGMEMB12b6 "A non-critical error has been detected: "\
    "Memory bus/switch internal error."

#define DEFER_MSGMEMB12b7 "A non-critical error has been detected: "\
    "Memory time-out error."

#define MSGMEMB13b0 "System bus parity error"

#define DEFER_MSGMEMB13b0 "A non-critical error has been detected: "\
    "System bus parity error."

#define MSGMEMB13b1 "System bus time-out error"

#define DEFER_MSGMEMB13b1 "A non-critical error has been detected: " \
    "System bus time-out error."

#define MSGMEMB13b2 "System bus protocol/transfer error"

#define DEFER_MSGMEMB13b2 "A non-critical error has been detected: " \
    "System bus protocol/transfer error."

#define DEFER_MSGMEMB13b3 "A non-critical error has been detected: " \
    "I/O Host Bridge time-out error."

#define DEFER_MSGMEMB13b4 "A non-critical error has been detected: "\
    "I/O Host Bridge address/data parity error."

#define MSGMEMB13b6 "System support function error."

#define DEFER_MSGMEMB13b6 "A non-critical error has been detected: "\
    "System support function error."

#define MSGMEMB13b7 "System bus internal hardware/switch error."

#define DEFER_MSGMEMB13b7 "A non-critical error has been detected: "\
    "System bus internal hardware/switch error."

#define DEFER_MSGIOB12b0 "A non-critical error has been detected: "\
    "I/O Bus Address Parity Error"

#define DEFER_MSGIOB12b1 "A non-critical error has been detected: "\
    "I/O Bus Data Parity Error"

#define MSGIOB12b2 "I/O bus time-out, access, or other error"

#define DEFER_MSGIOB12b2 "A non-critical error has been detected: " \
    "I/O Bus time-out, access or other error"

#define DEFER_MSGIOB12b4 "A non-critical error has been detected: "\
    "I/O Error on ISA bus"

#define MSG_UNKNOWN_V3 "Error log analysis is unable to determine the " \
    "error. The error log indicates the following physical FRU location(s) " \
    "as the probable cause(s)."

#define MSGCRIT_FAN_SHUT "System shutdown due to a fan failure."

#define MSGCRIT_VOLT_SHUT "System shutdown due to voltage outside the " \
    "normal range."

#define MSGCRIT_THERM_SHUT "System shutdown due to an abnormally high " \
    "internal temperature."

#define MSGCRIT_POWER_SHUT "System shutdown due to a power supply failure."

#define MSGSPB19b0 "Power Control Network general connection failure."

#define DEFER_MSGSPB19b0 "A non-critical error has been detected: "\
    "Power Control Network general connection failure."

#define MSGSPB19b1 "Power Control Network node failure."

#define DEFER_MSGSPB19b1 "A non-critical error has been detected: "\
    "Power Control Network node failure."

#define MSGSPB19b4 "Service Processor error accessing Power Control Network."

#define DEFER_MSGSPB19b4 "A non-critical error has been detected: "\
    "Service Processor error accessing Power Control Network."

#define MSGRESERVED "Error with reserved description."

#define DEFER_MSGRESERVED "A non-critical error has been detected: "\
    "Error with reserved description."

#define MSGCRIT_UNK_SHUT "System shutdown due to FRU that has failed."

#define MSGIOB12b5B13b3 "System bus address parity error"

#define DEFER_MSGIOB12b5B13b3 "A non-critical error has been detected: " \
    "System bus address parity error"

#define MSGIOB12b6B13b3 "System bus data parity error"

#define DEFER_MSGIOB12b6B13b3 "A non-critical error has been detected: " \
    "System bus data parity error"

#define MSGIOB12b7B13b3 "System bus time-out error"

#define DEFER_MSGIOB12b7B13b3 "A non-critical error has been detected: " \
    "System bus time-out error"

#define MSGIOB13b3 "Error on system bus"

#define DEFER_MSGIOB13b3 "A non-critical error has been detected: "\
    "Error on system bus"

#define MSGEPOWALLZERO "An environmental and power warning has been detected. "\
    "The error log indicates the following physical FRU location(s) as the " \
    "probable cause(s)."
    
#define MSGEPOWB16b2 "Fan failure"

#define MSGEPOWB17b2C12 "Internal power supply failure"

#define MSGEPOWB17b2C37 "System shutdown due to internal power supply failure"

#define MSGEPOWB16b3 "Over temperature condition"

#define MSGEPOWB16b2b3 "Over temperature and fan failure"

#define MSGSPALLZERO "An error has been detected by the Service Processor. " \
    "The error log indicates the following physical FRU location(s) as the " \
    "probable cause(s)."

#define DEFER_MSGSPB16b0 "A non-critical error has been detected: " \
    "Time-out on communication response from Service Processor"

#define DEFER_MSGSPB16b1 "A non-critical error has been detected: " \
    "I/O general bus error"

#define DEFER_MSGSPB16b2 "A non-critical error has been detected: " \
    "Secondary I/O general bus error"

#define DEFER_MSGSPB16b3 "A non-critical error has been detected: " \
    "internal Service Processor memory error"

#define DEFER_MSGSPB16b4 "A non-critical error has been detected: " \
    "Service Processor error accessing special registers"

#define DEFER_MSGSPB16b5 "A non-critical error has been detected: " \
    "Service Processor reports unknown communication error"

#define DEFER_MSGSPB16b7 "A non-critical error has been detected: " \
    "Other internal Service Processor hardware error"

#define DEFER_MSGSPB17b0 "A non-critical error has been detected: " \
    "Service Processor error accessing Vital Product Data EEPROM"

#define DEFER_MSGSPB17b1 "A non-critical error has been detected: " \
    "Service Processor error accessing Operator Panel"

#define DEFER_MSGSPB17b2 "A non-critical error has been detected: " \
    "Service Processor error accessing Power Controller"

#define DEFER_MSGSPB17b3 "A non-critical error has been detected: " \
    "Service Processor error accessing Fan Sensor"

#define DEFER_MSGSPB17b4 "A non-critical error has been detected: " \
    "Service Processor error accessing Thermal Sensor"

#define DEFER_MSGSPB17b5 "A non-critical error has been detected: " \
    "Service Processor error accessing Voltage Sensor" 

#define DEFER_MSGSPB18b0 "A non-critical error has been detected: " \
    "Service Processor error accessing serial port"

#define MSGSPB18b1 "Service Processor detected NVRAM error"

#define DEFER_MSGSPB18b1 "A non-critical error has been detected: " \
    "Service Processor detected NVRAM error"

#define DEFER_MSGSPB18b2 "A non-critical error has been detected: " \
    "Service Processor error accessing Real-Time Clock/Time-of-day clock"

#define DEFER_MSGSPB18b4 "A non-critical error has been detected: " \
    "Service Processor detects loss of voltage from the battery"

#define MSGSPB18b7 "Service Processor detected a surveillance time-out"

#define DEFER_MSGSPB18b7 "A non-critical error has been detected: " \
    "Service Processor detected a surveillance time-out"

#define MSGMENUG171 "651171 ANALYZING SYSTEM ERROR LOG\n" \
    "A problem was logged that could be either hardware, software\n" \
    "or firmware. If new software or firmware has been recently\n" \
    "installed, check if there are any known problems. To further\n" \
    "analyze the problem, enter the following command from the\n" \
    "command line.\n\n" \
    "  diag -ed sysplanar0"

#define PREDICT_UNRECOV "Recoverable errors on resource indicate trend " \
    "toward unrecoverable error. However the resource could not be " \
    "deconfigured and is still in use."

#define FAIL_BY_PLATFORM "Resource was marked failed by the platform. "\
    "System is operating in degraded mode."

#define MSGMEMB12b4B13b3 "I/O Host Bridge Time-out caused by software"

#define MSGMENUG174 "651174 ANALYZING SYSTEM ERROR LOG\n" \
    "Error log analysis indicates an I/O Host Bridge time-out\n" \
    "that was caused by software.  This error is caused by an\n" \
    "attempt to access an invalid I/O device address.  Contact\n" \
    "the software support structure for assistance if needed.\n"

#define MSGMENUG175 "651175 ANALYZING SYSTEM ERROR LOG\n" \
    "Error log analysis indicates a power fault that was\n" \
    "caused by either using the Power-off button without \n" \
    "performing a proper shutdown, or a loss of power to \nthe system.\n"

#define MSGSPB16b6 "Internal Service Processor firmware error or incorrect " \
    "version"

#define DEFER_MSGSPB16b6 "A non-critical error has been detected: "\
    "Internal Service Processor firmware error or incorrect version"

#define MSGSPB18b3 "Service Processor error accessing Scan controller/hardware"

#define DEFER_MSGSPB18b3 "A non-critical error has been detected: "\
    "Service Processor error accessing Scan controller/hardware"

#define MSGSPB18b6 "Loss of heart beat from Service Processor"

#define DEFER_MSGSPB18b6 "A non-critical error has been detected: " \
    "Loss of heart beat from Service Processor"

#define MSGSPB19b5 "Non-supported hardware"

#define DEFER_MSGSPB19b5 "A non-critical error has been detected: "\
    "Non-supported hardware"

#define MSGSPB19b6 "Error detected while handling an attention/interrupt " \
    "from the system hardware"

#define DEFER_MSGSPB19b6 "A non-critical error has been detected: " \
    "Error detected while handling an attention/interrupt from the " \
    "system hardware"

#define MSGSPB28b1 "Wire Test Error"
    
#define DEFER_MSGSPB28b1 "A non-critical error has been detected: "\
    "Wire Test Error"

#define MSGSPB28b2 "Mainstore or Cache IPL Diagnositc Error"

#define DEFER_MSGSPB28b2 "A non-critical error has been detected: " \
    "Mainstore or Cache IPL Diagnositc Error"

#define MSGSPB28b3 "Other IPL Diagnostic Error"

#define DEFER_MSGSPB28b3 "A non-critical error has been detected: "\
    "Other IPL Diagnostic Error"

#define MSGSPB28b4 "Clock or PLL Error"

#define DEFER_MSGSPB28b4 "A non-critical error has been detected: "\
    "Clock or PLL Error"

#define MSGSPB28b5 "Hardware Scan or Initialization Error"

#define DEFER_MSGSPB28b5 "A non-critical error has been detected: "\
    "Hardware Scan or Initialization Error"

#define MSGSPB28b6 "Chip ID Verification Error"

#define DEFER_MSGSPB28b6 "A non-critical error has been detected: " \
    "Chip ID Verification Error"

#define MSGSPB28b7 "FRU Presence/Detect Error (Mis-Plugged)"
    
#define DEFER_MSGSPB28b7 "A non-critical error has been detected: " \
    "FRU Presence/Detect Error (Mis-Plugged)"

#define MSGEPOWB16b1B17b4 "Power fault specifically due to internal " \
    "battery failure"

#define MSGMENUG202 "651202 ANALYZING SYSTEM ERROR LOG\n" \
    "An environmental problem has been corrected and is \n" \
    "no longer a problem.  The problem that was corrected \n" \
    "can be viewed by using the command:\n" \
    "\t errpt -a -l %d"

#define MSGMENUG203 "651203 ANALYZING SYSTEM ERROR LOG\n" \
    "An environmental problem has been corrected and is no longer a problem."

#define MSGMENUG204 "651204 ANALYZING SYSTEM ERROR LOG\n" \
    "A loss of redundancy on input power was detected.\n\n" \
    "Check for the following:\n" \
    "  1. Loose or disconnected power source connections.\n" \
    "  2. Loss of the power source.\n" \
    "  3. For multiple enclosure systems, loose or\n" \
    "     disconnected power and/or signal connections\n" \
    "     between enclosures."

#define MSGSPB28b0 "Array or Logic Built in Self Test Error"
    
#define DEFER_MSGSPB28b0 "A non-critical error has been detected: " \
    "Array or Logic Built in Self Test Error"

#define PREDICT_REPAIR_PENDING "Recoverable errors on a resource indicate " \
    "a potential for an unrecoverable error. The resource cannot be " \
    "deconfigured and is still in use. The problem may be corrected by "\
    "array bit steering. Use map 0235."

#define MSGMENUG205 "651205 ANALYZING SYSTEM ERROR LOG\n" \
    "The system lost input power.\n\n" \
    "If the system has battery backup and input power is\n" \
    "not restored, then the system will lose all power in\n" \
    "a few minutes. Take the necessary precautions with\n" \
    "running applications for a system shut down due to\n" \
    "loss of power.\n\n" \
    "If the system does not have battery backup, then this\n" \
    "message is displayed after power has already been\n" \
    "restored and your system rebooted successfully.\n\n" \
    "Check the following for the cause of lost input power:\n" \
    "  1. Loose or disconnected power source connections.\n" \
    "  2. Loss of site power.\n" \
    "  3. For multiple enclosure systems, loose or\n" \
    "     disconnected power and/or signal connections\n "\
    "     between enclosures."

#define MSG_BYPASS "The following resource is unavailable due to an error. "\
    "System is operating in degraded mode."

#define MSGMENUG_REFC "\n\nSupporting data:\nRef. Code:\t%X"

#define MSGMENUG_LOC "\nLocation Codes:\t%s"

#define MSGMENUG206 "651206 ANALYZING SYSTEM ERROR LOG\n" \
    "The system shutdown because the battery backup\n" \
    "lost power. The system was running on battery\n" \
    "backup because of loss of input power."
    
#define MSGMENUGPEL_ERROR "651300 ANALYZING SYSTEM ERROR LOG\n" \
    "\nThe platform reports the following error:\n\n"

#define MSGMENUG_SRC "\n\nSupporting data:\n\nSRC: %s\n"

#define MSGMENUG_PRIORITY "Priority: %c %s\n"

#define MSGMENUG_FRU	  "FRU: %s"
#define MSGMENUG_EXT_FRU  "External FRU: %s"
#define MSGMENUG_CODEFRU  "Code FRU Procedure: %s"
#define MSGMENUG_EXT_CODEFRU  "External Code FRU Procedure: %s"
#define MSGMENUG_TOOL_FRU "Tool FRU: %s"
#define MSGMENUG_SYM_FRU  "Symbolic FRU Id: %s"
#define MSGMENUG_CFG  	  "Configuration Procedure Id: %s"
#define MSGMENUG_MAINT	  "Maintenance Procedure Id: %s"
#define MSGMENUG_RESERVED "Reserved FRU Type: %s"

#define MSGMENUG_SERIAL	  " Serial Number: %s"
#define MSGMENUG_CCIN  	  " CCIN: %s"

#define MSGMENUG_LOCATION "\nLocation: %s\n"

#define MSGMENUGPEL_EVENT "651301 ANALYZING PLATFORM EVENT\n\n"

#define V6_INVALID_SUBID "Invalid subsystem ID, cannot determine message."

#define V6_RESERVED_SUBID "Reserved subsystem ID, cannot determine message."

#define V6_ER_10_00 "Processor subsystem including internal cache " \
    "Informational (non-error) Event. Refer to the system service " \
    "documentation for more information."
    
#define V6_ER_10_10 "Processor subsystem including internal cache " \
    "Recovered Error, general. Refer to the system service documentation " \
    "for more information."
    
#define V6_ER_10_20 "Processor subsystem including internal cache " \
    "Predictive Error, general. Refer to the system service documentation " \
    "for more information."

#define V6_ER_10_21 "Processor subsystem including internal cache " \
    "Predictive Error, degraded performance. Refer to the system service " \
    "documentation for more information."

#define V6_ER_10_22 "Processor subsystem including internal cache " \
    "Predictive Error, fault may be corrected after platform re-IPL. "\
    "Refer to the system service documentation for more information."
    
#define V6_ER_10_23 "Processor subsystem including internal cache " \
    "Predictive Error, fault may be corrected after IPL, degraded " \
    "performance. Refer to the system service documentation for more " \
    "information."

#define V6_ER_10_24 "Processor subsystem including internal cache " \
    "Predictive Error, loss of redundancy. Refer to the system service " \
    "documentation for more information."

#define V6_ER_10_40 "Processor subsystem including internal cache " \
    "Unrecovered Error, general. Refer to the system service documentation " \
    "for more information."
    
#define V6_ER_10_41 "Processor subsystem including internal cache " \
    "Unrecovered Error, bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_10_44 "Processor subsystem including internal cache " \
    "Unrecovered Error, bypassed with loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_10_45 "Processor subsystem including internal cache " \
    "Unrecovered Error, bypassed with loss of redundancy and performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_10_48 "Processor subsystem including internal cache " \
    "Unrecovered Error, bypassed with loss of function. Refer to the " \
    "system service documentation for more information."

#define V6_ER_10_60 "Processor subsystem including internal cache " \
    "Error on diagnostic test, general. Refer to the system service " \
    "documentation for more information."

#define V6_ER_10_61 "Processor subsystem including internal cache " \
    "Error on diagnostic test, resource may produce incorrect result. "\
    "Refer to the system service documentation for more information."

#define V6_ER_20_00 "Memory subsystem including external cache " \
    "Informational (non-error) Event. "\
    "Refer to the system service documentation for more information."

#define V6_ER_20_10 "Memory subsystem including external cache " \
     "Recovered Error, general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_20_20 "Memory subsystem including external cache " \
    "Predictive Error, general. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_20_21 "Memory subsystem including external cache " \
    "Predictive Error, degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_20_22 "Memory subsystem including external cache " \
    "Predictive Error, fault may be corrected after platform re-IPL. "\
    "Refer to the system service documentation for more information."

#define V6_ER_20_23 "Memory subsystem including external cache " \
    "Predictive Error, fault may be corrected after IPL, degraded " \
    "performance. Refer to the system service documentation for more " \
    "information."

#define V6_ER_20_24 "Memory subsystem including external cache " \
    "Predictive Error, loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_20_40 "Memory subsystem including external cache " \
    "Unrecovered Error, general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_20_41 "Memory subsystem including external cache " \
    "Unrecovered Error, bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_20_44 "Memory subsystem including external cache " \
    "Unrecovered Error, bypassed with loss of redundancy. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_20_45 "Memory subsystem including external cache " \
    "Unrecovered Error, bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_20_48 "Memory subsystem including external cache " \
    "Unrecovered Error, bypassed with loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_20_60 "Memory subsystem including external cache " \
    "Error on diagnostic test, general. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_20_61 "Memory subsystem including external cache Error on " \
    "diagnostic test, resource may produce incorrect result. "\
    "Refer to the system service documentation for more information."
    
#define V6_ER_30_00 "I/O subsystem (hub, bridge, bus) Informational " \
    "(non-error) Event. " \
    "Refer to the system service documentation for more information."

#define V6_ER_30_10 "I/O subsystem (hub, bridge, bus) Recovered " \
    "Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_30_20 "I/O subsystem (hub, bridge, bus) Predictive " \
    "Error, general. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_30_21 "I/O subsystem (hub, bridge, bus) Predictive Error, " \
    "degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_30_22 "I/O subsystem (hub, bridge, bus) Predictive Error, " \
    "fault may be corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_30_23 "I/O subsystem (hub, bridge, bus) Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_30_24 "I/O subsystem (hub, bridge, bus) Predictive Error, " \
    "loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_30_40 "I/O subsystem (hub, bridge, bus) Unrecovered Error, " \
    "general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_30_41 "I/O subsystem (hub, bridge, bus) Unrecovered Error, " \
    "bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_30_44 "I/O subsystem (hub, bridge, bus) Unrecovered Error, " \
    "bypassed with loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_30_45 "I/O subsystem (hub, bridge, bus) Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_30_48 "I/O subsystem (hub, bridge, bus) Unrecovered Error, " \
    "bypassed with loss of function. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_30_60 "I/O subsystem (hub, bridge, bus) Error on " \
    "diagnostic test, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_30_61 "I/O subsystem (hub, bridge, bus) Error on " \
    "diagnostic test, resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_40_00 "I/O adapter, device and peripheral Informational " \
    "(non-error) Event. " \
    "Refer to the system service documentation for more information."

#define V6_ER_40_10 "I/O adapter, device and peripheral Recovered Error, " \
    "general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_40_20 "I/O adapter, device and peripheral Predictive Error, " \
    "general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_40_21 "I/O adapter, device and peripheral Predictive Error, " \
    "degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_40_22 "I/O adapter, device and peripheral Predictive Error, " \
    "fault may be corrected after platform re-IPL. "\
    "Refer to the system service documentation for more information."

#define V6_ER_40_23 "I/O adapter, device and peripheral Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_40_24 "I/O adapter, device and peripheral Predictive Error, " \
    "loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_40_40 "I/O adapter, device and peripheral Unrecovered Error, " \
    "general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_40_41 "I/O adapter, device and peripheral Unrecovered Error, " \
    "bypassed with degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_40_44 "I/O adapter, device and peripheral Unrecovered Error, " \
    "bypassed with loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_40_45 "I/O adapter, device and peripheral Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_40_48 "I/O adapter, device and peripheral Unrecovered Error, " \
    "bypassed with loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_40_60 "I/O adapter, device and peripheral Error on diagnostic " \
    "test, general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_40_61 "I/O adapter, device and peripheral Error on diagnostic " \
    "test, resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_00 "CEC hardware Informational (non-error) Event. "\
    "Refer to the system service documentation for more information."

#define V6_ER_50_10 "CEC hardware Recovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_20 "CEC hardware Predictive Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_21 "CEC hardware Predictive Error, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_22 "CEC hardware Predictive Error, fault may be " \
    "corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_23 "CEC hardware Predictive Error, fault may be " \
    "corrected after IPL, degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_50_24 "CEC hardware Predictive Error, loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_40 "CEC hardware Unrecovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_41 "CEC hardware Unrecovered Error, bypassed with " \
    "degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_44 "CEC hardware Unrecovered Error, bypassed with " \
    "loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_50_45 "CEC hardware Unrecovered Error, bypassed with " \
    "loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_48 "CEC hardware Unrecovered Error, bypassed with " \
    "loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_60 "CEC hardware Error on diagnostic test, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_50_61 "CEC hardware Error on diagnostic test, resource " \
    "may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_60_00 "Power/Cooling subsystem Informational (non-error) Event."\
    "Refer to the system service documentation for more information."

#define V6_ER_60_10 "Power/Cooling subsystem Recovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_60_20 "Power/Cooling subsystem Predictive Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_60_21 "Power/Cooling subsystem Predictive Error, " \
    "degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_60_22 "Power/Cooling subsystem Predictive Error, " \
    "fault may be corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."

#define V6_ER_60_23 "Power/Cooling subsystem Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_60_24 "Power/Cooling subsystem Predictive Error, " \
    "loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_60_40 "Power/Cooling subsystem Unrecovered Error, general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_60_41 "Power/Cooling subsystem Unrecovered Error, " \
    "bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_60_44 "Power/Cooling subsystem Unrecovered Error, " \
    "bypassed with loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_60_45 "Power/Cooling subsystem Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_60_48 "Power/Cooling subsystem Unrecovered Error, " \
    "bypassed with loss of function. "\
    "Refer to the system service documentation for more information."

#define V6_ER_60_60 "Power/Cooling subsystem Error on diagnostic test, " \
    "general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_60_61 "Power/Cooling subsystem Error on diagnostic test, " \
    "resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_00 "Other subsystem Informational (non-error) Event. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_10 "Other subsystem Recovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_20 "Other subsystem Predictive Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_21 "Other subsystem Predictive Error, degraded performance."\
    "Refer to the system service documentation for more information."

#define V6_ER_70_22 "Other subsystem Predictive Error, " \
    "fault may be corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_23 "Other subsystem Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_70_24 "Other subsystem Predictive Error, loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_70_40 "Other subsystem Unrecovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_41 "Other subsystem Unrecovered Error, " \
    "bypassed with degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_70_44 "Other subsystem Unrecovered Error, " \
    "bypassed with loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_45 "Other subsystem Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_70_48 "Other subsystem Unrecovered Error, " \
    "bypassed with loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_60 "Other subsystem Error on diagnostic test, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_70_61 "Other subsystem Error on diagnostic test, " \
    "resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_00 "Surveillance Error Informational (non-error) Event. "\
    "Refer to the system service documentation for more information."

#define V6_ER_7A_10 "Surveillance Error Recovered Error, general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_7A_20 "Surveillance Error Predictive Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_21 "Surveillance Error Predictive Error, " \
    "degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_22 "Surveillance Error Predictive Error, " \
    "fault may be corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_23 "Surveillance Error Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_24 "Surveillance Error Predictive Error, " \
    "loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_40 "Surveillance Error Unrecovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_41 "Surveillance Error Unrecovered Error, " \
    "bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_44 "Surveillance Error Unrecovered Error, " \
    "bypassed with loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_7A_45 "Surveillance Error Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_48 "Surveillance Error Unrecovered Error, " \
    "bypassed with loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_60 "Surveillance Error Error on diagnostic test, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_7A_61 "Surveillance Error Error on diagnostic test, " \
    "resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_00 "Platform Firmware Informational (non-error) Event. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_10 "Platform Firmware Recovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_20 "Platform Firmware Predictive Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_21 "Platform Firmware Predictive Error, " \
    "degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_22 "Platform Firmware Predictive Error, " \
    "fault may be corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_23 "Platform Firmware Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_24 "Platform Firmware Predictive Error, loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_80_40 "Platform Firmware Unrecovered Error, general. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_80_41 "Platform Firmware Unrecovered Error, " \
    "bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_44 "Platform Firmware Unrecovered Error, " \
    "bypassed with loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_45 "Platform Firmware Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_48 "Platform Firmware Unrecovered Error, " \
    "bypassed with loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_60 "Platform Firmware Error on diagnostic test, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_80_61 "Platform Firmware Error on diagnostic test, " \
    "resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_00 "Software Informational (non-error) Event. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_10 "Software Recovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_20 "Software Predictive Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_21 "Software Predictive Error, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_22 "Software Predictive Error, " \
    "fault may be corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_23 "Software Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_24 "Software Predictive Error, loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_40 "Software Unrecovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_41 "Software Unrecovered Error, " \
    "bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_44 "Software Unrecovered Error, "\
    "bypassed with loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_45 "Software Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_48 "Software Unrecovered Error, " \
    "bypassed with loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_90_60 "Software Error on diagnostic test, general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_90_61 "Software Error on diagnostic test, " \
    "resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_00 "External environment Informational (non-error) Event. "\
    "Refer to the system service documentation for more information."

#define V6_ER_A0_10 "External environment Recovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_20 "External environment Predictive Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_21 "External environment Predictive Error, " \
    "degraded performance. "\
    "Refer to the system service documentation for more information."

#define V6_ER_A0_22 "External environment Predictive Error, " \
    "fault may be corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_23 "External environment Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_24 "External environment Predictive Error, " \
    "loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_40 "External environment Unrecovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_41 "External environment Unrecovered Error, " \
    "bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_44 "External environment Unrecovered Error, " \
    "bypassed with loss of redundancy. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_45 "External environment Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_48 "External environment Unrecovered Error, " \
    "bypassed with loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_A0_60 "External environment Error on diagnostic test, general. "\
    "Refer to the system service documentation for more information."

#define V6_ER_A0_61 "External environment Error on diagnostic test, " \
    "resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_00 "Reserved Informational (non-error) Event. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_10 "Reserved Recovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_20 "Reserved Predictive Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_21 "Reserved Predictive Error, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_22 "Reserved Predictive Error, " \
    "fault may be corrected after platform re-IPL. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_23 "Reserved Predictive Error, " \
    "fault may be corrected after IPL, degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_24 "Reserved Predictive Error, loss of redundancy. "\
    "Refer to the system service documentation for more information."

#define V6_ER_B0_40 "Reserved Unrecovered Error, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_41 "Reserved Unrecovered Error, " \
    "bypassed with degraded performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_44 "Reserved Unrecovered Error, " \
    "bypassed with loss of redundancy. " \
    "Refer to the system service documentation for more information."
    
#define V6_ER_B0_45 "Reserved Unrecovered Error, " \
    "bypassed with loss of redundancy and performance. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_48 "Reserved Unrecovered Error, " \
    "bypassed with loss of function. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_60 "Reserved Error on diagnostic test, general. " \
    "Refer to the system service documentation for more information."

#define V6_ER_B0_61 "Reserved Error on diagnostic test, " \
    "resource may produce incorrect result. " \
    "Refer to the system service documentation for more information."

#define V6_EV_10_00  "Processor subsystem including internal cache Not " \
    "applicable."

#define V6_EV_10_01  "Processor subsystem including internal cache " \
    "Miscellaneous, Information Only."

#define V6_EV_10_08  "Processor subsystem including internal cache " \
    "Dump Notification."

#define V6_EV_10_10  "Processor subsystem including internal cache " \
    "Previously reported error has been corrected by the system."

#define V6_EV_10_20  "Processor subsystem including internal cache " \
    "System resources manually deconfigured by user."

#define V6_EV_10_21  "Processor subsystem including internal cache " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_10_22  "Processor subsystem including internal cache " \
    "Resource deallocation event notification."

#define V6_EV_10_30  "Processor subsystem including internal cache " \
    "Customer environmental problem has returned to normal."

#define V6_EV_10_40  "Processor subsystem including internal cache " \
    "Concurrent Maintenance Event."

#define V6_EV_10_60  "Processor subsystem including internal cache " \
    "Capacity Upgrade Event."

#define V6_EV_10_70  "Processor subsystem including internal cache " \
    "Resource Sparing Event."

#define V6_EV_10_80  "Processor subsystem including internal cache " \
    "Dynamic Reconfiguration Event."

#define V6_EV_10_D0  "Processor subsystem including internal cache " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_10_E0  "Processor subsystem including internal cache " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_20_00  "Memory subsystem including external cache " \
    "Not applicable."

#define V6_EV_20_01  "Memory subsystem including external cache " \
    "Miscellaneous, Information Only."

#define V6_EV_20_08  "Memory subsystem including external cache " \
    "Dump Notification."

#define V6_EV_20_10  "Memory subsystem including external cache " \
    "Previously reported error has been corrected by the system."

#define V6_EV_20_20  "Memory subsystem including external cache " \
    "System resources manually deconfigured by user."

#define V6_EV_20_21  "Memory subsystem including external cache " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_20_22  "Memory subsystem including external cache " \
    "Resource deallocation event notification."

#define V6_EV_20_30  "Memory subsystem including external cache " \
    "Customer environmental problem has returned to normal."

#define V6_EV_20_40  "Memory subsystem including external cache " \
    "Concurrent Maintenance Event."

#define V6_EV_20_60  "Memory subsystem including external cache " \
    "Capacity Upgrade Event."

#define V6_EV_20_70  "Memory subsystem including external cache " \
    "Resource Sparing Event."

#define V6_EV_20_80  "Memory subsystem including external cache " \
    "Dynamic Reconfiguration Event."

#define V6_EV_20_D0  "Memory subsystem including external cache " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_20_E0  "Memory subsystem including external cache " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_30_00  "I/O subsystem (hub, bridge, bus) Not applicable."

#define V6_EV_30_01  "I/O subsystem (hub, bridge, bus) " \
    "Miscellaneous, Information Only."

#define V6_EV_30_08  "I/O subsystem (hub, bridge, bus) " \
    "Dump Notification."

#define V6_EV_30_10  "I/O subsystem (hub, bridge, bus) " \
    "Previously reported error has been corrected by the system."

#define V6_EV_30_20  "I/O subsystem (hub, bridge, bus) " \
    "System resources manually deconfigured by user."

#define V6_EV_30_21  "I/O subsystem (hub, bridge, bus) " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_30_22  "I/O subsystem (hub, bridge, bus) " \
    "Resource deallocation event notification."

#define V6_EV_30_30  "I/O subsystem (hub, bridge, bus) " \
    "Customer environmental problem has returned to normal."

#define V6_EV_30_40  "I/O subsystem (hub, bridge, bus) " \
    "Concurrent Maintenance Event."

#define V6_EV_30_60  "I/O subsystem (hub, bridge, bus) Capacity Upgrade Event."

#define V6_EV_30_70  "I/O subsystem (hub, bridge, bus) Resource Sparing Event."

#define V6_EV_30_80  "I/O subsystem (hub, bridge, bus) " \
    "Dynamic Reconfiguration Event."

#define V6_EV_30_D0  "I/O subsystem (hub, bridge, bus) " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_30_E0  "I/O subsystem (hub, bridge, bus) " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_40_00  "I/O adapter, device and peripheral Not applicable."

#define V6_EV_40_01  "I/O adapter, device and peripheral " \
    "Miscellaneous, Information Only."

#define V6_EV_40_08  "I/O adapter, device and peripheral " \
    "Dump Notification."

#define V6_EV_40_10  "I/O adapter, device and peripheral " \
    "Previously reported error has been corrected by the system."

#define V6_EV_40_20  "I/O adapter, device and peripheral " \
    "System resources manually deconfigured by user."

#define V6_EV_40_21  "I/O adapter, device and peripheral " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_40_22  "I/O adapter, device and peripheral " \
    "Resource deallocation event notification."

#define V6_EV_40_30  "I/O adapter, device and peripheral " \
    "Customer environmental problem has returned to normal."

#define V6_EV_40_40  "I/O adapter, device and peripheral " \
    "Concurrent Maintenance Event."

#define V6_EV_40_60  "I/O adapter, device and peripheral " \
    "Capacity Upgrade Event."

#define V6_EV_40_70  "I/O adapter, device and peripheral " \
    "Resource Sparing Event."

#define V6_EV_40_80  "I/O adapter, device and peripheral " \
    "Dynamic Reconfiguration Event."

#define V6_EV_40_D0  "I/O adapter, device and peripheral " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_40_E0  "I/O adapter, device and peripheral " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_50_00  "CEC hardware Not applicable."
    
#define V6_EV_50_01  "CEC hardware Miscellaneous, Information Only."

#define V6_EV_50_08  "CEC hardware Dump Notification."

#define V6_EV_50_10  "CEC hardware " \
    "Previously reported error has been corrected by the system."

#define V6_EV_50_20  "CEC hardware " \
    "System resources manually deconfigured by user."

#define V6_EV_50_21  "CEC hardware " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_50_22  "CEC hardware Resource deallocation event notification."

#define V6_EV_50_30  "CEC hardware " \
    "Customer environmental problem has returned to normal."

#define V6_EV_50_40  "CEC hardware Concurrent Maintenance Event."

#define V6_EV_50_60  "CEC hardware Capacity Upgrade Event."

#define V6_EV_50_70  "CEC hardware Resource Sparing Event."

#define V6_EV_50_80  "CEC hardware Dynamic Reconfiguration Event."

#define V6_EV_50_D0  "CEC hardware " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_50_E0  "CEC hardware " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_60_00  "Power/Cooling subsystem Not applicable."

#define V6_EV_60_01  "Power/Cooling subsystem Miscellaneous, Information Only."

#define V6_EV_60_08  "Power/Cooling subsystem Dump Notification."

#define V6_EV_60_10  "Power/Cooling subsystem " \
    "Previously reported error has been corrected by the system."

#define V6_EV_60_20  "Power/Cooling subsystem " \
    "System resources manually deconfigured by user."

#define V6_EV_60_21  "Power/Cooling subsystem " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_60_22  "Power/Cooling subsystem " \
    "Resource deallocation event notification."

#define V6_EV_60_30  "Power/Cooling subsystem " \
    "Customer environmental problem has returned to normal."

#define V6_EV_60_40  "Power/Cooling subsystem Concurrent Maintenance Event."

#define V6_EV_60_60  "Power/Cooling subsystem Capacity Upgrade Event."

#define V6_EV_60_70  "Power/Cooling subsystem Resource Sparing Event."

#define V6_EV_60_80  "Power/Cooling subsystem Dynamic Reconfiguration Event."

#define V6_EV_60_D0  "Power/Cooling subsystem " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_60_E0  "Power/Cooling subsystem " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_70_00  "Other subsystem Not applicable."

#define V6_EV_70_01  "Other subsystem Miscellaneous, Information Only."

#define V6_EV_70_08  "Other subsystem Dump Notification."

#define V6_EV_70_10  "Other subsystem " \
    "Previously reported error has been corrected by the system."

#define V6_EV_70_20  "Other subsystem " \
    "System resources manually deconfigured by user."

#define V6_EV_70_21  "Other subsystem " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_70_22  "Other subsystem Resource deallocation event notification."

#define V6_EV_70_30  "Other subsystem " \
    "Customer environmental problem has returned to normal."

#define V6_EV_70_40  "Other subsystem Concurrent Maintenance Event."

#define V6_EV_70_60  "Other subsystem Capacity Upgrade Event."

#define V6_EV_70_70  "Other subsystem Resource Sparing Event."

#define V6_EV_70_80  "Other subsystem Dynamic Reconfiguration Event."

#define V6_EV_70_D0  "Other subsystem " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_70_E0  "Other subsystem " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_7A_00  "Surveillance Error Not applicable."

#define V6_EV_7A_01  "Surveillance Error Miscellaneous, Information Only."

#define V6_EV_7A_08  "Surveillance Error Dump Notification."

#define V6_EV_7A_10  "Surveillance Error " \
    "Previously reported error has been corrected by the system."

#define V6_EV_7A_20  "Surveillance Error " \
    "System resources manually deconfigured by user."

#define V6_EV_7A_21  "Surveillance Error " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_7A_22  "Surveillance Error " \
    "Resource deallocation event notification."

#define V6_EV_7A_30  "Surveillance Error " \
    "Customer environmental problem has returned to normal."

#define V6_EV_7A_40  "Surveillance Error Concurrent Maintenance Event."

#define V6_EV_7A_60  "Surveillance Error Capacity Upgrade Event."

#define V6_EV_7A_70  "Surveillance Error Resource Sparing Event."

#define V6_EV_7A_80  "Surveillance Error Dynamic Reconfiguration Event."

#define V6_EV_7A_D0  "Surveillance Error " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_7A_E0  "Surveillance Error " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_80_00  "Platform Firmware Not applicable."

#define V6_EV_80_01  "Platform Firmware Miscellaneous, Information Only."

#define V6_EV_80_08  "Platform Firmware Dump Notification."

#define V6_EV_80_10  "Platform Firmware " \
    "Previously reported error has been corrected by the system."

#define V6_EV_80_20  "Platform Firmware " \
    "System resources manually deconfigured by user."

#define V6_EV_80_21  "Platform Firmware " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_80_22  "Platform Firmware " \
    "Resource deallocation event notification."

#define V6_EV_80_30  "Platform Firmware " \
    "Customer environmental problem has returned to normal."

#define V6_EV_80_40  "Platform Firmware Concurrent Maintenance Event."

#define V6_EV_80_60  "Platform Firmware Capacity Upgrade Event."

#define V6_EV_80_70  "Platform Firmware Resource Sparing Event."

#define V6_EV_80_80  "Platform Firmware Dynamic Reconfiguration Event."

#define V6_EV_80_D0  "Platform Firmware " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_80_E0  "Platform Firmware " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_90_00 "Software Not applicable."

#define V6_EV_90_01 "Software Miscellaneous, Information Only."

#define V6_EV_90_08 "Software Dump Notification."

#define V6_EV_90_10 "Software " \
    "Previously reported error has been corrected by the system."

#define V6_EV_90_20 "Software " \
    "System resources manually deconfigured by user."

#define V6_EV_90_21 "Software " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_90_22 "Software Resource deallocation event notification."

#define V6_EV_90_30 "Software " \
    "Customer environmental problem has returned to normal."

#define V6_EV_90_40 "Software Concurrent Maintenance Event."

#define V6_EV_90_60 "Software Capacity Upgrade Event."

#define V6_EV_90_70 "Software Resource Sparing Event."

#define V6_EV_90_80 "Software Dynamic Reconfiguration Event."

#define V6_EV_90_D0 "Software Normal system/platform shutdown or powered off."

#define V6_EV_90_E0 "Software " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_A0_00 "External environment Not applicable."

#define V6_EV_A0_01 "External environment Miscellaneous, Information Only."

#define V6_EV_A0_08 "External environment Dump Notification."

#define V6_EV_A0_10 "External environment " \
    "Previously reported error has been corrected by the system."

#define V6_EV_A0_20 "External environment " \
    "System resources manually deconfigured by user."

#define V6_EV_A0_21 "External environment " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_A0_22 "External environment " \
    "Resource deallocation event notification."

#define V6_EV_A0_30 "External environment " \
    "Customer environmental problem has returned to normal."

#define V6_EV_A0_40 "External environment Concurrent Maintenance Event."

#define V6_EV_A0_60 "External environment Capacity Upgrade Event."

#define V6_EV_A0_70 "External environment Resource Sparing Event."

#define V6_EV_A0_80 "External environment Dynamic Reconfiguration Event."

#define V6_EV_A0_D0 "External environment " \
    "Normal system/platform shutdown or powered off."

#define V6_EV_A0_E0 "External environment " \
    "Platform powered off by user with normal shutdown."

#define V6_EV_B0_00 "Reserved Not applicable."

#define V6_EV_B0_01 "Reserved Miscellaneous, Information Only."

#define V6_EV_B0_08 "Reserved Dump Notification."

#define V6_EV_B0_10 "Reserved " \
    "Previously reported error has been corrected by the system."

#define V6_EV_B0_20 "Reserved System resources manually deconfigured by user."

#define V6_EV_B0_21 "Reserved " \
    "System resources deconfigured by system due to prior error event."

#define V6_EV_B0_22 "Reserved Resource deallocation event notification."

#define V6_EV_B0_30 "Reserved " \
    "Customer environmental problem has returned to normal."

#define V6_EV_B0_40 "Reserved Concurrent Maintenance Event."

#define V6_EV_B0_60 "Reserved Capacity Upgrade Event."

#define V6_EV_B0_70 "Reserved Resource Sparing Event."

#define V6_EV_B0_80 "Reserved Dynamic Reconfiguration Event."

#define V6_EV_B0_D0 "Reserved Normal system/platform shutdown or powered off."

#define V6_EV_B0_E0 "Reserved " \
    "Platform powered off by user with normal shutdown."

#endif
