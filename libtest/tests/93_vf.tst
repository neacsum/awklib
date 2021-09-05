BEGIN { i = 1 }
{print $(i+i); i++}
{print $(0)}
##
The quick brown fox jumps over the lazy dog
The quick brown fox jumps over the lazy dog
The quick brown fox jumps over the lazy dog
The quick brown fox jumps over the lazy dog
##
quick
The quick brown fox jumps over the lazy dog
fox
The quick brown fox jumps over the lazy dog
over
The quick brown fox jumps over the lazy dog
lazy
The quick brown fox jumps over the lazy dog
##
