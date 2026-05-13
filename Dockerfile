FROM ubuntu:22.04 AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y software-properties-common ca-certificates \
    && add-apt-repository -y universe \
    && add-apt-repository -y multiverse \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
      build-essential \
      cmake \
      ninja-build \
      pkg-config \
      curl \
      git \
      clang-14 \
      lld-14 \
      llvm-14-dev \
      libclang-14-dev \
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
      libavfilter-dev \
      libavdevice-dev \
      libopencv-dev \
      libzstd-dev \
      libbrotli-dev \
      libsqlite3-dev \
      libicu-dev \
      libffi-dev \
      libfftw3-dev \
      nvidia-cuda-toolkit \
    && ln -sf /usr/bin/clang-14 /usr/local/bin/clang \
    && ln -sf /usr/bin/clang++-14 /usr/local/bin/clang++ \
    && if [ ! -e /usr/local/cuda ]; then ln -s /usr /usr/local/cuda; fi \
    && rm -rf /var/lib/apt/lists/*

ENV CC=clang-14 \
    CXX=clang++-14 \
    CUDAToolkit_ROOT=/usr/local/cuda \
    CUDA_HOME=/usr/local/cuda \
    CUDA_PATH=/usr/local/cuda \
    LLVM_PREFIX=/usr/lib/llvm-14 \
    LLVM_DIR=/usr/lib/llvm-14/lib/cmake/llvm \
    Clang_DIR=/usr/lib/llvm-14/lib/cmake/clang \
    CMAKE_PREFIX_PATH=/usr/lib/llvm-14

WORKDIR /xer
COPY . .

ARG CMAKE_CUDA_ARCHITECTURES=75;86
RUN cmake --preset default \
      -DLLVM_DIR="${LLVM_DIR}" \
      -DClang_DIR="${Clang_DIR}" \
      -DCUDAToolkit_ROOT="${CUDAToolkit_ROOT}" \
      -DCMAKE_CUDA_ARCHITECTURES="${CMAKE_CUDA_ARCHITECTURES}" \
    && cmake --build --preset default -- -j"$(nproc)" \
    && install -m 0755 out/build/default/XER /usr/local/bin/XER

ENTRYPOINT ["/usr/local/bin/XER"]
CMD ["version"]
