CXX ?= g++
LVL ?= -g -O3
Opts += $(LVL)

ifeq ($(MT), false)
	Opts += -D_PREDIXY_SINGLE_THREAD_
endif

ifeq ($(TS), true)
	Opts += -D_PREDIXY_TIMER_STATS_
endif

EV ?= auto

LDLIBCPP = -static-libstdc++

ifeq ($(EV), auto)
	plt = $(shell uname)
	ifeq ($(plt), Linux)
		EV = epoll
		Opts += -D_PREDIXY_BACKTRACE_
	else ifeq ($(plt), Darwin)
		EV = kqueue
		Opts += -D_PREDIXY_BACKTRACE_
		LDLIBCPP = -static-libstdc++
	else ifeq ($(plt), FreeBSD)
		EV = kqueue
		Opts += -D_PREDIXY_BACKTRACE_
		LDFLAGS += $(shell pkg info -Dx gcc|grep 'rpath') -lexecinfo
	else ifeq ($(plt), OpenBSD)
		EV = kqueue
	else
		EV = poll
	endif
endif

ifeq ($(EV), epoll)
	multiplexor = EpollMultiplexor
	Opts += -D_EPOLL_
else ifeq ($(EV), poll)
	multiplexor = PollMultiplexor
	Opts += -D_POLL_
else ifeq ($(EV), kqueue)
	multiplexor = KqueueMultiplexor
	Opts += -D_KQUEUE_
else
$(error Unknown event:$(EV))
endif

CFLAGS = -std=c++11 -Wall -w $(Opts)
INCFLAGS =
LDFLAGS += $(LDLIBCPP) -rdynamic -lpthread


target = predixy
objs = \
	 Crc16.o \
	 HashFunc.o \
	 Timer.o \
	 Logger.o \
	 LogFileSink.o \
	 Alloc.o \
	 Socket.o \
	 ListenSocket.o \
	 AcceptSocket.o \
	 ConnectSocket.o \
	 $(multiplexor).o \
	 Subscribe.o \
	 Connection.o \
	 AcceptConnection.o \
	 ConnectConnection.o \
	 Buffer.o \
	 Command.o \
	 Distribution.o \
	 Enums.o \
	 Reply.o \
	 ConfParser.o \
	 Conf.o \
	 Auth.o \
	 DC.o \
	 LatencyMonitor.o \
	 RequestParser.o \
	 Request.o \
	 ResponseParser.o \
	 Response.o \
	 Server.o \
	 ServerGroup.o \
	 ServerPool.o \
	 ClusterNodesParser.o \
	 ClusterServerPool.o \
	 StandaloneServerPool.o \
	 ConnectConnectionPool.o \
	 Handler.o \
	 Proxy.o \
	 main.o

.PHONY : default debug clean

default: $(target)


$(target): $(objs)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

debug:
	@make LVL=-g

clean:
	@rm -rf $(objs) $(target)
	@echo Done.

%.o : %.cpp
	$(CXX) $(CFLAGS) -c $^ $(INCFLAGS)

