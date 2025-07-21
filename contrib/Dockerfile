FROM alpine:3.22.1 AS builder

RUN apk add --no-cache \
  build-base \
  automake \
  autoconf \
  m4

COPY ./ /app
WORKDIR /app
RUN autoreconf --install && ./configure && make

FROM alpine:3.22.1

LABEL org.opencontainers.image.title="fping" \
      org.opencontainers.image.description="High performance ping tool " \
      org.opencontainers.image.source="https://github.com/schweikert/fping"

COPY --from=builder /app/src/fping /usr/local/bin/fping

ENTRYPOINT ["fping"]
