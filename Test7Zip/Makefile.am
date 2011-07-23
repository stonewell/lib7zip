bin_PROGRAMS = test7zip test7zipmulti

test7zip_SOURCES = \
	stdafx.cpp  stdafx.h  Test7Zip.cpp

test7zipmulti_SOURCES = \
	stdafx.cpp stdafx.h Test7ZipMulti.cpp

test7zip_LDADD = ../Lib7Zip/lib7zip.a \
	-ldl

test7zipmulti_LDADD = ../Lib7Zip/lib7zip.a \
	-ldl

EXTRA_DIST = Test7Zip.vcproj
