vpath %.a ../lib
vpath %.h ./inc

PAPP_PATH=/home/mesasoft/frag_rssb/

#CFLAGS          =  -g3 -Wall -fPIC -Werror -O
#CFLAGS          =  -g3 -Wall -fPIC -O
INCLUDES        = -I./inc/
INCLUDES        += -I/usr/include/MESA/
INCLUDES        += -I./support/hiredisv4/
CFLAGS          =  -g3 -Wall -fPIC -D__STDC_FORMAT_MACROS
CFLAGS	       	+= $(INCLUDES)

ifeq ($(MODE), PIC)
CFLAGS += -DPIC_FUNC=1
TARGET	= pic_rssb_maskey
else ifeq ($(MODE), VOIP)
CFLAGS += -DPIC_FUNC=1
TARGET	= voip_rssb_maskey
else
TARGET	= av_rssb_maskey
endif
#LDDICTATOR      = -Wl,-wrap,malloc -Wl,-wrap,calloc -Wl,-wrap,free -Wl,-wrap,realloc
LDDICTATOR      =
CC              = g++
CCC             = g++

LIB		= -L./lib/ 
LIB		+= -lsoqav_dedup
#LIB		+= -lappdetect
LIB		+= -lrdkafka
LIB		+= -lssl -lcrypto
LIB		+= -lMESA_handle_logger -lMESA_prof_load -lmaatframe -lrulescan -lMESA_htable -lMESA_field_stat2 -lpthread -lasmislog -lwiredcfg -lWiredLB
LIB		+= ./lib/libsifter.a ./lib/libmy_socket.a ./lib/libbusinessman.a ./lib/libMESA_trace.a ./lib/lib_interval_index.a ./lib/lib_MESA_timer.a
#LIB             += -lhiredis_vip
LIB		+= ./lib/libhiredis_vip.a
LIB		+= ./lib/usm_comm.a
#LIB		+= ./lib/libdictator_debug.a

LIB_FILE	= $(wildcard ../lib/*.a)
SOURCES		= $(wildcard *.c)
SOURCESCPP  = $(wildcard *.cpp)
OBJECTS 	= $(SOURCES:.c=.o)
OBJECTSCPP  = $(SOURCESCPP:.cpp=.o)
#DEPS		= $(SOURCES:.c=.d)

.PHONY:clean all 

all:$(TARGET)

$(TARGET):$(OBJECTS) $(OBJECTSCPP)  $(LIB_FILE)
	#$(CCC) -shared $(CFLAGS) $(OBJECTS) $(LIB) -o $@
	$(CCC) $(CFLAGS) $(OBJECTS) $(OBJECTSCPP)  $(LIB) $(LDDICTATOR) -o $@
	cp $(TARGET) ../bin/
#	cp $(TARGET) /home/mesasoft/frag_rssb/
	
.c.o:
#%.d:%.c
#	$(CCC) $< -MM $(INCLUDES) > $@

.cpp.o:
#%.dpp:%.cpp
	$(CCC) $(CFLAGS) -c $<

-include $(DEPS)

clean :
	rm -f $(OBJECTS) $(DEPS) $(TARGET) 

