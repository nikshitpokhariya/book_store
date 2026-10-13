# ========================================================
# BookBazzar Backend - Multi-Stage Production Dockerfile
# Optimized for Linux Render Web Service deployment
# Build Context: repository root (.)
# Dockerfile Path: backend/Dockerfile
# ========================================================

# --- STAGE 1: Builder ---
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/opt/vcpkg

# Limit concurrency to 1-2 build jobs to prevent exceeding Render's 8GB RAM ceiling
ARG BUILD_CONCURRENCY=2
ENV VCPKG_MAX_CONCURRENCY=${BUILD_CONCURRENCY}
ENV CMAKE_BUILD_PARALLEL_LEVEL=${BUILD_CONCURRENCY}

# Install build prerequisites and native compilation tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    curl \
    zip \
    unzip \
    tar \
    autoconf \
    automake \
    libtool \
    python3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Clone shallow vcpkg repo to reduce disk and memory overhead
RUN git clone --depth=1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /app/backend

# Copy vcpkg manifest and custom Release-only triplet first for layer caching
COPY backend/vcpkg.json /app/backend/
COPY backend/triplets/ /app/backend/triplets/

# Configure base triplet to Release-only to avoid compiling unneeded Debug binaries
RUN cp /app/backend/triplets/x64-linux.cmake "$VCPKG_ROOT/triplets/community/x64-linux.cmake" \
    && cp /app/backend/triplets/x64-linux.cmake "$VCPKG_ROOT/triplets/x64-linux.cmake"

# Pre-install dependencies in manifest mode with strictly capped concurrency (1-2)
RUN "$VCPKG_ROOT/vcpkg" install \
    --triplet x64-linux \
    --overlay-triplets=/app/backend/triplets \
    --clean-after-build

# Copy backend source tree
COPY backend/ /app/backend/

# Configure and compile Release executable with capped parallel jobs
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DVCPKG_OVERLAY_TRIPLETS=/app/backend/triplets \
    -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel ${BUILD_CONCURRENCY} \
    && strip /app/backend/build/BookExchangeBackend

# Collect dynamic shared libraries (.so) produced by vcpkg
RUN mkdir -p /app/backend/lib && \
    for d in /app/backend/vcpkg_installed/x64-linux/lib /app/backend/build/vcpkg_installed/x64-linux/lib; do \
      if [ -d "$d" ]; then \
        find "$d" -maxdepth 1 -name "*.so*" -exec cp -d {} /app/backend/lib/ \; ; \
      fi; \
    done

# --- STAGE 2: Minimal Runtime ---
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install essential runtime dependencies (SSL certificates, curl fallback, common shared libs)
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    libssl3 \
    zlib1g \
    libuuid1 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app/backend

# Copy shared libraries from builder and register with dynamic linker
COPY --from=builder /app/backend/lib/ /usr/local/lib/
RUN ldconfig

# Copy application binary, Drogon config, assets, and startup script
COPY --from=builder /app/backend/build/BookExchangeBackend /app/backend/BookExchangeBackend
COPY --from=builder /app/backend/config.json /app/backend/config.json
COPY --from=builder /app/backend/public /app/backend/public
COPY --from=builder /app/backend/start.sh /app/backend/start.sh

# Ensure startup script has Unix line endings and executable permissions
RUN sed -i 's/\r$//' /app/backend/start.sh && chmod +x /app/backend/start.sh

# Render binds dynamically to the PORT environment variable (default: 10000 on Render)
ENV PORT=10000
ENV LD_LIBRARY_PATH=/usr/local/lib:${LD_LIBRARY_PATH:-}

EXPOSE 10000

CMD ["/app/backend/start.sh"]
