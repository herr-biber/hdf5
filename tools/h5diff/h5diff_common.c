/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have          *
 * access to either file, you may request a copy from help@hdfgroup.org.     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>
#include <string.h>
#include "h5diff.h"
#include "h5diff_common.h"
#include "h5tools_utils.h"

static int check_n_input( const char* );
static int check_p_input( const char* );
static int check_d_input( const char* );

/*
 * Command-line options: The user can specify short or long-named
 * parameters.
 */
static const char *s_opts = "hVrvqn:d:p:Nc";
static struct long_options l_opts[] = {
    { "help", no_arg, 'h' },
    { "version", no_arg, 'V' },
    { "report", no_arg, 'r' },
    { "verbose", no_arg, 'v' },
    { "quiet", no_arg, 'q' },
    { "count", require_arg, 'n' },
    { "delta", require_arg, 'd' },
    { "relative", require_arg, 'p' },
    { "nan", no_arg, 'N' },
    { "compare", no_arg, 'c' },
    { "use-system-epsilon", no_arg, 'e' },
    { "follow-symlinks", no_arg, 'l' },
    { "no-dangling-links", no_arg, 'x' },
    { NULL, 0, '\0' }
};


/*-------------------------------------------------------------------------
 * Function: parse_command_line
 *
 * Purpose: parse command line input
 *
 *-------------------------------------------------------------------------
 */

void parse_command_line(int argc,
                        const char* argv[],
                        const char** fname1,
                        const char** fname2,
                        const char** objname1,
                        const char** objname2,
                        diff_opt_t* options)
{

    int opt;

    /* process the command-line */
    memset(options, 0, sizeof (diff_opt_t));

    /* assume equal contents initially */
    options->contents = 1;

    /* NaNs are handled by default */
    options->do_nans = 1;

    /* parse command line options */
    while ((opt = get_option(argc, argv, s_opts, l_opts)) != EOF)
    {
        switch ((char)opt)
        {
        default:
            usage();
            h5diff_exit(EXIT_FAILURE);
        case 'h':
            usage();
            h5diff_exit(EXIT_SUCCESS);
        case 'V':
            print_version(h5tools_getprogname());
            h5diff_exit(EXIT_SUCCESS);
        case 'v':
            options->m_verbose = 1;
            break;
        case 'q':
            /* use quiet mode; supress the message "0 differences found" */
            options->m_quiet = 1;
            break;
        case 'r':
            options->m_report = 1;
            break;
        case 'l':
            options->follow_links = 1;
            break;
        case 'x':
            options->no_dangle_links = 1;
            break;
        case 'd':
            options->d=1;

            if ( check_d_input( opt_arg )==-1)
            {
                printf("<-d %s> is not a valid option\n", opt_arg );
                usage();
                h5diff_exit(EXIT_FAILURE);
            }
            options->delta = atof( opt_arg );

            /* -d 0 is the same as default */
            if (options->delta == 0)
            options->d=0;

            break;

        case 'p':

            options->p=1;
            if ( check_p_input( opt_arg )==-1)
            {
                printf("<-p %s> is not a valid option\n", opt_arg );
                usage();
                h5diff_exit(EXIT_FAILURE);
            }
            options->percent = atof( opt_arg );

            /* -p 0 is the same as default */
            if (options->percent == 0)
            options->p = 0;

            break;

        case 'n':

            options->n=1;
            if ( check_n_input( opt_arg )==-1)
            {
                printf("<-n %s> is not a valid option\n", opt_arg );
                usage();
                h5diff_exit(EXIT_FAILURE);
            }
            options->count = atol( opt_arg );

            break;

        case 'N':
            options->do_nans = 0;
            break;
        case 'c':
            options->m_list_not_cmp = 1;
            break;
        case 'e':
            options->use_system_epsilon = 1;
            break;
        }
    }

    /* if use system epsilon, unset -p and -d option */
    if (options->use_system_epsilon)
        options->d = options->p = 0;

    /* check for file names to be processed */
    if (argc <= opt_ind || argv[ opt_ind + 1 ] == NULL)
    {
        error_msg(h5tools_getprogname(), "missing file names\n");
        usage();
        h5diff_exit(EXIT_FAILURE);
    }

    *fname1 = argv[ opt_ind ];
    *fname2 = argv[ opt_ind + 1 ];
    *objname1 = argv[ opt_ind + 2 ];

    if ( *objname1 == NULL )
    {
        *objname2 = NULL;
        return;
    }

    if ( argv[ opt_ind + 3 ] != NULL)
    {
        *objname2 = argv[ opt_ind + 3 ];
    }
    else
    {
        *objname2 = *objname1;
    }


}


