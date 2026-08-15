#serial 1000
dnl @synopsis AX_CHECK_QT6(version, action-if, action-if-not)
dnl
dnl @summary check for Qt6 of sufficient version.
dnl
dnl Defines QT6_LIBS, QT6_CFLAGS
dnl
dnl @category InstalledPackages
dnl @author Sven Hesse <drmccoy@drmccoy.de>
dnl @version 2026-08-15
dnl @license Creative Commons CC0 1.0 Universal Public Domain Dedication

AC_DEFUN([AX_CHECK_QT6], [
	PKG_CHECK_MODULES([QT6], [Qt6Core >= $1 Qt6Gui >= $1 Qt6Widgets >= $1 Qt6Concurrent >= $1], [$2], [$3])

	# Qt6 needs -fPIC...
	QT6_CFLAGS="$QT6_CFLAGS -fPIC"

	AC_SUBST(QT6_CFLAGS)
	AC_SUBST(QT6_LIBS)
])
