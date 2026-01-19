
.PHONY : default debug clean test test-docker lint cppcheck docker-buildx

make = make
plt = $(shell uname)
ifeq ($(plt), FreeBSD)
	make = gmake
else ifeq ($(plt), OpenBSD)
	make = gmake
endif

IMAGE ?= predixy
TAG ?= latest
IMAGE_TAG = $(IMAGE):$(TAG)
BUILD_PLATFORMS ?= linux/amd64,linux/arm64
DOCKERFILE ?= Dockerfile
DOCKER_CONTEXT ?= .

default:
	@$(make) -C src -f Makefile

debug:
	@$(make) -C src -f Makefile debug

clean:
	@$(make) -C src -f Makefile clean

test: default
	@./test/run.sh

test-docker:
	docker build -f $(DOCKERFILE) --target test $(DOCKER_CONTEXT)

lint:
	@$(make) -C src -f Makefile lint

cppcheck:
	@$(make) -C src -f Makefile cppcheck

docker-buildx:
	docker buildx build --platform $(BUILD_PLATFORMS) \
		-t $(IMAGE_TAG) -f $(DOCKERFILE) $(DOCKER_CONTEXT)
