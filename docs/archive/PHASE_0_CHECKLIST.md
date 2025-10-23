# Phase 0: Critical Fixes - Implementation Checklist
## 3-4 Day Sprint to Fix Blocking Issues

**Start Date:** TBD
**Target Completion:** 3-4 days
**Status:** Ready to Start

---

## DAY 1: MEMORY SAFETY FIXES

### Morning: LockFreeQueue Memory Leak (CRITICAL-001)

**File:** `src/network/streaming_processor.cpp`
**Estimated Time:** 4 hours

#### Tasks:
- [ ] Add destructor to `LockFreeQueue` class
  - [ ] Drain remaining items
  - [ ] Delete all nodes in chain
  - [ ] Clean up retired nodes vector

- [ ] Implement epoch-based memory reclamation in `pop()`
  - [ ] Add `m_pop_epoch` atomic counter
  - [ ] Add `m_retired_nodes` vector with mutex
  - [ ] Implement `retire_node()` method
  - [ ] Implement `cleanup_retired_nodes()` method
  - [ ] Replace direct delete with deferred deletion

- [ ] Fix ABA problem
  - [ ] Use `compare_exchange_strong` for head update
  - [ ] Add memory ordering annotations
  - [ ] Protect against pointer reuse

#### Verification:
```bash
# Build and test
cd build
make

# Run with Valgrind
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes \
         ./DABAnalyzer_Enhanced_FilteringBuild

# Check for "definitely lost" - should be 0 bytes
```

- [ ] Valgrind shows 0 bytes leaked
- [ ] No "still reachable" errors
- [ ] Application runs without crashes

---

### Afternoon: Circular Buffer Alignment (CRITICAL-002)

**File:** `src/network/circular_buffer_manager.cpp`
**Estimated Time:** 3 hours

#### Tasks:
- [ ] Replace `uint8_t[]` allocation with `std::aligned_alloc`
  - [ ] Add `#include <cstdlib>` for `aligned_alloc`
  - [ ] Calculate alignment: `alignof(BufferedETIFrame)`
  - [ ] Calculate size: `capacity * sizeof(BufferedETIFrame)`
  - [ ] Allocate aligned memory

- [ ] Update deleter to use `std::free` instead of `delete[]`
  - [ ] Create custom deleter struct
  - [ ] Store capacity for destructor loop
  - [ ] Call destructors explicitly

- [ ] Initialize objects with placement new
  - [ ] Loop through allocated memory
  - [ ] Call placement new for each object
  - [ ] Store capacity for later cleanup

- [ ] Update all `reinterpret_cast` to proper alignment
  - [ ] Verify cast is safe (alignment checked)
  - [ ] Add comments explaining alignment guarantee

#### Verification:
```bash
# Build with AddressSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address" \
      ..
make

# Run test
./DABAnalyzer_Enhanced_FilteringBuild

# Should see no alignment warnings
```

- [ ] AddressSanitizer clean
- [ ] Test on different buffer sizes
- [ ] No alignment warnings in logs

---

## DAY 2: THREAD SAFETY FIXES

### Morning: Multi-Stream Processor (CRITICAL-005)

**File:** `src/core/multi_stream_processor.cpp`
**Estimated Time:** 4 hours

#### Tasks:
- [ ] Create `WorkerContext` struct
  - [ ] Add `unique_ptr<QThread>`
  - [ ] Add `unique_ptr<StreamProcessingWorker>`
  - [ ] Add stream ID field

- [ ] Update `addStream()` method
  - [ ] Create context with both thread and worker
  - [ ] Connect signals BEFORE starting thread
  - [ ] Use `Qt::DirectConnection` for cleanup
  - [ ] Store context in map
  - [ ] Start thread after setup complete

- [ ] Implement proper `stopAllStreams()`
  - [ ] Call `quit()` on each thread
  - [ ] Wait with timeout (5 seconds)
  - [ ] Terminate if doesn't stop
  - [ ] Clear workers map

- [ ] Add destructor
  - [ ] Call `stopAllStreams()`
  - [ ] Ensure all threads stopped

#### Verification:
```bash
# Build with ThreadSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread" \
      ..
make

# Run with multiple streams
./DABAnalyzer_Enhanced_FilteringBuild

# ThreadSanitizer should report no data races
```

