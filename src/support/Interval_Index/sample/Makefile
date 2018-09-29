CC=gcc -g -O2
LIB_PATH=../lib
INC=-I../include
LIB=../lib/lib_interval_index.a

TARGET=sample

all:$(TARGET)

sample:sample.c
	$(CC)  -o $@ $(INC) $^ -L$(LIB_PATH) $(LIB)
clean:
	rm -f $(TARGET)
