	{ x[NR] = $0 }
END {
	for (i = 1; i <= NR; i++) {
		print i, x[i]
		if (x[i] ~ /shen/)
			break
	}
	print "got here"
	print i, x[i]
}
##Input
17379	mel
16693	bwk	me
16116	ken	him	someone else
15713	srb
11895	lem
10409	scj
10252	rhm
 9853	shen
 9748	a68
 9492	sif
 9190	pjw
 8912	nls
 8895	dmr
 8491	cda
 8372	bs
 8252	llc
##Output
1 17379	mel
2 16693	bwk	me
3 16116	ken	him	someone else
4 15713	srb
5 11895	lem
6 10409	scj
7 10252	rhm
8  9853	shen
got here
8  9853	shen
##END
