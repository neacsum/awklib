# Function with argument
function f(a) { print "hello"; return a }
{ print "<" f($1) ">" }
## Input
rec1
rec2
rec3
## Output
hello
<rec1>
hello
<rec2>
hello
<rec3>
##