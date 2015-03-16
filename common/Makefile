#
# Makefile for ppc64-diag/common
#

include ../rules.mk

SRC = platform.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install:

uninstall:

clean:
	@echo "Cleaning up $(WORK_DIR) files..."
	@rm -f $(OBJ)
