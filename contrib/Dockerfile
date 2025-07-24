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

# OCI annotations
LABEL org.opencontainers.image.title="fping" \
      org.opencontainers.image.authors="fping Community" \
      org.opencontainers.image.description="High performance ping tool" \
      org.opencontainers.image.base.name="docker.io/library/alpine:3.22.1" \
      org.opencontainers.image.source="https://github.com/schweikert/fping" \
      org.opencontainers.image.documentation="https://fping.org/fping.8.html"

COPY --from=builder /app/src/fping /usr/local/bin/fping

ENTRYPOINT ["fping"]
