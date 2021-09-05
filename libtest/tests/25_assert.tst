# tests whether function returns sensible type bits

function assert(cond) { # assertion
    if (!cond) print "   >>> assert failed <<<"
}

function i(x) { return x }

{ print; m = length($1); n = length($2); n = i(n); assert(m > n) }
##Input
dog bite
bite dog
##Output
dog bite
   >>> assert failed <<<
bite dog
##END