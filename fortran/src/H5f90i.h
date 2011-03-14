#ifndef _H5f90i_H
#define _H5f90i_H

#ifdef GOT_MACHINE
#undef GOT_MACHINE
#endif

#define  DFMT_IRIX         0x1111

/* 
 * Standard header files needed all the time 
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#if (defined (UNICOS) || (defined (_UNICOS)))

#ifndef UNICOS 
#define UNICOS 
#endif

#include <memory.h>
#include <fortran.h>
#ifndef O_RDONLY
#include <fcntl.h>              /* for unbuffered i/o stuff */
#define L_INCR  1
#include <sys/stat.h>
#endif /*O_RDONLY*/

#ifdef _CRAYIEEE
#define DF_MT   DFMT_UNICOSIEEE
#else
#define DF_MT   DFMT_UNICOS
#endif
/*typedef char*              _fcd;*/
typedef long               hsize_t_f;
typedef long               hssize_t_f;
typedef long               size_t_f;
typedef long               int_f;
typedef long               hid_t_f;
typedef double             real_f;
#define DF_CAPFNAMES 
/*#define _fcdtocp(desc) (desc)*/

#endif /* UNICOS */


/* LINUX definitions */
#if defined(i386) && defined(linux)
#define DF_MT             DFMT_LINIX
typedef char              *_fcd;
typedef long long         hsize_t_f;
typedef long long         hssize_t_f;
typedef int               size_t_f;
typedef int               int_f;
typedef int               hid_t_f;
typedef float             real_f;
#define FNAME_POST_UNDERSCORE
#define _fcdtocp(desc) (desc)

#endif /*LINUX*/

#if defined(IRIX) || defined(IRIS4) || defined(sgi) || defined(__sgi__) || defined(__sgi)

#ifndef IRIX
#define IRIX
#endif

#if (_MIPS_SZLONG == 64)
/* IRIX 64 bits objects.  It is nearly the same as the conventional
 * 32 bits objects.  Let them share IRIX definitions for now.
 */
#define IRIX64
#endif


#ifdef GOT_MACHINE
If you get an error on this line more than one machine type has been defined.
Please check your Makefile.
#endif
#define GOT_MACHINE 1

#   define BSD
#ifndef __GNUC__
#include <memory.h>
#endif /* __GNUC__ */
#include <sys/file.h>               /* for unbuffered i/o stuff */
#include <sys/stat.h>
#define DF_MT         DFMT_IRIX
typedef char          *_fcd;

typedef long          hsize_t_f;
typedef long          hssize_t_f;
typedef long          size_t_f;
typedef int           int_f;
typedef int           hid_t_f;
typedef float         real_f;
#define FNAME_POST_UNDERSCORE
#define _fcdtocp(desc) (desc)
#ifdef IRIX64
#define BIG_LONGS
#endif


#define HAVE_STDC
#define INCLUDES_ARE_ANSI

#endif /* IRIX */

#if (defined(SUN) || defined(sun) || defined(__sun__) || defined(__SUNPRO_C)) & !defined(__i386)
#ifdef __STDC__
#define ANSISUN
#else /* __STDC__ */
#define KNRSUN
#endif /* __STDC__ */
#endif /* SUN || sun */

#if defined(ANSISUN)

#if !defined(SUN)
#define SUN
#endif

#ifdef GOT_MACHINE
If you get an error on this line more than one machine type has been defined.
Please check your Makefile.
#endif
#define GOT_MACHINE

#include <unistd.h>                 /* for some file I/O stuff */
#include <sys/time.h>
#include <sys/file.h>               /* for unbuffered i/o stuff */
#include <sys/stat.h>
#define DF_MT             DFMT_SUN
typedef char              *_fcd;
typedef int               hsize_t_f;
typedef int               hssize_t_f;
typedef int               size_t_f;
typedef int               int_f;
typedef int               hid_t_f;
typedef float             real_f;
#define FNAME_POST_UNDERSCORE
#define _fcdtocp(desc) (desc)

#endif     /*SUN*/


#if defined DEC_ALPHA || (defined __alpha && defined __unix__)

#ifndef DEC_ALPHA
#define DEC_ALPHA
#endif