- [ ] ThreadSanitizer clean
- [ ] Rapid start/stop test passes
- [ ] Multiple simultaneous streams work

---

### Afternoon: Signal Connection Race (HIGH-002)

**File:** `src/main.cpp`
**Estimated Time:** 2 hours

#### Tasks:
- [ ] Remove all `QTimer::singleShot` delayed connections
  - [ ] Search for pattern: `QTimer::singleShot.*connect`
  - [ ] Replace with immediate connections
  - [ ] Use `Qt::QueuedConnection` for cross-thread

- [ ] Add proper destructor to `DABAnalyzerWindow`
  - [ ] Stop ETI processor
  - [ ] Stop multi-stream processor
  - [ ] Disconnect all signals
  - [ ] Brief wait for threads (100ms)

- [ ] Fix error detector connection
  - [ ] Connect immediately after creation
  - [ ] Use `Qt::QueuedConnection`
  - [ ] Remove delayed connection workaround

#### Verification:
```bash
# Stress test
# Open and close window rapidly 100 times
for i in {1..100}; do
    timeout 2 ./DABAnalyzer_Enhanced_FilteringBuild &
    sleep 0.1
    killall DABAnalyzer_Enhanced_FilteringBuild
done

# Should not crash
```

- [ ] No crashes on rapid open/close
- [ ] ThreadSanitizer clean
- [ ] Valgrind clean

---

## DAY 3: VALIDATION & ERROR HANDLING

### Morning: Input Validation (CRITICAL-006)

**File:** `src/core/advanced_fig_analyzer.cpp`
**Estimated Time:** 4 hours

#### Tasks:
- [ ] Add bounds checking to `processFIB()`
  - [ ] Validate input size (not empty, not too large)
  - [ ] Check header access (offset + 2 <= size)
  - [ ] Validate FIG length doesn't exceed data
  - [ ] Add error messages for each check

- [ ] Create safe extraction helpers
  - [ ] Implement `extractUInt16()` with bounds checking
  - [ ] Implement `extractUInt32()` with bounds checking
  - [ ] Return 0 and log warning on invalid access
  - [ ] Add const correctness

- [ ] Update all FIG processors
  - [ ] Replace direct array access with helpers
  - [ ] Add validation before extraction
  - [ ] Check data size for each field
  - [ ] Add specific error messages

- [ ] Add unit tests
  - [ ] Test with empty data
  - [ ] Test with truncated data
  - [ ] Test with oversized length field
  - [ ] Verify error messages

#### Verification:
```bash
# Create test with malformed FIG data
# Run with AddressSanitizer
./test_fig_validation

# Should handle gracefully, no crashes
```

- [ ] No buffer overflows
- [ ] Graceful error handling
- [ ] Appropriate error messages
- [ ] Unit tests pass

---

### Afternoon: CRC Validation (CRITICAL-007)

**File:** `src/core/enhanced_eti_processor_qt.cpp`
**Estimated Time:** 3 hours

#### Tasks:
- [ ] Implement CRC-16-CCITT calculation
  - [ ] Add CRC lookup table (256 entries)
  - [ ] Implement `calculateCRC16()` method
  - [ ] Use polynomial 0x1021
  - [ ] Initial value 0xFFFF

- [ ] Implement frame CRC validation
  - [ ] Calculate over bytes 0-6141
  - [ ] Compare with stored CRC (bytes 6142-6143)
  - [ ] Emit error on mismatch
  - [ ] Set `is_valid` flag correctly

- [ ] Implement FIC CRC validation
  - [ ] Validate each of 3 FIBs
  - [ ] Calculate over 30 bytes per FIB
  - [ ] Compare with last 2 bytes of FIB
  - [ ] Report which FIB failed

- [ ] Update `processEtiFrame()`
  - [ ] Call validation before processing
  - [ ] Reject invalid frames
  - [ ] Count CRC errors
  - [ ] Log detailed error info

#### Verification:
```bash
# Test with known-good ETI file
./DABAnalyzer_Enhanced_FilteringBuild good.eti

# Should process without CRC errors

# Test with corrupted file
./DABAnalyzer_Enhanced_FilteringBuild corrupted.eti

# Should detect and report CRC errors
```

- [ ] Good files pass validation
- [ ] Corrupted files detected
- [ ] Accurate error reporting
- [ ] Performance acceptable

