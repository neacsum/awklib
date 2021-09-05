$0 ~ /\=AB/ {print "pat1", $0}
$0 ~ /= AB/ {print "pat2", $0}

## Input
=ABBA
==AB=BA
AB=CD
= ABBA
## Output
pat1 =ABBA
pat1 ==AB=BA
pat2 = ABBA
##END