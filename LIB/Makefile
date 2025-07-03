#-------------------------------------------------------------------------------
# makefile for program config library
#-------------------------------------------------------------------------------

LIB_DIR  = .

libc_prgcfg.a :	$(LIB_DIR)/mystrings.o $(LIB_DIR)/prgcfg.o;
		ar rcs libc_prgcfg.a $(LIB_DIR)/mystrings.o $(LIB_DIR)/prgcfg.o

$(LIB_DIR)/mystrings.o : $(LIB_DIR)/mystrings.h $(LIB_DIR)/mystrings.c;
		gcc -c $(LIB_DIR)/mystrings.c -D_GNU_SOURCE -O3 -Wall -o $(LIB_DIR)/mystrings.o

$(LIB_DIR)/prgcfg.o :    $(LIB_DIR)/prgcfg.h $(LIB_DIR)/prgcfg.c;
		gcc -c  $(LIB_DIR)/prgcfg.c -D_GNU_SOURCE -O3 -Wall -o $(LIB_DIR)/prgcfg.o
