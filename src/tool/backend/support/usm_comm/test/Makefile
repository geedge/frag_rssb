CC=gcc
CCC=g++

CFLAGS=-g -Wall

INCLUDE=-I../inc/
LIB=-lrt -lpthread -lMESA_htable -lMESA_handle_logger
LIB+=../lib/usm_comm.so
#LIB+=../lib/libcommon.a

TARGET=test_writer test_reader0 test_reader1 test_reader2

all:
	gcc $(CFLAGS) -o test_writer test_writer.c $(INCLUDE) $(LIB)
	gcc $(CFLAGS) -o test_reader0 test_reader.c $(INCLUDE) $(LIB)
	gcc $(CFLAGS) -o test_reader1 test_reader1.c $(INCLUDE) $(LIB)
	gcc $(CFLAGS) -o test_reader2 test_reader2.c $(INCLUDE) $(LIB)
clean:
	rm -f *.o $(TARGET)
