
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

deb:
	@$(make) -C src -f Makefile
	mkdir -p package/predixy_1.0.5-1_amd64/etc/predixy/conf.d
	mkdir -p package/predixy_1.0.5-1_amd64/usr/bin
	mkdir -p package/predixy_1.0.5-1_amd64/var/log/predixy
	cp src/predixy package/predixy_1.0.5-1_amd64/usr/bin
	cp -r conf/* package/predixy_1.0.5-1_amd64/etc/predixy/
	dpkg-deb --build package/predixy_1.0.5-1_amd64/
