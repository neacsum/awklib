# Embedded AWK Interpreter

Copyright (c) 2019-2021 Mircea Neacsu

This library is an AWK interpreter that can be embedded in C/C++
programs. The code is based on the [One True AWK](https://github.com/onetrueawk/awk)
code.

## Description ##
The library allows you to build and run an AWK interpreter in your C/C++
program using only a few function calls. The AWK code can call external functions
provided by your C program and, conversely, the C code can access AWK variables.

Here is an example of a simple word counting application:
````C++
#include <awklib.h>

int main (int argc, char **argv)
{
  AWKINTERP* interp = awk_init (NULL);
  awk_setprog (interp, R"(
    {wc += NF; bc += length($0)}\
    END {print NR, wc, bc, ARGV[1]})";
  awk_compile (interp);
  if (argc > 1)
    awk_addarg (interp, argv[1]);
  awk_exec (interp);
  awk_end (interp);
}
````
The program produces an output similar to the 'wc' utility.

See the [API document](api.md) for details.

## Contents ##
- __awklib__    - the AWK library
- __libtest__   - library test suite
- __testdir__   - tests for stand-alone interpreter (and by extension for the AWK library)
- __samples__   - various sample applications:
  - __wc__      - Word counting application shown above
  - __interp__  - Stand-alone AWK interpreter using the AWK library

## Building ##
You need a YACC or bison compiler to compile the AWK grammar. The library itself
doesn't have any other dependencies.

Edit and run the _makelinks.bat_ file to create the required symbolic links
before building the project.

The library test suite requires the
[UTPP test framework](https://bitbucket.org/neacsum/utpp) 

## Installation ##
All projects have been tested under Visual Studio 2019.

## Contributing ##
Contributors are welcome. Currently version 2 is in the making with important enhancements.

For style rules please consult my [style guide](https://gist.github.com/neacsum/2abf84e818cf3fe06fe73a7640bf4703).

## Development Notes ##
### Version 1 ###
When doing this project I had the definite sensation of touching a piece of
computer science history. Just the AWK name invokes, in my mind, revered giants
of this field (my first book on compilers was Aho and Ullman 'green dragon book').

As such, I've tried to touch the code as little as possible, choosing only to wrap
existing code in the high level library functions. Some areas however had to be
changed more extensively.

Function call mechanism has been almost completely rewritten. It needed to be
enhanced to allow for external function calls. At the same time I dissolved
the function call stack into the C program stack.

In many cases allocated memory was not released. The original code was
for a stand-alone program that has the benefit of an operating system doing
cleanup when the program finishes. Meanwhile an embedded interpreter
can be invoked many times so allocated memory has to be released.

I would have liked to maintain the project as pure C code. Unfortunately error
handling restrictions forced me to move to C++. In a stand-alone program,
exiting abruptly when an error condition is encountered is perfectly OK.
When this is converted to a function call, the function has to return (with an
error code indication). The simplest solution that preserved most of the
original code was to wrap the code in try-catch blocks.

### Version 2 ###
After spending more time with this project, I discovered a number of inconveniences derived from the original purpose of the code base (it was a standalone program) or from its age. In this version I gave up on any pretense that this is anything but C++ code. Here is a list of what has been addressed so far in version 2:
- Inability to reuse a compiled AWK script with different input data.
- Lack of thread safety. This is still somewhat of a work in progress. Currently the the whole API is protected by a `awk_in_use` mutex. Future versions should have a more fine grained mechanism.
- Regular expression machinery (in b.cpp file) was replaced with `std::regex` objects 
- Most memory management moved to `new/delete` operators from `malloc/free`.
- True multidimensional arrays similar to [arrays of arrays](https://www.gnu.org/software/gawk/manual/html_node/Arrays-of-Arrays.html) in *gawk*. I didn't really _need_ those but the change in grammar, required to implement them, was so small that I couldn't resist :)
- Much expanded testing (see `libtest` project) infrastructure. Apart from API tests, most tests (over 100 of them at last count) were taken from the One True AWK project.

The API has remained largely unchanged. Only the `awk_err` function takes an additional pointer to interpreter parameters. 

