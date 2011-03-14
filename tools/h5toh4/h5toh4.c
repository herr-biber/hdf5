/******************************************************************************

  Description: This file contains routines to translate H5 files to H4 files.
 
  Author:  Paul Harten for the University of Illinois, NCSA
 
  Creation Date:  04 October 1998

  History:


*****************************************************************************/

#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include "h5toh4.h"
#include "h5tools.h"
#include "h5tools_utils.h"


#ifdef WIN32
typedef unsigned int mode_t;
#endif  /* WIN32 */

#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode)&0xF000) == S_IFDIR)
#endif
extern void PrintOptions_h5toh4(void);
extern char *BuildFilename(char *h5_filename, char *h4_extension);
extern int test_file(char *filename, int oflag, mode_t mode);
extern int test_dir(char *);
extern int h5toh4(char *, char *);

extern herr_t convert_group(hid_t, char *, op_data_t *);
extern herr_t convert_dataset(hid_t, char *, op_data_t *);
extern herr_t convert_all(hid_t, char *, op_data_t *);
extern herr_t convert_attr(hid_t, char *, op_data_t *);
extern herr_t convert_shared_dataset(hid_t, int, op_data_t *);
extern herr_t convert_shared_group(hid_t, int, op_data_t *);
extern herr_t convert_dataset_string(hid_t, char *, op_data_t *);
extern int32 h5type_to_h4type(hid_t);
extern hid_t h4type_to_memtype(int32);

typedef herr_t (*H5G_operator_t)(hid_t, const char*, void*);

static int prefix_len = 1024;
static char *prefix;
static table_t *group_table, *dset_table, *type_table;

static herr_t h5atomic_type_to_h4type(const hid_t h5type, hid_t* h5memtype, size_t* h5memsize, int32* h4type);


/*****************************************************************************

  Routine: main()
 
  Description: This routine check out arguments sent, makes sure everything is
               ok, chooses between the acceptable argument formats and then
               calls h5toh4().
           
  Input: arg	- the number of arguments from the call + 1;
         argv	- a pointer to an array of strings which are the arguments.
                  This includes the routine name as the first string.
 
  Output: function return	- error status

*****************************************************************************/

int
main(int argc, char *argv[])
{
	char *h5_filename=NULL;
	char *h4_filename=NULL;
	char *h4_extension;
	int status = 0;
	int status2 = 0;


	argc--;
	argv++;

	if (argc == 0) {
		fprintf(stderr,"\nError: Invalid Arguments\n");
		PrintOptions_h5toh4();
		return -1;
	}

	/* take care -h (help) option first */
	{   int i;
	    for (i=0; i < argc; i++)
		if ( HDstrcmp(argv[i],"-h") == 0 ) {
			PrintOptions_h5toh4();
			return 0;
		}
	}

	/*fargv = argv; */

	/* Change -m with 1 filename to as 1 filename case */
	if (argc == 2 && HDstrcmp(argv[0],"-m") == 0) {
		argv++;
		argc--;
	}

	switch(argc) {

	case 0:

		PrintOptions_h5toh4();
		break;

	case 1:	/* h5toh4 file1 */
		h5_filename = argv[0];

		if (HDstrcmp(h5_filename,"-m") == 0) {
			fprintf(stderr,"\nError: Invalid Arguments\n");
			PrintOptions_h5toh4();
			status = -1;
			break;
		}
#ifndef WIN32
		if (test_file(h5_filename,O_EXCL,(mode_t)(S_IRUSR|S_IRGRP|S_IROTH)) != 0 ) { /* 292 Decimal - 0444 Octal, a+r */
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
			status = -1;
			break;
		}
		if (test_dir(h5_filename) != 0 ) {
			fprintf(stderr,"%s: Is a directory\n",h5_filename);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
			status = -1;
			break;
		}
#endif
		h4_extension = HDstrdup("hdf");

		h4_filename = BuildFilename(h5_filename,h4_extension);
		if (h4_filename == NULL) {
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
			status = -1;
			break;
		}
#ifndef WIN32
		if (test_file(h4_filename,O_CREAT|O_EXCL,(mode_t)(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH)) != 0) { /* 436 Decimal - 0664 Octal, ug+rw,o+r */
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
			status = -1;
			break;
		}
#endif
		status = h5toh4(h5_filename, h4_filename);
		if ( status != 0 ) {
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
		}
		if (h4_filename != NULL) {
			HDfree(h4_filename);
		}

		break;

	case 2:	/* h5toh4 file_in file_out */
		h5_filename = argv[0];
		h4_filename = argv[1];

		if (HDstrcmp(h4_filename,"-m") == 0) {
			fprintf(stderr,"\nError: Invalid Arguments\n");
			PrintOptions_h5toh4();
			status = -1;
			break;
		}
#ifndef WIN32
		if (test_file(h5_filename,O_EXCL,292) != 0 ) { /* 292 Decimal - 0444 Octal, a+r */
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
			status = -1;
			break;
		}
		if (test_dir(h5_filename) != 0 ) {
			fprintf(stderr,"%s: Is a directory\n",h5_filename);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
			status = -1;
			break;
		}
		if (test_file(h4_filename,O_CREAT|O_RDWR,436) != 0) { /* 436 Decimal - 0664 Octal, ug+rw,o+r */
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
			status = -1;
			break;
		}
		if (test_dir(h4_filename) != 0 ) {
			fprintf(stderr,"%s: Is a directory\n",h4_filename);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
			status = -1;
			break;
		}
#endif
		status = h5toh4(h5_filename, h4_filename);
		if ( status != 0 ) {
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
		}
		break;

	default:	/* h5toh4 -m file1 file2 file3 ... */
		if (HDstrcmp(argv[0],"-m") != 0) {
			fprintf(stderr,"\nError: Invalid Arguments\n");
			PrintOptions_h5toh4();
			status = -1;
			break;
		}
		argv++;
		argc--;

		while ( argc > 0 ) {
			h5_filename = argv[0];
			argv++;
			argc--;

			if (HDstrcmp(h5_filename,"-m") == 0) {
				fprintf(stderr,"\nError: Invalid Arguments\n");
				PrintOptions_h5toh4();
				status2 = -1;
				break;
			}
#ifndef WIN32
			if (test_file(h5_filename,O_EXCL,292) != 0 ) { /* 292 Decimal - 0444 Octal, a+r */
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
				status2 = -1;
				continue;
			}
			if (test_dir(h5_filename) != 0 ) {
				fprintf(stderr,"%s: Is a directory\n",h5_filename);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
				status2 = -1;
				continue;
			}
#endif
			h4_extension = HDstrdup("hdf");

			h4_filename = BuildFilename(h5_filename,h4_extension);
			if (h4_filename == NULL) {
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
				status2 = -1;
				continue;
			}
#ifndef WIN32
			if (test_file(h4_filename,O_CREAT|O_EXCL,436) != 0) { /* 436 Decimal - 0664 Octal, ug+rw,o+r */
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
				status2 = -1;
				continue;
			}
#endif
			status = h5toh4(h5_filename, h4_filename);
			if ( status != 0 ) {
				fprintf(stderr,"Error: Problem with %s\n",h5_filename);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "main", __FILE__, __LINE__);
				status2 = status;
			}
			if (h4_filename != NULL) {
				HDfree(h4_filename);
			}

		}
		status = status2;
		break;
	}

	return status;

}


/*****************************************************************************

  Routine: PrintOptions_h5toh4()
 
  Description: This routine prints the acceptable argument formats out to stderr.
           
  Input: None
 
  Output: output to stderr

*****************************************************************************/

void PrintOptions_h5toh4(void)
{
		fprintf(stderr,"\nh5toh4 -h (gives this print-out)\n");
		fprintf(stderr,"h5toh4 file.h5 file.hdf\n");
		fprintf(stderr,"h5toh4 file.h5\n");
		fprintf(stderr,"h5toh4 -m file1.h5 file2.h5 ...\n\n");
}


/*****************************************************************************

  Routine: h5toh4()
 
  Description: This routine translates information from a HDF5 file into a
               HDF4 file.
           
  Input: h5_filename	- filename of HDF5 file,
         h4_filename	- filename of HDF4 file
 
  Output: function return	- error status

*****************************************************************************/

