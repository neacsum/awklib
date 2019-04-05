BEGIN {
	print "this is before calling myabort"
	myabort(1)
	print "this is after calling myabort"
} 
function myabort(n) {
	print "in myabort - before exit", n
	exit 2
	print "in myabort - after exit"
}
END {
	print "into END"
	myabort(2)
	print "should not see this"
}
##Input
##Output
this is before calling myabort
in myabort - before exit 1
into END
in myabort - before exit 2
## end
