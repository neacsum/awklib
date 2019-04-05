#tests whether function returns sensible type bits

function assert(cond) { # assertion
    if (cond) print 1; else print 0
}

function i(x) { return x }

{ m=$1; n=i($2); assert(m>n) }
##
10 2
2 10
10 10
10 1e1
1e1 9
##
1
0
0
0
1
##