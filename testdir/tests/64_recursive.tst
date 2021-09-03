# Factorial function
function fact(n) {
  k++
	if (n == 1) return 1
  return n * fact(n-1)
}
{ k = 0; print fact($1), "(" k " calls)" }
## Input
1
2
3
5
10
## Output
1 (1 calls)
2 (2 calls)
6 (3 calls)
120 (5 calls)
3628800 (10 calls)
## end
