FROM gcc:11 AS builder

WORKDIR /opt/
ADD / .
RUN make


FROM debian:bullseye-slim

COPY --from=builder /opt/src/predixy /bin/
COPY /conf/* /etc/predixy/
ENTRYPOINT ["predixy"]
CMD ["/etc/predixy/predixy.conf"]
