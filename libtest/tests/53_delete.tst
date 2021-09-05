NR < 50 { 
  n = split($0, x)
  for (i = 1; i <= n; i++)
    for (j = 1; j <= n; j++)
      y[i,j] = n * i + j
      
  for (i = 1; i <= n; i++)
    delete y[i,i]
  k = 0
  for (i in y)
    k++
  if (k != int(n^2-n))
    printf "delete2 miscount %d vs %d at %d\n", k, n^2-n, NR
}
## Input
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
