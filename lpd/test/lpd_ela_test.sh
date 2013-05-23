#!/bin/bash
################################################################################
# Light Path Diagnostics Test cases
#
# Note:
#	- This script wipes out servicelog database and deletes some of
#	  important log files. *Do not* run this script on production
#	  environment.
#
# USAGE:	./lpd_ela_test.sh <message catalog file>
#
# Author	Vasant Hegde <hegdevasant@linux.vnet.ibm.com>
#
################################################################################
TEST_GROUP="Power Diagnostics"
TEST_SCENARIO="Light Path Diagnostics"

# Takes the name of the script before the "." and composes log filenames
SCRIPT_NAME=$(basename "$0" | cut -f 1 -d ".")
LOG_DIR=`pwd`
EXECUTION_LOG="$LOG_DIR/$SCRIPT_NAME.log"
ERROR_LOG="$LOG_DIR/$SCRIPT_NAME.err"

TMP_DIR="/var/tmp/ras"
mkdir -p $TMP_DIR
MESSAGE_FILE="$TMP_DIR/messages"
TMP_FILE="$TMP_DIR/$SCRIPT_NAME.tmp"

# Initializing the variable that will determine PASS or FAILURE for the script
IS_FAILED=0

################################################################################
# init_logs ()
# Purpose	: Clean up old logs and start the new execution ones
# Parametrs	: None
# Returns	: 0
################################################################################
init_logs ()
{
    # mkdir -p $LOG_DIR
    rm -f $EXECUTION_LOG $ERROR_LOG
    touch $EXECUTION_LOG $ERROR_LOG
    return 0
}

################################################################################
# print_log ()
# Purpose	: Takes the first argument, prints to stdout and to the script
#		  log
# Parameters	: None
# Returns	: 0
################################################################################
print_log ()
{
    echo $1 | tee -a $EXECUTION_LOG
    return 0
}

################################################################################
# exec_log ()
# Purpose	: Executes the command passed as the first argument, prints to
#		  stdout and to the script log
# Parameters	: String with the command you want to execute, log and check
#		  exit code
# Returns	: Same of the command passed as a parameter
################################################################################
exec_log ()
{
    print_log "Output of the command $1:"
    eval $1 1>&2 >$TMP_FILE
    EXIT_CODE=$?
    cat $TMP_FILE | tee -a $EXECUTION_LOG
    rm -f $TMP_FILE

    if [ $EXIT_CODE != 0 ]; then
        print_log "ERROR: Command $1 ended abnormally with exit code $EXIT_CODE."
	print_log "Please verify." | tee -a $ERROR_LOG
        IS_FAILED=1
    fi

    return $EXIT_CODE
}

################################################################################
# clean_servicelog ()
# Purpose	: Truncate the servicelog database, and below files
#			/var/log/platform
#			/var/spool/mail/root
#			/var/log/lp_diag.log
#			/var/log/indicators
# Parameters	: None
# Returns	: The exit code of the servicelog command
################################################################################
clean_servicelog ()
{
    exec_log "servicelog_manage --truncate events --force"
    SLOG_EXIT=$?
    truncate -s 0 /var/log/platform
    truncate -s 0 /var/spool/mail/root
    truncate -s 0 /var/log/lp_diag.log
    truncate -s 0 /var/log/indicators
    return $SLOG_EXIT
}


################################################################################
# print_log_header ()
# Purpose	: Initializes the execution log truncating it
#		  (tee without -a flag)
# Parameters	: None
# Returns	: 0
################################################################################
print_log_header ()
{
cat << TEXT | tee $EXECUTION_LOG
$(date +'%d/%m/%y %H:%M')
********************************************************************************
* PowerLinux RAS Testcase
* $TEST_GROUP
* $TEST_SCENARIO
********************************************************************************

TEXT

return 0
}

################################################################################
# print_log_footer ()
# Purpose	: Checks whether the testcase ended successfuly and prints
#		  its status
# Parameters	: None
# Returns	: 0
################################################################################
print_log_footer ()
{
  if [ $IS_FAILED = 0 ]; then
      TEST_STATUS='PASS'
  else
      TEST_STATUS='FAILED'
      print_log "Showing testcase errors:"
      exec_log "cat $ERROR_LOG"
  fi

cat << TEXT | tee -a $EXECUTION_LOG

********************************************************************************
* PowerLinux RAS Testcase
* $TEST_GROUP
* $TEST_SCENARIO
* Test status: $TEST_STATUS
********************************************************************************
$(date +'%d/%m/%y %H:%M')
TEXT

  return 0
}

