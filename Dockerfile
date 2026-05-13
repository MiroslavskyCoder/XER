ARG XER_BASE_IMAGE=ubuntu:24.04
FROM ${XER_BASE_IMAGE} AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN sed -i 's/Components: main/Components: main universe multiverse/g' /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
      build-essential \
      ca-certificates \
      cmake \
      ninja-build \
      pkg-config \
      curl \
      git \
      clang-18 \
      lld-18 \
      llvm-18-dev \
      libclang-18-dev \
      libabsl-dev \
      libxml2-dev \
      libcurl4-openssl-dev \
      libssl-dev \
      zlib1g-dev \
      libnode-dev \
      nodejs \
      libeigen3-dev \
      libpthreadpool-dev \
      libuv1-dev \
      libjsoncpp-dev \
      libre2-dev \
      librange-v3-dev \
      libavutil-dev \
      libavcodec-dev \
      libavformat-dev \
      libswscale-dev \
      libswresample-dev \
      libzstd-dev \
      libbrotli-dev \
      libsqlite3-dev \
      libicu-dev \
      libffi-dev \
      libfftw3-dev \
    && ln -sf /usr/bin/clang-18 /usr/local/bin/clang \
    && ln -sf /usr/bin/clang++-18 /usr/local/bin/clang++ \
    && rm -rf /var/lib/apt/lists/*

ENV CC=clang-18 \
    CXX=clang++-18 \
    LLVM_PREFIX=/usr/lib/llvm-18 \
    LLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm \
    Clang_DIR=/usr/lib/llvm-18/lib/cmake/clang \
    CMAKE_PREFIX_PATH=/usr/lib/llvm-18

WORKDIR /xer
COPY . .

ARG ENABLE_CUDA=OFF
ARG ENABLE_CUDNN=OFF
ARG CMAKE_CUDA_ARCHITECTURES=75;86
RUN cmake --preset default \
      -DLLVM_DIR="${LLVM_DIR}" \
      -DClang_DIR="${Clang_DIR}" \
      -DENABLE_CUDA="${ENABLE_CUDA}" \
      -DENABLE_CUDNN="${ENABLE_CUDNN}" \
      -DCMAKE_CUDA_ARCHITECTURES="${CMAKE_CUDA_ARCHITECTURES}" \
    && cmake --build --preset default -- -j"$(nproc)" \
    && install -m 0755 out/build/default/XER /usr/local/bin/XER

ENTRYPOINT ["/usr/local/bin/XER"]
CMD ["version"]
