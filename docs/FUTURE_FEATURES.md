# Future Features - StreamDAB Analyser

> **Status:** 📋 Planned features for future releases  
> **Current Version:** v1.0 (Phase 3B Complete)  
> **Next Planned:** Phase 4, 5, 6

---

## Phase 4: CLI Extract Features

**Status:** ⏳ Planned (not implemented)  
**Priority:** Medium (high-value for broadcast engineers)  
**Estimated Timeline:** 3-4 weeks  
**Target Release:** v1.1 or v1.2

### Goal

Extract decoded content from ETI streams to standard file formats for post-processing and archive.

### Planned Extract Commands

```bash
# Extract DAB+ audio from service(s)
streamdab-analyser --input stream.eti --extract audio \
  --service 0xC201 --output audio.aac

# Extract slideshow images (MOT)
streamdab-analyser --input stream.eti --extract slideshow \
  --service 0xC201 --output-dir ./slides/

# Extract EPG (EIT) to XMLTV
streamdab-analyser --input stream.eti --extract epg \
  --output epg.xmltv

# Extract Journaline content
streamdab-analyser --input stream.eti --extract journaline \
  --output-dir ./journaline/

# Extract raw MSC data per subchannel
streamdab-analyser --input stream.eti --extract msc \
  --subchannel 10 --output msc_sc10.bin

# Extract TPEG messages
streamdab-analyser --input stream.eti --extract tpeg \
  --output tpeg.json

# Batch extract all services
streamdab-analyser --input stream.eti --extract all \
  --output-dir ./extracted/
```

### Supported Extract Types

| Type | Standard | Output Format | Status |
|------|----------|---------------|--------|
| **DAB+ Audio** | ETSI TS 102 563 | AAC/ADTS, MP4, WAV | ⏳ Planned |
| **Slideshow (MOT)** | ETSI TS 101 756 | JPEG/PNG + metadata JSON | ⏳ Planned |
| **EPG (EIT)** | ETSI TS 102 371 | XMLTV, JSON | ⏳ Planned |
| **Journaline** | ETSI TS 102 979 | HTML, JSON, plain text | ⏳ Planned |
| **TPEG** | ETSI TS 102 894 | JSON, binary | ⏳ Planned |
| **Raw MSC** | ETSI EN 300 401 | Binary per subchannel | ⏳ Planned |
| **PAD/XPAD** | ETSI TS 102 563 | Binary + analysis | ⏳ Planned |

### Architecture

```cpp
namespace cli {

enum class ExtractType {
    Audio,      // DAB+ superframes → AAC
    Slideshow,  // MOT objects → images
    EPG,        // EIT → XMLTV
    Journaline, // Journaline objects → HTML/JSON
    TPEG,       // TPEG messages → JSON
    MSC,        // Raw subchannel data
    PAD         // PAD/XPAD data
};

class ExtractManager : public QObject
{
    Q_OBJECT
    
public:
    struct ExtractOptions {
        ExtractType type;
        QString input_file;
        QString output_path;      // file or directory
        quint32 service_id = 0;   // 0 = all services
        int subchannel_id = -1;   // -1 = all
        QStringList filters;      // Additional filters
    };
    
    // Main extraction entry point
    bool extract(const ExtractOptions& options);
    
    // Progress tracking
    int progress() const;
    QString currentOperation() const;
    
signals:
    void progressChanged(int percent);
    void operationStarted(const QString& operation);
    void operationFinished(bool success, const QString& message);
    
private:
    // Per-type extractors
    std::unique_ptr<AudioExtractor> m_audioExtractor;
    std::unique_ptr<SlideshowExtractor> m_slideshowExtractor;
    std::unique_ptr<EPGExtractor> m_epgExtractor;
    std::unique_ptr<JournalineExtractor> m_journalineExtractor;
    std::unique_ptr<TPEGExtractor> m_tpegExtractor;
    std::unique_ptr<MSCExtractor> m_mscExtractor;
};

}
```

