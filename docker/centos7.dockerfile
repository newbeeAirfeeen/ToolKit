ARG VERSION=7
FROM centos:${VERSION} As dev

ARG PROXY=""
ARG HTTP_PROXY=${PROXY}
ARG HTTPS_PROXY=${PROXY}
ARG CMAKE_VERSION=3.18
ARG CMAKE_FULL_VERSION=3.18.4

RUN yum update -y && \
    yum install -y \
        gcc \
        gcc-c++ \
        gdb \
        perf \
        kernel-devel \
        kernel-headers  \
        git \
        wget \
        make \
        openssl \
        openssl-devel

WORKDIR /opt
RUN wget -e "https_proxy=${HTTPS_PROXY}" https://cmake.org/files/v${CMAKE_VERSION}/cmake-${CMAKE_FULL_VERSION}.tar.gz \
    && tar -zxvf cmake-${CMAKE_FULL_VERSION}.tar.gz \
    && cd cmake-${CMAKE_FULL_VERSION}  \
    && ./bootstrap \
    && make -j8 \
    && make install
