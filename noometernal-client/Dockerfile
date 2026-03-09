FROM alpine:latest

RUN apk add --no-cache \
    gcc \
    g++ \
    musl-dev \
    make \
    cmake \
    ffmpeg \
    ffmpeg-dev \
    git \
    curl \
    pkgconfig \
    linux-headers \
    xorg-server-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    doxygen \
    graphviz

COPY . /client-code

WORKDIR /client-code