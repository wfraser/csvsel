CFLAGS=-Wall -Werror -std=c99 -O0 -gstabs
YFLAGS=-t

all: csvsel

csvsel: main.o growbuf.o csvsel.o query.tab.o

query.tab.c: query.y
	bison $(YFLAGS) -p query_ query.y

clean:
	rm -f csvsel *.o *.tab.c
