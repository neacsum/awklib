{print $2 " " $1}
{print $1 " " "is", $2}
{print $2 FS "is" FS $1}
{print length($1 $2), length($1) + length($2)}
## Input
cat green
dog blue
bird yellow
## Output
green cat
cat is green
green is cat
8 8
blue dog
dog is blue
blue is dog
7 7
yellow bird
bird is yellow
yellow is bird
10 10
##END