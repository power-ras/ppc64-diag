#
#  Makefile for ppc64-diag/diags/test
#
include ../../rules.mk

INCLUDE = -I$(COMMON_DIR) -I../

PROGS = bh_structs bh_mk_healthy bh_mk_hot_power bh_dump_pg2 bh_vpd
SAMPLE_PAGES = bh_healthy.pg2 bh_hot_power.pg2
CFLAGS = -g -Wall
DIAG_ENCL_LIBS = -lservicelog -lrtasevent

all: $(PROGS) $(SAMPLE_PAGES)

bh_mk_healthy: bh_mk_healthy.o test_utils.o bh_healthy.o ../encl_util.o $(COMMON_DIR)/utils.o
	$(CC) $(CFLAGS) $(INCLUDE) -o bh_mk_healthy bh_mk_healthy.o test_utils.o bh_healthy.o ../encl_util.o $(COMMON_DIR)/utils.o $(DIAG_ENCL_LIBS)

bh_mk_hot_power: bh_mk_hot_power.o test_utils.o bh_healthy.o ../encl_util.o $(COMMON_DIR)/utils.o
	$(CC) $(CFLAGS) $(INCLUDE) -o bh_mk_hot_power bh_mk_hot_power.o test_utils.o bh_healthy.o ../encl_util.o $(COMMON_DIR)/utils.o $(DIAG_ENCL_LIBS)

bh_structs: bh_structs.o
	$(CC) $(CFLAGS) $(INCLUDE) -o bh_structs bh_structs.o

bh_healthy.pg2: bh_mk_healthy
	./bh_mk_healthy bh_healthy.pg2

bh_hot_power.pg2: bh_mk_hot_power
	./bh_mk_hot_power bh_hot_power.pg2

bh_dump_pg2: bh_dump_pg2.o test_utils.o ../encl_util.o $(COMMON_DIR)/utils.o
	$(CC) $(CFLAGS) $(INCLUDE) -o bh_dump_pg2 bh_dump_pg2.o test_utils.o ../encl_util.o $(COMMON_DIR)/utils.o $(DIAG_ENCL_LIBS)

bh_vpd: bh_vpd.o ../encl_util.o $(COMMON_DIR)/utils.o
	$(CC) $(CFLAGS) $(INCLUDE) -o bh_vpd bh_vpd.o ../encl_util.o $(COMMON_DIR)/utils.o $(DIAG_ENCL_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

clean:
	rm -f $(PROGS) *.o $(SAMPLE_PAGES)
