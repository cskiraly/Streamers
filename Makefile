CFLAGS = -Wall
CFLAGS += -Wdeclaration-after-statement
CFLAGS += -Wno-switch -Wpointer-arith -Wredundant-decls
CFLAGS += -Wno-pointer-sign 
CFLAGS += -g

CPPFLAGS = -I$(GRAPES)/include
CPPFLAGS += -I$(GRAPES)/som

ifdef DEBUG
CPPFLAGS += -DDEBUG
endif

LDFLAGS = -L$(GRAPES)/som/TopologyManager -L$(GRAPES)/som/ChunkTrading -L$(GRAPES)/som/ChunkBuffer
LDLIBS = -ltrading -lcb -ltopman

OBJS = dumbstreamer.o streaming.o output.o net_helpers.o
ifdef THREADS
OBJS += loop-mt.o
CFLAGS += -pthread
LDFLAGS += -pthread
else
OBJS += loop.o
endif

ifdef FFDIR
FFSRC ?= $(FFDIR)
OBJS += Chunkiser/input-avs.o
LDFLAGS += -L$(FFDIR)/libavcodec -L$(FFDIR)/libavformat -L$(FFDIR)/libavutil
LDLIBS += -lavformat -lavcodec -lavutil
LDLIBS += -lz -lm	#FIXME!
else
OBJS += input-dummy.o
endif

all: dumbstreamer

dumbstreamer: $(OBJS) $(GRAPES)/som/net_helper.o

Chunkiser/input-avs.o: CPPFLAGS += -I$(FFSRC) 

clean:
	rm -f dumbstreamer
	rm -f *.o