### Dependencies

**Required (existing):**
- ✅ ETI frame parsing (modern_eti_frame_parser.cpp)
- ✅ FIG decoding (fig_parser.cpp)
- ✅ Service discovery (service_manager.cpp)
- ✅ MOT decoder (mot_protocol.cpp)
- ✅ EPG decoder (epg_decoder.cpp)
- ✅ Journaline decoder (journaline_decoder.cpp)
- ✅ TPEG decoder (tpeg_decoder.cpp)

**New (to implement):**
- AudioExtractor: AAC frame assembly, ADTS/MP4 muxing (libfaad, libfdk-aac)
- SlideshowExtractor: MOT object reassembly, image decoding (libjpeg, libpng)
- EPGExtractor: EIT table parsing, XMLTV generation
- JournalineExtractor: Journaline object tree, HTML/JSON export
- TPEGExtractor: TPEG message parsing, JSON export
- MSCExtractor: Raw subchannel data with framing

### Testing Strategy

**Unit Tests:**
- Each extractor with synthetic ETI data
- Output format validation (XMLTV schema, MP4 validity)

**Integration Tests:**
- Real Bangkok ETI fixture (bkk_20062022_141637.eti)
- Verify extracted audio plays correctly
- Verify slideshow images render correctly
- Verify XMLTV validates against schema

**Reference Implementations:**
- etisnoop (ETI analysis reference)
- fdk-aac (AAC encoding)
- xmltv-util (XMLTV validation)
- mot-utils (MOT extraction)

### Performance Targets

| Extract Type | Speed Target | Notes |
|--------------|--------------|-------|
| Audio (5001 frames) | <10 seconds | ~500x realtime |
| Slideshow (100 images) | <5 seconds | Including decode |
| EPG (full schedule) | <3 seconds | |
| Journaline | <2 seconds | |
| Raw MSC | <1 second | Minimal processing |

---

## Phase 5: Multi-stream Comparative Analysis

**Status:** ⏳ Planned (not implemented)  
**Priority:** Low (advanced professional feature)  
**Estimated Timeline:** 3-4 weeks  
**Target Release:** v2.0

### Goal

Simultaneous analysis of multiple ETI streams for comparative analysis and validation.

### Use Cases

1. **Multi-vendor Comparison**
   - Compare DAB multiplexers from different vendors
   - Validate configuration consistency
   - Identify vendor-specific FIG implementations

2. **Before/After Analysis**
   - Compare stream before and after configuration changes
   - Validate upgrade impacts
   - Track service organisation changes

3. **Multi-region Analysis**
   - Analyse DAB broadcasts from different regions
   - Compare ensemble configurations
   - Study international DAB implementations

4. **Quality Assurance**
   - Automated regression testing
   - Configuration validation
   - Standards compliance verification

### Planned Features

#### Multi-stream Management
- Load 2-4 ETI streams simultaneously
- Independent playback controls per stream
- Synchronized frame-by-frame navigation
- Stream state management (play/pause/stop individual streams)

#### Comparative Analysis Views

**1. Side-by-Side Ensemble Comparison**
```
┌─────────────────────┬─────────────────────┐
│ Stream A: BBC DAB   │ Stream B: DR DAB    │
├─────────────────────┼─────────────────────┤
│ Ensemble: BBC       │ Ensemble: DR P1     │
│ Services: 12        │ Services: 8         │
│ Subchannels: 10     │ Subchannels: 6      │
│ Total CU: 720/864   │ Total CU: 580/864   │
└─────────────────────┴─────────────────────┘
```

**2. FIG Difference Viewer**
- Highlight FIG types present in one stream but not another
- Show FIG content differences (service labels, bitrates, etc.)
- Colour-coded diff view (added/removed/changed)

**3. Service Organisation Comparison**
- Compare service IDs and labels
- Identify missing/extra services
- Bitrate and protection level differences
- Subchannel allocation comparison

