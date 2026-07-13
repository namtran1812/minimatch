# ============================================================
# Build stage
# ============================================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libboost-dev \
    libboost-system-dev \
    libboost-thread-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY CMakeLists.txt ./
COPY include ./include
COPY src ./src
COPY apps ./apps
COPY tests ./tests

RUN cmake \
    -S . \
    -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DMINIMATCH_BUILD_TESTS=OFF \
    -DMINIMATCH_BUILD_BENCHMARKS=OFF

RUN cmake --build build \
    --target minimatch_web \
    -j2

# ============================================================
# Runtime stage
# ============================================================
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PORT=10000
ENV LIVE_PROVIDER=coinbase
ENV LIVE_SYMBOL=btcusd

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-dev \
    libboost-system-dev \
    libboost-thread-dev \
    python3 \
    python3-pip \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /src/build/minimatch_web ./build/minimatch_web

COPY frontend ./frontend
COPY live_feed ./live_feed
COPY scripts/start_hosted.sh ./scripts/start_hosted.sh

RUN python3 -m pip install --break-system-packages \
    aiohttp \
    websockets

RUN chmod +x ./scripts/start_hosted.sh

EXPOSE 10000

CMD ["./scripts/start_hosted.sh"]
