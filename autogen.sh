#!/bin/sh
libtoolize --force --copy
aclocal
automake -a
autoconf
./configure $*
