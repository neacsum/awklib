{	for (i = 1; i <= NF; i++) {
		for (j = 1; j <= NF; j++)
			if (j == 2)
				break;
		print "inner", i, j
	}
	print "outer", i, j
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
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
inner 3 2
outer 4 2
inner 1 2
inner 2 2
inner 3 2
inner 4 2
inner 5 2
outer 6 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
inner 1 2
inner 2 2
outer 3 2
##END