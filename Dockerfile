# Build C!

FROM debian:testing

MAINTAINER Joseph Coffland <joseph@cauldrondevelopment.com>

# Get the prerequisites
RUN apt-get update && \
  apt-get install -y scons git build-essential libssl-dev \
  libboost-iostreams-dev libboost-system-dev libboost-filesystem-dev \
  libboost-regex-dev libv8-dev

# Move the source into the image
ADD . /opt/cbang
WORKDIR /opt/cbang
ENV CBANG_HOME /opt/cbang

# Build it
RUN scons -j 4

