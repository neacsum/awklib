{ print (substr($2,1,1) > substr($2,2,1)) ? $1 : $2 }
{ x = substr($1, 1, 1); y = substr($1, 2, 1); z = substr($1, 3, 1)
  print (x > y ? (x > z ? x : z) : y > z ? y : z) }

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
17379
7
bwk
6
16116
6
15713
7
11895
8
10409
4
10252
2
9853
9
9748
9
9492
9
9190
9
8912
9
dmr
9
cda
9
bs
8
llc
8
## END