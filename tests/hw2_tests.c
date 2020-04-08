#include <stdio.h>
#include <unistd.h>
#include <criterion/criterion.h>
#include <string.h>

#define TEST_TIMEOUT 15

#define TEST_REFBIN_DIR "tests/refbin"
#define TEST_REF_DIR "tests/rsrc"
#define TEST_OUTPUT_DIR "tests.out"
#define REF_OUTPUT_DIR "ref.out"

#define STDOUT_EXT ".out"
#define STDERR_EXT ".err"

extern int errors, warnings;

static char program_options[500];
static char test_input_file[500];
static char test_output_subdir[100];
static char ref_output_subdir[100];
static char test_log_outfile[100];
static char ref_log_outfile[100];

/*
 * Test coverage info:
 *
 *   TEST				DETECTED BY
 *   Use-after-free bug			valgrind-leak_test
 *					valgrind-uninitialized_test
 *   Uninitialized pointer variable	(many tests)
 *   Malloc sizeof bug			(many tests)
 *   Missing call to closedir 		valgrind-leak_test
 *   Missing pointer initialization	sort-test
 *   > instead of strcmp		sort-test
 *   Undo size calculation correction	(many tests)
 *   Undo correct inode printout	total_inode_test
 *   --no-follow-symlinks fails		no_follow_symlinks_neg_test
 *   Height limit (-h #)		height_test
 *   Subdirectories (-h # -s)		subdir_test
 *   Totals (-t)			total_test
 *   Total with inodes (-t -i)		total_inode_test
 *   Height no number (-h)		height_bad_arg_test
 *   Sorted output (-o)			sort_test
 *   Quick display (-q)			quick_test
 *   Visual display (-v)		visual_test
 *   Verbose output (-VV)		verbose_test
 *   Abbreviated long options		getopt_test
 *
 * Compilation options that should be tried:
 *   MEMORY_BASED  --  with sort test
 *   HSTATS -- 
 *   LSTAT -- with symbolic link tests
 */


/*
 * Sets up to run a test.
 * Initialize various filenames, using the name of the test as a base,
 * and then initialize and run a command to remove old output from this test
 * and to make sure that the test output directory exists.
 */
int setup_test(char *name) {
    char cmd[500];
    sprintf(test_input_file, "%s/%s.in", TEST_REF_DIR, name);
    sprintf(test_log_outfile, "%s/%s", TEST_OUTPUT_DIR, name);
    sprintf(ref_log_outfile, "%s/%s", REF_OUTPUT_DIR, name);
    sprintf(cmd, "rm -f %s.out %s.err %s%s %s%s; mkdir -p %s; mkdir -p %s",
	    test_log_outfile, test_log_outfile,
	    ref_log_outfile, STDOUT_EXT, ref_log_outfile, STDERR_EXT,
	    TEST_OUTPUT_DIR, REF_OUTPUT_DIR);
    fprintf(stderr, "setup(%s)\n", cmd);
    return system(cmd);
}

/*
 * Run the program as a "black box" using system().
 * A shell command is constructed and run that first performs test setup,
 * then runs the program to be tested with input redirected from a test input file
 * and standard and error output redirected to separate output files.
 */
int run_using_system(char *name, char *pre_cmd, char *valgrind_cmd) {
    char cmd[500];
    setup_test(name);
    sprintf(cmd, "%scat %s | %s %s/vtree %s > %s%s 2> %s%s",
	    pre_cmd, test_input_file, valgrind_cmd, TEST_REFBIN_DIR, program_options,
	    ref_log_outfile, STDOUT_EXT, ref_log_outfile, STDERR_EXT);
    fprintf(stderr, "run(%s)\n", cmd);
    int ret = system(cmd);
    if(ret == -1) {
      cr_log_error("Failed to run test reference: ret = %d\n", ret);
      abort();
    }
    sprintf(cmd, "ulimit -t 60; %scat %s | %s bin/vtree %s > %s%s 2> %s%s",
	    pre_cmd, test_input_file, valgrind_cmd, program_options,
	    test_log_outfile, STDOUT_EXT, test_log_outfile, STDERR_EXT);
    fprintf(stderr, "run(%s)\n", cmd);
    return system(cmd);
}

void assert_normal_exit(int status) {
    cr_assert_eq(status, 0, "The program did not exit normally (status = 0x%x).\n", status);
}

void assert_error_exit(int status) {
    cr_assert_eq(WEXITSTATUS(status), 0xff,
		 "The program did not exit with status 0xff (status was 0x%x).\n",
		 status);
}

