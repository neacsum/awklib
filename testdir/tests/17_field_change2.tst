{$1=$2; print}

# this should print nothing for an empty input line
# since it has only referred to $2, not created it,
# and thus only $1 exists (and it's null).

# is this right???
# Mircea - no it is not. It prints a blank line

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
 3796	moh
 3481	xchar
 3200	tbl
 2845	s
 2774	tgs
 2641	met
 2566	jck
 2511	port
 2479	sue
 2127	root
 1989	bsb
 1989	jeg
 1933	eag
##Output
mel mel

bwk bwk me
ken ken him someone else
srb srb
lem lem
scj scj
rhm rhm
shen shen
a68 a68
sif sif
pjw pjw
nls nls
dmr dmr
cda cda
bs bs
llc llc
mb mb
ava ava
jrv jrv
bin bin
greg greg
dict dict
lck lck
rje rje
lwf lwf
dave dave
jhc jhc
agf agf
doug doug
valerie valerie
jca jca
bbs bbs
moh moh
xchar xchar
tbl tbl
s s
tgs tgs
met met
jck jck
port port
sue sue
root root
bsb bsb
jeg jeg
eag eag
##END