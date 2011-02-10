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
ULPLAYER ?= ../StreamerPlayerChunker
ULPLAYER_EXTERNAL_LIBS ?= external_libs

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
OBJS += dbg.o
endif


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
LINKER=g++
endif
endif
LDLIBS += -levent -lrt
endif

OBJS += streaming.o
OBJS += net_helpers.o 

ifdef ALTO
OBJS += topology-ALTO.o
OBJS += config.o
else
OBJS += topology.o
endif

OBJS += chunk_signaling.o
OBJS += chunklock.o
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

IO ?= ffmpeg
ifeq ($(IO), grapes)
OBJS += input-grapes.o output-grapes.o
ifdef FFMPEG_DIR
CPPFLAGS += -I$(FFMPEG_DIR)
LDFLAGS += -L$(FFMPEG_DIR)/libavcodec -L$(FFMPEG_DIR)/libavformat -L$(FFMPEG_DIR)/libavutil -L$(FFMPEG_DIR)/libavcore
CFLAGS += -pthread
LDFLAGS += -pthread
LDLIBS += -lavformat -lavcodec -lavutil
LDLIBS += $(call ld-option, -lavcore)
LDLIBS += -lm
LDLIBS += $(call ld-option, -lz)
LDLIBS += $(call ld-option, -lbz2)
endif
endif
ifeq ($(IO), ffmpeg)
OBJS += input.o
OBJS += Chunkiser/input-stream-avs.o 
OBJS += output.o 
OBJS += out-stream-avf.o
CPPFLAGS += -I$(FFMPEG_DIR)/include
LDFLAGS += -L$(FFMPEG_DIR)/lib
CFLAGS += -pthread
LDFLAGS += -pthread
LDLIBS += -lavformat -lavcodec -lavcore -lavutil
LDLIBS += -lm
LDLIBS += $(call ld-option, -lz)
LDLIBS += $(call ld-option, -lbz2)
endif
ifeq ($(IO), httpmhd)
CPPFLAGS += -DHTTPIO_MHD
CPPFLAGS += -DHTTPIO
OBJS += $(ULPLAYER)/chunker_player/chunk_puller.o
OBJS += $(ULPLAYER)/chunker_streamer/chunk_pusher_curl.o
CPPFLAGS += -DCRAP -I$(ULPLAYER) -I$(ULPLAYER)/chunk_transcoding
CFLAGS += -pthread
LDFLAGS += -pthread
OBJS += input-http.o
OBJS += output-http.o output.o

LOCAL_MHD=$(ULPLAYER)/$(ULPLAYER_EXTERNAL_LIBS)/libmicrohttpd/temp_mhd_install
CPPFLAGS += -I$(LOCAL_MHD)/include
LDFLAGS += -L$(LOCAL_MHD)/lib
LDLIBS += $(LOCAL_MHD)/lib/libmicrohttpd.a

LOCAL_CURL=$(ULPLAYER)/$(ULPLAYER_EXTERNAL_LIBS)/curl/temp_curl_install
CPPFLAGS += -I$(LOCAL_CURL)/include
LDFLAGS += -L$(LOCAL_CURL)/lib
LDLIBS += $(LOCAL_CURL)/lib/libcurl.a -lrt
endif
ifeq ($(IO), httpevent)
CPPFLAGS += -DHTTPIO_EVENT
CPPFLAGS += -DHTTPIO
OBJS += $(ULPLAYER)/event_http/event_http_server.o
LDFLAGS += -L$(NAPA)/dclog
LDLIBS += -ldclog
OBJS += $(ULPLAYER)/chunker_streamer/chunk_pusher_curl.o
CPPFLAGS += -I$(ULPLAYER) -I$(ULPLAYER)/chunk_transcoding -I$(ULPLAYER)/event_http -DCRAP
OBJS += input-http.o
OBJS += output-http.o output.o

LOCAL_CURL=$(ULPLAYER)/$(ULPLAYER_EXTERNAL_LIBS)/curl/temp_curl_install
CPPFLAGS += -I$(LOCAL_CURL)/include
LDFLAGS += -L$(LOCAL_CURL)/lib
LDLIBS += $(LOCAL_CURL)/lib/libcurl.a -lrt
endif
ifeq ($(IO), dummy)
OBJS += input.o
OBJS += input-stream-dummy.o 
OBJS += output.o 
OBJS += out-stream-dummy.o
endif
ifeq ($(IO), udp)
OBJS += input-udp.o
OBJS += output.o
OBJS += out-stream-udp.o
endif

EXECTARGET = offerstreamer
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

all: $(EXECTARGET)

ifndef ML
$(EXECTARGET): $(OBJS) $(GRAPES)/src/net_helper.o $(EXECTARGET).o
else
$(EXECTARGET): $(OBJS) $(GRAPES)/src/net_helper-ml.o $(EXECTARGET).o
endif
	$(LINKER) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) $(LDFLAGSPOST) -o $@

$(EXECTARGET).o: streamer.o
	ln -sf streamer.o $(EXECTARGET).o

out-stream-avf.o Chunkiser/input-stream-avs.o: CPPFLAGS += -I$(FFMPEG_DIR)/include 

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
	rm -f $(EXECTARGET)
	rm -f $(GRAPES)/src/net_helper-ml.o
	rm -f $(GRAPES)/src/net_helper.o
	rm -f *.o
	rm -f Chunkiser/*.o
	rm -f $(ULPLAYER)/chunker_player/chunk_puller.o
	rm -f $(ULPLAYER)/chunker_streamer/chunk_pusher_curl.o
	rm -f $(ULPLAYER)/event_http/event_http_server.o
