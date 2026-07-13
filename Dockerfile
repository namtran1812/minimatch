# ============================================================
# Build stage
# ============================================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
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
    -DMINIMATCH_BUILD_TESTS=OFF

RUN cmake --build build \
    --target minimatch_web \
    -j2

# ============================================================
# Runtime stage
# ============================================================
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PORT=8080

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-system1.83.0 \
    libboost-thread1.83.0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /src/build/minimatch_web ./build/minimatch_web
COPY frontend ./frontend
COPY scripts/start_web.sh ./scripts/start_web.sh

RUN chmod +x ./scripts/start_web.sh

EXPOSE 8080

CMD ["./scripts/start_web.sh"]
