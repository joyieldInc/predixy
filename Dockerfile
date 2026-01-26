#-----------------------------------------------------------
    FROM --platform=$BUILDPLATFORM debian:trixie-slim AS build

WORKDIR /src/predixy
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
    && rm -rf /var/lib/apt/lists/*

#-----------------------------------------------------------
FROM build AS bin
COPY src src
COPY Makefile .
RUN make clean && make

#-----------------------------------------------------------
FROM build AS test

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        python3 \
        python3-venv \
        pipx \
        redis-server \
    && rm -rf /var/lib/apt/lists/*
RUN pipx install uv \
    && ln -s /root/.local/bin/uv /usr/local/bin/uv
COPY src src
COPY test test
COPY conf conf
COPY pyproject.toml uv.lock Makefile .
RUN make clean && make -j8
RUN make test

#-----------------------------------------------------------
FROM debian:trixie-slim

RUN useradd -r -s /usr/sbin/nologin predixy
WORKDIR /etc/predixy

COPY --from=bin /src/predixy/src/predixy /usr/local/bin/predixy
COPY --from=bin /src/predixy/conf/ /etc/predixy/

EXPOSE 7617
USER predixy
ENTRYPOINT ["/usr/local/bin/predixy", "/etc/predixy/predixy.conf"]
