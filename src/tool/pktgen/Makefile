# CREATED by Guo Le, 08/23/2010
#
CC		=   gcc
PROGRAM = 	pktgen 
CFLAGS += -g -Wall -D_DEBUG_ 
LDFLAGS	+= 	-lpthread -lm
OBJS 	= 	pktgen.o test.o
INCLUDE = -I. -I../common

.PHONY : all clean 
all: pktgen 

pktgen: pktgen.o 
	$(CC) $(LDFLAGS) \
		pktgen.o common.a -o pktgen

test: test.o 
	$(CC) $(LDFLAGS) \
		test.o ../lib/common.a -o test 

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	$(RM) *.o $(PROGRAM) common.a
	
