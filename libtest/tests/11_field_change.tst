#Changing a field changes the whole record
{$(1) = "xxx"; print $1,$0}

##Input
1 Now is the time for all good men to come to the aid of the party.
2 party. Now is the time for all good men to come to the aid of the
3 the party. Now is the time for all good men to come to the aid of
4 of the party. Now is the time for all good men to come to the aid
5 aid of the party. Now is the time for all good men to come to the
6 the aid of the party. Now is the time for all good men to come to
7 to the aid of the party. Now is the time for all good men to come
8 come to the aid of the party. Now is the time for all good men to
9 to come to the aid of the party. Now is the time for all good men
10 men to come to the aid of the party. Now is the time for all good
11 good men to come to the aid of the party. Now is the time for all
12 all good men to come to the aid of the party. Now is the time for
13 for all good men to come to the aid of the party. Now is the time
14 time for all good men to come to the aid of the party. Now is the
15 the time for all good men to come to the aid of the party. Now is
16 is the time for all good men to come to the aid of the party. Now
##Output
xxx xxx Now is the time for all good men to come to the aid of the party.
xxx xxx party. Now is the time for all good men to come to the aid of the
xxx xxx the party. Now is the time for all good men to come to the aid of
xxx xxx of the party. Now is the time for all good men to come to the aid
xxx xxx aid of the party. Now is the time for all good men to come to the
xxx xxx the aid of the party. Now is the time for all good men to come to
xxx xxx to the aid of the party. Now is the time for all good men to come
xxx xxx come to the aid of the party. Now is the time for all good men to
xxx xxx to come to the aid of the party. Now is the time for all good men
xxx xxx men to come to the aid of the party. Now is the time for all good
xxx xxx good men to come to the aid of the party. Now is the time for all
xxx xxx all good men to come to the aid of the party. Now is the time for
xxx xxx for all good men to come to the aid of the party. Now is the time
xxx xxx time for all good men to come to the aid of the party. Now is the
xxx xxx the time for all good men to come to the aid of the party. Now is
xxx xxx is the time for all good men to come to the aid of the party. Now
##END