---

## DAY 4: RESOURCE LIMITS & FINAL TESTING

### Morning: Resource Limits (HIGH-007)

**File:** `src/core/enhanced_eti_processor_qt.cpp`
**Estimated Time:** 2 hours

#### Tasks:
- [ ] Add frame storage limits
  - [ ] Define `MAX_STORED_FRAMES = 10000`
  - [ ] Define `FRAME_CLEANUP_THRESHOLD = 1000`
  - [ ] Implement FIFO cleanup in `storeFrame()`
  - [ ] Log when cleanup occurs

- [ ] Add file size validation
  - [ ] Define `MAX_FILE_SIZE = 500 MB`
  - [ ] Check before reading entire file
  - [ ] Validate size is multiple of 6144
  - [ ] Provide helpful error messages

- [ ] Enhance file loading validation
  - [ ] Check file exists
  - [ ] Check file is readable
  - [ ] Validate size > 0
  - [ ] Check for errors during read
  - [ ] Verify read size matches file size

#### Verification:
```bash
# Test with large file
# Monitor memory usage
./DABAnalyzer_Enhanced_FilteringBuild huge.eti

# Memory should stabilize, not grow indefinitely

# Test with oversized file (>500MB)
# Should reject with clear error
```

- [ ] Memory usage bounded
- [ ] Large files handled gracefully
- [ ] Oversized files rejected
- [ ] Clear error messages

---

### Afternoon: Comprehensive Testing

**Estimated Time:** 4 hours

#### Tasks:
- [ ] Run full test suite
  ```bash
  cd build
  ctest --verbose
  ```

- [ ] Memory leak detection
  ```bash
  valgrind --leak-check=full \
           --show-leak-kinds=all \
           --track-origins=yes \
           --log-file=valgrind.log \
           ./DABAnalyzer_Enhanced_FilteringBuild
  ```

- [ ] Thread safety validation
  ```bash
  cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..
  make
  ./DABAnalyzer_Enhanced_FilteringBuild
  ```

- [ ] Address sanitizer
  ```bash
  cmake -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" ..
  make
  ./DABAnalyzer_Enhanced_FilteringBuild
  ```

- [ ] Stress testing
  - [ ] Load multiple large files
  - [ ] Rapid window open/close
  - [ ] Multi-stream processing
  - [ ] Long-running session (1+ hour)

- [ ] Create summary report
  - [ ] List all fixes completed
  - [ ] Verification results
  - [ ] Remaining known issues
  - [ ] Recommendations for Phase 1

#### Final Checklist:
- [ ] All 7 critical issues fixed
- [ ] Valgrind: 0 bytes definitely lost
- [ ] ThreadSanitizer: No data races
- [ ] AddressSanitizer: No errors
- [ ] Unit tests: All passing
- [ ] Code compiles: Zero warnings
- [ ] Documentation: Updated
- [ ] Git commit: Clean with good message

---

## VERIFICATION COMMANDS

### Complete Test Suite
```bash
#!/bin/bash
# run_phase0_verification.sh

echo "=== Phase 0 Verification Suite ==="

# Build clean
echo "Building clean..."
rm -rf build
mkdir build
cd build

# Standard build
echo "Standard build test..."
cmake ..
make
if [ $? -ne 0 ]; then
    echo "FAIL: Standard build"
    exit 1
fi

# Run application
echo "Smoke test..."
timeout 5 ./DABAnalyzer_Enhanced_FilteringBuild &
sleep 2
killall DABAnalyzer_Enhanced_FilteringBuild
if [ $? -ne 0 ]; then
    echo "FAIL: Application crashed"
    exit 1
fi

# Valgrind test
echo "Valgrind memory leak test..."
valgrind --leak-check=full \
         --error-exitcode=1 \
         --timeout 30 \
         ./DABAnalyzer_Enhanced_FilteringBuild 2>&1 | tee valgrind.log

if grep -q "definitely lost: [1-9]" valgrind.log; then
    echo "FAIL: Memory leaks detected"
    exit 1
fi

# ThreadSanitizer
echo "Thread safety test..."
cd ..
rm -rf build
mkdir build
cd build
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" ..
make
timeout 30 ./DABAnalyzer_Enhanced_FilteringBuild 2>&1 | tee tsan.log

if grep -q "WARNING: ThreadSanitizer: data race" tsan.log; then
    echo "FAIL: Thread safety issues detected"
    exit 1
fi

# AddressSanitizer
echo "Address sanitizer test..."
cd ..
rm -rf build
mkdir build
cd build
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g" ..
make
timeout 30 ./DABAnalyzer_Enhanced_FilteringBuild 2>&1 | tee asan.log

if grep -q "ERROR: AddressSanitizer" asan.log; then
    echo "FAIL: Address sanitizer errors detected"
    exit 1
fi

# Unit tests (if exist)
echo "Running unit tests..."
ctest --verbose
if [ $? -ne 0 ]; then
    echo "FAIL: Unit tests failed"
    exit 1
fi

echo "=== ALL PHASE 0 VERIFICATIONS PASSED ==="
echo "Ready to proceed to Phase 1"
```

