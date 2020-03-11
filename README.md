# Homework 2 Debugging and Fixing - CSE 320 - Spring 2020
#### Professor Eugene Stark

### **Due Date: Friday 3/6/2020 @ 11:59pm**

# Introduction

In this assignment you are tasked with updating an old piece of
software, making sure it compiles, and that it works properly
in your VM environment.

Maintaining old code is a chore and an often hated part of software
engineering. It is definitely one of the aspects which are seldom
discussed or thought about by aspiring computer science students.
However, it is prevalent throughout industry and a worthwhile skill to
learn.  Of course, this homework will not give you a remotely
realistic experience in maintaining legacy code or code left behind by
previous engineers but it still provides a small taste of what the
experience may be like.  You are to take on the role of an engineer
whose supervisor has asked you to correct all the errors in the
program, plus add additional functionality.

By completing this homework you should become more familiar
with the C programming language and develop an understanding of:

- How to use tools such as `gdb` and `valgrind` for debugging C code.
- Modifying existing C code.
- C memory management and pointers.

## The Existing Program

Your goal will be to debug and extend an old program called `vtree`,
which was posted to Usenet sometime between March and May, 1989.
It was written by Jonathan Bayer of Intelligent Software Products, Inc.,
and it ran under the following operating systems:
4.2BSD, 4.3BSD, System V, System III, SCO Xenix 386, and SCO Xenix 286.
The version I am handing out is very close to the original version,
except that I have made a few changes for this assignment.
First of all, I rearranged the source tree and re-wrote the `Makefile`
to conform to what we are using for the other assignments in this course.
I also introduced a few bugs here and there to make things more interesting
and educational for you :wink:.
Aside from these changes and the introduced bugs, which only involve a few
lines, the code is identical to the original, functioning version.
During my analysis of this program for this assignment, I also found various
bugs in the original version, which I have left for you to find as well.

The purpose of the `vtree` program is to traverse a portion of the Unix
filesystem hierarchy, collect information on the number of inodes
(basically, files and directories) and the amount of disk space used,
and to display a representation of the hierarchy in a "visual" format,
possibly with summary statistics.  The functionality is similar to,
but less refined than, the more modern program `tree`, which is installed
on your Linux Mint system.

In the default mode of execution (which occurs when you invoke the program
just using `bin/vtree` with no arguments), the program traverses the filesystem
hierarchy rooted at the current working directory and displays the name of
each file and directory and the number of K-bytes of disk space occupied by
each of them.  Various options can be supplied to modify the behavior of
the program, as discussed in more detail below.

The `vtree` program is pretty simple, though there are some technical aspects
involved in traversing directories.  Although it is not a very good example of
how to write readable code, with some effort you should be able to get the
understanding you need of how it works in order to be able to complete the
assignment.  Note that you probably don't need to understand every detail.
This is pretty realistic as far as working on legacy software is concerned:
often one would be given code that needs to be updated, ported, or have some
bugs fixed, and this has to be done without (at least initially) having a full
understanding of structure and function of the code.
In this kind of situation, you have to be careful not to make arbitrary changes
to the code that might impact things that you don't fully understand.
Limit your changes to the minimum necessary to achieve the specified objectives.

### Getting Started - Obtain the Base Code

