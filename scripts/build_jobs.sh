#!/usr/bin/env bash
# ============================================================================
# Build parallelism helper for StreamDAB-Analyser
#
# Why: `-j$(nproc)` on a many-core box spawns one full g++ per core; with the
# project's `-O3 -march=native` Release flags each compiler can hold 1-2 GB, so
# a 16-core/32 GB machine can thrash (and recursive sub-makes -- e.g. the
# Qt-ADS FetchContent subproject -- can add more).
#
# Usage -- EITHER of these forms is safe:
#   (a) execute it:  cmake --build build --parallel "$(bash scripts/build_jobs.sh)"
#   (b) source it:
#       . "$(dirname "$0")/build_jobs.sh"
#       JOBS=$(streamdab_build_jobs)
#       export CMAKE_BUILD_PARALLEL_LEVEL="$JOBS"   # nested cmake/make honour this
#       cmake --build build --parallel "$JOBS"
#
# SAFETY: executing this script MUST always print a positive integer. A bare
# `-j` / `--parallel` with no value means "unbounded" to make, which forks one
# compiler per target and freezes the machine -- that is exactly the failure
# this file exists to prevent. Never use `-j$(...)` with a command that can
# print nothing.
#
# Overrides (highest first):
#   $JOBS                      explicit value from any caller
#   $CMAKE_BUILD_PARALLEL_LEVEL
#   STREAMDAB_MAX_JOBS         cap (default 8)
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

    cap=${STREAMDAB_MAX_JOBS:-8}
    by_mem=$(( mem_gb / 2 ))          # ~2 GB per compiler job
    [ "$by_mem" -lt 1 ] && by_mem=1

    jobs=$cpus
    [ "$by_mem" -lt "$jobs" ] && jobs=$by_mem
    [ "$jobs" -gt "$cap" ] && jobs=$cap
    # Belt and braces: never emit 0/empty (a bare -j is unbounded).
    case "$jobs" in
        ''|*[!0-9]*) jobs=1 ;;
    esac
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

# When EXECUTED (not sourced), print the resolved job count so that
# "$(bash scripts/build_jobs.sh)" -- and even "$(sh scripts/build_jobs.sh)"
# -- always prints a positive number. Sources via the bash API instead.
if [ -z "${BASH_SOURCE:-}" ] || [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    streamdab_build_jobs
fi
