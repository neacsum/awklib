# this program fails if awk is created without separate I&D
# prints garbage if no $3
# Mircea - No, it doesn't. It prints an empty field which is quite sensible

{ print $1, $3 }

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
16693 me
16116 him
15713 
11895 
10409 
10252 
9853 
9748 
9492 
9190 
8912 
8895 
8491 
8372 
8252 
##END