**4. Timing and Synchronization**
- Frame timing comparison
- Synchronization accuracy metrics
- CIF counter alignment
- Drift detection

### Architecture

**Pattern:** Multi-worker threads (Pattern 3) + Shared data model (Pattern 5)

```cpp
class MultiStreamManager : public QObject
{
    Q_OBJECT
    
public:
    // Manage up to 4 streams
    void addStream(const QString& filepath, int streamIndex);
    void removeStream(int streamIndex);
    
    // Synchronized navigation
    void seekToFrame(quint32 frameNumber);  // All streams
    void playAll();
    void pauseAll();
    
    // Comparison operations
    ComparisonResult compareEnsembles(int stream1, int stream2);
    ComparisonResult compareFIGs(int stream1, int stream2);
    ComparisonResult compareServices(int stream1, int stream2);
    
signals:
    void streamAdded(int streamIndex, const StreamInfo& info);
    void streamRemoved(int streamIndex);
    void comparisonResultReady(const ComparisonResult& result);
    
private:
    QVector<ETIReaderThread*> m_readerThreads;  // One per stream
    QVector<SharedETIData*> m_sharedData;       // One per stream
    ComparisonEngine* m_comparisonEngine;
};

class ComparisonEngine : public QObject
{
    Q_OBJECT
    
public:
    // Compare two streams
    ComparisonResult compareStreams(SharedETIData* stream1, 
                                     SharedETIData* stream2,
                                     ComparisonType type);
    
    // Generate diff report
    QString generateDiffReport(const ComparisonResult& result);
    
private:
    // FIG comparison
    FIGDiff compareFIGs(const FICDatabase& fic1, const FICDatabase& fic2);
    
    // Service comparison
    ServiceDiff compareServices(const QList<ServiceInfo>& services1,
                                 const QList<ServiceInfo>& services2);
    
    // Subchannel comparison
    SubchannelDiff compareSubchannels(const QMap<int, SubchannelConfig>& subch1,
                                       const QMap<int, SubchannelConfig>& subch2);
};
```

### UI Design

**Tab 4: Multi-stream Analysis** (future)

```
┌────────────────────────────────────────────────────────────┐
│ Multi-stream Controls                                       │
│ [Add Stream] [Remove Stream] [Compare Selected]            │
├─────────────────────┬──────────────────────────────────────┤
│ Active Streams      │ Comparative Analysis                 │
│                     │                                      │
│ ☑ Stream A          │ Comparison Type:                     │
│   BBC DAB           │ ○ Ensembles  ● Services  ○ FIGs     │
│   Status: Loaded    │                                      │
│   Frame: 1024       │ Results:                             │
│                     │ ┌────────────────────────────────┐  │
│ ☑ Stream B          │ │ Service Differences:           │  │
│   DR DAB            │ │                                │  │
│   Status: Playing   │ │ + Stream A has 4 extra svcs   │  │
│   Frame: 1024       │ │ - Stream B missing "BBC R4"   │  │
│                     │ │ Δ "BBC R1": 128→192 kbps      │  │
│ ☐ Stream C          │ │                                │  │
│   (empty)           │ │ FIG Differences:               │  │
│                     │ │ + FIG 0/17 in Stream A only   │  │
│ ☐ Stream D          │ │ Δ FIG 0/10: Different LTO     │  │
│   (empty)           │ └────────────────────────────────┘  │
│                     │                                      │
│ [Sync Navigation]   │ [Export Report] [Save Comparison]   │
└─────────────────────┴──────────────────────────────────────┘
```

### Data Structures

