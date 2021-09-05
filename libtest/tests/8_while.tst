#while loop and different number formats
{
x = $1
print x
while (x > 1) {
	x = x / 10
	print x
}
print "=="
}
##Input
123
234e2
425.6e-1
567.123
678abcd
789efgf
-89.0
##Output
123
12.3
1.23
0.123
==
234e2
2340
234
23.4
2.34
0.234
==
425.6e-1
4.256
0.4256
==
567.123
56.7123
5.67123
0.567123
==
678abcd
67.8
6.78
0.678
==
789efgf
78.9
7.89
0.789
==
-89.0
==
##END