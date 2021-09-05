{
for (i = 1; i <= NF; i++) {
	if ($i ~ /^[0-9]+$/)
		continue;
	print $i, " is non-numeric"
	next
}
print $0, "is all numeric"
}
## Input
abc 456 789
123 def 789
123 456 ghi
123 456 789
## Output
abc  is non-numeric
def  is non-numeric
ghi  is non-numeric
123 456 789 is all numeric
##
