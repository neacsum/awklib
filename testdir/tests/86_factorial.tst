# Factorial function
function f(n) {
  if (n <= 1)
    return 1
  else
    return n * f(n-1)
}
{ print f($1) }
##
0
1
2
3
4
5
6
7
8
9
##
1
1
2
6
24
120
720
5040
40320
362880
