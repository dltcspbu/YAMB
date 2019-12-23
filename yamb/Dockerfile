FROM dltc/libsourcey-base:latest

WORKDIR /app

RUN apt-get install -yq libboost-all-dev g++-8

RUN rm /usr/bin/gcc
RUN rm /usr/bin/g++
RUN ln -s /usr/bin/gcc-8 /usr/bin/gcc
RUN ln -s /usr/bin/g++-8 /usr/bin/g++

RUN cp -r /vendor/webrtc-28114-9863f3d-linux-x64 /app

ADD . /app

RUN cp /vendor/libuv/out/cmake/libuv_a.a /app/libuv.a
RUN cp /vendor/webrtc-28114-9863f3d-linux-x64/lib/x64/Debug/libwebrtc_full.a /app/libwebrtc.a

RUN cmake . && make -j 12

RUN chmod +x start.sh

CMD ["./start.sh"]
