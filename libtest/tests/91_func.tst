# Function without return
function foo() { i = 0 }
BEGIN { x = foo(); printf "<%s> %d\n", x, x }
##
##
<> 0
##