/*
 * Compare output from the program being tested with reference output,
 * after first possibly applying a filter.
 */
void assert_file_matches(char *name, char *ext, char *filter) {
    char cmd[500];
    sprintf(cmd, "cat %s%s | %s > %s/%s_flt%s; cat %s%s | %s > %s/%s_flt%s; "
	    "diff --ignore-tab-expansion --ignore-trailing-space --ignore-space-change --ignore-blank-lines %s/%s_flt%s %s/%s_flt%s",
	    test_log_outfile, ext, filter ? filter : "cat",
	    TEST_OUTPUT_DIR, name, ext,
	    ref_log_outfile, ext, filter ? filter : "cat",
	    REF_OUTPUT_DIR, name, ext,
	    TEST_OUTPUT_DIR, name, ext, REF_OUTPUT_DIR, name, ext);
    int err = system(cmd);
    cr_assert_eq(err, 0, "The output was not what was expected (diff exited with status %d).\n",
		 WEXITSTATUS(err));
}

void assert_no_valgrind_errors(int status) {
    cr_assert_neq(WEXITSTATUS(status), 37, "Valgrind reported errors -- see %s%s",
		  test_log_outfile, STDERR_EXT);
}

/*
 * The basic test configuration is using MEMORY_BASED.
 * Most tests will be run this way.
 */

#if defined(MEMORY_BASED)

/*
 * Tests the basic program operation, with the "quick display" option.
 */
