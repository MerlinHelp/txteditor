# default make call
PROG	= txteditor
all:	$(PROG)

# Tell make abt file dependencies
HEAD    = txteditor.h rawmode.h editor_io.h global.h
SRC     = txteditor.c rawmode.c editor_io.c global.c
OBJ     = ${SRC:%.c=%.o}

# special libraries (Can be blank)
LIB		=
LIBFLAGS    = -L ./ $(LIB)

# select the compiler and flags
# you can over-ride in cmd, (e.g. make DEBUG=...)
CC		= gcc
DEBUG	= -ggdb
# Flag for specifying C standard version (e.g. -std=c11)
CSTD	= 
# Extra warning flags, -Wall enables all "common warning" flags in compiler
# -Wextra enables additional warnings that are not enabled by -Wall
WARN	= -Wall -Wextra -Werror -pedantic
# CDEFS is var used to specify cpp defs to be passed to cc
# e.g. -DMYDEF -DMYVAL=123
CDEFS	= 
CFLAGS	= -I. $(DEBUG) $(WARN) $(CSTD) $(CDEFS)

$(OBJ):		$(HEAD)

# Specify how to compile the target
$(PROG):	$(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIB) -o $@

# remove binaries
.PHONY: clean
clean:
	rm -f $(OBJ) $(PROG)
