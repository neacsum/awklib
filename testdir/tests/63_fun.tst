# function call
function g() { return "{" f() "}" }
function f() { return $1 }
 { print "<" g() ">" }
##Input
/dev/rrp3:

17379	mel
16693	bwk	me
16116	ken	him	someone else
15713	srb
11895	lem
10409	scj
10252	rhm
 9853	shen
 9748	a68
## Output
<{/dev/rrp3:}>
<{}>
<{17379}>
<{16693}>
<{16116}>
<{15713}>
<{11895}>
<{10409}>
<{10252}>
<{9853}>
<{9748}>
