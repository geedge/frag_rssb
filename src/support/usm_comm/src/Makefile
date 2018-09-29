CC=gcc
CCC=g++

CFLAGS=-g -Wall -fPIC -shared
LIBPATH=../lib/
H_DIR=-I../inc

OBJS=usm.o output.o

TARGET=usm_comm.a usm_comm.so

all:$(TARGET)

.c.o:
	$(CC) -c $(CFLAGS) -I. $(H_DIR) $<

.cpp.o:
	$(CCC) -c $(CFLAGS) -I. $(H_DIR) $<

usm_comm.a: $(OBJS)
	echo  making static lib ...
	(rm -f $@ ;ar -r $@ $^; cp $@ $(LIBPATH);)

usm_comm.so: $(OBJS)
	echo  making dynamic lib ...
	(rm -f $@; gcc -o $@ $(CFLAGS) $^; cp $@ $(LIBPATH);)

clean:
	rm -f *.o $(TARGET)
