
CFLAGS = -Wall -Werror -Wno-unused-result -g

LDFLAGS = -lpthread -ldl

COMM_OBJS = sqlite3.o

PROGS = sqlite3shell \
	testmemvfs

all: $(PROGS)

testmemvfs: $(COMM_OBJS) spmemvfs.o testmemvfs.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

sqlite3shell: $(COMM_OBJS) sqlite3shell.o
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

sqlite3.o: CFLAGS = -g

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o lip/*.o $(PROGS) vgcore.* core

