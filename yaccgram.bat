yacc -d awkgram.y
copy /y y.tab.c ytab.c
copy /y y.tab.h ytab.h
del y.tab.*