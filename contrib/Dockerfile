FROM alpine:latest AS builder

RUN apk add --no-cache \
  build-base \
  automake \
  autoconf \
  m4

COPY ./ /app
WORKDIR /app
RUN autoreconf --install && ./configure && make

FROM alpine:latest

# OCI annotations
LABEL org.opencontainers.image.title="fping" \
      org.opencontainers.image.authors="fping Community" \
      org.opencontainers.image.description="High performance ping tool" \
      org.opencontainers.image.base.name="docker.io/library/alpine:latest" \
      org.opencontainers.image.source="https://github.com/schweikert/fping" \
      org.opencontainers.image.documentation="https://fping.org/fping.8.html"

RUN apk add --no-cache mandoc

COPY --from=builder /app/src/fping /usr/local/bin/fping
COPY --from=builder /app/doc/fping.8 /usr/share/man/man8/fping.8

ENTRYPOINT ["fping"]
