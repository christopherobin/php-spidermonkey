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

JSBool js_stream_read(JSContext *ctx, unsigned argc, JS::Value *vp)
{
	TSRMLS_FETCH();
	PHPJS_START(ctx);

	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*cls;
	JSObject				*obj  = JS_THIS_OBJECT(ctx, vp);
	jsval					*argv = JS_ARGV(ctx, vp);
	jsval					*rval = &JS_RVAL(ctx, vp);

	intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
	cls = &intern->script_class;

	if (obj == intern->obj) {
		cls =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(ctx, obj, cls, NULL);

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
			reportError(ctx, "Failed to read stream", NULL);
			PHPJS_END(ctx);
			return JS_FALSE;
		}

		buf = (char*)emalloc(buf_len * sizeof(char));
		memset(buf, 0, buf_len);

		// read from string
		nbytes = php_stream_read(stream, buf, buf_len);

		if (nbytes > 0)	{
			jstr = JS_NewStringCopyN(ctx, buf, nbytes);
			*rval = STRING_TO_JSVAL(jstr);
		}
		else {
			*rval = JSVAL_NULL;
		}

		efree(buf);
	}

	PHPJS_END(ctx);
	return JS_TRUE;
}

/* this native is used to retrieve a line from a stream */
JSBool js_stream_getline(JSContext *ctx, unsigned argc, JS::Value *vp)
{
	TSRMLS_FETCH();
	PHPJS_START(ctx);

	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*cls;
	JSObject				*obj  = JS_THIS_OBJECT(ctx, vp);
	jsval					*argv = JS_ARGV(ctx, vp);
	jsval					*rval = &JS_RVAL(ctx, vp);

	intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
	cls = &intern->script_class;

	if (obj == intern->obj) {
		cls = &intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(ctx, obj, cls, NULL);

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
			// default buffer length is 4K
			buf_len = 4096;
		}
		// fetch php_stream
		php_stream_from_zval_no_verify(stream, &jsref->obj);

		if (stream == NULL) {
			reportError(ctx, "Failed to read stream", NULL);
			PHPJS_END(ctx);
			return JS_FALSE;
		}

		buf = (char*)emalloc(buf_len * sizeof(char));

		// read from string
		if (php_stream_get_line(stream, buf, buf_len, &nbytes) == NULL) {
			nbytes = 0;
		}

		if (nbytes > 0)	{
			jstr = JS_NewStringCopyN(ctx, buf, nbytes);
			*rval = STRING_TO_JSVAL(jstr);
		}
		else {
			*rval = JSVAL_NULL;
		}

		efree(buf);
	}

	PHPJS_END(ctx);

	return JS_TRUE;
}

/* this native is used to seek in a stream */
JSBool js_stream_seek(JSContext *ctx, unsigned argc, JS::Value *vp)
{
	TSRMLS_FETCH();
	PHPJS_START(ctx);

	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*cls;
	JSObject				*obj  = JS_THIS_OBJECT(ctx, vp);
	jsval					*argv = JS_ARGV(ctx,vp);
	jsval					*rval = &JS_RVAL(ctx,vp);

	intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
	cls = &intern->script_class;

	if (obj == intern->obj) {
		cls =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(ctx, obj, cls, NULL);

	if (jsref != NULL && jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_RESOURCE && argc >= 1) {
		off_t	pos;
		int		whence;

		// fetch php_stream
		php_stream_from_zval_no_verify(stream, &jsref->obj);

		if (stream == NULL) {
			reportError(ctx, "Failed to access stream", NULL);
			PHPJS_END(ctx);
			return JS_FALSE;
		}

		pos = JSVAL_TO_INT(argv[0]);

		if (argc >= 2) {
			whence = JSVAL_TO_INT(argv[1]);
		}
		else {
			// default buffer length is 4K
			whence = SEEK_SET;
		}

		php_stream_seek(stream, pos, whence);

		// this function doesn't return anything
		*rval = JSVAL_VOID;
	}

	PHPJS_END(ctx);

	return JS_TRUE;
}

/* this native is used for writing to a stream */
JSBool js_stream_write(JSContext *ctx, unsigned argc, JS::Value *vp)
{
	TSRMLS_FETCH();
	PHPJS_START(ctx);

	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*cls;
	JSObject				*obj  = JS_THIS_OBJECT(ctx, vp);
	jsval					*argv = JS_ARGV(ctx,vp);
	jsval					*rval = &JS_RVAL(ctx,vp);

	intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
	cls = &intern->script_class;

	if (obj == intern->obj) {
		cls =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(ctx, obj, cls, NULL);

	if (jsref != NULL && jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_RESOURCE && argc >= 1) {
		JSString	*jstr;
		size_t		buf_len, nbytes;

		// fetch php_stream
		php_stream_from_zval_no_verify(stream, &jsref->obj);

		if (stream == NULL) {
			reportError(ctx, "Failed to write to stream", NULL);
			PHPJS_END(ctx);
			return JS_FALSE;
		}

		jstr = JS_ValueToString(ctx, argv[0]);
		if (jstr != NULL) {
			// then we retrieve the pointer to the string
			char *txt = JS_EncodeString(ctx, jstr);

			if (argc >= 2)
			{
				buf_len = JSVAL_TO_INT(argv[1]);
				nbytes = php_stream_write(stream, txt, buf_len);
			}
			else
			{
				nbytes = php_stream_write_string(stream, txt);
			}

			JS_free(ctx, txt);
		}
		else {
			reportError(ctx, "Failed to convert type to string", NULL);
			PHPJS_END(ctx);
			return JS_FALSE;
		}

		JS_SET_RVAL(ctx, rval, JS_NumberValue(nbytes));
	}

	PHPJS_END(ctx);

	return JS_TRUE;
}

/* this native is used for telling the position in the file */
JSBool js_stream_tell(JSContext *ctx, unsigned argc, JS::Value *vp)
{
	TSRMLS_FETCH();
	PHPJS_START(ctx);

	php_jscontext_object	*intern;
	php_jsobject_ref		*jsref;
	php_stream				*stream = NULL;
	JSClass					*cls;
	JSObject				*obj  = JS_THIS_OBJECT(ctx, vp);
	jsval					*argv = JS_ARGV(ctx,vp);
	jsval					*rval = &JS_RVAL(ctx,vp);

	intern = (php_jscontext_object*)JS_GetContextPrivate(ctx);
	cls = &intern->script_class;

	if (obj == intern->obj) {
		cls =&intern->global_class;
	}

	jsref = (php_jsobject_ref*)JS_GetInstancePrivate(ctx, obj, cls, NULL);

	if (jsref != NULL && jsref->obj != NULL && Z_TYPE_P(jsref->obj) == IS_RESOURCE) {
		off_t	file_pos;

		// fetch php_stream
		php_stream_from_zval_no_verify(stream, &jsref->obj);

		if (stream == NULL) {
			reportError(ctx, "Failed to fetch stream", NULL);
			PHPJS_END(ctx);
			return JS_FALSE;
		}

		file_pos = php_stream_tell(stream);

		// read from string
		JS_SET_RVAL(ctx, rval, JS_NumberValue(file_pos));
	}

	PHPJS_END(ctx);

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
