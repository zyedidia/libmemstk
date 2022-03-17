PROG = main
all: $(PROG)

include rules.mk

MEMSTK = memstk_mprotect

INCDIR = include
INC = $(INCDIR)
INC_PARAMS=$(foreach d, $(INC), -I$d)
CFLAGS += $(INC_PARAMS)

SRC = $(wildcard *.c)

%.o: %.c $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -c $<

$(PROG): $(PROG).o $(MEMSTK).o
	$(CC) $(CFLAGS) $(LDFLAGS) $(O) -o $@ $^

# for inspecting assembly
%.s: %.c $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -S $<

clean:
	rm -rf $(ALLPROGRAMS) *.o $(DEPSDIR)

format:
	clang-format -i $(SRC) $(wildcard $(INCDIR)/*.h)

.PHONY: all clean format
