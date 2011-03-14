/********************************************************************
 *
 * Testing thread safety. Deliberate per-thread errors to test error stack
 * -----------------------------------------------------------------------
 *
 * Create 16 multiple threads to create datasets with the same name. The
 * library should respond with 15 equivalent error stack printouts (one for
 * each bad thread). The final hdf5 file should be a valid file with one
 * entry.
 *
 * Temporary files generated:
 *   ttsafe_error.h5
 *
 * HDF5 APIs exercised in thread:
 * H5Screate_simple, H5Tcopy, H5Tset_order, H5Dcreate, H5Dclose,
 * H5Tclose, H5Sclose.
 *
 * Created: Apr 28 2000
 * Programmer: Chee Wai LEE
 *
 * Modification History
 * --------------------
 *
 * 	19 May 2000, Bill Wendling
 * 	Modified so that it creates a unique HDF5 file and removes it on
 * 	cleanup. Also added the num_errs flag and increment it when necessary
 * 	to report the errors.
 *
 ********************************************************************/
#include "ttsafe.h"

#ifndef H5_HAVE_THREADSAFE
static int dummy;	/* just to create a non-empty object file */
#else

#define NUM_THREAD	16
#define FILENAME	"ttsafe_error.h5"

/* Having a common dataset name is an error */
#define DATASETNAME		"commonname"
#define EXPECTED_ERROR_DEPTH	3
#define WRITE_NUMBER		37

/* Global variables */
extern int num_errs;
extern int Verbosity;

herr_t error_callback(void *);
herr_t walk_error_callback(int, H5E_error_t *, void *);
void *tts_error_thread(void *);
hid_t error_file;

typedef struct err_num_struct {
	int maj_num;
	int min_num;
} err_num_t;

err_num_t expected[] = {
	{H5E_DATASET, H5E_CANTINIT},
	{H5E_DATASET, H5E_CANTINIT},
	{H5E_SYM, H5E_EXISTS}
};

int error_flag = 0;
int error_count = 0;
pthread_mutex_t error_mutex;

void tts_error(void)
{
	pthread_t threads[NUM_THREAD];
	pthread_attr_t attribute;
	H5E_auto_t old_error_cb;
	void *old_error_client_data;
	hid_t dataset;
	int value, i;

	/* set up mutex for global count of errors */
	pthread_mutex_init(&error_mutex, NULL);

	/* preserve previous error stack handler */
	H5Eget_auto(&old_error_cb, &old_error_client_data);

	/* set our own auto error stack handler */
	H5Eset_auto(error_callback, NULL);

	/* make thread scheduling global */
	pthread_attr_init(&attribute);
	pthread_attr_setscope(&attribute, PTHREAD_SCOPE_SYSTEM);

	/*
	 * Create a hdf5 file using H5F_ACC_TRUNC access, default file
	 * creation plist and default file access plist
	 */
	error_file = H5Fcreate(FILENAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	for (i = 0; i < NUM_THREAD; i++)
		pthread_create(&threads[i], &attribute, tts_error_thread, NULL);

	for (i = 0; i < NUM_THREAD; i++)
		pthread_join(threads[i],NULL);

	if (error_flag) {
		fprintf(stderr, "Threads reporting different error values!\n");
		num_errs++;
	}

	if (error_count != NUM_THREAD - 1) {
		fprintf(stderr, "Error: %d threads failed instead of %d\n",
			error_count, NUM_THREAD-1);
		num_errs++;
	}

	dataset = H5Dopen(error_file, DATASETNAME);
	H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &value);

	if (value != WRITE_NUMBER) {
		fprintf(stderr,
			"Error: Successful thread wrote value %d instead of %d\n",
			value, WRITE_NUMBER);
		num_errs++;
	}

	H5Dclose(dataset);
	H5Fclose(error_file);

	/* turn our error stack handler off */
	H5Eset_auto(old_error_cb, old_error_client_data);
}

void *tts_error_thread(void *arg)
{
	hid_t dataspace, datatype, dataset;
	hsize_t dimsf[1]; /* dataset dimensions */
	int value;

	/* define dataspace for dataset */
	dimsf[0] = 1;
	dataspace = H5Screate_simple(1,dimsf,NULL);

	/* define datatype for the data using native little endian integers */
	datatype = H5Tcopy(H5T_NATIVE_INT);
	H5Tset_order(datatype, H5T_ORDER_LE);

	/* create a new dataset within the file */
	dataset = H5Dcreate(error_file, DATASETNAME, datatype, dataspace,
			    H5P_DEFAULT);
	if (dataset >= 0) {   /* not an error */
		value = WRITE_NUMBER;
		H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
			 H5P_DEFAULT, &value);
		H5Dclose(dataset);
	}

	H5Tclose(datatype);
	H5Sclose(dataspace);
	return (NULL);
}

herr_t error_callback(void *client_data)
{
	pthread_mutex_lock(&error_mutex);
	error_count++;
	pthread_mutex_unlock(&error_mutex);
	return H5Ewalk(H5E_WALK_DOWNWARD, walk_error_callback, NULL);
}

herr_t walk_error_callback(int n, H5E_error_t *err_desc, void *client_data)
{
	int maj_num, min_num;

	if (err_desc) {
		maj_num = err_desc->maj_num;
		min_num = err_desc->min_num;

		if (n < EXPECTED_ERROR_DEPTH && maj_num == expected[n].maj_num &&
			min_num == expected[n].min_num)
				return SUCCEED;
	}

	error_flag = -1;
	return SUCCEED;
}

void cleanup_error(void)
{
	HDunlink(FILENAME);
}

#endif /*H5_HAVE_THREADSAFE*/
