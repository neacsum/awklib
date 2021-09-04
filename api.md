# AWK Library API 

The API is structured around an opaque `AWKINTERP` object that represents the
AWK language interpreter. The life cycle of this object follows a series of
irreversible state transitions: initialization, program compilation, program
execution and destruction.

## Functions

### awk_init
Initializes a new AWK interpreter object.

#### Prototype:

`AWKINTERP* awk_init (const char **vars);`

#### Parameters:

`vars`  - array of variable definitions with the same format as the -v command-line 
arguments of stand-alone AWK interpreter. The array is terminated with a NULL string.

#### Return:
A pointer to an AWK interpreter object or NULL if an error occurs.

#### Example
````C
    char **v = {"FS=:", "OFS=|", NULL};
    AWKINTERP *pi = awk_init (v);
````

### awk_setprog
Set the program text for an interpreter.

#### Prototype:
`int awk_setprog (AWKINTERP* pi, const char *prog);`

#### Parameters:
  `pi` - pointer to an interpreter object  
  `prog` - pointer to AWK language program
 
#### Return:
1 if successful, 0 otherwise.

#### Example:
````C
    AWKINTERP *pi = awk_init (NULL);
    int ok = awk_setprog (pi, "{print NR, $0}");
````

### awk_addprogfile
Add the content of a file as AWK program. The functionality is equivalent with
the `-f` switch on the command line of stand-alone interpreter. Just like the
`-f` switch, this function can be called repeatedly to add multiple programs.

#### Prototype:
`int awk_addprogfile (AWKINTERP* pi, const char *progfile);`

#### Parameters:
`pi` - pointer to an interpreter object  
`progfile` - filename for AWK language program

#### Return:
1 if successful, 0 otherwise.

#### Example
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_addprogfile (pi, "prog1.awk");
````
### awk_compile
Compiles the AWK language program(s) that have been specified using the
[awk_setprog](#awk_setprog) or [awk_addprogfile](#awk_addprogfile) functions.

#### Prototype:
`int awk_compile (AWKINTERP* pi);`

#### Parameters:
`pi` - pointer to an interpreter object

#### Return:
1 if successful, 0 otherwise.

#### Example:
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "{print NR, $0}");
    awk_compile (pi);
````
### awk_addarg
Add a new argument to the interpreter. The argument can be an input file name or
a variable definition if it has the syntax `var=value`.

#### Prototype:
`int awk_addarg (AWKINTERP* pi, const char *arg);`

#### Parameters:
`pi` - pointer to an interpreter object  
`arg` - program argument

#### Return:
1 if successful, 0 otherwise.

#### Example
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "{print pass+1 \"-\" NR, $0}");
    awk_compile (pi);
    awk_addarg (pi, "infile.txt");
    awk_addarg (pi, "pass=1");
    awk_addarg (pi, "infile.txt");
````

### awk_exec
Execute a compiled program.

#### Prototype:
`int awk_exec (AWKINTERP* pi);`

#### Parameters:
`pi` - pointer to an interpreter object

#### Return:
The value returned by exit statement or a negative error code if something went
wrong.

If a program terminates without an exit statement, the returned value is 0. 
Otherwise the function returns the value specified in the exit statement.
Small negative values should be considered reserved for error conditions.

#### Example:
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "{print NR, $0}");
    awk_compile (pi);
    awk_addarg (pi, "infile.txt");
    awk_exec (pi);
````

### awk_run
Compile and execute a program.

#### Prototype:
`int awk_run (AWKINTERP* pi, const char *progfile);`

#### Parameters:
`pi` - pointer to an interpreter object
`prog` - pointer AWK program

#### Return
The value returned by exit statement or a negative error code if something went
wrong.

This function combines in one call the calls to `awk_setprog`, `awk_compile`
and `awk_exec` functions.

If a program terminates without an exit statement, the returned value is 0. 
Otherwise the function returns the value specified in the exit statement.
Small negative values should be considered reserved for error conditions.

#### Example:
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_run (pi, "{print NR, $0}");
````

### awk_end
Releases all memory allocated by the interpreter object. 

