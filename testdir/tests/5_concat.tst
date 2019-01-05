#string concatenation
{i="count" $1 $2; print i , $0}

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
count17379mel 17379	mel
count16693bwk 16693	bwk	me
count16116ken 16116	ken	him	someone else
count15713srb 15713	srb
count11895lem 11895	lem
count10409scj 10409	scj
count10252rhm 10252	rhm
count9853shen  9853	shen
count9748a68  9748	a68
count9492sif  9492	sif
count9190pjw  9190	pjw
count8912nls  8912	nls
count8895dmr  8895	dmr
count8491cda  8491	cda
count8372bs  8372	bs
count8252llc  8252	llc
##END