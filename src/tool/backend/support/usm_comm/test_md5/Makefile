CC=gcc
CCC=g++

CFLAGS=-g -Wall
INCLUDE=-I../inc/
LIB=-lrt -lpthread -lcrypto -lssl -lMESA_htable -lMESA_handle_logger
LIB+=../lib/usm_comm.so

TARGET=md5_writer md5_reader0 md5_reader1 md5_reader2

all:
	gcc $(CFLAGS) -o md5_reader0 md5_reader.c $(INCLUDE) $(LIB)
	gcc $(CFLAGS) -o md5_reader1 md5_reader1.c $(INCLUDE) $(LIB)
	gcc $(CFLAGS) -o md5_reader2 md5_reader2.c $(INCLUDE) $(LIB)
	gcc $(CFLAGS) -o md5_writer md5_writer.c $(INCLUDE) $(LIB)
clean:
	rm -f *.o $(TARGET)
