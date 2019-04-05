{ x[$1] = $1
  delete x[$1]
  n = 0
  for (i in x) n++
  if (n != 0)
	print "error", n, "at", NR
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