---

## SUCCESS CRITERIA

### Must Have (Blocking)
- [ ] **CRITICAL-001 Fixed:** LockFreeQueue no memory leaks
- [ ] **CRITICAL-002 Fixed:** Circular buffer properly aligned
- [ ] **CRITICAL-005 Fixed:** Thread safety in multi-stream
- [ ] **CRITICAL-006 Fixed:** Input validation complete
- [ ] **CRITICAL-007 Fixed:** CRC validation implemented
- [ ] **Valgrind Clean:** 0 bytes definitely lost
- [ ] **ThreadSanitizer Clean:** No data races
- [ ] **AddressSanitizer Clean:** No errors

### Should Have (Important)
- [ ] **HIGH-002 Fixed:** Signal connection race removed
- [ ] **HIGH-007 Fixed:** Resource limits in place
- [ ] Unit test coverage for fixes
- [ ] Documentation updated
- [ ] Clean git history

### Nice to Have (Optional)
- [ ] Performance benchmarks
- [ ] Code style improvements
- [ ] Additional tests
- [ ] Refactoring for readability

---

## DAILY STANDUP QUESTIONS

### Each Morning:
1. What did I complete yesterday?
2. What am I working on today?
3. Any blockers or issues?
4. On track for 3-4 day completion?

### Each Evening:
1. What did I accomplish today?
2. What tests passed?
3. What issues remain?
4. Plan for tomorrow?

---

## EMERGENCY CONTACTS / RESOURCES

### If Stuck On:
- **Memory Issues:** Review RAII patterns, smart pointer usage
- **Thread Safety:** Study Qt thread documentation, atomic operations
- **CRC Algorithm:** Reference ETSI EN 300 799 Annex B
- **Qt Signals/Slots:** Check Qt documentation on thread safety

### Useful Resources:
- [Qt Thread Documentation](https://doc.qt.io/qt-6/threads.html)
- [Valgrind Manual](https://valgrind.org/docs/manual/manual.html)
- [ThreadSanitizer Guide](https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual)
- [ETSI Standards Portal](https://www.etsi.org/standards)

---

## PHASE 0 COMPLETION REPORT TEMPLATE

```markdown
# Phase 0 Completion Report

## Summary
- **Start Date:** [DATE]
- **Completion Date:** [DATE]
- **Duration:** [X] days
- **Status:** ✅ COMPLETE / ⚠️ ISSUES REMAIN

## Critical Issues Fixed
- [ ] CRITICAL-001: LockFreeQueue memory leak
- [ ] CRITICAL-002: Circular buffer alignment
- [ ] CRITICAL-005: Thread safety
- [ ] CRITICAL-006: Input validation
- [ ] CRITICAL-007: CRC validation

## Verification Results
- Valgrind: [PASS/FAIL] - [X] bytes leaked
- ThreadSanitizer: [PASS/FAIL] - [X] warnings
- AddressSanitizer: [PASS/FAIL] - [X] errors
- Unit Tests: [PASS/FAIL] - [X]/[Y] passed

## Issues Remaining
- [List any issues not fully resolved]

## Recommendations for Phase 1
- [Suggestions for next phase]

## Ready for Phase 1: [YES/NO]
```

---

**Status:** Ready to start Day 1
**Next Action:** Begin LockFreeQueue memory leak fix
**Expected Phase 0 Completion:** Day 4 afternoon
