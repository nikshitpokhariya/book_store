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

# Clone and bootstrap vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /app/backend

# Copy vcpkg manifest first for layer caching
COPY backend/vcpkg.json /app/backend/

# Pre-install dependencies in manifest mode
RUN "$VCPKG_ROOT/vcpkg" install --triplet x64-linux

# Copy backend source tree
COPY backend/ /app/backend/

# Configure and compile Release executable
RUN cmake -S . -B build -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel 2 \
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
