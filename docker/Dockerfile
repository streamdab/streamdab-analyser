# ============================================================================
# StreamDAB-Analyser — canonical container build (Qt 6 / CMake / headless-CI)
#
# Canonical build instead of the historical Dockerfile.build (which predates
# the Qt-ADS migration and fails on current tree: missing libzmq3-dev, no
# install() rules, legacy binary name, Qt-for-Windows toolchain absent).
#
# Stages:
#   build   — deps + cmake configure + make + (optional) ctest
#   runtime — minimal image with the two binaries (GUI + CLI)
#
# Usage:
#   docker build -t streamdab:build --target build -f docker/Dockerfile .
#   docker build -t streamdab:latest     -f docker/Dockerfile .
#   # run CLI on a capture (fixtures are gitignored, mount them):
#   docker run --rm -v $PWD/eti:/data streamdab:latest streamdab-cli \
#       --input /data/bkk_20062022_141637.eti --output /data/out.yaml
#   # GUI needs a display: -e QT_QPA_PLATFORM=offscreen for headless smoke.
#   # Run the full test suite (fixtures required):
#   docker run --rm -v $PWD:/src -w /src -e QT_QPA_PLATFORM=offscreen \
#       streamdab:build sh -c "cmake -B /tmp/bld -S /src && cmake --build /tmp/bld -j\$(nproc) && (cd /tmp/bld && ctest --output-on-failure)"
# ============================================================================

FROM ubuntu:24.04 AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV QT_QPA_PLATFORM=offscreen

# Build deps. Qt 6.4.2 (24.04) is newer than nothing — the reference build is
# Qt 6.11 (host); 6.4.2 is validated by this container build.
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    ca-certificates \
    qt6-base-dev \
    qt6-base-private-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libfaad-dev \
    libfftw3-dev \
    libzmq3-dev \
    libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

# build stage: configure + compile + (optionally) test.
# RUN_CTEST defaults OFF because eti/ fixtures are gitignored and absent from
# the build context; set ON when fixtures are present (e.g. CI with secrets
# mounted) to run the full 49-test suite.
FROM base AS build

ARG RUN_CTEST=OFF
WORKDIR /workspace
COPY . /workspace/

RUN cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j$(nproc) \
    && if [ "$RUN_CTEST" = "ON" ]; then cd build && ctest --output-on-failure; fi

# runtime stage: minimal image, two binaries + Qt runtime libs.
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive
ENV QT_QPA_PLATFORM=offscreen

RUN apt-get update && apt-get install -y --no-install-recommends \
    libqt6core6 \
    libqt6gui6 \
    libqt6widgets6 \
    libqt6network6 \
    libqt6test6t64 \
    libfaad2 \
    libfftw3-double3 \
    libzmq5 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /workspace/build/StreamDABAnalyser /usr/local/bin/StreamDABAnalyser
COPY --from=build /workspace/build/streamdab-cli /usr/local/bin/streamdab-cli

WORKDIR /data
CMD ["streamdab-cli", "--help"]