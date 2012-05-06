noinst_LIBRARIES = lib7zip.a

AM_CPPFLAGS = -I../includes -I../includes/CPP -I../includes/CPP/myWindows -I../includes/CPP/include_windows


lib7zip_a_SOURCES = \
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

nodist_lib7zip_a_SOURCES= ../includes/CPP/Common/MyWindows.cpp ../includes/CPP/Common/StdAfx.h \
	../includes/CPP/Windows/PropVariant.cpp ../inclues/CPP/Windows/StdAfx.h

EXTRA_DIST = Lib7Zip.vcproj 

DISTCLEANFILES = ../includes/CPP ../includes/C

dist-hook:
	cp -rf $(srcdir)/Lib7Zip\ -\ VS2008.vcproj $(distdir)

