AC_INIT([lib7zip], [1.4.0], [jingnan.si@gmail.com])
AC_CONFIG_SRCDIR(Lib7Zip/lib7zip.h)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_CXX
AC_C_CONST
AC_C_INLINE
AC_HEADER_STDBOOL

AC_PROG_RANLIB

AC_LANG(C++)

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stddef.h string.h unistd.h dlfcn.h dirent.h])
AC_CHECK_FUNCS([memset memmove])
AC_CHECK_FUNCS([sqrt])

dnl check for p7zip source
AC_ARG_VAR([P7ZIP_SOURCE_DIR], [p7zip source directory, must be set before compile source])

if test "x$P7ZIP_SOURCE_DIR" = "x"; then
AC_MSG_FAILURE(P7ZIP_SOURCE_DIR is not set)
fi

if test ! -d "$P7ZIP_SOURCE_DIR/C"; then
AC_MSG_FAILURE($P7ZIP_SOURCE_DIR/C is invalid)
fi

if test ! -d "$P7ZIP_SOURCE_DIR/CPP"; then
AC_MSG_FAILURE(P7ZIP_SOURCE_DIR is invalid)
fi

dnl create link for p7zip source
AC_CONFIG_COMMANDS([p7zip_src_cpp_link], [ln -sf $P7ZIP_SOURCE_DIR/CPP includes/CPP])
AC_CONFIG_COMMANDS([p7zip_src_c_link], [ln -sf $P7ZIP_SOURCE_DIR/C includes/C])

AC_CONFIG_FILES([
	Makefile
	includes/Makefile
	Lib7Zip/Makefile
	Test7Zip/Makefile
])

AC_OUTPUT