Test(base_suite, quick_test_1, .timeout=TEST_TIMEOUT) {
    char *name = "quick_test_1";
    sprintf(program_options, "-q tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * Tests the basic program operation, with the "quick display" option.
 * This time, check that the error output matches (should be empty).
 */
Test(base_suite, quick_test_2, .timeout=TEST_TIMEOUT) {
    char *name = "quick_test_2";
    sprintf(program_options, "-q tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDERR_EXT, NULL);
}

/*
 * This test will fail unless long options have been implemented, as per the
 * assignment handout.
 */
Test(options_suite, getopt_test, .timeout=TEST_TIMEOUT) {
    char *name = "getopt_test";
    sprintf(program_options, "--visual tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, "grep -v /home/gene");
}

/*
 * Bad argument error, "-h" without number.
 */
Test(options_suite, height_bad_arg_test_1, .timeout=TEST_TIMEOUT) {
    char *name = "height_bad_arg_test_1";
    sprintf(program_options, "-h tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_error_exit(err);
    // The assignment didn't specify the exact message to be printed, only that there
    // should be one.  We will just check if the file is nonempty.
    assert_file_matches(name, STDERR_EXT, "head -1 | wc -l");
}

/*
 * Bad argument error, "-h" without number.
 * This time, check the standard output (should be empty).
 */
Test(options_suite, height_bad_arg_test_2, .timeout=TEST_TIMEOUT) {
    char *name = "height_bad_arg_test_2";
    sprintf(program_options, "-h tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_error_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * When the "-o" flag is given, the program is supposed to sort the entries within
 * each directory, so that they are printed out in lexicographic order.  Otherwise,
 * they are printed out in the order in which they are stored in the directory,
 * which is somewhat indeterminate.
 */
Test(feature_suite, sort_test, .timeout=TEST_TIMEOUT) {
    char *name = "sort_test";
    sprintf(program_options, "-o tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * When the "-i" flag is enabled, the program counts "inodes".  There is one inode
 * for each distinct file or directory that is encountered.
 */
Test(feature_suite, inode_test, .timeout=TEST_TIMEOUT) {
    char *name = "inode_test";
    sprintf(program_options, "-i tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, "grep -v /home/gene");
}

/*
 * This test sets up a test tree in which there are three "hard links" to the same file.
 * Normally, the program will detect these using its hash table and avoid counting the
 * space used multiple times.  With the "-d" flag, the duplicate checking is disabled,
 * so the space usage reported will be higher.
 */
Test(feature_suite, duplicates_test, .timeout=TEST_TIMEOUT) {
    char *name = "duplicates_test";
    system("rm -fr tests/rsrc/dups; mkdir tests/rsrc/dups; echo foo > tests/rsrc/dups/foo; "
           "ln tests/rsrc/dups/foo tests/rsrc/dups/bar; ln tests/rsrc/dups/foo tests/rsrc/dups/mumble");
    sprintf(program_options, "-d tests/rsrc/dups");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * "-h" option test
 */
Test(feature_suite, height_test, .timeout=TEST_TIMEOUT) {
    char *name = "height_test";
    sprintf(program_options, "-h 2 tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * "-s" option test
 */
Test(feature_suite, subdir_test, .timeout=TEST_TIMEOUT) {
    char *name = "subdir_test";
    sprintf(program_options, "-h 2 -s tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * "-t" option test
 */
Test(feature_suite, total_test, .timeout=TEST_TIMEOUT) {
    char *name = "total_test";
    sprintf(program_options, "-t tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * "-t -i" option test
 */
Test(feature_suite, total_inode_test, .timeout=TEST_TIMEOUT) {
    char *name = "total_inode_test";
    sprintf(program_options, "-t -i tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * "-v" option test
 */
Test(feature_suite, visual_test, .timeout=TEST_TIMEOUT) {
    char *name = "visual_test";
    sprintf(program_options, "-v tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * "-VV" option test
 */
Test(feature_suite, verbose_test, .timeout=TEST_TIMEOUT) {
    char *name = "verbose_test";
    sprintf(program_options, "-VV tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * This test runs valgrind to check for the use of uninitialized variables.
 */
Test(valgrind_suite, valgrind_uninitialized_test, .timeout=TEST_TIMEOUT) {
    char *name = "valgrind_uninitialized_test";
    sprintf(program_options, "-q tests/rsrc/test_tree");
    int err = run_using_system(name, "", "valgrind --leak-check=no --undef-value-errors=yes --error-exitcode=37");
    assert_no_valgrind_errors(err);
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * This test runs valgrind to check for memory leaks.
 */
Test(valgrind_suite, valgrind_leak_test, .timeout=TEST_TIMEOUT) {
    char *name = "valgrind_leak_test";
    sprintf(program_options, "-q tests/rsrc/test_tree");
    int err = run_using_system(name, "", "valgrind --leak-check=full --undef-value-errors=no --error-exitcode=37");
    assert_no_valgrind_errors(err);
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

#else

/*
 * If we are not using MEMORY_BASED, then re-run the options printout test to
 * verify that it now says "disk based" instead of "memory based".
 */

/*
 * "-VV" option test
 */
Test(no_mb_suite, no_mb_verbose_test, .timeout=TEST_TIMEOUT) {
    char *name = "no_mb_verbose_test";
    sprintf(program_options, "-VV tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * Since the original program accepted the -o option even when MEMORY_BASED was not
 * used (but it did not sort), I had the idea of testing this behavior.  However,
 * it seems like many students conditionalized the -o option so that it would not
 * be accepted unless MEMORY_BASED was defined, so I am turning this test off.
 */

#if 0
/*
 * When the "-o" flag is given, the program is supposed to sort the entries within
 * each directory, so that they are printed out in lexicographic order.  Otherwise,
 * they are printed out in the order in which they are stored in the directory,
 * which is somewhat indeterminate.
 */
Test(no_mb_suite, no_mb_sort_test, .timeout=TEST_TIMEOUT) {
    char *name = "no_mb_sort_test";
    sprintf(program_options, "-o tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}
#endif

#endif

/*
 * Secondary test configuration is with LSTAT and HSTATS, but without MEMORY_BASED.
 * A few extra tests are run to cover these cases.
 */

#ifdef LSTAT
/*
 * The --no-follow-symlinks argument should work properly if the program
 * has been compiled with LSTAT.
 */
Test(lstat_suite, no_follow_symlinks_test, .timeout=TEST_TIMEOUT) {
    char *name = "no_follow_symlinks_test";
    sprintf(program_options, "--no-follow-symlinks tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}
#else
/*
 * The --no-follow-symlinks argument should elicit an error unless the program
 * has been compiled with LSTAT.
 */
Test(lstat_suite, no_follow_symlinks_neg_test, .timeout=TEST_TIMEOUT) {
    char *name = "no_follow_symlinks_neg_test";
    sprintf(program_options, "--no-follow-symlinks tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_error_exit(err);
}
#endif

#ifdef HSTATS
/*
 * Tests the basic program operation, with the "quick display" option.
 */
Test(hstats_suite, hstats_quick_test_1, .timeout=TEST_TIMEOUT) {
    char *name = "hstats_quick_test_1";
    sprintf(program_options, "-q tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDOUT_EXT, NULL);
}

/*
 * Tests the basic program operation, with the "quick display" option.
 * This time, check that the error output matches (should be empty).
 */
Test(hstats_suite, hstats_quick_test_2, .timeout=TEST_TIMEOUT) {
    char *name = "hstats_quick_test_2";
    sprintf(program_options, "-q tests/rsrc/test_tree");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_file_matches(name, STDERR_EXT, NULL);
}
#endif