#### Prototype:
`void awk_end (AWKINTERP* pi);`

#### Parameters:
`pi` - pointer to an interpreter object

#### Example
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "{print NR, $0}");
    awk_compile (pi);
    awk_addarg (pi, "infile.txt");
    awk_exec (pi);
    awk_end (pi);
````
### awk_setinput
Forces interpreter to read input from a file.

#### Prototype:
`int awk_setinput (AWKINTERP* pi, const char *fname);`

#### Parameters:
`pi` - pointer to an interpreter object  
`fname` - filename to be used for input

#### Return:
1 if successful, 0 otherwise.

#### Example:
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "{print NR, $0}");
    awk_compile (pi);
    awk_setinput (pi, "infile.txt");
    awk_exec (pi);
````

### awk_infunc
Change the input function with a user-defined function. 

#### Prototype:
`void awk_infunc (AWKINTERP* pi, inproc fn);`

#### Parameters:
`pi` - pointer to an interpreter object  
`fn` - pointer to input redirection function

#### Example
````C
    std::istrstream instr{
        "Record 1\n"
        "Record 2\n"
    };

    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "{print NR, $0}");
    awk_compile (pi);
    awk_infunc (pi, []()->int {return instr.get (); });
````

### awk_setoutput
Redirect interpreter output to a file.

#### Prototype:
`int awk_setoutput (AWKINTERP* pi, const char *fname);`

#### Parameters:
`pi` - pointer to an interpreter object  
`fname` - filename to be used for output

#### Return:
1 if successful, 0 otherwise.

#### Example:
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "BEGIN {print \"Output redirected\"}");
    awk_compile (pi);
    awk_setoutput (pi, "results.txt");
    awk_exec (pi);
````

### awk_redirect
Redirect one of the interpreters' standard files (stdin, stdout or stderr) to a file.

#### Prototype:
`int awk_redirect (AWKINTERP* pi, int n, const char *fname);`

#### Parameters:
`pi` - pointer to an interpreter object
`n` - file to redirect (0 - stdin, 1 - stdout, 2 - stderr)
`fname` - filename to be used for output

#### Return:
1 if successful, 0 otherwise.

#### Example:
````C
    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "BEGIN {print \"Error redirected\" > \"/dev/stderr\"}");
    awk_compile (pi);
    awk_redirect (pi, 2, "errors.txt");
    awk_exec (pi);
````

If `fname` is "-" (a single dash character), the redirection is terminated and
file goes back to original destination (stdin, stdout or stderr).

### awk_outfunc
Change the output function with a user-defined function. 

#### Prototype:
`void awk_outfunc (AWKINTERP* pi, outproc fn);`

#### Parameters:
`pi` - pointer to an interpreter object  
`fn` - pointer to output redirection function

#### Example
````C
    std::ostringstream out;
    int strout (const char *buf, size_t sz)
    {
        out.write (buf, sz);
        return out.bad ()? - 1 : 1;
    }
...
    AWKINTERP *pi = awk_init (NULL);
    awk_setprog (pi, "BEGIN {print \"Output redirected\"}");
    awk_compile (pi);
    awk_outfunc (pi, strout);
````

### awk_getvar
Retrieves the value of an AWK variable.

#### Prototype:
`int awk_getvar (AWKINTERP *pi, awksymb* var);`

#### Parameters:
`pi` - pointer to an interpreter object  
`var` - pointer to an [awksymb] structure that receives the value

#### Return:
1 if successful or a negative error code otherwise.

Caller has to specify the name of the variable and the function returns the
value of the variable. The `flags` member of the structure is set to indicate
the type of information available.

If the variable is an array and the `index` member is NULL, the function
returns `AWK_ERR_ARRAY` error code. 

For string variables, the `AWKSYMB_STR` flag is set and the function allocates
the memory needed for the string by calling `malloc`. The user has to release
the memory by calling `free`. 

#### Example
````C
    AWKINTERP *pi = awk_init (NULL);
    awksymb var{ "NR" };

    awk_setprog (pi, "{print NR, $0}\n");
    awk_compile (pi);
    awk_getvar (pi, &var);
````

