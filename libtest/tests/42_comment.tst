# this is a comment line
# so is this
/#/	{ print "this one has a # in it: " $0	# comment
	print "again:" $0
	}
##Input
alpha
#beta
gamma
## Output
this one has a # in it: #beta
again:#beta
##END