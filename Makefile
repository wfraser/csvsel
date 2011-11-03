CFLAGS=-Wall -Werror -Wno-unused-function -std=c99 -O0 -gstabs -D_POSIX_SOURCE -D_XOPEN_SOURCE=500
YFLAGS=-t -v
LFLAGS=-d

all: csvsel

csvsel: main.o growbuf.o csvsel.o queryparse.tab.o querylex.tab.o util.o

csvsel.o: queryparse.tab.h

queryparse.tab.c: queryparse.y
	bison $(YFLAGS) -p query_ --defines=queryparse.tab.h queryparse.y

queryparse.tab.h: queryparse.tab.c

querylex.tab.c: querylex.l
	flex -P query_ -o querylex.tab.c querylex.l

test: test.o queryparse.tab.o querylex.tab.o growbuf.o util.o

test.o: queryparse.tab.h

clean:
	rm -f csvsel test *.o *.tab.c *.tab.h