#ifdef GOT_MACHINE
If you get an error on this line more than one machine type has been defined.
Please check your Makefile.
#endif
#define GOT_MACHINE

#include <sys/file.h>               /* for unbuffered i/o stuff */
#include <sys/stat.h>
#define DF_MT            DFMT_ALPHA
typedef char             *_fcd;
typedef long             hsize_t_f;
typedef long             hssize_t_f;
typedef long             size_t_f;
typedef int              int_f;
typedef int              hid_t_f;
typedef float            real_f;
#define FNAME_POST_UNDERSCORE
#define _fcdtocp(desc) (desc)

#endif /* DEC_ALPHA */


#if defined(HP9000) || (!defined(__convexc__) && (defined(hpux) || defined(__hpux)))

#ifndef HP9000
#define HP9000
#endif

#ifdef GOT_MACHINE
If you get an error on this line more than one machine type has been defined.
Please check your Makefile.
#endif
#define GOT_MACHINE

#ifndef HAVE_UNISTD_H
#define HAVE_UNISTD_H  /* unistd.h - close, fork,..etc */
#endif

#   define BSD
#ifndef __GNUC__
#include <memory.h>
#endif /* __GNUC__ */
#include <sys/file.h>               /* for unbuffered i/o stuff */
#include <sys/stat.h>
#define DF_MT          DFMT_HP9000
typedef char           *_fcd;
typedef long long           hsize_t_f;
typedef long long          hssize_t_f;
typedef long           size_t_f;
typedef int            int_f;
typedef int            hid_t_f;
typedef float          real_f;
#define _fcdtocp(desc) (desc)
#ifdef  HAVE_FMPOOL
#define FILELIB PAGEBUFIO  /* enable page buffering */
#else
#define FILELIB UNIXBUFIO
#endif

#endif /* HP9000 */


#if defined _WINDOWS || defined WIN32
#define GOT_MACHINE 1

#pragma comment( lib, "oldnames" )
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#include <conio.h> 
#include <malloc.h>
#include <ctype.h>          /* for character macros */
#ifdef __WATCOMC__
#include <stddef.h>         /* for the 'fortran' pragma */
#endif

#define DF_MT             DFMT_PC

typedef char              *_fcd;
typedef int               hsize_t_f;
typedef int               hssize_t_f;
typedef int               size_t_f;
typedef int               int_f;
typedef int               hid_t_f;
typedef float             real_f;

#if defined _M_ALPHA
#define FNAME_PRE_UNDERSCORE
#endif

#define DF_CAPFNAMES
#define _fcdtocp(desc) (desc)

#ifdef  HAVE_FMPOOL
#define FILELIB PAGEBUFIO  /* enable page buffering */
#else
#define FILELIB UNIXBUFIO
#endif

#endif /*WINDOWS */

/*----------------------------------------------------------------
** MACRO FNAME for any fortran callable routine name.
**
**  This macro prepends, appends, or does not modify a name
**  passed as a macro parameter to it based on the FNAME_PRE_UNDERSCORE,
**  FNAME_POST_UNDERSCORE macros set for a specific system.
**
**---------------------------------------------------------------*/
#if defined(FNAME_PRE_UNDERSCORE) && defined(FNAME_POST_UNDERSCORE)
#   define FNAME(x)     _##x##_
#endif
#if defined(FNAME_PRE_UNDERSCORE) && !defined(FNAME_POST_UNDERSCORE)
#   define FNAME(x)     _##x
#endif
#if !defined(FNAME_PRE_UNDERSCORE) && defined(FNAME_POST_UNDERSCORE)
#   define FNAME(x)     x##_
#endif
#if !defined(FNAME_PRE_UNDERSCORE) && !defined(FNAME_POST_UNDERSCORE)
#   define FNAME(x)     x
#endif

#  define HDfree(p)        (free((void*)p))
#  define HDmalloc(s)      (malloc((size_t)s))
#  define HDstrlen(s)       (strlen((const char *)(s))) 
#  define HDmemcpy(dst,src,n)   (memcpy((void *)(dst),(const void *)(src),(size_t)(n)))


#endif /* _H5f90i_H */
