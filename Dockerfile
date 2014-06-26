# Build C!

FROM debian

MAINTAINER Joseph Coffland <joseph@cauldrondevelopment.com>

RUN apt-get update
RUN apt-get install scons git build-essential libssl-dev \
  libboost-iostreams-dev libboost-system-dev libboost-filesystem-dev \
  libboost-regex-dev libv8-dev

RUN scons
