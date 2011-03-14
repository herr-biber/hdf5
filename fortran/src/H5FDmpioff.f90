!
! This file contains Fortran90 interfaces for H5P functions needed by || MPI programs.
!
     MODULE H5FDMPIO
         USE H5GLOBAL
         CONTAINS

!----------------------------------------------------------------------
! Name:		h5pset_fapl_mpio_f 
!
! Purpose: 	Stores MPI IO communicator information to the file 
!		access property list. 
!
! Inputs:  	
!		prp_id		- file access property list identifier
!		comm		- MPI-2 communicator
!		info		- MPI-2 info object
! Outputs:  
!		hdferr:		- error code		
!				 	Success:  0
!				 	Failure: -1   
! Optional parameters:
!				NONE
!
! Programmer:	Elena Pourmal
!		November, 2000
!
! Modifications:
!
! Comment:		
!----------------------------------------------------------------------
         SUBROUTINE h5pset_fapl_mpio_f(prp_id, comm, info, hdferr) 
            IMPLICIT NONE
            INTEGER(HID_T), INTENT(IN) :: prp_id ! Property list identifier
            INTEGER, INTENT(IN) :: comm ! MPI communicator to be used for file open
                                        ! as defined in MPI_FILE_OPEN of MPI-2
            INTEGER, INTENT(IN) :: info ! MPI info object to be used for file open
                                        ! as defined in MPI_FILE_OPEN of MPI-2
            INTEGER, INTENT(OUT) :: hdferr  ! Error code

            INTEGER, EXTERNAL :: h5pset_fapl_mpio_c
            hdferr = h5pset_fapl_mpio_c(prp_id, comm, info)
          END SUBROUTINE h5pset_fapl_mpio_f

!----------------------------------------------------------------------
! Name:		h5pget_fapl_mpio_f 
!
! Purpose: 	Returns MPI communicator information. 	
!
! Inputs:  
!		prp_id		- file access property list identifier
! Outputs:  
!		comm		- MPI-2 communicator
!		info		- MPI-2 info object
!		hdferr:		- error code		
!				 	Success:  0
!				 	Failure: -1   
! Optional parameters:
!				NONE
!
! Programmer:	Elena Pourmal
!		November, 2000
!
! Modifications: 
!
! Comment:		
!----------------------------------------------------------------------

          SUBROUTINE h5pget_fapl_mpio_f(prp_id, comm, info, hdferr) 
            IMPLICIT NONE
            INTEGER(HID_T), INTENT(IN) :: prp_id ! Property list identifier
            INTEGER, INTENT(OUT) :: comm ! buffer to return communicator 
            INTEGER, INTENT(OUT) :: info ! buffer to return info object 
                                        ! as defined in MPI_FILE_OPEN of MPI-2
            INTEGER, INTENT(OUT) :: hdferr  ! Error code

            INTEGER, EXTERNAL :: h5pget_fapl_mpio_c
            hdferr = h5pget_fapl_mpio_c(prp_id, comm, info)
          END SUBROUTINE h5pget_fapl_mpio_f

!----------------------------------------------------------------------
! Name:		h5pset_dxpl_mpio_f 
!
! Purpose: 	Sets data transfer mode. 
!
! Inputs:  
!		prp_id		- data transfer property list identifier
!		data_xfer_mode	- transfer mode
!				  Possible values are:
!				  H5FD_MPIO_INDEPENDENT_F
!				  H5FD_MPIO_COLLECTIVE_F
! Outputs:  
!		hdferr:		- error code		
!				 	Success:  0
!				 	Failure: -1   
! Optional parameters:
!				NONE
!
! Programmer:	Elena Pourmal
!		November, 2000
!
! Modifications: 	
!
! Comment:		
!----------------------------------------------------------------------

         SUBROUTINE h5pset_dxpl_mpio_f(prp_id, data_xfer_mode, hdferr) 
            IMPLICIT NONE
            INTEGER(HID_T), INTENT(IN) :: prp_id ! Property list identifier
            INTEGER, INTENT(IN) :: data_xfer_mode ! Data transfer mode. Possible values are:
                                                  ! H5FD_MPIO_INDEPENDENT_F
                                                  ! H5FD_MPIO_COLLECTIVE_F
            INTEGER, INTENT(OUT) :: hdferr  ! Error code

            INTEGER, EXTERNAL :: h5pset_dxpl_mpio_c 
            hdferr = h5pset_dxpl_mpio_c(prp_id, data_xfer_mode)
          END SUBROUTINE h5pset_dxpl_mpio_f

!----------------------------------------------------------------------
! Name:		h5pget_dxpl_mpio_f 
!
! Purpose: 	Returns the data transfer mode. 
!
! Inputs:  
!		prp_id		- data transfer property list identifier
! Outputs:  
!		data_xfer_mode	- transfer mode
!				  Possible values are:
!				  H5FD_MPIO_INDEPENDENT_F
!				  H5FD_MPIO_COLLECTIVE_F
!		hdferr:		- error code		
!				 	Success:  0
!				 	Failure: -1   
! Optional parameters:
!				NONE
!
! Programmer:	Elena Pourmal
!		November, 2000
!
! Modifications: 
!
! Comment:		
!----------------------------------------------------------------------

         SUBROUTINE h5pget_dxpl_mpio_f(prp_id, data_xfer_mode, hdferr) 
            IMPLICIT NONE
            INTEGER(HID_T), INTENT(IN) :: prp_id ! Property list identifier
            INTEGER, INTENT(OUT) :: data_xfer_mode ! Data transfer mode. Possible values are:
                                                  ! H5FD_MPIO_INDEPENDENT_F
                                                  ! H5FD_MPIO_COLLECTIVE_F
            INTEGER, INTENT(OUT) :: hdferr  ! Error code

            INTEGER, EXTERNAL :: h5pget_dxpl_mpio_c
            hdferr = h5pget_dxpl_mpio_c(prp_id, data_xfer_mode)
          END SUBROUTINE h5pget_dxpl_mpio_f

    END MODULE H5FDMPIO
