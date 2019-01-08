$2 ~ /^[a-l]/	{ x["a"] = x["a"] + 1 }
$2 ~ /^[m-z]/	{ x["m"] = x["m"] + 1 }
$2 !~ /^[a-z]/	{ x["other"] = x["other"] + 1 }
END { print NR, x["a"], x["m"], x["other"] }

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
 7450	mb
 7360	ava
 7273	jrv
 7080	bin
 7063	greg
 6567	dict
 6462	lck
 6291	rje
 6211	lwf
 5671	dave
 5373	jhc
 5220	agf
 5167	doug
 5007	valerie
 3963	jca
 3895	bbs
    1	122sec
 3796	moh
##Output
34 21 12 1
##END