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

SRCS = $(objs:.o=.cpp)
LINT_SRCS ?= $(SRCS)

CLANG_TIDY ?= clang-tidy
ifeq ($(shell command -v brew 2>/dev/null),)
	CLANG_TIDY_BREW :=
else
	LLVM_PREFIX := $(shell brew --prefix llvm 2>/dev/null)
	CLANG_TIDY_BREW := $(LLVM_PREFIX)/bin/clang-tidy
endif
ifeq ($(shell command -v $(CLANG_TIDY) 2>/dev/null),)
	CLANG_TIDY_CMD := $(CLANG_TIDY_BREW)
else
	CLANG_TIDY_CMD := $(CLANG_TIDY)
endif
CPPCHECK ?= cppcheck
CPPCHECK_FLAGS ?= --std=c++11 --enable=warning,performance,style --suppress=missingIncludeSystem

.PHONY : default debug clean lint cppcheck

default: $(target)


$(target): $(objs)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)

debug:
	@make LVL=-g

clean:
	@rm -rf $(objs) $(target)
	@echo Done.

lint:
	@command -v "$(CLANG_TIDY_CMD)" >/dev/null 2>&1 || { \
		echo "error: clang-tidy not found; set CLANG_TIDY or install llvm via Homebrew"; \
		exit 1; \
	}
	$(CLANG_TIDY_CMD) $(LINT_SRCS) -- $(CFLAGS) $(INCFLAGS)

cppcheck:
	@command -v "$(CPPCHECK)" >/dev/null 2>&1 || { \
		echo "error: cppcheck not found; set CPPCHECK or install cppcheck via Homebrew"; \
		exit 1; \
	}
	$(CPPCHECK) $(CPPCHECK_FLAGS) $(INCFLAGS) $(LINT_SRCS)

%.o : %.cpp
	$(CXX) $(CFLAGS) -c $^ $(INCFLAGS)

