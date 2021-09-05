function f(n) {
	while (n < 10) {
		print n
		n = n + 1
	}
}
function g(n) {
	print "g", n
}
{ f($1); g($1); print n }

## Input
6
8
9
## Output
6
7
8
9
g 6

8
9
g 8

9
g 9

##