```cpp
struct ComparisonResult {
    int stream1_index;
    int stream2_index;
    ComparisonType type;
    
    // Ensemble differences
    struct {
        QString label_diff;     // "BBC" vs "DR P1"
        int service_count_diff; // +4 services in stream A
        int cu_usage_diff;      // +140 CU in stream A
    } ensemble;
    
    // Service differences
    struct {
        QList<ServiceInfo> added_services;    // In stream A, not B
        QList<ServiceInfo> removed_services;  // In stream B, not A
        QList<ServiceChange> changed_services; // Different configs
    } services;
    
    // FIG differences
    struct {
        QSet<QString> added_figs;    // "FIG 0/17"
        QSet<QString> removed_figs;  // "FIG 0/16"
        QList<FIGChange> changed_figs;
    } figs;
    
    // Statistics
    float similarity_score;  // 0.0-1.0 (1.0 = identical)
    int total_differences;
    QDateTime comparison_time;
};

struct ServiceChange {
    uint32_t service_id;
    QString field_name;     // "bitrate", "protection_level", "label"
    QVariant old_value;     // From stream A
    QVariant new_value;     // From stream B
};

struct FIGChange {
    QString fig_type;       // "FIG 0/1"
    int occurrence_count_diff;  // +5 occurrences in stream A
    QString content_diff;   // Detailed differences
};
```

### Implementation Phases

**Phase 5.1: Core Multi-stream Infrastructure (Week 1)**
- Implement `MultiStreamManager` class
- Add stream loading and management
- Implement synchronized playback controls
- Test with 2 streams

**Phase 5.2: Comparison Engine (Week 2)**
- Implement `ComparisonEngine` class
- Add ensemble comparison
- Add service comparison
- Add FIG comparison
- Generate diff reports

**Phase 5.3: UI Implementation (Week 3)**
- Create Tab 4 Multi-stream Analysis
- Implement active streams tree
- Implement comparative analysis panel
- Add diff visualization (colour-coded)

**Phase 5.4: Testing & Polish (Week 4)**
- End-to-end testing with real streams
- Performance optimization (4 streams simultaneously)
- Export functionality (CSV, JSON, PDF reports)
- Documentation and user guide

### Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| **Streams Supported** | 2-4 | Simultaneously |
| **CPU Usage** | <80% per stream | Total <320% (4 cores) |
| **Memory Usage** | <100 MB per stream | Total <400 MB |
| **Comparison Speed** | <500 ms | For full ensemble comparison |
| **UI Responsiveness** | <16 ms | 60 fps minimum |

### Dependencies

**Required:**
- ✅ Phase 1-3B complete (ETI parsing, FIG decoding, services)
- ✅ Pattern 3 worker threads (already implemented)
- ✅ Pattern 5 shared data model (already implemented)

**New:**
- Comparison algorithms (new)
- Diff report generation (new)
- Multi-stream UI components (new)

### Export Formats

**1. CSV Export**
```csv
Stream,Ensemble,Services,Subchannels,Total_CU,Frame_Rate
Stream_A,BBC,12,10,720,250.5
Stream_B,DR_P1,8,6,580,250.3
```

**2. JSON Export**
```json
{
  "comparison_type": "services",
  "stream_a": "BBC DAB",
  "stream_b": "DR DAB",
  "timestamp": "2025-10-25T14:30:00Z",
  "similarity_score": 0.65,
  "differences": {
    "added_services": [
      {"sid": "0xC221", "label": "BBC Radio 4 Extra"}
    ],
    "removed_services": [],
    "changed_services": [
      {
        "sid": "0xC201",
        "label": "BBC Radio 1",
        "field": "bitrate",
        "old_value": 128,
        "new_value": 192
      }
    ]
  }
}
```

**3. PDF Report**
- Executive summary
- Visual diff charts
- Detailed comparison tables
- Recommendations

### Testing Strategy

**Unit Tests:**
- `ComparisonEngine::compareStreams()` correctness
- `FIGDiff` detection accuracy
- `ServiceDiff` detection accuracy

**Integration Tests:**
- Load 2 streams and compare ensembles
- Load 4 streams and sync navigation
- Compare identical streams (should be 100% similar)
- Compare completely different streams

**Performance Tests:**
- 4 streams playing simultaneously at 250 fps each
- CPU and memory profiling
- Comparison speed benchmarks

**UI Tests:**
- Add/remove streams
- Synchronized playback
- Comparison result display
- Export functionality

