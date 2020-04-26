FROM gcc:8.4

RUN mkdir -p /usr/src/jtop
WORKDIR /usr/src/jtop

COPY . /usr/src/jtop

RUN apt-get update && \
  apt-get -y install cmake && \
  cmake -B docker-build && cd docker-build && \
  make && \
  cp jtop /usr/bin/ && \
  rm -rf /usr/src/jtop

