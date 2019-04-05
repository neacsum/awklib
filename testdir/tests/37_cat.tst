{print x $0}	# should precede by zero. Mircea - no, it shouldn't!
# x is an empty string. To force conversion to number read the manual or see below.
{print 0+x $0}
##Input
alpha
beta
gamma
delta
##Output
alpha
0alpha
beta
0beta
gamma
0gamma
delta
0delta
##END