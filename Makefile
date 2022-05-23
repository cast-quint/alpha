# Dimitris Koropoulis 3967
# csd3967@csd.uoc.gr
# CS340 - Spring 2021
# Makefile

CC     = gcc
LEX    = flex
YACC   = bison
CFLAGS = -Wall
LFLAGS = --outfile=scan.c
YFLAGS = --warnings=all

COMP_NAME = alc
AVM_NAME  = avm
INTERMEDIATE = *.o scan.c parse.c parse.h  $(COMP_NAME) $(AVM_NAME) *.abc logs
COMP_OBJECTS = memory-management.o list.o symtable.o stack.o scan.o tcode.o util.o parse.o comp_main.o
AVM_OBJECTS = memory-management.o avm_util.o avm_assign.o avm_arithm.o avm_relop.o avm_eqop.o avm_func.o avm_lib.o avm_table.o avm_main.o

.PHONY: all clean comp vm

all: clean comp vm

comp: $(COMP_OBJECTS)
	$(CC) $(CFLAGS) $^ -o $(COMP_NAME)

vm: $(AVM_OBJECTS)
	$(CC) $(CFLAGS) $^  -lm -o $(AVM_NAME)

main.o: main.c scan.o parse.o
	$(CC) $(CFLAGS) -c $< -o $@

scan.o: scan.c parse.c
	$(CC) $(CFLAGS) -c $< -o $@

scan.c: scan.l
	$(LEX) $(LFLAGS) $<

parse.o: parse.c parse.h
	$(CC) $(CFLAGS) -c $< -o $@

parse.c: parse.y
	$(YACC) $(YFLAGS) $< -o $@

symtable.o: symtable.c symtable.h
	$(CC) $(CFLAGS) -c $< -o $@

util.o: util.c util.h
	$(CC) $(CFLAGS) -c $< -o $@

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c $< -o $@

list.o: list.c list.h util.o
	$(CC) $(CFLAGS) -c $< -o $@

tcode.o: tcode.c tcode.h
	$(CC) $(CFLAGS) -c $< -o $@

avm_util.o: avm_util.c avm_util.h
	$(CC) $(CFLAGS) -c $< -o $@

avm_assign.o: avm_assign.c avm_inst.h
	$(CC) $(CFLAGS) -c $< -o $@

avm_arithm.o: avm_arithm.c avm_inst.h
	$(CC) $(CFLAGS) -c $< -o $@

avm_relop.o: avm_relop.c avm_inst.h
	$(CC) $(CFLAGS) -c $< -o $@

avm_eqop.o: avm_eqop.c avm_inst.h
	$(CC) $(CFLAGS) -c $< -o $@

avm_func.o: avm_func.c avm_inst.h
	$(CC) $(CFLAGS) -c $< -o $@

avm_lib.o: avm_lib.c avm_lib.h
	$(CC) $(CFLAGS) -c $< -o $@

avm_table.o: avm_table.c avm_table.h
	$(CC) $(CFLAGS) -c $< -o $@

memory-management.o: memory-management.c memory-management.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(INTERMEDIATE)
