# if you type 'make' without arguments, this is the default
PROG    = csv
all:    $(PROG)

# Tell make about the file dependencies
HEAD	= splitrow.h wrtrow.h setorder.h version.h
SRC     = csv.c wrtrow.c splitrow.c setorder.c
OBJ     = ${SRC:%.c=%.o}

# special libraries This can be blank
LIB     = libpa4.a
LIBFLAGS    = -L ./ $(LIB)

# select the compiler and flags
# you can over-ride these on the command line e.g. make DEBUG= 
CC      = gcc
DEBUG	= -ggdb
CSTD	=
WARN	= -Wall -Wextra
CDEFS	=
CFLAGS	= -I. $(DEBUG) $(WARN) $(CSTD) $(CDEFS)

$(OBJ):     $(HEAD)

# specify how to compile the target
$(PROG):	$(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIB) -o $@

#list of test number
LIST    = 1 2

test:    $(PROG)
	./runtest $(LIST)

# remove binaries
.PHONY: clean 
clean:
	rm -f $(OBJ) $(PROG)
