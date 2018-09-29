CC=gcc
CCC=g++

CFLAGS=-g -Wall

INCLUDE=-I../inc/
LIB=-lrt -lpthread -lMESA_htable -lMESA_handle_logger
LIB+=../lib/usm_comm.so
#LIB+=../lib/libcommon.a

TARGET=write_file read_file

all:
	gcc $(CFLAGS) -o write_file write_file.c $(INCLUDE) $(LIB)
	gcc $(CFLAGS) -o read_file read_file.c $(INCLUDE) $(LIB)
clean:
	rm -f *.o $(TARGET)
