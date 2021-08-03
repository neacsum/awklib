# Per GAWK manual:
# Inside a BEGIN rule, the value of FILENAME is "", because there are no input
# files being processed yet.
# https://www.gnu.org/software/gawk/manual/gawk.html#Auto_002dset

BEGIN { print FILENAME }
END { print NR }
##Input
1
2
3
##Output

3
##END