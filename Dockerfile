FROM debian:8.6

MAINTAINER Guy John <slowradio@rumblesan.com>

RUN apt-get update
RUN apt-get install -y clang cmake make git

RUN apt-get install -y libfftw3-dev

RUN apt-get install -y libshout3-dev libconfig-dev libvorbis-dev libsndfile-dev
ENV CC /usr/bin/clang

RUN git clone https://github.com/rumblesan/bclib.git /opt/bclib
RUN cd /opt/bclib/build && cmake .. && make && make install

RUN git clone https://github.com/rumblesan/libpstretch.git /opt/libpstretch
RUN cd /opt/libpstretch/build && cmake .. && make && make install


COPY . /opt/slowradio
RUN cd /opt/slowradio/build; cmake ..; make; make install

WORKDIR /opt/slowradio
CMD ["slow", "/opt/cfg/radio.cfg"]
