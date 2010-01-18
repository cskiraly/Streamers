CFLAGS = -Wall
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-switch -Wpointer-arith -Wredundant-decls
CFLAGS += -Wno-pointer-sign 
CFLAGS += -g

CPPFLAGS = -I$(GRAPES)/include
CPPFLAGS += -I$(GRAPES)/som

LDFLAGS = -L$(GRAPES)/som/TopologyManager -L$(GRAPES)/som/ChunkTrading -L$(GRAPES)/som/ChunkBuffer
LDLIBS = -ltrading -lcb -ltopman

OBJS = dumbstreamer.o streaming.o output.o input.o net_helpers.o
ifdef THREADS
OBJS += loop-mt.o
CFLAGS += -pthread
LDFLAGS += -pthread
else
OBJS += loop.o
endif


all: dumbstreamer

dumbstreamer: $(OBJS) $(GRAPES)/som/net_helper.o

clean:
	rm -f dumbstreamer
	rm -f *.o
