ARG ubuntu_version=latest

FROM ubuntu:${ubuntu_version}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update \
  && apt install -y \
  ninja-build \
  build-essential \
  libcurl4-openssl-dev \
  wget \
  && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.22.1/cmake-3.22.1-linux-x86_64.sh -qO /tmp/cmake-install.sh \
  && chmod u+x /tmp/cmake-install.sh \
  && /tmp/cmake-install.sh --skip-license --prefix=/usr/local \
  && rm /tmp/cmake-install.sh

COPY . .

RUN mkdir build \
  && cd build \
  && mkdir artifacts \
  && cmake .. -G "Ninja" -DCMAKE_INSTALL_PREFIX=artifacts \
  && ninja -j$(nproc) \
  && ninja install
