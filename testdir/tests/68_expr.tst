# A line with just a '#' separates input and output for different sub-tests

BEGIN {test = 1}
END {print "Last test ", test >"/dev/stderr"}
$1 == "#" {test++; print "#"; next}

test == 1 { print ($1 == 1) ? "yes" : "no"}
test == 2 && $1 > 0
test == 3 {print NF}
test == 4 { print NF, $NF }
test == 5 { i=1; print ($++$++i) } # this horror prints $($2+1)
test == 6 { x = $1++++$2; print $1, x } # concatenate $1 and ++$2; print new $1 and concatenated value
test == 7 { print ($1~/abc/ !$2) }
test == 8 { print $1 ~ $2 }
test == 9 { print $1 || $2 }
test == 10 { print $1 && $2 }
test == 11 { $1 = $2; $1 = $1; print $1 }
test == 12 { f = 1; $f++; print f, $f } # $f++ => ($f)++
test == 13 { g[1]=1; g[2]=2; print $g[1], $g[1]++, $g[2], g[1], g[2] }
test == 14 { print $1, +$1, -$1, - -$1 }
test == 15 { printf("a%*sb\n", $1, $2) }
test == 16 { printf("a%-*sb\n", $1, $2) }
test == 17 { printf("a%*.*sb\n", $1, $2, "hello") }
test == 18 { printf("a%-*.*sb\n", $1, $2, "hello") }
test == 19 { printf("%d %ld\n", $1, $1) }
test == 20 { printf("%x %lx\n", $1, $1) }
##Input
1
1.0
1E0
0.1E1
10E-1
01
10
10E-2
# test 2
1
2
0
-1
1e0
0e1
-2e64
3.1e4
# test 3

x
x y
# test 4

x
x y
x yy zzz
# test 5
1
1 2 3
abc
# test 6
1 3
# test 7
0 0
0 1
abc 0
xabcd 1
# test 8
a \141
a \142
a \x61
a \x061
a \x62
0 \060
0 \60
0 \0060
Z \x5a
Z \x5A
# test 9

1
0 0
1 0
0 1
1 1
a b
# test 10

1
0 0
1 0
0 1
1 1
a b
# test 11
abc def
abc def	ghi
# test 12
11 22 33
# test 13
111 222 333
# test 14
1
-1
0
x
# test 15
1 x
2 x
3 x
# test 16
1 x
2 x
3 x
# test 17
1 1
2 1
3 1
# test 18
1 1
2 1
3 1
# test 19
1
10
10000
# test 20
1
10
10000
##Output
yes
yes
yes
yes
yes
yes
no
no
#
1
2
1e0
3.1e4
#
0
1
2
#
0 
1 x
2 y
3 zzz
#
1
3
abc
#
2 14
#
01
00
11
10
#
1
0
1
0
0
1
1
0
1
1
#
0
1
0
1
1
1
1
#
0
0
0
0
0
1
1
#
def
def
#
1 12
#
111 111 222 2 2
#
1 1 -1 1
-1 -1 1 -1
0 0 0 0
x 0 0 0
#
axb
a xb
a  xb
#
axb
ax b
ax  b
#
ahb
a hb
a  hb
#
ahb
ah b
ah  b
#
1 1
10 10
10000 10000
#
1 1
a a
2710 2710
##