/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2009 The PHP Group                                     |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Christophe Robin <crobin@php.net>                            |
  +----------------------------------------------------------------------+

  $Id$
  $Revision$
*/

#include "php_spidermonkey.h"

/* this native is used for read from streams */

#if JS_VERSION < 185
JSBool js_stream_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#else
JSBool js_stream_read(JSContext *cx, uintN argc, jsval *vp)
#endif
{
	#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
	#endif
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*class;
#if JS_VERSION >= 185
	JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);
#endif

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	class = &intern->script_class;

	if (obj == intern->obj) {
		class =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, class, NULL);

	if (jsref != NULL && jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_RESOURCE) {
		JSString	*jstr;
		char		*buf;
		size_t		buf_len, nbytes;

		if (argc >= 1)
		{
			buf_len = JSVAL_TO_INT(argv[0]);
		}
		else
		{
			/* default buffer length is 4K */
			buf_len = 4096;
		}

		/* fetch php_stream */
		php_stream_from_zval_no_verify(stream, &jsref->obj);
		
		if (stream == NULL) {
			reportError(cx, "Failed to read stream", NULL);
			return JS_FALSE;
		}

		buf = emalloc(buf_len * sizeof(char));
		memset(buf, 0, buf_len);

		// read from string
		nbytes = php_stream_read(stream, buf, buf_len);

		if (nbytes > 0)	{
			jstr = JS_NewStringCopyN(cx, buf, nbytes);
			*rval = STRING_TO_JSVAL(jstr);
		}
		else {
			*rval = JSVAL_NULL;
		}
		
		efree(buf);
	}

	return JS_TRUE;
}

/* this native is used to retrieve a line from a stream */
#if JS_VERSION < 185
JSBool js_stream_getline(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#else
JSBool js_stream_getline(JSContext *cx, uintN argc, jsval *vp)
#endif
{
	#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
	#endif
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*class;
#if JS_VERSION >= 185
	JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);
#endif


	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	class = &intern->script_class;

	if (obj == intern->obj) {
		class =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, class, NULL);

	if (jsref != NULL && jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_RESOURCE) {
		JSString	*jstr;
		char		*buf;
		size_t		buf_len, nbytes;
		
		if (argc >= 1)
		{
			buf_len = JSVAL_TO_INT(argv[0]);
		}
		else
		{
			/* default buffer length is 4K */
			buf_len = 4096;
		}
		/* fetch php_stream */
		php_stream_from_zval_no_verify(stream, &jsref->obj);
		
		if (stream == NULL) {
			reportError(cx, "Failed to read stream", NULL);
			return JS_FALSE;
		}

		buf = emalloc(buf_len * sizeof(char));

		// read from string
		if (php_stream_get_line(stream, buf, buf_len, &nbytes) == NULL) {
			nbytes = 0;
		}

		if (nbytes > 0)	{
			jstr = JS_NewStringCopyN(cx, buf, nbytes);
			*rval = STRING_TO_JSVAL(jstr);
		}
		else {
			*rval = JSVAL_NULL;
		}
		
		efree(buf);
	}

	return JS_TRUE;
}

/* this native is used to seek in a stream */
#if JS_VERSION < 185
JSBool js_stream_seek(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#else
JSBool js_stream_seek(JSContext *cx, uintN argc, jsval *vp)
#endif
{
	#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
	#endif
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*class;
#if JS_VERSION >= 185
	JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);
#endif

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	class = &intern->script_class;

	if (obj == intern->obj) {
		class =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, class, NULL);

	if (jsref != NULL && jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_RESOURCE && argc >= 1) {
		off_t	pos;
		int		whence;
		
		/* fetch php_stream */
		php_stream_from_zval_no_verify(stream, &jsref->obj);
		
		if (stream == NULL) {
			reportError(cx, "Failed to access stream", NULL);
			return JS_FALSE;
		}

		pos = JSVAL_TO_INT(argv[0]);

		if (argc >= 2) {
			whence = JSVAL_TO_INT(argv[1]);
		}
		else {
			/* default buffer length is 4K */
			whence = SEEK_SET;
		}

		php_stream_seek(stream, pos, whence);

		/* this function doesn't return anything */
		*rval = JSVAL_VOID;
	}

	return JS_TRUE;
}

/* this native is used for writing to a stream */
#if JS_VERSION < 185
JSBool js_stream_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#else
JSBool js_stream_write(JSContext *cx, uintN argc, jsval *vp)
#endif
{
	#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
	#endif
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*class;
#if JS_VERSION >= 185
	JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);
#endif

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	class = &intern->script_class;

	if (obj == intern->obj) {
		class =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, class, NULL);

	if (jsref != NULL && jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_RESOURCE && argc >= 1) {
		JSString	*jstr;
		size_t		buf_len, nbytes;

		/* fetch php_stream */
		php_stream_from_zval_no_verify(stream, &jsref->obj);
		
		if (stream == NULL) {
			reportError(cx, "Failed to write to stream", NULL);
			return JS_FALSE;
		}

		jstr = JS_ValueToString(cx, argv[0]);
		if (jstr != NULL) {
			/* then we retrieve the pointer to the string */
#if JS_VERSION < 185
			char *txt = JS_GetStringBytes(jstr);
#else
			char *txt = JS_EncodeString(cx, jstr);
#endif
			if (argc >= 2)
			{
				buf_len = JSVAL_TO_INT(argv[1]);
				nbytes = php_stream_write(stream, txt, buf_len);
			}
			else
			{
				nbytes = php_stream_write_string(stream, txt);
			}

#if JS_VERSION >= 185
			JS_free(cx, txt);
#endif
		}
		else {
			reportError(cx, "Failed to convert type to string", NULL);
			return JS_FALSE;
		}
		
		JS_NewNumberValue(cx, nbytes, rval);
	}

	return JS_TRUE;
}

/* this native is used for telling the position in the file */
#if JS_VERSION < 185
JSBool js_stream_tell(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#else
JSBool js_stream_tell(JSContext *cx, uintN argc, jsval *vp)
#endif
{
	#ifdef ZTS
	TSRMLS_FETCH();			/* MSVC9 : NULL statement, C compiler won't allow variable definitions below this line.
								Cannot compile as C++ because conflict with the name 'class' */
	#endif
	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*class;
#if JS_VERSION >= 185
	JSObject				*obj  = JS_THIS_OBJECT(cx, vp);
	jsval					*argv = JS_ARGV(cx,vp);
	jsval					*rval = &JS_RVAL(cx,vp);
#endif

	intern = (php_jscontext_object*)JS_GetContextPrivate(cx);
	class = &intern->script_class;

	if (obj == intern->obj) {
		class =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(cx, obj, class, NULL);

	if (jsref != NULL && jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_RESOURCE) {
		off_t	file_pos;

		/* fetch php_stream */
		php_stream_from_zval_no_verify(stream, &jsref->obj);
		
		if (stream == NULL) {
			reportError(cx, "Failed to fetch stream", NULL);
			return JS_FALSE;
		}

		file_pos = php_stream_tell(stream);

		// read from string
		JS_NewNumberValue(cx, file_pos, rval);
	}

	return JS_TRUE;
}


/*
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim600: fdm=marker
 * vim: noet sw=4 ts=4
 */
