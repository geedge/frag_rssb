LIB_PATH=./
SRC=./
OBJ=./
INCLUDE= -I ./inc
INCLUDE+= -I /usr/include/MESA/

LIB=-lsitdc++ -lpthread  -lMESA_handle_logger -lMESA_prof_load -lmaatframe 
CC=g++ -g

G_FLAG_SO = -Wall -fPIC $(INCLUDE)
G_FLAG = -Wall $(INCLUDE)

SRCFILE= $(wildcard  $(SRC)*.c)
DETACHDIR= $(notdir $(SRCFILE))
OBJECTS = $(patsubst %.c, $(OBJ)%.o, $(DETACHDIR)) 

TARGETLIB = $(LIB_PATH)libsifter.a
TARGETSO  = $(LIB_PATH)libsifter.so


$(OBJ)%.o:$(SRC)%.c
	$(CC) -c -o $@ $(LIB) $(G_FLAG_SO)  $<

all:$(TARGETLIB) 
#all:$(TARGETSO) 

$(OBJ)mem_common.o:$(SRC)mem_common.c
	$(CC) -O2 -c -o $@ $(LIB) $(G_FLAG_SO)  $<

$(TARGETLIB):$(OBJECTS)
	ar rs $@  $^
	cp $@ ../bin/
$(TARGETSO):$(OBJECTS)
	$(CC) -o $@ $(G_FLAG) -shared $^
	cp $@ ../bin/
clean:
	-rm -rf $(OBJECTS) $(TARGETSO) $(TARGETLIB) *.o 

.PHONY:all clean
