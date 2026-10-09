# Test Data Directory

This directory contains test data files and tools for integration testing.

## Overview

The `testdata` directory provides:
- **Synthetic ETI frame generation tools**
- **Test ETI files** (when generated or added)
- **Documentation** on test data formats
- **Guidelines** for adding real ETI capture files

## Directory Contents

```
tests/testdata/
├── README.md                      # This file
├── synthetic_eti_generator.cpp    # ETI generation tool (source)
├── *.eti                          # Generated test ETI files (gitignored)
└── real_captures/                 # Real ETI captures (create if needed)
```

## Synthetic ETI Generator

### Building the Generator

```bash
cd /home/seksan/workspace/streamdab-analyser/build
g++ -std=c++20 -o synthetic_eti_generator ../tests/testdata/synthetic_eti_generator.cpp
```

### Usage

Generate test ETI files with various configurations:

```bash
# Generate 100 minimal frames
./synthetic_eti_generator test_minimal.eti

# Generate 1000 frames with audio data
./synthetic_eti_generator --frames 1000 --with-audio test_audio.eti

# Generate 500 frames with MOT data service
./synthetic_eti_generator --frames 500 --with-mot test_mot.eti
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--frames N` | Generate N ETI frames | 100 |
| `--with-audio` | Include synthetic audio data | No |
| `--with-mot` | Include MOT data service | No |
| `--help` | Show help message | - |

## Test File Formats

### ETI Frame Structure (6144 bytes)

```
Offset   Size    Description
------   ----    -----------
0        4       Sync pattern (0xFF1F491F or 0xFF1FC4FF)
4        4       LIDATA (FCT, FICF, NST, etc.)
8        96      FIC data (3 FIBs × 32 bytes for Mode I)
104      ~4608   MSC data (Main Service Channel)
~5880    ?       EOF, TIST, padding
```

### Synthetic Frame Types

#### 1. Minimal Frame
- Valid ETI structure
- Basic FIG 0/0 (ensemble info)
- Empty MSC data
- Use for: Frame parsing tests, structure validation

#### 2. Audio Frame (`--with-audio`)
- Valid ETI structure
- FIG 0/1 (subchannel organization)
- Synthetic DAB+ audio superframe in MSC
- HE-AAC v2 profile flags (SBR + PS)
- Use for: Audio pipeline integration tests

#### 3. MOT Frame (`--with-mot`)
- Valid ETI structure
- FIG 0/2 (data service component)
- MOT data groups in MSC
- Transport ID = 42
- Use for: MOT pipeline integration tests

## Adding Real ETI Captures

For comprehensive testing with real broadcast data:

### 1. Capture Real ETI Streams

Use tools like:
- `dablin` with ETI-over-IP capture
- `ODR-DabMux` output files
- SDR tools (rtl-sdr, hackrf) with DAB demodulators

### 2. Recommended Capture Format

```bash
# Create captures directory
mkdir -p tests/testdata/real_captures/

# Example: Capture from multicast ETI stream
# (Tool-specific command - adjust for your setup)
```

### 3. Organize Real Captures

Suggested naming convention:

```
tests/testdata/real_captures/
├── thailand_bangkok_pbs_2025-10-22.eti       # Thai PBS Bangkok
├── thailand_bangkok_mcot_2025-10-22.eti      # MCOT Bangkok
├── test_case_audio_only.eti                  # Audio service only
├── test_case_mot_slideshow.eti               # MOT SlideShow active
└── test_case_multi_service.eti               # Multiple services
```

### 4. Document Real Captures

Create a `real_captures/README.md` with:
- Capture date and time
- Location (city, country)
- Ensemble name and ID
- Service list
- Special features (MOT, EPG, announcements, etc.)
- Capture duration
- Known issues or anomalies

Example:

