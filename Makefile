
.PHONY : default debug clean test lint cppcheck

make = make
plt = $(shell uname)
ifeq ($(plt), FreeBSD)
	make = gmake
else ifeq ($(plt), OpenBSD)
	make = gmake
endif

default:
	@$(make) -C src -f Makefile

debug:
	@$(make) -C src -f Makefile debug

clean:
	@$(make) -C src -f Makefile clean

test: default
	@./test/run.sh

lint:
	@$(make) -C src -f Makefile lint

cppcheck:
	@$(make) -C src -f Makefile cppcheck
