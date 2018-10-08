# CREATED by Guo Le, 08/23/2010
#
CC		=   gcc
PROGRAM = 	
CFLAGS  = -g -Wall -D_DEBUG_ 

LDFLAGS	= 	
OBJS 	= 	input.o output.o loadprof.o log.o error.o ioctl_op.o

.PHONY : all clean 
all: common.a

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

common.a: $(OBJS)
	ar -r $@ $^
	cp $@ ../preprocessor/lib/libcommon.a
	cp $@ ../pktgen

clean:
	$(RM) *.o common.a


