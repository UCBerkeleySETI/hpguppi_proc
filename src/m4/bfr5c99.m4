# serial 1 bfr5c99.m4
AC_DEFUN([AX_CHECK_BFR5C99],
[AC_PREREQ([2.65])dnl
AC_ARG_WITH([bfr5c99],
            AC_HELP_STRING([--with-bfr5c99=DIR],
                           [Location of BFR5C99 files (/home/mruzinda/bfr5c99/src)]),
            [BFR5C99DIR="$withval"])

AC_CHECK_FILE([${BFR5C99DIR}/include/bfr5.h],
              # Found
              AC_SUBST(BFR5C99_INCDIR,${BFR5C99DIR}/include),
              # Not found there, check BFR5C99DIR
              AC_CHECK_FILE([${BFR5C99DIR}/bfr5.h],
                            # Found
                            AC_SUBST(BFR5C99_INCDIR,${BFR5C99DIR}),
                            # Not found there, error
                            AC_CHECK_FILE([${BFR5C99DIR}/bfr5.h],
                                      # Found
                                      AC_SUBST(BFR5C99_INCDIR,${BFR5C99DIR}),
                                      # Not found there, error
                                      AC_MSG_ERROR([bfr5.h header file not found]))
                            ))

orig_LDFLAGS="${LDFLAGS}"
LDFLAGS="${orig_LDFLAGS} -L${BFR5C99DIR}/lib"
AC_CHECK_LIB([bfr5c99], [BFR5read_all],
             # Found
             AC_SUBST(BFR5C99_LIBDIR,${BFR5C99DIR}/lib),
             # Not found there, check BFR5C99DIR
             AS_UNSET(ac_cv_lib_bfr5c99_BFR5read_all)
             LDFLAGS="${orig_LDFLAGS} -L${BFR5C99DIR}"
             AC_CHECK_LIB([bfr5c99], [BFR5read_all],
                          # Found
                          AC_SUBST(BFR5C99_LIBDIR,${BFR5C99DIR}),
                          # Not found there, error
                          AC_MSG_ERROR([BFR5C99 library not found])))
LDFLAGS="${orig_LDFLAGS}"

])
