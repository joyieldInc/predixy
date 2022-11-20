FROM alpine

COPY output/conf /app/conf
COPY output/predixy /app/predixy

WORKDIR /app

# RUN cat /configs/default.toml
CMD ./prefixy conf/predixy.conf