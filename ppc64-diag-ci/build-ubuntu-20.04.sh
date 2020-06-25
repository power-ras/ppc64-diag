#!/bin/bash

set -uo pipefail
set -e
set -vx
MAKE_J=$(grep -c processor /proc/cpuinfo)
BUILDDIR=`pwd`

#Build the sources
./autogen.sh
./configure
make -j $MAKE_J
make -j $MAKE_J check

#Run tests
cd $BUILDDIR/opal_errd/ && ./run_tests
cd $BUILDDIR/common && ./run_tests
cd $BUILDDIR/diags/test && ./run_tests
cd $BUILDDIR

#Build rpms
make dist-gzip
