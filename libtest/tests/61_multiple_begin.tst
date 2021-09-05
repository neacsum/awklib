BEGIN {print "Begin 1"}
{print $0}
BEGIN {print "Begin 2"}

## Input
17379	mel
16693	bwk	me
16116	ken	him	someone else
15713	srb
11895	lem
10409	scj
10252	rhm
## Output
Begin 1
Begin 2
17379	mel
16693	bwk	me
16116	ken	him	someone else
15713	srb
11895	lem
10409	scj
10252	rhm