/*-------------------------------------------------------------------------
 * Function: print_info
 *
 * Purpose: print several information messages after h5diff call
 *
 *-------------------------------------------------------------------------
 */

 void  print_info(diff_opt_t* options)
 {
     if (options->m_quiet || options->err_stat )
         return;

     if (options->cmn_objs==0)
     {
         printf("No common objects found. Files are not comparable.\n");
         if (!options->m_verbose)
             printf("Use -v for a list of objects.\n");
     }

     if (options->not_cmp==1)
     {
         if ( options->m_list_not_cmp == 0 )
         {
             printf("--------------------------------\n");
             printf("Some objects are not comparable\n");
             printf("--------------------------------\n");
             printf("Use -c for a list of objects.\n");
         }


     }

 }

/*-------------------------------------------------------------------------
 * Function: check_n_input
 *
 * Purpose: check for valid input
 *
 * Return: 1 for ok, -1 for fail
 *
 * Programmer: Pedro Vicente, pvn@ncsa.uiuc.edu
 *
 * Date: May 9, 2003
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
check_n_input( const char *str )
{
    unsigned i;
    char c;

    for ( i = 0; i < strlen(str); i++)
    {
        c = str[i];
        if ( i==0 )
        {
            if ( c < 49 || c > 57  ) /* ascii values between 1 and 9 */
                return -1;
        }
        else
            if ( c < 48 || c > 57  ) /* 0 also */
                return -1;
    }
    return 1;
}

/*-------------------------------------------------------------------------
 * Function: check_p_input
 *
 * Purpose: check for a valid p option input
 *
 * Return: 1 for ok, -1 for fail
 *
 * Date: May 9, 2003
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
check_p_input( const char *str )
{
    double x;

    /*
    the atof return value on a hexadecimal input is different
    on some systems; we do a character check for this
    */
    if (strlen(str)>2 && str[0]=='0' && str[1]=='x')
        return -1;

    x=atof(str);
    if (x<0)
        return -1;

    return 1;
}

/*-------------------------------------------------------------------------
 * Function: check_d_input
 *
 * Purpose: check for a valid d option input
 *
 * Return: 1 for ok, -1 for fail
 *
 * Date: November 11, 2007
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
check_d_input( const char *str )
{
    double x;

    /*
    the atof return value on a hexadecimal input is different
    on some systems; we do a character check for this
    */
    if (strlen(str)>2 && str[0]=='0' && str[1]=='x')
        return -1;

    x=atof(str);
    if (x <0)
        return -1;

    return 1;
}

/*-------------------------------------------------------------------------
 * Function: usage
 *
 * Purpose: print a usage message
 *
 * Return: void
 *
 *-------------------------------------------------------------------------
 */

