FROM ubuntu:16.04

RUN apt-get update && \
  apt-get install -y software-properties-common && \
        add-apt-repository -y ppa:ubuntu-toolchain-r/test


# Install dependencies
RUN apt-get update && apt-get install -yq \
  build-essential \
  pkg-config \
  git \
  curl \
  libx11-dev \
  libglu1-mesa-dev \
  gcc-8 \
  g++-8 \
  wget cmake

RUN rm /usr/bin/gcc
RUN rm /usr/bin/g++
RUN ln -s /usr/bin/gcc-8 /usr/bin/gcc
RUN ln -s /usr/bin/g++-8 /usr/bin/g++

RUN mkdir -p /vendor

RUN cd /vendor && git clone https://github.com/sourcey/webrtc-builds && cd webrtc-builds && ./build.sh -c x64 -l jsoncpp -e '1' -r 9863f3d246e2da7a2e1f42bbc5757f6af5ec5682 -d '1' \
    && cp -r ./out/src/third_party/abseil-cpp ./out/webrtc-28114-9863f3d-linux-x64/include/third_party/ \
    && mv ./out/webrtc-28114-9863f3d-linux-x64 ../ && cd .. && rm -r webrtc-builds

RUN version=3.11 \
    && build=3 \
    && mkdir ~/temp \
    && cd ~/temp \
    && wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz \
    && tar -xzvf cmake-$version.$build.tar.gz \
    && cd cmake-$version.$build/ \
    && ./bootstrap \
    && make -j40 \
    && sudo make install \
    && cmake --version

RUN cd /vendor && git clone https://github.com/libuv/libuv \
    && cd libuv \
    && sed -i 's/-fvisibility=hidden/-fPIC -fvisibility=hidden/g' CMakeLists.txt \
    && mkdir -p out/cmake ; cd out/cmake \
    && cmake ../.. -DBUILD_TESTING=ON \
    && cmake --build .

