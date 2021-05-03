GCC=g++
CPPFLAGS=-std=c++11 -o3 -Wall

SRCS=structs.cpp hsa.cpp io.cpp \
	 toposort.cpp closure.cpp hungarian.cpp hopcroftkarp.cpp pathcover.cpp \
	 coloring.cpp edmonds.cpp assignment.cpp calculation.cpp

OBJS=$(SRCS:%.cpp=%.o)

all: setup tss

setup: $(OBJS)
	$(GCC) $(CPPFLAGS) $(OBJS) setup.cpp -o setup

tss: $(OBJS)
	$(GCC) $(CPPFLAGS) $(OBJS) tss.cpp -o tss

clean:
	rm *.o setup tss
