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



#include "H5private.h"		/* Generic Functions			*/
#include "H5Dpkg.h"		/* Datasets				*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5Fprivate.h"		/* File access				*/
#include "H5FDprivate.h"	/* File drivers				*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Pprivate.h"         /* Property lists                       */
#include "H5Sprivate.h"		/* Dataspaces 				*/

#ifdef H5_HAVE_PARALLEL

static herr_t
H5D_mpio_spaces_xfer(H5D_io_info_t *io_info, size_t elmt_size,
                     const H5S_t *file_space, const H5S_t *mem_space,
                     void *buf/*out*/, 
		     hbool_t do_write);


static herr_t
H5D_mpio_spaces_span_xfer(H5D_io_info_t *io_info, size_t elmt_size,
                     const H5S_t *file_space, const H5S_t *mem_space,
                     void *buf/*out*/, 
		     hbool_t do_write);

/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_opt_possible
 *
 * Purpose:	Checks if an direct I/O transfer is possible between memory and
 *                  the file.
 *
 * Return:	Success:        Non-negative: TRUE or FALSE
 *		Failure:	Negative
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, April 3, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5D_mpio_opt_possible( const H5D_t *dset, const H5S_t *mem_space, const H5S_t *file_space, const unsigned flags)
{
    htri_t c1,c2;               /* Flags whether a selection is optimizable */
    htri_t ret_value=TRUE;

    FUNC_ENTER_NOAPI(H5D_mpio_opt_possible, FAIL);

    /* Check args */
    assert(dset);
    assert(mem_space);
    assert(file_space);

    /* Parallel I/O conversion flag must be set, if it is not collective IO, go to false. */
    if(!(flags&H5S_CONV_PAR_IO_POSSIBLE))
        HGOTO_DONE(FALSE);

    /* Check whether these are both simple or scalar dataspaces */
    if (!((H5S_SIMPLE==H5S_GET_EXTENT_TYPE(mem_space) || H5S_SCALAR==H5S_GET_EXTENT_TYPE(mem_space))
            && (H5S_SIMPLE==H5S_GET_EXTENT_TYPE(file_space) || H5S_SCALAR==H5S_GET_EXTENT_TYPE(file_space))))
        HGOTO_DONE(FALSE);

    /* Check whether both selections are "regular" */
#ifndef KYANG
    c1=H5S_SELECT_IS_REGULAR(file_space);
    c2=H5S_SELECT_IS_REGULAR(mem_space);
    if(c1==FAIL || c2==FAIL)
        HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "invalid check for single selection blocks");
    if(c1==FALSE || c2==FALSE)
        HGOTO_DONE(FALSE);
