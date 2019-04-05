{$0 = $1; print; print NF, $0; print $2}
{$(0) = $1; print; print NF, $0; print $2}
{ i = 1; $(i) = $i+1; print }
##
The quick brown fox jumps over lazy dog
Now is the time for all good men
##
The
1 The

The
1 The

1
Now
1 Now

Now
1 Now

1
