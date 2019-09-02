FROM ubuntu:18.04 as binary
RUN apt-get update \
	&& apt-get install -y build-essential \
	&& apt-get clean

ADD . /predixy
WORKDIR /predixy

RUN make

FROM frolvlad/alpine-glibc:alpine-3.9_glibc-2.29

COPY --from=binary /predixy/src/predixy /usr/local/bin
COPY conf /predixy/conf

ENTRYPOINT ["/usr/local/bin/predixy", "/predixy/conf/predixy.conf"]
