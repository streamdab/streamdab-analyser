# ETI Format Detection - Usage Guide

## Overview

The ETI format auto-detection feature allows the StreamDAB Analyzer to automatically identify whether an ETI file is in ETI-LI or ETI-NI format based on sync patterns.

## Quick Start

```cpp
#include "eti_types.hpp"

// Read first 4 bytes of ETI file
uint8_t sync_bytes[4];
file.read(reinterpret_cast<char*>(sync_bytes), 4);

// Detect format
eti::ETIFormat format = eti::detectETIFormat(sync_bytes, 4);

// Handle accordingly
switch (format) {
    case eti::ETIFormat::ETI_LI:
        qDebug() << "ETI-LI format detected (standard)";
        // Use ETI-LI parser
        break;
    
    case eti::ETIFormat::ETI_NI:
        qDebug() << "ETI-NI format detected (network independent)";
        // Use ETI-NI parser
        break;
    
    case eti::ETIFormat::UNKNOWN:
        qWarning() << "Unknown or invalid ETI format";
        break;
}
```

## Format Specifications

### ETI-LI (Linear)
- **Sync Pattern:** `0x49 0x93 0x1E 0x03`
- **Usage:** Most common format, used in file storage
- **Frame Size:** 6144 bytes
- **Reference:** ETSI EN 300 799 Section 5.1

### ETI-NI (Network Independent)
- **Sync Pattern Variant 1:** `0xFF 0xF8`
- **Sync Pattern Variant 2:** `0xFF 0x07`
- **Usage:** Network transmission, Bangkok DAB files
- **Frame Size:** Variable (depends on configuration)
- **Reference:** ETSI EN 300 799 Section 5.1

## API Reference

### `detectETIFormat()`

```cpp
ETIFormat detectETIFormat(const uint8_t* data, size_t size);
```

**Parameters:**
- `data`: Pointer to frame data (minimum 4 bytes required)
- `size`: Size of data buffer

**Returns:**
- `ETIFormat::ETI_LI` - Linear format detected
- `ETIFormat::ETI_NI` - Network Independent format detected
- `ETIFormat::ETI_NA` - Network Adapted format (reserved for future)
- `ETIFormat::UNKNOWN` - Invalid or unrecognized format

**Example:**
```cpp
QFile file("stream.eti");
file.open(QIODevice::ReadOnly);
QByteArray header = file.read(4);

eti::ETIFormat format = eti::detectETIFormat(
    reinterpret_cast<const uint8_t*>(header.data()),
    header.size()
);
```

### `formatToString()`

```cpp
const char* formatToString(ETIFormat format);
```

**Parameters:**
- `format`: ETI format enum value

**Returns:**
- String representation: "ETI-LI", "ETI-NI", "ETI-NA", or "UNKNOWN"

**Example:**
```cpp
eti::ETIFormat format = eti::detectETIFormat(data, size);
qDebug() << "Detected format:" << eti::formatToString(format);
```

## Error Handling

The function safely handles edge cases:

```cpp
// Null pointer - returns UNKNOWN
eti::ETIFormat format1 = eti::detectETIFormat(nullptr, 100);
// Result: ETIFormat::UNKNOWN

// Buffer too small - returns UNKNOWN
uint8_t data[2] = {0x49, 0x93};
eti::ETIFormat format2 = eti::detectETIFormat(data, 2);
// Result: ETIFormat::UNKNOWN

// Invalid sync pattern - returns UNKNOWN
uint8_t data[4] = {0x00, 0x00, 0x00, 0x00};
eti::ETIFormat format3 = eti::detectETIFormat(data, 4);
// Result: ETIFormat::UNKNOWN
```

## Real-World Example

```cpp
#include <QFile>
#include "eti_types.hpp"

bool processETIFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << filename;
        return false;
    }
    
    // Read sync bytes
    QByteArray sync = file.read(4);
    if (sync.size() < 4) {
        qWarning() << "File too small";
        return false;
    }
    
    // Auto-detect format
    eti::ETIFormat format = eti::detectETIFormat(
        reinterpret_cast<const uint8_t*>(sync.data()),
        sync.size()
    );
    
    qInfo() << "File:" << filename;
    qInfo() << "Format:" << eti::formatToString(format);
    
    // Process accordingly
    switch (format) {
        case eti::ETIFormat::ETI_LI:
            return processETI_LI(file);
        
        case eti::ETIFormat::ETI_NI:
            return processETI_NI(file);
        
        default:
            qWarning() << "Unsupported ETI format";
            return false;
    }
}
```

## Performance

- **Detection Time:** <1 microsecond (simple byte comparison)
- **Memory Usage:** Zero allocations (no heap usage)
- **Thread Safety:** Yes (pure function, no shared state)

## Testing

Run the comprehensive test suite:

```bash
cd build
./tests/test_eti_format_detection
```

Or via CTest:

```bash
cd build
ctest -R test_eti_format_detection -V
```

## Troubleshooting

### Issue: Format detected as UNKNOWN

**Possible Causes:**
1. File is corrupted or not a valid ETI file
2. Buffer size too small (< 4 bytes)
3. Custom/proprietary ETI format variant

**Solution:**
- Verify file with hex editor: `hexdump -C file.eti | head -1`
- Check first 4 bytes match known sync patterns
- Ensure at least 4 bytes are passed to detection function

### Issue: Wrong format detected

**Possible Causes:**
1. File has been modified or truncated
2. Using fragment of larger file

**Solution:**
- Verify sync bytes at file start (offset 0)
- Check file integrity with CRC validation
- Compare with known-good reference file

## Known ETI Files

### Bangkok DAB File
- **File:** `eti/bkk_20062022_141637.eti`
- **Format:** ETI-NI (0xFF 0xF8)
- **Verified:** ✅ Automatically detected

### Standard ETI Files
- **Format:** ETI-LI (0x49 0x93 0x1E 0x03)
- **Sources:** Most DAB multiplexers, file archives

---

**Reference:** ETSI EN 300 799 - Digital Audio Broadcasting (DAB); Distribution interfaces; Ensemble Transport Interface (ETI)
