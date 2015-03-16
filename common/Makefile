#
# Makefile for ppc64-diag/common
#
include ../rules.mk

SRC = platform.c utils.c
OBJ = $(SRC:.c=.o)

SUBDIRS = tests

.PHONY: all clean

all: $(OBJ) $(CMD)
	@$(foreach d, $(SUBDIRS), $(MAKE) -C $d;)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install:

uninstall:

clean:
	@$(foreach d, $(SUBDIRS), $(MAKE) -C $d clean;)
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -f $(OBJ)
