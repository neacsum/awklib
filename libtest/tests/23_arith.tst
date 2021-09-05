
{ print NR, $1, $1+$1, $1-$1, $1 * $1 }
{ print NR, $1/NR, $1 % NR }
# { print NR, $1++, $1--, --$1, $1-- }  # this depends on order of eval of args!
{ print NR, -$1 }
$1 > 0 { print NR, $1 ^ 0.5 }
$1 > 0 { print NR, $1 ** 0.5 }
##Input
16
25
##Output
1 16 32 0 256
1 16 0
1 -16
1 4
1 4
2 25 50 0 625
2 12.5 1
2 -25
2 5
2 5
##END