### awk_setvar
Changes the value of an AWK variable.

#### Prototype:
`int awk_setvar (AWKINTERP *pi, awksymb* var);`

#### Parameters:
`pi`  -pointer to an interpreter object  
`var`  -pointer to an [awksymb] structure with information about the variable

#### Return:
1 if successful or a negative error code otherwise.

The user has to set the `flags` member of the [awksymb] structure to indicate
which values are valid (string or numerical). In addition, for array members
the user has to specify the index and set the `AWKSYMB_ARR flag.

If the variable does not exist, it is created.

#### Example
````C
    AWKINTERP *pi = awk_init (NULL);
    awksymb v{ "myvar", NULL, AWKSYMB_NUM, 25.0 };
    awk_setprog (pi, "{myvar++; print myvar}\n");
    awk_compile (interp);

    awk_compile (pi);
    awk_setvar (pi, &v);
    awk_exec (pi);  //output is "26"
````

### awk_addfunc
Add a user defined function to the interpreter.

#### Prototype:
`int awk_addfunc (AWKINTERP *pi, const char *name, awkfunc fn, int nargs);`

#### Parameters:
`pi` - pointer to an interpreter object  
`name` - function name  
`fn` - pointer to function. See [awkfunc] for prototype.  
`nargs` - number of function arguments

#### Return:
1 if successful or a negative error code otherwise.

External user-defined functions can be called from AWK code just like any AWK
user-defined function. The `nargs` parameter specifies the expected
number of parameters but, like with any AWK function, the number of actual
arguments can be different. The interpreter will provide null values for any
missing parameters.

The function can return a value by setting it into the `ret` variable and setting
the appropriate flags. String values must be allocated using `malloc`.

#### Example
````C
    void fact (AWKINTERP *pi, awksymb* ret, int nargs, awksymb* args)
    {
      int prod = 1;
      for (int i = 2; i <= args[0].fval; i++)
        prod *= i;
      ret->fval = prod;
      ret->flags = AWKSYMB_NUM;
    }
...
    awk_setprog (pi, " BEGIN {n = factorial(3); print n}");
    awk_compile (pi);
    awk_addfunc (pi, "factorial", fact, 1);
    awk_exec (pi);
````

### awk_err
Return last error code and message.

#### Prototype:
int awk_err (AWKINTERP *pi, const char **msg);

#### Parameters:
`msg`   - address of a pointer to the last error message.

#### Return
Last error code.

## Data Types and Structures

### awksymb
Structure used to set or retrieve a variable from an interpreter. Also used
to pass parameters to an AWK callable user function.

#### Definition:
````C
struct awksymb {
  const char *name;     //variable name
  const char *index;    //array index
  unsigned int flags;   //variable type flags
  double fval;          //numerical value
  char *sval;           //string value
};
````
#### Members:
`name` - name of the referenced AWK variable  
`index` - array index string when referencing an array element  
`flags` - combination of flags indicating data type:
  * AWKSYMB_NUM - `fval` member of the data structure is valid
  * AWKSYMB_STR - `sval` member of the data structure is valid
  * AWKSYMB_ARR - the variable is an array and `index` member of the data structure
                  is the array index.

`fval` - numerical value  
`sval` - string value

The current record and fields are accessed with variables named $0, $1, etc.

### inproc
This is a pointer to a user-defined function returning an input character.
It is used by the [awk_infunc] API function.

#### Prototype:
`typedef int (*inproc)();`

#### Return:
Next input character or EOF at the end.

### outproc
This is a pointer to a user-defined function used to write

#### Prototype:
`typedef int (*outproc)(const char *buf, size_t len);`

#### Parameters:
`buf` - characters to output
`len` - number of characters to output

#### Return
Nonnegative if successful, negative otherwise.

### awkfunc
A user defined function callable from the interpreter.

#### Prototype:
`typedef void (*awkfunc)(AWKINTERP *pi, awksymb* ret, int nargs, awksymb* args);`

#### Prameters:
`pi`  - pointer to an interpreter object  
`ret` - pointer to the return value  
`nargs` - number of arguments  
`args` - array of arguments
