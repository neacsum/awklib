#What are we testing here?
END {	print i, NR
	if (i < NR)
		print i, NR
}
##Input
Rec
##Output
 1
 1
##END