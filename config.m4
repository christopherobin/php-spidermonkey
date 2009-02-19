AC_DEFUN([PHP_SPIDERMONKEY_CHECK_VERSION],[
  old_CPPFLAGS=$CPPFLAGS
  CPPFLAGS=-I$SPIDERMONKEY_INCDIR
  AC_MSG_CHECKING(for spidermonkey version)
  if test -f $SPIDERMONKEY_INCDIR/jsconfig.h; then
    SPIDERMONKEY_VERSION_H="jsconfig.h"
  else
    if test -f $SPIDERMONKEY_INCDIR/jsversion.h; then
      SPIDERMONKEY_VERSION_H="jsversion.h"
    fi
  fi
  if test -z "$SPIDERMONKEY_VERSION_H"; then
    AC_MSG_ERROR(unknown version of libjs.)
  else
    AC_EGREP_CPP(yes,[
#include <$SPIDERMONKEY_VERSION_H>
#if JS_VERSION >= 170
  yes
#endif
    ],[
      AC_MSG_RESULT(>= 1.7.0)
    ],[
      AC_MSG_ERROR(libjs version 1.7.0 or greater required.)
    ])
  fi
  CPPFLAGS=$old_CPPFLAGS
])  

PHP_ARG_WITH(spidermonkey, whether to enable spidermonkey support,
[  --with-spidermonkey[=DIR]     Enable spidermonkey support])

if test "$PHP_SPIDERMONKEY" != "no"; then
  for i in $PHP_SPIDERMONKEY /usr/local /usr; do
    for j in js mozjs; do
      test -f $i/include/$j/jsapi.h && SPIDERMONKEY_BASEDIR=$i && SPIDERMONKEY_INCDIR=$i/include/$j && SPIDERMONKEY_LIBNAME=$j && break
    done
    test -f $i/include/$j/jsapi.h && break
  done

  if test -z "$SPIDERMONKEY_INCDIR"; then
    AC_MSG_ERROR(jsapi.h not found. Please reinstall libjs.)
  fi

  PHP_SPIDERMONKEY_CHECK_VERSION

  PHP_ADD_LIBRARY_WITH_PATH($SPIDERMONKEY_LIBNAME, $SPIDERMONKEY_BASEDIR/$PHP_LIBDIR, SPIDERMONKEY_SHARED_LIBADD)
  PHP_ADD_INCLUDE($SPIDERMONKEY_INCDIR)
  PHP_SUBST(SPIDERMONKEY_SHARED_LIBADD)

  PHP_NEW_EXTENSION(spidermonkey, spidermonkey.c spidermonkey_runtime.c spidermonkey_context.c spidermonkey_external.c, $ext_shared)
fi

