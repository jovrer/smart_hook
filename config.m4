dnl $Id$
dnl config.m4 for extension smart_hook

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(smart_hook, for smart_hook support,
dnl Make sure that the comment is aligned:
dnl [  --with-smart_hook             Include smart_hook support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(smart_hook, whether to enable smart_hook support,
dnl Make sure that the comment is aligned:
[  --enable-smart_hook           Enable smart_hook support])

if test "$PHP_SMART_HOOK" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-smart_hook -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/smart_hook.h"  # you most likely want to change this
  dnl if test -r $PHP_SMART_HOOK/$SEARCH_FOR; then # path given as parameter
  dnl   SMART_HOOK_DIR=$PHP_SMART_HOOK
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for smart_hook files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       SMART_HOOK_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$SMART_HOOK_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the smart_hook distribution])
  dnl fi

  dnl # --with-smart_hook -> add include path
  dnl PHP_ADD_INCLUDE($SMART_HOOK_DIR/include)

  dnl # --with-smart_hook -> check for lib and symbol presence
  dnl LIBNAME=smart_hook # you may want to change this
  dnl LIBSYMBOL=smart_hook # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $SMART_HOOK_DIR/$PHP_LIBDIR, SMART_HOOK_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_SMART_HOOKLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong smart_hook lib version or lib not found])
  dnl ],[
  dnl   -L$SMART_HOOK_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(SMART_HOOK_SHARED_LIBADD)

  PHP_NEW_EXTENSION(smart_hook, smart_hook.c, $ext_shared)
fi
