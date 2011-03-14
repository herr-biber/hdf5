/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

/* $Id: testhdf5.h,v 1.27.2.3 2002/01/23 22:30:25 koziol Exp $ */

/*
 * This header file contains information required for testing the HDF5 library.
 */

#ifndef HDF5TEST_H
#define HDF5TEST_H

/*
 * Include required headers.  This file tests internal library functions,
 * so we include the private headers here.
 */
#include "H5private.h"
#include "H5Eprivate.h"

#ifndef HDF5_TEST_MASTER
extern int              num_errs;
extern int              Verbosity;
#endif /* HDF5_TEST_MASTER */

/* Use %ld to print the value because long should cover most cases. */
/* Used to make certain a return value _is_not_ a value */
#define CHECK(ret, val, where) do {					      \
    if (Verbosity>9) print_func("   Call to routine: %15s at line %4d "	      \
				"in %s returned %ld \n",		      \
				where, (int)__LINE__, __FILE__,		      \
				(long)ret);				      \
    if (ret == val) {							      \
	print_func("*** UNEXPECTED RETURN from %s is %ld at line %4d "	      \
		   "in %s\n", where, (long)ret, (int)__LINE__, __FILE__);     \
	num_errs++;							      \
	H5Eprint (stdout);						      \
    }									      \
    H5Eclear();								      \
} while(0)

#define CHECK_I(ret,where) {						      \
   if (Verbosity>9) {							      \
      print_func("   Call to routine: %15s at line %4d in %s returned %ld\n", \
                 (where), (int)__LINE__, __FILE__, (long)(ret));	      \
   }									      \
   if ((ret)<0) {							      \
      print_func ("*** UNEXPECTED RETURN from %s is %ld line %4d in %s\n",    \
                  (where), (long)(ret), (int)__LINE__, __FILE__);	      \
      H5Eprint (stdout);						      \
      num_errs++;							      \
   }									      \
   H5Eclear ();								      \
}

#define CHECK_PTR(ret,where) {						      \
   if (Verbosity>9) {							      \
      print_func("   Call to routine: %15s at line %4d in %s returned %p\n",  \
                 (where), (int)__LINE__, __FILE__, (ret));		      \
   }									      \
   if (!(ret)) {							      \
      print_func ("*** UNEXPECTED RETURN from %s is NULL line %4d in %s\n",   \
                  (where), (int)__LINE__, __FILE__);			      \
      H5Eprint (stdout);						      \
      num_errs++;							      \
   }									      \
   H5Eclear ();								      \
}

/* Used to make certain a return value _is_ a value */
#define VERIFY(x, val, where) do {					      \
    if (Verbosity>9) {							      \
	print_func("   Call to routine: %15s at line %4d in %s had value "    \
		   "%ld \n", where, (int)__LINE__, __FILE__, (long)x);	      \
    }									      \
    if (x != val) {							      \
	print_func("*** UNEXPECTED VALUE from %s is %ld at line %4d "	      \
		   "in %s\n", where, (long)x, (int)__LINE__, __FILE__);	      \
	H5Eprint (stdout);						      \
	num_errs++;							      \
    }									      \
    H5Eclear();								      \
} while(0)

/* Used to document process through a test and to check for errors */
#define RESULT(ret,func) do {						      \
    if (Verbosity>8) {							      \
	print_func("   Call to routine: %15s at line %4d in %s returned "     \
		   "%ld\n", func, (int)__LINE__, __FILE__, (long)ret);	      \
    }									      \
    if (Verbosity>9) HEprint(stdout, 0);				      \
    if (ret == FAIL) {							      \
	print_func("*** UNEXPECTED RETURN from %s is %ld at line %4d "	      \
		   "in %s\n", func, (long)ret, (int)__LINE__, __FILE__);      \
	H5Eprint (stdout);						      \
	num_errs++;							      \
    }									      \
    H5Eclear();								      \
} while(0)

/* Used to document process through a test */
#define MESSAGE(V,A) {if (Verbosity>(V)) print_func A;}

/* definitions for command strings */
#define VERBOSITY_STR   "Verbosity"
#define SKIP_STR        "Skip"
#define TEST_STR        "Test"
#define CLEAN_STR       "Cleanup"

/* Prototypes for the support routines */
int                     print_func(const char *,...);

/* Prototypes for the test routines */
void                    test_metadata(void);
void                    test_tbbt(void);
void                    test_file(void);
void                    test_h5t(void);
void                    test_h5s(void);
void                    test_h5d(void);
void                    test_attr(void);
void                    test_select(void);
void                    test_time(void);
void                    test_reference(void);
void                    test_vltypes(void);
void                    test_vlstrings(void);
void                    test_iterate(void);
void                    test_array(void);
void                    test_genprop(void);
void			test_configure(void);
void                    test_misc(void);

/* Prototypes for the cleanup routines */
void                    cleanup_metadata(void);
void                    cleanup_file(void);
void                    cleanup_h5s(void);
void                    cleanup_attr(void);
void                    cleanup_select(void);
void                    cleanup_time(void);
void                    cleanup_reference(void);
void                    cleanup_vltypes(void);
void                    cleanup_vlstrings(void);
void                    cleanup_iterate(void);
void                    cleanup_array(void);
void                    cleanup_genprop(void);
void			cleanup_configure(void);
void                    cleanup_misc(void);

#endif /* HDF5cleanup_H */
