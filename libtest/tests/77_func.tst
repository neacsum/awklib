function f(a, n) {
  for (i=1; i <= n; i++)
    print "  " a[i]
}

{  print
  n = split($0, x)
  f(x, n)
}
##
Now is the time for all good men
##
Now is the time for all good men
  Now
  is
  the
  time
  for
  all
  good
  men
##