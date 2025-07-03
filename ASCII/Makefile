OBJ_DIR = .
LIB_DIR = ../LIB
EXE	= amov
OBJECTS = ${OBJ_DIR}/${EXE}.o
LINKLIB = -L$(LIB_DIR) -lc_prgcfg

target: $(OBJECTS);
	gcc -Wall \
	-o $(EXE) \
	$(OBJECTS) \
	$(LINKLIB)


$(OBJ_DIR)/${EXE}.o :  ascii_mov.c;
	gcc -c ascii_mov.c -Wall -o $(OBJ_DIR)/${EXE}.o
