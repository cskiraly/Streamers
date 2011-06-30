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

LINKER = $(CC)
STATIC ?= 0

NAPA ?= ../../NAPA-BASELIBS
GRAPES ?= ../../GRAPES

CPPFLAGS = -I$(NAPA)/include
CPPFLAGS += -I$(GRAPES)/include
CPPFLAGS += -Itransition

ifdef GPROF
CFLAGS += -pg -O0
LDFLAGS += -pg
endif

ifdef DEBUG
CFLAGS += -O0
CPPFLAGS += -DDEBUG
endif
OBJS += dbg.o


ifdef DEBUGOUT
CPPFLAGS += -DDEBUGOUT
endif

LDFLAGS += -L$(GRAPES)/src
LDLIBS += -lgrapes
ifdef ALTO
LDFLAGS += -L$(NAPA)/ALTOclient
LDFLAGS += -L$(LIBXML2_DIR)/lib
LDLIBS += -lALTO -lxml2
CFLAGS += -pthread
LDFLAGS += -pthread
LDLIBS += $(call ld-option, -lz)
endif
ifdef ML
LDFLAGS += -L$(NAPA)/ml -L$(LIBEVENT_DIR)/lib
LDLIBS += -lml -lm
CPPFLAGS += -I$(NAPA)/ml/include -I$(LIBEVENT_DIR)/include
ifdef MONL
LDFLAGS += -L$(NAPA)/dclog -L$(NAPA)/rep -L$(NAPA)/monl -L$(NAPA)/common
LDLIBS += -lstdc++ -lmon -lrep -ldclog -lcommon
CPPFLAGS += -DMONL
ifneq ($(STATIC), 0)
LINKER=$(CXX)
endif
endif
LDLIBS += -levent
LDLIBS += $(call ld-option, -lrt)
endif

OBJS += streaming.o
OBJS += net_helpers.o 

ifdef ALTO
OBJS += topology-ALTO.o
OBJS += config.o
else
OBJS += topology.o nodeid_set.o
endif

OBJS += chunk_signaling.o
OBJS += chunklock.o
OBJS += transaction.o
OBJS += ratecontrol.o
OBJS += channel.o
ifdef THREADS
CPPFLAGS += -DTHREADS
OBJS += loop-mt.o
CFLAGS += -pthread
LDFLAGS += -pthread
else
OBJS += loop.o
endif

ifdef MONL
OBJS += measures-monl.o
else
OBJS += measures.o
endif

IO ?= grapes
ifeq ($(IO), grapes)
CFLAGS += -DIO_GRAPES
OBJS += input-grapes.o output-grapes.o
ifdef FFMPEG_DIR
CPPFLAGS += -I$(FFMPEG_DIR)
LDFLAGS += -L$(FFMPEG_DIR)/libavcodec -L$(FFMPEG_DIR)/libavformat -L$(FFMPEG_DIR)/libavutil -L$(FFMPEG_DIR)/libavcore -L$(FFMPEG_DIR)/lib
CFLAGS += -pthread
LDFLAGS += -pthread
LDLIBS += -lavformat -lavcodec -lavutil
LDLIBS += $(call ld-option, -lavcore)
LDLIBS += -lm
LDLIBS += $(call ld-option, -lz)
LDLIBS += $(call ld-option, -lbz2)
LDLIBS += $(call ld-option, -lva)
ifdef X264_DIR
CPPFLAGS += -I$(X264_DIR) -I$(X264_DIR)/include
LDFLAGS += -L$(X264_DIR) -L$(X264_DIR)/lib
LDLIBS += -lx264
endif
endif
endif
ifeq ($(IO), chunkstream)
CFLAGS += -DIO_CHUNKSTREAM
OBJS += input-chunkstream.o output-chunkstream.o
endif

EXECTARGET = streamer
ifdef ML
EXECTARGET := $(EXECTARGET)-ml
endif
ifdef MONL
EXECTARGET := $(EXECTARGET)-monl
endif
ifdef ALTO
EXECTARGET := $(EXECTARGET)-alto
endif
ifdef THREADS
EXECTARGET := $(EXECTARGET)-threads
endif
ifdef IO
EXECTARGET := $(EXECTARGET)-$(IO)
endif

ifeq ($(STATIC), 1)
EXECTARGET := $(EXECTARGET)-halfstatic
LDFLAGS += -Wl,-static
LDFLAGSPOST += -Wl,-Bdynamic
endif
ifeq ($(STATIC), 2)
EXECTARGET := $(EXECTARGET)-static
LDFLAGS += -static
endif

ifdef DEBUG
EXECTARGET := $(EXECTARGET)-debug
endif

ifdef RELEASE
EXECTARGET := $(EXECTARGET)-$(RELEASE)
endif

ifeq ($(HOSTARCH), mingw32)
LDLIBS += -lmsvcrt -lwsock32 -lws2_32 -liberty
EXECTARGET := $(EXECTARGET).exe
else
LDLIBS += $(LIBRT)
endif

all: $(EXECTARGET)

ifndef ML
$(EXECTARGET): $(OBJS) $(GRAPES)/src/net_helper.o $(EXECTARGET).o
else
$(EXECTARGET): $(OBJS) $(GRAPES)/src/net_helper-ml.o $(EXECTARGET).o
endif
	$(LINKER) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) $(LDFLAGSPOST) -o $@

$(EXECTARGET).o: streamer.o
	ln -sf streamer.o $(EXECTARGET).o

GRAPES:
	git clone http://www.disi.unitn.it/~kiraly/PublicGits/GRAPES.git
	cd GRAPES; git checkout -b for-streamer-0.8.3 origin/for-streamer-0.8.3

ffmpeg:
	(wget http://ffmpeg.org/releases/ffmpeg-checkout-snapshot.tar.bz2; tar xjf ffmpeg-checkout-snapshot.tar.bz2; mv ffmpeg-checkout-20* ffmpeg) || svn checkout svn://svn.ffmpeg.org/ffmpeg/trunk ffmpeg
	cd ffmpeg; ./configure

prepare: $(GRAPES) $(FFSRC)
	$(MAKE) -C $(GRAPES) -f Makefile
ifdef ML
	cd $(NAPA); ./autogen.sh; $(MAKE)
endif
	$(MAKE) -C $(FFSRC)

clean:
	rm -f streamer-*
	rm -f $(GRAPES)/src/net_helper-ml.o
	rm -f $(GRAPES)/src/net_helper.o
	rm -f *.o
	rm -f Chunkiser/*.o
