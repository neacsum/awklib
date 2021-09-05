{ if($1>1000) print $1,"yes"
  else print $1,"no"
}
##Input
1	mel
2000	bwk	me
3000	ken	him	someone else
2	srb
##Output
1 no
2000 yes
3000 yes
2 no
## END
