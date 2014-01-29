PHP_ARG_WITH(spidermonkey, whether to enable spidermonkey support,
[  --with-spidermonkey[=DIR]     Enable spidermonkey support])

if test "$PHP_SPIDERMONKEY" != "no"; then
  for i in $PHP_SPIDERMONKEY /usr/local /usr; do
  	test -f $i/bin/js24-config && SPIDERMONKEY_CONFIG=$i/bin/js24-config && break
  done

  if test -z "$SPIDERMONKEY_CONFIG"; then
    AC_MSG_ERROR(js24-config not found. Please reinstall mozjs.)
  fi
  
  SPIDERMONKEY_INCLUDE=`$SPIDERMONKEY_CONFIG --includedir`
  SPIDERMONKEY_LIBDIR=`$SPIDERMONKEY_CONFIG --libdir`
  SPIDERMONKEY_CFLAGS=`$SPIDERMONKEY_CONFIG --cflags`

  PHP_REQUIRE_CXX()
  PHP_ADD_LIBRARY_WITH_PATH(mozjs-24, $SPIDERMONKEY_LIBDIR, SPIDERMONKEY_SHARED_LIBADD)
  PHP_ADD_INCLUDE($SPIDERMONKEY_INCLUDE)
  PHP_SUBST(SPIDERMONKEY_SHARED_LIBADD)

  PHP_NEW_EXTENSION(spidermonkey, spidermonkey.cc spidermonkey_context.cc spidermonkey_external.cc, $ext_shared,,$SPIDERMONKEY_CFLAGS -Wno-write-strings -Wno-invalid-offsetof)
fi

