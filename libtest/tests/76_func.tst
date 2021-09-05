function f(n) { while ((n /= 10) > 1)  print n }
function g(n) { print "g", n }
{ f($1); g($1) }
##
4000
300
20
1
##
400
40
4
g 4000
30
3
g 300
2
g 20
g 1
##