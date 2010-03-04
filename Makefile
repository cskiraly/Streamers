include utils.mak

CFLAGS = -g -Wall
CFLAGS += $(call cc-option, -Wdeclaration-after-statement)
CFLAGS += $(call cc-option, -Wno-switch)
CFLAGS += $(call cc-option, -Wdisabled-optimization)
CFLAGS += $(call cc-option, -Wpointer-arith)
CFLAGS += $(call cc-option, -Wredundant-decls)
CFLAGS += $(call cc-option, -Wno-pointer-sign)
CFLAGS += $(call cc-option, -Wcast-qual)
CFLAGS += $(call cc-option, -Wwrite-strings)
CFLAGS += $(call cc-option, -Wtype-limits)
CFLAGS += $(call cc-option, -Wundef)

CFLAGS += $(call cc-option, -funit-at-a-time)

GRAPES ?= GRAPES

CPPFLAGS = -I$(GRAPES)/include
CPPFLAGS += -I$(GRAPES)/som

ifdef DEBUG
CFLAGS += -O0
CPPFLAGS += -DDEBUG
endif

ifdef DEBUGOUT
CPPFLAGS += -DDEBUGOUT
endif

ifdef ML
LDFLAGS += -L$(GRAPES)/som -L$(GRAPES)/ml -L$(LIBEVENT)/lib -L$(GRAPES)/dclog
LDLIBS += -lsom -lml -levent -ldclog -lm
CPPFLAGS += -I$(LIBEVENT)/include
else
LDFLAGS = -L$(GRAPES)/som/TopologyManager -L$(GRAPES)/som/ChunkTrading -L$(GRAPES)/som/ChunkBuffer
LDLIBS = -ltrading -lcb -ltopman
endif

OBJS = streamer.o streaming.o output.o net_helpers.o input.o out-stream.o
ifdef THREADS
OBJS += loop-mt.o
CFLAGS += -pthread
LDFLAGS += -pthread
else
OBJS += loop.o
endif

ifdef FFDIR
FFSRC ?= $(FFDIR)
OBJS += Chunkiser/input-stream-avs.o
LDFLAGS += -L$(FFDIR)/libavcodec -L$(FFDIR)/libavformat -L$(FFDIR)/libavutil
LDLIBS += -lavformat -lavcodec -lavutil
LDLIBS += -lm
LDLIBS += $(call ld-option, -lz)
LDLIBS += $(call ld-option, -lbz2)
else
OBJS += input-stream-dummy.o
endif

EXECTARGET = dumbstreamer
ifdef ML
EXECTARGET := $(EXECTARGET)-ml
endif

all: $(EXECTARGET)

ifndef ML
$(EXECTARGET): $(OBJS) $(GRAPES)/som/net_helper.o
else
$(EXECTARGET): $(OBJS) $(GRAPES)/som/Tests/net_helper-ml.o $(GRAPES)/som/Tests/ml_helpers.o
endif
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

Chunkiser/input-stream-avs.o: CPPFLAGS += -I$(FFSRC) 

GRAPES:
	git clone http://www.disi.unitn.it/~kiraly/PublicGits/GRAPES.git
	cd GRAPES; git checkout -b for-streamer-0.7.1 origin/for-streamer-0.7.1

prepare: GRAPES
ifndef ML
	$(MAKE) -C GRAPES/som -f Makefile.som
else
	cd GRAPES; ./autogen.sh; $(MAKE)
endif

clean:
	rm -f $(EXECTARGET)
	rm -f *.o
