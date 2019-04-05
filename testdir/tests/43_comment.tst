#comment
       #
BEGIN { x = 1 }
/abc/ { print $0 }
#comment
END { print NR }
#comment

## Input
123
456
abcdef
ghijkl
##Output
abcdef
4
##END

