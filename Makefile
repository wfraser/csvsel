CFLAGS=-pedantic -std=c1x -O3 -D_GNU_SOURCE
YFLAGS=-t -v
LFLAGS=-d

OBJS=csvsel.o growbuf.o csvformat.o queryeval.o queryparse.tab.o querylex.tab.o util.o functions.o

all: csvsel

csvsel: main.o $(OBJS)

queryeval.o: queryparse.tab.h

queryparse.tab.c: queryparse.y
	bison $(YFLAGS) -p query_ --defines=queryparse.tab.h queryparse.y

queryparse.tab.h: queryparse.tab.c

querylex.tab.c: querylex.l queryparse.tab.h
	flex -Pquery_ -oquerylex.tab.c querylex.l

test: test.o unittests.o $(OBJS)

test.o: queryparse.tab.h unittests.h

util.o: queryparse.tab.h

unittests.o: queryparse.tab.h

clean:
	rm -f csvsel test *.o *.tab.c *.tab.h
