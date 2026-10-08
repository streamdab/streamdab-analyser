# ETI Test Data Management

## Overview

This project uses binary ETI (Ensemble Transport Interface) test files for real-world validation of the DAB stream analyser. The **captures are World DAB licensed material and are never published** — the test suite is split so the PRIVATE (capture-dependent) tests run only where the files exist locally, and the PUBLIC suite runs in CI (see `docs/TEST_SUITE_PUBLIC_PRIVATE_SPLIT.md`, and read the companion `.txt` files in `eti/` for the stream descriptions).

## Current Test Files

| **File** | **Size** | **In git?** | **Description** | **Suite** |
|----------|----------|-------------|-----------------|-----------|
| `eti/bkk_20062022_141637.eti` + `.txt` | 30 MB | ✅ tracked (private repo only) | Bangkok DAB ensemble capture - June 20, 2022 (EPG + TPEG carried, Thai labels, DLS+, MOT slideshow) | **PRIVATE** |
| `eti/HessischerRundfunk-Warntag2024-2024-09-12_journalline/hr_20240912T105801_EWS_Start.eti` / `…T114211_Entwarnung.eti` + `HR_20240912.txt` | 293 MB | ⛔ gitignored | Hessischer Rundfunk warning-day: EWS start + all-clear, Journaline | **PRIVATE** (EWS/Journaline work) |
| `eti/BayerischerRundfunk-Warntag2024-2024-09-12/` | 118 MB | ⛔ gitignored | German broadcast (Bayerischer Rundfunk) Emergency Warning System test | **PRIVATE** (EWS) |
| `eti/EWSv1_1/` | 967 MB | ⛔ gitignored | EWS v1.1 comprehensive suite (`EWS1..9.eti`) | **PRIVATE** (EWS) |

**Gitignored total:** ~1.4 GB

## Why are the large captures Excluded from Git?

1. **Repository Size**: The extra captures would inflate the git repository by over 1 GB
2. **Git Performance**: Large binaries slow down clone, fetch, and pull operations significantly
3. **Development Efficiency**: Developers who don't need the EWS suites shouldn't download them
4. **Bandwidth**: Reduces bandwidth usage for remote operations
5. **Best Practice**: Git is optimized for text source code, not large binary test data

(The 30 MB Bangkok fixture is the deliberate exception: it fits GitHub's per-file
limit, exercises Thai UTF-8 + DLS+/MOT, and keeps the public CI gate green.)

## .gitignore Configuration

The following patterns in `.gitignore` exclude ETI test files:

```gitignore
# ETI test files (large binary files)
*.eti
*.raw
test_data/streams/
samples/large/

# Large ETI test data directories (excluded from git to prevent repo bloat)
# Developers should obtain these files separately for testing
eti/BayerischerRundfunk-Warntag2024-2024-09-12/
eti/EWSv1_1/
eti/*.eti
```

## How to Obtain Test Files

### For Project Maintainers:
If you already have the test files locally, they will remain in your `eti/` directory and work normally for testing. Git will simply ignore them during commits.

### For New Developers:

**Option 1: Request from Project Maintainer**
- Contact the project maintainer for access to the test file archive
- Files may be shared via:
  - Private file sharing service (Google Drive, Dropbox, etc.)
  - Direct transfer if on same network
  - External storage device

**Option 2: Capture Your Own ETI Streams**
- Use DAB/DAB+ reception equipment with ETI output capability
- Record ETI-over-IP streams from broadcast transmitters (if available)
- Use professional DAB modulation equipment to generate test streams

**Option 3: Use Sample ETI Files**
- Some broadcasters provide sample ETI files for testing
- Check DAB industry forums and development communities
- ETSI standards may include reference test vectors

### Installing Test Files:

Once obtained, place test files in the `eti/` directory:

```bash
# From project root
cd eti/

# Extract or copy your ETI test files here
# Example:
unzip BayerischerRundfunk-Warntag2024.zip
tar -xzf EWSv1_1.tar.gz
cp /path/to/your/bangkok.eti bkk_20062022_141637.eti

# Verify files are present
ls -lh eti/
```

Git will automatically ignore these files thanks to `.gitignore`.

## Verifying Test Setup

After adding ETI test files, verify they work:

```bash
# Build the project
cd build/
cmake .. && make

# Run the analyser
./StreamDABAnalyser

# In the GUI:
# 1. Click "Open ETI File"
# 2. Navigate to eti/ directory
# 3. Select a test file (e.g., bkk_20062022_141637.eti)
# 4. Verify services are discovered and displayed
```