void usage(void)
{
 printf("usage: h5diff [OPTIONS] file1 file2 [obj1[obj2]] \n");
 printf("  file1                    File name of the first HDF5 file\n");
 printf("  file2                    File name of the second HDF5 file\n");
 printf("  [obj1]                   Name of an HDF5 object, in absolute path\n");
 printf("  [obj2]                   Name of an HDF5 object, in absolute path\n");
 printf("\n");
 printf("  OPTIONS\n");
 printf("   -h, --help              Print a usage message and exit.\n");
 printf("   -V, --version           Print version number and exit.\n");
 printf("   -r, --report            Report mode. Print differences.\n");
 printf("   -v, --verbose           Verbose mode. Print differences, list of objects.\n");
 printf("   -q, --quiet             Quiet mode. Do not produce output.\n");
 printf("   --follow-symlinks       Follow symbolic links (soft links and external links)\n");
 printf("                           and compare the links' target objects.\n");
 printf("                           If symbolic link(s) with the same name exist in the\n");
 printf("                           files being compared, then determine whether the \n");
 printf("                           target of each link is an existing object (dataset,\n");
 printf("                           group, or named datatype) or the link is a dangling\n");
 printf("                           link (a soft or external link pointing to a target\n");
 printf("                           object that does not yet exist).\n");
 printf("                           - If both symbolic links are dangling links, they\n");
 printf("                             are treated as being the same; by default, h5diff\n");
 printf("                             returns an exit code of 0. If, however, \n");
 printf("                             --no-dangling-links is used with --follow-symlinks,\n");
 printf("                             this situation is treated as an error and h5diff \n");
 printf("                             returns an exit code of 2.\n");
 printf("                           - If only one of the two links is a dangling link,\n");
 printf("                             they are treated as being different and h5diff \n");
 printf("                             returns an exit code of 1. If, however, \n");
 printf("                             --no-dangling-links is used with --follow-symlinks,\n");
 printf("                             this situation is treated as an error and h5diff \n");
 printf("                             returns an exit code of 2.\n");
 printf("                           - If both symbolic links point to existing objects,\n");
 printf("                             h5diff compares the two objects.\n");
 printf("                           If any symbolic link specified in the call to h5diff\n");
 printf("                           does not exist, h5diff treats it as an error and\n");
 printf("                           returns an exit code of 2.\n");
 printf("   --no-dangling-links     Must be used with --follow-symlinks option;\n");
 printf("                           otherwise, h5diff shows error message and returns\n");
 printf("                           an exit code of 2.\n");
 printf("                           Check for any symbolic links (soft links or external\n");
 printf("                           links) that do not resolve to an existing object\n");
 printf("                           (dataset, group, or named datatype).  If any\n");
 printf("                           dangling link is found, this situation is treated as\n");
 printf("                           an error and h5diff returns an exit code of 2.\n");
 printf("   -c, --compare           List objects that are not comparable\n");
 printf("   -N, --nan               Avoid NaNs detection\n");
 printf("   -n C, --count=C         Print differences up to C number, C is a positive\n");
 printf("                           integer.\n");
 printf("   -d D, --delta=D         Print difference if (|a-b| > D), D is a positive\n");
 printf("                           number.\n");
 printf("   -p R, --relative=R      Print difference if (|(a-b)/b| > R), R is a positive\n");
 printf("                           number.\n");
 printf("   --use-system-epsilon    Print difference if (|a-b| > EPSILON),\n");
 printf("                           where EPSILON (FLT_EPSILON or FLT_EPSILON) is the\n");
 printf("                           system epsilon value. \n");
 printf("                           If the system epsilon is not defined, use the value\n");
 printf("                           below:\n");
 printf("                               FLT_EPSILON = 1.19209E-07 for float\n");
 printf("                               DBL_EPSILON = 2.22045E-16 for double\n");
 printf("                           -d, -p, and --use-system-epsilon options are used for\n");
 printf("                           comparing floating point values.\n");
 printf("                           By default, strict equality is used. Use -p or -d to\n");
 printf("                           set specific tolerance.\n");
 printf("\n");

 printf(" Modes of output:\n");
 printf("  Default mode: print the number of differences found and where they occured\n");
 printf("  -r Report mode: print the above plus the differences\n");
 printf("  -v Verbose mode: print the above plus a list of objects and warnings\n");
 printf("  -q Quiet mode: do not print output\n");

 printf("\n");

 printf(" Compare criteria\n");
 printf("  If no objects [obj1[obj2]] are specified, h5diff only compares objects\n");
 printf("  with the same absolute path in both files\n");
 printf("\n");

 printf(" The compare criteria is:\n");
 printf("  1) datasets: numerical array differences\n");
 printf("  2) groups: name string difference\n");
 printf("  3) datatypes: the return value of H5Tequal\n");
 printf("  4) links: name string difference of the linked value as default\n");
 printf("            (refer to --follow-symlinks option).\n");
 printf("\n");
 printf(" Exit code:\n");
 printf("  0 if no differences, 1 if differences found, 2 if error\n");
 printf("\n");
 printf(" Examples of use:\n");
 printf(" 1) h5diff file1 file2 /g1/dset1 /g1/dset2\n");
 printf("    Compares object '/g1/dset1' in file1 with '/g1/dset2' in file2\n");
 printf("\n");
 printf(" 2) h5diff file1 file2 /g1/dset1\n");
 printf("    Compares object '/g1/dset1' in both files\n");
 printf("\n");
 printf(" 3) h5diff file1 file2\n");
 printf("    Compares all objects in both files\n");
 printf("\n");
 printf(" Notes:\n");
 printf("  file1 and file2 can be the same file.\n");
 printf("  Use h5diff file1 file1 /g1/dset1 /g1/dset2 to compare\n");
 printf("  '/g1/dset1' and '/g1/dset2' in the same file\n");
 printf("\n");
}


/*-------------------------------------------------------------------------
 * Function: h5diff_exit
 *
 * Purpose: dismiss phdiff worker processes and exit
 *
 * Return: none
 *
 * Programmer: Albert Cheng
 * Date: Feb 6, 2005
 *
 * Comments:
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void h5diff_exit(int status)
{
#ifdef H5_HAVE_PARALLEL
    /* if in parallel mode, dismiss workers, close down MPI, then exit */
    if((g_nTasks > 1) && g_Parallel) {
        phdiff_dismiss_workers();
        MPI_Barrier(MPI_COMM_WORLD);
    }
    if(g_Parallel)
        MPI_Finalize();
#endif
    exit(status);
}

