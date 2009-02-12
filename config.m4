PHP_ARG_ENABLE(spidermonkey,
  [Whether to enable the "spidermonkey" extension],
  [  --enable-spidermonkey        Enable "spidermonkey" extension support])

if test $PHP_SPIDERMONKEY != "no"; then
  PHP_ADD_LIBRARY_WITH_PATH(js, /usr/lib, SPIDERMONKEY_SHARED_LIBADD)
  PHP_ADD_INCLUDE(/usr/include/js)
  PHP_SUBST(SPIDERMONKEY_SHARED_LIBADD)
  PHP_NEW_EXTENSION(spidermonkey, spidermonkey.c spidermonkey_runtime.c spidermonkey_context.c spidermonkey_object.c spidermonkey_external.c, $ext_shared)
fi

