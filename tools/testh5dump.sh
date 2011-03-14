#!/bin/sh

h5dump=h5dump		# a relative name
cmp='cmp -s'
diff='diff -c'

nerrors=0
verbose=yes

# The build (current) directory might be different than the source directory.
if test "X$srcdir" = X; then
    srcdir=.
fi
mkdir testfiles >/dev/null 2>&1

# Print a line-line message left justified in a field of 70 characters
# beginning with the word "Testing".
TESTING()
{
    SPACES="                                                               "
    echo "Testing $* $SPACES" |cut -c1-70 |tr -d '\012'
}

# Run a test and print PASS or *FAIL*.  If a test fails then increment
# the `nerrors' global variable and (if $verbose is set) display the
# difference between the actual output and the expected output. The
# expected output is given as the first argument to this function and
# the actual output file is calculated by replacing the `.ddl' with
# `.out'.  The actual output is not removed if $HDF5_NOCLEANUP has a
# non-zero value.
DUMP()
{
    expect=$srcdir/testfiles/$1
    actual="testfiles/`basename $1 .ddl`.out"
    shift
    full=`pwd`/$h5dump

    # Run test.
    TESTING $h5dump $@
    (
	echo "#############################"
	echo "Expected output for '$h5dump $@'" 
	echo "#############################"
	cd $srcdir/testfiles
        $RUNSERIAL $full "$@" 2>/dev/null
    ) >$actual
    
    if $cmp $expect $actual; then
	echo " PASSED"
    else
	echo "*FAILED*"
	echo "    Expected result (*.ddl) differs from actual result (*.out)"
	nerrors="`expr $nerrors + 1`"
	test yes = "$verbose" && $diff $expect $actual |sed 's/^/    /'
    fi

    # Clean up output file
    if [ X = ${HDF5_NOCLEANUP:-X} ]; then
	rm -f $actual
    fi
}



##############################################################################
##############################################################################
###			  T H E   T E S T S                                ###
##############################################################################
##############################################################################

# test for displaying groups
DUMP tgroup-1.ddl tgroup.h5
# test for displaying the selected groups
DUMP tgroup-2.ddl -g /g2 / /y tgroup.h5

# test for displaying simple space datasets
DUMP tdset-1.ddl tdset.h5
# test for displaying selected datasets
DUMP tdset-2.ddl -header -d dset1 /dset2 dset3 tdset.h5

# test for displaying attributes
DUMP tattr-1.ddl tattr.h5
# test for displaying the selected attributes of string type and scalar space
DUMP tattr-2.ddl -a attr1 attr4 attr5 tattr.h5
# test for header and error messages
DUMP tattr-3.ddl -header -a attr2 attr tattr.h5

# test for displaying soft links
DUMP tslink-1.ddl tslink.h5
# test for displaying the selected link
DUMP tslink-2.ddl -l slink2 tslink.h5

# tests for hard links
DUMP thlink-1.ddl thlink.h5
DUMP thlink-2.ddl -d /g1/dset2 /dset1 /g1/g1.1/dset3 thlink.h5
DUMP thlink-3.ddl -d /g1/g1.1/dset3 /g1/dset2 /dset1 thlink.h5
DUMP thlink-4.ddl -g /g1 thlink.h5
DUMP thlink-5.ddl -d /dset1 -g /g2 -d /g1/dset2 thlink.h5

# tests for compound data types
DUMP tcomp-1.ddl tcompound.h5
# test for named data types
DUMP tcomp-2.ddl -t /type1 /type2 /group1/type3 tcompound.h5
# test for unamed type 
DUMP tcomp-3.ddl -t /#5992:0 -g /group2 tcompound.h5

#test for the nested compound type
DUMP tnestcomp-1.ddl tnestedcomp.h5

# test for options
DUMP tall-1.ddl tall.h5
DUMP tall-2.ddl -header -g /g1/g1.1 -a attr2 tall.h5
DUMP tall-3.ddl -d /g2/dset2.1 -l /g1/g1.2/g1.2.1/slink tall.h5

# test for loop detection
DUMP tloop-1.ddl tloop.h5

# test for string 
DUMP tstr-1.ddl tstr.h5

if test $nerrors -eq 0 ; then
	echo "All h5dump tests passed."
fi

exit $nerrors
