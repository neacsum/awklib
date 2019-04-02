# Ackermann function
function ack(m,n) {
	k = k+1
	if (m == 0) return n+1
	if (n == 0) return ack(m-1, 1)
	return ack(m-1, ack(m, n-1))
}
{ k = 0; print ack($1,$2), "(" k " calls)" }
## Input
0 0
1 1
2 2
3 3
3 4
3 5
## Output
1 (1 calls)
3 (4 calls)
7 (27 calls)
61 (2432 calls)
125 (10307 calls)
253 (42438 calls)
## end
