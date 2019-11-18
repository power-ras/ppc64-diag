#!/bin/bash

set -uo pipefail
set -e
set -vx
MAKE_J=$(grep -c processor /proc/cpuinfo)


./autogen.sh
./configure
make dist-gzip
mkdir -p /root/rpmbuild/SOURCES/
cp ppc64-diag-*.tar.gz /root/rpmbuild/SOURCES/
rpmbuild -ba ppc64-diag.spec
