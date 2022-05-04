PROG = main

MPROT ?= mprotect
DUNE ?= ../dune

ifeq ($(MPROT),mprotect)
	OBJ = memstk_cow.o memstk_basic.o mprotect.o
else ifeq ($(MPROT),userfaultfd)
	OBJ = memstk_cow.o memstk_basic.o userfaultfd.o
else
	OBJ = memstk_cow.o memstk_basic.o dune.o
	CFLAGS += -I$(DUNE)
	LDFLAGS += -static -L$(DUNE)/libdune -ldune
endif

CC = gcc
all: $(PROG)

include rules.mk

SRC = $(wildcard *.c)

%.o: %.c $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -c $<

$(PROG): $(PROG).o $(OBJ)
	$(CC) $(CFLAGS) $(O) -o $@ $^ $(LDFLAGS)

test: test.o $(OBJ)
	$(CC) $(CFLAGS) $(O) -o $@ $^ $(LDFLAGS)

# for inspecting assembly
%.s: %.c $(BUILDSTAMP)
	$(CC) $(CFLAGS) $(O) $(DEPCFLAGS) -o $@ -S $<

clean:
	rm -rf $(ALLPROGRAMS) *.o $(DEPSDIR)

format:
	clang-format -i $(SRC) $(wildcard $(INCDIR)/*.h)

.PHONY: all clean format
