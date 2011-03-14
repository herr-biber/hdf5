#ifndef _H5Exception_H
#define _H5Exception_H

#include <string>

#ifndef H5_NO_NAMESPACE
namespace H5 {
using namespace std;
#endif

class Exception {
   public:
	// Default constructor
	Exception();

	// Creates an exception with a function name where the failure occurs
	// and an optional detailed message
	Exception( const string& func_name, const string& message = NULL);
	Exception( const char* func_name, const char* message = NULL);

	// copy constructor
	Exception( const Exception& orig);

	// Returns the character string that describes an error specified by
	// a major error number.
	string getMajorString( H5E_major_t major_num ) const;

	// Returns the character string that describes an error specified by
	// a minor error number.
	string getMinorString( H5E_minor_t minor_num ) const;

	// Returns the detailed message set at the time the exception is thrown
	string getDetailMsg() const;
	const char* getCDetailMsg() const;	// C string of detailed message
	string getFuncName() const;	// function name as a string object
	const char* getCFuncName() const;	// function name as a char string 

	// Turns on the automatic error printing.
	static void setAutoPrint( H5E_auto_t func, void* client_data);

	// Turns off the automatic error printing.
	static void dontPrint();

	// Retrieves the current settings for the automatic error stack 
	// traversal function and its data.
	static void getAutoPrint( H5E_auto_t& func, void** client_data);

	// Clears the error stack for the current thread.
	static void clearErrorStack();

	// Walks the error stack for the current thread, calling the 
	// specified function.
	static void walkErrorStack( H5E_direction_t direction, 
				H5E_walk_t func, void* client_data);

	// Default error stack traversal callback function that prints 
	// error messages to the specified output stream.
	static void walkDefErrorStack( int n, H5E_error_t& err_desc,
				void* client_data);

	// Prints the error stack in a default manner.
	virtual void printError( FILE* stream = NULL ) const;

	// virtual Destructor
	virtual ~Exception();

   private:
	string detailMessage;
	string funcName;
};

class FileIException : public Exception {
   public:
	FileIException();
	FileIException( const string& func_name, const string& message = NULL);
	FileIException( const char* func_name, const char* message = NULL);
	virtual ~FileIException();
};

class GroupIException : public Exception {
   public:
	GroupIException();
	GroupIException( const string& func_name, const string& message=NULL);
	GroupIException( const char* func_name, const char* message = NULL);
	virtual ~GroupIException();
};

class DataSpaceIException : public Exception {
   public:
	DataSpaceIException();
	DataSpaceIException(const string& func_name, const string& message=NULL);
	DataSpaceIException(const char* func_name, const char* message = NULL);
	virtual ~DataSpaceIException();
};

class DataTypeIException : public Exception {
   public:
	DataTypeIException();
	DataTypeIException(const string& func_name, const string& message = NULL);
	DataTypeIException(const char* func_name, const char* message = NULL);
	virtual ~DataTypeIException();
};

class PropListIException : public Exception {
   public:
	PropListIException();
	PropListIException(const string& func_name, const string& message=NULL);
	PropListIException(const char* func_name, const char* message = NULL);
	virtual ~PropListIException();
};

class DataSetIException : public Exception {
   public:
	DataSetIException();
	DataSetIException(const string& func_name, const string& message=NULL);
	DataSetIException(const char* func_name, const char* message = NULL);
	virtual ~DataSetIException();
};

class AttributeIException : public Exception {
   public:
	AttributeIException();
	AttributeIException(const string& func_name, const string& message=NULL);
	AttributeIException(const char* func_name, const char* message = NULL);
	virtual ~AttributeIException();
};

class ReferenceException : public Exception {
   public:
	ReferenceException();
	ReferenceException(const string& func_name, const string& message=NULL);
	ReferenceException(const char* func_name, const char* message = NULL);
	virtual ~ReferenceException();
};

class LibraryIException : public Exception {
   public:
	LibraryIException();
	LibraryIException(const string& func_name, const string& message=NULL);
	LibraryIException(const char* func_name, const char* message = NULL);
	virtual ~LibraryIException();
};

class IdComponentException : public Exception {
   public:
	IdComponentException();
	IdComponentException(const string& func_name, const string& message=NULL);
	IdComponentException(const char* func_name, const char* message = NULL);
	virtual ~IdComponentException();
};

// The following are from Java API but not done here:
// AtomException, BtreeException, DataFiltersException, 
// ExternalFilelistException, FunctionEntryExitException, 
// HeapException, InternalErrorException, LowLevelIOException, 
// MetaDataCacheException, ResourceUnavailableException, 
// SymbolTableException, ObjectHeaderException, FunctionArgumentException,
// DataStorageException
#ifndef H5_NO_NAMESPACE
}
#endif

#endif // _H5Exception_H