################################################################################
# validate_lpd_tool ()
# Purpose	: Check Light Path command and registeration scripts
# Parameters	: None
# Returns	: 0
################################################################################
validate_lpd_tool ()
{
    if [ ! -x /usr/sbin/lp_diag ]
    then
        print_log "lp_diag command does not exist. "
	print_log "Install ppc64-diag package\n"
        IS_FAILED=1
    fi

    event=`servicelog_notify -l |grep "lp_diag_notify \-e"`
    if [ "$event" == "" ]
    then
	print_log "Service event notification script is not registered\n"
	IS_FAILED=1
    fi

    event=`servicelog_notify -l |grep "lp_diag_notify \-r"`
    if [ "$event" == "" ]
    then
	print_log "Repair event notification script is not registered\n"
	IS_FAILED=1
    fi
}

################################################################################
# check_fault_indicator ()
# Purpose	: Check fault indicator status
# Parameters	: ON/OFF
# Returns	: 0
################################################################################
check_fault_indicator ()
{
    location=`servicelog --dump |grep "Location" | awk 'BEGIN{FS=":";}{print $2}'`
    location=`echo $location | sed 's/ *$//g'`
    if [ "$location" == "" ]
    then
	print_log "Empty location code in service event"
	location=`usysattn | head -1 | awk '{print $1}'`
    fi

    state=`echo $1 | awk '{print tolower($0)}'`
    led=`usysattn -l $location -t | tail -1`
    if [ "$led" != "$state" ]
    then
	print_log "Unable to modify fault indicator"
	IS_FAILED=1
	return
    fi

    indicator=`grep "$1" /var/log/indicators | awk 'BEGIN{FS=":";}{print $5}'`

    print_log "$indicator = $led"
}

################################################################################
# close_service_event ()
# Purpose	: Close serviceable event
# Parameters	: None
# Returns	: 0
################################################################################
close_service_event ()
{
    procedure=`servicelog --dump |grep "Procedure Id" | awk 'BEGIN{FS=":";}{print $2}'`
    procedure=`echo $procedure | sed 's/ *$//g'`
    location=`servicelog --dump |grep "Location" | awk 'BEGIN{FS=":";}{print $2}'`
    location=`echo $location | sed 's/ *$//g'`

    exec_log "log_repair_action -q -l \"$location\" -p \"$procedure\""

}

# Validate input file
if [ "$1" != "" ]
then
	if [ -e $1 ]
	then
		INPUT_FILE=$1
	fi
fi
if [ "$INPUT_FILE" == "" ]
then
	echo "Usage : $0 <message catalog file>"
	exit 1
fi

# Initializing test case logs
init_logs

# Printing log header
print_log_header

print_log "1 - Checking lpd tools"
validate_lpd_tool

print_log "2 - Creating messages file"
cp $INPUT_FILE $MESSAGE_FILE

print_log "3 - Running explain_syslog"
exec_log "explain_syslog -m $MESSAGE_FILE"

MESSAGE=$(explain_syslog -m $MESSAGE_FILE | grep "unrecognized message" )
if [ "$MESSAGE" = "" ]; then
    print_log "explain_syslog successfull"
else
    print_log "ERROR: explain_syslog does not explain the message" | tee -a $ERROR_LOG
    IS_FAILED=1
fi

print_log "4 - Clearing servicelog events"
clean_servicelog

print_log "5 - Running syslog_to_svclog - Injecting serviceable event"
exec_log "syslog_to_svclog -m $MESSAGE_FILE"

print_log "6 - Checking events in servicelog"
exec_log "servicelog  --dump"

sleep 2	# Wait for lp_diag to close log file

print_log "7 - Checking fault indicator status"
check_fault_indicator "ON"

print_log "8 - Close service event"
close_service_event

sleep 2	# Wait for lp_diag to close log file

print_log "9 - Checking fault indicator status"
check_fault_indicator "OFF"

print_log_footer
exit $IS_FAILED
