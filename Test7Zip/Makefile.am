bin_PROGRAMS = test7zip test7zipmulti test7zprops test7zcrypt test7zipsig test7ziprar5 test7zip2

test7zip_SOURCES = \
	stdafx.cpp  stdafx.h  Test7Zip.cpp
test7zip2_SOURCES = \
	stdafx.cpp  stdafx.h  Test7Zip2.cpp

test7zipmulti_SOURCES = \
	stdafx.cpp stdafx.h Test7ZipMulti.cpp

test7zprops_SOURCES = \
	stdafx.cpp stdafx.h test7zipprops.cpp

test7zcrypt_SOURCES = \
	stdafx.cpp stdafx.h Test7ZipCryptFileName.cpp

test7zipsig_SOURCES = \
	stdafx.cpp  stdafx.h  Test7ZipSignature.cpp

test7ziprar5_SOURCES = \
	stdafx.cpp  stdafx.h  Test7ZipRar5.cpp
if WIN32
A_LIBS = -loleaut32 -luuid
else
A_LIBS = -ldl
endif

test7zip_LDADD = ../Lib7Zip/lib7zip.a \
	$(A_LIBS)

test7zip2_LDADD = ../Lib7Zip/lib7zip.a \
	$(A_LIBS)

test7zipsig_LDADD = ../Lib7Zip/lib7zip.a \
	$(A_LIBS)

test7zipmulti_LDADD = ../Lib7Zip/lib7zip.a \
	$(A_LIBS)

test7zprops_LDADD = ../Lib7Zip/lib7zip.a \
	$(A_LIBS)

test7zcrypt_LDADD = ../Lib7Zip/lib7zip.a \
	$(A_LIBS)

test7ziprar5_LDADD = ../Lib7Zip/lib7zip.a \
	$(A_LIBS)

EXTRA_DIST = 
