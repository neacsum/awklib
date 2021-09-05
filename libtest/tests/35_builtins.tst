#Some builtin math functions
/^[0-9]/ { print $1,
	length($1),
	log($1),
	sqrt($1),
	int(sqrt($1)),
	exp($1 % 10) }
##Input
16
25
100
30
4
##Output
16 2 2.77259 4 4 403.429
25 2 3.21888 5 5 148.413
100 3 4.60517 10 10 1
30 2 3.4012 5.47723 5 1
4 1 1.38629 2 2 54.5982
##END