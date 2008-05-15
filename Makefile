YACC = bison
YFLAGS = -y -d -t
CC = gcc
CFLAGS = -I /homes/owre/src/oaa2.3.2/src/oaalib/c/include \
         -I /homes/owre/src/oaa2.3.2/src/oaalib/c/external/glib/glib-2.10.1/glib \
         -I /homes/owre/src/oaa2.3.2/lib/x86-linux/glib-2.0/include \
         -I /homes/owre/src/oaa2.3.2/src/oaalib/c/external/glib/glib-2.10.1 \
         -Wall -g # -Wextra
LD = ld 
XCFLAGS =

# pceobj = array_hash_map.o hash_functions.o int_array_sort.o int_stack.o \
#          memalloc.o pce.tab.o samplesat.o symbol_tables.o \
#          test_array_hash_map.o test_samplesat.o pce.o

commonobj = samplesat.o symbol_tables.o array_hash_map.o \
         int_stack.o memalloc.o hash_functions.o int_array_sort.o \
         integer_stack.o utils.o gcd.o vectors.o input.o string_heap.o

mcsatobj = yacc.o mcsat.o $(commonobj)

pceobj = pce.o $(commonobj)

.SUFFIXES: .c .o
.c.o : ; $(CC) $(XCFLAGS) ${CFLAGS} -c $< -o $@

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