#endif
    /* Can't currently handle point selections */
    if (H5S_SEL_POINTS==H5S_GET_SELECT_TYPE(mem_space) || H5S_SEL_POINTS==H5S_GET_SELECT_TYPE(file_space))
        HGOTO_DONE(FALSE);

    /* Dataset storage must be contiguous or chunked */
    if ((flags&H5S_CONV_STORAGE_MASK)!=H5S_CONV_STORAGE_CONTIGUOUS && 
            (flags&H5S_CONV_STORAGE_MASK)!=H5S_CONV_STORAGE_CHUNKED)
        HGOTO_DONE(FALSE);

    if ((flags&H5S_CONV_STORAGE_MASK)==H5S_CONV_STORAGE_CHUNKED) {
        hsize_t chunk_dim[H5O_LAYOUT_NDIMS];        /* Chunk dimensions */
        hsize_t startf[H5S_MAX_RANK],      /* Selection start bounds */
            endf[H5S_MAX_RANK];     /* Selection end bounds */
        unsigned dim_rankf;         /* Number of dimensions of file dataspace */
        int pcheck_hyper,check_hyper,   /* Flags for checking if selection is in one chunk */
            tnum_chunkf,            /* Number of chunks selection overlaps */
            max_chunkf,             /* Maximum number of chunks selection overlaps */
            min_chunkf,             /* Minimum number of chunks selection overlaps */
            num_chunks_same;        /* Flag indicating whether all processes have the same # of chunks to operate on */
        unsigned dim_chunks;        /* Temporary number of chunks in a dimension */
        MPI_Comm comm;              /* MPI communicator for file */
        int mpi_rank;               /* Rank in MPI communicator */
        int mpi_code;               /* MPI return code */
        unsigned u;                 /* Local index variable */

        /* Disallow collective I/O if there are any I/O filters on chunks */
        if(dset->shared->dcpl_cache.pline.nused>0)
            HGOTO_DONE(FALSE)

        /* Getting MPI communicator and rank */
        if((comm = H5F_mpi_get_comm(dset->ent.file))==MPI_COMM_NULL)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't retrieve MPI communicator")
        if((mpi_rank = H5F_mpi_get_rank(dset->ent.file))<0)
            HGOTO_ERROR(H5E_DATASPACE, H5E_CANTGET, FAIL, "can't retrieve MPI rank")

      /* Currently collective chunking storage 
	 inside HDF5 is supported for either one of the following two cases:
	 1. All the hyperslabs for one process is inside one chunk.
	 2. For single hyperslab selection, the number of chunks that covered 
	    the single selection for all processes should be equal. 
	    KY, 2004/7/14
      */

      /* Quincey, please read.
	 This is maybe redundant, I think only when both memory and file space be SCALAR
	 space, the collective IO can work. Otherwise, SELECT_POINT will be reached,collective
	 IO shouldn't work.
	 Please clarify and correct the code on the following,
         Quincey said that it was probably okay if only one data space is SCALAR, 
         Still keep the code here until we added more tests later.
	 Kent */
        if(H5S_SCALAR==H5S_GET_EXTENT_TYPE(mem_space) || H5S_SCALAR ==H5S_GET_EXTENT_TYPE(file_space)) {
            if(!(H5S_SCALAR==H5S_GET_EXTENT_TYPE(mem_space) && H5S_SCALAR ==H5S_GET_EXTENT_TYPE(file_space)))
                HGOTO_DONE(FALSE)
            else
                HGOTO_DONE(TRUE)
        } /* end if */

        dim_rankf = H5S_GET_EXTENT_NDIMS(file_space);

        if(H5S_SELECT_BOUNDS(file_space,startf,endf)==FAIL)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE,FAIL, "invalid check for single selection blocks");

        for(u=0; u < dset->shared->layout.u.chunk.ndims; u++) 
            chunk_dim[u] = dset->shared->layout.u.chunk.dim[u];

        /* Case 1: check whether all hyperslab in this process is inside one chunk.
           Note: we don't handle when starting point is less than zero since that may cover
           two chunks. */

        /*for file space checking*/
        pcheck_hyper = 1;
        for (u=0; u<dim_rankf; u++)
            if(endf[u]/chunk_dim[u]!=startf[u]/chunk_dim[u]) {
                pcheck_hyper = 0;
                break;
            }
      
        if (MPI_SUCCESS != (mpi_code= MPI_Reduce(&pcheck_hyper,&check_hyper,1,MPI_INT,MPI_LAND,0,comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Reduce failed", mpi_code)
        if (MPI_SUCCESS != (mpi_code= MPI_Bcast(&check_hyper,1,MPI_INT,0,comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code)

        /*if check_hyper is true, condition for collective IO case is fulfilled, no
         need to do further test. */
        if(check_hyper)
            HGOTO_DONE(TRUE); 
    
      /* Case 2:Check whether the number of chunks that covered the single hyperslab is the same.
	 If not,no collective chunk IO. 
	 KY, 2004/7/14
      */
	 
        c1 = H5S_SELECT_IS_SINGLE(file_space);
        c2 = H5S_SELECT_IS_SINGLE(mem_space);

        if(c1==FAIL || c2 ==FAIL)
            HGOTO_ERROR(H5E_DATASPACE, H5E_BADRANGE, FAIL, "invalid check for single selection blocks");
        if(c1==FALSE || c2 ==FALSE)
            HGOTO_DONE(FALSE);

        /* Compute the number of chunks covered by the selection on this process */
        tnum_chunkf = 1;
        for (u=0; u<dim_rankf; u++) {
            dim_chunks = (endf[u]/chunk_dim[u]-startf[u]/chunk_dim[u])+1;
            tnum_chunkf = dim_chunks*tnum_chunkf;
        }

        /* Determine the minimum and maximum # of chunks for all processes */
        if (MPI_SUCCESS != (mpi_code= MPI_Reduce(&tnum_chunkf,&max_chunkf,1,MPI_INT,MPI_MAX,0,comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Reduce failed", mpi_code)
        if (MPI_SUCCESS != (mpi_code= MPI_Reduce(&tnum_chunkf,&min_chunkf,1,MPI_INT,MPI_MIN,0,comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Reduce failed", mpi_code)
  
        /* Let the rank==0 process determine if the same number of chunks will be operated on by all processes */
        if(mpi_rank == 0)
            num_chunks_same = (max_chunkf==min_chunkf);
                    
        /* Broadcast the flag indicating the number of chunks are the same */
        if (MPI_SUCCESS != (mpi_code= MPI_Bcast(&num_chunks_same,1,MPI_INT,0,comm)))
            HMPI_GOTO_ERROR(FAIL, "MPI_Bcast failed", mpi_code)

        /* Can't handle case when number of chunks is different (yet) */
        if(!num_chunks_same)
            HGOTO_DONE(FALSE);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* H5D_mpio_opt_possible() */


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
H5D_mpio_spaces_xfer(H5D_io_info_t *io_info, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    void *_buf /*out*/, hbool_t do_write )
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
    assert (io_info);
    assert (io_info->dset);
    assert (file_space);
    assert (mem_space);
    assert (buf);
    assert (IS_H5FD_MPIO(io_info->dset->ent.file));
    /* Make certain we have the correct type of property list */
    assert(TRUE==H5P_isa_class(io_info->dxpl_id,H5P_DATASET_XFER));

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
    if(io_info->dset->shared->layout.type == H5D_CONTIGUOUS)
       addr = H5D_contig_get_addr(io_info->dset) + mpi_file_offset;
    else {
        haddr_t   chunk_addr; /* for collective chunk IO */

        assert(io_info->dset->shared->layout.type == H5D_CHUNKED); 
        chunk_addr=H5D_istore_get_addr(io_info,NULL);
        addr = H5F_BASE_ADDR(io_info->dset->ent.file) + chunk_addr + mpi_file_offset;
    }

    /*
     * Pass buf type, file type to the file driver. Request an MPI type
     * transfer (instead of an elementary byteblock transfer).
     */
    if(H5FD_mpi_setup_collective(io_info->dxpl_id, mpi_buf_type, mpi_file_type)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI-I/O properties");
    plist_is_setup=1;

    /* Adjust the buffer pointer to the beginning of the selection */
    buf+=mpi_buf_offset;

    /* transfer the data */
    if (do_write) {
    	if (H5F_block_write(io_info->dset->ent.file, H5FD_MEM_DRAW, addr, mpi_buf_count, io_info->dxpl_id, buf) <0)
	    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,"MPI write failed");
    } else {
    	if (H5F_block_read (io_info->dset->ent.file, H5FD_MEM_DRAW, addr, mpi_buf_count, io_info->dxpl_id, buf) <0)
	    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL,"MPI read failed");
    }

done:
    /* Reset the dxpl settings */
    if(plist_is_setup) {
        if(H5FD_mpi_teardown_collective(io_info->dxpl_id)<0)
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


/** The following function has been tested, don't call this
    function until you don't see this line. Nov. 11,2004, KY**/

static herr_t
H5D_mpio_spaces_span_xfer(H5D_io_info_t *io_info, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    void *_buf /*out*/, hbool_t do_write )
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

    FUNC_ENTER_NOAPI_NOINIT(H5D_mpio_spaces_span_xfer);

    /* Check args */
    assert (io_info);
    assert (io_info->dset);
    assert (file_space);
    assert (mem_space);
    assert (buf);
    assert (IS_H5FD_MPIO(io_info->dset->ent.file));
    /* Make certain we have the correct type of property list */
    assert(TRUE==H5P_isa_class(io_info->dxpl_id,H5P_DATASET_XFER));

    printf("coming to span tree xfer \n");
    /* create the MPI buffer type */
    if(H5S_SELECT_IS_REGULAR(mem_space)==TRUE){
      if (H5S_mpio_space_type( mem_space, elmt_size,
			       /* out: */
			       &mpi_buf_type,
			       &mpi_buf_count,
			       &mpi_buf_offset,
			       &mbt_is_derived )<0)
    	HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI buf type");}
      else {
    if (H5S_mpio_space_span_type( mem_space, elmt_size,
			       /* out: */
			       &mpi_buf_type,
			       &mpi_buf_count,
			       &mpi_buf_offset,
			       &mbt_is_derived )<0)
    	HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI buf type");
      }
    printf("mpi_buf_count %d\n",mpi_buf_count);
    /* create the MPI file type */

       if(H5S_SELECT_IS_REGULAR(file_space)== TRUE){ 
 if ( H5S_mpio_space_type( file_space, elmt_size,
			       /* out: */
			       &mpi_file_type,
			       &mpi_file_count,
			       &mpi_file_offset,
			       &mft_is_derived )<0)
    	HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI file type");
       }
       else {
	 if ( H5S_mpio_space_span_type( file_space, elmt_size,
			       /* out: */
			       &mpi_file_type,
			       &mpi_file_count,
			       &mpi_file_offset,
			       &mft_is_derived )<0)
    	HGOTO_ERROR(H5E_DATASPACE, H5E_BADTYPE, FAIL,"couldn't create MPI file type");
       }
    /* Get the base address of the contiguous dataset or the chunk */
    if(io_info->dset->shared->layout.type == H5D_CONTIGUOUS)
       addr = H5D_contig_get_addr(io_info->dset) + mpi_file_offset;
    else {
        haddr_t   chunk_addr; /* for collective chunk IO */

        assert(io_info->dset->shared->layout.type == H5D_CHUNKED); 
        chunk_addr=H5D_istore_get_addr(io_info,NULL);
        addr = H5F_BASE_ADDR(io_info->dset->ent.file) + chunk_addr + mpi_file_offset;
    }

    /*
     * Pass buf type, file type to the file driver. Request an MPI type
     * transfer (instead of an elementary byteblock transfer).
     */
    if(H5FD_mpi_setup_collective(io_info->dxpl_id, mpi_buf_type, mpi_file_type)<0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "can't set MPI-I/O properties");
    plist_is_setup=1;

    /* Adjust the buffer pointer to the beginning of the selection */
    buf+=mpi_buf_offset;

    /* transfer the data */
    if (do_write) {
    	if (H5F_block_write(io_info->dset->ent.file, H5FD_MEM_DRAW, addr, mpi_buf_count, io_info->dxpl_id, buf) <0)
	    HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL,"MPI write failed");
    } else {
    	if (H5F_block_read (io_info->dset->ent.file, H5FD_MEM_DRAW, addr, mpi_buf_count, io_info->dxpl_id, buf) <0)
	    HGOTO_ERROR(H5E_IO, H5E_READERROR, FAIL,"MPI read failed");
    }

done:
    /* Reset the dxpl settings */
    if(plist_is_setup) {
        if(H5FD_mpi_teardown_collective(io_info->dxpl_id)<0)
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
} /* end H5D_mpio_spaces_span_xfer() */


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
H5D_mpio_spaces_read(H5D_io_info_t *io_info,
    size_t UNUSED nelmts, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    void *buf/*out*/)
{
    herr_t ret_value;

    FUNC_ENTER_NOAPI_NOFUNC(H5D_mpio_spaces_read);

    ret_value = H5D_mpio_spaces_xfer(io_info, elmt_size, file_space,
        mem_space, buf, 0/*read*/);

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
H5D_mpio_spaces_write(H5D_io_info_t *io_info,
    size_t UNUSED nelmts, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    const void *buf)
{
    herr_t ret_value;

    FUNC_ENTER_NOAPI_NOFUNC(H5D_mpio_spaces_write);

    /*OKAY: CAST DISCARDS CONST QUALIFIER*/
    ret_value = H5D_mpio_spaces_xfer(io_info, elmt_size, file_space,
        mem_space, (void*)buf, 1/*write*/);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5D_mpio_spaces_write() */



/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_spaces_span_read
 *
 * Purpose:	MPI-IO function to read directly from app buffer to file for
                span-tree
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	KY
 * Note      :  Don't call this routine
 *              until you don't see this line. 11/11/2004, KY
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
H5D_mpio_spaces_span_read(H5D_io_info_t *io_info,
    size_t UNUSED nelmts, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    void *buf/*out*/)
{
    herr_t ret_value;

    FUNC_ENTER_NOAPI_NOFUNC(H5D_mpio_spaces_span_read);

    ret_value = H5D_mpio_spaces_span_xfer(io_info, elmt_size, file_space,
        mem_space, buf, 0/*read*/);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5D_mpio_spaces_read() */


/*-------------------------------------------------------------------------
 * Function:	H5D_mpio_spaces_span_write
 *
 * Purpose:	MPI-IO function to write directly from app buffer to file.
 *
 * Return:	non-negative on success, negative on failure.
 *
 * Programmer:	KY
 * Note:        Don't call this funtion until you don't see this line.
 *              KY,  11/11/04
                 
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
H5D_mpio_spaces_span_write(H5D_io_info_t *io_info,
    size_t UNUSED nelmts, size_t elmt_size,
    const H5S_t *file_space, const H5S_t *mem_space,
    const void *buf)
{
    herr_t ret_value;

    FUNC_ENTER_NOAPI_NOFUNC(H5D_mpio_spaces_span_write);

    printf(" coming to spaces_span_write function\n");
    fflush(stdout);
    /*OKAY: CAST DISCARDS CONST QUALIFIER*/
    printf("element size %d\n",elmt_size);
    ret_value = H5D_mpio_spaces_span_xfer(io_info, elmt_size, file_space,
        mem_space, (void*)buf, 1/*write*/);

    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5D_mpio_spaces_span_write() */
#endif  /* H5_HAVE_PARALLEL */