int h5toh4(char *h5_filename, char *h4_filename)
{

	hid_t fid, gid;
	hid_t plist=H5P_DEFAULT;
	int status = 0;
	int32 hfile_id;
	int32 sd_id;
	op_data_t op_data;
	void *edata;
	hid_t (*func)(void*);


	find_objs_t *info = malloc(sizeof(find_objs_t));



	/* open hdf5 file */
	if ((fid = H5Fopen (h5_filename, H5F_ACC_RDONLY, plist)) <= 0) {
		fprintf(stderr,"Error: Unable to open file %s\n",h5_filename);
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
		return (fid);
	}

	/* open root group */
	if ((gid = H5Gopen (fid, "/")) <= 0 ) {
		fprintf(stderr,"Error: Unable to open root group\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
		return (gid);
	} else {

		/* open hdf4 file */
		if ((hfile_id = Hopen(h4_filename, DFACC_CREATE, 0)) <= 0) {
			fprintf(stderr,"Error: Unable to open file %s\n", h4_filename);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
			return (hfile_id);
		} else {

			if ((sd_id = SDstart(h4_filename, DFACC_RDWR)) <= 0) {
				fprintf(stderr,"Error: Unable to open file %s, using SDstart\n", h4_filename);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
				return (sd_id);
			} else {

				/* Initialize Vgroup interface */
				if ((status = Vstart(hfile_id)) != SUCCEED) {
					fprintf(stderr,"Error: Unable to initialize Vgroup interface\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
					status = FAIL;
				}

				/* allocate and initialize internal data structure */
				init_table(&group_table);
				init_table(&type_table);
				init_table(&dset_table);
				init_prefix(&prefix, prefix_len);

				/* init the find_objs_t*/
				info->threshold = 0;
				info->prefix_len = prefix_len;
				info->prefix = prefix;
				info->group_table = group_table;
				info->type_table = type_table;
				info->dset_table = dset_table;
				info->status = status;



				/* Disable error reporting */
				H5Eget_auto (&func, &edata);
				H5Eset_auto (NULL, NULL);

				/* find all objects one time */
				if ((status = H5Giterate(fid, "/", NULL, (H5G_operator_t)find_objs, (void*)info)) != SUCCEED ) {
					fprintf(stderr,"Error: Unable to iterate over all of the groups\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
				}

				/* Enable error reporting */
				H5Eset_auto (func, edata);

#ifdef H5DUMP_DEBUG
				dump_tables();
#endif

				if (status != SUCCEED) {
					fprintf(stderr,"Error: internal error! \n");
					goto done;
				}

				op_data.hfile_id = hfile_id;
				op_data.sd_id = sd_id;
				op_data.vgroup_id = 0;
 	
 				/* start at root group */
				if (( status = convert_group (gid, HDstrdup("/"), &op_data)) != SUCCEED) {
					fprintf(stderr,"Error: convert_group did not work for %s\n","/");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
					status = FAIL;
				}

	  			/* Terminate access to Vgroup interface */
				if ((status = Vend(hfile_id)) != SUCCEED) {
					fprintf(stderr,"Error: Unable to terminate Vgroup interface\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
					status = FAIL;
				}
			}

			/* close hdf4 SDS */
			if ((status = SDend(sd_id)) != SUCCEED) {
				fprintf(stderr,"Error: Unable to close file %s, from SDstart\n", h4_filename);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
				status = FAIL;
			}

		}

		/* close hdf4 file */
		if ((status = Hclose(hfile_id)) != SUCCEED) {
			fprintf(stderr,"Error: Unable to close file %s\n", h4_filename);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
			status = FAIL;
		}

	}

	if ((status = H5Gclose (gid)) != SUCCEED) {
		fprintf(stderr,"Error: Unable to close root group\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
		status = FAIL;
	}

	if ((status = H5Fclose (fid)) != SUCCEED) {
		fprintf(stderr,"Error: Unable to close file %s\n", h5_filename);
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5toh4", __FILE__, __LINE__);
		status = FAIL;
	}

done:
	free_table(&group_table);
	free_table(&dset_table);
	free_table(&type_table);

	return status;

}

/*-------------------------------------------------------------------------
 * Function:    convert_group
 *
 * Purpose:     Dump everything within the specified group
 *
 * Return:      status
 *
 * Programmer:  Paul Harten
 *
 * Modifications: 
 *
 *-----------------------------------------------------------------------*/
herr_t
convert_group (hid_t gid, char *name, op_data_t *op_data) {
H5G_stat_t statbuf;

	int32 hfile_id;
	int32 vgroup_id;
	int32 obj_idx;
	int32 status;
	int idx, flag;

	hfile_id = op_data->hfile_id;

	if ((vgroup_id = Vattach(hfile_id, -1, "w")) <= 0 ) {
		fprintf(stderr,"Error: Unable to create new vgroup\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
		return(vgroup_id);
	}

	if ((status = Vsetname(vgroup_id, name)) != SUCCEED ) {
		fprintf(stderr,"Error: Unable to set name on new vgroup\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
		return(status);
	}

	if ((status = Vsetclass(vgroup_id, "HDF5")) != SUCCEED ) {
		fprintf(stderr,"Error: Unable to set class on new vgroup\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
		return(status);
	}

	if (op_data->vgroup_id == 0) {
		obj_idx = -1; /* Indexes assigned below start from 0 */
	} else {
		if ((obj_idx = Vinsert(op_data->vgroup_id, vgroup_id)) < 0) {
			fprintf(stderr,"Error: Index %d of the new vgroup is illegal\n",(int)obj_idx);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
			return(obj_idx);
		}
	}
	op_data->vgroup_id = vgroup_id;
	op_data->sds_id = 0;
	op_data->vdata_id = 0;
	op_data->obj_idx = obj_idx;
		

	/* hard link */
	if ((status = H5Gget_objinfo(gid, ".", TRUE, &statbuf)) != SUCCEED ) {
		fprintf(stderr,"Error: H5Gget_objinfo() did not work\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
		return (status);
	}

        if (HDstrcmp(name,"/") == 0) { /* this is the root group, just iterate */

		if ((status = H5Aiterate(gid, NULL, (H5A_operator_t)convert_attr, op_data)) != SUCCEED ) {
			fprintf(stderr,"Error: Unable to iterate over all of the attributes\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
			return(status);
		}

		if ((status = H5Giterate(gid, ".", NULL, (H5G_operator_t)convert_all, op_data)) != SUCCEED ) {
			fprintf(stderr,"Error: Unable to iterate over all of the groups\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
			return(status);
		}

	} else {

		if ((idx = get_table_idx(group_table, statbuf.objno)) < 0 ) {

			fprintf(stderr,"Error: object not found, %s\n",name);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
			status = FAIL;

		} else if((flag = get_tableflag(group_table,idx)) < 0 ) {
			
			fprintf(stderr,"Error: get_tableflag() should never return < 0\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
			status = FAIL;

		} else if(flag == TRUE ) { /* this has already been converted, don't convert the attributes again */

			if ((status = H5Giterate(gid, ".", NULL, (H5G_operator_t)convert_all, op_data)) != SUCCEED ) {
				fprintf(stderr,"Error: Unable to iterate over all of the groups\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
				return(status);
			}

		} else { /* flag == FALSE */

			/* this is now being converted */
			if ((status = set_tableflag(group_table,idx)) < 0 ) {
				fprintf(stderr,"Error: set_tableflag should never return < 0\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
				return(status);
			}

			if ((status = H5Aiterate(gid, NULL, (H5A_operator_t)convert_attr, op_data)) != SUCCEED ) {
				fprintf(stderr,"Error: Unable to iterate over all of the attributes\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
				return(status);
			}

			if ((status = H5Giterate(gid, ".", NULL, (H5G_operator_t)convert_all, op_data)) != SUCCEED ) {
				fprintf(stderr,"Error: Unable to iterate over all of the groups\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
				return(status);
			}

		}

	}


	if ((status = Vdetach(vgroup_id)) != SUCCEED ) {
		fprintf(stderr,"Error: Unable to detach the new Vgroup\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_group", __FILE__, __LINE__);
		status = FAIL;
	}

	return status;

}
 

/*-------------------------------------------------------------------------
 * Function:    convert_dataset
 *
 * Purpose:     Convert the specified data set
 *
 * Return:      status
 *
 * Programmer:  Paul Harten
 *
 * Modifications:
 *
 *-----------------------------------------------------------------------*/
herr_t
convert_dataset (hid_t did, char *name, op_data_t *op_data) {
hid_t  type, space, class, mem_type, type2;
/* H5G_stat_t statbuf; */
size_t typesize;
int i, idx;
int32 dim_sizes[32], start[32], edges[32];
int ndims;
int ndimf;
hsize_t dims[32], maxdims[32];
hsize_t dimf[4];
int permf[4];
int32 hfile_id;
int32 sd_id;
int32 sds_id;
int32 vdata_id;
int32 vgroup_id;
int32 n_values;
int32 status;
int32 h4_type;
int32 recsize;
int32 n_records, num_of_recs, record_pos;
int nmembers;
char *buffer=NULL; /* read/write buffer*/
char fieldname_list[4096] = "\0";
char *fieldname=NULL;
hid_t fieldtype;
int32 order;
off_t offset;
off_t offset_array[512];
hid_t h4type_array[512], memtype_array[512];
int32 order_array[512];


    /* hard link */
/*
    if ((status = H5Gget_objinfo(did, ".", TRUE, &statbuf)) != SUCCEED ) {
	fprintf(stderr,"Error: H5Gget_objinfo() did not work\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	return (status);
    }
*/

    if ((type = H5Dget_type(did)) <= 0) {
	fprintf(stderr, "Error: H5Dget_type() didn't return appropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	status = FAIL;
        return status;
    }

    if ((space = H5Dget_space(did)) <= 0) {
	fprintf(stderr, "Error: H5Dget_space() didn't return appropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	status = FAIL;
        return status;
    }

    if ((n_values = H5Sget_simple_extent_npoints(space)) <= 0) {
	fprintf(stderr, "Error: H5Sget_simple_extent_npoints() returned inappropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	status = FAIL;
        return status;
    }

    if ((ndims = H5Sget_simple_extent_dims(space,dims,maxdims)) < 0 ) {
	fprintf(stderr, "Error: Problems getting ndims, dims, and maxdims of dataset\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	status = ndims;
        return status;
    }

    if ((class = H5Tget_class(type)) < 0 ) {
        fprintf(stderr,"Error: problem with getting class\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	status = class;
        return status;
    }

    switch (class) {
    case H5T_INTEGER:
    case H5T_FLOAT:
        sd_id = op_data->sd_id;
#define NEWWAY
#ifdef NEWWAY
	if (FAIL==h5atomic_type_to_h4type(type, &mem_type, &typesize, &h4_type)){
		fprintf(stderr, "Error: Problems translating h5 type to h4 type\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
		status = FAIL;
		break;
	}
#else
	if ((h4_type = h5type_to_h4type(type)) == FAIL ) {
		fprintf(stderr, "Error: Problems translating h5 type to h4 type\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
		status = FAIL;
		break;
	}
	if ((mem_type = h4type_to_memtype(h4_type)) == FAIL ) {
		fprintf(stderr, "Error: Problems translating h4 type to mem type\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
		status = FAIL;
        	return status;
	}
	if ((typesize = H5Tget_size(mem_type)) <= 0) {
		fprintf(stderr, "Error: H5Tget_size() didn't return appropriate value.\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
		status = FAIL;
		break;
	}
#endif
    	if ((buffer = HDmalloc(n_values*typesize)) == NULL) {
		fprintf(stderr, "Error: Problems with HDmalloc of memory space\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
		status = FAIL;
		break;
	}
        if ((status = H5Dread(did, mem_type, space, space, H5P_DEFAULT, buffer)) != SUCCEED) {
		fprintf(stderr, "Error: Problems with H5Dread\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
		status = FAIL;
		break;
	}
	for (i=0;i<ndims;i++) {
		if (maxdims[i] == H5S_UNLIMITED) {
			if ( i == 0 ) {
				dim_sizes[0] = 0;	/* this is how HDF4 communicates unlimited dimension */
			} else {
				dim_sizes[i] = (int32)dims[i];
			}
		} else {
			dim_sizes[i] = (int32)maxdims[i];
		}
		start[i] = 0;
		edges[i] = (int32)dims[i];
	}
	if ((sds_id = SDcreate(sd_id, name, h4_type, ndims, dim_sizes)) <= 0 ) {
		fprintf(stderr, "Error: Unable to create SDS %s.\n",name);
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
		status = FAIL;
		break;
	}
	op_data->sds_id = sds_id;
	if ((status = SDwritedata(sds_id, start, NULL, edges, buffer)) != SUCCEED ) {
		fprintf(stderr, "Error: Unable to write SDS %s.\n",name);
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	}
    	if ((status = H5Aiterate(did, NULL, (H5A_operator_t)convert_attr, op_data)) < 0 ) {
        	fprintf(stderr,"Error: iterate over attributes\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
    	}
	if ((status = SDendaccess(sds_id)) != SUCCEED ) {
		fprintf(stderr, "Error: Unable to end access to SDS %s.\n",name);
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	}
        break;
    case H5T_TIME:
        fprintf(stderr,"Warning: H5T_TIME not yet implemented.\n");
        break;
    case H5T_STRING:
        if (ndims==1) {
		convert_dataset_string(did, name, op_data);
	} else {
        	fprintf(stderr,"Warning: HDF5 datasets of H5T_STRING type with ndims > 1 are not converted.\n");
	}
        break;
    case H5T_BITFIELD:
        fprintf(stderr,"Warning: H5T_BITFIELD not yet implemented.\n");
        break;
    case H5T_OPAQUE:
        fprintf(stderr,"Warning: H5T_OPAQUE not yet implemented.\n");
        break;
    case H5T_ARRAY:
        fprintf(stderr,"Warning: H5T_OPAQUE not yet implemented.\n");
        break;
    case H5T_COMPOUND:
	if (ndims==1) {
		if ((nmembers = H5Tget_nmembers(type)) <= 0 ) {
			fprintf(stderr, "Error: Unable to get information about compound datatype %d\n",nmembers);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
		}

		offset = 0;
		for (idx=0;idx<nmembers;idx++) {
			if ((fieldtype = H5Tget_member_type(type, idx)) < 0 ) {
        			fprintf(stderr,"Error: H5 datasets of H5T_COMPOUND type with fieldtype %d, idx %d.\n",fieldtype,idx);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
				break;
			}
#ifdef H5_WANT_H5_V1_2_COMPAT
            /* v1.2 returns the base type of an array field, work around this */
            {
                hid_t new_fieldtype;         /* datatype for array, if necessary */
                int     arrndims;       /* Array rank for reading */
                size_t	dims[H5S_MAX_RANK];    /* Array dimensions for reading */
                hsize_t	arrdims[H5S_MAX_RANK];    /* Array dimensions for reading */
                int k;              /* Local index variable */

                /* Get the array dimensions */
                arrndims=H5Tget_member_dims(type,idx,dims,NULL);

                /* Patch up array information */
                if(arrndims>0) {
                    for(k=0; k<arrndims; k++)
                        arrdims[k]=dims[k];
                    new_fieldtype=H5Tarray_create(fieldtype,arrndims,arrdims,NULL);
                    H5Tclose(fieldtype);
                    fieldtype=new_fieldtype;
                } /* end if */
            }
#endif /* WANT_H5_V1_2_COMPAT */
            /* Special case for array fields */
            if(H5Tget_class(fieldtype)==H5T_ARRAY) {
                hid_t   arr_base_type;

                /* Get the number of dimensions */
                if ((ndimf = H5Tget_array_ndims(fieldtype)) < 0 || ndimf > H5S_MAX_RANK ) {
                    fprintf(stderr, "Error: rank of members of compound type should not be %d\n",ndimf);
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                        return FAIL;
                } 
                /* Get the dimensions and dimension permutations */
                if (H5Tget_array_dims(fieldtype,dimf,permf) < 0) {
                    fprintf(stderr, "Error: cannot retrieve dimensions for array\n");
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                        return FAIL;
                } 
                /* Get the 'real' field type */
                if ((arr_base_type=H5Tget_super(fieldtype)) < 0) {
                    fprintf(stderr, "Error: cannot retrieve base type for array\n");
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                        return FAIL;
                } 
                if (H5Tclose(fieldtype) < 0 ) {
                    fprintf(stderr,"Error: closing type\n");
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                        return FAIL;
                }

                /* Assign the array's base type as the field type for future use */
                fieldtype=arr_base_type;
            } /* end if */
            else
                ndimf=0;
#ifdef NEWWAY
			if (FAIL==h5atomic_type_to_h4type(fieldtype, &mem_type, &typesize, &h4_type)){
				fprintf(stderr, "Error: Problems translating h5 type to h4 type\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
				status = FAIL;
				break;
			}
#else
			if ((h4_type = h5type_to_h4type(fieldtype)) < 0 ) {
				fprintf(stderr, "Error: Problems translating h5 type to h4 type\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
				break;
			}
			if ((mem_type = h4type_to_memtype(h4_type)) == FAIL ) {
				fprintf(stderr, "Error: Problems translating h4 type to mem type\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
       				return FAIL;
			}
			if ((typesize = H5Tget_size(mem_type)) <= 0) {
				fprintf(stderr, "Error: H5Tget_size() didn't return appropriate value.\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
				status = FAIL;
				break;
			}
#endif
			order = 1;
			if (ndimf > 0) {
				order *= dimf[permf[0]];
				if (ndimf > 1) {
					order *= dimf[permf[1]];
					if (ndimf > 2) {
						order *= dimf[permf[2]];
						if (ndimf > 3) {
							order *= dimf[permf[3]];
						}
					}
				}
			}
			h4type_array[idx] = h4_type;	/* used for VSfdefine */
			memtype_array[idx] = mem_type;	/* used during the build of the memory compound type */
			offset_array[idx] = offset;	/* used during the build of the memory compound type */
			order_array[idx] = order;	/* used for VSfdefine */

			offset += order * typesize;	/* calculating packed size of memory compound type */
		}
		hfile_id = op_data->hfile_id;
		vgroup_id = op_data->vgroup_id;
		if ((vdata_id = VSattach(hfile_id, -1, "w")) <= 0 ) {
			fprintf(stderr, "Error: Unable to create vdata %s.\n",name);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			status = FAIL;
			break;
		}
/*
 *		This step is done during the convert_shared_dataset() call instead of here.
 *
		if ((idx = Vinsert(vgroup_id, vdata_id)) < 0 ) {
			fprintf(stderr, "Error: Unable to insert vdata %s.\n",name);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			status = FAIL;
			break;
		}
*/
		op_data->vdata_id = vdata_id;
		if ((status = VSsetname(vdata_id, name)) != SUCCEED ) {
			fprintf(stderr, "Error: Unable to set vdata name %s.\n",name);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
		}
		if ((status = VSsetclass(vdata_id, "HDF5")) != SUCCEED ) {
			fprintf(stderr, "Error: Unable to set class on vdata %s\n", name);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
		}
		if ((type2 = H5Tcreate(H5T_COMPOUND, (size_t)offset)) <= 0 ) {
			fprintf(stderr, "Error: unable to execute H5Tcreate()\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
		}
		for (idx=0;idx<nmembers;idx++) {
            /* Special case for array fields */
            if(H5Tget_member_class(type,idx)==H5T_ARRAY) {
                hid_t   arr_type;

                if ((arr_type = H5Tget_member_type(type, idx)) < 0 ) {
                        fprintf(stderr,"Error: H5 datasets of H5T_COMPOUND type with fieldtype %d, idx %d.\n",arr_type,idx);
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                    break;
                }
#ifdef H5_WANT_H5_V1_2_COMPAT
            /* v1.2 returns the base type of an array field, work around this */
            {
                hid_t new_arr_type;         /* datatype for array, if necessary */
                int     arrndims;       /* Array rank for reading */
                size_t	dims[H5S_MAX_RANK];    /* Array dimensions for reading */
                hsize_t	arrdims[H5S_MAX_RANK];    /* Array dimensions for reading */
                int k;              /* Local index variable */

                /* Get the array dimensions */
                arrndims=H5Tget_member_dims(type,idx,dims,NULL);

                /* Patch up array information */
                if(arrndims>0) {
                    for(k=0; k<arrndims; k++)
                        arrdims[k]=dims[k];
                    new_arr_type=H5Tarray_create(arr_type,arrndims,arrdims,NULL);
                    H5Tclose(arr_type);
                    arr_type=new_arr_type;
                } /* end if */
            }
#endif /* WANT_H5_V1_2_COMPAT */
                /* Get the number of dimensions */
                if ((ndimf = H5Tget_array_ndims(arr_type)) < 0 || ndimf > H5S_MAX_RANK ) {
                    fprintf(stderr, "Error: rank of members of compound type should not be %d\n",ndimf);
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                        return FAIL;
                } 
                /* Get the dimensions and dimension permutations */
                if (H5Tget_array_dims(arr_type,dimf,permf) < 0) {
                    fprintf(stderr, "Error: cannot retrieve dimensions for array\n");
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                        return FAIL;
                } 
                if (H5Tclose(arr_type) < 0 ) {
                    fprintf(stderr,"Error: closing type\n");
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                        return FAIL;
                }
            } /* end if */
            else
                ndimf=0;

			if ((fieldname = H5Tget_member_name(type, idx)) == NULL ) {
				fprintf(stderr, "Error: Unable to get fieldname for compound type %d, idx %d\n", type, idx);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
				break;
			}
			if ((offset = H5Tget_offset(memtype_array[idx])) < 0 || offset >= 128 ) {
				fprintf(stderr, "Error: H5Tget_offset() is returning a bad value %d\n",(int)offset);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
       				return FAIL;
			}
			if (ndimf == 0 ) {
				if ((status = H5Tinsert(type2,fieldname,offset_array[idx]+offset,memtype_array[idx])) != SUCCEED ) {
					fprintf(stderr, "Error: Problems inserting field into compound datatype\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
       					return FAIL;
				}
			} else {
                hid_t   arr_type;

                /* Create the array datatype */
                if ((arr_type = H5Tarray_create(memtype_array[idx], ndimf, dimf, permf)) < 0 ) {
                        fprintf(stderr,"Error: cannot create array datatype\n");
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                    break;
                }

				if ((status = H5Tinsert(type2,fieldname,offset_array[idx]+offset,arr_type)) != SUCCEED ) {
					fprintf(stderr, "Error: Problems inserting field into compound datatype\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
       					return FAIL;
				}

                if (H5Tclose(arr_type) < 0 ) {
                    fprintf(stderr,"Error: closing type\n");
                    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
                        return FAIL;
                }
			}
			if ((status = VSfdefine(vdata_id, fieldname, h4type_array[idx], order_array[idx])) != SUCCEED ) {
				fprintf(stderr, "Error: Unable to set field %d\n", idx);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
				break;
			}
			strcat(fieldname_list,fieldname);
			if (idx<nmembers-1) {
				strcat(fieldname_list,", ");
			}

			if (fieldname != NULL) {
				HDfree(fieldname);
			}
		}
		if ((status = VSsetfields(vdata_id, fieldname_list)) != SUCCEED ) {
			fprintf(stderr, "Error: Unable to set fieldname list  %s\n", name);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
		}
		if ((status = VSsetinterlace(vdata_id, FULL_INTERLACE)) != SUCCEED ) {
			fprintf(stderr, "Error: Unable to set FULL_INTERLACE mode, status %d\n", (int)status);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
		}
		if ((recsize = H5Tget_size(type2)) <= 0 ) {
			fprintf(stderr, "Error: Unable to get record size %d\n", (int)recsize);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
		}
/*
		Since the space is rank 1, n_records does not depend on maxdims.
*/
		n_records = n_values;
    		if ((buffer = HDmalloc(n_records*recsize)) == NULL) {
			fprintf(stderr, "Error: Problems with HDmalloc of memory space\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			status = FAIL;
			break;
		}
        	if ((status = H5Dread(did, type2, space, space, H5P_DEFAULT, buffer)) != SUCCEED) {
			fprintf(stderr, "Error: Problems with H5Dread\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			status = FAIL;
			break;
		}
    		if ((status = H5Tclose(type2)) < 0 ) {
        		fprintf(stderr,"Error: closing type\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			status = FAIL;
			break;
    		}

		if ((record_pos = VSseek(vdata_id, 0)) != 0 ) {
			fprintf(stderr, "Error: Could not seek the beginning of the Vdata, %d\n", (int)record_pos);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			status = record_pos;
			break;
		}
		if ((num_of_recs = VSwrite(vdata_id, (void *)buffer, n_records, FULL_INTERLACE)) != n_records ) {
			fprintf(stderr, "Error: Only able to write %d of %d records\n", (int)num_of_recs, (int)n_records);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			status = num_of_recs;
			break;
		}

		/* there are only vdata attributes, no field attributes */
    		if ((status = H5Aiterate(did, NULL, (H5A_operator_t)convert_attr, op_data)) < 0 ) {
        		fprintf(stderr,"Error: iterate over attributes\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
    		}
		if ((status = VSdetach(vdata_id)) != SUCCEED ) {
			fprintf(stderr, "Error: Unable to detach to vdata %s.\n",name);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
			break;
		}

	} else {

        	fprintf(stderr,"Warning: HDF5 datasets of H5T_COMPOUND type with ndims > 1 are not converted.\n");

	}
	break;
    default:
        fprintf(stderr,"Error: %d class not found\n",class);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
    }

    if ((status = H5Tclose(type)) < 0 ) {
        fprintf(stderr,"Error: closing type\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
    }

    if ((status = H5Sclose(space)) < 0 ) {
        fprintf(stderr,"Error: closing space\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
    }

    if (buffer != NULL) {
	HDfree(buffer);
    }

    return status;
    
}


/*-------------------------------------------------------------------------
 * Function:    convert_attr
 *
 * Purpose:     dump the attribute
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        FAIL
 *
 * Programmer: Paul Harten
 *
 * Modifications: 
 *
 *-----------------------------------------------------------------------*/

herr_t
convert_attr (hid_t attr, char *attr_name, op_data_t *op_data)
{
hid_t  attr_id, type, space, mem_type, class;
size_t typesize;
char   *attr_values=NULL;
int32  status;
int32  h4_type;
int32  sds_id;
int32  vdata_id;
int32  vgroup_id;
int32  n_values;
int32  order;
hid_t fxdlenstr;
size_t lenstr;
H5T_cset_t cset;
H5T_str_t strpad;

	sds_id = op_data->sds_id;
	vdata_id = op_data->vdata_id;
	vgroup_id = op_data->vgroup_id;

	if ((attr_id = H5Aopen_name (attr, attr_name))>= 0) {

		if ((type = H5Aget_type(attr_id)) <= 0) {
			fprintf(stderr, "Error: H5Aget_type() didn't return appropriate value.\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
			status = FAIL;
			return status;
		}
		
    		if ((class = H5Tget_class(type)) < 0 ) {
       			fprintf(stderr,"Error: problem with getting class\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
			status = class;
       			return status;
    		}

    		switch (class) {
    		case H5T_INTEGER:
    		case H5T_FLOAT:


			if ((space = H5Aget_space(attr_id)) <= 0) {
				fprintf(stderr, "Error: H5Dget_space() didn't return appropriate value.\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if ((n_values = H5Sget_simple_extent_npoints(space)) <= 0) {
				fprintf(stderr, "Error: H5sget_simple_extent_npoints() didn't return correct value.\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

#ifdef NEWWAY
			if (FAIL==h5atomic_type_to_h4type(type, &mem_type, &typesize, &h4_type)){
				fprintf(stderr, "Error: Problems translating h5 type to h4 type\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
				status = FAIL;
				break;
			}
#else
			if ((h4_type = h5type_to_h4type(type)) == FAIL ) {
				fprintf(stderr, "Error: Problems translating h5 type to h4 type\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if ((mem_type = h4type_to_memtype(h4_type)) == FAIL ) {
				fprintf(stderr, "Error: Problems translating h4 type to mem type\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if ((typesize = H5Tget_size(mem_type)) <= 0) {
				fprintf(stderr, "Error: H5Tget_size() didn't return appropriate value.\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}
#endif

			if ((attr_values = HDmalloc(n_values*typesize)) == NULL) {
				fprintf(stderr, "Error: Problems with HDmalloc of memory space\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if ((status = H5Aread(attr_id, mem_type, attr_values)) != SUCCEED) {
				fprintf(stderr, "Error: Problems with H5Aread\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}
		
			if (sds_id != 0) {
				if ((status = SDsetattr(sds_id, attr_name, h4_type, n_values, attr_values)) != SUCCEED ) {
					fprintf(stderr, "Error: Unable to set %s attribute.\n",attr_name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
					status = FAIL;
					return status;
				}
			} else if (vdata_id != 0) {
				if ((status = VSsetattr(vdata_id, -1, attr_name, h4_type, n_values, attr_values)) != SUCCEED ) {
					fprintf(stderr, "Error: Unable to set %s attribute.\n",attr_name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
					status = FAIL;
					return status;
				}
			} else {
				if ((status = Vsetattr(vgroup_id, attr_name, h4_type, n_values, attr_values)) != SUCCEED ) {
					fprintf(stderr, "Error: Unable to set %s attribute.\n",attr_name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
					status = FAIL;
					return status;
				}
			}

			if ((status = H5Sclose(space)) != SUCCEED ) {
				fprintf(stderr, "Error: Problems closing H5Sclose\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}
			if ((status = H5Aclose(attr_id)) != SUCCEED ) {
				fprintf(stderr, "Error: Problems closing H5Aclose\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if (attr_values != NULL) {
				HDfree(attr_values);
			}

			status = SUCCEED;
        		break;

    		case H5T_TIME:
        		fprintf(stderr,"Warning: H5T_TIME attribute not yet implemented.\n");
        		break;
    		case H5T_STRING:

			fxdlenstr = type;
			h4_type = DFNT_CHAR;

			if ((space = H5Aget_space(attr_id)) <= 0) {
				fprintf(stderr, "Error: H5Dget_space() didn't return appropriate value.\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if ((n_values = H5Sget_simple_extent_npoints(space)) <= 0) {
				fprintf(stderr, "Error: H5sget_simple_extent_npoints() didn't return correct value.\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if ((mem_type = H5Tcopy(H5T_C_S1)) == FAIL ) {
				fprintf(stderr, "Error: Problems translating h4 type to mem type\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = mem_type;
				return status;
			}
			if ((lenstr = H5Tget_size(fxdlenstr)) <= 0 ) {
				fprintf(stderr, "Error: size of fixed length string type should not be %d\n",(int)lenstr);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
       				return lenstr;
			}
			if ((status = H5Tset_size(mem_type,lenstr)) != SUCCEED ) {
				fprintf(stderr, "Error: Problem with H5Tset_size()\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				return status;
			}

			strpad = H5Tget_strpad(fxdlenstr);
			if ((strpad != H5T_STR_NULLTERM) && (strpad != H5T_STR_NULLPAD) && (strpad != H5T_STR_SPACEPAD)) {
				fprintf(stderr, "Error: Invalid string padding value, %d\n",(int)strpad);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				return strpad;
			}
			if ((status = H5Tset_strpad(mem_type, H5T_STR_SPACEPAD)) != SUCCEED ) {
				fprintf(stderr, "Error: Problem with H5Tset_strpad()\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				return status;
			}

			if ((cset = H5Tget_cset(fxdlenstr)) != H5T_CSET_ASCII ) {
				fprintf(stderr, "Error: cset value != %d\n",(int)H5T_CSET_ASCII);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				return cset;
			}
			if ((status = H5Tset_cset(mem_type,cset)) != SUCCEED ) {
				fprintf(stderr, "Error: Problem with H5Tset_cset()\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				return status;
			}

			order = n_values * lenstr;
			if ((attr_values = HDmalloc(order)) == NULL) {
				fprintf(stderr, "Error: Problems with HDmalloc of memory space\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if ((status = H5Aread(attr_id, mem_type, attr_values)) != SUCCEED) {
				fprintf(stderr, "Error: Problems with H5Aread\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}
		
			if (sds_id != 0) {
				if ((status = SDsetattr(sds_id, attr_name, h4_type, order, attr_values)) != SUCCEED ) {
					fprintf(stderr, "Error: Unable to set %s attribute.\n",attr_name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
					status = FAIL;
					return status;
				}
			} else if (vdata_id != 0) {
				if ((status = VSsetattr(vdata_id, -1, attr_name, h4_type, order, attr_values)) != SUCCEED ) {
					fprintf(stderr, "Error: Unable to set %s attribute.\n",attr_name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
					status = FAIL;
					return status;
				}
			} else {
				if ((status = Vsetattr(vgroup_id, attr_name, h4_type, order, attr_values)) != SUCCEED ) {
					fprintf(stderr, "Error: Unable to set %s attribute.\n",attr_name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
					status = FAIL;
					return status;
				}
			}

			if ((status = H5Sclose(space)) != SUCCEED ) {
				fprintf(stderr, "Error: Problems closing H5Sclose\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}
			if ((status = H5Aclose(attr_id)) != SUCCEED ) {
				fprintf(stderr, "Error: Problems closing H5Aclose\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
				status = FAIL;
				return status;
			}

			if (attr_values != NULL) {
				HDfree(attr_values);
			}

			status = SUCCEED;
        		break;
    		case H5T_BITFIELD:
        		fprintf(stderr,"Warning: H5T_BITFIELD attribute not yet implemented.\n");
        		break;
    		case H5T_OPAQUE:
        		fprintf(stderr,"Warning: H5T_OPAQUE attribute not yet implemented.\n");
        		break;
    		case H5T_COMPOUND:
        		fprintf(stderr,"Warning: H5T_COMPOUND attribute not implemented.\n");
        		break;
    		default:
        		fprintf(stderr,"Error: %d class not found\n",class);
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
			status = FAIL;
    		}

		if ((status = H5Tclose(type)) != SUCCEED ) {
			fprintf(stderr, "Error: Problems closing H5Tclose\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_attr", __FILE__, __LINE__);
			status = FAIL;
			return status;
		}

	} else {

		status = FAIL;

	}

	return status;

}


/*-------------------------------------------------------------------------
 * Function:    convert_all
 *
 * Purpose:     Dump everything in the specified object
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        FAIL
 *
 * Programmer:  Paul Harten
 *
 * Modifications: 
 *
 *-----------------------------------------------------------------------*/
herr_t
convert_all (hid_t group, char *name, op_data_t *op_data)
{
    hid_t obj;
    H5G_stat_t statbuf, statbuf2;
    int status;
    op_data_t op_data_save;
    int32 vgroup_id;
    int32 sd_id;
    int32 sds_id;
    int idx, flag;
    void *edata;
    hid_t (*func)(void*);

    op_data_save = *op_data;

    vgroup_id = op_data->vgroup_id;
    sd_id = op_data->sd_id;
    sds_id = op_data->sds_id;

    if ((status = H5Gget_objinfo(group, name, FALSE, &statbuf)) != SUCCEED ) {
	fprintf(stderr,"Error: H5Gget_objinfo() did not work\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
	return (status);
    }
    statbuf2 = statbuf;

    switch (statbuf.type) {

    case H5G_LINK: /* this is a soft link only */


	/* Disable error reporting */
	H5Eget_auto (&func, &edata);
	H5Eset_auto (NULL, NULL);

	/* test to see if object exists */
    	if ((status = H5Gget_objinfo(group, name, TRUE, NULL)) != SUCCEED ) {
		fprintf(stderr,"Warning: the object pointed to by the symbolic link \"%s\" does not exist.\n",name);
    	}

	/* Enable error reporting */
	H5Eset_auto (func, edata);

	if (status != SUCCEED) {
		break;
	}

	/* follow link for type */
    	if ((status = H5Gget_objinfo(group, name, TRUE, &statbuf)) != SUCCEED ) {
		fprintf(stderr,"Error: H5Gget_objinfo() did not work\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
		return (status);
    	}

	if (statbuf.type==H5G_DATASET ) {
			
		if ((idx = get_table_idx(dset_table, statbuf.objno)) < 0 ) {

			fprintf(stderr,"Error: object not found\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
			status = FAIL;

		} else if((flag = get_tableflag(dset_table,idx)) < 0 ) {
			
			fprintf(stderr,"Error: get_tableflag() should never return < 0\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
			status = FAIL;

		} else if(flag == TRUE ) { /* this has already been converted, add as a tag/ref */

			if ((obj = H5Dopen (group, name)) <= 0 ) {
				fprintf(stderr,"Error: Unable to open H5 dataset\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				return (obj);
			} else {
				if ((status = convert_shared_dataset(obj, idx, op_data)) != SUCCEED ) {
					fprintf(stderr,"Error: Unable to convert to tag/ref\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					status = FAIL;
				}
			}
			if ((status = H5Dclose(obj)) != SUCCEED) {
				fprintf(stderr,"Error: Unable to close H5 dataset %s\n",name);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				status = FAIL;
				break;
			}

		} else { /* flag == FALSE */

			if ((obj = H5Dopen (group, name)) <= 0 ) {
				fprintf(stderr,"Error: Unable to open H5 dataset\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				return (obj);
			} else {
				if (( status = convert_dataset (obj, name, op_data)) != SUCCEED) {
					fprintf(stderr,"Error: convert_dataset did not work for %s\n",name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					status = FAIL;
					break;
				}
				if ((status = convert_shared_dataset(obj, idx, op_data)) != SUCCEED ) {
					fprintf(stderr,"Error: Unable to convert to tag/ref\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					status = FAIL;
				}
				if(( status = set_tableflag(dset_table,idx)) != SUCCEED ) {
					fprintf(stderr,"Error: set_tableflag() did not work for %s\n", name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					break;
				}
			}
			if ((status = H5Dclose(obj)) != SUCCEED) {
				fprintf(stderr,"Error: Unable to close H5 dataset %s\n",name);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				status = FAIL;
				break;
			}

		}

	} else if (statbuf.type==H5G_GROUP ) {

		if ((idx = get_table_idx(group_table, statbuf.objno)) < 0 ) {

			fprintf(stderr,"Error: object not found\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
			status = FAIL;

		} else if((flag = get_tableflag(group_table,idx)) < 0 ) {
			
			fprintf(stderr,"Error: get_tableflag() should never return < 0\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
			status = FAIL;

		} else if(flag == TRUE ) {

			if (( status = convert_shared_group (group, idx, op_data)) != SUCCEED) {
				fprintf(stderr,"Error: convert_group did not work for %s\n",name);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				status = FAIL;
				break;
			}

		} else { /* flag == FALSE */

			if ((obj = H5Gopen (group, name)) <= 0 ) {
				fprintf(stderr,"Error: Unable to open group\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				return (obj);
			} else {
				if (( status = convert_group (obj, name, op_data)) != SUCCEED) {
					fprintf(stderr,"Error: convert_group did not work for %s\n",name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					status = FAIL;
					break;
				}
			}
			if ((status = H5Gclose(obj)) != SUCCEED) {
				fprintf(stderr,"Error: Unable to close group %s\n",name);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				status = FAIL;
				break;
			}

		}

	}

        break;

    case H5G_GROUP:

		if ((idx = get_table_idx(group_table, statbuf.objno)) < 0 ) {

			fprintf(stderr,"Error: object not found\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
			status = FAIL;

		} else if((flag = get_tableflag(group_table,idx)) < 0 ) {
			
			fprintf(stderr,"Error: get_tableflag() should never return < 0\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
			status = FAIL;

		} else if(flag == TRUE ) {

			if (( status = convert_shared_group (group, idx, op_data)) != SUCCEED) {
				fprintf(stderr,"Error: convert_group did not work for %s\n",name);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				status = FAIL;
				break;
			}

		} else { /* flag == FALSE */

			if ((obj = H5Gopen (group, name)) <= 0 ) {
				fprintf(stderr,"Error: Unable to open group\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				return (obj);
			} else {
				if (( status = convert_group (obj, name, op_data)) != SUCCEED) {
					fprintf(stderr,"Error: convert_group did not work for %s\n",name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					status = FAIL;
					break;
				}
			}
			if ((status = H5Gclose(obj)) != SUCCEED) {
				fprintf(stderr,"Error: Unable to close group %s\n",name);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				status = FAIL;
				break;
			}

		}

	break;

    case H5G_DATASET:

		if ((idx = get_table_idx(dset_table, statbuf.objno)) < 0 ) {

			fprintf(stderr,"Error: object not found\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
			status = FAIL;

		} else if((flag = get_tableflag(dset_table,idx)) < 0 ) {
			
			fprintf(stderr,"Error: get_tableflag() should never return < 0\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
			status = FAIL;

		} else if(flag == TRUE ) { /* this has already been converted, add as a tag/ref */

			if ((obj = H5Dopen (group, name)) <= 0 ) {
				fprintf(stderr,"Error: Unable to open H5 dataset\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				return (obj);
			} else {
				if ((status = convert_shared_dataset(obj, idx, op_data)) != SUCCEED ) {
					fprintf(stderr,"Error: Unable to convert to tag/ref\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					status = FAIL;
				}
			}
			if ((status = H5Dclose(obj)) != SUCCEED) {
				fprintf(stderr,"Error: Unable to close H5 dataset %s\n",name);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				status = FAIL;
				break;
			}

		} else { /* flag == FALSE */

			if ((obj = H5Dopen (group, name)) <= 0 ) {
				fprintf(stderr,"Error: Unable to open H5 dataset\n");
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				return (obj);
			} else {
				if (( status = convert_dataset (obj, name, op_data)) != SUCCEED) {
					fprintf(stderr,"Error: convert_dataset did not work for %s\n",name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					status = FAIL;
					break;
				}
				if ((status = convert_shared_dataset(obj, idx, op_data)) != SUCCEED ) {
					fprintf(stderr,"Error: Unable to convert to tag/ref\n");
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					status = FAIL;
				}
				if(( status = set_tableflag(dset_table,idx)) != SUCCEED ) {
					fprintf(stderr,"Error: set_tableflag() did not work for %s\n", name);
					DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
					break;
				}
			}
			if ((status = H5Dclose(obj)) != SUCCEED) {
				fprintf(stderr,"Error: Unable to close H5 dataset %s\n",name);
				DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_all", __FILE__, __LINE__);
				status = FAIL;
				break;
			}

		}

	break;

    case H5G_TYPE:
	/* object is ignored */
        break;

    default:
        fprintf (stderr,"Unknown Object %s\n", name);
        status = FAIL; 
        break;

    }

    *op_data = op_data_save;

    return SUCCEED;

}


/*-------------------------------------------------------------------------
 * Function:    convert_shared_dataset
 *
 * Purpose:     Handle a shared dataset which has already been converted.
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        FAIL
 *
 * Programmer:  Paul Harten
 *
 * Modifications: 
 *
 *-----------------------------------------------------------------------*/
herr_t
convert_shared_dataset(hid_t did, int idx, op_data_t *op_data)
{
    int status=SUCCEED;
    int32 vgroup_id;
    char *dataset_name=NULL;
    char *dataset_name2=NULL;
    int32 hfile_id;
    int32 sd_id;
    int32 sds_id;
    int32 sds_ref;
    int32 vdata_ref;
    int32 sds_index;
    int32 numtagref;
    hid_t  type, space, class;
    hsize_t dims[32], maxdims[32];
    int n_values, ndims;

    vgroup_id = op_data->vgroup_id;

    if ((dataset_name = get_objectname(dset_table, idx)) == NULL ) {
	fprintf(stderr,"Error: get_objectname() did not work\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
	return (status);
    }

    if ((dataset_name2 = strrchr(dataset_name,'/')) == NULL) {	/* find last "/" in dataset_name */
	dataset_name2 = dataset_name;				/* no "/"s were found */
    } else {
	dataset_name2 = dataset_name2 + sizeof(char);		/* 1 character past last "/" */
    }

    if ((type = H5Dget_type(did)) <= 0) {
	fprintf(stderr, "Error: H5Dget_type() didn't return appropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
	status = FAIL;
       	return status;
    }

    if ((space = H5Dget_space(did)) <= 0) {
	fprintf(stderr, "Error: H5Dget_space() didn't return appropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
	status = FAIL;
       	return status;
    }

    if ((n_values = H5Sget_simple_extent_npoints(space)) <= 0) {
	fprintf(stderr, "Error: H5Sget_simple_extent_npoints() returned inappropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
	status = FAIL;
       	return status;
    }

    if ((ndims = H5Sget_simple_extent_dims(space,dims,maxdims)) < 0 ) {
	fprintf(stderr, "Error: Problems getting ndims, dims, and maxdims of dataset\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
	status = ndims;
       	return status;
    }

    if ((class = H5Tget_class(type)) < 0 ) {
       	fprintf(stderr,"Error: problem with getting class\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
	status = class;
       	return status;
    }

    switch (class) {
    case H5T_INTEGER:
    case H5T_FLOAT:
	sd_id = op_data->sd_id;
	if ((sds_index = SDnametoindex(sd_id, dataset_name2)) < 0 ) {
       		fprintf (stderr,"Error: Problem with SDnametoindex().\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
		return (sds_index);
	}
	if ((sds_id = SDselect(sd_id, sds_index)) < 0 ) {
       		fprintf (stderr,"Error: Problem with SDselect().\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
		return (sds_id);
       	}
	if ((sds_ref = SDidtoref(sds_id)) < 0 ) {
       		fprintf (stderr,"Error: Problem with SDidtoref().\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
		return (sds_ref);
       	}
	if ((numtagref = Vaddtagref(vgroup_id, DFTAG_NDG, sds_ref)) < 0 ) {
       		fprintf (stderr,"Error: Problem with Vaddtagref().\n");
		DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
		return (numtagref);
       	}
	break;
    case H5T_TIME:
        fprintf(stderr,"Error: H5T_TIME not yet implemented.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
        break;
    case H5T_STRING:
	hfile_id = op_data->hfile_id;
        if (ndims==1) {
		if ((vdata_ref = VSfind(hfile_id,dataset_name2)) <= 0 ) {
       			fprintf (stderr,"Error: Problem with VSfind().\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
			return (vdata_ref);
       		}
		if ((numtagref = Vaddtagref(vgroup_id, DFTAG_VH, vdata_ref)) < 0 ) {
       			fprintf (stderr,"Error: Problem with Vaddtagref().\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
			return (numtagref);
       		}
	}
        break;
    case H5T_BITFIELD:
        fprintf(stderr,"Error: H5T_BITFIELD not yet implemented.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
        break;
    case H5T_OPAQUE:
        fprintf(stderr,"Error: H5T_OPAQUE not yet implemented.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
        break;
    case H5T_COMPOUND:
	hfile_id = op_data->hfile_id;
        if (ndims==1) {
		if ((vdata_ref = VSfind(hfile_id,dataset_name2)) <= 0 ) {
       			fprintf (stderr,"Error: Problem with VSfind().\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
			return (vdata_ref);
       		}
		if ((numtagref = Vaddtagref(vgroup_id, DFTAG_VH, vdata_ref)) < 0 ) {
       			fprintf (stderr,"Error: Problem with Vaddtagref().\n");
			DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
			return (numtagref);
       		}
       	}
        break;
    default:
        fprintf(stderr,"Error: %d class not found\n",class);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
	status = FAIL;
    }

    if ((status = H5Tclose(type)) < 0 ) {
   	fprintf(stderr,"Error: closing type\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset", __FILE__, __LINE__);
	status = FAIL;
    }

    if (dataset_name != NULL) {
	HDfree (dataset_name);
    }

    return status;

}


/*-------------------------------------------------------------------------
 * Function:    convert_shared_group
 *
 * Purpose:     Handle a shared group which has already been converted.
 *
 * Return:      status
 *
 * Programmer:  Paul Harten
 *
 * Modifications: 
 *
 *-----------------------------------------------------------------------*/
herr_t
convert_shared_group (hid_t group, int idx, op_data_t *op_data) {

    int32 hfile_id;
    int32 vgroup_id;
    int32 vgroup_ref;
    int32 numtagref;
    int32 status = SUCCEED;
    hid_t group2;
    char *group_name=NULL;
    char *group_name2=NULL;
    char vgroup_name[VGNAMELENMAX];

    group2 = group;
    hfile_id = op_data->hfile_id;

    if ((group_name = get_objectname(group_table, idx)) == NULL ) {
	fprintf(stderr,"Error: get_objectname() did not work\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_group", __FILE__, __LINE__);
        status = FAIL;
	return (status);
    }

    if ((group_name2 = strrchr(group_name,'/')) == NULL) {	/* find last "/" in group_name */
	group_name2 = group_name;				/* no "/"s were found */
    } else {
	group_name2 = group_name2 + sizeof(char);		/* 1 character past last "/" */
    }

    if ((status = Vgetname(op_data->vgroup_id,vgroup_name)) < 0 ) {
    	fprintf (stderr,"Error: Problem with Vfind().\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_group", __FILE__, __LINE__);
	return (status);
    }

    if ((vgroup_ref = Vfind(hfile_id,vgroup_name)) <= 0 ) {
    	fprintf (stderr,"Error: Problem with Vfind().\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_group", __FILE__, __LINE__);
	return (vgroup_ref);
    }

    if ((vgroup_id = Vattach(hfile_id, vgroup_ref, "w")) <= 0 ) {
	fprintf(stderr,"Error: Unable to create new vgroup\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_group", __FILE__, __LINE__);
	return(vgroup_id);
    }

    if ((vgroup_ref = Vfind(hfile_id,group_name2)) <= 0 ) {
    	fprintf (stderr,"Error: Problem with Vfind().\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_group", __FILE__, __LINE__);
	return (vgroup_ref);
    }

    if ((numtagref = Vaddtagref(vgroup_id, DFTAG_VG, vgroup_ref)) < 0 ) {
   	fprintf (stderr,"Error: Problem with Vaddtagref().\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_group", __FILE__, __LINE__);
	return (numtagref);
    }

    if ((status = Vdetach(vgroup_id)) != SUCCEED ) {
	fprintf(stderr,"Error: Unable to detach the new Vgroup\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_group", __FILE__, __LINE__);
	status = FAIL;
    }

    if (group_name != NULL) {
	HDfree(group_name);
    }

    return status;

}


/*-------------------------------------------------------------------------
 * Function:    convert_dataset_string
 *
 * Purpose:     convert a dataset with string elements.
 *
 * Return:      status
 *
 * Programmer:  Paul Harten
 *
 * Modifications: 
 *
 *-----------------------------------------------------------------------*/
herr_t
convert_dataset_string (hid_t did, char *name, op_data_t *op_data) {

    int32 hfile_id;
    int32 vgroup_id;
    int32 vdata_id;
hid_t fxdlenstr, space, class, mem_type;
const char* fieldname = {"string"};
const char* fieldname_list = fieldname;
char *buffer;
int32 status;
int32 h4_type;
int32 recsize, n_records, n_values, num_of_recs, record_pos;
size_t lenstr;
H5T_cset_t cset;
H5T_str_t strpad;
	
    if ((fxdlenstr = H5Dget_type(did)) <= 0) {
	fprintf(stderr, "Error: H5Dget_type() didn't return appropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = FAIL;
        return status;
    }

    if ((class = H5Tget_class(fxdlenstr)) != H5T_STRING ) {
       	fprintf(stderr,"Error: problem with getting class\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_shared_dataset", __FILE__, __LINE__);
	status = class;
       	return status;
    }

    if ((space = H5Dget_space(did)) <= 0) {
	fprintf(stderr, "Error: H5Dget_space() didn't return appropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = FAIL;
       	return status;
    }

    if ((n_values = H5Sget_simple_extent_npoints(space)) <= 0) {
	fprintf(stderr, "Error: H5Sget_simple_extent_npoints() returned inappropriate value.\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = FAIL;
       	return status;
    }

    h4_type = DFNT_CHAR;
    if ((mem_type = H5Tcopy(H5T_C_S1)) == FAIL ) {
	fprintf(stderr, "Error: Problems translating h4 type to mem type\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = mem_type;
       	return status;
    }
    if ((lenstr = H5Tget_size(fxdlenstr)) <= 0 ) {
	fprintf(stderr, "Error: size of fixed length string type should not be %d\n",(int)lenstr);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
       	return lenstr;
    }
    if ((status = H5Tset_size(mem_type,lenstr)) != SUCCEED ) {
	fprintf(stderr, "Error: Problem with H5Tset_size()\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
       	return status;
    }

    strpad = H5Tget_strpad(fxdlenstr);
    if ((strpad != H5T_STR_NULLTERM) && (strpad != H5T_STR_NULLPAD) && (strpad != H5T_STR_SPACEPAD)) {
	fprintf(stderr, "Error: Invalid string padding value, %d\n",(int)strpad);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
       	return strpad;
    }
    if ((status = H5Tset_strpad(mem_type, strpad)) != SUCCEED ) {
	fprintf(stderr, "Error: Problem with H5Tset_strpad()\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
       	return status;
    }

    if ((cset = H5Tget_cset(fxdlenstr)) != H5T_CSET_ASCII ) {
	fprintf(stderr, "Error: cset value != %d\n",(int)H5T_CSET_ASCII);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
       	return cset;
    }
    if ((status = H5Tset_cset(mem_type,cset)) != SUCCEED ) {
	fprintf(stderr, "Error: Problem with H5Tset_cset()\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
       	return status;
    }

    hfile_id = op_data->hfile_id;
    vgroup_id = op_data->vgroup_id;
    if ((vdata_id = VSattach(hfile_id, -1, "w")) <= 0 ) {
	fprintf(stderr, "Error: Unable to create vdata %s.\n",name);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = FAIL;
	return status;
    }

    op_data->vdata_id = vdata_id;
    if ((status = VSsetname(vdata_id, name)) != SUCCEED ) {
	fprintf(stderr, "Error: Unable to set vdata name %s.\n",name);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	return status;
    }
    if ((status = VSsetclass(vdata_id, "HDF5")) != SUCCEED ) {
	fprintf(stderr, "Error: Unable to set class on vdata %s\n", name);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	return status;
    }

    if ((status = VSfdefine(vdata_id, fieldname, h4_type, lenstr)) != SUCCEED ) {
	fprintf(stderr, "Error: Unable to VSfdefine() field\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	return status;
    }

    if ((status = VSsetfields(vdata_id, fieldname_list)) != SUCCEED ) {
	fprintf(stderr, "Error: Unable to set fieldname list  %s\n", fieldname_list);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	return status;
    }
    if ((status = VSsetinterlace(vdata_id, FULL_INTERLACE)) != SUCCEED ) {
	fprintf(stderr, "Error: Unable to set FULL_INTERLACE mode, status %d\n", (int)status);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	return status;
    }
    if ((recsize = H5Tget_size(mem_type)) <= 0 ) {
	fprintf(stderr, "Error: Unable to get record size %d\n", (int)recsize);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = recsize;
	return status;
    }
/*
    Since the space is rank 1, n_records does not depend on maxdims.
*/
    n_records = n_values;
    if ((buffer = HDmalloc(n_records*recsize)) == NULL) {
	fprintf(stderr, "Error: Problems with HDmalloc of memory space\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = FAIL;
	return status;
    }
    if ((status = H5Dread(did, mem_type, space, space, H5P_DEFAULT, buffer)) != SUCCEED) {
	fprintf(stderr, "Error: Problems with H5Dread\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	return status;
    }

    if ((record_pos = VSseek(vdata_id, 0)) != 0 ) {
	fprintf(stderr, "Error: Could not seek the beginning of the Vdata, %d\n", (int)record_pos);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = record_pos;
	return status;
    }
    if ((num_of_recs = VSwrite(vdata_id, (void *)buffer, n_records, FULL_INTERLACE)) != n_records ) {
	fprintf(stderr, "Error: Only able to write %d of %d records\n", (int)num_of_recs, (int)n_records);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	status = num_of_recs;
	return status;
    }

    /* there are only vdata attributes, no field attributes */
    if ((status = H5Aiterate(did, NULL, (H5A_operator_t)convert_attr, op_data)) < 0 ) {
	fprintf(stderr,"Error: iterate over attributes\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	return status;
    }
    if ((status = VSdetach(vdata_id)) != SUCCEED ) {
	fprintf(stderr, "Error: Unable to detach to vdata %s.\n",name);
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "convert_dataset_string", __FILE__, __LINE__);
	return status;
    }

    if (buffer != NULL) {
	HDfree(buffer);
    }

    return status;

}


/*-------------------------------------------------------------------------
 * Function:    h5atomic_type_to_h4type
 *
 * Purpose:     Match an H5 atomic type to an appropriate H4 type.
 *              Assign an appropirate H5 memory type that matches the H4 type.
 *              Return the H5 memory type, H4 type and sizeof H4 via pointers.
 *
 * Return:      SUCCEED on suceed, FAIL on failure.
 *              When fail, pointer values of h5memtype, h5memsize and h4_type
 *              may have changed and are undefined.
 *
 * Programmer:  Albert Cheng, March 2000
 *
 * Modifications:
 *
 *-----------------------------------------------------------------------*/
static herr_t h5atomic_type_to_h4type(const hid_t h5type, hid_t* h5memtype, size_t* h5memsize, int32* h4type)
{
    H5T_class_t class;
    size_t h5typesize, h4typesize;
    H5T_sign_t sign;
    hid_t mem_datatype = FAIL;

    if ((class = H5Tget_class(h5type)) < 0 ) {
        fprintf(stderr,"Error: problem with getting type class\n");
	DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5atomic_type_to_h4type", __FILE__, __LINE__);
        return FAIL;
    }
    
    switch(class){
    case H5T_INTEGER:
	if ((h5typesize = H5Tget_size(h5type)) == 0 ) {
	    fprintf(stderr,"Error: problem with getting type size\n");
	    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5atomic_type_to_h4type", __FILE__, __LINE__);
	    return FAIL;
	}
	if ((sign = H5Tget_sign(h5type)) == H5T_SGN_ERROR) {
	    fprintf(stderr,"Error: problem with getting type sign\n");
	    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5atomic_type_to_h4type", __FILE__, __LINE__);
	    return FAIL;
	}
	/* deduce the proper HDF4 integer type to use according to the size of the HDF5 type. */
	/* Current HDF4 types can be 8, 16, 32 bits (1, 2, 4 bytes). */
	switch(h5typesize){
	case 1:
	    *h4type = (sign == H5T_SGN_2 ? DFNT_INT8 : DFNT_UINT8);
	    h4typesize = sizeof(int8);
	    if (h4typesize == H5Tget_size(H5T_NATIVE_CHAR)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_SCHAR : H5T_NATIVE_UCHAR);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_SHORT)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_SHORT : H5T_NATIVE_USHORT);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_INT)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_INT : H5T_NATIVE_UINT);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_LONG)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_LONG : H5T_NATIVE_ULONG);
	    }else
		return(FAIL);
	    break;
	case 2:
	    *h4type = (sign == H5T_SGN_2 ? DFNT_INT16 : DFNT_UINT16);
	    h4typesize = sizeof(int16);
	    if (h4typesize == H5Tget_size(H5T_NATIVE_CHAR)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_SCHAR : H5T_NATIVE_UCHAR);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_SHORT)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_SHORT : H5T_NATIVE_USHORT);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_INT)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_INT : H5T_NATIVE_UINT);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_LONG)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_LONG : H5T_NATIVE_ULONG);
	    }else
		return(FAIL);
	    break;
	case 4:
	    *h4type = (sign == H5T_SGN_2 ? DFNT_INT32 : DFNT_UINT32);
	    h4typesize = sizeof(int32);
	    if (h4typesize == H5Tget_size(H5T_NATIVE_CHAR)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_SCHAR : H5T_NATIVE_UCHAR);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_SHORT)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_SHORT : H5T_NATIVE_USHORT);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_INT)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_INT : H5T_NATIVE_UINT);
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_LONG)){
		mem_datatype = (sign == H5T_SGN_2 ? H5T_NATIVE_LONG : H5T_NATIVE_ULONG);
	    }else
		return(FAIL);
	    break;
	default:
	    fprintf(stderr,"Error: unmatchable integer type\n");
	    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5atomic_type_to_h4type", __FILE__, __LINE__);
	    return FAIL;
	}
	break;
	/* end of case H5T_INTEGER */

    case H5T_FLOAT:
	if ((h5typesize = H5Tget_size(h5type)) == 0 ) {
	    fprintf(stderr,"Error: problem with getting type size\n");
	    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5atomic_type_to_h4type", __FILE__, __LINE__);
	    return FAIL;
	}
	/* deduce the proper HDF4 floating point type to use according to the size of the HDF5 type. */
	/* Current HDF4 types can be 32 or 64 bits (4 or 8 bytes). */
	switch(h5typesize){
	case 4:
	    *h4type = DFNT_FLOAT32;
	    h4typesize = sizeof(float32);
	    if (h4typesize == H5Tget_size(H5T_NATIVE_FLOAT)){
		mem_datatype = H5T_NATIVE_FLOAT;
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_DOUBLE)){
		mem_datatype = H5T_NATIVE_DOUBLE;
	    }else
		return(FAIL);
	    break;
	case 8:
	    *h4type = DFNT_FLOAT64;
	    h4typesize = sizeof(float64);
	    if (h4typesize == H5Tget_size(H5T_NATIVE_FLOAT)){
		mem_datatype = H5T_NATIVE_FLOAT;
	    }else if (h4typesize == H5Tget_size(H5T_NATIVE_DOUBLE)){
		mem_datatype = H5T_NATIVE_DOUBLE;
	    }else
		return(FAIL);
	    break;
	default:
	    fprintf(stderr,"Error: unmatchable H5 float type\n");
	    DEBUG_PRINT("Error detected in %s() [%s line %d]\n", "h5atomic_type_to_h4type", __FILE__, __LINE__);
	    return FAIL;
	}
	break;
	/* end of case H5T_FLOAT */

    default:
	return FAIL;
    }

    *h5memsize = h4typesize;
    *h5memtype = mem_datatype;
    return(SUCCEED);
}
#ifndef NEWWAY
/*****************************************************************************

  Routine: h5type_to_h4type(h5type)
 
  Description: Translate h5 datatype into h4 datatype

  Input: h5 datatype
 
  Output: function return, h4 datatype

*****************************************************************************/

int32 h5type_to_h4type(hid_t h5_datatype)
{
    int32 h4_datatype;

	if (H5Tequal(h5_datatype,H5T_STD_I8BE)) {
 		h4_datatype = DFNT_INT8;
	} else if (H5Tequal(h5_datatype,H5T_STD_I16BE)) {
 		h4_datatype = DFNT_INT16;
	} else if (H5Tequal(h5_datatype,H5T_STD_I32BE)) {
 		h4_datatype = DFNT_INT32;
/*
 *	This is not supported by HDF4
 *
	} else if (H5Tequal(h5_datatype,H5T_STD_I64BE)) {
 		h4_datatype = DFNT_INT64;
*/
	} else if (H5Tequal(h5_datatype,H5T_STD_U8BE)) {
 		h4_datatype = DFNT_UINT8;
	} else if (H5Tequal(h5_datatype,H5T_STD_U16BE)) {
 		h4_datatype = DFNT_UINT16;
	} else if (H5Tequal(h5_datatype,H5T_STD_U32BE)) {
 		h4_datatype = DFNT_UINT32;
	} else if (H5Tequal(h5_datatype,H5T_STD_U64BE)) {
 		h4_datatype = DFNT_UINT64;
	} else if (H5Tequal(h5_datatype,H5T_IEEE_F32BE)) {
		h4_datatype = DFNT_FLOAT32;
	} else if (H5Tequal(h5_datatype,H5T_IEEE_F64BE)) {
 		h4_datatype = DFNT_FLOAT64;
	} else if (H5Tequal(h5_datatype,H5T_STD_I8LE)) {
 		h4_datatype = DFNT_INT8;
	} else if (H5Tequal(h5_datatype,H5T_STD_I16LE)) {
 		h4_datatype = DFNT_INT16;
	} else if (H5Tequal(h5_datatype,H5T_STD_I32LE)) {
 		h4_datatype = DFNT_INT32;
/*
 *	This is not supported by HDF4
 *
	} else if (H5Tequal(h5_datatype,H5T_STD_I64LE)) {
 		h4_datatype = DFNT_INT64;
*/
	} else if (H5Tequal(h5_datatype,H5T_STD_U8LE)) {
 		h4_datatype = DFNT_UINT8;
	} else if (H5Tequal(h5_datatype,H5T_STD_U16LE)) {
 		h4_datatype = DFNT_UINT16;
	} else if (H5Tequal(h5_datatype,H5T_STD_U32LE)) {
 		h4_datatype = DFNT_UINT32;
	} else if (H5Tequal(h5_datatype,H5T_STD_U64LE)) {
 		h4_datatype = DFNT_UINT64;
	} else if (H5Tequal(h5_datatype,H5T_IEEE_F32LE)) {
 		h4_datatype = DFNT_FLOAT32;
	} else if (H5Tequal(h5_datatype,H5T_IEEE_F64LE)) {
 		h4_datatype = DFNT_FLOAT64;
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_SCHAR)) {
 		h4_datatype = DFNT_INT8;
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_UCHAR)) {
 		h4_datatype = DFNT_UINT8;
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_SHORT)) {
 		h4_datatype = DFNT_INT16;
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_USHORT)) {
 		h4_datatype = DFNT_UINT16;
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_INT)) {
 		h4_datatype = DFNT_INT32;
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_UINT)) {
 		h4_datatype = DFNT_UINT32;
/*
 *	This is not supported by HDF4
 *
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_LONG)) {
 		h4_datatype = DFNT_INT64;
*/
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_ULONG)) {
 		h4_datatype = DFNT_UINT64;
/*
 *	This is not supported by HDF4
 *
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_LLONG)) {
 		h4_datatype = DFNT_INT128;
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_ULLONG)) {
 		h4_datatype = DFNT_UINT128;
*/
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_FLOAT)) {
		h4_datatype = DFNT_FLOAT32;
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_DOUBLE)) {
 		h4_datatype = DFNT_FLOAT64;
/*
 *	This is not supported by HDF4
 *
	} else if (H5Tequal(h5_datatype,H5T_NATIVE_LDOUBLE)) {
 		h4_datatype = DFNT_FLOAT128;
*/
	} else {
 		h4_datatype = FAIL;
	}

	return h4_datatype;

}


/*****************************************************************************

  Routine: h4type_to_memtype(h4type)
 
  Description: Translate h4 datatype into mem datatype

  Input: h4 datatype
 
  Output: function return, mem datatype

*****************************************************************************/

hid_t h4type_to_memtype(int32 h4_datatype)
{
    hid_t mem_datatype;

	switch (h4_datatype) {
	case DFNT_INT8:
	case DFNT_NINT8:
	case DFNT_LINT8:
		mem_datatype = H5T_NATIVE_SCHAR; break;
	case DFNT_UINT8:
	case DFNT_NUINT8:
	case DFNT_LUINT8:
		mem_datatype = H5T_NATIVE_UCHAR; break;
 	case DFNT_INT16:
 	case DFNT_NINT16:
 	case DFNT_LINT16:
 		mem_datatype = H5T_NATIVE_SHORT; break;
 	case DFNT_UINT16:
 	case DFNT_NUINT16:
 	case DFNT_LUINT16:
 		mem_datatype = H5T_NATIVE_USHORT; break;
 	case DFNT_INT32:
 	case DFNT_NINT32:
 	case DFNT_LINT32:
 		mem_datatype = H5T_NATIVE_INT; break;
 	case DFNT_UINT32:
 	case DFNT_NUINT32:
 	case DFNT_LUINT32:
 		mem_datatype = H5T_NATIVE_UINT; break;
 	case DFNT_INT64:
 	case DFNT_NINT64:
 	case DFNT_LINT64:
 		mem_datatype = H5T_NATIVE_LONG; break;
 	case DFNT_UINT64:
 	case DFNT_NUINT64:
 	case DFNT_LUINT64:
 		mem_datatype = H5T_NATIVE_ULONG; break;
 	case DFNT_INT128:
 	case DFNT_NINT128:
 	case DFNT_LINT128:
 		mem_datatype = H5T_NATIVE_LLONG; break;
 	case DFNT_UINT128:
 	case DFNT_NUINT128:
 	case DFNT_LUINT128:
 		mem_datatype = H5T_NATIVE_ULLONG; break;
 	case DFNT_FLOAT32:
 	case DFNT_NFLOAT32:
 	case DFNT_LFLOAT32:
 		mem_datatype = H5T_NATIVE_FLOAT; break;
 	case DFNT_FLOAT64:
 	case DFNT_NFLOAT64:
 	case DFNT_LFLOAT64:
 		mem_datatype = H5T_NATIVE_DOUBLE; break;
 	case DFNT_FLOAT128:
 	case DFNT_NFLOAT128:
 	case DFNT_LFLOAT128:
 		mem_datatype = H5T_NATIVE_LDOUBLE; break;
	default:
 		mem_datatype = FAIL;
	}

	return mem_datatype;

}
#endif


/*****************************************************************************

  Routine: test_file
 
  Description: Test a file for read/write - ability.
 
  Input: filename	- Unix filename
 
  Output: function return,  global variable - errno

*****************************************************************************/

int test_file(char *filename,int oflag,mode_t mode)
{
	int	fid;

	errno = 0;

	fid = open(filename, oflag, mode);
	if (fid < 0) {
		perror(filename);
	}
	close(fid);

	return errno;

}


/*****************************************************************************

  Routine: test_dir
 
  Description: Test pathway to determine if it is a directory
 
  Input: path	- pathname given
 
  Output:  function return TRUE/FALSE

*****************************************************************************/

int test_dir(char *path)
{

	struct stat buf;
	struct stat *buf_ptr;
	int idir;

	buf_ptr = &buf;

	idir = stat(path, buf_ptr);
	if (idir < 0) {
		if (errno == 2) {
			return 0;
		} else {
			perror(path);
		}
	}

	return S_ISDIR(buf_ptr->st_mode);
}

/*****************************************************************************

  Routine: BuildFilename()
 
  Description: Build a filename with new extension
 
  Input: filename	- present filename
		 ext		- extension to root of filename
 
  Output: (filename:r).ext

*****************************************************************************/

char *BuildFilename(char *filename, char *ext)
{
 	/* build outgoing filename */

	char *filename_out;
	char *lastper_ptr, *lastdir_ptr;
	int root_len;

	lastper_ptr = strrchr(filename,'.');
	lastdir_ptr = strrchr(filename,'/');

	if ( lastper_ptr <= lastdir_ptr ) { /* no extension */
		root_len = strlen(filename);
	} else {	/* existing extension */
		root_len = (int)(lastper_ptr - filename); 
	}

	filename_out = (char *)HDmalloc(root_len + strlen(ext) + 2);
	filename_out = strncpy(filename_out, filename, (size_t)root_len);
	filename_out[root_len] = '\0';
	filename_out = strcat(filename_out,".");
	filename_out = strcat(filename_out,ext);

	return filename_out;
}

