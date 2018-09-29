#################################################################################
#			Businessman Data Exchange System v0.1			#
#			Build by zhengchao@iie.ac.cn				#
#					2011-09-19				#
#################################################################################
CC	= g++
CFLAGS	= -g -O2 -Wall -fPIC -fvisibility=hidden
LDFLAGS = -lstdc++
#CFLAGS	+=-DBIZMAN_DEBUG
CFLAGS	+=-DBIZMAN_DEBUG_LIST
CFLAGS	+=-DDEPLOY_YSP_SYSTEM
#CFLAGS	+=-DBIZMAN_DEBUG_DETAILS
LIBS	= -lpthread -lz
OBJECTS = hash.o list.o businessman_error.o businessman_time.o network_layer.o smooth_send.o\
		  reliable_send.o reliable_recv.o stream_recover.o businessman_core.o  businessman_packet.o
TEST_OBJ = test.o
TARGETLIB = libbusinessman.a
TARGETSO  = libbusinessman.so
TARGETEXE = test_bizman
.cpp.o:  
	$(CC) -c -o $@ $(CFLAGS) $< 

.PHONY: all clean
all:$(TARGETLIB) $(TARGETSO) 
#$(TARGETEXE) 
	cp $(TARGETLIB) ../../lib/
$(TARGETLIB):$(OBJECTS)
	ar cqs $(TARGETLIB) $(OBJECTS)
$(TARGETSO):$(OBJECTS)
	$(CC)  -o $(TARGETSO) $(CFLAGS) $(OBJECTS) -shared
$(TARGETEXE):$(TEST_OBJ) $(TARGETLIB) 
	$(CC) -o $(TARGETEXE) $(CFLAGS) $(TEST_OBJ) $(TARGETLIB) $(LIBS)
hash.o:hash.cpp hash.h
list.o:list.cpp list.h
businessman_error.o:businessman_error.cpp businessman_error.h
businessman_time.o:businessman_time.cpp businessman_time.h businessman_limit.h
businessman_packet.o:businessman_packet.cpp businessman_packet.h
network_layer.o:network_layer.cpp network_layer.h
smooth_send.o:smooth_send.cpp smooth_send.h businessman_limit.h businessman_packet.h businessman_lock.h
reliable_send.o:reliable_send.cpp reliable_send.h businessman_limit.h businessman_packet.h
reliable_recv.o:reliable_recv.cpp reliable_recv.h businessman_limit.h businessman_packet.h
stream_recover.o:stream_recover.cpp stream_recover.h businessman_limit.h businessman_packet.h businessman_lock.h
businessman_core.o:businessman_core.cpp businessman_core.h bizman.h businessman_packet.h smooth_send.h reliable_send.h\
   				reliable_recv.h stream_recover.h
test.o:test.cpp bizman.h
clean:
	rm -f $(TARGETEXE)	$(TARGETLIB) $(TARGETSO) $(OBJECTS) $(TEST_OBJ)


