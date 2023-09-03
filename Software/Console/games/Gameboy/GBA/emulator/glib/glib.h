#include "stdint.h"
#include "stdbool.h"

#ifndef GLIB__H
#define GLIB__H

#define __GLIB_H_INSIDE__

#define G_MINFLOAT	FLT_MIN
#define G_MAXFLOAT	FLT_MAX
#define G_MINDOUBLE	DBL_MIN
#define G_MAXDOUBLE	DBL_MAX
#define G_MINSHORT	SHRT_MIN
#define G_MAXSHORT	SHRT_MAX
#define G_MAXUSHORT	USHRT_MAX
#define G_MININT	INT_MIN
#define G_MAXINT	INT_MAX
#define G_MAXUINT	UINT_MAX
#define G_MINLONG	LONG_MIN
#define G_MAXLONG	LONG_MAX
#define G_MAXULONG	ULONG_MAX

#define TRUE true
#define FALSE false

typedef signed char gint8;
typedef unsigned char guint8;
typedef signed short gint16;
typedef unsigned short guint16;
typedef char gchar;
typedef int32_t gint;
typedef uint32_t guint;
typedef bool gboolean;
#define G_GINT16_MODIFIER "h"
#define G_GINT16_FORMAT "hi"
#define G_GUINT16_FORMAT "hu"
typedef int32_t gint32;
typedef uint32_t guint32;
#define G_GINT32_MODIFIER ""
#define G_GINT32_FORMAT "i"
#define G_GUINT32_FORMAT "u"
#define G_HAVE_GINT64 1          /* deprecated, always true */
typedef int64_t gint64;
typedef uint64_t guint64;
#define G_GINT64_CONSTANT(val)	(val##L)
#define G_GUINT64_CONSTANT(val)	(val##UL)
#define G_GINT64_MODIFIER "l"
#define G_GINT64_FORMAT "li"
#define G_GUINT64_FORMAT "lu"
#define GLIB_SIZEOF_VOID_P 8
#define GLIB_SIZEOF_LONG   8
#define GLIB_SIZEOF_SIZE_T 8
typedef signed long gssize;
typedef unsigned long gsize;
#define G_GSIZE_MODIFIER "l"
#define G_GSSIZE_FORMAT "li"
#define G_GSIZE_FORMAT "lu"
#define G_MAXSIZE	G_MAXULONG
#define G_MINSSIZE	G_MINLONG
#define G_MAXSSIZE	G_MAXLONG
typedef gint64 goffset;
#define G_MINOFFSET	G_MININT64
#define G_MAXOFFSET	G_MAXINT64
#define G_GOFFSET_MODIFIER      G_GINT64_MODIFIER
#define G_GOFFSET_FORMAT        G_GINT64_FORMAT
#define G_GOFFSET_CONSTANT(val) G_GINT64_CONSTANT(val)
#define GPOINTER_TO_INT(p)	((gint)  (glong) (p))
#define GPOINTER_TO_UINT(p)	((guint) (gulong) (p))
#define GINT_TO_POINTER(i)	((gpointer) (glong) (i))
#define GUINT_TO_POINTER(u)	((gpointer) (gulong) (u))
typedef signed long gintptr;
typedef unsigned long guintptr;
#ifdef NeXT /* @#%@! NeXTStep */
# define g_ATEXIT(proc)	(!atexit (proc))
#else
# define g_ATEXIT(proc)	(atexit (proc))
#endif
#define g_memmove(dest,src,len) G_STMT_START { memmove ((dest), (src), (len)); } G_STMT_END
#define GLIB_MAJOR_VERSION 2
#define GLIB_MINOR_VERSION 20
#define GLIB_MICRO_VERSION 0
#define G_OS_UNIX
#define G_VA_COPY	va_copy
#define G_VA_COPY_AS_ARRAY 1
#ifdef	__cplusplus
#define	G_HAVE_INLINE	1
#else	/* !__cplusplus */
#define G_HAVE_INLINE 1
#define G_HAVE___INLINE 1
#define G_HAVE___INLINE__ 1
#endif	/* !__cplusplus */
#ifdef	__cplusplus
#define G_CAN_INLINE	1
#else	/* !__cplusplus */
#define G_CAN_INLINE	1
#endif
#ifndef __cplusplus
# define G_HAVE_ISO_VARARGS 1
#endif
#ifdef __cplusplus
# define G_HAVE_ISO_VARARGS 1
#endif
/* gcc-2.95.x supports both gnu style and ISO varargs, but if -ansi
 * is passed ISO vararg support is turned off, and there is no work
 * around to turn it on, so we unconditionally turn it off.
 */

// save game
typedef struct {
  void *address;
  int size;
} variable_desc;

typedef void* gpointer;
typedef const void *gconstpointer;

#endif