### Documentation

**User Guide:**
- How to load multiple streams
- How to perform comparisons
- How to interpret diff results
- Export options

**Developer Guide:**
- Architecture overview
- Extending comparison types
- Adding new diff algorithms

---

## Phase 6: Advanced Data Services

**Status:** ⏳ Planned  
**Priority:** Medium  
**Timeline:** 4-5 weeks

### TPEG (Traffic and Travel Information)

**Standard:** ETSI TS 102 894

**Features:**
- TPEG message parsing
- Traffic event decoding
- Location referencing
- Real-time traffic display

### Broadcast Website (BWS) Enhancements

**Standard:** ETSI TS 101 498-1

**Features:**
- Complete HTML rendering
- Image carousel support
- JavaScript execution (limited)
- Link navigation

### Advanced MOT Features

**Features:**
- Object versioning
- Trigger time handling
- Category filtering
- Advanced carousel management

---

## Phase 7: Performance Optimisation

**Status:** ⏳ Planned  
**Priority:** High (for production deployment)  
**Timeline:** 2-3 weeks

### Goals

- Sustained >1000 FPS processing (currently ~250 FPS target)
- <50 MB memory usage (currently ~100 MB)
- <50% CPU usage on single core (currently ~80%)
- Zero memory leaks (Valgrind clean - already achieved)
- Zero data races (ThreadSanitizer clean - already achieved)

### Optimisation Targets

**1. ETI Parsing Optimisation**
- SIMD acceleration for CRC checks
- Zero-copy buffer handling
- Batch frame processing

**2. FIG Processing Optimisation**
- FIG cache to avoid re-parsing
- Incremental updates only
- Lazy evaluation

**3. UI Rendering Optimisation**
- Virtual scrolling for large tables
- Throttling improvements (Pattern 8)
- GPU acceleration for charts

**4. Memory Optimisation**
- Circular buffer for frame storage
- Smart pointer optimization
- String interning for labels

---

## Phase 8: Professional Packaging

**Status:** ⏳ Planned  
**Priority:** Medium  
**Timeline:** 2 weeks

### Distribution Formats

**Linux:**
- AppImage (portable, no installation)
- Debian package (.deb)
- RPM package (.rpm)
- Flatpak (sandboxed)

**Windows:**
- Installer (NSIS or WiX)
- Portable ZIP
- Windows Store package (MSIX)

**macOS:**
- DMG installer
- Homebrew formula
- Mac App Store package

### CI/CD Pipeline

**Automated Builds:**
- GitHub Actions workflow
- Nightly builds
- Release builds
- Multi-platform compilation

**Automated Testing:**
- Unit tests on commit
- Integration tests on PR
- E2E GUI tests on merge
- Performance regression tests

### Release Process

1. Version bump (semantic versioning)
2. Changelog generation
3. Build all platforms
4. Run full test suite
5. Package distributables
6. Sign binaries (Windows, macOS)
7. Upload to GitHub Releases
8. Update documentation site

---

## Community Requests

**Placeholder for user-requested features**

Submit feature requests at: https://github.com/streamdab/streamdab-analyser/issues

### Most Requested (Placeholder)

1. **Audio Playback** (Phase 3 - optional MOT decoding)
2. **Real-time Spectrum Analyser** (Phase 7)
3. **Configuration Export/Import** (Phase 8)

---

## Priority Matrix

| Feature | Priority | Effort | Value | Planned Phase |
|---------|----------|--------|-------|---------------|
| Multi-stream Comparison | Low | High | Medium | Phase 5 |
| TPEG Support | Medium | High | Medium | Phase 6 |
| Performance >1000 FPS | High | Medium | High | Phase 7 |
| Professional Packaging | Medium | Medium | High | Phase 8 |
| Audio Playback | Medium | High | Low | Optional |

---

**Last Updated:** 2025-10-25  
**Document Version:** 1.0  
**Maintained By:** StreamDAB Development Team
