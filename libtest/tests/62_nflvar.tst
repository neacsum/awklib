# Test if NF can be used on left side of assignment
  {
    print "NF=",NF
    NF += 2
    print "New NF=", NF
    $0 = "rec 1 modified"
    print "Modified NF=", NF 
  }
  
##
rec 1
##
NF= 2
New NF= 4
Modified NF= 3
##END
 