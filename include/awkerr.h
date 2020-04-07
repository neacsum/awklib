/*!
  \file awkerr.h
  \brief Error codes from embedded AWK library

  (c) Mircea Neacsu 2019
  See README file for full copyright information.
*/
#pragma once

//AWK error codes
#define AWK_ERR_NOMEM       -1  //out of memory
#define AWK_ERR_NOPROG      -2  //no program given
#define AWK_ERR_INPROG      -3  //cannot open program file
#define AWK_ERR_INVVAR      -4  //invalid variable argument
#define AWK_ERR_BADSTAT     -5  //invalid interpreter status
#define AWK_ERR_PROGFILE    -6  //program file or string already set
#define AWK_ERR_SYNTAX      -7  //syntax error
#define AWK_ERR_INFILE      -8  //cannot open input file
#define AWK_ERR_LIMIT       -9  //limit exceeded
#define AWK_ERR_ARG         -10 //invalid argument
#define AWK_ERR_FPE         -11 //floating point error
#define AWK_ERR_OTHER       -12 //other errors
#define AWK_ERR_RUNTIME     -13 //runtime error
#define AWK_ERR_OUTFILE     -14 //output file error
#define AWK_ERR_PIPE        -15 //pipe error
#define AWK_ERR_NOVAR       -16 //variable not found
#define AWK_ERR_ARRAY       -17 //variable is an array
#define AWK_ERR_INVTYPE     -18 //invalid variable type
