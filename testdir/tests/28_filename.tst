# some question of what FILENAME ought to be before execution.
# current belief:  "-", or name of first file argument.
# this may not be sensible.

# Mircea - current observed behaviour: FILENAME is empty.
# Seems a sesnsible behaviour to me.

BEGIN { print FILENAME }
END { print NR }
##Input
1
2
3
##Output

3
##END