Fetch base code for `hw2` as you did for the previous assignments.
You can find it at this link:
[https://gitlab02.cs.stonybrook.edu/cse320/hw2](https://gitlab02.cs.stonybrook.edu/cse320/hw2).

Once again, to avoid a merge conflict with respect to the file `.gitlab-ci.yml`,
use the following command to merge the commits:

<pre>
  git merge -m "Merging HW2_CODE" HW2_CODE/master --strategy-option=theirs
</pre>

  > :nerd: I hope that by now you would have read some `git` documentation to find
  > out what the `--strategy-option=theirs` does, but in case you didn't :angry:
  > I will say that merging in `git` applies a "strategy" (the default strategy
  > is called "recursive", I believe) and `--strategy-option` allows an option
  > to be passed to the strategy to modify its behavior.  In this case, `theirs`
  > means that whenever a conflict is found, the version of the file from
  > the branch being merged (in this case `HW2_CODE/master`) is to be used in place
  > of the version from the currently checked-out branch.  An alternative to
  > `theirs` is `ours`, which makes the opposite choice.  If you don't specify
  > one of these options, `git` will leave conflict indications in the file itself
  > and it will be necessary for you to edit the file and choose the code you want
  > to use for each of the indicated conflicts.

Here is the structure of the base code:

<pre>
.
├── .gitlab-ci.yml
└── hw2
    ├── doc
    │   ├── README
    │   └── vtree.1
    ├── include
    │   ├── customize.h
    │   ├── debug.h
    │   ├── hash.h
    │   └── patchlevel.h
    ├── Makefile
    ├── src
    │   ├── direct.c
    │   ├── hash.c
    │   ├── main.c
    │   └── vtree.c
    └── tests
        ├── hw2_tests.c
        └── rsrc
            ├── duplicates_test.err
            ├── duplicates_test.in
            ├── duplicates_test.out
            ├── getopt_test.err
            ├── getopt_test.in
            ├── getopt_test.out
            ├── inode_test.err
            ├── inode_test.in
            ├── inode_test.out
            ├── option_error_test.err
            ├── option_error_test.in
            ├── option_error_test.out
            ├── quick_test.err
            ├── quick_test.in
            ├── quick_test.out
            ├── sort_test.err
            ├── sort_test.in
            ├── sort_test.out
            ├── test_tree
            │   ├── A
            │   ├── B
            │   ├── C
            │   ├── D
            │   └── S
            │       ├── a
            │       │   └── 4K
            │       ├── b
            │       │   └── 8K
            │       ├── c
            │       │   └── 12K
            │       ├── d
            │       │   └── 16K
            │       └── E
            ├── valgrind_leak_test.err
            ├── valgrind_leak_test.in
            ├── valgrind_leak_test.out
            ├── valgrind_uninitialized_test.err
            ├── valgrind_uninitialized_test.in
            └── valgrind_uninitialized_test.out
</pre>

The `doc` directory included with the assignment basecode contains the
original documentation files (a `README` and a Unix-style `man` page) that
were distributed with the program.  You can format and read the `man` page
using the following command:

<pre>
nroff -man doc/vtree.1 | less
</pre>

  > :nerd:  Since time immemorial, Unix `man` pages have been written in
  > a typesetting language called `roff` (short for "run-off").
  > The `nroff` program processes `roff` source and produces text formatted
  > for reading in a terminal window.

The source files for the `vtree` application are in the `src` directory, with associated
header files in the `include` directory.  There are only three source files:
`vtree.c` which is the main part of the code, `hash.c`, which implements a hash table
for the purpose of avoiding double-counting of files that can be accessed via more than
one path, and `direct.c`, which contains some implementations of directory-reading functions
that were required on Unix variants  that did not have the `readdir` function supported
by BSD and System V.
This function, which is now part of the POSIX standard, is supported by modern Unix-like
systems including Linux, so we will not need to actually use the functions in this file.
However, as our objective is to make the program compile and run on Linux without affecting
its ability to compile and run on the other supported systems (though we won't really be able
to test this), we do not want to delete this file entirely.

The code contains sections that are conditionalized on various preprocessor symbols
being defined:

- The operating system on which the program is being compiled is specified by defining
  one of the symbols `BSD`, `SYS_V`, `SYS_III`, `SCO_XENIX`.  You will be adding support
  for Linux, conditionalized on `LINUX`.

- Some other variations can be obtained by defining zero or more of the symbols
  `MEMORY_BASED`, `HSTATS`, and `LSTAT`, and `ONEPERLINE`:

	- Compiling with `MEMORY_BASED` defined causes code to be used that builds a data structure
      in memory to represent the directory hierarchy being traversed.  This avoids having
	  to traverse each directory twice and it also permits directory entries to be presented in
	  sorted order.
	- Compiling with `HSTATS` defined causes the program to print out statistics about the
      hash table.
	- Compiling with `LSTAT` defined supposedly enables code to avoid following symbolic links,
      but it seems that it doesn't function reasonably.  You will correct it.
    - I didn't manage to understand clearly what `ONEPERLINE` was supposed to do, so you can
	  just ignore it.

The `Makefile` provides you with the option of selecting various compilation flags from
the command line.  For example:
```
$ make OS=BSD
```
will define the `BSD` preprocessor symbol instead of the `LINUX` symbol, which is the default.
Also:
```
make OPTIONS="-DMEMORY_BASED -DHSTATS"
```
will define both the `MEMORY_BASED` and `HSTATS` preprocessor symbols.

The `tests` directory contains C source code (in file `hw2_tests.c`) for some
Criterion tests I have supplied.
The subdirectory `tests/rsrc` contains data files that are used by the tests.

You can modify anything you want in the assignment (except `main.c`), but limit your changes
to the minimum necessary to restore functionality to the program.  Assume that the program
is essentially correct -- it just has a few lingering bugs that need to be fixed.

Before you begin work on this assignment, you should read the rest of this
document.  In addition, we additionally advise you to read the
[Debugging Document](DebuggingRef.md).

# Part 1: Debugging and Fixing

The command line arguments and expected operation of the program are described
by the following "Usage" message, which is printed within `vtree_main()` in `vtree.c`:

```
bin/vtree: [ -d ] [ -h # ] [ -i ] [ -o ] [ -s ] [ -q ] [ -v ] [ -V ]
	-d	count duplicate inodes
	-f	floating column widths
	-h #	height of tree to look at
	-i	count inodes
	-o	sort directories before processing
	-s	include subdirectories not shown due to -h option
	-t	totals at the end
	-q	quick display, no counts
	-v	visual display
	-V	show current version
		(2 Vs shows specified options)
```

  > :anguished: What is now the function `vtree_main()` was simply `main()` in the original code.
  > I have changed it so that `main()` now resides in a separate file `main.c` and
  > simply calls `vtree_main()`.  This is to make the structure conform to what is
  > needed in order to be able to use Criterion tests with the program.  **Do not make
  > any modifications to `main.c`.**

- The `-d` option causes `vtree` not to use a hash table to check whether a particular inode
(file or directory) has already been encountered and its size tallied.  This might result in
the program over-estimating the total space used.

- I don't know what the `-f` option is really supposed to do.  Ignore it.

- The `-h` option causes `vtree` to limit its traversal of subdirectories to the specified
"height" (personally, I would call it "depth").

- The `-i` option asks `vtree` to count and report the numbers of inodes (files or directories),
in addition to the amount of disk space used.

- The `-o` option causes `vtree` to sort directory entries in lexicographic order before printing
them.  This option will only work if the program has been compiled with `MEMORY_BASED` defined.

- The `-s` option causes `vtree` to descend into subdirectories and calculate their size, etc.,
even if these subdirectories are not shown due to their being beyond the horizon specified with `-h`.

- The `-t` option causes `vtree` to show summary totals at the end of the tree printout.

- The `-q` option just displays the directories, without size or inode information.

- The `-v` option enables a "visual" display, which perhaps shows the tree structure more clearly.

- The `-V` option displays the program version and whether or not the program was compiled
with the `MEMORY_BASED` compilation option.  The `-VV` option also shows information about
the command-line options that were supplied when the program was run.

You are to complete the following steps:

1. Clean up the code; fixing any compilation issues, so that it compiles
   without error using the compiler options that have been set for you in
   the `Makefile`.

    > :nerd: When you look at the code, you will likely notice very quickly that
    > (except for `main.c`, which I added) it is written in the "K+R" dialect of C
    > (the ANSI standard either didn't exist yet or was newly introduced when the program was written).
    > In K+R C, function definitions gave the names of their
    > formal parameters within parentheses after the function name, but the types of the formal
    > parameters were declared between the parameter list and the function body,
    > as in the following example taken from `search.c`:
    >
    > ```c
    > 	get_data(path,cont)
    >   char           *path;
    >   int		cont;    
    >   {
	>   ...
	>   }
    > ```

    As part of your code clean-up, you should provide ANSI C function prototypes where required.
    For functions defined in one `.c` file and used in another, their function prototypes should
    be placed in a `.h` file that is included both in the `.c` file where the function is defined
    and in the `.c` files where the function is used.  For functions used only in the `.c` file
    in which they are defined, if a function prototype is required (it will be, if any use
    of the function occurs before its definition) then the function prototype should be put in
    that same `.c` file and the function should be declared `static`.
    It is not necessary to re-write the existing function definitions into ANSI C style; just
    add the required function prototypes.

    > :nerd: As you clean up the code, you will notice that return types have not been specified
    > for some functions that sometimes return a value and that some functions that have been
    > declared to return a value actually do not.  You should select a prototype for each such
    > function that is appropriate to the way the function was intended to work.  If the function
    > does not actually return any value, use return type `void`.

    Use `git` to keep track of the changes you make and the reasons for them, so that you can
    later review what you have done and also so that you can revert any changes you made that
    don't turn out to be a good idea in the end.

2. Fix bugs.

    Run the program, exercising the various options, and look for cases in which the program
    crashes or otherwise misbehaves in an obvious way.  We are only interested in obvious
    misbehavior here; don't agonize over program behavior that might just have been the choice
    of the original author.  You should use the provided Criterion tests to help point the way,
	though they are not exhaustive.

    One way in which the program does not behave "correctly" is that in tallying the
	number of inodes and amount of disk space consumes, it only accumulates information for
	the regular files and does not take into account the directories themselves.  You should
	correct this.  Also, the calculation of the amount of disk space consumed is inaccurate
	in that, besides omitting the disk space used by directories, it calculates the space
	consumed by regular files using the file length, rather than the number of disk blocks
	actually used.  The file length comes from the `st_size` field of the `struct stat`
	structure (do `man 2 stat` to read about the `stat` system call).  The original program
	just takes this value, rounds up to the next multiple of 1K bytes and sums over all the
	files.  However, for various reasons, a file may occupy more disk space than this calculation
	would indicate.   A more accurate calculation would use the `st_blocks` field of the
	`struct stat` structure.  This field gives the actual number of 512-byte disk blocks
	consumed by the file.  You should change the program so that it produces accurate results
	in this way.  When you get it right, your program should produce totals that agree with
	those produced by running `du -s` (do `man du` to find out about this command).

3. Use `valgrind` to identify any memory leaks or other memory access errors.
   Fix any errors you find.

    Run `valgrind` using a command of the following form:

    <pre>
      $ valgrind --leak-check=full --show-leak-kinds=all [VTREE PROGRAM AND ARGS]
    </pre>

    Note that the bugs that are present will all manifest themselves in some way
    either as program crashes or as memory errors that can be detected by `valgrind`.
    It is not necessary to go hunting for obscure issues with the program output.
    Also, do not make gratuitous changes to the program output, as this will
    interfere with our ability to test your code.

   > :scream:  Note that we are not considering memory that is "still reachable"
   > to be a memory leak.  This corresponds to memory that is in use when
   > the program exits and can still be reached by following pointers from variables
   > in the program.  Although some people consider it to be untidy for a program
   > to exit with "still reachable" memory, it doesn't cause any particular problem.

   > :scream: You are **NOT** allowed to share or post on PIAZZA
   > solutions to the bugs in this program, as this defeats the point of
   > the assignment. You may provide small hints in the right direction,
   > but nothing more.

# Part 2: Adding Features

The original program uses `getopt()` to do options processing, but it only understands
traditional, single-character options.
For this part of the assignment, rewrite the options processing so that it uses
`getopt_long()` instead of `getopt()`, and it understands the following alternative
forms for the various options (as well as the original short forms):

   - `--duplicates` as equivalent to `-d`
   - `--floating-column-widths` as equivalent to `-f`
   - `--height` as equivalent to `-h`
   - `--inodes` as equivalent to `-i`
   - `--sort-directories` as equivalent to `-o`
   - `--totals` as equivalent to `-t`
   - `--quick-display` as equivalent to `-q`
   - `--visual-display` as equivalent to `-v`
   - `--version` as equivalent to `-V`

You will probably need to read the Linux "man page" on the `getopt` package.
This can be accessed via the command `man 3 getopt`.  If you need further information,
search for "GNU getopt documentation" on the Web.

> :scream: You MUST use the `getopt_long()` function to process the command line
> arguments passed to the program.  Your program should be able to handle cases where
> the (non-positional) flags are passed IN ANY order.  Make sure that you test the
> program with prefixes of the long option names, as well as the full names.

In addition, add the following new option:

   - `--no-follow-symlinks` as equivalent to `-l`

     This option should only be available if the program was compiled with the `LSTAT`
	 preprocessor variable defined.  It's effect is to cause the program **not** to
     follow symbolic links when they are encountered, as opposed to the default behavior,
	 which is to follow symbolic links.  In addition, the existence of the option will
	 be reported in the `Usage` message.
     If the program was not compiled with `LSTAT` defined, then this option should be illegal
     and it should not be mentioned in the `Usage` message.

    > :nerd: Symbolic links are created using the `ln` (link) command; *e.g.*
	> ```
	> $ ln -s /tmp tempdir
	> ```
    > which will create a symbolic link that will be appear as follows using `ls -l`:
	> ```
	> lrwxrwxrwx 1 gene gene    4 Feb 10 17:15 tempdir -> /tmp
	> ```
	> The name `tempdir` essentially becomes an "alias" for `/tmp`: whenever `tempdir`
	> is encountered in a pathname, the system will replace it by `/tmp`.
    > The default behavior of `vtree` is to descend into directories that are linked
	> in this way.

Finally, arrange for the `-o` (sorted output) option to be available only if the program
has been compiled with the `MEMORY_BASED` symbol defined.  If this symbol has not defined,
then the program should neither accept the `-o` option nor mention it in the "Usage" message.

# Part 3: Testing the Program

For this assignment, you have been provided with a basic set of
Criterion tests to help you debug the program.  We encourage you
to write your own as well as it can help to quickly test inputs to and
outputs from functions in isolation.

In the `tests/base_tests.c` file, there are seven test examples.
You can run these with the following command:

<pre>
    $ bin/vtree_tests
</pre>

To obtain more information about each test run, you can supply the
additional option `--verbose=1`.

The tests have been constructed so that they will point you at most of the
problems with the program.
Each test has one or more assertions to make sure that the code functions
properly.  If there was a problem before an assertion, such as a "segfault",
the test will print the error to the screen and continue to run the
rest of the tests.
Two of the tests use `valgrind` to verify that no memory errors are found.
If errors are found, then you can look at the log file that is left behind by
the test code.
Alternatively, you can better control the information that `valgrind` provides
if you run it manually.

The tests included in the base code are not true "unit tests", because they all
run the program as a black box using `system()`.
You should be able to follow the pattern to construct some additional tests of
your own, and you might find this helpful while working on the program.
For this program, there are a few functions (such as `h_enter` in `hash.c`)
that are amenable to individual testing using true unit tests.
You are encouraged to try to write some of these tests so that you learn how
to do it, and it is possible that some of the tests we use for grading will be
true unit tests like this.  Note that in the next homework assignment unit tests
will likely be very helpful to you and you will be required to write some of your own.
Criterion documentation for writing your own tests can be found
[here](http://criterion.readthedocs.io/en/master/).

  > :scream: Be sure that you test non-default program options to make sure that
  > the program does not crash when they are used.

  > :scream: Make sure that your program compiles and runs correctly with various
  > combinations of the `MEMORY_BASED`, `HSTATS`, and `LSTAT` preprocessor options.

# Hand-in Instructions

Ensure that all files you expect to be on your remote repository are committed
and pushed prior to submission.

This homework's tag is: `hw2`

<pre>
$ git submit hw2
</pre>
