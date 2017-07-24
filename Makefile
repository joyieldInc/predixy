
.PHONY : default debug clean

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
