	{ x[NR] = $0 }

END {
	i = 1
	while (i <= NR) {
		print x[i]
		split (x[i], y)
		usage = y[1]
		name = y[2]
		print "   ", name, usage
		i++
	}
}

##Input
cat scratch
dog bite
bird sing
##Output
cat scratch
    scratch cat
dog bite
    bite dog
bird sing
    sing bird
##END
