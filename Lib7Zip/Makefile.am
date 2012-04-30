noinst_LIBRARIES = lib7zip.a

AM_CPPFLAGS = -I../includes -I../includes/CPP -I../includes/CPP/myWindows -I../includes/CPP/include_windows


lib7zip_a_SOURCES = \
	7ZipArchive.cpp  7ZipArchiveItem.cpp  7zipLibrary.cpp  HelperFuncs.cpp  HelperFuncs.h  lib7zip.h 
nodist_lib7zip_a_SOURCES= ../includes/CPP/Common/MyWindows.cpp ../includes/CPP/Common/StdAfx.h \
	../includes/CPP/Windows/PropVariant.cpp ../inclues/CPP/Windows/StdAfx.h

EXTRA_DIST = ReadMe.txt Lib7Zip.vcproj 

DISTCLEANFILES = ../includes/CPP ../includes/C

dist-hook:
	cp -rf $(srcdir)/Lib7Zip\ -\ VS2008.vcproj $(distdir)

