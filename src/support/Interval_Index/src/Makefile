CC=gcc
CCC=g++

CFLAGS=-g -O2 -Wall -fPIC -shared
LIBPATH=../lib
H_DIR=-I../include

OBJS=interval_index.o rbtree.o

TARGET=lib_interval_index.a lib_interval_index.so

all:$(TARGET)

.c.o:
	$(CC) -c $(CFLAGS) -I. $(H_DIR) $<
.cpp.o:
	$(CCC) -c $(CFLAGS) -I. $(H_DIR) $<
lib_interval_index.a: $(OBJS)
	(rm -f $@ ;ar -r $@ $^; cp $@ $(LIBPATH);)
lib_interval_index.so: $(OBJS)
	(rm -f $@; gcc -o $@ $(CFLAGS) $^; cp $@ $(LIBPATH);)
clean:
	rm -f *.o $(TARGET)
