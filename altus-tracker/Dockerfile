FROM ubuntu:24.04 AS builder

RUN apt-get update && \
  apt-get -y upgrade && \
  apt-get install -y \
    build-essential \
    cmake \
    gnuradio-dev \
    gr-osmosdr \
    libosmosdr-dev \
    libboost-all-dev

WORKDIR /src

COPY . .

WORKDIR /src/build

RUN cmake .. && make -j$(nproc) && make DESTDIR=/newroot install

FROM ubuntu:24.04
RUN apt-get update && apt-get -y upgrade && apt-get install -y gnuradio gr-osmosdr

COPY --from=builder /newroot /

RUN mkdir -p /etc/gnuradio/conf.d/ && echo 'log_level = warn' >> /etc/gnuradio/conf.d/gnuradio-runtime.conf && ldconfig
WORKDIR /app

COPY run.sh .

ENV HOME=/tmp

CMD [ "/app/run.sh" ]