## Running Tests with ETI Files

The test suite expects ETI test files to be present:

```bash
# From build directory
cd build/

# Run all tests (requires ETI files)
ctest --output-on-failure

# Run specific ETI parsing tests
./tests/test_eti_frame_parser

# Run FIG processing tests
./tests/test_fig0_processor
./tests/test_fig1_processor
```

If test files are missing, some tests will be skipped with appropriate warnings.

## Git Status Verification

To verify ETI files are correctly ignored:

```bash
# Check git status
git status

# Should NOT show:
# - eti/BayerischerRundfunk-Warntag2024-2024-09-12/
# - eti/EWSv1_1/
# - eti/*.eti files

# Should only show actual source code changes
```

## Storage Requirements

### Minimum Configuration:
- **No ETI files**: ~77 MB (git repository only)
- **Source build**: ~150 MB (includes build artifacts)

### Full Development Configuration:
- **Git repository**: ~77 MB
- **Build artifacts**: ~73 MB
- **ETI test files**: ~1.1 GB
- **Total**: ~1.25 GB

### Recommended Disk Space:
- **Minimum**: 1 GB free
- **Comfortable**: 5 GB free (room for additional test files and recordings)

## Adding New Test Files

When adding new ETI test files to your local development environment:

1. **Place in `eti/` directory**: Files will be automatically ignored by git
2. **Update this documentation**: If the file is significant, add it to the table above
3. **Do NOT commit**: Verify with `git status` before committing
4. **Share separately**: If other developers need the file, share via external means

### Example:

```bash
# Add a new test file
cp /path/to/new_test.eti eti/

# Verify git ignores it
git status
# Should NOT show eti/new_test.eti as untracked

# If you want to document it (optional)
# Update the table in ETI_TEST_DATA.md with file details
```

## Troubleshooting

### Problem: "Test file not found" errors

**Solution**: Ensure ETI test files are present in `eti/` directory:
```bash
ls -lh eti/
# Should show your test files
```

### Problem: Git shows ETI files as untracked

**Solution**: Verify `.gitignore` is up to date:
```bash
git check-ignore -v eti/bkk_20062022_141637.eti
# Should output: .gitignore:238:*.eti    eti/bkk_20062022_141637.eti
```

If not ignored, update `.gitignore` and verify patterns match your file paths.

### Problem: Repository size is too large

**Solution**: Check if ETI files were accidentally committed:
```bash
# Check repository size
du -sh .git/
# Should be ~77 MB, not >1 GB

# If too large, check for committed ETI files
git log --all --pretty=format: --name-only --diff-filter=A | sort -u | grep -E '\\.eti$'

# If ETI files found in history, they need to be removed
# (Contact maintainer for help with git history cleanup)
```

## Best Practices

1. **Never commit ETI files**: Always verify `git status` before `git commit`
2. **Keep test files local**: Share via external means, not git
3. **Document significant files**: Update this file if adding important test cases
4. **Use descriptive names**: Name ETI files to indicate their purpose
5. **Organise by category**: Create subdirectories in `eti/` if needed (e.g., `eti/german/`, `eti/asian/`)

## ETI File Format Reference

### Standard ETI Frame Structure:
- **Frame Size**: 6144 bytes (fixed)
- **SYNC Pattern**: 0xFF1F, 0x491F, or 0xC4FF (4 bytes)
- **Frame Header**: LIDATA fields (4 bytes)
- **FIC (Fast Information Channel)**: 0 or 96 bytes (3 FIBs)
- **MSC (Main Service Channel)**: Variable size
- **EOF (End of Frame)**: 4 bytes

### Validation:
Our ETI parser validates:
- SYNC pattern recognition
- Frame counter sequence (0-249)
- CRC-16 checksums
- ETSI EN 300 799 compliance

## Related Documentation

- [PHASE_1_COMPLETE.md](PHASE_1_COMPLETE.md) - Phase 1 implementation including real ETI processing
- [docs/ETI_PROCESSING_IMPLEMENTATION.md](docs/ETI_PROCESSING_IMPLEMENTATION.md) - ETI parser technical details
- [docs/FIG_PROCESSING_REQUIREMENTS.md](docs/FIG_PROCESSING_REQUIREMENTS.md) - FIG decoding implementation
- [README.md](README.md) - Main project documentation

## Contact

For questions about obtaining ETI test files or setting up your development environment, contact the project maintainer.

---

**Last Updated**: October 17, 2025
**Maintainer**: StreamDAB Analyser Development Team
**Licence**: Same as main project
