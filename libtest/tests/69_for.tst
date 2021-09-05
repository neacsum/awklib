{
	for (i=1;;i++) {
		if (i > NF)
			next
		print i, $i
	}
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
##Output
1 Now
1 Now
2 is
1 Now
2 is
3 the
1 Now
2 is
3 the
4 time
1 Now
2 is
3 the
4 time
5 for
1 Now
2 is
3 the
4 time
5 for
6 all
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
9 to
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
9 to
10 come
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
9 to
10 come
11 to
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
9 to
10 come
11 to
12 the
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
9 to
10 come
11 to
12 the
13 aid
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
9 to
10 come
11 to
12 the
13 aid
14 of
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
9 to
10 come
11 to
12 the
13 aid
14 of
15 the
1 Now
2 is
3 the
4 time
5 for
6 all
7 good
8 men
9 to
10 come
11 to
12 the
13 aid
14 of
15 the
16 party.
##
