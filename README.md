# Adaptation of Vtree
#### Avik Kadakia and Eugene Stark

# Introduction

In this project I have updated an old piece of
software, making sure it compiles, and that it works properly.

Maintaining old code is a chore and an often hated part of software
engineering. It is definitely one of the aspects which are seldom
discussed or thought about by aspiring computer science students.
However, it is prevalent throughout industry and a worthwhile skill to
learn.  Of course, this project did not give you a remotely
realistic experience in maintaining legacy code or code left behind by
previous engineers but it still provides a small taste of what the
experience may be like.  I took on the role of an engineer
whose supervisor has asked me to correct all the errors in the
program, plus add additional functionality.

## The Existing Program

The goal is extend an old program called `vtree`,
which was posted to Usenet sometime between March and May, 1989.
It was written by Jonathan Bayer of Intelligent Software Products, Inc.,
and it ran under the following operating systems:
4.2BSD, 4.3BSD, System V, System III, SCO Xenix 386, and SCO Xenix 286.

The purpose of the `vtree` program is to traverse a portion of the Unix
filesystem hierarchy, collect information on the number of inodes
(basically, files and directories) and the amount of disk space used,
and to display a representation of the hierarchy in a "visual" format,
possibly with summary statistics.  The functionality is similar to,
but less refined than, the more modern program `tree`, which is installed
on a Linux Mint system.

In the default mode of execution (which occurs when you invoke the program
just using `bin/vtree` with no arguments), the program traverses the filesystem
hierarchy rooted at the current working directory and displays the name of
each file and directory and the number of K-bytes of disk space occupied by
each of them.  Various options can be supplied to modify the behavior of
the program, as discussed in more detail below.

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

  > Since time immemorial, Unix `man` pages have been written in
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
  one of the symbols `BSD`, `SYS_V`, `SYS_III`, `SCO_XENIX`.  I added support
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
Criterion tests Eugene Stark have supplied.
The subdirectory `tests/rsrc` contains data files that are used by the tests.

# Present Command Line Arguments

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

# Part 2: Added Features

The original program uses `getopt()` to do options processing, but it only understands
traditional, single-character options.
For this part of the assignment, I rewrote the options processing so that it uses
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

In addition, I also added the following new option:

   - `--no-follow-symlinks` as equivalent to `-l`


    This option is only be available if the program was compiled with the `LSTAT`
  preprocessor variable defined.  It's effect is to cause the program **not** to
    follow symbolic links when they are encountered, as opposed to the default behavior,
	which is to follow symbolic links.  In addition, the existence of the option will
	be reported in the `Usage` message.
    If the program was not compiled with `LSTAT` defined, then this option is illegal
    and is not be mentioned in the `Usage` message.

Finally, arranged for the `-o` (sorted output) option to be available only if the program
has been compiled with the `MEMORY_BASED` symbol defined.  If this symbol has not defined,
then the program does not accept the `-o` option nor mention it in the "Usage" message.