# Local variables
{
  fld = 1
  C = A( $fld, NR )
  print C;
}

function A(par1, par2,   str) {
  str = par2 ":" par1 "+" par1
  return str;
 }
##
line1
line2
##
1:line1+line1
2:line2+line2
##