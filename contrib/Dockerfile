FROM ubuntu:20.04

# Base
RUN apt-get update && apt-get install -y \
  build-essential \
  automake \
  m4

# Add source code
COPY ./ /app

# Compile
WORKDIR /app
RUN autoreconf --install
RUN ./configure && make && make install
ENTRYPOINT ["fping"]