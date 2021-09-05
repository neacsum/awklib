# Array access

	{if (amount[$2] "" == "") item[++num] = $2;
	 amount[$2] += $1
	}
END	{for (i=1; i<=num; i++)
		print item[i], amount[item[i]]
	}
##Input
12.5 carrots
3.2 apples
1.2 oranges
10.2 carrots again
##Output
carrots 22.7
apples 3.2
oranges 1.2
##END