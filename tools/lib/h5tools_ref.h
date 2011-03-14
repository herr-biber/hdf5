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

#ifndef H5TOOLS_REF_H__
#define H5TOOLS_REF_H__

#include "hdf5.h"

typedef struct ref_path_table_entry_t {
    hid_t                 obj;
    hobj_ref_t             *obj_ref;
    char                   *apath;
    H5G_stat_t  statbuf;
    struct ref_path_table_entry_t *next;
}ref_path_table_entry_t;

#ifdef __cplusplus
extern "C" {
#endif

char*       lookup_ref_path(hobj_ref_t * ref);
herr_t      fill_ref_path_table(hid_t, const char *, void *);
int         get_next_xid(void);
haddr_t     get_fake_xid (void);
struct      ref_path_table_entry_t *ref_path_table_lookup(const char *);
hobj_ref_t  *ref_path_table_put(hid_t, const char *);
struct      ref_path_table_entry_t *ref_path_table_gen_fake(const char *);

#ifdef __cplusplus
}
#endif



#endif


