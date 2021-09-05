{ split("1 1.2 abc", x)
  x[$1]++
  delete x[1]
  delete x[1.2]
  delete x["abc"]
  delete x[$1]
  delete x[0]
}
##Input
3
4
5
##Output
##END
