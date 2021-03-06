/*
   +----------------------------------------------------------------------+
   | ZoeeyPHP Framework                                                   |
   +----------------------------------------------------------------------+
   | Copyright (c) 2011 moxie(system128@gmail.com)                        |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
 */

#ifndef ZE_HELPER_C
#define ZE_HELPER_C

#include "php.h"
#include "php_ini.h"
#include "php_zoeey.h"

#include "helper.h"


PHP_ZOEEY_API void ze_error(int type TSRMLS_DC, const char * name, ... ) {
    va_list ap                ;
    int     len             = 0;
    char *  buf             = NULL;
    char *  msg             = NULL;

    if (EG(exception)) {
        zend_throw_exception_object(EG(exception) TSRMLS_CC);
        return;
    }

    va_start(ap, name);
    len = vspprintf(&buf, 0, name, ap);
    va_end(ap);
    spprintf(&msg, 0, INI_STR("errors_doc_url"), buf);

    php_error(type, msg);
    efree(msg);
    efree(buf);
}


PHP_ZOEEY_API zval * ze_call_method(zval **object_pp, zend_class_entry *obj_ce
                             , zend_function **fn_proxy, char *function_name
                             , int function_name_len, zval **retval_ptr_ptr
                             , int param_count, zval *** params TSRMLS_DC) {

        int result;
        zend_fcall_info fci;
        zval z_fname;
        zval *retval;
        HashTable *function_table;

        fci.size = sizeof(fci);
        /*fci.function_table = NULL; will be read form zend_class_entry of object if needed */
        #if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION == 2
        fci.object_pp = object_pp;
        #elif PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION == 3
        fci.object_ptr = object_pp ? *object_pp : NULL;
        #endif

        fci.function_name = &z_fname;
        fci.retval_ptr_ptr = retval_ptr_ptr ? retval_ptr_ptr : &retval;
        fci.param_count = param_count;
        fci.params = params;
        fci.no_separation = 1;
        fci.symbol_table = NULL;

        if (!fn_proxy && !obj_ce) {
                /* no interest in caching and no information already present that is
                 * needed later inside zend_call_function. */
                ZVAL_STRINGL(&z_fname, function_name, function_name_len, 0);
                fci.function_table = !object_pp ? EG(function_table) : NULL;
                result = zend_call_function(&fci, NULL TSRMLS_CC);
        } else {
                zend_fcall_info_cache fcic;

                fcic.initialized = 1;
                if (!obj_ce) {
                        obj_ce = object_pp ? Z_OBJCE_PP(object_pp) : NULL;
                }
                if (obj_ce) {
                        function_table = &obj_ce->function_table;
                } else {
                        function_table = EG(function_table);
                }
                if (!fn_proxy || !*fn_proxy) {
                        if (zend_hash_find(function_table, function_name, function_name_len+1, (void **) &fcic.function_handler) == FAILURE) {
                                /* error at c-level */
                                zend_error(E_CORE_ERROR, "Couldn't find implementation for method %s%s%s", obj_ce ? obj_ce->name : "", obj_ce ? "::" : "", function_name);
                        }
                        if (fn_proxy) {
                                *fn_proxy = fcic.function_handler;
                        }
                } else {
                        fcic.function_handler = *fn_proxy;
                }
                fcic.calling_scope = obj_ce;

                #if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION == 2
                fcic.object_pp = object_pp;
                #elif PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION == 3
                if (object_pp) {
                        fcic.called_scope = Z_OBJCE_PP(object_pp);
                } else if (obj_ce &&
                        !(EG(called_scope) &&
                        instanceof_function(EG(called_scope), obj_ce TSRMLS_CC)))
                {
                        fcic.called_scope = obj_ce;
                } else {
                        fcic.called_scope = EG(called_scope);
                }
                fcic.object_ptr = object_pp ? *object_pp : NULL;
                #endif
                result = zend_call_function(&fci, &fcic TSRMLS_CC);
        }
        if (result == FAILURE) {
                /* error at c-level */
                if (!obj_ce) {
                        obj_ce = object_pp ? Z_OBJCE_PP(object_pp) : NULL;
                }
                if (!EG(exception)) {
                        zend_error(E_CORE_ERROR, "Couldn't execute method %s%s%s", obj_ce ? obj_ce->name : "", obj_ce ? "::" : "", function_name);
                }
        }
        if (!retval_ptr_ptr) {
                if (retval) {
                        zval_ptr_dtor(&retval);
                }
                return NULL;
        }
        return *retval_ptr_ptr;
}

#endif /* ZE_HELPER_C */
