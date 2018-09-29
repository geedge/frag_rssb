LIB_PATH=./lib/
SRC=./
OBJ=./
INCLUDE=./inc

LIB=-L./lib  -lsitdc++ -lpthread 
CC=g++ -g

G_FLAG_SO = -Wall -fPIC -I$(INCLUDE)
G_FLAG = -Wall -I$(INCLUDE)

SRCFILE= $(wildcard  $(SRC)*.c)
DETACHDIR= $(notdir $(SRCFILE))
OBJECTS = $(patsubst %.c, $(OBJ)%.o, $(DETACHDIR)) 

TARGETLIB = $(LIB_PATH)libMESA_trace.a
TARGETSO  = $(LIB_PATH)libMESA_trace.so


$(OBJ)%.o:$(SRC)%.c
	$(CC) -c -o $@ $(G_FLAG_SO)  $<

all:$(TARGETLIB) 
#all:$(TARGETSO) 

$(TARGETLIB):$(OBJECTS)
	ar rs $@  $^
$(TARGETSO):$(OBJECTS)
	$(CC) -o $@ $(G_FLAG) -shared $^
	
clean:
	-rm -rf $(OBJECTS) $(TARGETSO) $(TARGETLIB) *.o 

.PHONY:all clean
