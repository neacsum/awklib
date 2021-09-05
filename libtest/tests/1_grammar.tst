# This abomination runs through all grammar rules

BEGIN {
a = true &&           # rule 3, 4
      false
b = true ||           # rule 5, 6
      false
for (i=0; i < 3 ; i++) # rule 13, 14
  a = a+1;
for (i=0; ; i++)      #rule 15, 16
  if (i == 3)
    break
  else
    v[i] = 1
for (t in v) print t, t+1, t-1, t*t, t^2, t**2, t/2, t%3
 i++; i--;
 g()
 t += ++u;
 
 t -= --u;;
 ;
 t *= +2
 t /= -2
 t ^= 2
 t %= 2
 t = (!/abc/)
 t = (2 >=3)? (!/abc/) : /abc/
 if (t in v)
   u = t ~ /abc/
 if ((1,2) in v)
  print "a" "b"
 else
   print
 t = /ab/ ~ (/ab/ && /cd/)
 atan2(sin(rand()),rand)
 index(/ab/, "abc")
 index(/cd/, /cd/ && /ef/)
 match(/ab/, /cd/)
 match(/ab/, /cd/ && /ef/)
 split(/ab/, v, /cd/)
 split(/ab/, v, /cd/ && /ef/)
 split (/ab/, arr)
 for (;;)
   break;
}

/Rec/ { gsub (/Record_/, "the record ") }
/1/, /3/ {print $0, (1<2)?"1-3":"4-5", $u}
/4/, /5/
{print ("Any record", $(1-1))}

/nada/

5 <= 6  {print /ab/ || (t in v) && a ~ (/abc/ && /def/), (1, 2,3) in v}

4 != 4 {
  print $1 t in v ~ /ab/
  " " | getline
  " " | getline s
  getline a < getline
  getline < getline a
  
  next;nextfile;
  delete v[1]
  delete v
  exit
  exit 2
  print a=b | "cmd"
  print !3 > "out"
  print 4 >> "out"
};;

;

function f(a, 
  b, c)
{
  sprintf ("%s%s%s", a, b, c)
  return a b c;
}

function g () {
u = 3
s = "a=b=c="
s1 = sub (/=/, "_", s)
s2 = gsub ((/=/), "_")
s3 = sub ((/=/), "_", s)
substr (s, 1, 3)
substr (s, 1)
return
}
END {
  i = 5; 
  do
    i--;
  while (i>3)
  while (i>=0) {
    NF /= 2;
    i--
    continue;
  }
 printf f("a", "b", "c");
 close (t)
}
## Input
Record 1
Record 2
Record 3
Record 4
## Output
2 3 1 4 4 4 1 2
0 1 -1 0 0 0 0 0
1 2 0 1 1 1 0.5 1

Record 1 1-3 Record 1
Any record Record 1
0 0
Record 2 1-3 Record 2
Any record Record 2
0 0
Record 3 1-3 Record 3
Any record Record 3
0 0
Record 4
Any record Record 4
0 0
abc
