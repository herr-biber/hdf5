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

/*
 * Programmer:  rky 980813
 *
 * Purpose:	Functions to read/write directly between app buffer and file.
 *
 * 		Beware of the ifdef'ed print statements.
 *		I didn't make them portable.
 */

#define H5D_PACKAGE		/*suppress error about including H5Dpkg	  */


/* Pablo information */
/* (Put before include files to avoid problems with inline functions) */
#define PABLO_MASK	H5D_mpio_mask

#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Sprivate.h"		/* Dataspaces 				*/

#ifdef H5_HAVE_PARALLEL

/* Interface initialization */
#define INTERFACE_INIT  NULL
static int             interface_initialize_g = 0;

static herr_t
H5D_mpio_spaces_xfer(H5F_t *f, const H5D_t *dset, size_t elmt_size,
                     const H5S_t *file_space, const H5S_t *mem_space,
                     hid_t dxpl_id, void *buf/*out*/, 
		     const H5D_storage_t *store,
		     hbool_t do_write);


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_spaces_xfer
 *
 * Purpose:	Use MPI-IO to transfer data efficiently
 *		directly between app buffer and file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	rky 980813
 *
 * Notes:
 *      For collective data transfer only since this would eventually call
 *      H5FD_mpio_setup to do setup to eveually call MPI_File_set_view in
 *      H5FD_mpio_read or H5FD_mpio_write.  MPI_File_set_view is a collective
 *      call.  Letting independent data transfer use this route would result in
 *      hanging.
 *
 *      The preconditions for calling this routine are located in the
 *      H5S_mpio_opt_possible() routine, which determines whether this routine
 *      can be called for a given dataset transfer.
 *
 * Modifications:
 *	rky 980918
 *	Added must_convert parameter to let caller know we can't optimize
 *	the xfer.
 *
 *	Albert Cheng, 001123
 *	Include the MPI_type freeing as part of cleanup code.
 *
 *      QAK - 2002/04/02
 *      Removed the must_convert parameter and move preconditions to
 *      H5S_mpio_opt_possible() routine
 *
 *      QAK - 2002/06/17
 *      Removed 'disp' parameter from H5FD_mpio_setup routine and use the
 *      address of the dataset in MPI_File_set_view() calls, as necessary.
 *
 *      QAK - 2002/06/18
 *      Removed 'dc_plist' parameter, since it was not used.  Also, switch to
 *      getting the 'extra_offset' setting for each selection.
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_mpio_spaces_xfer(H5F_t *f, const H5D_t *dset, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    hid_t dxpl_id, void *_buf /*out*/,
    const H5D_storage_t *store,
    hbool_t do_write )
{
    haddr_t	 addr;                  /* Address of dataset (or selection) within file */
    size_t	 mpi_buf_count, mpi_file_count;       /* Number of "objects" to transfer */
    hsize_t	 mpi_buf_offset, mpi_file_offset;       /* Offset within dataset where selection (ie. MPI type) begins */
    MPI_Datatype mpi_buf_type, mpi_file_type;   /* MPI types for buffer (memory) and file */
    hbool_t	 mbt_is_derived=0,      /* Whether the buffer (memory) type is derived and needs to be free'd */
		 mft_is_derived=0;      /* Whether the file type is derived and needs to be free'd */
    hbool_t	 plist_is_setup=0;      /* Whether the dxpl has been customized */
    uint8_t	*buf=(uint8_t *)_buf;   /* Alias for pointer arithmetic */
    int          mpi_code;              /* MPI return code */
    herr_t	 ret_value = SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_mpio_spaces_xfer);

    /* Check args */
    assert (f);
    assert (dset);
    assert (file_space);
    assert (mem_space);
    assert (buf);
    assert (IS_H5FD_MPIO(f));
    /* Make certain we have the correct type of property list */
    assert(TRUE==H5P_isa_class(dxpl_id,H5P_DATASET_XFER));

    /* create the MPI buffer type */
    if (H5S_mpio_space_type( mem_space, elmt_size,
			       /* out: */
			       &mpi_buf_type,
			       &mpi_buf_count,
			       &mpi_buf_offset,
			       &mbt_is_derived )<0)
    	HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI buf type");

    /* create the MPI file type */
    if ( H5S_mpio_space_type( file_space, elmt_size,
			       /* out: */
			       &mpi_file_type,
			       &mpi_file_count,
			       &mpi_file_offset,
			       &mft_is_derived )<0)
    	HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI file type");

    /* Get the base address of the contiguous dataset or the chunk */
    if(dset->layout.type == H5D_CONTIGUOUS)
       addr = H5D_contig_get_addr(dset) + mpi_file_offset;
    else {
        haddr_t   chunk_addr; /* for collective chunk IO */

        assert(dset->layout.type == H5D_CHUNKED); 
        chunk_addr=H5D_istore_get_addr(f,dxpl_id,&(dset->layout),store->chunk.offset,NULL);
        addr = H5F_BASE_ADDR(f) + chunk_addr + mpi_file_offset;
    }

    /*
     * Pass buf type, file type to the file driver. Request an MPI type
     * transfer (instead of an elementary byteblock transfer).
     */
    if(H5FD_mpi_setup_collective(dxpl_id, mpi_buf_type, mpi_file_type)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI-I/O properties");
    plist_is_setup=1;

    /* Adjust the buffer pointer to the beginning of the selection */
    buf+=mpi_buf_offset;

    /* transfer the data */
    if (do_write) {
    	if (H5F_block_write(f, H5FD_MEM_DRAW, addr, mpi_buf_count, dxpl_id, buf) <0)
	    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,"MPI write failed");
    } else {
    	if (H5F_block_read (f, H5FD_MEM_DRAW, addr, mpi_buf_count, dxpl_id, buf) <0)
	    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL,"MPI read failed");
    }

