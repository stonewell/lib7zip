noinst_LIBRARIES = lib7zip.a

AM_CPPFLAGS = -I../includes -I../includes/CPP -I../includes/CPP/myWindows -I../includes/CPP/include_windows

SOURCES = \
	7ZipArchive.cpp  7ZipArchiveItem.cpp  7zipLibrary.cpp  HelperFuncs.cpp  HelperFuncs.h  lib7zip.h \
	7ZipArchiveOpenCallback.cpp \
	7ZipArchiveOpenCallback.h \
	7ZipCodecInfo.cpp \
	7ZipCodecInfo.h \
	7ZipCompressCodecsInfo.cpp \
	7ZipCompressCodecsInfo.h \
	7ZipDllHandler.cpp \
	7ZipDllHandler.h \
	7ZipFormatInfo.cpp \
	7ZipFormatInfo.h \
	7ZipFunctions.h \
	7ZipObjectPtrArray.cpp \
	7ZipOpenArchive.cpp \
	OSFunctions.h \
	OSFunctions_OS2.h \
	OSFunctions_UnixLike.cpp \
	OSFunctions_UnixLike.h \
	OSFunctions_Win32.cpp \
	OSFunctions_Win32.h \
	7ZipInStreamWrapper.cpp \
	7ZipInStreamWrapper.h

NODIST_SOURCES = ../includes/CPP/Common/MyWindows.cpp ../includes/CPP/Common/StdAfx.h \
	../includes/CPP/Windows/PropVariant.cpp ../inclues/CPP/Windows/StdAfx.h

lib7zip_a_SOURCES = $(SOURCES)

nodist_lib7zip_a_SOURCES= $(NODIST_SOURCES)

lib7zip_a_CFLAGS = $(AM_CFLAGS)
lib7zip_a_CPPFLAGS = $(AM_CPPFLAGS)

lib_LTLIBRARIES = lib7zip.la
lib7zip_la_SOURCES = $(SOURCES)

nodist_lib7zip_la_SOURCES= $(NODIST_SOURCES)

# The -no-undefined makes it possible to build DLLs for Windows,
# or shared libraries for Tru64 or AIX (according to the autobook
# chapter on "Portable Library Design"). It doesn't seem to hurt
# elsewhere, so we can leave it in.
# -export-symbols lib7zip.sym \
#LIB7ZIP_VERSION_INFO=1.6.0
# -version-info @LIB7ZIP_VERSION_INFO@
lib7zip_la_LDFLAGS =  -no-undefined


EXTRA_DIST = Lib7Zip.vcproj 

DISTCLEANFILES = ../includes/CPP ../includes/C

dist-hook:
	cp -rf $(srcdir)/Lib7Zip\ -\ VS2008.vcproj $(distdir)

