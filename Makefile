PROG = main
OBJ = memstk.o mprotect.o
# OBJ = memstk.o userfaultfd.o
# OBJ = memstk_basic.o
all: $(PROG)

include rules.mk

SRC = $(wildcard *.c)

%.o: %.c $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -c $<

$(PROG): $(PROG).o $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $(O) -o $@ $^

# for inspecting assembly
%.s: %.c $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -S $<

clean:
	rm -rf $(ALLPROGRAMS) *.o $(DEPSDIR)

format:
	clang-format -i $(SRC) $(wildcard $(INCDIR)/*.h)

.PHONY: all clean format