done:
    /* Reset the dxpl settings */
    if(plist_is_setup) {
        if(H5FD_mpi_teardown_collective(dxpl_id)<0)
    	    HDONE_ERROR(H5E_DATASPACE, H5E_CANTFREE, FAIL, "unable to reset dxpl values");
    } /* end if */

    /* free the MPI buf and file types */
    if (mbt_is_derived) {
	if (MPI_SUCCESS != (mpi_code= MPI_Type_free( &mpi_buf_type )))
            HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code);
    }
    if (mft_is_derived) {
	if (MPI_SUCCESS != (mpi_code= MPI_Type_free( &mpi_file_type )))
            HMPI_DONE_ERROR(FAIL, "MPI_Type_free failed", mpi_code);
    }

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5D_mpio_spaces_xfer() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_spaces_read
 *
 * Purpose:	MPI-IO function to read directly from app buffer to file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	rky 980813
 *
 * Modifications:
 *
 * rky 980918
 * Added must_convert parameter to let caller know we can't optimize the xfer.
 *
 *      QAK - 2002/04/02
 *      Removed the must_convert parameter and move preconditions to
 *      H5S_mpio_opt_possible() routine
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_mpio_spaces_read(H5F_t *f, const H5D_dxpl_cache_t UNUSED *dxpl_cache, hid_t dxpl_id,
    H5D_t *dset, const H5D_storage_t  *store,
    size_t UNUSED nelmts, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    void *buf/*out*/)
{
    herr_t ret_value;

    FUNC_ENTER_NOAPI(H5D_mpio_spaces_read, FAIL);

    ret_value = H5D_mpio_spaces_xfer(f, dset, elmt_size, file_space,
        mem_space, dxpl_id, buf, store, 0/*read*/);

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5D_mpio_spaces_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_spaces_write
 *
 * Purpose:	MPI-IO function to write directly from app buffer to file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	rky 980813
 *
 * Modifications:
 *
 * rky 980918
 * Added must_convert parameter to let caller know we can't optimize the xfer.
 *
 *      QAK - 2002/04/02
 *      Removed the must_convert parameter and move preconditions to
 *      H5S_mpio_opt_possible() routine
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5D_mpio_spaces_write(H5F_t *f, const H5D_dxpl_cache_t UNUSED *dxpl_cache, hid_t dxpl_id,
    H5D_t *dset, const H5D_storage_t  *store,
    size_t UNUSED nelmts, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    const void *buf)
{
    herr_t ret_value;

    FUNC_ENTER_NOAPI(H5D_mpio_spaces_write, FAIL);

    /*OKAY: CAST DISCARDS CONST QUALIFIER*/
    ret_value = H5D_mpio_spaces_xfer(f, dset, elmt_size, file_space,
        mem_space, dxpl_id, (void*)buf, store, 1/*write*/);

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5D_mpio_spaces_write() */
#endif  /* H5_HAVE_PARALLEL */