```markdown
## thailand_bangkok_pbs_2025-10-22.eti

- **Date:** October 22, 2025 14:30 UTC+7
- **Location:** Bangkok, Thailand
- **Ensemble:** Thai PBS (0xE001)
- **Duration:** 5 minutes (1500 frames)
- **Services:**
  - Thai PBS Radio (SID: 0xE1C00379) - DAB+ 96 kbps HE-AAC v2
  - Thai PBS News (SID: 0xE1C0037A) - DAB+ 64 kbps HE-AAC v2
- **Features:**
  - MOT SlideShow active
  - Thai language labels (UTF-8)
  - FIG Type 2 extended labels present
```

## Integration Test Usage

### In C++ Tests

```cpp
// tests/integration/test_audio_pipeline.cpp

#include "test_utils.h"

// Option 1: Use synthetic frames
QByteArray frame = TestUtils::generateMinimalETIFrame();

// Option 2: Load real capture file
QFile real_file("../tests/testdata/real_captures/test.eti");
real_file.open(QIODevice::ReadOnly);
QByteArray real_frame = real_file.read(6144);
```

### Test Data Selection Guidelines

| Test Type | Use Synthetic | Use Real Capture |
|-----------|---------------|------------------|
| Frame structure validation | ✓ | ✓ |
| Basic FIG parsing | ✓ | ✓ |
| Audio pipeline flow | ✓ | ✓ |
| MOT pipeline flow | ✓ | ✓ |
| Real service discovery | - | ✓ |
| Thai language labels | - | ✓ |
| Multi-service scenarios | - | ✓ |
| Long-duration tests | ✓ | ✓ |
| Performance benchmarks | ✓ | ✓ |

## File Size Considerations

### Synthetic Files
- 1 frame = 6144 bytes
- 100 frames = ~600 KB
- 1000 frames = ~6 MB
- 10000 frames = ~60 MB

### Real Captures
- 1 second = ~40 frames = ~240 KB
- 1 minute = ~14.4 MB
- 5 minutes = ~72 MB
- **Recommendation:** Keep captures under 100 MB (≈7 minutes)

## Git Configuration

Test data files are **not committed** to the repository:

```gitignore
# .gitignore entries
tests/testdata/*.eti
tests/testdata/real_captures/
```

**Exception:** Documentation files (`README.md`) are committed.

## Quality Assurance

### Validate Synthetic Frames

```bash
# Build and run frame validator
cd build
make test_eti_frame_parser
./tests/test_eti_frame_parser
```

### Validate Real Captures

```cpp
// Quick validation script
bool validate_eti_file(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    int frame_count = 0;
    while (!file.atEnd()) {
        QByteArray frame = file.read(6144);
        if (frame.size() != 6144) {
            qWarning() << "Frame" << frame_count << "invalid size:" << frame.size();
            return false;
        }
        
        // Verify sync pattern
        if (!((frame[0] == 0xFF && frame[1] == 0x1F))) {
            qWarning() << "Frame" << frame_count << "invalid sync";
            return false;
        }
        
        frame_count++;
    }
    
    qInfo() << "Valid ETI file:" << frame_count << "frames";
    return true;
}
```

## Best Practices

1. **Synthetic Data**: Use for automated CI/CD testing
2. **Real Captures**: Use for validation and edge case testing
3. **Documentation**: Always document real capture sources
4. **Size Management**: Keep individual files under 100 MB
5. **Organization**: Use descriptive filenames
6. **Validation**: Always validate before use in tests

## Future Enhancements

Planned improvements:

- [ ] Enhanced synthetic generator with configurable FIG types
- [ ] Automated capture tools for Thai DAB broadcasts
- [ ] Test data catalog with searchable metadata
- [ ] Compressed archive storage for large captures
- [ ] Online test data repository (optional)

## Support

For questions or issues with test data:
- Review this README
- Check integration test examples in `tests/integration/`
- Consult main project documentation in `CLAUDE.md`

---

**Last Updated:** October 22, 2025  
**Phase:** 3A Wave 3.1 - Integration & Testing Infrastructure
