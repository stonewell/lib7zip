bin_PROGRAMS = test7zip test7zipmulti test7zprops

test7zip_SOURCES = \
	stdafx.cpp  stdafx.h  Test7Zip.cpp

test7zipmulti_SOURCES = \
	stdafx.cpp stdafx.h Test7ZipMulti.cpp

test7zprops_SOURCES = \
	stdafx.cpp stdafx.h test7zipprops.cpp

test7zip_LDADD = ../Lib7Zip/lib7zip.a \
	-ldl

test7zipmulti_LDADD = ../Lib7Zip/lib7zip.a \
	-ldl

test7zprops_LDADD = ../Lib7Zip/lib7zip.a \
	-ldl

EXTRA_DIST = Test7Zip.vcproj
