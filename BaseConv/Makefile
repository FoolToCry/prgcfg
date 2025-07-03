OBJ_DIR = .
LIB_DIR = ../LIB
EXE     = base_conv
OBJECTS = ${OBJ_DIR}/${EXE}.o
LINKLIB = -L$(LIB_DIR) -lc_prgcfg -lm

target: $(OBJECTS);
	gcc -g -Wall \
	-o $(EXE) \
	$(OBJECTS) \
	$(LINKLIB)


$(OBJ_DIR)/${EXE}.o : base_conv.c;
	gcc -c base_conv.c -Wall -D_GNU_SOURCE -O3 -o $(OBJ_DIR)/${EXE}.o
