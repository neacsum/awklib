NF > 0 { 
  n = split($0, x)
  if (n != NF)
    printf("split screwed up %d %d\n", n, NF)
  delete x[1]
  k = 0
  for (i in x)
    k++
  if (k != NF-1)
    printf "delete miscount %d elems should be %d at line %d\n", k, NF-1, NR 
}

##Input
Now
Now is
Now is the
Now is the time
Now is the time for
Now is the time for all
Now is the time for all good
Now is the time for all good men
Now is the time for all good men to
Now is the time for all good men to come
Now is the time for all good men to come to
Now is the time for all good men to come to the
Now is the time for all good men to come to the aid
Now is the time for all good men to come to the aid of
Now is the time for all good men to come to the aid of the
Now is the time for all good men to come to the aid of the party.
##
##END
