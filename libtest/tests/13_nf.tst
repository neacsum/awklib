# Field counter NF
{print NF,$0}

##Input
The quick brown fox jumps over lazy dog
The quick brown fox jumps over lazy
The quick brown fox jumps over
The quick brown fox jumps
The quick brown fox
The quick brown
The quick
The

##Output
8 The quick brown fox jumps over lazy dog
7 The quick brown fox jumps over lazy
6 The quick brown fox jumps over
5 The quick brown fox jumps
4 The quick brown fox
3 The quick brown
2 The quick
1 The
0 
##END