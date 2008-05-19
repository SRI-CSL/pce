SHELL=/bin/sh

YACC = bison
YFLAGS = -y -d -t
CC = gcc
CFLAGS = -Wall -g # -Wextra
LD = ld 
XCFLAGS =

#
# Special flag for compilation in MINGW
#
OS=$(shell uname -s)
ifeq ($(OS),MINGW32_NT-5.1) 
  CFLAGS += -DMINGW
endif

commonobj = samplesat.o symbol_tables.o array_hash_map.o \
         int_stack.o memalloc.o hash_functions.o int_array_sort.o \
         integer_stack.o utils.o gcd.o vectors.o input.o string_heap.o

mcsatobj = yacc.o mcsat.o $(commonobj)

pceobj = pce.o $(commonobj)


.SUFFIXES: .c .o
.c.o :
	 $(CC) $(XCFLAGS) $(CFLAGS) -c $< -o $@

all: mcsat #pce

mcsat: $(mcsatobj)
	$(CC) -g -o mcsat -lm $^

pce: $(pceobj)
	$(CC) -g -o pce -lm $^

mcsat.o : mcsat.c y.tab.h samplesat.h utils.h
pce.o : pce.c y.tab.h samplesat.h
yacc.o : yacc.c samplesat.h
samplesat.o : samplesat.h
integer_stack.c : integer_stack.h
utils.c : utils.h



#
# Clean up: delete objects and others
#
clean:
	rm -f *.o
	rm -f yacc.c y.tab.h


#
# Build the tar file
#
dist=./distribution
tmpdir=$(dist)/mcsat
tarfile=./mcsat-alpha.tgz

tarfile: mcsat mcsat.exe
	rm -rf $(tmpdir)
	mkdir -p $(tmpdir)
	mkdir $(tmpdir)/bin
	mkdir $(tmpdir)/doc
	mkdir $(tmpdir)/tests
	cp *.h *.c *.y Makefile $(tmpdir)
	cp ./doc/mcsat.muse ./doc/mcsat.pdf ./doc/mcsat.html $(tmpdir)/doc
	cp ./tests/* $(tmpdir)/tests
	cp mcsat mcsat.exe $(tmpdir)/bin
	chmod -R og+rX $(tmpdir)
	(cd $(dist); tar -czf $(tarfile) mcsat)


.PHONY: tarfile clean
