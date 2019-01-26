NF > 0 {
	t = $0
	gsub(/[ \t]+/, "", t)
	n = split($0, y)
	if (n > 0) {
		i = 1
		s = ""
		do {
			s = s $i
		} while (i++ < NF)
	}
	if (s != t)
		print "bad at", NR
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
