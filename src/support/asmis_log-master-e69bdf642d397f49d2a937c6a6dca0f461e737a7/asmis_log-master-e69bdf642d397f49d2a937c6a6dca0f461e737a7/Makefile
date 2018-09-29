opt: OPTFLAGS = -O2
export OPTFLAGS

CCC		= g++
INCLUDES	= 
override CFLAGS	= -Wall $(INCLUDES)
override CFLAGS	+= -g -fPIC
CFLAGS		+= $(OPTFLAGS)

SOURCES		= asmis_log.cpp
OBJS		= $(SOURCES:.cpp=.o)
SO_TARGET	= libasmislog.so
TEST_TARGET	= testmain

.PHONY: all clean deps test opt

all: $(SO_TARGET) $(TEST_TARGET)

opt:
	$(MAKE) all

clean:
	rm -f $(TARGETS) $(TEST_TARGETS) *.o core core.*

deps:
	@$(CC) -MM $(SOURCES)
.c.o:
	$(CC) -c $(CFLAGS) -I. $(H_DIR) $<

.cpp.o:
	$(CCC) -c $(CFLAGS) -I. $(H_DIR) $<

$(SO_TARGET): $(OBJS)
	$(CCC) -o $@ -shared $^

$(TEST_TARGET): testmain.cpp $(SO_TARGET)
	$(CCC) -o $@ $(CFLAGS) $^  -lpthread
