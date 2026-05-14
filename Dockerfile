FROM ubuntu:22.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    g++ cmake make \
    libmysqlclient-dev \
    libhiredis-dev \
    libboost-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# 构建 Muduo
RUN git clone --depth 1 https://github.com/chenshuo/muduo /tmp/muduo && \
    cd /tmp/muduo && sed -i 's/-Werror//' CMakeLists.txt && \
    sed -i 's/-Wold-style-cast/-Wno-old-style-cast/' CMakeLists.txt && \
    mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local .. && \
    make muduo_net muduo_base -j4 && \
    make install && \
    rm -rf /tmp/muduo

WORKDIR /app
COPY . .

RUN mkdir -p build && cd build && \
    cmake .. && make -j4

EXPOSE 6000
CMD ["./bin/ChatServer", "0.0.0.0", "6000"]
