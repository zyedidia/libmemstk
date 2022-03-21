PROG = main
OBJ = memstk_cow.o memstk_basic.o mprotect.o
# OBJ = memstk_cow.o memstk_basic.o userfaultfd.o
all: $(PROG)

include rules.mk

SRC = $(wildcard *.c)

%.o: %.c $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -c $<

$(PROG): $(PROG).o $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(O) -o $@ $^

test: test.o $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(O) -o $@ $^

# for inspecting assembly
%.s: %.c $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -S $<

clean:
	rm -rf $(ALLPROGRAMS) *.o $(DEPSDIR)

format:
	clang-format -i $(SRC) $(wildcard $(INCDIR)/*.h)

.PHONY: all clean format
