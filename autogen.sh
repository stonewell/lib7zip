#!/bin/sh
aclocal
automake -a
autoconf
./configure $*
