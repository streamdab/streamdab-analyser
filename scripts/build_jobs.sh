#!/usr/bin/env bash
# ============================================================================
# Build parallelism helper for StreamDAB-Analyzer
#
# Why: `-j$(nproc)` on a many-core box spawns one full g++ per core; with the
# project's `-O3 -march=native` Release flags each compiler can hold 1-2 GB, so
# a 16-core/32 GB machine can thrash (and recursive sub-makes -- e.g. the
# Qt-ADS FetchContent subproject -- can add more).
#
# Usage (source it):
#   . "$(dirname "$0")/build_jobs.sh"
#   JOBS=$(streamdab_build_jobs)
#   export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"   # nested cmake/make honour this
#   cmake --build build --parallel "$JOBS"
#
# Overrides (highest first):
#   $JOBS                      explicit value from any caller
#   $CMAKE_BUILD_PARALLEL_LEVEL
#   STREAMDAB_MAX_JOBS         cap (default 12)
# ============================================================================

streamdab_build_jobs() {
    # 1) explicit override from the caller
    if [ -n "${JOBS:-}" ]; then
        printf '%s\n' "$JOBS"
        return 0
    fi
    # 2) cmake's own parallel level
    if [ -n "${CMAKE_BUILD_PARALLEL_LEVEL:-}" ]; then
        printf '%s\n' "$CMAKE_BUILD_PARALLEL_LEVEL"
        return 0
    fi

    local cpus mem_gb cap by_mem jobs
    cpus=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

    if [ -r /proc/meminfo ]; then
        mem_gb=$(( $(awk '/^MemTotal:/{print $2}' /proc/meminfo) / 1024 / 1024 ))
    elif command -v sysctl >/dev/null 2>&1; then
        mem_gb=$(( $(sysctl -n hw.memsize 2>/dev/null || echo 8589934592) / 1024 / 1024 / 1024 ))
    else
        mem_gb=8
    fi

    cap=${STREAMDAB_MAX_JOBS:-12}
    by_mem=$(( mem_gb / 2 ))          # ~2 GB per compiler job
    [ "$by_mem" -lt 1 ] && by_mem=1

    jobs=$cpus
    [ "$by_mem" -lt "$jobs" ] && jobs=$by_mem
    [ "$jobs" -gt "$cap" ] && jobs=$cap
    [ "$jobs" -lt 1 ] && jobs=1

    printf '%s\n' "$jobs"
}

# Convenience: export the resolved value for nested cmake/make invocations.
streamdab_export_build_jobs() {
    local jobs
    jobs=$(streamdab_build_jobs)
    export CMAKE_BUILD_PARALLEL_LEVEL="$jobs"
    printf '%s\n' "$jobs"
}
