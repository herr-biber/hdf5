/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* $Id: ttime.c,v 1.1.2.3 2002/06/10 19:48:51 wendling Exp $ */

/***********************************************************
*
* Test program:	 ttime
*
* Test the Time Datatype functionality
*
*************************************************************/

#include "testhdf5.h"

#include "hdf5.h"

#define FILE   "ttime.h5"

/****************************************************************
**
**  test_time(): Main time datatype testing routine.
** 
****************************************************************/
void 
test_time(void)
{
    hid_t       file_id, tid;  /* identifiers */
    herr_t      status;

    /* Output message about test being performed */
    MESSAGE(5, ("Testing Time Datatypes\n"));

    /* Create a new file using default properties. */
    file_id = H5Fcreate(FILE, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    CHECK(file_id, FAIL, "H5Fcreate");
 
    tid = H5Tcopy (H5T_UNIX_D32LE);
    CHECK(tid, FAIL, "H5Tcopy");
    status = H5Tcommit(file_id, "Committed D32LE type", tid);
    CHECK(status, FAIL, "H5Tcommit");
    status = H5Tclose (tid);
    CHECK(status, FAIL, "H5Tclose");

    tid = H5Tcopy (H5T_UNIX_D32BE);
    CHECK(tid, FAIL, "H5Tcopy");
    status = H5Tcommit(file_id, "Committed D32BE type", tid);
    CHECK(status, FAIL, "H5Tcommit");
    status = H5Tclose (tid);
    CHECK(status, FAIL, "H5Tclose");

    tid = H5Tcopy (H5T_UNIX_D64LE);
    CHECK(tid, FAIL, "H5Tcopy");
    status = H5Tcommit(file_id, "Committed D64LE type", tid);
    CHECK(status, FAIL, "H5Tcommit");
    status = H5Tclose (tid);
    CHECK(status, FAIL, "H5Tclose");

    tid = H5Tcopy (H5T_UNIX_D64BE);
    CHECK(tid, FAIL, "H5Tcopy");
    status = H5Tcommit(file_id, "Committed D64BE type", tid);
    CHECK(status, FAIL, "H5Tcommit");
    status = H5Tclose (tid);
    CHECK(status, FAIL, "H5Tclose");
 
    /* Close the file. */
    status = H5Fclose(file_id);
    CHECK(status, FAIL, "H5Fclose");
    
    file_id = H5Fopen(FILE, H5F_ACC_RDWR, H5P_DEFAULT);
    CHECK(file_id, FAIL, "H5Fopen");
   
    tid = H5Topen(file_id, "Committed D32LE type");
    CHECK(tid, FAIL, "H5Topen");
 
    if(!H5Tequal(tid, H5T_UNIX_D32LE)) {
        num_errs++;
        MESSAGE(0, ("H5T_UNIX_D32LE datatype not found"));
    } /* end if */
 
    status = H5Tclose (tid);
    CHECK(status, FAIL, "H5Tclose");

    tid = H5Topen(file_id, "Committed D32BE type");
    CHECK(tid, FAIL, "H5Topen");
 
    if(!H5Tequal(tid, H5T_UNIX_D32BE)) {
        num_errs++;
        MESSAGE(0, ("H5T_UNIX_D32BE datatype not found"));
    } /* end if */
 
    status = H5Tclose (tid);
    CHECK(status, FAIL, "H5Tclose");

    tid = H5Topen(file_id, "Committed D64LE type");
    CHECK(tid, FAIL, "H5Topen");
 
    if(!H5Tequal(tid, H5T_UNIX_D64LE)) {
        num_errs++;
        MESSAGE(0, ("H5T_UNIX_D64LE datatype not found"));
    } /* end if */
 
    status = H5Tclose (tid);
    CHECK(status, FAIL, "H5Tclose");

    tid = H5Topen(file_id, "Committed D64BE type");
    CHECK(tid, FAIL, "H5Topen");
 
    if(!H5Tequal(tid, H5T_UNIX_D64BE)) {
        num_errs++;
        MESSAGE(0, ("H5T_UNIX_D64BE datatype not found"));
    } /* end if */
 
    status = H5Tclose (tid);
    CHECK(status, FAIL, "H5Tclose");

    status = H5Fclose(file_id);
    CHECK(status, FAIL, "H5Fclose");
 
}   /* test_reference() */


/*-------------------------------------------------------------------------
 * Function:	cleanup_time
 *
 * Purpose:	Cleanup temporary test files
 *
 * Return:	none
 *
 * Programmer:	Quincey Koziol
 *              October 19, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
cleanup_time(void)
{
    remove(FILE);
}

