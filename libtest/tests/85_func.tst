# this exercises multiple free of temp cells
BEGIN   { eprocess("eqn", "x", contig) 
    process("tbl" )
    eprocess("eqn" "2", "x", contig) 
  }
function eprocess(file, first, contig) {
  print file
}
function process(file) {
  close(file)
}
##
##
eqn
eqn2
##