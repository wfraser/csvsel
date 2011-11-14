CFLAGS=-Wall -Werror -Wno-unused-function -Wno-unused-label -std=gnu99 -O0 -gstabs -D_GNU_SOURCE
YFLAGS=-t -v
LFLAGS=-d

all: csvsel

csvsel: csvsel.o growbuf.o csvformat.o queryeval.o queryparse.tab.o querylex.tab.o util.o functions.o

queryeval.o: queryparse.tab.h

queryparse.tab.c: queryparse.y
	bison $(YFLAGS) -p query_ --defines=queryparse.tab.h queryparse.y

queryparse.tab.h: queryparse.tab.c

querylex.tab.c: querylex.l
	flex -Pquery_ -oquerylex.tab.c querylex.l

test: test.o queryparse.tab.o querylex.tab.o growbuf.o util.o functions.o

test.o: queryparse.tab.h

clean:
	rm -f csvsel test *.o *.tab.c *.tab.h
