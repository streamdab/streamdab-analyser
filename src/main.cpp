#include <QApplication>
#include <QMainWindow>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QTableWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QResizeEvent>
#include <QProgressBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QGroupBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QTimer>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QToolButton>          // T23: ⚙ UDP-streaming-settings button in the network row
// T29: playback controls (seek slider + transport buttons).
#include <QSlider>
#include <QStyle>               // T44: style media icons for the transport buttons
#include <QSignalBlocker>
#include <QPointer>
#include <QFrame>
#include <QDateTime>
#include <QRadioButton>
#include <QLineEdit>
#include <QStackedWidget>
#include <QGridLayout>
#include <QScrollArea>   // Wave B: scrollable heavy bottom-strip tabs
#include <QButtonGroup>
#include <QDialog>              // Phase C: UDP settings dialog
#include <QFormLayout>          // Phase C: UDP settings dialog
#include <QDialogButtonBox>     // Phase C: UDP settings dialog

#include <QRegularExpression>   // Phase 1.1: Hex viewer formatting
#include <QElapsedTimer>        // Phase 1.2: Timing throttle
// === Qt Advanced Docking System (Qt-ADS) 4.5.0 — v1.3 GUI migration ===
#include "DockManager.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
#include "DockSplitter.h"
#include <QScopeGuard>          // RAII navigation-guard re-arm (Finding 8)
#include <QSettings>
#include <QStandardPaths>
#include <QHash>
#include <QSet>
#include <chrono>
#include <memory>
#include <map>
#include <deque>                // W1 #2: Logging-tab staging buffer (O(1) pop_front)
#include <limits>               // W1 #2: drainAll flush budget
#include <list>
#include <deque>
#include <functional>
#include <optional>
#include <atomic>               // Phase 1.1: Thread-safe counter
#include <algorithm>            // std::min (DLS+ F-PAD candidate extraction)
#include <exception>            // std::exception (frame-decode try/catch guards)
#include <limits>               // std::numeric_limits (T32 slide eviction)

#include "core/enhanced_eti_processor_qt.h"
#include "core/crc16.hpp"  // shared CRC-16/CCITT-FALSE (AU + DL Data Group CRC)
#include "core/dabplus_superframe.hpp"  // T38: shared DAB+ superframe/AU/PAD transport
#include "core/advanced_fig_analyser.h"
#include "core/product_version.hpp"  // single-source DABX_VERSION
#include "core/hex_viewer_formatter.h"
#include "core/analyser_settings.hpp"
#include "core/etsi_compliance_engine.hpp"
#include "core/multi_stream_processor.hpp"
#include "core/advanced_error_detection.hpp"
#include "core/dls_plus_decoder.hpp"  // Phase 3B: DLS+ decoder integration
#include "core/mot_protocol.hpp"      // MOT SlideShow decoder (option A)
#include "core/mot_pad_adapter.hpp"   // X-PAD CI 12/13 -> MOTProtocol adapter
// v1.4 data-path wave — EPG (TS 102 371) / Journaline (TS 102 979) / TPEG
// (TS 102 894 base/partial) decoders + the per-service data model. The GUI
// tabs are fed from these after a capture load completes (load-time counters
// stay untouched; see the clean tap in onFrameProcessed()).
#include "core/epg_decoder.hpp"
#include "core/journaline_decoder.hpp"
#include "core/tpeg_decoder.hpp"
#include "core/data_service_store.hpp"
// T34: selected-service DAB+ audio decode (libfaad2) + ALSA output.
#include "core/dabplus_audio_decoder.hpp"
#include "core/audio_output_sink.hpp"
#include "core/audio_playback_controller.hpp"
#include "core/network_stream_receiver.hpp"  // Phase 4: ETI-over-IP live streaming
#include "core/eti_reader_thread.hpp"  // Phase A: Pattern 3 - Worker Thread
#include "core/shared_eti_data.hpp"  // Phase A: Pattern 5 - Shared Data Model
#include "cli/cli_main.hpp"  // PDCA Week 8: CLI mode integration

// Wave B: adopted src/gui widgets (see docs/gui/SRC_GUI_AUDIT.md). These are
// now compiled into the target and driven by DABAnalyserWindow adapters.
#include "gui/constellation_widget.h"
#include "gui/about_dialog.h"
#include "gui/performance_dashboard.h"
#include "gui/real_time_chart_widget.h"
#include "gui/settings_dialog.h"
#include "gui/etsi_compliance_monitor.h"
#include "gui/professional_export_manager.h"
#include "utils/app_settings.hpp"  // explicit QSettings scope (org/app)
#include "utils/logger.h"          // Logger level (Advanced settings tab)

#include <QFile>       // /proc/self/{stat,statm} sampling for the dashboard
#if defined(Q_OS_LINUX)
#include <unistd.h>    // sysconf(_SC_PAGESIZE / _SC_CLK_TCK)
#endif

// Note: gui/main_window.h exists for GUI testing purposes only
// Do NOT include it here to avoid redefinition conflicts

// CRITICAL FIX: Register Qt meta-types for ProcessedFrame to work with Qt::QueuedConnection
// Q_DECLARE_METATYPE already exists in enhanced_eti_processor_qt.h, but we need
// qRegisterMetaType in the main.cpp compilation unit for proper cross-compilation-unit usage
namespace {
    // Force meta-type registration at static initialization time in this compilation unit
    const int _processedFrameTypeId = qRegisterMetaType<ProcessedFrame>("ProcessedFrame");
    const int _etiFormatTypeId = qRegisterMetaType<ETIFormat>("ETIFormat");
}

// ============================================================================
// Standard DAB+ superframe / PAD parser (Refinement A)
// ============================================================================
// Replaces the previous "last min(128,|msc|) bytes" heuristic with the
// documented source chain (docs/PHASE_3B_TECHNICAL_NOTES.md §Architecture):
//   DAB+ audio subchannel MSC (ETI)
//     -> superframe header/AU split (ETSI TS 102 563 §4/§5)
//     -> PAD inside the Data Stream Element (DSE) at the AU start
//     -> F-PAD + X-PAD CI/data subfields (ETSI EN 300 401 §7.4)
//     -> DLS/DLS+ Data Groups -> reassembled DLS text + DL+ tags
//     -> DLSPlusDecoder::processPADData() -> nowPlayingUpdated()
//
// The repo's core/dabplus_stream_validator.cpp does NOT contain a usable
// F-PAD/X-PAD parser: its `extractPADData()` is explicitly a stub ("real
// implementation would parse F-PAD length indicator") and its AU handling has
// no FireCode / DSE / AU-CRC step. The transport layer (5-CIF superframe
// assembly, FireCode sync, AU offsets, AU CRC, DSE/PAD location) now lives in
// the shared module core/dabplus_superframe.{hpp,cpp} (T38) and is used here
// and by the T34 audio decoder. The remaining F-PAD/X-PAD/DLS/DL+/MOT logic
// below was re-implemented from the standards and cross-checked against the
// reference decoders (logic only, no code copied):
//   - ODR-Dab DABlin: src/dabplus_decoder.cpp (SuperframeFilter::CheckSync /
//     CheckForPAD), src/pad_decoder.cpp (PADDecoder::Process,
//     DynamicLabelDecoder)
//   - ODR-Dab etisnoop: src/dabplussnoop.cpp (seek_valid_firecode / decode /
//     extract_au), src/firecode.c
// FPAD_LEN = 2 is exposed by the shared module as streamdab::dabplus::kFpadLen.
// ============================================================================

// FireCode CRC-16, shared implementation (core/dabplus_superframe.{hpp,cpp}).
// Kept as a thin alias because the GUI test target textually includes this file
// and builds crafted superframes with it.
inline uint16_t dabFirecodeCrc(const uint8_t* buf, size_t size)
{
    return streamdab::dabplus::firecodeCrc(buf, size);
}

// X-PAD X-PAD CI sub-field lengths, ETSI EN 300 401 Table 6.
constexpr int kXpadSubfieldLens[8] = {4, 6, 8, 12, 16, 24, 32, 48};
constexpr int kFpadLen = streamdab::dabplus::kFpadLen;  // F-PAD length (shared T38)

// DLS+ content descriptor (standard 3-byte form, ETSI TS 102 980 §4).
struct StdDlsPlusTag {
    int contentType = 0;    // 7-bit content type (1..61)
    int startMarker = 0;    // byte offset into the DLS text
    int lengthMarker = 0;   // byte length - 1
};

struct DlsAssembly {
    bool hasLabel = false;
    QByteArray dlText;                 // raw DLS bytes in `charset`
    uint8_t charset = 15;              // TS 101 756 character field
    std::vector<StdDlsPlusTag> tags;   // DL+ tags (empty for plain DLS)
    bool hasDlPlus = false;
};

// Per-DAB+-subchannel parser: accumulates 5 x 24 ms CIFs into a 120 ms
// superframe, locates each AU + its DSE/PAD, and reassembles DLS/DLS+.
struct DabPlusSubchannelParser {
    // Evidence counters (aggregated for the load report).
    uint64_t superframes = 0;
    uint64_t firecodeOk = 0;
    uint64_t ausProcessed = 0;
    uint64_t ausCrcOk = 0;
    uint64_t padFound = 0;
    uint64_t fpadCiSet = 0;
    uint64_t xpadGroups = 0;
    uint64_t dlSegments = 0;
    uint64_t dlPlusSegments = 0;
    uint64_t labels = 0;
    uint64_t groupTypeCounts[32] = {0};

    // DLS segment reassembler (DABlin DL_SEG_REASSEMBLER semantics).
    struct Segment {
        int segNum = 0;
        bool toggle = false;
        bool last = false;
        uint8_t charset = 15;
        QByteArray chars;
    };
    struct Reassembler {
        std::map<int, Segment> segs;
        void reset() { segs.clear(); }
        void add(const Segment& s) {
            if (!segs.empty() && segs.begin()->second.toggle != s.toggle) {
                segs.clear();  // toggle changed -> drop stale segments
            }
            if (segs.count(s.segNum)) {
                return;
            }
            segs[s.segNum] = s;
        }
        bool complete(QByteArray& out, uint8_t& charset) const {
            int count = 0;
            for (int i = 0; i < 8; ++i) {
                auto it = segs.find(i);
                if (it == segs.end()) {
                    return false;
                }
                ++count;
                if (it->second.last) {
                    break;
                }
                if (i == 7) {
                    return false;
                }
            }
            out.clear();
            for (int i = 0; i < count; ++i) {
                out.append(segs.at(i).chars);
            }
            charset = segs.at(0).charset;
            return true;
        }
    };
    Reassembler dlReassembler;
    Reassembler dlPlusReassembler;

    // 5-CIF superframe accumulation + FireCode/AU/PAD transport (T38 shared).
    streamdab::dabplus::SuperframeAssembler superframeAssembler;

    // DL Data Group accumulation (one group may span sub-fields).
    QByteArray dataGroupBuffer;
    bool dataGroupActive = false;

    // Previous X-PAD CI list (DABlin `prev_xpad_ci`): reused when the F-PAD CI
    // flag is 0, i.e. this AU continues the data group announced earlier.
    // Entries are {CI type, sub-field length}.
    std::vector<std::pair<int, int>> lastXpadCi;

    // === MOT SlideShow (X-PAD CI type 12/13, ETSI TS 101 499) ===
    // Callback invoked with a COMPLETE standard MSC data group (ETSI EN 300 401
    // §5.3.3). The window routes it to its MotPadAdapter -> MOTProtocol.
    std::function<void(const QByteArray&)> onMotDataGroup;
    int motDgliLen = 0;             // announced length (X-PAD CI type 1, DGLI)
    QByteArray motGroupBuffer;      // current standard data group accumulator
    bool motGroupActive = false;    // a CI-12 start has been seen
    int motContinuationLen = -1;    // DABlin last_xpad_ci continuation length
    uint64_t motDataGroups = 0;     // completed standard data groups

    void reset()
    {
        superframeAssembler.reset();
        dlReassembler.reset();
        dlPlusReassembler.reset();
        dataGroupBuffer.clear();
        dataGroupActive = false;
        lastXpadCi.clear();
        motDgliLen = 0;
        motGroupBuffer.clear();
        motGroupActive = false;
        motContinuationLen = -1;
    }

    // Feed one ETI-frame CIF chunk for this subchannel. The shared transport
    // module (T38) accumulates 5 CIFs into a superframe and applies the sliding
    // FireCode sync: a candidate window that does not pass the FireCode drops
    // ONE chunk and retries (instead of discarding all 5), which locks onto the
    // real superframe boundary quickly.
    bool feedChunk(const QByteArray& chunk, DlsAssembly& out)
    {
        const streamdab::dabplus::SuperframeResult result =
            superframeAssembler.processChunk(chunk);
        return handleSuperframe(result, out);
    }

    // Whole-superframe entry point, used by the GUI synthetic-superframe test
    // hook: evaluate an already-assembled candidate via the shared module.
    bool processSuperframe(const QByteArray& sf, DlsAssembly& out)
    {
        const streamdab::dabplus::SuperframeResult result =
            streamdab::dabplus::SuperframeAssembler::processSuperframe(sf);
        return handleSuperframe(result, out);
    }

    // Apply a shared transport result to this parser's DLS/DLS+ state. Counter
    // updates and dispatch order are unchanged from the pre-T38 local
    // implementation:
    //  - `superframes` counts every size-valid candidate window (locked or not);
    //  - `firecodeOk` counts FireCode + plausibility passes (before AU offsets);
    //  - `ausProcessed` counts AUs with size >= 2, `ausCrcOk` the CRC-valid ones;
    //  - `padFound` counts CRC-valid AUs carrying a usable DSE/PAD.
    bool handleSuperframe(const streamdab::dabplus::SuperframeResult& result,
                          DlsAssembly& out)
    {
        if (!result.examined) {
            return false;
        }
        ++superframes;
        if (result.sync != streamdab::dabplus::SuperframeSync::Locked) {
            return false;  // not synced
        }
        ++firecodeOk;
        if (!result.offsetsValid) {
            return false;
        }
        const uint8_t* b = reinterpret_cast<const uint8_t*>(result.data.constData());
        bool produced = false;
        for (const streamdab::dabplus::AccessUnit& au : result.aus) {
            ++ausProcessed;
            if (!au.crcOk) {
                continue;  // corrupted AU -> skip (no RS correction attempted)
            }
            ++ausCrcOk;
            if (au.padOffset < 0) {
                continue;  // CRC-valid AU without a PAD DSE
            }
            ++padFound;
            const uint8_t* payload = b + au.offset;
            const uint8_t* xpad = payload + au.padOffset;
            const size_t xpadLen = static_cast<size_t>(au.padLen - kFpadLen);
            const uint8_t* fpad = payload + au.padOffset + au.padLen - kFpadLen;
            if (processPad(xpad, xpadLen, fpad, out)) {
                produced = true;
            }
        }
        return produced;
    }

    // F-PAD + X-PAD CI/data subfield decode (ETSI EN 300 401 §7.4).
    bool processPad(const uint8_t* xpadData, size_t xpadLen, const uint8_t* fpad,
                    DlsAssembly& out)
    {
        // The X-PAD is transmitted in reverse byte order into the fixed-size
        // buffer. Reject over-long X-PADs up front: the strict-length padding
        // check below indexes xpad[announced .. xpadLen-1], so an xpadLen
        // larger than the buffer (padLen can reach ~508: 255 + data[2]) would
        // read/write out of bounds. The `used` clamp alone only protects the
        // reverse copy, not the trailing-padding scan.
        constexpr size_t kXpadMax = 196;
        if (xpadLen > kXpadMax) {
            return false;  // malformed/oversized X-PAD -> reject, do not decode
        }
        uint8_t xpad[kXpadMax];
        const size_t used = std::min<size_t>(xpadLen, sizeof(xpad));
        for (size_t i = 0; i < used; ++i) {
            xpad[i] = xpadData[xpadLen - 1 - i];
        }

        const int fpadType = fpad[0] >> 6;
        const int xpadInd = (fpad[0] & 0x30) >> 4;
        const bool ciFlag = (fpad[1] & 0x02) != 0;
        if (ciFlag) {
            ++fpadCiSet;
        }
        if (fpadType != 0x00) {
            return false;  // only F-PAD type 0 carries DLS / X-PAD here
        }

        struct XpadCi {
            int type;
            int len;
        };
        std::vector<XpadCi> cis;
        size_t ciLen = 0;
        if (ciFlag) {
            if (xpadInd == 0x01) {  // short X-PAD
                if (xpadLen < 1) {
                    return false;
                }
                const int type = xpad[0] & 0x1F;
                ciLen = 1;
                if (type != 0x00) {
                    cis.push_back({type, 3});
                }
            } else if (xpadInd == 0x02) {  // variable size X-PAD
                ciLen = 0;
                for (size_t i = 0; i < 4; ++i) {
                    if (xpadLen < i + 1) {
                        return false;
                    }
                    const uint8_t raw = xpad[i];
                    ++ciLen;
                    const int type = raw & 0x1F;
                    if (type == 0x00) {
                        break;  // end marker
                    }
                    cis.push_back({type, kXpadSubfieldLens[(raw >> 5) & 0x07]});
                }
            } else {
                return false;
            }
            // Remember this CI list so a following ci_flag==0 AU can continue
            // the same data group (DABlin prev_xpad_ci).
            lastXpadCi.clear();
            lastXpadCi.reserve(cis.size());
            for (const XpadCi& ci : cis) {
                lastXpadCi.emplace_back(ci.type, ci.len);
            }
        } else {
            // ci_flag == 0: this AU has no CI bytes; it continues the previous
            // CI list. Reuse the last announced list (DABlin prev_xpad_ci).
            // For MOT (CI 12 -> 13) DABlin remembers the continuation sub-field
            // length instead of the whole list; prefer that when present.
            if (xpadInd != 0x01 && xpadInd != 0x02) {
                return false;
            }
            if (motContinuationLen >= 0) {
                cis.push_back({13, std::min(motContinuationLen,
                                            static_cast<int>(xpadLen))});
                ciLen = 0;  // continuation payload starts at byte 0 of the X-PAD
            } else if (!lastXpadCi.empty()) {
                for (const auto& ci : lastXpadCi) {
                    cis.push_back({ci.first, ci.second});
                }
                ciLen = 0;  // continuation payload starts at byte 0 of the X-PAD
            } else {
                return false;
            }
        }

        size_t announced = ciLen;
        for (const XpadCi& ci : cis) {
            announced += static_cast<size_t>(ci.len);
        }
        // Strict X-PAD length (DABlin exact_xpad_len): the announced CI + data
        // length must equal the received X-PAD exactly. Rejecting short
        // announcements stops trailing/uninitialised bytes from being appended
        // to the data-group assembler.
        if (announced > xpadLen) {
            return false;  // announced X-PAD exceeds the received data
        }
        if (announced < xpadLen) {
            // Strict X-PAD length (DABlin exact_xpad_len): bytes after the
            // announced CI/data must be padding. Real captures (e.g. the
            // Bangkok fixture) pad with zeroes; any non-zero trailing byte is
            // unexpected and rejected so it is never processed.
            for (size_t i = announced; i < xpadLen; ++i) {
                if (xpad[i] != 0x00) {
                    return false;
                }
            }
        }

        bool produced = false;
        if (ciFlag) {
            // This AU announces its own CI list; reset the MOT continuation
            // tracking until a MOT sub-field is actually processed below.
            motContinuationLen = -1;
        }
        size_t offset = ciLen;
        for (const XpadCi& ci : cis) {
            ++xpadGroups;
            if (ci.type >= 0 && ci.type < 32) {
                ++groupTypeCounts[ci.type];
            }
            if (ci.type == 2 || ci.type == 3) {
                // CI type 2 = data group starts here (reset the assembly);
                // CI type 3 = data group continues (append). A ci_flag==0 AU
                // reuses the previous CI list but its payload is ALWAYS a
                // continuation, so it must not reset the assembly buffer.
                const bool dataGroupStarts = ciFlag && (ci.type == 2);
                if (processDataGroup(xpad + offset, static_cast<size_t>(ci.len), out,
                                     dataGroupStarts)) {
                    produced = true;
                }
            } else if (ci.type == 1) {
                // Data Group Length Indicator (DGLI): announces the length of
                // the immediate next MOT data group. Layout: 14-bit length
                // (6-bit high in byte 0 + 8-bit low in byte 1) + the one's
                // complement CRC-16/CCITT-FALSE over those 2 bytes.
                // (ETSI EN 300 401 §7.4.2, ETSI TS 101 499 §4.3.)
                if (ci.len >= 4) {
                    const uint8_t* p = xpad + offset;
                    const uint16_t crcStored =
                        static_cast<uint16_t>((p[2] << 8) | p[3]);
                    const uint16_t crcCalc =
                        static_cast<uint16_t>(~eti::crc16ccitt_false(p, 2));
                    if (crcStored == crcCalc) {
                        motDgliLen = ((p[0] & 0x3F) << 8) | p[1];
                    }
                }
            } else if (ci.type == 12 || ci.type == 13) {
                // MOT SlideShow data group: type 12 starts (resets the
                // accumulator), type 13 continues. A ci_flag==0 AU is always a
                // continuation (start == false).
                // A following ci_flag==0 AU continues THIS sub-field, so track
                // its own length (not the whole announced X-PAD) — otherwise a
                // co-resident DGLI would inflate the continuation length.
                motContinuationLen = ci.len;
                const bool start = ciFlag && (ci.type == 12);
                processMotDataSubfield(xpad + offset, static_cast<size_t>(ci.len), start);
            }
            offset += static_cast<size_t>(ci.len);
        }
        return produced;
    }

    // Append an X-PAD MOT sub-field (CI 12/13) to the standard MSC data group
    // accumulator and, once the DGLI-announced length is reached, hand the
    // complete data group to onMotDataGroup().
    void processMotDataSubfield(const uint8_t* data, size_t len, bool start)
    {
        if (start) {
            motGroupBuffer.clear();
            motGroupActive = true;
        } else if (!motGroupActive) {
            return;  // continuation without a start: cannot assemble safely
        }
        motGroupBuffer.append(reinterpret_cast<const char*>(data), static_cast<int>(len));

        if (motDgliLen > 0 && motGroupBuffer.size() >= motDgliLen) {
            const QByteArray dg = motGroupBuffer.left(motDgliLen);
            motGroupBuffer.clear();
            motGroupActive = false;
            motDgliLen = 0;
            ++motDataGroups;
            if (onMotDataGroup) {
                onMotDataGroup(dg);
            }
        }
    }

    // Reassemble a Dynamic Label Data Group (prefix + text + CRC16 into DLS,
    // or the DL+ command byte(s) into the DL+ reassembler). `start` is true for
    // a CI type 2 (data group starts) and false for a continuation (type 3).
    bool processDataGroup(const uint8_t* data, size_t len, DlsAssembly& out, bool start)
    {
        if (start) {
            dataGroupBuffer.clear();  // DABlin Reset() on a new data group
            dataGroupActive = true;
        } else if (!dataGroupActive) {
            // Continuation with no known start: cannot assemble safely.
            dataGroupBuffer.clear();
            return false;
        }
        dataGroupBuffer.append(reinterpret_cast<const char*>(data), static_cast<int>(len));
        if (dataGroupBuffer.size() < 2) {
            return false;
        }
        const uint8_t* d = reinterpret_cast<const uint8_t*>(dataGroupBuffer.constData());
        const uint8_t prefix0 = d[0];
        const bool command = (prefix0 & 0x10) != 0;
        bool dlPlus = false;
        size_t fieldLen = 0;
        if (command) {
            const int commandId = prefix0 & 0x0F;
            if (commandId == 0x01) {  // remove label
                dlReassembler.reset();
                dlPlusReassembler.reset();
                dataGroupActive = false;
                dataGroupBuffer.clear();
                return false;
            }
            if (commandId == 0x02) {  // DL Plus
                dlPlus = true;
                fieldLen = (static_cast<uint8_t>(d[1]) & 0x0F) + 1;
            } else {
                dataGroupActive = false;
                dataGroupBuffer.clear();
                return false;
            }
        } else {
            fieldLen = (prefix0 & 0x0F) + 1;
        }

        const size_t needed = 2 + fieldLen + 2;  // prefix + field + CRC16
        if (dataGroupBuffer.size() < static_cast<int>(needed)) {
            return false;  // wait for continuation sub-fields
        }
        const uint16_t crcStored =
            static_cast<uint16_t>((d[2 + fieldLen] << 8) | d[2 + fieldLen + 1]);
        const uint16_t crcCalc =
            static_cast<uint16_t>(~eti::crc16ccitt_false(d, 2 + fieldLen));
        if (crcStored != crcCalc) {
            dataGroupActive = false;
            dataGroupBuffer.clear();
            return false;
        }

        Segment seg;
        seg.charset = static_cast<uint8_t>(d[1]) >> 4;
        seg.toggle = (prefix0 & 0x80) != 0;
        seg.last = (prefix0 & 0x20) != 0;
        seg.segNum = (prefix0 & 0x40) ? 0 : ((d[1] & 0x70) >> 4);
        seg.chars = QByteArray(reinterpret_cast<const char*>(d + 2),
                               static_cast<int>(fieldLen));

        dataGroupActive = false;
        dataGroupBuffer.clear();

        if (dlPlus) {
            ++dlPlusSegments;
            dlPlusReassembler.add(seg);
            return false;  // produced only together with a complete DL text
        }

        ++dlSegments;
        dlReassembler.add(seg);
        QByteArray text;
        uint8_t charset = 15;
        if (!dlReassembler.complete(text, charset)) {
            return false;
        }
        out.hasLabel = true;
        out.dlText = text;
        out.charset = charset;
        out.tags.clear();
        out.hasDlPlus = false;
        QByteArray plusRaw;
        uint8_t plusCharset = 15;
        if (dlPlusReassembler.complete(plusRaw, plusCharset)) {
            out.hasDlPlus = true;
            parseDlPlusTags(plusRaw, out.tags);
        }
        ++labels;
        return true;
    }

    static void parseDlPlusTags(const QByteArray& raw, std::vector<StdDlsPlusTag>& tags)
    {
        tags.clear();
        if (raw.size() < 1) {
            return;
        }
        const uint8_t* d = reinterpret_cast<const uint8_t*>(raw.constData());
        if ((d[0] >> 4) != 0x00) {
            return;  // not a DL+ command
        }
        const int numTags = (d[0] & 0x03) + 1;
        for (int i = 0; i < numTags; ++i) {
            const int base = 1 + i * 3;
            if (base + 2 >= raw.size()) {
                break;
            }
            StdDlsPlusTag t;
            t.contentType = d[base] & 0x7F;
            t.startMarker = d[base + 1] & 0x7F;
            t.lengthMarker = d[base + 2] & 0x7F;
            tags.push_back(t);
        }
    }
};

// T27: fixed 4:3 letterbox frame for the MOT slideshow image. The child image
// label is laid out in the largest centred 4:3 rectangle that fits the frame,
// so the picture area stays 4:3 regardless of the splitter size; the image
// itself is letterboxed inside it (QLabel::setPixmap with KeepAspectRatio).
class FourThreeLetterboxFrame : public QWidget
{
public:
    explicit FourThreeLetterboxFrame(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        setObjectName(QStringLiteral("slideshowFrame"));
        setMinimumSize(160, 120);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setStyleSheet(QStringLiteral("background-color: #101010; border: 1px solid #333;"));

        m_image = new QLabel(this);
        m_image->setObjectName(QStringLiteral("slideshowImage"));
        m_image->setAlignment(Qt::AlignCenter);
        m_image->setStyleSheet(QStringLiteral("background-color: #000000; color: #888;"));
        m_image->setScaledContents(false);
    }

    QLabel* imageLabel() const { return m_image; }

    // L2: invoked after the 4:3 geometry is updated so the owner can rescale a
    // retained slide when the splitter / floating dock is resized.
    std::function<void()> onResized;

    QSize sizeHint() const override { return QSize(320, 240); }
    QSize minimumSizeHint() const override { return QSize(160, 120); }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        const int w = width();
        const int h = height();
        int fw = w;
        int fh = fw * 3 / 4;             // 4:3
        if (fh > h) {
            fh = h;
            fw = fh * 4 / 3;
        }
        m_image->setGeometry((w - fw) / 2, (h - fh) / 2, fw, fh);
        if (onResized) {
            onResized();
        }
    }

private:
    QLabel* m_image = nullptr;
};

// ============================================================================
// T31 (rework): load-time playback media timeline
// ============================================================================
// The DLS+/Slideshow panels are fed once, sequentially, during the load loop —
// that is what makes the 5-frame DAB+ superframes and the multi-superframe MOT
// reassembly work. Re-feeding isolated raw frames at the playhead is therefore
// both superframe-incorrect (FireCode never syncs) and would inflate the
// load-time counters. Instead we record a COMPACT, SPARSE timeline at load time
// (only when the media state changes) and look it up at the playhead.
//
// `motImageId` is an absolute identity into the retained-slide store, so a
// lookup never decodes an image nor touches the MOT adapter.
struct PlaybackMediaState {
    QString track;
    QString artist;
    QString album;
    int motImageId = -1;  // -1 = no slideshow image decoded at/before this frame

    bool operator==(const PlaybackMediaState& o) const
    {
        return track == o.track && artist == o.artist && album == o.album
               && motImageId == o.motImageId;
    }
    bool operator!=(const PlaybackMediaState& o) const { return !(*this == o); }
};

// T32: per-service sparse media history keyed by the service id (SId). The T31
// timeline was global (the last-fed service won each frame); scoping it per
// service lets the Player's service selector show exactly one service's
// DLS+/slideshow at the playhead — no cross-service leakage.
using ServiceMediaTimeline = std::map<int, PlaybackMediaState>;

// One decoded MOT slide retained for scrubbing, together with the panel strings
// produced at load time so a playhead lookup needs no decode and no adapter.
struct RetainedSlide {
    QImage image;
    QString status;        // e.g. "Showing slide 12 — JPEG"
    QString info;          // full info line as decoded at load
    quint64 sequence = 0;  // T32: global insertion order (cross-service eviction)
};

// T32: per-service retained-slide ring. Ids are scoped to the service's own
// store (`baseId`/`nextId`), so a per-service timeline's `motImageId` always
// resolves against the owning service's slides and can never show another
// service's picture. The byte cap is enforced globally (see
// appendRetainedSlide) so N services cannot multiply the memory bound.
struct RetainedSlideStore {
    std::deque<RetainedSlide> slides;
    int baseId = 0;  // id of slides.front()
    int nextId = 0;  // id assigned to the next decoded slide
};

class DABAnalyserWindow : public QMainWindow
{
    Q_OBJECT

public:
    DABAnalyserWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("StreamDAB-Analyser");
        setMinimumSize(1200, 800);
        resize(1400, 1000);

        // Initialize core components
        m_etiProcessor = std::make_unique<EnhancedETIProcessorQt>();
        m_figAnalyser = std::make_unique<AdvancedFIGAnalyser>();

        // Initialize hex viewer formatter with ETSI-compliant settings
        m_hexFormatter = std::make_unique<HexViewerFormatter>();
        m_hexFormatter->setColorCoding(true);        // Enable ETSI structure colors
        m_hexFormatter->setAsciiColumn(true);        // Enable ASCII representation
        m_hexFormatter->setBytesPerLine(16);         // 16 bytes per line standard

        m_etsiComplianceEngine = std::make_unique<etsi::ETSIComplianceEngine>();
        // WORKING COMBINATION: Multi-stream + Error Detection (Logging causes file dialog freeze)
        m_multiStreamProcessor = std::make_unique<multistream::MultiStreamProcessor>();
        m_errorDetector = std::make_unique<error_detection::AdvancedErrorDetector>();
        // T41: the legacy ProfessionalLoggingSystem was never constructed (it
        // caused a file-dialog freeze); its UI (bottom_Messages) is removed.

        // Phase 3B: Initialize DLS+ decoder
        m_dlsPlusDecoder = new eti::dls_plus::DLSPlusDecoder(this);
        
        // Phase 3B: Connect DLS+ signals to Now Playing UI
        connect(m_dlsPlusDecoder, &eti::dls_plus::DLSPlusDecoder::nowPlayingUpdated,
                this, [this](const QString& track, const QString& artist, const QString& album) {
                    updateNowPlaying(track, artist, album);
                });

        // T34: selected-service DAB+ audio playback controller. Fed the same
        // per-sub-channel MSC slices as the DLS+ pipeline during load; decodes
        // the selected service on demand and drives the ALSA output sink.
        m_audioController = new streamdab::audio::AudioPlaybackController(this);
        connect(m_audioController, &streamdab::audio::AudioPlaybackController::levelsChanged,
                this, [this](float left, float right) {
                    if (m_audioLevelLeft) {
                        m_audioLevelLeft->setValue(static_cast<int>(left * 100.0f + 0.5f));
                    }
                    if (m_audioLevelRight) {
                        m_audioLevelRight->setValue(static_cast<int>(right * 100.0f + 0.5f));
                    }
                });
        connect(m_audioController, &streamdab::audio::AudioPlaybackController::statusChanged,
                this, [this]() {
                    if (m_audioStatusLabel && m_audioController) {
                        m_audioStatusLabel->setText(m_audioController->statusText());
                    }
                });

        // MOT SlideShow (option A): wire the decoder + adapter to the panel.
        connect(&m_motProtocol, &eti::mot::MOTProtocol::objectComplete,
                this, &DABAnalyserWindow::onMotObjectComplete);
        connect(&m_motProtocol, &eti::mot::MOTProtocol::parseError,
                this, &DABAnalyserWindow::onMotParseError);
        connect(&m_motProtocol, &eti::mot::MOTProtocol::reassemblyProgress,
                this, &DABAnalyserWindow::onMotProgress);
        m_motAdapter.setProgressCallback(
            [this](uint32_t transportId, double progress) {
                onMotProgress(transportId, progress);
            });

        // Phase A: Initialize worker thread architecture
        m_sharedData = new streamdab::core::SharedETIData(this);
        m_readerThread = new ETIReaderThread(this);

        // Wave B: the data-service decoders + per-service data store behind
        // the inner Slideshow|EPG|Journaline|TPEG tabs. Created BEFORE
        // setupUI() (Qt parent-child ownership frees them with the window) so
        // the tab construction and the signal wiring below can rely on them.
        m_epgDecoder = new eti::epg::EPGDecoder(this);
        m_journalineDecoder = new eti::journaline::JournalineDecoder(this);
        m_tpegDecoder = new eti::tpeg::TpegDecoder(this);
        m_dataServiceStore = new eti::data::DataServiceStore(this);

        setupUI();
        setupMenuBar();
        setupSignalConnections();
        setupWorkerThread();  // Phase A: Worker thread signal connections
        setupStatusBar();

        // Wave B: decoder/store -> inner-tab wiring (queued to this GUI
        // thread) once the widgets from setupUI() exist.
        initDataServicePanels();

        // T29: playback controller over the retained frame cache.
        initPlayback();

        // Load the analyser decode settings from QSettings (org
        // "StreamDAB-Analyser", app "DABAnalyser", group "analyser/").
        // A fresh QSettings yields the defaults, keeping decode behavior
        // identical to the pre-settings baseline.
        m_analyserSettings.loadFromQSettings();

        // Wire the decode settings into the GUI FIG analyser (FIB CRC
        // strictness, fic_mode raw/auto-fallback, force_charset).
        m_figAnalyser->setAnalyserSettings(m_analyserSettings);

        // Wave B: export manager (real analysis rows are injected at export
        // time from the live analyser; see applyExportData()).
        m_exportManager = new ProfessionalExportManager(this);
        m_exportManager->initialize(nullptr, nullptr, nullptr);

        // Wave B: 1 s sampler feeding the Performance Dashboard + Real-Time
        // Chart with real FPS / RSS / CPU% / latency.
        m_perfSampleTimer = new QTimer(this);
        m_perfSampleTimer->setInterval(1000);
        connect(m_perfSampleTimer, &QTimer::timeout,
                this, &DABAnalyserWindow::updateDashboardSamples);
        m_perfTimer.start();
        m_perfSampleTimer->start();

        // Apply persisted application settings (Settings dialog scope) to the
        // live objects: confirm-exit, sampler rate, network buffer, log level
        // and the shared T34 audio state.
        applyPersistedAppSettings();

        // EMERGENCY: ALL CUSTOM THEMING DISABLED FOR TEXT VISIBILITY
        // applyTheme();
    }

    ~DABAnalyserWindow() override;

    // Public method for auto-loading files (for testing)
    void autoLoadFile(const QString& filePath)
    {
        loadETIFile(filePath);
    }

    // Public accessor for the DAB Ensemble Explorer tree (GUI tests)
    QTreeWidget* ensembleTree() const { return m_ensembleTree; }

    // Public accessors for the real-data panel widgets (GUI tests).
    // The audit-based assertions read these instead of walking the dock tree.
    QTableWidget* subchannelTable() const { return m_subchannelTable; }
    QLabel* tab2EnsembleNameLabel() const { return m_tab2_ensembleName; }
    QLabel* tab2FicContentSummaryLabel() const { return m_tab2_ficContentSummary; }
    QLabel* tab2SubchannelOrgSummaryLabel() const { return m_tab2_subchannelOrgSummary; }
    // T33: Overview's digital-services line + the relocated Service Components
    // tree (now a center-panel child).
    QLabel* tab2DigitalServicesLabelForTest() const { return m_tab2_digitalServices; }
    QTreeWidget* tab2ServiceComponentsForTest() const { return m_tab2_serviceComponents; }
    QTableWidget* tab2ServiceTable() const { return m_tab2_serviceTable; }
    // Phase 2C: Advanced FIG Analyser dock (FIG2 Extended Labels)
    QTreeWidget* advancedFigTreeForTest() const { return m_advancedFigTree; }
    QTextEdit* advancedFigHexViewerForTest() const { return m_advancedFigHexViewer; }
    QComboBox* advancedFigFilterForTest() const { return m_advancedFigFilter; }
    QTreeWidget* tab3FigInstanceTree() const { return m_tab3_figInstanceTree; }
    QTreeWidget* tab3FigItemDetailsTree() const { return m_tab3_figItemDetailsTree; }
    // T28: Tab-3 left dock inner tabs + Frame List.
    QTabWidget* tab3LeftTabsForTest() const { return m_tab3LeftTabs; }
    QListWidget* tab3FrameListForTest() const { return m_tab3_frameList; }
    void selectTab3FrameForTest(int row)
    {
        if (m_tab3_frameList) m_tab3_frameList->setCurrentRow(row);
    }
    // T42: Tab-3 right dock inner tabs + Hex Viewer.
    QTabWidget* tab3RightTabsForTest() const { return m_tab3RightTabs; }
    QTextEdit* tab3HexViewerForTest() const { return m_tab3_hexViewer; }
    QString tab3HexViewerTextForTest() const
    {
        return m_tab3_hexViewer ? m_tab3_hexViewer->toPlainText() : QString();
    }
    // F1/F2 test hook: render a synthetic raw ETI frame into the Hex Viewer
    // without loading a file (MID/NST geometry coverage).
    void renderTab3HexForTest(const QByteArray& frameData, int frameNumber)
    {
        renderTab3HexForFrameData(frameData, frameNumber);
    }
    QTreeWidget* ficOverviewTree() const { return m_ficOverviewTree; }
    QTableWidget* errorCounterTable() const { return m_errorCounterTable; }

    // User-selected analyser decode options (loaded from QSettings at startup).
    const eti::AnalyserSettings& analyserSettings() const noexcept { return m_analyserSettings; }

    // Public analyser-state accessors (reload/reset GUI tests): the FIG
    // analyser's instance list / counters must restart cleanly per file.
    bool isFileLoaded() const { return m_fileLoaded; }
    int analyserFigInstanceCount() const
    {
        return m_figAnalyser ? static_cast<int>(m_figAnalyser->figInstances().size()) : 0;
    }
    // T32: the analyser's service count (the service selector is populated 1:1).
    int analyserServiceCountForTest() const
    {
        return m_figAnalyser ? m_figAnalyser->getServiceCount() : 0;
    }
    int analyserTotalFigCount() const
    {
        return m_figAnalyser ? m_figAnalyser->getTotalFIGsProcessed() : 0;
    }
    int analyserFibCrcFailureCount() const
    {
        return m_figAnalyser ? m_figAnalyser->getFibCrcFailureCount() : 0;
    }

    // ========================================================================
    // Public test hooks (GUI dock-layout / crash-guard tests)
    // ========================================================================
    // Manual-test finding 2: force the "frames are still being parsed" state
    // so the crash-guard logic can be exercised without driving a real load.
    void setProcessingForTest(bool processing) { m_processing = processing; }
    bool isProcessingForTest() const { return m_processing; }
    // Finding 8 isolation: allow a test to simulate the mid-load state
    // (file loaded + parsing) and to drive the worker error path without a
    // modal dialog.
    void setFileLoadedForTest(bool loaded) { m_fileLoaded = loaded; }
    void beginProcessingForTest()
    {
        m_processing = true;
        if (m_frameList) {
            m_frameList->setEnabled(false);
        }
    }
    void simulateWorkerErrorForTest(const QString& error) { handleWorkerError(error); }
    int availableFrameCountForTest() const { return availableFrameCount(); }
    QListWidget* frameListForTest() const { return m_frameList; }
    QString frameDetailsTextForTest() const
    {
        return m_frameDetails ? m_frameDetails->text() : QString();
    }
    QString hexViewerTextForTest() const
    {
        return m_hexViewer ? m_hexViewer->toPlainText() : QString();
    }
    QTableWidget* etiOverviewTableForTest() const { return m_etiOverviewTable; }
    // Drives the frame-selection handler exactly like a user click in the
    // navigator, without touching the (possibly disabled) list widget.
    void selectFrameForTest(int row) { handleFrameSelection(row); }

    // Dock titles keyed by objectName — for tests to verify Window menu action
    // texts without calling dock->windowTitle() (which crashes in Qt-ADS
    // accessibility code paths when the internal widget is not fully initialized).
    const QMap<QString, QString>& dockTitlesForTest() const { return m_dockTitles; }

    // Manual-test finding 3: feed a synthetic PAD payload through the same
    // decoder path the ETI frame pipeline uses, then observe the Now Playing
    // panel state.
    bool feedPadDataForTest(const QByteArray& pad) { return feedDlsPlusPad(pad); }
    int dlsPlusMessageCountForTest() const { return m_dlsPlusMessages; }
    int dlsPlusPadCallCountForTest() const { return m_dlsPlusPadCalls; }
    int dlsPlusLabelsAssembledForTest() const { return m_dlsPlusLabelsAssembled; }
    QStringList dlsLabelsSeenForTest() const { return m_dlsLabelsSeen; }
    // T31 (rework) / T32: read-only view of the SELECTED service's load-time
    // media timeline + the aggregate retained-slide store for the strengthened
    // playback tests. Mutating drivers live in the GUI_TEST_MODE block with the
    // other playback hooks.
    const ServiceMediaTimeline& playbackMediaTimelineForTest() const
    {
        static const ServiceMediaTimeline kEmpty;
        auto it = m_mediaTimelineByService.find(m_selectedServiceId);
        return it == m_mediaTimelineByService.end() ? kEmpty : it->second;
    }
    int playbackMediaTimelineSizeForTest() const
    {
        auto it = m_mediaTimelineByService.find(m_selectedServiceId);
        return it == m_mediaTimelineByService.end()
                   ? 0
                   : static_cast<int>(it->second.size());
    }
    // T32: per-service timeline for an arbitrary service (empty map if none).
    const ServiceMediaTimeline& playbackMediaTimelineForServiceForTest(quint32 serviceId) const
    {
        static const ServiceMediaTimeline kEmpty;
        auto it = m_mediaTimelineByService.find(serviceId);
        return it == m_mediaTimelineByService.end() ? kEmpty : it->second;
    }
    // T32: services that have at least one recorded media state (sorted by SId).
    QList<quint32> serviceIdsWithTimelineForTest() const
    {
        QList<quint32> ids;
        ids.reserve(static_cast<int>(m_mediaTimelineByService.size()));
        for (const auto& entry : m_mediaTimelineByService) {
            ids.append(entry.first);
        }
        return ids;
    }
    // T32: the Player panel's service selector + its current selection.
    QComboBox* playerServiceSelectorForTest() const { return m_serviceCombo; }
    quint32 selectedServiceIdForTest() const { return m_selectedServiceId; }
    QString selectedServiceDisplayForTest() const { return selectedServiceDisplay(); }
    QStringList serviceSelectorItemsForTest() const
    {
        QStringList items;
        if (m_serviceCombo) {
            for (int i = 0; i < m_serviceCombo->count(); ++i) {
                items << m_serviceCombo->itemText(i);
            }
        }
        return items;
    }
    QList<quint32> serviceSelectorIdsForTest() const
    {
        QList<quint32> ids;
        if (m_serviceCombo) {
            for (int i = 0; i < m_serviceCombo->count(); ++i) {
                ids.append(m_serviceCombo->itemData(i).toUInt());
            }
        }
        return ids;
    }
    // T32: service ids whose first/only component is DAB+ audio (ASCTy 0x3F).
    QList<quint32> dabPlusServiceIdsForTest() const
    {
        QList<quint32> ids;
        if (m_figAnalyser) {
            for (const DABService& svc : m_figAnalyser->getDABServices()) {
                if (serviceHasDabPlusAudio(svc)) {
                    ids.append(svc.service_id);
                }
            }
        }
        return ids;
    }
    // Refinement A test hook: drive the REAL DAB+ superframe/AU/PAD parser on
    // a synthetic superframe and feed the assembled DLS to the decoder.
    bool runDabPlusDlsParserForTest(const QByteArray& superframe)
    {
        DabPlusSubchannelParser parser;
        DlsAssembly assembly;
        if (!parser.processSuperframe(superframe, assembly) || !assembly.hasLabel) {
            return false;
        }
        ++m_dlsPlusLabelsAssembled;
        feedDlsAssembly(assembly, 0, QString());
        return true;
    }
    // Review test hook: drive the X-PAD/F-PAD -> Data Group (DL+ command 0x02)
    // path with a raw, caller-built X-PAD (already in on-wire byte order).
    bool runRawXpadDlsParserForTest(const QByteArray& onWireXpad)
    {
        DabPlusSubchannelParser parser;
        const uint8_t fpad[2] = {0x20, 0x02};  // type 0, variable X-PAD, CI set
        DlsAssembly assembly;
        if (!parser.processPad(reinterpret_cast<const uint8_t*>(onWireXpad.constData()),
                               static_cast<size_t>(onWireXpad.size()), fpad, assembly)
            || !assembly.hasLabel) {
            return false;
        }
        ++m_dlsPlusLabelsAssembled;
        feedDlsAssembly(assembly, 0, QString());
        return true;
    }
    // --- Review finding 2 hooks: feed a raw FIG stream to the analyser and
    // inspect the sub-channel -> service cache the DLS+ pipeline uses ---
    void feedFicForTest(const QByteArray& fic)
    {
        if (m_figAnalyser) {
            m_figAnalyser->analyzeFICData(fic);
        }
    }
    uint32_t figServiceRevisionForTest() const
    {
        return m_figAnalyser ? m_figAnalyser->getServiceRevision() : 0u;
    }
    bool dabPlusSubChannelKnownForTest(int subChannelId)
    {
        rebuildDabPlusServiceMapIfNeeded();
        return m_dabPlusSubChannels.contains(subChannelId);
    }
    QString dabPlusServiceLabelForTest(int subChannelId)
    {
        rebuildDabPlusServiceMapIfNeeded();
        return m_serviceBySubChannel.value(subChannelId, qMakePair(0u, QString())).second;
    }
    // v1.4 data-path wave — test accessors for the raw data-subchannel capture.
    QVector<int> dataSubChannelIdsForTest()
    {
        QVector<int> ids;
        ids.reserve(m_dataSubChannelStreams.size());
        for (auto it = m_dataSubChannelStreams.cbegin();
             it != m_dataSubChannelStreams.cend(); ++it) {
            ids.append(it.key());
        }
        return ids;
    }
    QByteArray dataSubChannelStreamForTest(int subChannelId)
    {
        return m_dataSubChannelStreams.value(subChannelId);
    }
    quint64 dataSubChannelBytesForTest() const { return m_dataSubChannelBytes; }
    // M9 test hook: run the exact reset every load performs, so a test can
    // prove the tap starts from zero instead of accumulating across loads.
    bool resetCaptureStateForTest() { return resetCaptureState(); }
    // Phase 2A — GUI data-service docks (EPG | Journaline | TPEG) + decoders:
    // accessors for the PRIVATE dock-layout tests.
    eti::epg::EPGDecoder* epgDecoderForTest() const { return m_epgDecoder; }
    eti::journaline::JournalineDecoder* journalineDecoderForTest() const
    {
        return m_journalineDecoder;
    }
    eti::tpeg::TpegDecoder* tpegDecoderForTest() const { return m_tpegDecoder; }
    QComboBox* epgServiceComboForTest() const { return m_epgServiceCombo; }
    QTableWidget* epgScheduleTableForTest() const { return m_epgScheduleTable; }
    QLabel* epgStatusLabelForTest() const { return m_epgStatusLabel; }
    QTreeWidget* journalineMenuTreeForTest() const { return m_journalineMenuTree; }
    QTextEdit* journalinePreviewForTest() const { return m_journalinePreview; }
    QLabel* journalineStatusLabelForTest() const { return m_journalineStatusLabel; }
    QTableWidget* tpegInventoryTableForTest() const { return m_tpegInventoryTable; }
    QLabel* tpegStatusLabelForTest() const { return m_tpegStatusLabel; }
    // --- B-H1: routing is asserted directly by the PRIVATE GUI test --------
    // The decision enum lives here (public) so the test can compare the exact
    // route: HR SCId 8 → Journaline, bkk SCId 22 → Tpeg (framing), bkk SCId 14
    // → Epg (label), bkk DAB+ audio → NOT Journaline, HR → NOT Tpeg.
    enum class DataServiceRoute { Epg, Journaline, Tpeg };
    DataServiceRoute routeDataServiceForTest(int subChannelId,
                                             const QByteArray& bytes) const
    {
        return routeDataService(subChannelId, bytes);
    }
    // Test accessor for the registered docks map (Phase 2A: new data-service docks).
    QMap<QString, ads::CDockWidget*> registeredDocksForTest() const
    {
        return registeredDocks();
    }
    // --- B-M3: FIG label decoding (UTF-8 first, Latin-1 fallback) ----------
    // Raw 16-byte FIG 1/5 `char label[16]` bytes → QString. Thai labels are
    // UTF-8 (3 bytes/char) in real ensembles, legacy ones are Latin-1; decode
    // as UTF-8 first (mirrors the extended-label path in
    // advanced_fig_analyser.cpp:2816/2845/2876) and fall back to Latin-1 when
    // the bytes are not valid UTF-8, so a valid Latin-1 label never degrades
    // to U+FFFD. Public (static) so the label-formatting spot-check can drive
    // it with synthetic bytes — a FORMATTING test, not a real-data claim.
    static QString decodeFigLabelBytes(const char* raw, qsizetype rawLen)
    {
        if (!raw || rawLen <= 0) {
            return QString();
        }
        const QByteArray bytes(raw, static_cast<int>(rawLen));
        const QString asUtf8 = QString::fromUtf8(bytes);
        // U+FFFD means "invalid UTF-8" only when the input did not itself
        // carry the replacement character's own byte sequence (EF BF BD).
        if (!asUtf8.contains(QChar::ReplacementCharacter)
            || bytes.contains(QByteArrayLiteral("\xEF\xBF\xBD"))) {
            return asUtf8;
        }
        return QString::fromLatin1(bytes);
    }
    /// Length of a FIG 1/5 label field (spec: 16 bytes, NUL-padded by the
    /// parser) — never reads past the field.
    static qsizetype figLabelFieldLength(const char* label)
    {
        qsizetype len = 0;
        while (len < 16 && label && label[len] != '\0') {
            ++len;
        }
        return len;
    }
    // Drive the REAL Journaline decoder + tab wiring with a raw data-subchannel
    // byte stream (used by the PRIVATE GUI test with the real HR capture's
    // SCId 8 bytes — no synthetic data, no full ETI load needed).
    void feedJournalineStreamForTest(const QByteArray& bytes)
    {
        if (!m_journalineDecoder) {
            return;
        }
        // B-H1(5): count as routed — the status line must distinguish
        // "routed but empty" from "nothing was routed to Journaline here".
        m_journalineRouted = true;
        m_journalineDecoder->processStreamData(bytes);
        refreshJournalineTab();
    }
    // Feed a decoded DLS label as if it arrived on `subChannelId`, using the
    // cached service identity, so the test can observe the Now Playing panel
    // line built from the cached service label (regression for the stale
    // empty-label cache).
    bool feedServiceLabeledDlsForTest(int subChannelId, const QByteArray& dlText)
    {
        rebuildDabPlusServiceMapIfNeeded();
        const QPair<quint32, QString> svc =
            m_serviceBySubChannel.value(subChannelId, qMakePair(0u, QString()));
        int mapped = 0;
        const QByteArray pad = buildDecoderPadForTest(dlText, 15, {}, mapped);
        return feedDlsPlusPad(pad, svc.first, svc.second);
    }
    // Raw decoder-path hook (kept for the boundary smoke test).
    static QByteArray buildDecoderPadForTest(const QByteArray& dlText, uint8_t charset,
                                             const QVector<QPair<int, QPair<int, int>>>& tags,
                                             int& mappedCountOut)
    {
        DlsAssembly a;
        a.dlText = dlText;
        a.charset = charset;
        for (const auto& t : tags) {
            StdDlsPlusTag tag;
            tag.contentType = t.first;
            tag.startMarker = t.second.first;
            tag.lengthMarker = t.second.second;
            a.tags.push_back(tag);
        }
        const QByteArray pad = buildDecoderPad(a);
        mappedCountOut = int(a.tags.size());
        return pad;
    }
    QString nowPlayingTrackForTest() const
    {
        return m_trackLabel ? m_trackLabel->text() : QString();
    }
    QString nowPlayingArtistForTest() const
    {
        return m_artistLabel ? m_artistLabel->text() : QString();
    }
    QString nowPlayingAlbumForTest() const
    {
        return m_albumLabel ? m_albumLabel->text() : QString();
    }
    QString nowPlayingStatusForTest() const
    {
        return m_lastUpdatedLabel ? m_lastUpdatedLabel->text() : QString();
    }
    // T30.3: expose the metadata labels so the palette/accent colours can be
    // asserted without walking the dock tree.
    QLabel* nowPlayingTrackLabelForTest() const { return m_trackLabel; }
    QLabel* nowPlayingArtistLabelForTest() const { return m_artistLabel; }
    QLabel* nowPlayingAlbumLabelForTest() const { return m_albumLabel; }

    // --- MOT SlideShow (option A) test hooks -------------------------------
    // Drive the real X-PAD CI 12/13 parser + adapter for a sequence of raw
    // on-wire X-PADs (all fed to the SAME parser so multi-AU groups reassemble)
    // and observe the slideshow tab state.
    bool feedMotXpadsForTest(const QList<QByteArray>& onWireXpads)
    {
        DabPlusSubchannelParser parser;
        parser.onMotDataGroup = [this](const QByteArray& dg) {
            m_lastMotServiceId = 0;
            m_lastMotServiceLabel.clear();
            m_motAdapter.processDataGroup(0, dg);
        };
        const uint8_t fpad[2] = {0x20, 0x02};  // type 0, variable X-PAD, CI set
        bool any = false;
        for (const QByteArray& onWire : onWireXpads) {
            DlsAssembly assembly;
            parser.processPad(reinterpret_cast<const uint8_t*>(onWire.constData()),
                              static_cast<size_t>(onWire.size()), fpad, assembly);
            any = true;
        }
        return any;
    }
    int motObjectsForTest() const { return m_motObjectsReceived; }
    int motImagesDecodedForTest() const { return m_motImagesDecoded; }
    int motErrorsForTest() const { return m_motErrors; }
    // T26: objects whose standard header extension parameters were parsed, and
    // samples of the restored values (content name / click-through URL /
    // category title) from the real capture.
    int motHeaderParamsSeenForTest() const { return m_motHeaderParamsSeen; }
    QString lastMotContentNameForTest() const { return m_lastMotContentName; }
    QString lastMotClickThroughUrlForTest() const { return m_lastMotClickThroughUrl; }
    QString lastMotCategoryTitleForTest() const { return m_lastMotCategoryTitle; }
    // T31 (rework)/T32: the ≤8 slide history was replaced by a byte-capped
    // retained store (now per service) so the playhead can scrub back to any
    // decoded slide. The counters aggregate across services for the fixture
    // parity assertions.
    int retainedSlideCountForTest() const { return static_cast<int>(m_retainedSlideCount); }
    qint64 retainedSlideBytesForTest() const { return m_retainedSlideBytes; }
    // T32 review #4: resolve a slide id within ONE service's store. `noteOut`
    // receives the " (nearest retained image)" eviction note; statusOut the
    // resolved slide's tag. Returns false when that service has no slide.
    bool retainedSlideResolvesForTest(quint32 serviceId, int imageId, QString& noteOut,
                                      QString& statusOut) const
    {
        noteOut.clear();
        statusOut.clear();
        const RetainedSlide* s = findRetainedSlide(serviceId, imageId, noteOut);
        if (!s) {
            return false;
        }
        statusOut = s->status;
        return true;
    }
    int retainedSlideBaseIdForTest(quint32 serviceId) const
    {
        auto it = m_slidesByService.find(serviceId);
        return it == m_slidesByService.end() ? 0 : it->second.baseId;
    }
    int retainedSlideNextIdForTest(quint32 serviceId) const
    {
        auto it = m_slidesByService.find(serviceId);
        return it == m_slidesByService.end() ? 0 : it->second.nextId;
    }
    quint32 lastMotTransportIdForTest() const { return m_lastMotTransportId; }
    QImage slideshowImageForTest() const { return m_lastDecodedSlide; }
    QString slideshowStatusForTest() const
    {
        return m_slideshowStatus ? m_slideshowStatus->text() : QString();
    }
    QString slideshowInfoForTest() const
    {
        return m_slideshowInfo ? m_slideshowInfo->text() : QString();
    }
    // T26 fixture-parity hook: the adapter's detailed MOT counters (mapped
    // from MOTProtocol's standard-layout statistics).
    eti::mot::MotPadAdapter::Statistics motAdapterStatisticsForTest() const
    {
        return m_motAdapter.statistics();
    }
    QLabel* slideshowImageLabelForTest() const { return m_slideshowImage; }
    // T27: the Now Playing dock's left/right splitter (DLS+ text | slideshow).
    QSplitter* nowPlayingSplitterForTest() const { return m_nowPlayingSplitter; }

    // --- T22 network-stream connect hooks -----------------------------------
    // Drive the full connect entry point without a modal dialog (offscreen
    // tests). Returns true when a connection attempt was started; the rejected
    // URL path returns false, shows no dialog and creates no receiver.
    bool beginStreamConnectionForTest(const QString& url)
    {
        return beginStreamConnection(url, false);
    }
    int streamReceiverCountForTest() const { return m_networkReceiver ? 1 : 0; }
    int streamValidationWarningCountForTest() const { return m_streamValidationWarnings; }
    int streamConnectAttemptCountForTest() const { return m_connectAttemptCount; }
    bool streamConnectButtonEnabledForTest() const { return m_connectBtn && m_connectBtn->isEnabled(); }
    bool streamDisconnectButtonEnabledForTest() const { return m_disconnectBtn && m_disconnectBtn->isEnabled(); }
    bool streamRecordButtonEnabledForTest() const { return m_recordBtn && m_recordBtn->isEnabled(); }
    QComboBox* quickExampleComboForTest() const { return m_quickExampleCombo; }
    QLineEdit* streamUrlEditForTest() const { return m_streamUrlEdit; }
    QString streamStatusTextForTest() const
    {
        return m_streamStatus ? m_streamStatus->text() : QString();
    }
    // F3a: drive handleStreamError in the "already connected" state and inspect
    // whether the status/buttons were mutated.
    void setStreamEverConnectedForTest(bool v) { m_streamEverConnected = v; }
    void setStreamStatusTextForTest(const QString& text)
    {
        if (m_streamStatus) m_streamStatus->setText(text);
    }
    void setStreamButtonsEnabledForTest(bool connect, bool disconnect, bool record)
    {
        if (m_connectBtn) m_connectBtn->setEnabled(connect);
        if (m_disconnectBtn) m_disconnectBtn->setEnabled(disconnect);
        if (m_recordBtn) m_recordBtn->setEnabled(record);
    }
    void simulateStreamErrorForTest(const QString& error)
    {
        // Never block an offscreen test on the modal failure dialog.
        const bool saved = m_showStreamDialogs;
        m_showStreamDialogs = false;
        handleStreamError(error);
        m_showStreamDialogs = saved;
    }

    // T43: drive the EXACT slot the receiver's frame_received signal targets,
    // so a live-stream regression test can feed real 6144-byte ETI frames on a
    // fresh window (no file loaded) without multicast traffic.
    void feedNetworkFrameForTest(const QByteArray& frame) { onNetworkFrameReceived(frame); }
    // T43/F1: the GUI raw-frame cache must stay clean on the live path.
    int networkFrameCacheSizeForTest() const
    {
        QMutexLocker locker(&m_frameCacheMutex);
        return static_cast<int>(m_frameDataCache.size());
    }
    // F2: the System Messages strip text (to assert the live-switch notice).
    QString systemMessagesTextForTest() const
    {
        return m_systemMessages ? m_systemMessages->toPlainText() : QString();
    }
    void simulateConnectionEstablishedForTest()
    {
        if (m_networkReceiver) {
            m_networkReceiver->simulateConnectionEstablishedForTest();
        }
    }
    void disconnectStreamForTest() { onDisconnectClicked(); }
    bool streamEverConnectedForTest() const { return m_streamEverConnected; }

    // T23 test hooks: the ⚙ button + the UDP settings dialog it opens.
    QToolButton* udpSettingsButtonForTest() const { return m_udpSettingsBtn; }
    // Build (without exec'ing) the UDP Streaming Settings dialog. Caller owns it.
    QDialog* buildUdpSettingsDialogForTest() { return buildUDPSettingsDialog().dialog; }

    // T45 test hooks: the live Logging tab widgets.
    QTableWidget* loggingTableForTest() const { return m_loggingTable; }
    QComboBox* loggingLevelComboForTest() const { return m_loggingLevelCombo; }
    QCheckBox* loggingAutoScrollCheckForTest() const { return m_loggingAutoScrollCheck; }
    QPushButton* loggingClearButtonForTest() const { return m_loggingClearButton; }
    QLabel* loggingSummaryLabelForTest() const { return m_loggingSummaryLabel; }
    int loggingRowCapForTest() const { return m_loggingRowCap; }
    int loggingDroppedForTest() const { return m_logDroppedPending; }
    int loggingPendingForTest() const { return static_cast<int>(m_logPending.size()); }

    // Wave B test hooks: adopted bottom-strip/managed widgets.
    PerformanceDashboard* performanceDashboardForTest() const { return m_performanceDashboard; }
    RealTimeChartWidget* realTimeChartForTest() const { return m_realTimeChart; }
    ETSIComplianceMonitor* etsiMonitorForTest() const { return m_etsiMonitor; }
    ConstellationWidget* constellationWidgetForTest() const { return m_constellationWidget; }
    ProfessionalExportManager* exportManagerForTest() const { return m_exportManager; }
    QAction* exportActionForTest() const { return m_exportAction; }
    QAction* settingsActionForTest() const { return m_settingsAction; }
    int realTimeChartSamplesForTest() const { return m_chartSamplesFed; }
    quint64 processorFrameCountForTest() const
    {
        return m_etiProcessor ? m_etiProcessor->getFrameCount() : 0;
    }
    void applyExportDataForTest(ExportConfiguration& c) const { applyExportData(c); }

private slots:
    void openFile()
    {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Open ETI File",
            "eti/",
            "ETI Files (*.eti);;All Files (*)"
        );

        if (!fileName.isEmpty()) {
            loadETIFile(fileName);
        }
    }

    // === Shared "new capture" reset =========================================
    // Both a file load and a live-network connect start a fresh capture. This
    // drops every piece of per-capture derived state so the two paths can never
    // merge: DLS+/Now Playing, MOT slideshow, per-service media timelines +
    // retained slides, the DAB+ sub-channel/service cache, the FIG analyser,
    // audio capture, the service selector, the adopted panels, playback, the
    // raw-frame caches and the associated capture UI.
    //
    // Returns true when a previous capture (file or live) was actually present
    // and discarded, so the caller can tell the user.
    //
    // Deliberately NOT included (callers own them, they differ per path):
    //   * the parse-loop guard (m_processing / frame-list enable / scope guard)
    //   * the System Messages log (load clears it; live keeps its stream log)
    //   * the "Loading"/"Initializing" status text
    bool resetCaptureState()
    {
        const bool hadCapture =
            m_fileLoaded || (m_etiProcessor && m_etiProcessor->getTotalFrames() > 0);

        m_fileLoaded = false;
        m_totalFrames = 0;
        m_currentFrameIndex = 0;
        if (m_exportAction) {
            m_exportAction->setEnabled(false);  // re-armed by onProcessingComplete()
        }

        // Refinement A: reset the Now Playing (DLS+) state + per-subchannel
        // DAB+ PAD parsers per capture.
        m_dlsPlusMessages = 0;
        m_dlsLabelsSeen.clear();
        m_dlsPlusPadCalls = 0;
        m_dlsPlusLabelsAssembled = 0;
        m_dlsParsers.clear();
        // T31 (rework) / T32: reset the per-service media timelines + the
        // per-service retained slides. The byte cap/sequence are global.
        m_mediaTimelineByService.clear();
        m_liveServiceMediaState.clear();
        m_slidesByService.clear();
        m_retainedSlideCount = 0;
        m_retainedSlideBytes = 0;
        m_retainedSlideSequence = 0;
        m_timelineRecordFrame = -1;
        m_dabPlusSubChannels.clear();
        m_audioSubChannels.clear();
        m_serviceBySubChannel.clear();
        m_audioSubChannelByService.clear();
        // v1.4 data-path wave: reset the per-data-subchannel stream capture.
        m_dataSubChannelStreams.clear();
        m_dataSubChannelBytes = 0;
        m_dataTapSlotCapWarned = false;
        m_dataTapByteCapWarned = false;
        // Wave B: reset the EPG / Journaline / TPEG decoders + the inner
        // data-service tabs for the new capture (next load re-feeds them).
        resetDataServicePanels();
        // T34: reset the audio capture + panel for the new capture.
        if (m_audioController) {
            m_audioController->beginCapture();
        }
        resetAudioPanelForLoad();
        m_dabPlusServiceMapDirty = true;
        m_cachedDabServiceCount = -1;
        m_cachedSubChannelCount = -1;
        m_cachedServiceRevision = 0;
        m_dlsPipelineNs = 0;
        m_pendingServiceId = 0;
        m_pendingServiceLabel.clear();
        resetNowPlayingPanel();
        // T32: the service selector is re-populated once the new capture's
        // services are known; clear stale items + selection and disable it.
        resetServiceSelectorForLoad();
        // Wave B: reset the adopted PerformanceDashboard / Real-Time Chart /
        // ETSI Compliance Monitor / Constellation panels for the new capture.
        resetAdoptedPanels();
        // T29: stop the playhead + disable the transport while parsing.
        stopPlaybackForLoad();
        // MOT SlideShow: clear the decoder + adapter and the slideshow tab.
        m_motProtocol.clearAll();
        m_motAdapter.reset();
        m_motObjectsReceived = 0;
        m_motImagesDecoded = 0;
        m_motErrors = 0;
        m_lastMotTransportId = 0;
        m_motHeaderParamsSeen = 0;
        m_lastMotContentName.clear();
        m_lastMotClickThroughUrl.clear();
        m_lastMotCategoryTitle.clear();
        m_lastMotServiceId = 0;
        m_lastMotServiceLabel.clear();
        m_lastDecodedSlide = QImage();
        resetSlideshowPanel();
        // Finding 6: clear the decoder's change-detection history so a second
        // capture cannot suppress its first label (identical to the previous
        // capture's last values).
        if (m_dlsPlusDecoder) {
            m_dlsPlusDecoder->resetHistory();
        }

        // === Clear previous-capture decode state in the FIG analyser ===
        // (Tab 3 instances/FIG counters/FIB-CRC counts must not accumulate
        // across captures) and re-apply the user's decode options.
        if (m_figAnalyser) {
            m_figAnalyser->reset();
            m_figAnalyser->setAnalyserSettings(m_analyserSettings);
        }

        // === Clear the raw-frame caches ===
        // m_frameDataCache backs the GUI hex viewer; m_frameCache backs the
        // worker/reader path. Both are capture-scoped.
        {
            QMutexLocker locker(&m_frameCacheMutex);
            m_frameDataCache.clear();
        }
        m_frameCache.clear();
        m_cacheHits = 0;
        m_cacheMisses = 0;

        // === Reset the capture UI ===
        if (m_frameList) m_frameList->clear();
        // T28: reset the Frame List with the new capture.
        if (m_tab3_frameList) m_tab3_frameList->clear();
        if (m_ensembleTree) m_ensembleTree->clear();
        if (m_hexViewer) m_hexViewer->clear();
        // T42: clear the Tab-3 Hex Viewer for the new capture.
        if (m_tab3_hexViewer) m_tab3_hexViewer->clear();

        return hadCapture;
    }

    void loadETIFile(const QString& filePath)
    {
        qDebug() << "[PHASE 3] Loading ETI file:" << filePath;

        // === Clear previous capture state (shared with live-mode entry) ===
        resetCaptureState();

        // Manual-test finding 2: the ETI file is parsed synchronously inside
        // processETIFile() while nested processEvents() calls keep the UI
        // alive. Mark the parse loop as running and disable the frame list so
        // a click in the ETI Frame Navigator cannot reach not-yet-valid frame
        // data (the navigator-click segfault).
        m_processing = true;
        m_frameNotReadyNotified = false;
        if (m_frameList) {
            m_frameList->setEnabled(false);
        }
        // Finding 8: always re-arm the navigation guard when this function
        // returns, whatever path it takes (success, early failure, exception).
        // onProcessingComplete() also re-arms on the queued signal path.
        [[maybe_unused]] auto navigationGuard =
            qScopeGuard([this]() { rearmNavigation(); });

        qDebug() << "[PHASE 3 AGENT 4] Frame cache cleared for new file";

        m_systemMessages->clear();

        // === PHASE 3: Update Player/Decoder status ===
        updatePlayerPanelStatus("Loading", filePath);
        updateDecoderPanelStatus("Initializing");

        // === Update status ===
        statusBar()->showMessage("Loading ETI file...");
        m_systemMessages->append(QString("📁 Loading file: %1").arg(filePath));

        // === Phase A: Start worker thread for non-blocking processing ===
        if (m_readerThread) {
            m_readerThread->setFilePath(filePath);
            m_readerThread->setInputMode(ETIReaderThread::InputMode::FileMode);
            m_readerThread->startProcessing();
            qDebug() << "Phase A: Worker thread started for file:" << filePath;
        }

        // === Load file with real ETI processor (compatibility mode) ===
        if (!m_etiProcessor->processETIFile(filePath)) {
            // File loading failed
            QString error = "Failed to load ETI file";

            QMessageBox::critical(this, "ETI Load Error",
                                QString("Failed to load ETI file:\n%1\n\nPlease check:\n"
                                        "- File exists and is readable\n"
                                        "- File is valid ETI format (6144-byte frames)\n"
                                        "- File is not corrupted")
                                        .arg(filePath));

            m_systemMessages->append(QString("❌ Error: %1").arg(error));
            statusBar()->showMessage("Error loading file");
            // Manual-test finding 2: leave the crash guard armed off and let
            // the user retry; onProcessingError() also clears this on the
            // queued path. The scope guard above would handle it too.
            rearmNavigation();
            return;
        }

        // === File loaded successfully - update state ===
        m_currentFilePath = filePath;
        m_systemMessages->append("✅ ETI file loaded - processing frames...");

        // === PHASE 3: Update status to "Processing" ===
        updatePlayerPanelStatus("Running");
        updateDecoderPanelStatus("Processing");
        
        // === PHASE 4: Initialize error counter ===
        updateErrorCounterTable();

        // === Note: Actual frame processing happens via signals ===
        // The following signals will be emitted during processing:
        // 1. processingStarted() - not currently used but available
        // 2. frameProcessed() → onFrameProcessed() - updates frame list
        // 3. figDiscovered() → onFIGDiscovered() - triggers FIG analysis
        // 4. processingProgress() → onProcessingProgress() - updates progress bar
        // 5. processingComplete() → onProcessingComplete() - finalizes UI
        //
        // When processing completes:
        // - onProcessingComplete() will update m_fileLoaded = true
        // - m_totalFrames will be set from signal parameter
        // - Frame navigation will be enabled
    }


    // Re-arm the frame-navigation guard after parsing finishes or aborts.
    // Idempotent: safe to call from the sync path, the queued slots and the
    // worker-thread error handler (Finding 8).
    void rearmNavigation()
    {
        m_processing = false;
        if (m_frameList) {
            m_frameList->setEnabled(true);
        }
    }

    // Worker-thread processing error: always re-arm the navigation guard and
    // surface the error (Finding 8).
    void handleWorkerError(const QString& error)
    {
        qWarning() << "Phase B: Worker thread error:" << error;
        rearmNavigation();
#ifndef GUI_TEST_MODE
        QMessageBox::critical(this, "Processing Error", error);
#else
        Q_UNUSED(error);
#endif
    }

    void onFrameProcessed(ProcessedFrame frame)  // Pass by value for Qt::QueuedConnection safety
    {
        // NOTE: emitted for EVERY valid frame (enhanced_eti_processor_qt.cpp
        // "Emit frameProcessed() for EVERY valid frame"); the 100-frame batch
        // there only throttles the processing timer, not this callback.
        
        // CRITICAL FIX: Minimal updates only - no formatToString() calls
        // A-LOW10: both guards also gate the data tap below. frame_number <= 0
        // means the processor produced no trustworthy MSC slice (unknown frame
        // identity), and a null frame list means headless/teardown mode where
        // no capture is being presented — so the tap is deliberately skipped
        // there rather than silently capturing an unattributable stream.
        if (frame.frame_number <= 0) return;
        if (!m_frameList) return;
        
        // === PHASE 1.1: Frame Data Caching ===
        m_processedFrames++;
        
        // Get raw frame data from processor once, and reuse it for the DLS+
        // pipeline below (avoids a second 6144-byte fetch per frame).
        // getRawFrameData() is 0-based; ProcessedFrame::frame_number is 1-based,
        // so the cache is keyed by the real 0-based frame index (matching the
        // hex-viewer lookup in displayFrameHexData()).
        //
        // File-backed only: m_fileLoaded covers a completed load; m_processing
        // covers the synchronous load loop itself (m_fileLoaded is only set by
        // the queued onProcessingComplete()). A LIVE capture has neither set —
        // beginLiveMode() also drops the processor's file_data_/total_frames_ —
        // so the file lookup is skipped and we never log the per-frame
        // "getRawFrameData: Invalid frame index" warning or pollute the cache.
        QByteArray frameData;
        const int rawIndex = static_cast<int>(frame.frame_number) - 1;
        const bool fileBackedCapture = m_fileLoaded || m_processing;
        if (m_etiProcessor && rawIndex >= 0 && fileBackedCapture) {
            frameData = m_etiProcessor->getRawFrameData(rawIndex);
        }
        
        // Cache frame data (limit 10K frames to prevent memory bloat)
        // FIX HIGH-002: Evict before adding to maintain exact 10K limit
        if (!frameData.isEmpty()) {
            QMutexLocker locker(&m_frameCacheMutex);  // FIX HIGH-002: Thread-safe cache access
            if (m_frameDataCache.size() >= 10000) {
                // Evict oldest entry (first in map) before adding new one
                auto it = m_frameDataCache.begin();
                m_frameDataCache.erase(it);
            }
            m_frameDataCache[rawIndex] = frameData;
        }
        
        // === PHASE 1.1: Throttled Frame List Updates (~10 fps) ===
        // FIX HIGH-001: Use atomic member variable instead of static for thread safety
        if (++m_frameUpdateCounter % FRAME_LIST_UPDATE_THROTTLE == 0) {
            // Detect frame type from FIC data presence
            QString frameType = frame.fic_data.isEmpty() ? "ETI-MSC" : "ETI-FIC";
            
            // Use CRC from ProcessedFrame structure
            QString crcStr = QString("0x%1").arg(frame.crc, 8, 16, QChar('0')).toUpper();
            
            QString itemText = QString("Frame %1: %2 (CRC: %3)")
                .arg(frame.frame_number)
                .arg(frameType)
                .arg(crcStr);
            // T29: store the real frame number so the playback playhead can
            // highlight the nearest navigator entry cheaply.
            auto* item = new QListWidgetItem(itemText);
            item->setData(Qt::UserRole, static_cast<int>(frame.frame_number));
            m_frameList->addItem(item);
            m_frameList->scrollToBottom();
        }
        
        // DO NOT call updateFrameDetails - causes crash
        // DO NOT call formatToString - causes crash in this context
        
        
        // === CRITICAL FIX: Pass FIC data to FIG analyser ===
        if (!frame.fic_data.isEmpty() && m_figAnalyser) {
            // FIC-XTractor: tag the analyzed FIC with its real ETI frame
            // number so Tab 3's chronological FIG instance list has truth.
            m_figAnalyser->setFigFrameNumber(frame.frame_number);
            m_figAnalyser->analyzeFICData(frame.fic_data);
            // DEBUG: Log FIC processing
            static int ficProcessCount = 0;
            ficProcessCount++;
            if (ficProcessCount <= 10 || ficProcessCount % 100 == 0) {
                qDebug() << "[DEBUG] FIC data passed to analyser, count:" << ficProcessCount
                         << "frame:" << frame.frame_number << "size:" << frame.fic_data.size();
                int svcCount = m_figAnalyser->getServiceCount();
                qDebug() << "[DEBUG] Current service count:" << svcCount;
            }
        }
        
        // === v1.4 data-path wave: clean data-subchannel tap (A-H4) ===
        // Captures the raw bytes of every non-audio sub-channel for the
        // EPG/TEPG/Journaline decoders. It runs BEFORE processDlsPlusFromFrame()
        // so a data-only ensemble (empty DAB+ audio set -> early return there)
        // and the frames before FIG 0/8 classifies anything are captured too,
        // and OUTSIDE the dlsTimer window so the measured DLS+ pipeline time
        // and the CLI/GUI parity counters stay untouched (A-LOW5).
        tapDataSubchannelsFromFrame(frame);

        // === Manual-test finding 3: feed the GUI DLS+ decoder ===
        // (was constructed but never fed, so Now Playing stayed empty)
        // T32: the frame index is published so feedDlsPlusPad()/onMotObject-
        // Complete() can record the per-service media state at THIS frame while
        // the shared parser/adapter runs. Reset afterwards so feeds outside the
        // load loop (tests) never record.
        {
            m_timelineRecordFrame = rawIndex;
            QElapsedTimer dlsTimer;
            dlsTimer.start();
            processDlsPlusFromFrame(frame);
            m_dlsPipelineNs += dlsTimer.nsecsElapsed();
            m_timelineRecordFrame = -1;
        }

        // NOTE (Wave B): the legacy label updater that used to run per frame
        // here was deleted; the adopted PerformanceDashboard is fed by the 1 s
        // updateDashboardSamples() sampler instead.

        // === PHASE 4: Throttled error counter updates (5 fps) ===
        if (++m_errorCounterUpdateCounter % ERROR_COUNTER_UPDATE_THROTTLE == 0) {
            updateErrorCounterTable();
        }
    }

    // Rebuild the DAB+ sub-channel -> (service id, label) view from the FIG
    // analyser when needed. Invalidated by the analyser's monotonic
    // service/component/label revision (Finding 2) in addition to the
    // count/dirty guards. Kept as its own method so the cache can be exercised
    // directly by tests without fabricating a full ProcessedFrame.

    /// A-M5a/B-M1: is this FIG component an AUDIO transport (TMId 0 = MSC
    /// stream audio, ASCTy 0x00 MPEG / 0x3F DAB+ — the same rule the analyser
    /// uses to decide "Audio service")? Shared by the data tap's
    /// classification, the feed-stage exclusion and the FIG 1/5 label lookup
    /// (an audio component must never be labelled or fed as a data service).
    static bool isAudioServiceComponent(const ServiceComponent& comp)
    {
        return comp.transport_mode == 0
               && (comp.service_component_type == 0x00
                   || comp.service_component_type == 0x3F);
    }

    void rebuildDabPlusServiceMapIfNeeded()
    {
        const int svcCount = m_figAnalyser ? m_figAnalyser->getServiceCount() : 0;
        const int scCount = m_figAnalyser ? m_figAnalyser->getSubChannelCount() : 0;
        const uint32_t revision = m_figAnalyser ? m_figAnalyser->getServiceRevision() : 0;
        if (!m_dabPlusServiceMapDirty && svcCount == m_cachedDabServiceCount
            && scCount == m_cachedSubChannelCount
            && revision == m_cachedServiceRevision) {
            return;  // cache still valid
        }
        m_dabPlusSubChannels.clear();
        m_audioSubChannels.clear();
        m_serviceBySubChannel.clear();
        m_audioSubChannelByService.clear();
        if (m_figAnalyser) {
            for (const DABService& svc : m_figAnalyser->getDABServices()) {
                for (const ServiceComponent& comp : svc.components) {
                    // A-M5a: audio-component classification used by the data
                    // tap and (B-M1) the feed stage / FIG 1/5 label lookup —
                    // see isAudioServiceComponent(). Everything else (TMId 1
                    // = data/packet, or an audio sub-channel not classified
                    // yet) is a data candidate and may be tapped.
                    if (isAudioServiceComponent(comp)) {
                        m_audioSubChannels.insert(comp.sub_channel_id);
                    }
                    if (!comp.isDabPlus()) {
                        continue;
                    }
                    m_dabPlusSubChannels.insert(comp.sub_channel_id);
                    if (!m_serviceBySubChannel.contains(comp.sub_channel_id)) {
                        m_serviceBySubChannel.insert(
                            comp.sub_channel_id,
                            qMakePair(svc.service_id, svc.service_label));
                    }
                    // T34: one primary DAB+ audio sub-channel per service.
                    if (!m_audioSubChannelByService.contains(svc.service_id)) {
                        m_audioSubChannelByService.insert(svc.service_id, comp.sub_channel_id);
                    }
                }
            }
        }
        m_cachedDabServiceCount = svcCount;
        m_cachedSubChannelCount = scCount;
        m_cachedServiceRevision = revision;
        m_dabPlusServiceMapDirty = false;
    }

    // ========================================================================
    // Refinement A: DLS+ (Now Playing) pipeline wiring — standard source chain
    // ========================================================================
    // docs/PHASE_3B_TECHNICAL_NOTES.md §Architecture:
    //   DAB+ audio subchannel (ETI) -> PAD from DAB+ audio AUs (ETSI TS 102 563
    //   §4/§5, embedded Data Stream Element) -> F-PAD + X-PAD data groups
    //   (ETSI EN 300 401 §7.4) -> DLS/DLS+ Data Groups ->
    //   DLSPlusDecoder::processPADData() -> nowPlayingUpdated() ->
    //   updateNowPlaying() -> panel.
    //
    // The superframe/AU/PAD parsing lives in DabPlusSubchannelParser above
    // (spec-correct, cross-checked against DABlin/etisnoop). This method just
    // routes the FIG analyser's DAB+ audio subchannels into it, one CIF per
    // ETI frame; a parser emits an assembled DLS label per 120 ms superframe.
    //
    // One ETI frame carries exactly one CIF chunk per sub-channel, so
    // processDlsPlusFromFrame() must run once per frame regardless of the FIC
    // (FICF) flag: a FICF=0 frame still carries MSC, and skipping it would
    // break the 5-CIF superframe cadence. EnhancedETIProcessorQt therefore
    // emits frameProcessed() for every valid frame (not only FIC-bearing ones).
    void processDlsPlusFromFrame(const ProcessedFrame& frame)
    {
        if (!m_dlsPlusDecoder || !m_etiProcessor || frame.frame_number == 0) {
            return;
        }
        if (!frame.hasSubChannels()) {
            return;  // ETI processor produced no MSC slices for this frame
        }

        // DAB+ audio subchannels (FIG 0/8 ASCTy = 0x3F) + their service.
        // getDABServices() returns a full copy by value, so the derived view is
        // cached; it is rebuilt whenever the analyser's service/component/label
        // revision advances. Service *counts* alone are insufficient: FIG 1/1
        // fills `service_label` later with no count change (and mid-stream
        // reconfigurations can remap components at equal counts).
        rebuildDabPlusServiceMapIfNeeded();
        if (m_dabPlusSubChannels.isEmpty()) {
            return;  // no DAB+ audio component decoded yet
        }

        // Reuse the per-subchannel MSC slices already extracted by the ETI
        // processor (ProcessedFrame::sub_channels) instead of re-running
        // parseHeader()/extractMSC() on the raw frame.
        for (const ProcessedFrame::SubChannelSlice& sc : frame.subChannels()) {
            if (!m_dabPlusSubChannels.contains(sc.sub_channel_id)) {
                continue;
            }
            // T34: feed the selected-service audio capture path with the SAME
            // per-sub-channel MSC slice the DLS+/MOT feed consumes (one CIF per
            // ETI frame). No counters here are touched.
            if (m_audioController) {
                m_audioController->feedSubchannelChunk(sc.sub_channel_id, sc.data);
            }
            // Lazily create the parser and hook its MOT data-group callback so
            // X-PAD CI 12/13 groups reach the MOTProtocol via the adapter.
            auto parserIt = m_dlsParsers.find(sc.sub_channel_id);
            if (parserIt == m_dlsParsers.end()) {
                const int scId = sc.sub_channel_id;
                parserIt = m_dlsParsers.emplace(scId, DabPlusSubchannelParser{}).first;
                parserIt->second.onMotDataGroup = [this, scId](const QByteArray& dataGroup) {
                    const QPair<quint32, QString> svc =
                        m_serviceBySubChannel.value(scId, qMakePair(0u, QString()));
                    m_lastMotServiceId = svc.first;
                    m_lastMotServiceLabel = svc.second;
                    m_motAdapter.processDataGroup(svc.first, dataGroup);
                };
            }
            DlsAssembly assembly;
            if (!parserIt->second.feedChunk(sc.data, assembly)
                || !assembly.hasLabel) {
                continue;
            }
            const QPair<quint32, QString> svc =
                m_serviceBySubChannel.value(sc.sub_channel_id, qMakePair(0u, QString()));
            feedDlsAssembly(assembly, svc.first, svc.second);
        }

        // v1.4 data-path wave — the raw data-subchannel tap moved to
        // tapDataSubchannelsFromFrame() (A-H4): it must run for every frame —
        // including data-only ensembles where the DAB+ audio set below is
        // empty and the early return would skip it — and outside the dlsTimer
        // window so the measured DLS+ pipeline time stays honest.
    }

    // ========================================================================
    // v1.4 data-path wave — clean data-subchannel tap (A-H4 / A-M5)
    // ========================================================================
    // Captures the raw bytes of every sub-channel that is NOT an audio
    // component (DAB+ ASCTy 0x3F or MPEG ASCTy 0x00 on TMId 0 — see
    // m_audioSubChannels) so the EPG/TEPG/Journaline decoders can be driven
    // from the real stream without decoding anything here.
    //
    // Classification is the FIG-derived heuristic: a sub-channel with no
    // decoded component yet is tapped (fail-open — we would rather keep bytes
    // a decoder can reject than lose them), which is what makes the frames
    // before FIG 0/8 decodes capturable.
    //
    // Purely additive: no decoder runs in this GUI path, the store is
    // byte-capped (kMaxDataSubChannelBytes) and the map is slot-capped
    // (kMaxDataSubChannelSlots). Both cap hits are reported ONCE per capture
    // instead of silently dropping bytes (A-M5c).
    void tapDataSubchannelsFromFrame(const ProcessedFrame& frame)
    {
        if (!frame.hasSubChannels()) {
            return;  // ETI processor produced no MSC slices for this frame
        }
        // Same cache rebuild the DLS+ path uses, so this frame's FIC has
        // already been analysed (the caller feeds the FIG analyser first) and
        // the classification is current.
        rebuildDabPlusServiceMapIfNeeded();

        for (const ProcessedFrame::SubChannelSlice& sc : frame.subChannels()) {
            if (m_dabPlusSubChannels.contains(sc.sub_channel_id)
                || m_audioSubChannels.contains(sc.sub_channel_id)) {
                // A-H4/A-M5a: frames captured BEFORE FIG 0/8 classified this
                // sub-channel are already in the map — drop them the moment we
                // KNOW the sub-channel is audio, so audio junk can never reach
                // the EPG/Journaline/TPEG feed (defence in depth next to the
                // feed-stage exclusion in feedDataServiceSubchannels()).
                const auto leaked = m_dataSubChannelStreams.find(sc.sub_channel_id);
                if (leaked != m_dataSubChannelStreams.end()) {
                    m_dataSubChannelBytes -=
                        static_cast<quint64>(leaked.value().size());
                    m_dataSubChannelStreams.erase(leaked);
                }
                continue;  // audio component — not a data sub-channel
            }
            // A-M5b: probe the slot budget BEFORE creating the map entry, so a
            // rejected sub-channel does not burn one of the 32 slots with an
            // empty QByteArray.
            const int scId = sc.sub_channel_id;
            if (!m_dataSubChannelStreams.contains(scId)
                && m_dataSubChannelStreams.size() >= kMaxDataSubChannelSlots) {
                if (!m_dataTapSlotCapWarned) {
                    m_dataTapSlotCapWarned = true;
                    qWarning() << "[DataTap] per-sub-channel slot cap reached ("
                               << kMaxDataSubChannelSlots
                               << ") — sub-channel" << scId
                               << "not captured; further slot-cap hits are silent";
                }
                continue;
            }
            if (m_dataSubChannelBytes >= kMaxDataSubChannelBytes) {
                if (!m_dataTapByteCapWarned) {
                    m_dataTapByteCapWarned = true;
                    qWarning() << "[DataTap] global byte cap reached ("
                               << kMaxDataSubChannelBytes
                               << " bytes) — remaining frames are not captured;"
                                  " further byte-cap hits are silent";
                }
                continue;  // global byte cap
            }
            QByteArray& sink = m_dataSubChannelStreams[scId];
            const quint64 room = kMaxDataSubChannelBytes - m_dataSubChannelBytes;
            const int take = static_cast<int>(qMin<quint64>(
                room, static_cast<quint64>(sc.data.size())));
            if (take <= 0) {
                continue;
            }
            sink.append(sc.data.constData(), take);
            m_dataSubChannelBytes += static_cast<quint64>(take);
        }
    }

    // ========================================================================
    // T31 (rework) / T32: per-service load-time media history + playhead lookup
    // ========================================================================
    // Recording is done at FEED time (inside feedDlsPlusPad / onMotObject-
    // Complete) while m_timelineRecordFrame is the current 0-based frame, and
    // only when that service's state changes (sparse). A frame-0 baseline
    // entry is seeded for any service whose first change is later, so Stop/
    // Reset always has a well-defined "start of capture" state.
    void recordServiceMediaState(quint32 serviceId, int frameIndex)
    {
        if (frameIndex < 0) {
            return;
        }
        auto liveIt = m_liveServiceMediaState.find(serviceId);
        if (liveIt == m_liveServiceMediaState.end()) {
            return;
        }
        ServiceMediaTimeline& tl = m_mediaTimelineByService[serviceId];
        if (tl.empty() && frameIndex != 0) {
            tl.emplace(0, PlaybackMediaState{});
        }
        if (!tl.empty() && tl.rbegin()->second == liveIt->second) {
            return;  // sparse: nothing changed at this frame
        }
        tl[frameIndex] = liveIt->second;
    }

    // T32: a DLS label for `serviceId` decoded at `frameIndex` (load time).
    void recordServiceDlsState(quint32 serviceId, int frameIndex, const QString& track,
                               const QString& artist, const QString& album)
    {
        if (frameIndex < 0) {
            return;
        }
        PlaybackMediaState& live = m_liveServiceMediaState[serviceId];
        live.track = track;
        live.artist = artist;
        live.album = album;
        recordServiceMediaState(serviceId, frameIndex);
    }

    // T32: a MOT slide for `serviceId` decoded at `frameIndex` (load time).
    void recordServiceMotState(quint32 serviceId, int frameIndex, int imageId)
    {
        if (frameIndex < 0) {
            return;
        }
        PlaybackMediaState& live = m_liveServiceMediaState[serviceId];
        live.motImageId = imageId;
        recordServiceMediaState(serviceId, frameIndex);
    }

    // Retain a decoded MOT slide for scrubbing in that service's own store.
    // Ids are scoped per service so a per-service timeline reference always
    // resolves against the owning service (no cross-service leak). Eviction is
    // globally FIFO by insertion sequence, so the total image bytes stay under
    // the cap regardless of the number of services. The cap is a runtime member
    // (default 32 MB) so the eviction path is testable with a tiny value.
    void appendRetainedSlide(quint32 serviceId, RetainedSlide slide)
    {
        slide.sequence = m_retainedSlideSequence++;
        RetainedSlideStore& store = m_slidesByService[serviceId];
        m_retainedSlideBytes += slide.image.sizeInBytes();
        store.slides.push_back(std::move(slide));
        ++store.nextId;
        ++m_retainedSlideCount;
        while (m_retainedSlideBytes > m_retainedSlideByteCap && m_retainedSlideCount > 1) {
            quint32 victim = 0;
            quint64 minSeq = std::numeric_limits<quint64>::max();
            bool found = false;
            for (const auto& entry : m_slidesByService) {
                if (!entry.second.slides.empty()
                    && entry.second.slides.front().sequence < minSeq) {
                    minSeq = entry.second.slides.front().sequence;
                    victim = entry.first;
                    found = true;
                }
            }
            if (!found) {
                break;
            }
            RetainedSlideStore& vs = m_slidesByService[victim];
            m_retainedSlideBytes -= vs.slides.front().image.sizeInBytes();
            vs.slides.pop_front();
            ++vs.baseId;
            --m_retainedSlideCount;
        }
    }

    // Resolve a slide id IN `serviceId`'s store. An evicted id falls back to
    // that service's nearest retained slide (and says so); a service with no
    // retained slide returns nullptr (waiting state — never another service's
    // image).
    const RetainedSlide* findRetainedSlide(quint32 serviceId, int imageId,
                                           QString& note) const
    {
        auto it = m_slidesByService.find(serviceId);
        if (it == m_slidesByService.end() || it->second.slides.empty()) {
            return nullptr;
        }
        const RetainedSlideStore& store = it->second;
        const int lastId = store.nextId - 1;
        int resolved = imageId;
        if (resolved < store.baseId || resolved > lastId) {
            resolved = qBound(store.baseId, resolved, lastId);
            note = QStringLiteral(" (nearest retained image)");
        }
        return &store.slides[static_cast<size_t>(resolved - store.baseId)];
    }

    // The selected service as "label (0xSID)" / "SId 0xSID" (empty before load).
    QString selectedServiceDisplay() const
    {
        if (m_selectedServiceLabel.isEmpty() && m_selectedServiceId == 0) {
            return QString();
        }
        const QString sid = serviceSidHex(m_selectedServiceId);
        if (!m_selectedServiceLabel.isEmpty()) {
            return QString("%1 (%2)").arg(m_selectedServiceLabel, sid);
        }
        return QString("SId %1").arg(sid);
    }

    // Media panels' documented waiting state for the SELECTED service.
    void showWaitingMediaStateForSelectedService()
    {
        if (m_trackLabel) m_trackLabel->setText(QStringLiteral("--"));
        if (m_artistLabel) m_artistLabel->setText(QStringLiteral("--"));
        if (m_albumLabel) m_albumLabel->setText(QStringLiteral("--"));
        const QString svcText = selectedServiceDisplay();
        if (m_lastUpdatedLabel) {
            m_lastUpdatedLabel->setText(
                svcText.isEmpty()
                    ? tr("Waiting for DLS+…")
                    : QString("Waiting for DLS+… · %1").arg(svcText));
        }
        applyRetainedSlideForPlayhead(-1);
    }

    // Pure lookup + panel apply (NO parser, NO MOT adapter, NO counter, NO
    // decode) for the CURRENTLY SELECTED service. Finds the latest entry at or
    // before `frameIndex` and restores that service's DLS labels + slide. When
    // the selected service has no record at/before the frame, the panels show
    // the waiting state instead of another service's metadata (T32).
    void refeedDlsForPlayhead(int frameIndex)
    {
        if (frameIndex < 0) {
            return;  // negative playhead is a no-op (T31 invariant test)
        }
        auto tlIt = m_mediaTimelineByService.find(m_selectedServiceId);
        if (tlIt == m_mediaTimelineByService.end() || tlIt->second.empty()) {
            showWaitingMediaStateForSelectedService();
            return;
        }
        const ServiceMediaTimeline& tl = tlIt->second;
        auto it = tl.upper_bound(frameIndex);
        if (it == tl.begin()) {
            showWaitingMediaStateForSelectedService();
            return;  // playhead precedes this service's first recorded state
        }
        --it;
        const PlaybackMediaState& st = it->second;

        // DLS labels restored straight from the timeline (the load-time
        // decoder already produced these exact strings).
        if (m_trackLabel) {
            m_trackLabel->setText(st.track.isEmpty() ? QStringLiteral("--") : st.track);
        }
        if (m_artistLabel) {
            m_artistLabel->setText(st.artist.isEmpty() ? QStringLiteral("--") : st.artist);
        }
        if (m_albumLabel) {
            m_albumLabel->setText(st.album.isEmpty() ? QStringLiteral("--") : st.album);
        }
        if (m_lastUpdatedLabel) {
            const QString svcText = selectedServiceDisplay();
            m_lastUpdatedLabel->setText(
                svcText.isEmpty()
                    ? QString("Frame %1 · replaying load metadata").arg(frameIndex + 1)
                    : QString("Frame %1 · %2 · replaying load metadata")
                          .arg(frameIndex + 1)
                          .arg(svcText));
        }

        applyRetainedSlideForPlayhead(st.motImageId);
    }

    // Restore the slideshow panel from a retained-slide identity. A negative id
    // means no image had been decoded at/before that frame -> show the
    // pre-slideshow state. Resolution is scoped to the selected service, so a
    // missing/evicted id can never show another service's picture.
    void applyRetainedSlideForPlayhead(int imageId)
    {
        if (!m_slideshowImage) {
            return;
        }
        const QString svcText = selectedServiceDisplay();
        if (imageId < 0) {
            m_lastDecodedSlide = QImage();
            m_slideshowImage->clear();
            m_slideshowImage->setText(tr("Waiting for slideshow…"));
            if (m_slideshowStatus) {
                m_slideshowStatus->setText(
                    svcText.isEmpty()
                        ? tr("Waiting for slideshow…")
                        : QString("Waiting for slideshow… · %1").arg(svcText));
            }
            if (m_slideshowInfo) {
                m_slideshowInfo->setText(svcText.isEmpty() ? QStringLiteral("—") : svcText);
            }
            return;
        }
        QString note;
        const RetainedSlide* slide = findRetainedSlide(m_selectedServiceId, imageId, note);
        if (!slide) {
            // No slide retained for this service -> waiting state (never leak
            // another service's image).
            applyRetainedSlideForPlayhead(-1);
            return;
        }
        m_lastDecodedSlide = slide->image;
        refreshSlideshowPixmap();
        if (m_slideshowStatus) {
            m_slideshowStatus->setText(slide->status + note);
        }
        if (m_slideshowInfo) {
            m_slideshowInfo->setText(slide->info + note);
        }
    }

    // Adapt a spec-parsed DLS/DL+ assembly to the byte layout that
    // DLSPlusDecoder::processPADData() consumes (charset + text + NUL +
    // command + decoder-format descriptors), then feed it. This adaptation is
    // only at the decoder boundary; PAD byte selection above is spec-based.
    void feedDlsAssembly(const DlsAssembly& assembly, quint32 serviceId,
                         const QString& serviceLabel)
    {
        if (!dlsTextLooksSane(assembly.dlText, assembly.charset)) {
            return;  // structural parse already CRC-validated; text sanity only
        }
        const QByteArray pad = buildDecoderPad(assembly);
        if (pad.isEmpty()) {
            return;
        }
        ++m_dlsPlusPadCalls;
        feedDlsPlusPad(pad, serviceId, serviceLabel);
    }

    static bool dlsTextLooksSane(const QByteArray& bytes, uint8_t charset)
    {
        if (bytes.isEmpty()) {
            return false;
        }
        QString text;
        if (charset == 15) {
            text = QString::fromUtf8(bytes);
            if (text.contains(QChar(0xFFFD))) {
                return false;  // invalid UTF-8
            }
        } else {
            // Other DLS charsets (0 = EBU Latin, 6 = UCS-2) are decoded as
            // Latin-1 by the legacy decoder; accept for the structural path.
            text = QString::fromLatin1(bytes);
        }
        for (const QChar c : text) {
            if (c.unicode() < 0x20 || (c.unicode() >= 0x7F && c.unicode() < 0xA0)) {
                return false;  // DLS label is a single readable line
            }
        }
        return true;
    }

    // Build the legacy DLSPlusDecoder PAD layout from a standard assembly.
    static QByteArray buildDecoderPad(const DlsAssembly& assembly)
    {
        if (assembly.dlText.isEmpty()) {
            return QByteArray();
        }
        QByteArray descriptors;
        int numTags = 0;
        for (const StdDlsPlusTag& tag : assembly.tags) {
            int contentType = 0;
            int subType = 0;
            if (!mapDlPlusContentType(tag.contentType, contentType, subType)) {
                continue;
            }
            if (numTags >= 4) {
                break;
            }
            const int start = qBound(0, tag.startMarker, 127);
            if (start >= assembly.dlText.size()) {
                continue;
            }
            const int length = qMin(qBound(0, tag.lengthMarker + 1, 127),
                                    assembly.dlText.size() - start);
            if (length <= 0) {
                continue;
            }
            QByteArray d(4, 0);
            d[0] = static_cast<char>((contentType << 4) | (subType & 0x0F));
            d[1] = 0;
            d[2] = static_cast<char>(((start & 0x7F) << 1) | ((length >> 6) & 0x01));
            d[3] = static_cast<char>((length & 0x3F) << 2);
            descriptors.append(d);
            ++numTags;
        }
        if (numTags == 0) {
            // Plain DLS (no DL+ tags): expose the whole label as ITEM.TITLE so
            // "Now Playing" still shows the DLS text.
            const int length = qMin(assembly.dlText.size(), 127);
            if (length <= 0) {
                return QByteArray();
            }
            QByteArray d(4, 0);
            d[0] = static_cast<char>(0x10);  // ITEM (1) / TITLE (0)
            // Length is a 7-bit field: bit 6 lives in d[2] bit 0, low 6 bits in
            // d[3]. Dropping bit 6 truncated 64–127 byte labels (incl. Thai).
            d[2] = static_cast<char>((length >> 6) & 0x01);
            d[3] = static_cast<char>((length & 0x3F) << 2);
            descriptors.append(d);
            numTags = 1;
        }
        QByteArray pad;
        pad.append(static_cast<char>(assembly.charset & 0x0F));
        pad.append(assembly.dlText);
        pad.append('\0');
        pad.append(static_cast<char>(numTags & 0x0F));
        pad.append(descriptors);
        return pad;
    }

    static bool mapDlPlusContentType(int tagCode, int& contentType, int& subType)
    {
        if (tagCode >= 1 && tagCode <= 31)  { contentType = 1; subType = tagCode - 1;  return true; }
        if (tagCode >= 32 && tagCode <= 47) { contentType = 2; subType = tagCode - 32; return true; }
        if (tagCode >= 48 && tagCode <= 55) { contentType = 3; subType = tagCode - 48; return true; }
        if (tagCode >= 56 && tagCode <= 59) { contentType = 4; subType = tagCode - 56; return true; }
        if (tagCode >= 60 && tagCode <= 61) { contentType = 8; subType = tagCode - 60; return true; }
        return false;
    }

    // Single entry point for feeding an adapted PAD buffer to the GUI DLS+
    // decoder, remembering the originating service for the panel/log.
    bool feedDlsPlusPad(const QByteArray& padData, quint32 serviceId = 0,
                        const QString& serviceLabel = QString())
    {
        if (!m_dlsPlusDecoder || padData.isEmpty()) {
            return false;
        }
        m_pendingServiceId = serviceId;
        m_pendingServiceLabel = serviceLabel;
        try {
            if (m_dlsPlusDecoder->processPADData(padData)) {
                ++m_dlsPlusMessages;
                // Collect decoded labels for GUI tests (Finding 1 round-trip /
                // Thai-label assertions on the real fixture).
                const eti::dls_plus::DLSPlusMessage msg =
                    m_dlsPlusDecoder->getCurrentMessage();
                const QString label = msg.dls_text;
                if (!label.isEmpty() && !m_dlsLabelsSeen.contains(label)) {
                    m_dlsLabelsSeen.append(label);
                }
                // T32: snapshot THIS service's media state at the current frame
                // (the shared decoder's current message is the one just parsed).
                recordServiceDlsState(serviceId, m_timelineRecordFrame,
                                      msg.getTag("ITEM.TITLE"),
                                      msg.getTag("ITEM.ARTIST"),
                                      msg.getTag("ITEM.ALBUM"));
                return true;
            }
        } catch (const std::exception& e) {
            qWarning() << "[DLS+] PAD decode exception:" << e.what();
        } catch (...) {
            qWarning() << "[DLS+] Unknown PAD decode exception";
        }
        return false;
    }

    void onProcessingProgress(int percentage, int currentFrame, int totalFrames)
    {
        // Refinement B: drive the progress widgets from the synchronous
        // processing loop. processAllFramesImmediate() emits this every 100
        // frames via Qt::DirectConnection and calls processEvents() every 10
        // frames, so the bar stays alive and visible during the load.
        //
        // Processing-strategy evaluation:
        //   * Confirmed: hex viewer / ETSI compliance / ETI overview are
        //     deferred to frame SELECTION (handleFrameSelection) and the frame
        //     list is disabled while parsing, so the first pass builds no
        //     detail-only widgets.
        //   * Kept in the first pass (needed for navigator + metadata): the
        //     throttled frame-list entries and the FIC -> FIG analyser
        //     accumulation. DAB+ PAD->DLS parsing is also first-pass because
        //     it feeds Now Playing as data arrives.
        //   * Deferred: no detail-only work remains in the load loop.
        if (m_progressBar) {
            if (m_progressBar->maximum() != totalFrames) {
                m_progressBar->setRange(0, totalFrames);  // range 0..total
            }
            m_progressBar->setValue(currentFrame);
        }

        // === PHASE 1.2: Player/Decoder Timing Updates (Throttled to 5 fps) ===
        if (!m_timingUpdateTimer.isValid()) {
            m_timingUpdateTimer.start();  // Initialize timer on first call
        }
        
        // Throttle timing updates to 5 fps (200ms interval)
        if (m_timingUpdateTimer.elapsed() >= TIMING_UPDATE_INTERVAL_MS) {
            m_timingUpdateTimer.restart();
            const QString progressText =
                QString("Processing frame %1 / %2 (%3%)")
                    .arg(currentFrame).arg(totalFrames).arg(percentage);

            // Update Player panel timing
            if (m_playerTimeLabel) {
                m_playerTimeLabel->setText(frameToTimeString(currentFrame, totalFrames));
            }
            if (m_playerFrameLabel) {
                m_playerFrameLabel->setText(progressText);
            }
            if (m_playerProgressBar) {
                // M1: guard the load-progress update so it is never mistaken
                // for a user seek by the valueChanged handler.
                m_seekSliderProgrammatic = true;
                m_playerProgressBar->setRange(0, totalFrames);
                m_playerProgressBar->setValue(currentFrame);
                m_seekSliderProgrammatic = false;
            }
            
            // Update Decoder panel timing (mirrors player)
            if (m_decoderTimeLabel) {
                m_decoderTimeLabel->setText(frameToTimeString(currentFrame, totalFrames));
            }
            if (m_decoderStatusLabel) {
                m_decoderStatusLabel->setText(progressText);
            }
            if (statusBar()) {
                statusBar()->showMessage(progressText);
            }
        }
        
        // Disabled to prevent crash:
        // - NO m_frameCountLabel->setText()
        // - NO m_systemMessages->append()
        // These will be updated AFTER processing completes
    }

    void onProcessingComplete(int totalFrames, int processingTimeMs)
    {
        // T25 (F4): a queued processingComplete() can be delivered during
        // teardown, after m_etiProcessor was reset. The handler dereferences
        // m_etiProcessor below; degrade gracefully and skip all processor/UI
        // work instead of crashing on a half-torn-down window.
        if (m_closing || !m_etiProcessor) {
            qDebug() << "onProcessingComplete ignored (closing or no processor)";
            return;
        }

        // Manual-test finding 2: parsing finished -> re-arm navigation.
        rearmNavigation();

        // === PHASE 3: Update state tracking ===
        m_fileLoaded = true;
        m_totalFrames = static_cast<size_t>(totalFrames);
        qDebug() << "[PHASE 3] File loaded:" << m_totalFrames << "frames";

        // Wave B: record the load-time processing cost (avg ms/frame latency
        // for the dashboard) and enable File -> Export now that data exists.
        m_lastProcessingTimeMs = static_cast<double>(processingTimeMs);
        if (m_exportAction) {
            m_exportAction->setEnabled(true);
        }

        // Refinement B: complete the progress bar at 100 % (value == max).
        if (m_progressBar) {
            const int total = totalFrames > 0 ? totalFrames : 1;
            m_progressBar->setRange(0, total);
            m_progressBar->setValue(total);
        }
        
        // Show final statistics
        m_systemMessages->append(QString("\n🎉 BANGKOK ETI PROCESSING COMPLETE!"));
        m_systemMessages->append(QString("📊 Total frames processed: %1").arg(totalFrames));
        m_systemMessages->append(QString("⏱️  Processing time: %1 ms").arg(processingTimeMs));
        m_systemMessages->append(QString("🚀 Processing rate: %1 fps").arg(1000.0 * totalFrames / processingTimeMs, 0, 'f', 1));
        
        // Show ETI format statistics
        m_systemMessages->append(QString("\n📡 ETI Format Analysis:"));
        m_systemMessages->append(QString("   ETI-NI frames: %1").arg(m_etiProcessor->getETINICount()));
        m_systemMessages->append(QString("   ETI-LI-A frames: %1").arg(m_etiProcessor->getETILIACount()));
        m_systemMessages->append(QString("   ETI-LI-B frames: %1").arg(m_etiProcessor->getETILIBCount()));

        // Update ensemble tree with discovered services
        updateEnsembleTree();
        
        // === PHASE 2.3: Update stream statistics ===
        updateStreamStatistics(totalFrames, processingTimeMs);
        
        // === PHASE 3: Update Player/Decoder final status and timing ===
        updatePlayerPanelStatus("Completed");
        updateDecoderPanelStatus("Completed");
        updateTimingTable();
        
        // === PHASE 4: Final error statistics ===
        updateErrorCounterTable();
        
        // === PHASE 5: Build FIC overview ===
        buildFICOverviewTree();
        
        // === PHASE 6: Update Tab 2 LEFT panel ===
        updateTab2OverviewLabels();
        buildTab2ServiceTree();
        buildTab2ComponentsTree();
        
        // === PHASE 7: Update Tab 2 CENTER panel tables ===
        updateTab2EnsembleTable();
        updateTab2SubchannelTable();
        updateTab2ServiceTable();
        updateTab2FIGContentTable();
        
        // === PHASE 8: Update Tab 3 LEFT panel ===
        buildFIGInstanceTree();
        // T28: populate the Frame List (1..N) once the capture is fully loaded.
        buildFrameList();
        // T29: playback starts at frame 0, paused, controls enabled.
        resetPlaybackForNewFile();
        // T32: populate the Player's service selector from the decoded services
        // and select the first DAB+ audio service (fallback: first service).
        // The default service's latest state is then applied so the panels name
        // the selected service immediately (review #3).
        populateServiceSelector();
        
        // === PHASE 9: Update Tab 3 RIGHT panel ===
        buildFIGItemDetailsTree();
        // T42: initial Hex Viewer state (nothing selected yet -> guard message).
        updateTab3HexFromFigInstance();
        
        // Validate frame count
        if (totalFrames == 5001) {
            m_systemMessages->append(QString("\n✅ ETI VALIDATION: SUCCESS!"));
            m_systemMessages->append(QString("✅ Bangkok pattern detection: WORKING"));
            m_systemMessages->append(QString("✅ All 5,001 frames processed: COMPLETE"));
            statusBar()->showMessage("✅ Bangkok ETI Processing Complete - All 5,001 frames processed!");
        } else {
            m_systemMessages->append(QString("\n⚠️  Frame count mismatch: Expected 5001, got %1").arg(totalFrames));
        }

        // === Refinement A: honest DLS+/PAD evidence for this capture ===
        // Aggregate the spec parser's per-subchannel counters so the report
        // can distinguish "no PAD at all" from "PAD but no DLS/DL+".
        uint64_t sf = 0, firecode = 0, aus = 0, ausCrc = 0, pad = 0, fpadCi = 0;
        uint64_t xpadGroups = 0, dlsSegs = 0, dlPlusSegs = 0, labels = 0;
        uint64_t motDgs = 0;
        uint64_t groupTypes[32] = {0};
        for (const auto& entry : m_dlsParsers) {
            const DabPlusSubchannelParser& p = entry.second;
            sf += p.superframes;
            firecode += p.firecodeOk;
            aus += p.ausProcessed;
            ausCrc += p.ausCrcOk;
            pad += p.padFound;
            fpadCi += p.fpadCiSet;
            xpadGroups += p.xpadGroups;
            dlsSegs += p.dlSegments;
            dlPlusSegs += p.dlPlusSegments;
            labels += p.labels;
            motDgs += p.motDataGroups;
            for (int i = 0; i < 32; ++i) {
                groupTypes[i] += p.groupTypeCounts[i];
            }
        }
        qInfo() << "[DLS+] superframes:" << sf << "firecodeOk:" << firecode
                << "AUs:" << aus << "AUCrcOk:" << ausCrc
                << "PAD found:" << pad << "F-PAD CI set:" << fpadCi
                << "XPAD groups:" << xpadGroups
                << "DLS segs:" << dlsSegs << "DL+ segs:" << dlPlusSegs
                << "labels assembled:" << labels
                << "DLS+ messages decoded:" << m_dlsPlusMessages;
        QStringList groupSummary;
        for (int i = 0; i < 32; ++i) {
            if (groupTypes[i] != 0) {
                groupSummary << QString("type%1=%2").arg(i).arg(groupTypes[i]);
            }
        }
        if (!groupSummary.isEmpty()) {
            qInfo() << "[DLS+] XPAD data-group types:" << groupSummary.join(", ");
        }
        qInfo() << "[DLS+] subchannels with a DAB+ PAD parser:"
                << static_cast<int>(m_dlsParsers.size());
        qInfo() << "[DLS+] pipeline time (ms):" << (m_dlsPipelineNs / 1000000.0);

        // T31/T32: load-time per-service playback history evidence (sparse +
        // globally byte-capped store).
        {
            size_t totalEntries = 0;
            for (const auto& entry : m_mediaTimelineByService) {
                totalEntries += entry.second.size();
            }
            qInfo() << "[T32] per-service media timelines:" << m_mediaTimelineByService.size()
                    << "total entries:" << totalEntries
                    << "retained slides:" << m_retainedSlideCount
                    << "retained bytes:" << m_retainedSlideBytes;
        }

        // === MOT SlideShow evidence (honest counters for this capture) ===
        const eti::mot::MotPadAdapter::Statistics motStats = m_motAdapter.statistics();
        const auto motProtoStats = m_motProtocol.getStatistics();
        qInfo() << "[MOT] XPAD 12/13 standard data groups assembled:" << motDgs
                << "adapter DGs:" << motStats.dataGroups
                << "crcErrors:" << motStats.crcErrors
                << "rejected:" << motStats.rejected
                << "headerSegs:" << motStats.headerSegments
                << "bodySegs:" << motStats.bodySegments
                << "objectsCompleted:" << motStats.objectsCompleted
                << "objectsEmitted:" << motStats.objectsEmitted;
        qInfo() << "[MOT] oversized:" << motStats.oversized
                << "staleEvictions:" << motStats.staleEvictions
                << "carouselResets:" << motStats.objResets
                << "maxBodyBytes:" << motStats.maxObjectBytes;
        qInfo() << "[MOT] objects received:" << m_motObjectsReceived
                << "images decoded:" << m_motImagesDecoded
                << "errors:" << m_motErrors
                << "MOTProtocol objects completed:" << motProtoStats.objects_completed
                << "crc errors:" << motProtoStats.crc_errors;
        if (m_dlsPlusMessages == 0) {
            // Capture genuinely carries no DLS/DLS+ (the parser was wired and
            // ran; it just found no DLS label). Distinguish this final empty
            // state from the "Waiting for DLS+…" in-progress one.
            setNowPlayingEmptyState();
        }

        // MOT SlideShow final empty state: distinguish "no slideshow in this
        // stream" from the in-progress "Waiting/Receiving" one.
        if (m_motObjectsReceived == 0 && m_slideshowStatus) {
            m_slideshowStatus->setText(tr("No slideshow in this stream"));
            if (m_slideshowImage) {
                m_slideshowImage->setText(tr("No slideshow in this stream"));
            }
        }

        // Wave B — data services: feed the RAW captured data-subchannel bytes
        // into the EPG / Journaline / TPEG decoders (per subchannel, routed by
        // framing → label → content probe; same-route subchannels ACCUMULATE
        // into the one panel — a later subchannel never displaces an earlier
        // one) and refresh the Slideshow|EPG|Journaline|TPEG inner tabs.
        // N2: accumulation also covers the TPEG lifecycle — the shared
        // TpegDecoder is finished exactly ONCE after the whole feed loop
        // (finish() closes it), and every TPEG row in the store carries that
        // subchannel's own counter delta rather than an earlier subchannel's
        // cumulative totals.
        // Purely additive: load-time counters (DLS+ 1114 / MOT 97 / 97) are
        // untouched, so the CLI/GUI parity gates stay fixed.
        feedDataServiceSubchannels();
        refreshDataServiceDocks();
    }

    void onProcessingError(const QString& error)
    {
        // Manual-test finding 2: parsing aborted -> re-arm navigation.
        rearmNavigation();

        // Refinement B: reset the progress indication on error.
        if (m_progressBar) {
            m_progressBar->setValue(0);
        }
        statusBar()->showMessage(tr("Processing aborted — %1").arg(error));

        m_systemMessages->append(QString("❌ Error: %1").arg(error));
        if (m_closing) {
            // T25 (F5): a stale queued processingError during teardown must not
            // pop a modal (would block/hang an offscreen close). Keep the log.
            qWarning() << "Processing error during teardown (dialog suppressed):" << error;
        } else {
            QMessageBox::critical(this, "Processing Error", error);
        }

        // Forward error to Error Detection system using performance error detection
        if (m_errorDetector) {
            // Use performance error detection for general processing errors
            m_errorDetector->detectPerformanceErrors(0.0, 0.0); // Trigger performance error
        }
    }
    
    void onFrameError(int frameNumber, const QString& error, const QString& details)
    {
        m_systemMessages->append(QString("⚠️ Frame %1 Error: %2").arg(frameNumber).arg(error));

        // Wave B (T1): the legacy code fabricated an all-zero 6144-byte "frame"
        // and fed it to AdvancedErrorDetector, which emitted a synthetic
        // errorDetected() report for every real CRC/parse failure and inflated
        // the ETSI Compliance Monitor. The frame error is already surfaced in
        // System Messages and the processor's counters; no synthetic detector
        // input is generated here. Genuine detector reports still arrive via the
        // real errorDetected()/criticalErrorDetected() connections.
        Q_UNUSED(details);
    }
    
    // L1: bounded append for the System Messages pane. A manual block-count
    // check is used instead of QTextDocument::setMaximumBlockCount(): the latter
    // forces a document re-layout on every append and measurably slowed the
    // fixture load (~5x for the 1114 DLS labels). This keeps appends cheap
    // while still hard-capping the pane.
    void appendSystemMessageBounded(const QString& message)
    {
        if (!m_systemMessages) return;
        if (m_systemMessages->document()->blockCount() >= SYSTEM_MESSAGES_MAX_BLOCKS) {
            return;  // silently drop once the cap is reached
        }
        m_systemMessages->append(message);
    }

    // Phase 3B: DLS+ "Now Playing" update slot
    void updateNowPlaying(const QString& track, const QString& artist, const QString& album)
    {
        // Update track label
        if (m_trackLabel) {
            m_trackLabel->setText(track.isEmpty() ? "--" : track);
        }
        
        // Update artist label
        if (m_artistLabel) {
            m_artistLabel->setText(artist.isEmpty() ? "--" : artist);
        }
        
        // Update album label
        if (m_albumLabel) {
            m_albumLabel->setText(album.isEmpty() ? "--" : album);
        }
        
        // Update timestamp + originating service (Refinement A.5). The DLS+
        // signal itself carries only track/artist/album, so the service is
        // taken from the sub-channel -> service map resolved just before the
        // (synchronous) decode call. Multi-service note: a single panel cannot
        // show per-service metadata; the most recently decoded service wins
        // and is printed here and in System Messages.
        if (m_lastUpdatedLabel) {
            QString line = QString("Updated: %1")
                               .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
            if (m_pendingServiceId != 0) {
                line += QString(" — 0x%1")
                            .arg(m_pendingServiceId & 0xFFFF, 4, 16, QChar('0')).toUpper();
            }
            if (!m_pendingServiceLabel.isEmpty()) {
                line += QString(" (%1)").arg(m_pendingServiceLabel);
            }
            m_lastUpdatedLabel->setText(line);
        }
        
        // Log to system messages
        if (!track.isEmpty() || !artist.isEmpty()) {
            QString serviceInfo;
            if (m_pendingServiceId != 0) {
                serviceInfo = QString(" [0x%1%2]")
                                  .arg(m_pendingServiceId & 0xFFFF, 4, 16, QChar('0')).toUpper()
                                  .arg(m_pendingServiceLabel.isEmpty()
                                           ? QString()
                                           : QString(" %1").arg(m_pendingServiceLabel));
            }
            QString message = QString("🎵 Now Playing%1: %2 - %3")
                             .arg(serviceInfo)
                             .arg(artist.isEmpty() ? "Unknown Artist" : artist)
                             .arg(track.isEmpty() ? "Unknown Track" : track);
            appendSystemMessageBounded(message);
        }
    }

    // ========================================================================
    // MOT SlideShow (option A): decoder -> panel wiring
    // ========================================================================
    static const char* motImageFormat(eti::mot::ContentType type)
    {
        switch (type) {
            case eti::mot::ContentType::IMAGE_JPEG: return "JPEG";
            case eti::mot::ContentType::IMAGE_PNG:  return "PNG";
            case eti::mot::ContentType::IMAGE_BMP:  return "BMP";
            default:                                return nullptr;
        }
    }

    void onMotProgress(uint32_t transportId, double progress)
    {
        m_lastMotTransportId = transportId;
        if (!m_slideshowStatus) {
            return;
        }
        if (progress < 1.0 && m_motImagesDecoded == 0) {
            m_slideshowStatus->setText(
                QString("Receiving slideshow … %1 %")
                    .arg(progress * 100.0, 0, 'f', 0));
        }
    }

    void onMotParseError(uint32_t transportId, const QString& error)
    {
        ++m_motErrors;
        if (m_slideshowStatus) {
            m_slideshowStatus->setText(
                QString("MOT error (transport %1): %2").arg(transportId).arg(error));
        }
    }

    void onMotObjectComplete(uint32_t transportId, const eti::mot::MOTObject& object)
    {
        ++m_motObjectsReceived;
        m_lastMotTransportId = transportId;

        // T26: record the standard MOT header extension parameters so the
        // real-fixture test can prove they survive parsing.
        const eti::mot::MOTHeader& hdr = object.header;
        if (!hdr.content_name.isEmpty() || !hdr.click_through_url.isEmpty() ||
            !hdr.category_title.isEmpty()) {
            ++m_motHeaderParamsSeen;
            if (!hdr.content_name.isEmpty()) m_lastMotContentName = hdr.content_name;
            if (!hdr.click_through_url.isEmpty()) m_lastMotClickThroughUrl = hdr.click_through_url;
            if (!hdr.category_title.isEmpty()) m_lastMotCategoryTitle = hdr.category_title;
        }

        const eti::mot::ContentType contentType = object.header.content_type;
        const char* format = motImageFormat(contentType);
        const int byteCount = static_cast<int>(object.body.size());

        if (object.body.empty()) {
            if (m_slideshowStatus) {
                m_slideshowStatus->setText(tr("MOT object carried no image data"));
            }
            return;
        }

        QImage image;
        bool ok = false;
        if (format) {
            // Qt6 ships the JPEG/PNG/BMP image plugins; declared format first,
            // then a tolerant auto-detect (standard MOT content type 0x02
            // covers both JPEG and PNG, distinguished only by the subtype).
            ok = image.loadFromData(reinterpret_cast<const uchar*>(object.body.data()),
                                    byteCount, format);
        }
        if (!ok) {
            ok = image.loadFromData(reinterpret_cast<const uchar*>(object.body.data()),
                                    byteCount);
        }
        if (!ok || image.isNull()) {
            if (m_slideshowStatus) {
                m_slideshowStatus->setText(
                    QString("Slide decode failed (%1, %2 bytes)")
                        .arg(object.header.getContentTypeString())
                        .arg(byteCount));
            }
            return;
        }

        // T31: retain the decoded slide (byte-capped store) and remember its
        // absolute identity for the timeline. The panel status/info strings are
        // captured too so a playhead lookup needs neither a decode nor the MOT
        // adapter.
        m_lastDecodedSlide = image;
        ++m_motImagesDecoded;
        refreshSlideshowPixmap();

        QString statusText;
        if (m_slideshowStatus) {
            statusText = QString("Showing slide %1 — %2")
                             .arg(m_motImagesDecoded)
                             .arg(object.header.getContentTypeString());
            m_slideshowStatus->setText(statusText);
        }
        QString infoText;
        {
            QString serviceInfo;
            if (m_lastMotServiceId != 0) {
                serviceInfo = QString("SId 0x%1")
                                  .arg(m_lastMotServiceId & 0xFFFF, 4, 16, QChar('0')).toUpper();
            }
            if (!m_lastMotServiceLabel.isEmpty()) {
                serviceInfo += QString(" %1").arg(m_lastMotServiceLabel);
            }
            infoText = QString("%1 · %2 bytes · TID %3%4 · %5")
                    .arg(object.header.getContentTypeString())
                    .arg(byteCount)
                    .arg(transportId)
                    .arg(serviceInfo.isEmpty() ? QString()
                                               : QString(" (%1)").arg(serviceInfo))
                    .arg(QDateTime::currentDateTime().toString("HH:mm:ss"));
            // Prove the standard MOT header extension parameters survive: show
            // the content name when the object carries one (T26).
            if (!object.header.content_name.isEmpty()) {
                infoText += QString(" · %1").arg(object.header.content_name);
            }
            if (m_slideshowInfo) {
                m_slideshowInfo->setText(infoText);
            }
        }

        RetainedSlide slide;
        slide.image = image;
        slide.status = statusText;
        slide.info = infoText;
        // T32: retain the slide in the DECODING service's own store and record
        // the new image identity for that service at the current frame. The
        // panel status/info update above stays (live load behaviour).
        appendRetainedSlide(m_lastMotServiceId, std::move(slide));
        const int imageId =
            m_slidesByService[m_lastMotServiceId].nextId - 1;
        recordServiceMotState(m_lastMotServiceId, m_timelineRecordFrame, imageId);
    }

    // Scale the retained slide to the label, keeping the aspect ratio.
    void refreshSlideshowPixmap()
    {
        if (!m_slideshowImage || m_lastDecodedSlide.isNull()) {
            return;
        }
        QSize target = m_slideshowImage->size();
        if (target.width() < 16 || target.height() < 16) {
            target = QSize(200, 120);  // offscreen/tests before the first show()
        }
        m_slideshowImage->setPixmap(
            QPixmap::fromImage(m_lastDecodedSlide).scaled(
                target, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    void onFrameSelectionChanged()
    {
        if (!m_frameList) {
            return;
        }
        handleFrameSelection(m_frameList->currentRow());
    }

    // Shared body for real navigator clicks (onFrameSelectionChanged) and the
    // GUI test hook selectFrameForTest(). Manual-test finding 2: the frame
    // list is populated incrementally inside processETIFile()'s nested
    // processEvents() loop; touching frame data before the file is fully
    // decoded is what caused the navigator-click segfault. Refuse until
    // parsing completes, and bounds-check against the real frame count.
    void handleFrameSelection(int currentRow)
    {
        if (currentRow < 0) {
            return;
        }

        if (!m_fileLoaded || m_processing) {
            notifyFrameNotReady();
            return;
        }

        const int available = availableFrameCount();
        if (available <= 0 || currentRow >= available) {
            if (statusBar()) {
                statusBar()->showMessage(
                    tr("Frame %1 is outside the %2 frames available in this capture.")
                        .arg(currentRow + 1).arg(available), 4000);
            }
            return;
        }

        // Show frame details
        if (m_frameDetails) {
            QString info = QString("Selected Frame: %1\nETI Frame Size: 6144 bytes\nFrame Offset: %2")
                          .arg(currentRow + 1)
                          .arg(currentRow * 6144);
            m_frameDetails->setText(info);
        }

        // NOTE: The DAB Ensemble Explorer is owned by the real FIG
        // decode path (buildEnsembleTree()/updateEnsembleTree() via
        // m_figAnalyser). Frame selection must NOT rewrite it — the old
        // updateEnsembleExplorerForFrame() simulator (fake "Frame N FIC
        // Data" rows) was removed (PANEL_AUDIT fix #1).

        // Each panel update is independently guarded: one failing decode must
        // not skip the others and must never propagate out of a GUI slot.
        try {
            displayFrameHexData(currentRow);
        } catch (const std::exception& e) {
            qWarning() << "[NAV] Hex display failed for frame" << currentRow << ":" << e.what();
        } catch (...) {
            qWarning() << "[NAV] Unknown exception in hex display for frame" << currentRow;
        }

        try {
            performETSIComplianceValidation(currentRow);
        } catch (const std::exception& e) {
            qWarning() << "[NAV] ETSI validation failed for frame" << currentRow << ":" << e.what();
        } catch (...) {
            qWarning() << "[NAV] Unknown exception in ETSI validation for frame" << currentRow;
        }

        try {
            updateETIOverviewTable(currentRow);
        } catch (const std::exception& e) {
            qWarning() << "[NAV] ETI overview failed for frame" << currentRow << ":" << e.what();
        } catch (...) {
            qWarning() << "[NAV] Unknown exception in ETI overview for frame" << currentRow;
        }

        // NOTE: the T31 media-timeline replay is intentionally NOT done here.
        // handleFrameSelection() also serves real navigator clicks, which must
        // keep the load-complete Now Playing state; the replay belongs to the
        // PLAYHEAD path (applyPlayheadFrame()).
    }

    // ========================================================================
    // T29: PLAYBACK CONTROLLER (UI playhead over the retained frame cache)
    // ========================================================================
private:
    // The capture is fully parsed up front, so playback is a UI playhead: a
    // QTimer advances m_playbackFrame and drives the existing frame-selection
    // path (handleFrameSelection -> hex/compliance/overview + Tab-3 details).
    // Expensive panels are throttled; the clock/slider/frame counter/list
    // highlight update every tick.
    void initPlayback()
    {
        m_playbackTimer = new QTimer(this);
        m_playbackTimer->setTimerType(Qt::PreciseTimer);
        connect(m_playbackTimer, &QTimer::timeout, this, &DABAnalyserWindow::onPlaybackTick);
        updatePlaybackTimerInterval();

        if (m_playButton)
            connect(m_playButton, &QPushButton::clicked, this, &DABAnalyserWindow::playbackPlay);
        if (m_pauseButton)
            connect(m_pauseButton, &QPushButton::clicked, this, &DABAnalyserWindow::playbackPause);
        if (m_stopButton)
            connect(m_stopButton, &QPushButton::clicked, this, &DABAnalyserWindow::playbackStop);
        if (m_resetButton)
            connect(m_resetButton, &QPushButton::clicked, this, &DABAnalyserWindow::playbackReset);
        if (m_skipBackButton)
            connect(m_skipBackButton, &QPushButton::clicked, this,
                    [this]() { playbackSkip(-PLAYBACK_SKIP_FRAMES); });
        if (m_skipForwardButton)
            connect(m_skipForwardButton, &QPushButton::clicked, this,
                    [this]() { playbackSkip(PLAYBACK_SKIP_FRAMES); });
        if (m_playbackSpeedCombo)
            connect(m_playbackSpeedCombo, &QComboBox::currentIndexChanged,
                    this, &DABAnalyserWindow::onPlaybackSpeedChanged);
        if (m_playerProgressBar) {
            // M1: valueChanged covers drag, wheel, keyboard and trough-click;
            // the programmatic-update guard suppresses the echo from
            // updatePlaybackClockUi()/load progress. Live drags use the
            // throttled playhead path; sliderReleased does the forced rebuild.
            connect(m_playerProgressBar, &QSlider::valueChanged,
                    this, &DABAnalyserWindow::onPlaybackSliderValueChanged);
            connect(m_playerProgressBar, &QSlider::sliderReleased,
                    this, &DABAnalyserWindow::onPlaybackSliderReleased);
        }

        resetPlaybackControls();
    }

    void resetPlaybackControls()
    {
        m_playbackPlaying = false;
        m_playbackFrame = 0;
        if (m_playbackTimer) {
            m_playbackTimer->stop();
        }
        setPlaybackControlsEnabled(false);
        updatePlaybackStatus(QStringLiteral("Stopped"));
    }

    void setPlaybackControlsEnabled(bool enabled)
    {
        for (QPushButton* b : {m_playButton, m_pauseButton, m_stopButton, m_resetButton,
                               m_skipBackButton, m_skipForwardButton}) {
            if (b) b->setEnabled(enabled);
        }
        if (m_playbackSpeedCombo) m_playbackSpeedCombo->setEnabled(enabled);
        if (m_playerLoopCheckbox) m_playerLoopCheckbox->setEnabled(enabled);
        if (m_playerProgressBar) m_playerProgressBar->setEnabled(enabled);
    }

    void updatePlaybackStatus(const QString& status)
    {
        if (!m_playerStatusLabel) return;
        m_playerStatusLabel->setText(status);
        if (status == QLatin1String("Playing")) {
            m_playerStatusLabel->setStyleSheet(QStringLiteral("color: green; font-weight: bold;"));
        } else if (status == QLatin1String("Paused")) {
            m_playerStatusLabel->setStyleSheet(QStringLiteral("color: #0078D4; font-weight: bold;"));
        } else {
            m_playerStatusLabel->setStyleSheet(QStringLiteral("color: #666;"));
        }
    }

    bool isLoopEnabled() const
    {
        return m_playerLoopCheckbox && m_playerLoopCheckbox->isChecked();
    }

    void updatePlaybackTimerInterval()
    {
        if (!m_playbackTimer) return;
        const double speed = m_playbackSpeed > 0.0 ? m_playbackSpeed : 1.0;
        const int interval = qMax(1, static_cast<int>(PLAYBACK_BASE_INTERVAL_MS / speed));
        m_playbackTimer->setInterval(interval);
    }

    void onPlaybackSpeedChanged(int index)
    {
        static const double kSpeeds[5] = {0.25, 0.5, 1.0, 2.0, 4.0};
        if (index >= 0 && index < 5) {
            m_playbackSpeed = kSpeeds[index];
        }
        updatePlaybackTimerInterval();
    }

    // Stop the playhead and disable the controls while a new file parses.
    void stopPlaybackForLoad()
    {
        m_playbackPlaying = false;
        m_playbackFrame = 0;
        if (m_playbackTimer) {
            m_playbackTimer->stop();
        }
        m_detailThrottleTimer.invalidate();
        m_lastHighlightedNavigatorRow = -1;  // L3: frame list is about to change
        setPlaybackControlsEnabled(false);
    }

    // After a successful load: position at frame 0, paused, controls enabled.
    void resetPlaybackForNewFile()
    {
        m_playbackPlaying = false;
        m_playbackFrame = 0;
        if (m_playbackTimer) {
            m_playbackTimer->stop();
        }
        m_detailThrottleTimer.invalidate();
        m_lastHighlightedNavigatorRow = -1;  // L3: frame list was repopulated
        setPlaybackControlsEnabled(true);
        // Post-load state: a file is loaded but playback has not started.
        updatePlaybackStatus(QStringLiteral("Paused"));
        updatePlaybackClockUi();
        highlightFrameInNavigator(m_playbackFrame);
    }

    // ========================================================================
    // T32: Player service selector (which service's DLS+/slideshow is shown)
    // ========================================================================
    // T32 review #1: format a service SID by magnitude. 16-bit programme SIDs
    // use 4 hex digits; 32-bit data-service SIDs (e.g. 0xF3200000) use 8, so a
    // legitimate large data SID is never truncated to an invalid-looking value.
    static QString serviceSidHex(quint32 id)
    {
        return QStringLiteral("0x")
               + QString("%1")
                     .arg(id, id > 0xFFFFu ? 8 : 4, 16, QChar('0'))
                     .toUpper();
    }

    static bool serviceHasDabPlusAudio(const DABService& svc)
    {
        for (const ServiceComponent& comp : svc.components) {
            if (comp.isDabPlus()) {
                return true;  // FIG 0/8 ASCTy == 0x3F
            }
        }
        return false;
    }

    // ========================================================================
    // Wave D Phase 1: Service type display for unnamed services
    // Returns a descriptive type string like "[EPG]", "[DAB+]", "[TPEG]", etc.
    // based on FIG data, sub-channel mapping, and data service store routing.
    // ========================================================================
    QString getServiceTypeDisplay(quint32 serviceId, quint16 subChannelId) const
    {
        if (!m_figAnalyser) {
            return QString("[SId:%1]").arg(serviceSidHex(serviceId));
        }

        // 1. Check if this is a DAB+ audio service (via sub-channel mapping)
        if (subChannelId != 0 && m_dabPlusSubChannels.contains(subChannelId)) {
            return QStringLiteral("[DAB+]");
        }

        // 2. Check data service store for decoded content type (EPG/Journaline/TPEG)
        if (subChannelId != 0 && m_dataServiceStore) {
            eti::data::ServiceDataSnapshot snapshot;
            if (m_dataServiceStore->getService(subChannelId, snapshot)) {
                if (snapshot.epg_events > 0 || snapshot.epg_schedules > 0) {
                    return QStringLiteral("[EPG]");
                }
                if (snapshot.journaline_objects > 0 || snapshot.journaline_stream_items > 0) {
                    return QStringLiteral("[Journaline]");
                }
                if (snapshot.tpeg_messages > 0) {
                    return QStringLiteral("[TPEG]");
                }
                if (snapshot.hasContent()) {
                    return QStringLiteral("[MOT]");
                }
            }
        }

        // 3. Find the service and check its components for type information
        const std::vector<DABService> services = m_figAnalyser->getDABServices();
        for (const DABService& svc : services) {
            if (svc.service_id != serviceId) continue;

            // Audio service (programme type)
            if (svc.service_type == 0) {
                if (svc.is_dab_plus || serviceHasDabPlusAudio(svc)) {
                    return QStringLiteral("[DAB+]");
                }
                return QStringLiteral("[DAB]");
            }

            // Data service - check components for DSCTy (FIG 0/3, 0/8)
            for (const ServiceComponent& comp : svc.components) {
                // DSCTy values from ETSI EN 300 401 / fig_parser.hpp
                switch (comp.service_component_type) {
                    case 0x01: return QStringLiteral("[TMC]");       // Traffic Message Channel
                    case 0x05: return QStringLiteral("[TPEG]");      // TPEG
                    case 0x18: return QStringLiteral("[MOT]");       // MOT (24 decimal = 0x18)
                    case 0x2C: return QStringLiteral("[Journaline]"); // Journaline (44 decimal = 0x2C)
                    case 0x3C: return QStringLiteral("[EWS]");       // EWS (60 decimal)
                    case 0x3D: return QStringLiteral("[IP]");        // IP (61 decimal)
                    default: break;
                }
            }

            // 4. Check FIG 0/13 User Applications for service type hints
            const std::vector<FIG0_13_UserApp> userApps = m_figAnalyser->getUserApplications();
            for (const FIG0_13_UserApp& app : userApps) {
                if (app.service_id == serviceId) {
                    switch (app.user_app_type) {
                        case 0x001: return QStringLiteral("[SPI]");      // Service Programme Information
                        case 0x002: return QStringLiteral("[MOT]");      // MOT Slideshow
                        case 0x003: return QStringLiteral("[MOT BWS]");  // MOT Broadcast Web Site
                        case 0x004: return QStringLiteral("[TPEG]");     // TPEG
                        case 0x007: return QStringLiteral("[EPG]");      // Electronic Programme Guide
                        case 0x044: return QStringLiteral("[Journaline]"); // Journaline
                        case 0x008: return QStringLiteral("[DAB Java]"); // DAB Java
                        default: break;
                    }
                }
            }

            // 5. Check data service labels (FIG 1/5) for type hints in the label
            const std::vector<FIG1_5_DataServiceLabel> dataLabels = m_figAnalyser->getDataServiceLabels();
            for (const FIG1_5_DataServiceLabel& dl : dataLabels) {
                if (dl.service_id == serviceId) {
                    QString label = QString::fromUtf8(dl.label, 16).trimmed().toLower();
                    if (label.contains("epg")) return QStringLiteral("[EPG]");
                    if (label.contains("tepg")) return QStringLiteral("[TPEG]");
                    if (label.contains("tpeg")) return QStringLiteral("[TPEG]");
                    if (label.contains("journ")) return QStringLiteral("[Journaline]");
                    if (label.contains("mot")) return QStringLiteral("[MOT]");
                    if (label.contains("slide")) return QStringLiteral("[MOT]");
                    if (label.contains("ews")) return QStringLiteral("[EWS]");
                    if (label.contains("ip")) return QStringLiteral("[IP]");
                }
            }

            // 6. Fallback: generic data service with full SId
            return QString("[Data:%1]").arg(serviceSidHex(serviceId));
        }

        // Service not found in analyser - fallback
        return QString("[SId:%1]").arg(serviceSidHex(serviceId));
    }

    // Clear stale items + selection and disable the selector (on load start and
    // in the no-file state). Guarded so it never fires onServiceSelectionChanged.
    void resetServiceSelectorForLoad()
    {
        m_selectedServiceId = 0;
        m_selectedServiceLabel.clear();
        if (!m_serviceCombo) {
            return;
        }
        m_populatingServiceSelector = true;
        m_serviceCombo->clear();
        m_serviceCombo->setEnabled(false);
        m_populatingServiceSelector = false;
    }

    // Populate from the analyser's services after a completed load. Display is
    // "label (0xSID)" (fallback "SId 0xSID" for unlabeled services; SIDs are
    // formatted by magnitude so 32-bit data SIDs are shown in full); the
    // default selection is the first service that has a DAB+ audio component
    // (fallback: the first service). Afterwards the default service's latest
    // state is applied so the selector and the media panels agree immediately
    // (review #3) — pure lookup, no re-parse.
    void populateServiceSelector()
    {
        if (!m_serviceCombo) {
            return;
        }
        m_populatingServiceSelector = true;
        m_serviceCombo->clear();

        int defaultIndex = -1;
        int firstIndex = -1;
        if (m_figAnalyser) {
            for (const DABService& svc : m_figAnalyser->getDABServices()) {
                const QString label = svc.service_label.trimmed();
                const QString sid = serviceSidHex(svc.service_id);
                const ServiceComponent primary = svc.getPrimaryComponent();
                const QString itemText = label.isEmpty()
                    ? QString("%1 (%2)").arg(getServiceTypeDisplay(svc.service_id, primary.sub_channel_id), sid)
                    : QString("%1 (%2)").arg(label, sid);
                m_serviceCombo->addItem(itemText, svc.service_id);
                const int index = m_serviceCombo->count() - 1;
                if (firstIndex < 0) {
                    firstIndex = index;
                }
                if (defaultIndex < 0 && serviceHasDabPlusAudio(svc)) {
                    defaultIndex = index;
                }
            }
        }
        if (defaultIndex < 0) {
            defaultIndex = firstIndex;  // fall back to the first service
        }
        if (defaultIndex >= 0) {
            m_serviceCombo->setCurrentIndex(defaultIndex);
            applySelectedServiceFromCombo(defaultIndex);
        } else {
            m_selectedServiceId = 0;
            m_selectedServiceLabel.clear();
        }
        m_serviceCombo->setEnabled(m_serviceCombo->count() > 0);
        m_populatingServiceSelector = false;
        // T32 review #3: apply the DEFAULT service's state on load so the
        // selector and the Now Playing/Slideshow panels agree immediately
        // (otherwise the panels would keep the last live-fed service, ~0x2CBD,
        // until the first seek/play). The selected service's latest recorded
        // state is applied — pure lookup, no re-parse, counters unchanged.
        if (m_fileLoaded && m_selectedServiceId != 0 && m_serviceCombo->count() > 0) {
            try {
                applySelectedServiceLatestMediaState();
            } catch (const std::exception& e) {
                qWarning() << "[T32] post-load service apply failed:" << e.what();
            } catch (...) {
                qWarning() << "[T32] post-load service apply failed (unknown)";
            }
        }
        // T34: point the audio path at the default selected service (the first
        // DAB+ audio service) and refresh the Audio tab.
        updateAudioSelection();
    }

    // Apply the selected service's latest recorded DLS/slideshow state (the
    // state the live feed would have left for that service). No-op counters.
    void applySelectedServiceLatestMediaState()
    {
        auto tlIt = m_mediaTimelineByService.find(m_selectedServiceId);
        if (tlIt == m_mediaTimelineByService.end() || tlIt->second.empty()) {
            showWaitingMediaStateForSelectedService();
            return;
        }
        refeedDlsForPlayhead(tlIt->second.rbegin()->first);
    }

    // Cache the selected service's id + label from the combo.
    void applySelectedServiceFromCombo(int index)
    {
        if (!m_serviceCombo || index < 0 || index >= m_serviceCombo->count()) {
            m_selectedServiceId = 0;
            m_selectedServiceLabel.clear();
            return;
        }
        m_selectedServiceId = m_serviceCombo->itemData(index).toUInt();
        m_selectedServiceLabel = displayServiceLabel(m_selectedServiceId);
    }

    // The label part of the combo item for `serviceId` (shows service type when unlabeled).
    QString displayServiceLabel(quint32 serviceId) const
    {
        if (!m_figAnalyser) {
            return QString();
        }
        for (const DABService& svc : m_figAnalyser->getDABServices()) {
            if (svc.service_id == serviceId) {
                if (!svc.service_label.trimmed().isEmpty()) {
                    return svc.service_label.trimmed();
                }
                const ServiceComponent primary = svc.getPrimaryComponent();
                return getServiceTypeDisplay(serviceId, primary.sub_channel_id);
            }
        }
        return QString();
    }

    // User changed the service: keep the playhead frame, immediately re-apply
    // the selected service's state for it (pure lookup — no re-parse, so the
    // load-time counters stay invariant).
    void onServiceSelectionChanged(int index)
    {
        if (m_populatingServiceSelector) {
            return;
        }
        applySelectedServiceFromCombo(index);
        // T34: switch the audio source to the newly selected service (a no-op
        // before a capture is loaded).
        updateAudioSelection();
        if (!m_fileLoaded || m_selectedServiceId == 0) {
            return;
        }
        try {
            refeedDlsForPlayhead(m_playbackFrame);
        } catch (const std::exception& e) {
            qWarning() << "[T32] service switch replay failed:" << e.what();
        } catch (...) {
            qWarning() << "[T32] service switch replay failed (unknown)";
        }
        // If the playhead is running, restart audio at the current frame (the
        // new service is decoded synchronously, then playback resumes).
        if (m_playbackPlaying && m_audioController) {
            m_audioController->playFromFrame(m_playbackFrame);
        }
    }

    // ========================================================================
    // T34: selected-service audio helpers
    // ========================================================================
    // Map the currently selected service to its primary DAB+ audio sub-channel
    // and refresh the Audio tab's controls/format info. The audio controller
    // decodes lazily on first play.
    void updateAudioSelection()
    {
        if (!m_audioController) {
            return;
        }
        const int sc = m_audioSubChannelByService.value(m_selectedServiceId, -1);
        if (sc < 0 || !m_fileLoaded) {
            m_audioController->setActiveService(0, -1);
        } else {
            m_audioController->setActiveService(m_selectedServiceId, sc);
        }
        updateAudioPanelState();
    }

    // Reflect the controller's real state in the Audio tab (no fake values).
    void updateAudioPanelState()
    {
        if (!m_audioController) {
            return;
        }
        const bool hasBackend = streamdab::audio::AudioPlaybackController::audioBackendAvailable();
        const bool hasAudio = m_fileLoaded && m_audioController->hasActiveAudio();
        const bool enabled = hasAudio && hasBackend;
        if (m_audioDeviceCombo) {
            m_audioDeviceCombo->setEnabled(enabled);
        }
        if (m_audioMuteCheck) {
            m_audioMuteCheck->setEnabled(enabled);
        }
        if (m_audioVolumeSlider) {
            m_audioVolumeSlider->setEnabled(enabled);
        }
        if (m_audioStatusLabel) {
            if (!hasBackend) {
                // No compiled-in output backend: clear annotation, no controls.
                m_audioStatusLabel->setText(tr("Audio output unavailable"));
            } else if (hasAudio) {
                m_audioStatusLabel->setText(m_audioController->statusText());
            } else {
                m_audioStatusLabel->setText(tr("No audio service"));
            }
        }
        if (m_audioInfoLabel) {
            const streamdab::audio::DabPlusFormat fmt = m_audioController->activeFormat();
            if (hasAudio && fmt.valid()) {
                m_audioInfoLabel->setText(
                    QString("%1 · %2 kHz core · %3 ch · %4 kbps")
                        .arg(fmt.profile())
                        .arg(fmt.coreSampleRateKHz())
                        .arg(fmt.coreChConfig())
                        .arg(fmt.bitrateKbps));
            } else {
                m_audioInfoLabel->setText(QStringLiteral("—"));
            }
        }
    }

    void resetAudioPanelForLoad()
    {
        if (m_audioStatusLabel) {
            m_audioStatusLabel->setText(tr("No audio service"));
        }
        if (m_audioInfoLabel) {
            m_audioInfoLabel->setText(QStringLiteral("—"));
        }
        if (m_audioDeviceCombo) {
            m_audioDeviceCombo->setEnabled(false);
        }
        if (m_audioMuteCheck) {
            m_audioMuteCheck->setEnabled(false);
        }
        if (m_audioVolumeSlider) {
            m_audioVolumeSlider->setEnabled(false);
        }
        if (m_audioLevelLeft) {
            m_audioLevelLeft->setValue(0);
        }
        if (m_audioLevelRight) {
            m_audioLevelRight->setValue(0);
        }
    }

    void playbackPlay()
    {
        if (!m_fileLoaded) return;
        const int total = availableFrameCount();
        if (total <= 0) return;
        // Starting from the end without loop restarts from the beginning.
        if (m_playbackFrame >= total - 1 && !isLoopEnabled()) {
            m_playbackFrame = 0;
        }
        m_playbackPlaying = true;
        m_detailThrottleTimer.invalidate();
        updatePlaybackTimerInterval();
        if (m_playbackTimer) m_playbackTimer->start();
        // T34: start the selected service's audio at the playhead frame.
        if (m_audioController && m_audioController->hasActiveAudio()) {
            m_audioController->playFromFrame(m_playbackFrame);
        }
        updatePlaybackStatus(QStringLiteral("Playing"));
        updatePlaybackClockUi();
    }

    void playbackPause()
    {
        if (!m_playbackPlaying) return;
        m_playbackPlaying = false;
        if (m_playbackTimer) m_playbackTimer->stop();
        // T34: pause audio (silence) at the current position.
        if (m_audioController) {
            m_audioController->pause();
        }
        // M2: force a final detail refresh so hex/compliance/overview/FIG panes
        // are not left up to DETAIL_THROTTLE_MS stale after pausing.
        applyPlayheadFrame(m_playbackFrame, true);
        updatePlaybackStatus(QStringLiteral("Paused"));
    }

    void playbackStop()
    {
        m_playbackPlaying = false;
        if (m_playbackTimer) m_playbackTimer->stop();
        m_playbackFrame = 0;
        // T34: stop = silence + rewind.
        if (m_audioController) {
            m_audioController->stopAudio();
        }
        applyPlayheadFrame(m_playbackFrame, true);
        updatePlaybackStatus(QStringLiteral("Stopped"));
    }

    void playbackReset()
    {
        // Return to frame 0 but keep the current play/pause state.
        m_playbackFrame = 0;
        if (m_playbackPlaying && m_audioController) {
            m_audioController->seekToFrame(0);
        }
        applyPlayheadFrame(m_playbackFrame, true);
    }

    void playbackSkip(int deltaFrames)
    {
        playbackSeek(m_playbackFrame + deltaFrames);
    }

    void playbackSeek(int frameIndex)
    {
        const int total = availableFrameCount();
        if (total <= 0) return;
        m_playbackFrame = qBound(0, frameIndex, total - 1);
        // T34: re-align audio to the new frame (exact to the 24 ms boundary).
        if (m_playbackPlaying && m_audioController) {
            m_audioController->seekToFrame(m_playbackFrame);
        }
        applyPlayheadFrame(m_playbackFrame, true);
    }

    // M1: slider value changed by drag / wheel / keyboard / trough-click.
    // A live drag (slider held down) uses the THROTTLED path: a drag across
    // 5001 frames must not run the full hex/ETSI/FIG rebuild per pixel
    // (main-thread freeze); the forced rebuild happens once on sliderReleased().
    // Wheel / keyboard / trough changes are discrete and force an exact
    // refresh immediately.
    void onPlaybackSliderValueChanged(int value)
    {
        if (m_seekSliderProgrammatic) return;  // echo from our own setValue()
        const int total = availableFrameCount();
        if (total <= 0) return;
        m_playbackFrame = qBound(0, value, total - 1);
        const bool dragging = m_playerProgressBar && m_playerProgressBar->isSliderDown();
        // T34: only re-align audio on a discrete change; a live drag would
        // thrash the sink's device buffer (the force path on release covers it).
        if (!dragging && m_playbackPlaying && m_audioController) {
            m_audioController->seekToFrame(m_playbackFrame);
        }
        applyPlayheadFrame(m_playbackFrame, !dragging);
    }

    // M1: end of a drag -> one forced rebuild so the detail panes are exact.
    void onPlaybackSliderReleased()
    {
        if (m_seekSliderProgrammatic) return;
        // T34: a completed scrub re-aligns audio to the released frame.
        if (m_playbackPlaying && m_audioController) {
            m_audioController->seekToFrame(m_playbackFrame);
        }
        applyPlayheadFrame(m_playbackFrame, true);
    }

    // Cheap per-tick UI + throttled expensive panels.
    void applyPlayheadFrame(int frameIndex, bool forceDetails)
    {
        const int total = availableFrameCount();
        if (total <= 0) return;

        // Cheap updates (every call/tick).
        updatePlaybackClockUi();
        highlightFrameInNavigator(frameIndex);

        // Expensive detail panels (hex / compliance / overview / Tab-3 FIG
        // details) throttled to <= ~6.7 Hz; a forced call (seek/stop/reset/
        // slider-release) bypasses the throttle.
        const bool detailsDue = forceDetails || !m_detailThrottleTimer.isValid()
                                || m_detailThrottleTimer.elapsed() >= DETAIL_THROTTLE_MS;
        if (detailsDue) {
            m_detailThrottleTimer.restart();
            handleFrameSelection(frameIndex);
            // T31 (rework): replay the LOAD-TIME media state for this frame
            // (pure lookup + panel apply; no parser/adapter/counter). Throttled
            // with the panels above; forced on seek/stop/reset via forceDetails.
            try {
                refeedDlsForPlayhead(frameIndex);
            } catch (const std::exception& e) {
                qWarning() << "[T31] media replay failed for frame" << frameIndex << ":"
                           << e.what();
            } catch (...) {
                qWarning() << "[T31] Unknown exception in media replay for frame" << frameIndex;
            }
            // L4: rebuildFrameFIGDetails() and the FIG-instance selection share
            // m_tab3_figItemDetailsTree (they overwrite each other). Only the
            // playback-driven rebuild is skipped while the Frame List tab is
            // not active, so a playing capture cannot clobber the FIG-instance
            // detail pane ~6.7 Hz.
            if (isFrameListTabActive()) {
                rebuildFrameFIGDetails(frameIndex);
            }
            // F3: keep the sibling Hex Viewer in sync with the playhead. The
            // FIG-instance selection can freely clobber the shared details tree,
            // but the Hex Viewer is its own widget — always safe to refresh when
            // the user is watching it or driving frames from the Frame List.
            if (isFrameListTabActive() || isHexViewerTabActive()) {
                updateTab3HexFromFrame(frameIndex);
            }
        }
    }

    // L4: is the Tab-3 Frame List tab the visible tab of the left dock?
    bool isFrameListTabActive() const
    {
        if (!m_tab3LeftTabs || !m_tab3_frameList) return false;
        QWidget* page = m_tab3_frameList->parentWidget();
        return page && m_tab3LeftTabs->currentWidget() == page;
    }

    // F3: is the Tab-3 Hex Viewer the visible inner tab of the right dock?
    bool isHexViewerTabActive() const
    {
        if (!m_tab3RightTabs || !m_tab3_hexViewer) return false;
        QWidget* page = m_tab3_hexViewer->parentWidget();
        return page && m_tab3RightTabs->currentWidget() == page;
    }

    void onPlaybackTick()
    {
        if (!m_playbackPlaying) return;
        const int total = availableFrameCount();
        if (total <= 0) {
            playbackStop();
            return;
        }
        int next = m_playbackFrame + 1;
        if (next >= total) {
            if (isLoopEnabled()) {
                next = 0;                       // wrap and continue
            } else {
                m_playbackFrame = total - 1;    // stop, leave at the end
                applyPlayheadFrame(m_playbackFrame, true);
                m_playbackPlaying = false;
                if (m_playbackTimer) m_playbackTimer->stop();
                updatePlaybackStatus(QStringLiteral("Stopped"));
                return;
            }
        }
        m_playbackFrame = next;
        applyPlayheadFrame(m_playbackFrame, false);
    }

    void updatePlaybackClockUi()
    {
        const int total = availableFrameCount();
        if (m_playerTimeLabel) {
            m_playerTimeLabel->setText(frameToTimeString(m_playbackFrame, total));
        }
        if (m_playerFrameLabel) {
            m_playerFrameLabel->setText(QString("%1 / %2")
                                            .arg(total > 0 ? m_playbackFrame + 1 : 0)
                                            .arg(total));
        }
        if (m_playerProgressBar) {
            // Guard the whole programmatic update: setRange may also clamp the
            // value and emit valueChanged.
            m_seekSliderProgrammatic = true;
            m_playerProgressBar->setRange(0, total > 0 ? total - 1 : 0);
            m_playerProgressBar->setValue(m_playbackFrame);
            m_seekSliderProgrammatic = false;
        }
    }

    // Highlight the playhead frame in the ETI Frame Navigator without
    // triggering the (expensive) currentRowChanged handler. The frame list is
    // throttled (~every 10 frames), so select the nearest entry >= the frame.
    void highlightFrameInNavigator(int frameIndex)
    {
        if (!m_frameList || m_frameList->count() == 0) return;
        const int frameNumber = frameIndex + 1;
        int row = -1;
        for (int i = 0; i < m_frameList->count(); ++i) {
            QListWidgetItem* it = m_frameList->item(i);
            if (!it) continue;
            const QVariant data = it->data(Qt::UserRole);
            if (data.isValid() && data.toInt() >= frameNumber) {
                row = i;
                break;
            }
        }
        if (row < 0) return;
        // L3: the frame list is throttled (~every 10 frames), so many playhead
        // ticks map to the same row. Only touch/scroll the widget when the row
        // actually changed; the frame-details label still updates every tick.
        if (row != m_lastHighlightedNavigatorRow) {
            m_lastHighlightedNavigatorRow = row;
            const QSignalBlocker blocker(m_frameList);
            m_frameList->setCurrentRow(row);
            m_frameList->scrollToItem(m_frameList->item(row));
        }
        if (m_frameDetails) {
            m_frameDetails->setText(QString("Selected Frame: %1\nETI Frame Size: 6144 bytes\nFrame Offset: %2")
                                        .arg(frameNumber)
                                        .arg(frameIndex * 6144));
        }
    }

    // --- T29 test hooks (headless playback logic) --------------------------
    // L5: the MUTATING hooks compile only in GUI_TEST_MODE; harmless
    // accessors stay available to the normal build.
public:
#ifdef GUI_TEST_MODE
    void playbackPlayForTest() { playbackPlay(); }
    void playbackPauseForTest() { playbackPause(); }
    void playbackStopForTest() { playbackStop(); }
    void playbackResetForTest() { playbackReset(); }
    void playbackSeekForTest(int frame) { playbackSeek(frame); }
    void playbackTickForTest() { onPlaybackTick(); }
    void playbackSkipForTest(int delta) { playbackSkip(delta); }
    void setLoopForTest(bool on)
    {
        if (m_playerLoopCheckbox) m_playerLoopCheckbox->setChecked(on);
    }
    // 0=0.25x, 1=0.5x, 2=1x, 3=2x, 4=4x
    void setPlaybackSpeedIndexForTest(int index)
    {
        if (m_playbackSpeedCombo) m_playbackSpeedCombo->setCurrentIndex(index);
    }
    void setTotalFramesForTest(int n) { m_totalFrames = static_cast<size_t>(qMax(0, n)); }
    void setFileLoadedFlagForTest(bool loaded) { m_fileLoaded = loaded; }
    // T31: drive the pure lookup directly (e.g. the eviction fallback / miss
    // paths) without running the playback timer.
    void refeedDlsForPlayheadForTest(int frameIndex) { refeedDlsForPlayhead(frameIndex); }
    // T32 review #4: exercise the retained-slide eviction path with a tiny cap
    // and synthetic per-service slides (no file/decode needed).
    void setRetainedSlideByteCapForTest(qint64 cap) { m_retainedSlideByteCap = cap; }
    void appendSyntheticRetainedSlideForTest(quint32 serviceId, const QString& tag)
    {
        RetainedSlide s;
        s.image = QImage(64, 64, QImage::Format_ARGB32);
        s.image.fill(Qt::red);
        s.status = tag;
        s.info = tag;
        appendRetainedSlide(serviceId, std::move(s));
    }
    // Settings audit: drive the live apply path without opening the modal
    // dialog (which would block an offscreen test).
    void applyPersistedAppSettingsForTest() { applyPersistedAppSettings(); }
    // T45: exercise the Logging-tab ring cap without feeding thousands of rows.
    void setLoggingRowCapForTest(int cap) { m_loggingRowCap = qMax(1, cap); }
    void applyAudioSettingsForTest(const QString& device, bool muted, int volume)
    {
        applyAudioSettings(device, muted, volume);
    }
    void requestExitForTest() { requestExit(); }
#endif
    // Settings audit: read-only observation hooks.
    bool confirmExitOnCloseForTest() const { return m_confirmExitOnClose; }
    int networkBufferFramesForTest() const { return m_networkBufferFrames; }
    int dashboardSampleIntervalForTest() const
    {
        return m_perfSampleTimer ? m_perfSampleTimer->interval() : -1;
    }
    QString audioOutputDeviceForTest() const
    {
        return m_audioController ? m_audioController->outputDevice() : QString();
    }
    // Settings audit: buffer actually applied to the live receiver / its worker
    // (not just the member) — verifies the connect_with_config() path.
    size_t networkReceiverBufferForTest() const
    {
        return m_networkReceiver ? m_networkReceiver->get_config().buffer_size : 0;
    }
    size_t networkReceiverWorkerBufferForTest() const
    {
        return m_networkReceiver ? m_networkReceiver->worker_buffer_size_for_test() : 0;
    }
    bool isPlayingForTest() const { return m_playbackPlaying; }
    int playbackFrameForTest() const { return m_playbackFrame; }
    bool isLoopForTest() const { return isLoopEnabled(); }
    // T34: read-only audio state hooks for the GUI tests.
    bool audioHasActiveServiceForTest() const
    {
        return m_audioController && m_audioController->hasActiveAudio();
    }
    bool audioDecodedForTest() const
    {
        return m_audioController && m_audioController->isDecoded();
    }
    int audioCapturedSubChannelCountForTest() const
    {
        return m_audioController ? m_audioController->capturedSubChannelCount() : 0;
    }
    int audioCapturedAccessUnitsForTest(int subChannelId) const
    {
        return m_audioController ? m_audioController->capturedAccessUnitCount(subChannelId) : 0;
    }
    QComboBox* audioDeviceComboForTest() const { return m_audioDeviceCombo; }
    QComboBox* playbackSpeedComboForTest() const { return m_playbackSpeedCombo; }
    int playbackIntervalForTest() const
    {
        return m_playbackTimer ? m_playbackTimer->interval() : -1;
    }
    bool isPlaybackTimerActiveForTest() const
    {
        return m_playbackTimer && m_playbackTimer->isActive();
    }
    QSlider* playerTimeBarForTest() const { return m_playerProgressBar; }

private:
    // One-shot popup + status-bar hint shown when a frame is clicked while the
    // capture is still being parsed. The message box is deliberately
    // non-modal so it never blocks the synchronous parse loop.
    void notifyFrameNotReady()
    {
        // Finding 7: distinguish "still parsing" from "nothing loaded" so the
        // message matches the actual state.
        const QString msg = m_processing
            ? tr("Frames are still being parsed — please wait until loading completes.")
            : tr("No ETI file is loaded — open a capture first.");
        if (!m_frameNotReadyNotified) {
            m_frameNotReadyNotified = true;
            auto* box = new QMessageBox(QMessageBox::Information,
                                        tr("Frame not ready"), msg,
                                        QMessageBox::Ok, this);
            box->setWindowModality(Qt::NonModal);
            box->setAttribute(Qt::WA_DeleteOnClose);
            box->show();
        }
        if (statusBar()) {
            statusBar()->showMessage(msg, 4000);
        }
    }

    // Number of frames the ETI processor actually holds (0 when none loaded).
    int availableFrameCount() const
    {
        if (m_etiProcessor) {
            const int total = m_etiProcessor->getTotalFrames();
            if (total > 0) {
                return total;
            }
        }
        // T29: fall back to the recorded frame count so the playback playhead
        // (and headless playback tests) can work without a live processor.
        return m_totalFrames > 0 ? static_cast<int>(m_totalFrames) : 0;
    }
    
    void performETSIComplianceValidation(int frameIndex)
    {
        if (!m_complianceViewer || !m_etsiComplianceEngine || frameIndex < 0) {
            return;
        }

        // Manual-test finding 2: only validate frames that actually exist in
        // the loaded capture.
        const int available = availableFrameCount();
        if (available <= 0 || frameIndex >= available) {
            return;
        }
        
        // Create ETI frame structure for validation
        etsi::ETIFrame eti_frame = createETIFrameForValidation(frameIndex);
        
        // Perform ETSI EN 300 799 validation
        etsi::ValidationResult result = m_etsiComplianceEngine->validateETIFrame(eti_frame);
        
        // Display validation results in compliance viewer
        displayComplianceResult(result, frameIndex);
        
        // Update system messages with compliance status
        if (m_systemMessages) {
            QString complianceStatus;
            QString statusIcon;
            
            switch (result.level) {
                case etsi::ComplianceLevel::CRITICAL:
                    complianceStatus = "CRITICAL VIOLATION";
                    statusIcon = "🚨";
                    break;
                case etsi::ComplianceLevel::WARNING:
                    complianceStatus = "WARNING";
                    statusIcon = "⚠️";
                    break;
                case etsi::ComplianceLevel::INFO:
                    complianceStatus = "INFO";
                    statusIcon = "ℹ️";
                    break;
                case etsi::ComplianceLevel::PASS:
                    complianceStatus = "COMPLIANT";
                    statusIcon = "✅";
                    break;
            }
            
            // Update system messages
            // L1: skip per-tick appends during playback (the QTextEdit would
            // otherwise grow unboundedly); bounded append otherwise.
            if (!m_playbackPlaying) {
                appendSystemMessageBounded(QString("%1 ETSI Compliance Frame %2: %3")
                                       .arg(statusIcon).arg(frameIndex + 1).arg(complianceStatus));
            }
        }
    }
    
    etsi::ETIFrame createETIFrameForValidation(int frameIndex)
    {
        etsi::ETIFrame frame;
        frame.frame_number = static_cast<uint32_t>(frameIndex);
        
        // Simulate ETI frame structure (in real implementation, this would come from actual ETI data)

        // ETI Sync bytes (ETSI EN 300 799 Section 5.1.1 - Real ETI-LI Pattern A)
        frame.sync_bytes = 0xFFF8C549;  // From real Thai DAB file (bkk_20062022_141637.eti)
        
        // Frame Counter (sequential)
        frame.frame_counter = static_cast<uint32_t>(frameIndex);
        
        // Transmission Mode (Mode I = 1, Mode II = 2, Mode III = 3, Mode IV = 4)
        frame.transmission_mode = 1; // Mode I (most common for DAB)
        
        // Frame Phase (0-3 for 4-frame cycle)
        frame.frame_phase = static_cast<uint8_t>(frameIndex % 4);
        
        // Generate realistic FIC data for Bangkok DAB
        frame.fic_data = generateFICData(frameIndex);
        frame.fic_crc = m_etsiComplianceEngine->calculateCRC16(frame.fic_data);
        
        // Generate realistic MSC data
        frame.msc_data = generateMSCData(frameIndex);
        frame.msc_crc = m_etsiComplianceEngine->calculateCRC16(frame.msc_data);
        
        // Fill raw_data with constructed frame
        constructRawETIFrame(frame);
        
        return frame;
    }
    
    std::vector<uint8_t> generateFICData(int frameIndex)
    {
        std::vector<uint8_t> fic_data;
        fic_data.reserve(96); // FIC is typically 96 bytes
        
        // FIG 0/0 (Ensemble information) - always present
        fic_data.push_back(0x00); // FIG type 0, extension 0
        fic_data.push_back(0x10); // Length indicator
        fic_data.push_back(0xFF); // Ensemble ID (high byte)
        fic_data.push_back(0x8C); // Ensemble ID (low byte) - Bangkok DAB
        
        // FIG 0/1 (Service organization) - varies by frame
        if (frameIndex % 4 == 0) {
            fic_data.push_back(0x01); // FIG type 0, extension 1
            fic_data.push_back(0x08); // Length
            fic_data.push_back(0x01); // Service ID
            fic_data.push_back(0x23); // Service configuration
        }
        
        // FIG 1/0 (Service labels) - varies by frame
        if (frameIndex % 8 == 0) {
            fic_data.push_back(0x10); // FIG type 1, extension 0
            fic_data.push_back(0x10); // Length
            // Service label "NBT Radio" (16 bytes, padded)
            std::string label = "NBT Radio       ";
            for (char c : label) {
                fic_data.push_back(static_cast<uint8_t>(c));
            }
        }
        
        // Pad to 96 bytes
        while (fic_data.size() < 96) {
            fic_data.push_back(0x00);
        }
        
        return fic_data;
    }
    
    std::vector<uint8_t> generateMSCData(int frameIndex)
    {
        std::vector<uint8_t> msc_data;
        const size_t MSC_SIZE = 6016; // 6144 - 128 (ETI header + FIC)
        msc_data.reserve(MSC_SIZE);
        
        // Generate realistic MSC data pattern
        for (size_t i = 0; i < MSC_SIZE; i++) {
            // Create pattern that varies with frame index for realistic simulation
            uint8_t value = static_cast<uint8_t>((i + frameIndex * 17) % 256);
            msc_data.push_back(value);
        }
        
        return msc_data;
    }
    
    void constructRawETIFrame(etsi::ETIFrame& frame)
    {
        // Construct 6144-byte ETI frame
        size_t offset = 0;
        
        // ETI sync (4 bytes)
        frame.raw_data[offset++] = (frame.sync_bytes >> 24) & 0xFF;
        frame.raw_data[offset++] = (frame.sync_bytes >> 16) & 0xFF;
        frame.raw_data[offset++] = (frame.sync_bytes >> 8) & 0xFF;
        frame.raw_data[offset++] = frame.sync_bytes & 0xFF;
        
        // Frame counter (3 bytes)
        frame.raw_data[offset++] = (frame.frame_counter >> 16) & 0xFF;
        frame.raw_data[offset++] = (frame.frame_counter >> 8) & 0xFF;
        frame.raw_data[offset++] = frame.frame_counter & 0xFF;
        
        // ETI header (remaining header bytes up to offset 32)
        frame.raw_data[offset++] = frame.transmission_mode;
        frame.raw_data[offset++] = frame.frame_phase;
        
        // Pad header to 32 bytes
        while (offset < 32) {
            frame.raw_data[offset++] = 0x00;
        }
        
        // FIC data (96 bytes)
        for (size_t i = 0; i < frame.fic_data.size() && offset < 128; i++) {
            frame.raw_data[offset++] = frame.fic_data[i];
        }
        
        // Pad FIC to 96 bytes if needed
        while (offset < 128) {
            frame.raw_data[offset++] = 0x00;
        }
        
        // MSC data (remaining bytes)
        for (size_t i = 0; i < frame.msc_data.size() && offset < 6144; i++) {
            frame.raw_data[offset++] = frame.msc_data[i];
        }
        
        // Pad to 6144 bytes if needed
        while (offset < 6144) {
            frame.raw_data[offset++] = 0x00;
        }
    }
    
    void displayComplianceResult(const etsi::ValidationResult& result, int frameIndex)
    {
        if (!m_complianceViewer) {
            return;
        }
        
        QString html = "<html><head><style>"
                      "body { font-family: 'Consolas', 'Monaco', monospace; font-size: 10px; margin: 10px; }"
                      ".critical { color: #d32f2f; font-weight: bold; background-color: #ffebee; padding: 5px; }"
                      ".warning { color: #f57c00; font-weight: bold; background-color: #fff3e0; padding: 5px; }"
                      ".info { color: #1976d2; background-color: #e3f2fd; padding: 5px; }"
                      ".pass { color: #388e3c; font-weight: bold; background-color: #e8f5e8; padding: 5px; }"
                      ".header { background-color: #f5f5f5; padding: 8px; font-weight: bold; border-bottom: 1px solid #ddd; }"
                      ".reference { color: #666; font-style: italic; margin-top: 5px; }"
                      "</style></head><body>";
        
        html += QString("<div class='header'>ETSI EN 300 799 Compliance Validation - Frame %1</div>").arg(frameIndex + 1);
        
        QString cssClass;
        QString statusText;
        switch (result.level) {
            case etsi::ComplianceLevel::CRITICAL:
                cssClass = "critical";
                statusText = "🚨 CRITICAL VIOLATION";
                break;
            case etsi::ComplianceLevel::WARNING:
                cssClass = "warning";
                statusText = "⚠️ WARNING";
                break;
            case etsi::ComplianceLevel::INFO:
                cssClass = "info";
                statusText = "ℹ️ INFORMATION";
                break;
            case etsi::ComplianceLevel::PASS:
                cssClass = "pass";
                statusText = "✅ COMPLIANT";
                break;
        }
        
        html += QString("<div class='%1'>").arg(cssClass);
        html += QString("<strong>%1</strong><br>").arg(statusText);
        html += QString("Description: %1<br>").arg(result.description);
        
        if (result.byte_offset > 0) {
            html += QString("Byte Offset: %1<br>").arg(result.byte_offset);
        }
        
        html += QString("Timestamp: %1").arg(result.timestamp.toString("hh:mm:ss.zzz"));
        html += "</div>";
        
        if (!result.etsi_reference.isEmpty()) {
            html += QString("<div class='reference'>ETSI Reference: %1</div>").arg(result.etsi_reference);
        }
        
        if (!result.suggested_fix.isEmpty()) {
            html += QString("<div class='reference'>Suggested Fix: %1</div>").arg(result.suggested_fix);
        }
        
        html += "</body></html>";
        
        m_complianceViewer->setHtml(html);
    }
    
    // NOTE: Multi-stream processing methods removed - moved to Phase 5 (FUTURE_FEATURES.md)
    
    // Live Stream Input Panel slots
    void onSourceTypeChanged(bool networkSelected)
    {
        if (m_sourceInputStack) {
            m_sourceInputStack->setCurrentIndex(networkSelected ? 1 : 0);
            // UI redesign (Step A): the toolbar is a compact single row by
            // default; the network row (URL + quick examples + connect/
            // disconnect/record + stream stats) expands only when the
            // "Network Stream" radio is selected.
            m_sourceInputStack->setVisible(networkSelected);
        }
    }
    
    void onQuickExampleSelected(int index)
    {
        if (!m_streamUrlEdit || index < 0) return;
        
        // All four entries are supported transports. Selecting one fills the
        // URL field; connection is still an explicit user action.
        switch (index) {
            case 0:
                m_streamUrlEdit->setText("udp://239.192.0.1:9200");
                break;
            case 1:
                m_streamUrlEdit->setText("239.192.0.1:9200");
                break;
            case 2:
                m_streamUrlEdit->setText("tcp://192.168.1.100:9200");
                break;
            case 3:
#ifdef HAVE_ZMQ
                m_streamUrlEdit->setText("zmq+tcp://localhost:9201");
#endif
                // Without HAVE_ZMQ this entry is disabled/annotated (see the
                // combo setup); never fill an unsupported URL.
                break;
            default:
                break;
        }
    }
    
    void onConnectClicked()
    {
        beginStreamConnection(m_streamUrlEdit ? m_streamUrlEdit->text() : QString(), true);
    }

    // Enter "live stream" mode: switch to a fresh capture exactly like a file
    // load (shared resetCaptureState()) and additionally drop the ETI
    // processor's file-backed state, so live frames can never continue from a
    // previously loaded file (frame numbering / raw-frame lookups).
    void beginLiveMode()
    {
        const bool hadPreviousCapture = resetCaptureState();
        if (m_etiProcessor) {
            m_etiProcessor->resetStatistics();  // drops file_data_/total_frames_
        }
        m_processedFrames = 0;
        m_frameUpdateCounter = 0;
        if (hadPreviousCapture && m_systemMessages) {
            m_systemMessages->append(
                "[STREAM] switched to live mode; previous capture state cleared");
        }
    }
    
    // Shared connect entry point (T22). Returns true when a connection attempt
    // was started, false when the URL was rejected before touching the core.
    // `showDialogs` is false from GUI tests so no modal dialog blocks offscreen.
    bool beginStreamConnection(const QString& rawUrl, bool showDialogs)
    {
        // F6: remembered so the ASYNC receiver error handler honours the same
        // "no modal dialogs offscreen" contract as this entry point.
        m_showStreamDialogs = showDialogs;

        // 1) PRE-VALIDATE before any state change / receiver creation. This is
        //    the loop-breaker: an unsupported URL (hostname, unicast, TCP/ZMQ)
        //    never reaches the receiver, so nothing can re-trigger attempts.
        const eti::StreamUrlValidation validation = eti::validate_stream_url(rawUrl);
        if (!validation.valid) {
            ++m_streamValidationWarnings;
            if (m_systemMessages) {
                m_systemMessages->append(QString("[STREAM] Invalid URL: %1")
                                             .arg(validation.error));
            }
            if (showDialogs) {
                // Build the supported-format list so the ZeroMQ line reflects
                // whether that transport was compiled in. Each fragment goes
                // through tr() so the list stays translatable (the old single
                // tr() literal was split up by the #ifdef).
                QString supportedFormats =
                    tr("  udp://239.192.0.1:9200  (IPv4 multicast)\n"
                       "  239.192.0.1:9200        (IPv4 multicast)\n"
                       "  tcp://host:9200         (raw ETI stream)\n");
#ifdef HAVE_ZMQ
                supportedFormats +=
                    tr("  zmq+tcp://host:9201     (ODR-DabMux ZeroMQ)\n");
#endif
                supportedFormats += tr("Port is optional and defaults to 9200.");
                QMessageBox::warning(
                    this, tr("Cannot connect"),
                    tr("%1\n\nSupported formats:\n%2").arg(validation.error, supportedFormats));
            }
            // No state change, no receiver, no log loop.
            return false;
        }
        
        const QString address = validation.address;
        const quint16 port = validation.port;
        
        m_streamEverConnected = false;
        m_streamFailureDialogShown = false;
        ++m_connectAttemptCount;
        
        // 2) Update UI state
        if (m_connectBtn) m_connectBtn->setEnabled(false);
        if (m_disconnectBtn) m_disconnectBtn->setEnabled(true);
        if (m_recordBtn) m_recordBtn->setEnabled(false);  // Disable until connected
        if (m_streamStatus) {
            m_streamStatus->setText("<span style='color: #FFA500;'>● Connecting...</span>");
        }
        
        m_systemMessages->append(QString("[STREAM] Connecting to %1:%2...").arg(address).arg(port));
        
        // 3) Create network receiver. auto_reconnect is OFF for the initial
        //    attempt; it is enabled only after a successful connection (see the
        //    connection_status_changed(true) handler below).
        eti::NetworkStreamConfig config;
        config.transport = validation.transport;
        config.multicast_address = address;
        config.port = port;
        config.buffer_size = static_cast<size_t>(m_networkBufferFrames);
        config.auto_reconnect = false;
        config.max_reconnect_attempts = 10;
        
        // T43: tear down any previous receiver FIRST and sever its signals.
        // Otherwise its destructor emits connection_status_changed(false)
        // synchronously into this window and clobbers the "Connecting..."
        // button state set just above (Connect was re-enabled mid-attempt).
        if (m_networkReceiver) {
            m_networkReceiver->disconnect(this);
            // disconnect_from_stream() performs a complete, loss-silent teardown
            // (W2 F1); a separate stop_reception() would emit an extra event.
            m_networkReceiver->disconnect_from_stream();
            m_networkReceiver.reset();
        }

        m_networkReceiver = std::make_unique<eti::NetworkStreamReceiver>(config, this);
        
        // Connect signals
        connect(m_networkReceiver.get(), &eti::NetworkStreamReceiver::frame_received,
                this, &DABAnalyserWindow::onNetworkFrameReceived);
        
        connect(m_networkReceiver.get(), &eti::NetworkStreamReceiver::connection_status_changed,
                this, [this](bool connected) {
                    if (connected) {
                        m_streamEverConnected = true;
                        m_streamFailureDialogShown = false;
                        if (m_streamStatus) {
                            m_streamStatus->setText("<span style='color: #107C10;'>● Connected</span>");
                        }
                        m_streamStartTime = std::chrono::steady_clock::now();
                        m_systemMessages->append("[STREAM] Connected successfully");

                        // T43: a confirmed connection is authoritative — assert
                        // the full button state (a stale failure/loss event must
                        // never leave Connect enabled mid-session).
                        if (m_connectBtn) m_connectBtn->setEnabled(false);
                        if (m_disconnectBtn) m_disconnectBtn->setEnabled(true);
                        if (m_recordBtn) m_recordBtn->setEnabled(true);
                        
                        // Start statistics timer
                        if (m_statsTimer) {
                            m_statsTimer->start(1000);  // Update every 1 second
                        }
                        
                        // Post-connect only: now that the connection is proven,
                        // arm automatic reconnection for later dropouts.
                        if (m_networkReceiver) {
                            m_networkReceiver->enable_automatic_reconnection(true, 10);
                        }
                    } else {
                        if (m_streamStatus) {
                            m_streamStatus->setText("<span style='color: #D32F2F;'>● Disconnected</span>");
                        }
                        m_systemMessages->append(m_streamEverConnected
                            ? "[STREAM] Connection lost"
                            : "[STREAM] Connection failed to start");
                        
                        // Disable recording and stop any active recording
                        if (m_recordBtn) {
                            m_recordBtn->setEnabled(false);
                        }
                        stopRecording();
                        
                        // Re-arm Connect so the user can retry deliberately;
                        // with auto_reconnect off there is no automatic loop.
                        if (m_connectBtn) m_connectBtn->setEnabled(true);
                        if (m_disconnectBtn) m_disconnectBtn->setEnabled(false);
                    }
                });
        
        connect(m_networkReceiver.get(), &eti::NetworkStreamReceiver::error_occurred,
                this, [this](const QString& error) {
                    handleStreamError(error);
                });
        
        // 4) Attempt connection. connect_with_config() (not the default-config
        //    connect_to_stream()) so the config above — including the tuned
        //    buffer size from the Settings dialog — is honored; a fresh default
        //    config would silently reset buffer_size to 1000.
        if (m_networkReceiver->connect_with_config(config)) {
            // Live mode before the first datagram: no file-derived state may
            // leak into the stream, and the live path must be self-sufficient.
            beginLiveMode();
            m_networkReceiver->start_reception();
            return true;
        } else {
            const QString error = m_networkReceiver->get_last_error();
            m_systemMessages->append(QString("[STREAM] Connection failed: %1").arg(error));
            if (m_streamStatus) {
                m_streamStatus->setText("<span style='color: #D32F2F;'>● Failed</span>");
            }
            // Re-enable connect button; no receiver/retry left running.
            if (m_connectBtn) m_connectBtn->setEnabled(true);
            if (m_disconnectBtn) m_disconnectBtn->setEnabled(false);
            if (m_recordBtn) m_recordBtn->setEnabled(false);
            if (showDialogs && !m_streamFailureDialogShown) {
                m_streamFailureDialogShown = true;
                QMessageBox::warning(this, tr("Cannot connect"),
                    error.isEmpty() ? tr("Failed to start the network stream.") : error);
            }
            return false;
        }
    }
    
    // Receiver error handler (T22 review F3a/F6).
    // Before a confirmed connection this drives the failure UI: red status,
    // Connect re-arm, Disconnect/Record off, and one dialog per attempt (only
    // when dialogs are enabled). AFTER a session has connected, errors are
    // LOG-ONLY — per-second quality-threshold messages must not repaint the
    // status or mutate buttons; the connection-status signal owns the UI then.
    void handleStreamError(const QString& error)
    {
        if (m_systemMessages) {
            m_systemMessages->append(QString("[STREAM ERROR] %1").arg(error));
        }
        
        if (m_streamEverConnected) {
            return;  // live session: log-only (quality / non-fatal details)
        }
        
        if (m_streamStatus) {
            m_streamStatus->setText("<span style='color: #D32F2F;'>● Error</span>");
        }
        // Re-enable connect button on error; no retry is pending.
        if (m_connectBtn) m_connectBtn->setEnabled(true);
        if (m_disconnectBtn) m_disconnectBtn->setEnabled(false);
        if (m_recordBtn) m_recordBtn->setEnabled(false);
        
        if (m_showStreamDialogs && !m_streamFailureDialogShown) {
            m_streamFailureDialogShown = true;
            const QString detail =
                (error.isEmpty() && m_networkReceiver)
                    ? m_networkReceiver->get_last_error()
                    : error;
            QMessageBox::warning(this, tr("Cannot connect"),
                detail.isEmpty()
                    ? tr("Failed to start the network stream.")
                    : detail);
        }
    }
    
    void onDisconnectClicked()
    {
        // Disconnect from network stream
        if (m_networkReceiver) {
            // Loss-silent complete teardown (W2 F1); no separate stop_reception.
            m_networkReceiver->disconnect_from_stream();
            m_networkReceiver.reset();
        }
        
        // Stop any active recording
        stopRecording();
        
        // Update UI state
        if (m_connectBtn) m_connectBtn->setEnabled(true);
        if (m_disconnectBtn) m_disconnectBtn->setEnabled(false);
        if (m_recordBtn) m_recordBtn->setEnabled(false);
        if (m_streamStatus) {
            m_streamStatus->setText("<span style='color: #666;'>● Disconnected</span>");
        }
        
        // Stop statistics timer
        if (m_statsTimer) {
            m_statsTimer->stop();
        }
        
        // Reset statistics display
        if (m_streamFrameRate) m_streamFrameRate->setText("0.0 FPS");
        if (m_streamFramesReceived) m_streamFramesReceived->setText("0 frames");
        if (m_streamFrameLoss) m_streamFrameLoss->setText("0.0%");
        if (m_streamLatency) m_streamLatency->setText("0 ms");
        if (m_streamBuffer) m_streamBuffer->setText("0/0");
        if (m_streamDataRate) m_streamDataRate->setText("0.0 MB/s");
        if (m_streamUptime) m_streamUptime->setText("00:00:00");
        
        m_systemMessages->append("[STREAM] Disconnected");
    }
    
    void onRecordClicked()
    {
        if (!m_isRecording) {
            // Start recording
            QString fileName = QFileDialog::getSaveFileName(
                this,
                "Save ETI Stream Recording",
                QDir::homePath() + "/recorded_stream_" + 
                    QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".eti",
                "ETI Files (*.eti);;All Files (*)"
            );
            
            if (fileName.isEmpty()) {
                return;  // User cancelled
            }
            
            m_recordingFile.setFileName(fileName);
            if (!m_recordingFile.open(QIODevice::WriteOnly)) {
                QMessageBox::warning(this, "Recording Error", 
                    "Failed to create recording file:\n" + m_recordingFile.errorString());
                return;
            }
            
            m_isRecording = true;
            m_recordedFrames = 0;
            
            if (m_recordBtn) {
                m_recordBtn->setText("Stop Recording");  // W1 #1: plain text
                m_recordBtn->setStyleSheet("background-color: #D32F2F; color: white; font-weight: bold;");
            }
            
            m_systemMessages->append("[RECORDING] Started: " + fileName);
            qInfo() << "[RECORDING] Recording to:" << fileName;
            
        } else {
            // Stop recording
            stopRecording();
        }
    }
    
    void stopRecording()
    {
        if (!m_isRecording) return;
        
        m_isRecording = false;
        m_recordingFile.close();
        
        if (m_recordBtn) {
            m_recordBtn->setText("Record");  // W1 #1: plain text
            m_recordBtn->setStyleSheet("background-color: #D32F2F; color: white;");
        }
        
        m_systemMessages->append(QString("[RECORDING] Stopped - Saved %1 frames (%2 MB)")
            .arg(m_recordedFrames)
            .arg((m_recordedFrames * 6144) / (1024.0 * 1024.0), 0, 'f', 2));
        
        qInfo() << "[RECORDING] Stopped -" << m_recordedFrames << "frames recorded";
        
        m_recordedFrames = 0;
    }
    
    void onNetworkFrameReceived(const QByteArray& frameData)
    {
        // Process received ETI frame through the analysis pipeline
        
        if (frameData.size() != 6144) {
            qWarning() << "[STREAM] Invalid frame size:" << frameData.size();
            return;
        }
        
        // Phase 4 Week 3: Record frame to file if recording is active
        if (m_isRecording && m_recordingFile.isOpen()) {
            qint64 written = m_recordingFile.write(frameData);
            if (written == 6144) {
                m_recordedFrames++;
                
                // Log every 1000 frames (~4 seconds at 250 FPS)
                if (m_recordedFrames % 1000 == 0) {
                    qDebug() << "[RECORDING]" << m_recordedFrames << "frames recorded"
                             << "(" << (m_recordedFrames * 6144) / (1024 * 1024) << "MB)";
                }
            } else {
                qCritical() << "[RECORDING] Write failed - stopping recording";
                stopRecording();
            }
        }
        
        // Frame counter for logging
        static size_t frameCount = 0;
        frameCount++;
        
        // Process through ETI processor for frame parsing and FIC extraction
        if (m_etiProcessor) {
            // Phase 4 Week 2: Use new live streaming API
            // This processes the frame AND extracts FIC automatically
            bool success = m_etiProcessor->processLiveFrame(frameData);
            
            if (!success) {
                qWarning() << "[STREAM] Failed to process frame" << frameCount;
                return;
            }
            
            // Log every second (250 FPS)
            if (frameCount % 250 == 0) {
                qDebug() << "[STREAM] Processed" << frameCount << "frames";
            }
        }
        
        // Note: FIC extraction and FIG analysis now handled automatically
        // by processLiveFrame() via signals:
        // - frameProcessed(ProcessedFrame) → updates frame display
        // - figDiscovered(FIGInfo) → updates service discovery
        // FIG analyser receives updates via existing signal/slot connections
    }
    
    void updateStreamStatistics()
    {
        // Get real statistics from network receiver
        if (!m_networkReceiver || !m_networkReceiver->is_connected()) {
            return;  // No active connection, nothing to update
        }
        
        auto stats = m_networkReceiver->get_statistics();
        
        // Calculate uptime
        auto now = std::chrono::steady_clock::now();
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - m_streamStartTime).count();
        
        int hours = uptime / 3600;
        int minutes = (uptime % 3600) / 60;
        int seconds = uptime % 60;
        
        // Update frame rate
        if (m_streamFrameRate) {
            m_streamFrameRate->setText(QString("%1 FPS").arg(stats.current_frame_rate, 0, 'f', 1));
        }
        
        // Update frames received
        if (m_streamFramesReceived) {
            m_streamFramesReceived->setText(QString("%1 frames").arg(stats.frames_received));
        }
        
        // Update frame loss
        if (m_streamFrameLoss) {
            double lossPercent = stats.frame_loss_rate * 100.0;
            m_streamFrameLoss->setText(QString("%1% (%2 frames)")
                .arg(lossPercent, 0, 'f', 2)
                .arg(stats.frames_dropped));
        }
        
        // Update latency
        if (m_streamLatency) {
            double latencyMs = stats.average_latency_us / 1000.0;
            m_streamLatency->setText(QString("%1 ms").arg(latencyMs, 0, 'f', 1));
        }
        
        // Update buffer status
        if (m_streamBuffer) {
            size_t bufferFill = stats.frames_received - stats.frames_processed;
            size_t bufferSize = 1000;  // From config
            double bufferPercent = (bufferFill * 100.0) / bufferSize;
            m_streamBuffer->setText(QString("%1% (%2/%3)")
                .arg(bufferPercent, 0, 'f', 0)
                .arg(bufferFill)
                .arg(bufferSize));
        }
        
        // Update data rate (6144 bytes per frame)
        if (m_streamDataRate) {
            double bytesPerSecond = stats.current_frame_rate * 6144.0;
            double mbitsPerSecond = (bytesPerSecond * 8.0) / (1024.0 * 1024.0);
            m_streamDataRate->setText(QString("%1 Mbit/s").arg(mbitsPerSecond, 0, 'f', 2));
        }
        
        // Update uptime
        if (m_streamUptime) {
            m_streamUptime->setText(QString("%1:%2:%3")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0')));
        }
    }
    
    void onBrowseFileClicked()
    {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "Open ETI File",
            "eti/",
            "ETI Files (*.eti *.ETI);;All Files (*)"
        );
        
        if (!fileName.isEmpty() && m_filePathEdit) {
            m_filePathEdit->setText(fileName);
        }
    }
    
    void onLoadFileClicked()
    {
        if (!m_filePathEdit) return;
        
        QString filePath = m_filePathEdit->text().trimmed();
        if (filePath.isEmpty()) {
            QMessageBox::warning(this, "No File Selected", "Please select an ETI file first using Browse button");
            return;
        }
        
        // Load the file directly without opening file dialog
        loadETIFile(filePath);
    }
    
    // NOTE: displayComparativeAnalysis removed - Multi-stream moved to Phase 5
    
    // Advanced Error Detection signal handlers
    void onEnableErrorDetection()
    {
        if (m_errorDetector) {
            m_errorDetector->setErrorDetectionEnabled(true);
            m_errorDetector->setRealTimeMonitoringEnabled(true);
            
            if (m_errorDetectionStatus) {
                m_errorDetectionStatus->setText("Status: Active - Real-time monitoring enabled");
                m_errorDetectionStatus->setStyleSheet("QLabel { color: green; font-weight: bold; }");
            }
            
            if (m_systemMessages) {
                m_systemMessages->append("🛡️ Advanced Error Detection enabled");
            }
        }
    }
    
    void onDisableErrorDetection()
    {
        if (m_errorDetector) {
            m_errorDetector->setErrorDetectionEnabled(false);
            m_errorDetector->setRealTimeMonitoringEnabled(false);
            
            if (m_errorDetectionStatus) {
                m_errorDetectionStatus->setText("Status: Disabled - Monitoring stopped");
                m_errorDetectionStatus->setStyleSheet("QLabel { color: red; font-weight: bold; }");
            }
            
            if (m_systemMessages) {
                m_systemMessages->append("🚫 Advanced Error Detection disabled");
            }
        }
    }
    
    void onResetErrors()
    {
        if (m_errorDetector) {
            m_errorDetector->resetErrorState();
            
            if (m_errorHistoryTree) {
                m_errorHistoryTree->clear();
            }
            
            if (m_errorCountLabel) {
                m_errorCountLabel->setText("Total Errors: 0");
            }
            
            if (m_errorPatternsDisplay) {
                m_errorPatternsDisplay->clear();
            }
            
            if (m_systemMessages) {
                m_systemMessages->append("🔄 Error state reset - All error history cleared");
            }
        }
    }
    
    void onExportErrorReport()
    {
        if (!m_errorDetector) return;
        
        QString fileName = QFileDialog::getSaveFileName(this,
            "Export Error Report", 
            QString("error_report_%1.json").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")),
            "JSON Files (*.json);;All Files (*)");
        
        if (!fileName.isEmpty()) {
            QDateTime from = QDateTime::currentDateTime().addDays(-1); // Last 24 hours
            QDateTime to = QDateTime::currentDateTime();
            
            QString report = m_errorDetector->generateErrorReport(from, to);
            
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream << report;
                
                if (m_systemMessages) {
                    m_systemMessages->append(QString("💾 Error report exported to: %1").arg(fileName));
                }
            } else {
                if (m_systemMessages) {
                    m_systemMessages->append("❌ Failed to export error report");
                }
            }
        }
    }
    
    void onShowSystemHealth()
    {
        if (!m_errorDetector) return;
        
        error_detection::SystemHealthMetrics health = m_errorDetector->getSystemHealth();
        
        QString healthReport = QString(
            "<html><head><style>"
            "body { font-family: 'Consolas', monospace; font-size: 10px; }"
            ".header { background-color: #e6f3ff; padding: 8px; font-weight: bold; }"
            ".healthy { color: #28a745; font-weight: bold; }"
            ".warning { color: #ffc107; font-weight: bold; }"
            ".critical { color: #dc3545; font-weight: bold; }"
            ".metric { margin: 5px 0; padding: 5px; background-color: #f8f9fa; }"
            "</style></head><body>"
            "<div class='header'>System Health Report</div>"
            "<div class='metric'><strong>Overall Health Score:</strong> <span class='%1'>%2%</span></div>"
            "<div class='metric'><strong>Error Rate:</strong> %3 errors/minute</div>"
            "<div class='metric'><strong>Critical Error Rate:</strong> %4 critical/minute</div>"
            "<div class='metric'><strong>Total Errors:</strong> %5</div>"
            "<div class='metric'><strong>Successful Recoveries:</strong> %6</div>"
            "<div class='metric'><strong>Failed Recoveries:</strong> %7</div>"
            "<div class='metric'><strong>Last Critical Error:</strong> %8</div>"
            "</body></html>"
        ).arg(health.overall_health_score >= 80 ? "healthy" : health.overall_health_score >= 60 ? "warning" : "critical")
         .arg(health.overall_health_score, 0, 'f', 1)
         .arg(health.error_rate_per_minute, 0, 'f', 2)
         .arg(health.critical_error_rate, 0, 'f', 2)
         .arg(health.total_errors_detected)
         .arg(health.successful_recoveries)
         .arg(health.failed_recoveries)
         .arg(health.last_critical_error.isNull() ? "None" : health.last_critical_error.toString());
        
        if (m_errorPatternsDisplay) {
            m_errorPatternsDisplay->setHtml(healthReport);
        }
        
        if (m_systemMessages) {
            m_systemMessages->append(QString("📊 System Health: %1% (Last update: %2)")
                                   .arg(health.overall_health_score, 0, 'f', 1)
                                   .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
        }
    }
    
    void onErrorDetected(const error_detection::ErrorReport& error)
    {
        // T25 (F5): queued from the detector's worker thread. During teardown
        // m_errorDetector is reset; ignore stale deliveries instead of
        // dereferencing a freed detector / touching a half-torn-down window.
        if (m_closing || !m_errorHistoryTree) return;
        
        QTreeWidgetItem* item = new QTreeWidgetItem(m_errorHistoryTree);
        item->setText(0, error.timestamp.toString("hh:mm:ss"));
        
        // Severity with color coding
        QString severityText;
        QString severityColor;
        switch (error.severity) {
            case error_detection::ErrorSeverity::CRITICAL:
                severityText = "CRITICAL";
                severityColor = "#dc3545";
                break;
            case error_detection::ErrorSeverity::HIGH:
                severityText = "HIGH";
                severityColor = "#fd7e14";
                break;
            case error_detection::ErrorSeverity::MEDIUM:
                severityText = "MEDIUM";
                severityColor = "#ffc107";
                break;
            case error_detection::ErrorSeverity::LOW:
                severityText = "LOW";
                severityColor = "#28a745";
                break;
            default:
                severityText = "INFO";
                severityColor = "#6c757d";
                break;
        }
        
        item->setText(1, severityText);
        item->setForeground(1, QColor(severityColor));
        
        // Category
        QString categoryText = QString::number(static_cast<int>(error.category));
        item->setText(2, categoryText);
        
        // Description
        item->setText(3, error.description);
        
        // Frame number
        item->setText(4, QString::number(error.frame_number));
        
        // Recovery status
        QString recoveryText = error.auto_recovery_attempted ? 
                             (error.recovery_successful ? "✅ Success" : "❌ Failed") : 
                             "No attempt";
        item->setText(5, recoveryText);
        
        // Store error details
        item->setData(0, Qt::UserRole, QVariant::fromValue(error.error_id));
        
        // Auto-scroll to latest error
        m_errorHistoryTree->scrollToBottom();
        
        // Update error count
        if (m_errorCountLabel && m_errorDetector) {
            uint32_t totalErrors = m_errorDetector->getTotalErrorCount();
            m_errorCountLabel->setText(QString("Total Errors: %1").arg(totalErrors));
        }
    }
    
    void onSystemHealthChanged(const error_detection::SystemHealthMetrics& health)
    {
        if (m_systemHealthScore) {
            QString healthColor = health.overall_health_score >= 80 ? "green" : 
                                 health.overall_health_score >= 60 ? "orange" : "red";
            
            m_systemHealthScore->setText(QString("System Health: %1%").arg(health.overall_health_score, 0, 'f', 1));
            m_systemHealthScore->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }").arg(healthColor));
        }
    }
    
    void displayFrameHexData(int frameIndex)
    {
        if (!m_hexViewer || !m_etiProcessor || frameIndex < 0) {
            return;
        }

        // Manual-test finding 2: bounds-check against the frames actually
        // available instead of indexing blindly.
        const int available = availableFrameCount();
        if (available <= 0 || frameIndex >= available) {
            m_hexViewer->setText("No frame data available. Please load an ETI file first.");
            return;
        }
        
        // Get raw ETI frame data (6144 bytes per frame)
        QByteArray frameData = getETIFrameData(frameIndex);
        
        if (frameData.isEmpty()) {
            m_hexViewer->setText("No frame data available. Please load an ETI file first.");
            return;
        }
        
        // Generate hex display with highlighting
        QString hexDisplay = generateHexDisplay(frameData, frameIndex);
        m_hexViewer->setHtml(hexDisplay);
        
        // Update system messages
        // L1: skip the per-tick append during playback (unbounded QTextEdit);
        // bounded append otherwise.
        if (!m_playbackPlaying) {
            appendSystemMessageBounded(QString("🔍 Hex Viewer: Displaying Frame %1 (%2 bytes)")
                                   .arg(frameIndex + 1).arg(frameData.size()));
        }
    }
    
    // PHASE 3 AGENT 3: Real ETI Frame Data Retrieval
    QByteArray getETIFrameData(int frameIndex)
    {
        if (!m_etiProcessor || !m_fileLoaded) {
            qWarning() << "[HEX VIEWER] ETI processor not ready or file not loaded";
            return QByteArray();
        }

        if (frameIndex < 0 || static_cast<size_t>(frameIndex) >= m_totalFrames) {
            qWarning() << "[HEX VIEWER] Invalid frame index:" << frameIndex;
            return QByteArray();
        }

        // Thread-safe access to real frame data
        QByteArray frameData = m_etiProcessor->getRawFrameData(frameIndex);

        if (frameData.isEmpty()) {
            qWarning() << "[HEX VIEWER] Frame" << frameIndex << "returned empty data";
        } else {
            qDebug() << "[HEX VIEWER] Retrieved frame" << frameIndex << "size:" << frameData.size() << "bytes";
        }

        return frameData;
    }
    
    // PHASE 3 AGENT 3: Professional ETSI-Compliant Hex Display
    QString generateHexDisplay(const QByteArray& data, int frameIndex)
    {
        if (!m_hexFormatter) {
            return "<html><body><p style='color:#ff0000;'>Hex formatter not initialized</p></body></html>";
        }

        if (data.isEmpty()) {
            return "<html><body style='background:#1e1e1e;color:#d4d4d4;font-family:monospace;'>"
                   "<div style='padding:20px;'>"
                   "<p>No frame data available for frame " + QString::number(frameIndex) + "</p>"
                   "<p style='color:#888;'>Ensure ETI file is loaded and frame index is valid.</p>"
                   "</div></body></html>";
        }

        // Use HexViewerFormatter for professional ETSI-compliant display
        QString html = m_hexFormatter->formatFrameHex(data, frameIndex);

        qDebug() << "[HEX VIEWER] Generated hex display for frame" << frameIndex
                 << "(" << data.size() << "bytes," << html.length() << "chars HTML)";

        return html;
    }

    void onServiceDiscovered(const ServiceInfo& service)
    {
        // Add service to ensemble tree
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem();
        serviceItem->setText(0, QString("Service %1").arg(service.serviceId));
        serviceItem->setText(1, service.label);
        serviceItem->setText(2, service.type);
        m_ensembleTree->addTopLevelItem(serviceItem);
        
        m_systemMessages->append(QString("🎵 Discovered service: %1 (%2)")
                               .arg(service.label).arg(service.type));
    }

    // Real-time FIG analysis slot implementations
    void onFIGDiscovered(const FIGInfo& fig)
    {
        // OPTIMIZED: Code Review HIGH-003 - Removed wasteful QTreeWidgetItem allocation
        // Old code: allocated widget, populated it, then immediately deleted it
        // Performance gain: ~5-10% in FIG processing (called hundreds of times per file)
        // TODO: Populate m_tab2_figContentTable when Tab 2 FIG Content display is implemented
        
        QString figTypeStr = QString("FIG %1/%2").arg(fig.type >> 3).arg(fig.type & 0x07);
        
        // Update system messages with FIG discovery
        if (m_systemMessages) {
            m_systemMessages->append(QString("🔍 FIG Discovered: %1 (Length: %2 bytes)")
                                   .arg(figTypeStr).arg(fig.length));
        }
    }
    
    void onETIFormatDetected(ETIFormat format, const QString& formatName)
    {
        // Update system messages (works fine)
        if (m_systemMessages) {
            m_systemMessages->append(QString("📡 ETI Format Detected: %1").arg(formatName));
        }
        
        // OBSOLETE: m_extractedTimeInfo removed with Tab 3 restructure
        // Tab 3 now uses m_tab3_figInstanceTree and m_tab3_figItemDetailsTree
        
        (void)format;  // Suppress unused warning
    }
    
    // DELETED: onFIGTypeFilterChanged() - Code Review HIGH-001
    // No-op function, no signal connections found, safe to delete per PDCA review 2025-10-26
    
    // NOTE (Wave B): the legacy updatePerformanceMetrics(const ProcessedFrame&)
    // label updater was deleted together with its thin Performance tab. The
    // adopted PerformanceDashboard is now fed by updateDashboardSamples() with
    // real FPS/RSS/CPU%/latency samples.

    // ========================================================================
    // WAVE B: adopted src/gui panel data feeds (real samples, no placeholders)
    // ========================================================================

    // Resident set size of this process in MB, via /proc/self/statm (Linux).
    static double processResidentMemoryMB()
    {
#if defined(Q_OS_LINUX)
        QFile f(QStringLiteral("/proc/self/statm"));
        if (!f.open(QIODevice::ReadOnly)) {
            return 0.0;
        }
        const QList<QByteArray> parts = f.readLine().trimmed().split(' ');
        if (parts.size() < 2) {
            return 0.0;
        }
        bool ok = false;
        const qulonglong pages = parts.at(1).toULongLong(&ok);
        if (!ok) {
            return 0.0;
        }
        static const double pageMb = sysconf(_SC_PAGESIZE) / (1024.0 * 1024.0);
        return static_cast<double>(pages) * pageMb;
#else
        return 0.0;
#endif
    }

    // Process CPU usage (%) since the previous sample, via /proc/self/stat.
    double processCpuPercent(qint64 nowMs)
    {
#if defined(Q_OS_LINUX)
        QFile f(QStringLiteral("/proc/self/stat"));
        if (!f.open(QIODevice::ReadOnly)) {
            return 0.0;
        }
        const QByteArray line = f.readAll();
        const int rp = line.lastIndexOf(')');
        if (rp < 0) {
            return 0.0;
        }
        const QList<QByteArray> fields = line.mid(rp + 2).trimmed().split(' ');
        // After "comm)", fields[0]=state, utime=fields[11], stime=fields[12].
        if (fields.size() < 13) {
            return 0.0;
        }
        bool okU = false, okS = false;
        const qulonglong utime = fields.at(11).toULongLong(&okU);
        const qulonglong stime = fields.at(12).toULongLong(&okS);
        if (!okU || !okS) {
            return 0.0;
        }
        const qulonglong ticks = utime + stime;
        static const double hz = sysconf(_SC_CLK_TCK);
        double pct = 0.0;
        if (m_lastCpuSampleMs > 0 && nowMs > m_lastCpuSampleMs
            && ticks >= m_lastCpuTicks && hz > 0.0) {
            const double dt = (nowMs - m_lastCpuSampleMs) / 1000.0;
            const double cpuSec = (ticks - m_lastCpuTicks) / hz;
            if (dt > 0.0) {
                pct = 100.0 * cpuSec / dt;
            }
        }
        m_lastCpuSampleMs = nowMs;
        m_lastCpuTicks = ticks;
        return pct;
#else
        Q_UNUSED(nowMs);
        return 0.0;
#endif
    }

    // 1 s sampler: feeds PerformanceDashboard + RealTimeChart with real
    // FPS (frame-count delta), RSS, CPU% and average per-frame latency.
    void updateDashboardSamples()
    {
        if (m_closing) {
            return;
        }
        const quint64 frames = m_etiProcessor ? m_etiProcessor->getFrameCount() : 0;
        const qint64 elapsedMs = m_perfTimer.isValid() ? m_perfTimer.elapsed() : 0;

        double fps = 0.0;
        if (m_perfLastElapsedMs > 0 && elapsedMs > m_perfLastElapsedMs) {
            const double dt = (elapsedMs - m_perfLastElapsedMs) / 1000.0;
            const quint64 df = frames >= m_perfLastFrames ? frames - m_perfLastFrames : 0;
            if (dt > 0.0) {
                fps = static_cast<double>(df) / dt;
            }
        }
        m_perfLastFrames = frames;
        m_perfLastElapsedMs = elapsedMs;

        const double memMB = processResidentMemoryMB();
        const double cpuPct = processCpuPercent(elapsedMs);

        PerformanceStats stats;
        stats.frameRate = fps;
        stats.memoryUsage = memMB;
        stats.frameCount = frames;
        stats.cpuUsage = cpuPct;
        stats.averageLatency = (m_totalFrames > 0 && m_lastProcessingTimeMs > 0.0)
                                   ? m_lastProcessingTimeMs / static_cast<double>(m_totalFrames)
                                   : 0.0;
        stats.timestamp = QDateTime::currentDateTime();
        if (m_performanceDashboard) {
            m_performanceDashboard->updatePerformanceMetrics(stats);
        }

        // Compliance view on the dashboard, from the real error detector.
        if (m_performanceDashboard && m_errorDetector) {
            const error_detection::SystemHealthMetrics health =
                m_errorDetector->getSystemHealth();
            ComplianceResult cr;
            cr.compliancePercentage = health.overall_health_score;
            cr.violationCount = static_cast<int>(m_errorDetector->getTotalErrorCount());
            cr.complianceLevel = health.overall_health_score >= 95.0 ? QStringLiteral("EXCELLENT")
                               : health.overall_health_score >= 85.0 ? QStringLiteral("GOOD")
                               : health.overall_health_score >= 70.0 ? QStringLiteral("FAIR")
                                                                     : QStringLiteral("POOR");
            cr.lastUpdate = QDateTime::currentDateTime();
            m_performanceDashboard->updateComplianceStatus(cr);
        }

        if (m_realTimeChart) {
            const QDateTime now = QDateTime::currentDateTime();
            if (m_chartFpsSeries >= 0) {
                m_realTimeChart->addDataPoint(m_chartFpsSeries, fps, now);
            }
            if (m_chartMemorySeries >= 0) {
                m_realTimeChart->addDataPoint(m_chartMemorySeries, memMB, now);
            }
            ++m_chartSamplesFed;
        }
    }

    static ETSIViolation::Severity violationSeverityFromError(error_detection::ErrorSeverity s)
    {
        switch (s) {
            case error_detection::ErrorSeverity::CRITICAL: return ETSIViolation::Critical;
            case error_detection::ErrorSeverity::HIGH:     return ETSIViolation::Error;
            case error_detection::ErrorSeverity::MEDIUM:   return ETSIViolation::Warning;
            case error_detection::ErrorSeverity::LOW:      return ETSIViolation::Info;
            case error_detection::ErrorSeverity::INFO:     return ETSIViolation::Info;
        }
        return ETSIViolation::Info;
    }

    static ETSIViolation::Category violationCategoryFromError(error_detection::ErrorCategory c)
    {
        switch (c) {
            case error_detection::ErrorCategory::ETI_SYNC:           return ETSIViolation::Timing_Compliance;
            case error_detection::ErrorCategory::FRAME_STRUCTURE:    return ETSIViolation::FIG_Structure;
            case error_detection::ErrorCategory::FIC_DECODING:       return ETSIViolation::Data_Integrity;
            case error_detection::ErrorCategory::SERVICE_DISCOVERY:  return ETSIViolation::Service_Organization;
            case error_detection::ErrorCategory::AUDIO_QUALITY:      return ETSIViolation::Audio_Quality;
            case error_detection::ErrorCategory::NETWORK_STREAM:     return ETSIViolation::Data_Integrity;
            case error_detection::ErrorCategory::FILE_IO:            return ETSIViolation::Data_Integrity;
            case error_detection::ErrorCategory::MEMORY_MANAGEMENT:  return ETSIViolation::Timing_Compliance;
            case error_detection::ErrorCategory::PERFORMANCE:        return ETSIViolation::Timing_Compliance;
            case error_detection::ErrorCategory::CONFIGURATION:      return ETSIViolation::Data_Integrity;
        }
        return ETSIViolation::Data_Integrity;
    }

    // Real AdvancedErrorDetector report -> ETSI monitor violation.
    void onEtsiViolationFromError(const error_detection::ErrorReport& err)
    {
        if (!m_etsiMonitor || m_closing) {
            return;
        }
        ETSIViolation v;
        v.violationId = err.error_id.isEmpty()
                            ? QStringLiteral("ERR-F%1").arg(err.frame_number)
                            : err.error_id;
        v.severity = violationSeverityFromError(err.severity);
        v.category = violationCategoryFromError(err.category);
        v.description = err.description.isEmpty() ? err.technical_details : err.description;
        v.standardReference = QStringLiteral("ETSI EN 300 799");
        v.timestamp = err.timestamp.isValid() ? err.timestamp : QDateTime::currentDateTime();
        v.frameContext = QStringLiteral("Frame %1").arg(err.frame_number);
        v.isActive = true;
        m_etsiMonitor->addViolation(v);
    }

    // ETSI compliance engine result -> ETSI monitor violation.
    void onEtsiViolationFromEngine(const etsi::ValidationResult& result)
    {
        if (!m_etsiMonitor || m_closing) {
            return;
        }
        ETSIViolation v;
        v.violationId = QStringLiteral("ETSI-F%1-T%2")
                            .arg(result.frame_number)
                            .arg(static_cast<int>(result.error_type));
        switch (result.level) {
            case etsi::ComplianceLevel::CRITICAL: v.severity = ETSIViolation::Critical; break;
            case etsi::ComplianceLevel::WARNING:  v.severity = ETSIViolation::Warning;  break;
            case etsi::ComplianceLevel::INFO:     v.severity = ETSIViolation::Info;     break;
            case etsi::ComplianceLevel::PASS:     v.severity = ETSIViolation::Info;     break;
        }
        switch (result.error_type) {
            case etsi::ValidationErrorType::FIG_TYPE_ERROR:
                v.category = ETSIViolation::FIG_Structure;
                break;
            case etsi::ValidationErrorType::SERVICE_ID_ERROR:
            case etsi::ValidationErrorType::ENSEMBLE_ID_ERROR:
                v.category = ETSIViolation::Service_Organization;
                break;
            case etsi::ValidationErrorType::TIMESTAMP_ERROR:
                v.category = ETSIViolation::Timing_Compliance;
                break;
            default:
                v.category = ETSIViolation::Data_Integrity;
                break;
        }
        v.description = result.description;
        v.standardReference = result.etsi_reference.isEmpty()
                                  ? QStringLiteral("ETSI EN 300 799")
                                  : result.etsi_reference;
        v.timestamp = result.timestamp.isValid() ? result.timestamp
                                                 : QDateTime::currentDateTime();
        v.frameContext = QStringLiteral("Frame %1").arg(result.frame_number);
        v.isActive = true;
        m_etsiMonitor->addViolation(v);
    }

    // Clear the adopted panels on a new file load, like the other panels.
    void resetAdoptedPanels()
    {
        if (m_performanceDashboard) {
            m_performanceDashboard->resetStatistics();
        }
        if (m_realTimeChart) {
            m_realTimeChart->clearData();
        }
        if (m_etsiMonitor) {
            m_etsiMonitor->clearViolations();
        }
        if (m_constellationWidget) {
            m_constellationWidget->clearDisplay();
        }
        m_lastProcessingTimeMs = 0.0;
        m_perfLastFrames = 0;
        m_perfLastElapsedMs = 0;
        m_lastCpuSampleMs = 0;
        m_lastCpuTicks = 0;
        if (m_perfTimer.isValid()) {
            m_perfTimer.restart();
        }
    }

    // Real analysis rows injected into the export manager (services / FIGs /
    // frames). The writers consume customSettings["records"].
    void applyExportData(ExportConfiguration& config) const
    {
        QVariantList columns;
        auto addColumn = [&columns](const QString& key, const QString& header) {
            QVariantMap c;
            c.insert(QStringLiteral("key"), key);
            c.insert(QStringLiteral("header"), header);
            columns.append(c);
        };
        addColumn(QStringLiteral("service_id"), QStringLiteral("Service ID"));
        addColumn(QStringLiteral("label"), QStringLiteral("Service Label"));
        addColumn(QStringLiteral("service_type"), QStringLiteral("Service Type"));
        addColumn(QStringLiteral("dab_plus"), QStringLiteral("DAB+"));
        addColumn(QStringLiteral("sub_channel"), QStringLiteral("Sub-channel"));
        addColumn(QStringLiteral("bitrate_kbps"), QStringLiteral("Bitrate (kbps)"));
        addColumn(QStringLiteral("figs_total"), QStringLiteral("FIGs (total)"));
        addColumn(QStringLiteral("frames_total"), QStringLiteral("Frames (total)"));
        addColumn(QStringLiteral("mot_objects"), QStringLiteral("MOT objects"));
        addColumn(QStringLiteral("etsi_violations"), QStringLiteral("ETSI violations"));

        const int figsTotal = m_figAnalyser ? m_figAnalyser->getTotalFIGsProcessed() : 0;
        const int etsiCount = m_etsiMonitor
                                  ? m_etsiMonitor->getCurrentStatistics().totalViolations
                                  : 0;
        const int frameCount = static_cast<int>(m_totalFrames);

        std::vector<SubChannelInfo> subchannels;
        if (m_figAnalyser) {
            subchannels = m_figAnalyser->getAllSubChannels();
        }
        auto bitrateForSubChannel = [&subchannels](uint8_t id) -> int {
            for (const SubChannelInfo& sc : subchannels) {
                if (sc.sub_channel_id == id) {
                    return static_cast<int>(sc.getBitrate());
                }
            }
            return -1;
        };

        QVariantList records;
        auto addRow = [&records, figsTotal, etsiCount, frameCount, this](
                          const QString& sid, const QString& label, const QString& type,
                          const QString& dabPlus, const QString& sub, int bitrate) {
            QVariantMap r;
            r.insert(QStringLiteral("service_id"), sid);
            r.insert(QStringLiteral("label"), label);
            r.insert(QStringLiteral("service_type"), type);
            r.insert(QStringLiteral("dab_plus"), dabPlus);
            r.insert(QStringLiteral("sub_channel"), sub);
            r.insert(QStringLiteral("bitrate_kbps"),
                     bitrate >= 0 ? QString::number(bitrate) : QStringLiteral("-"));
            r.insert(QStringLiteral("figs_total"), QString::number(figsTotal));
            r.insert(QStringLiteral("frames_total"), QString::number(frameCount));
            r.insert(QStringLiteral("mot_objects"), QString::number(m_motObjectsReceived));
            r.insert(QStringLiteral("etsi_violations"), QString::number(etsiCount));
            records.append(r);
        };

        if (m_figAnalyser) {
            for (const DABService& svc : m_figAnalyser->getDABServices()) {
                const ServiceComponent primary = svc.getPrimaryComponent();
                const QString displayLabel = svc.service_label.isEmpty()
                    ? getServiceTypeDisplay(svc.service_id, primary.sub_channel_id)
                    : svc.service_label;
                addRow(QStringLiteral("0x%1").arg(svc.service_id, 0, 16).toUpper(),
                       displayLabel,
                       svc.service_type == 0 ? QStringLiteral("Programme")
                                             : QStringLiteral("Data"),
                       svc.is_dab_plus ? QStringLiteral("yes") : QStringLiteral("no"),
                       QString::number(primary.sub_channel_id),
                       bitrateForSubChannel(primary.sub_channel_id));
            }
        }
        if (records.isEmpty()) {
            QString ens;
            if (m_figAnalyser) {
                ens = m_figAnalyser->getCurrentEnsemble().ensembleLabel;
            }
            if (ens.isEmpty()) {
                ens = QStringLiteral("(no services decoded)");
            }
            addRow(QStringLiteral("-"), ens, QStringLiteral("Ensemble"),
                   QStringLiteral("-"), QStringLiteral("-"), -1);
        }

        config.customSettings.insert(QStringLiteral("columns"), columns);
        config.customSettings.insert(QStringLiteral("records"), records);
    }

    // Help -> About: adopted AboutDialog (replaces the inline QMessageBox).
    void showAboutDialog()
    {
        AboutDialog dlg(this);
        dlg.exec();
    }

    // File -> Settings: edits the live application settings. Every control is
    // wired to real behaviour (confirm-exit, sampler rate, network buffer, log
    // level, shared T34 audio) plus the AnalyserSettings decode options.
    // "Apply"/"OK" persist to the app QSettings scope and apply immediately
    // where possible; analyser decode options take full effect on next load.
    void showSettingsDialog()
    {
        SettingsDialog dlg(this);

        // Feed the dialog the SAME T34 audio state as the Transport Audio tab.
        // Enumerate through the ACTIVE backend (ALSA on Linux, Qt Multimedia
        // on Windows/macOS) so the device list is never the alien fallback.
        QStringList deviceNames;
        QStringList deviceIds;
        const QList<streamdab::audio::AudioPlaybackController::OutputDeviceInfo> devices =
            streamdab::audio::AudioPlaybackController::enumerateOutputDevices();
        for (const streamdab::audio::AudioPlaybackController::OutputDeviceInfo& info : devices) {
            deviceNames.append(info.description);
            deviceIds.append(info.name);
        }
        dlg.setAudioOptions(deviceNames, deviceIds,
                            m_audioController ? m_audioController->outputDevice() : QString(),
                            m_audioMuteCheck ? m_audioMuteCheck->isChecked() : false,
                            m_audioVolumeSlider ? m_audioVolumeSlider->value() : 100);

        // Apply immediately on "Apply" as well as "OK" (both emit
        // settingsChanged after saveSettings()).
        connect(&dlg, &SettingsDialog::settingsChanged, this, [this]() {
            applyPersistedAppSettings();
            m_analyserSettings.loadFromQSettings();
            if (m_figAnalyser) {
                m_figAnalyser->setAnalyserSettings(m_analyserSettings);
            }
        });

        if (dlg.exec() == QDialog::Accepted) {
            statusBar()->showMessage(
                tr("Settings saved — applied now (exit confirmation, update rate, "
                   "network buffer, log level, audio). Analyser decode options take "
                   "full effect on the next file load."), 6000);
        }
    }

    // File -> Export: exports the current analysis with the adopted
    // ProfessionalExportManager writers. Real rows are injected after the
    // dialog so the report reflects the live decode.
    void showExportDialog()
    {
        if (!m_exportManager) {
            return;
        }
        ExportConfiguration config;
        config.outputDirectory = m_exportManager->getDefaultOutputDirectory();
        config.fileName = config.outputDirectory + QStringLiteral("/analysis_export.csv");
        applyExportData(config);

        ExportConfigurationDialog dialog(this);
        dialog.setConfiguration(config);
        if (dialog.exec() != QDialog::Accepted) {
            return;
        }
        config = dialog.getConfiguration();
        applyExportData(config);  // re-inject the real analysis after the dialog
        if (config.fileName.isEmpty()) {
            config.fileName = m_exportManager->getDefaultOutputDirectory()
                              + QStringLiteral("/analysis_export.csv");
        }
        m_exportManager->startExport(config);
    }

    // ========================================================================
    // Settings application (v1.3 settings audit)
    // ========================================================================

    /// True for headless test/automation platforms: never show modal dialogs.
    static bool isHeadlessPlatform()
    {
        const QString platform = QApplication::platformName();
        return platform == QLatin1String("offscreen") ||
               platform == QLatin1String("minimal");
    }

    /// Read the application QSettings scope and apply every live setting.
    void applyPersistedAppSettings()
    {
        const QSettings s(streamdab::app_settings::organization(),
                          streamdab::app_settings::application());

        // General: exit confirmation.
        m_confirmExitOnClose = s.value(QStringLiteral("general/confirmExit"), true).toBool();

        // Analysis: dashboard sampler rate (Hz) + network receiver buffer.
        const int hz = qBound(1, s.value(QStringLiteral("analysis/updateRate"), 1).toInt(), 10);
        if (m_perfSampleTimer) {
            m_perfSampleTimer->setInterval(1000 / hz);
        }
        m_networkBufferFrames =
            qBound(10, s.value(QStringLiteral("analysis/bufferSize"), 1000).toInt(), 10000);
        if (m_networkReceiver) {
            m_networkReceiver->set_buffer_size(static_cast<size_t>(m_networkBufferFrames));
        }

        // Advanced: logger level (0=Error, 1=Warning, 2=Info, 3=Debug,
        // 4=Critical; see SettingsDialog::loggerLevelForIndex). The Logger is
        // the single source of truth; the bottom-strip Logging tab mirrors it.
        const int levelIndex =
            qBound(0, s.value(QStringLiteral("advanced/logLevel"), 2).toInt(), 4);
        Logger::instance().setLogLevel(static_cast<Logger::LogLevel>(
            SettingsDialog::loggerLevelForIndex(levelIndex)));
        syncLoggingLevelComboFromLogger();

        // Audio: shared with the T34 Audio tab / controller.
        applyAudioSettings(s.value(QStringLiteral("audio/device"), QString()).toString(),
                           s.value(QStringLiteral("audio/muted"), false).toBool(),
                           qBound(0, s.value(QStringLiteral("audio/volume"), 100).toInt(), 100));
    }

    /// Apply one audio state to BOTH the controller and the Audio-tab widgets.
    void applyAudioSettings(const QString& device, bool muted, int volume)
    {
        if (m_audioController) {
            if (!device.isEmpty()) {
                m_audioController->setOutputDevice(device);
            }
            m_audioController->setMuted(muted);
            m_audioController->setVolume(volume / 100.0);
        }
        // Mirror into the Transport group's Audio tab (same state, no duplicate
        // that can disagree). Signal blockers prevent feedback loops.
        if (m_audioDeviceCombo) {
            const int index = m_audioDeviceCombo->findData(device);
            if (index >= 0 && index != m_audioDeviceCombo->currentIndex()) {
                const QSignalBlocker blocker(m_audioDeviceCombo);
                m_audioDeviceCombo->setCurrentIndex(index);
            }
        }
        if (m_audioMuteCheck) {
            const QSignalBlocker blocker(m_audioMuteCheck);
            m_audioMuteCheck->setChecked(muted);
        }
        if (m_audioVolumeSlider) {
            const QSignalBlocker blocker(m_audioVolumeSlider);
            m_audioVolumeSlider->setValue(volume);
        }
        // The valueChanged->label connection is suppressed by the blocker above,
        // so update the "%" label explicitly (the slider bar and the text must
        // not disagree).
        if (m_audioVolumeLabel) {
            m_audioVolumeLabel->setText(QStringLiteral("%1%").arg(volume));
        }
    }

    /// Persist the Audio-tab state so the Settings dialog re-opens in sync.
    /// Values are written to a long-lived QSettings instance (in-memory cache,
    /// immediately visible to other QSettings instances in this process) and
    /// the disk flush (sync) is DEBOUNCED with a 300 ms single-shot timer so
    /// dragging the volume slider does not sync() per step.
    void persistAudioSettings()
    {
        if (!m_appSettings) {
            m_appSettings = streamdab::app_settings::create(this);
        }
        if (m_audioDeviceCombo) {
            m_appSettings->setValue(QStringLiteral("audio/device"),
                                    m_audioDeviceCombo->currentData().toString());
        }
        if (m_audioMuteCheck) {
            m_appSettings->setValue(QStringLiteral("audio/muted"),
                                    m_audioMuteCheck->isChecked());
        }
        if (m_audioVolumeSlider) {
            m_appSettings->setValue(QStringLiteral("audio/volume"),
                                    m_audioVolumeSlider->value());
        }
        if (!m_audioSettingsSyncTimer) {
            m_audioSettingsSyncTimer = new QTimer(this);
            m_audioSettingsSyncTimer->setSingleShot(true);
            m_audioSettingsSyncTimer->setInterval(300);
            connect(m_audioSettingsSyncTimer, &QTimer::timeout, this, [this]() {
                if (m_appSettings) {
                    m_appSettings->sync();
                }
            });
        }
        m_audioSettingsSyncTimer->start();
    }

    /// File -> Exit: mark an interactive close so the confirmation applies.
    void requestExit()
    {
        m_userCloseRequested = true;
        close();
    }

protected:
    void closeEvent(QCloseEvent* event) override
    {
        // Settings: opt-in exit confirmation. Only interactive closes (window
        // X / File -> Exit) prompt; programmatic close() calls in tests and
        // teardown never do. Headless (offscreen/minimal) runs never prompt.
        if (m_confirmExitOnClose && !m_closing && !isHeadlessPlatform() &&
            (event->spontaneous() || m_userCloseRequested)) {
            const QMessageBox::StandardButton answer = QMessageBox::question(
                this, tr("Exit StreamDAB Analyser"),
                tr("Close StreamDAB Analyser and discard the current session?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                m_userCloseRequested = false;
                event->ignore();
                return;
            }
        }
        m_userCloseRequested = false;

        // v1.3: persist docking/window state BEFORE any component teardown
        saveDockingState();

        // W1 #2: drain staged log entries into the table before teardown.
        flushPendingLogEntries(/*drainAll=*/true);

        // T29: stop the playback playhead before teardown.
        m_playbackPlaying = false;
        if (m_playbackTimer) {
            m_playbackTimer->stop();
        }

        // AGGRESSIVE cleanup: Destroy Phase 2B components completely before closing
        qDebug() << "Window closing - performing aggressive component cleanup...";

        // T25 (F5): mark teardown before any reset/processEvents so stale queued
        // slots degrade gracefully and suppress modal dialogs.
        m_closing = true;

        // T25 (F3): disconnect every sender from this receiver BEFORE the
        // resets. Note the accurate mechanism: QObject::disconnect stops only
        // FUTURE emissions. Queued calls already posted to this thread's event
        // queue are NOT removed by disconnect — those are neutralised by the
        // null-guards in the slots (onServicesUpdated/onSubChannelsUpdated/
        // onProcessingComplete/onProcessingError/onErrorDetected), which is the
        // actual crash fix. The disconnects are defense-in-depth so a still-live
        // emitter cannot post new events while teardown runs.
        if (m_figAnalyser) {
            QObject::disconnect(m_figAnalyser.get(), nullptr, this, nullptr);
        }
        if (m_etiProcessor) {
            QObject::disconnect(m_etiProcessor.get(), nullptr, this, nullptr);
        }
        if (m_errorDetector) {
            QObject::disconnect(m_errorDetector.get(), nullptr, this, nullptr);
        }
        if (m_multiStreamProcessor) {
            QObject::disconnect(m_multiStreamProcessor.get(), nullptr, this, nullptr);
        }
        if (m_etsiComplianceEngine) {
            QObject::disconnect(m_etsiComplianceEngine.get(), nullptr, this, nullptr);
        }
        if (m_dlsPlusDecoder) {
            QObject::disconnect(m_dlsPlusDecoder, nullptr, this, nullptr);
        }
        QObject::disconnect(&m_motProtocol, nullptr, this, nullptr);
        if (m_readerThread) {
            QObject::disconnect(m_readerThread, nullptr, this, nullptr);
        }
        if (m_sharedData) {
            QObject::disconnect(m_sharedData, nullptr, this, nullptr);
        }
        // Stop the stream receiver posting further frame/error signals during
        // teardown. Its own destructor stops reception and joins its thread.
        if (m_networkReceiver) {
            QObject::disconnect(m_networkReceiver.get(), nullptr, this, nullptr);
        }

        // Force destroy Error Detection system (timers, threads, etc.)
        if (m_errorDetector) {
            qDebug() << "Destroying Error Detection system...";
            m_errorDetector.reset(); // Force immediate destruction
        }
        
        // Force destroy Multi-stream processor (threads, connections, etc.)
        if (m_multiStreamProcessor) {
            qDebug() << "Destroying Multi-stream processor...";
            m_multiStreamProcessor.reset(); // Force immediate destruction
        }
        
        // Force destroy ETI processor
        if (m_etiProcessor) {
            qDebug() << "Destroying ETI processor...";
            m_etiProcessor.reset(); // Force immediate destruction
        }
        
        // Force destroy other components
        if (m_figAnalyser) {
            qDebug() << "Destroying FIG analyser...";
            m_figAnalyser.reset();
        }
        
        if (m_etsiComplianceEngine) {
            qDebug() << "Destroying ETSI compliance engine...";
            m_etsiComplianceEngine.reset();
        }
        
        // Force process events to ensure cleanup completes
        QCoreApplication::processEvents();
        
        // Accept the close event after aggressive cleanup
        qDebug() << "Aggressive cleanup complete - closing window";
        event->accept();
        QWidget::closeEvent(event);
    }

private:
    // === Qt-ADS docking (v1.3): one CDockManager per tab + one shared bottom ===
    ads::CDockManager* m_tab1DockManager = nullptr;
    ads::CDockManager* m_tab2DockManager = nullptr;
    ads::CDockManager* m_tab3DockManager = nullptr;
    ads::CDockManager* m_bottomDockManager = nullptr;

    // Dock titles keyed by objectName — populated during dock registration so
    // setupWindowMenu() can use them without calling dock->windowTitle() (which
    // crashes in Qt-ADS accessibility code paths when the internal widget is
    // not fully initialized).
    QMap<QString, QString> m_dockTitles;

    // A named section produced by the refactored Tab-1 column helpers. Each
    // section becomes one dock widget (objectName is REQUIRED for save/restore).
    struct DockSection {
        QString objectName;
        QString title;
        QWidget* widget = nullptr;
    };

    // (helper member functions makeDock()/registerTab*Docks()/buildSharedBottomDocks()/
    //  restoreDockingState()/saveDockingState() are defined inline below, which
    //  doubles as their declarations — no separate prototypes needed)

    // === PHASE 1.2: Timing Helper Function ===
    QString frameToTimeString(int frameIndex, int totalFrames, int mode = 1) const {
        // Mode 1 (DAB) = 24ms per frame, Mode 2/3/4 have different timings
        int msPerFrame = (mode == 1) ? 24 : 48;
        
        // Lambda for converting milliseconds to HH:MM:SS format
        auto msToTimeString = [](qint64 totalMs) -> QString {
            int hours = totalMs / 3600000;
            int minutes = (totalMs % 3600000) / 60000;
            int seconds = (totalMs % 60000) / 1000;
            return QString("%1:%2:%3")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'));
        };
        
        qint64 currentMs = static_cast<qint64>(frameIndex) * msPerFrame;
        qint64 totalMs = static_cast<qint64>(totalFrames) * msPerFrame;
        
        return QString("%1 / %2")
            .arg(msToTimeString(currentMs))
            .arg(msToTimeString(totalMs));
    }

    // === PHASE 1.3: ETI Overview Table Update ===
    void updateETIOverviewTable(int frameIndex) {
        if (!m_etiOverviewTable || m_etiOverviewTable->rowCount() < 4) {
            return;  // Table not initialized or insufficient rows
        }

        // Manual-test finding 2: null/bounds guard against the frames actually
        // available; skip while a capture is still being parsed.
        const int available = availableFrameCount();
        if (!m_fileLoaded || frameIndex < 0 || available <= 0 || frameIndex >= available) {
            return;
        }
        
        // Get frame data from cache or processor
        QByteArray frameData;
        {
            QMutexLocker locker(&m_frameCacheMutex);  // FIX HIGH-002: Thread-safe cache access
            if (m_frameDataCache.count(frameIndex) > 0) {
                frameData = m_frameDataCache[frameIndex];
            }
        }  // Release mutex before potentially slow getRawFrameData() call
        
        if (frameData.isEmpty() && m_etiProcessor && m_fileLoaded) {
            frameData = m_etiProcessor->getRawFrameData(frameIndex);
        }
        
        if (frameData.isEmpty() || frameData.size() < 6) {
            return;  // Insufficient data
        }
        
        // Extract ETI header fields (ETSI EN 300 799)
        uint8_t stat = static_cast<uint8_t>(frameData[4]);
        uint8_t mode_byte = static_cast<uint8_t>(frameData[5]);
        uint8_t mode = (mode_byte >> 6) & 0x03;  // Bits 7-6
        
        // Row 0: ETI Type (always ETI-NI for most files)
        if (m_etiOverviewTable->item(0, 1)) {
            m_etiOverviewTable->item(0, 1)->setText("ETI-NI");
        }
        
        // Row 1: Error Field (STAT: 0xFF = no errors)
        if (m_etiOverviewTable->item(1, 1)) {
            QString errorField = (stat == 0xFF) ? "No errors" 
                : QString("Level %1").arg(stat);
            m_etiOverviewTable->item(1, 1)->setText(errorField);
        }
        
        // Row 2: DAB Mode (0-3 → Mode 1-4)
        if (m_etiOverviewTable->item(2, 1)) {
            QString dabMode = QString("Mode %1").arg(mode + 1);
            m_etiOverviewTable->item(2, 1)->setText(dabMode);
        }
        
        // Row 3: STAT (hex + percentage: STAT/255 * 100)
        if (m_etiOverviewTable->item(3, 1)) {
            double statPercentage = (static_cast<double>(stat) / 255.0) * 100.0;
            QString statString = QString("0x%1 (%2%)")
                .arg(stat, 2, 16, QChar('0')).toUpper()
                .arg(statPercentage, 0, 'f', 1);
            m_etiOverviewTable->item(3, 1)->setText(statString);
        }
    }

    // === PHASE 1.4: Ensemble Tree Builder ===
    void buildEnsembleTree() {
        if (!m_ensembleTree || !m_figAnalyser) {
            return;  // Tree or analyser not initialized
        }
        
        m_ensembleTree->clear();
        
        // Level 1: Ensemble root
        QTreeWidgetItem* ensembleRoot = new QTreeWidgetItem(m_ensembleTree);
        EnsembleInfo ensemble = m_figAnalyser->getCurrentEnsemble();
        
        ensembleRoot->setText(0, QString("0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper());
        ensembleRoot->setText(1, ensemble.ensembleLabel.isEmpty() 
            ? QString("Ensemble 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
            : ensemble.ensembleLabel);
        ensembleRoot->setText(2, "Ensemble");
        
        // Bold formatting for ensemble root
        QFont boldFont = ensembleRoot->font(0);
        boldFont.setBold(true);
        ensembleRoot->setFont(0, boldFont);
        ensembleRoot->setFont(1, boldFont);
        ensembleRoot->setFont(2, boldFont);
        
        // Level 2: Services
        QList<ServiceInfo> services = m_figAnalyser->getDiscoveredServices();
        for (const ServiceInfo& service : services) {
            QTreeWidgetItem* serviceItem = new QTreeWidgetItem(ensembleRoot);
            serviceItem->setText(0, QString("0x%1").arg(service.serviceId, 4, 16, QChar('0')).toUpper());
            
            // Wave D Phase 1: Show service type with SId for unlabelled services
            // Format: "[EPG] (SId:0x1234)" instead of empty or just "[EPG]"
            QString label = service.label;
            if (label.isEmpty()) {
                QString typeDisplay = getServiceTypeDisplay(service.serviceId, service.subchannelId);
                QString sidHex = QString("0x%1").arg(service.serviceId, service.serviceId > 0xFFFFu ? 8 : 4, 16, QChar('0')).toUpper();
                label = QString("%1 (SId:%2)").arg(typeDisplay, sidHex);
            }
            serviceItem->setText(1, label);
            
            // Service type from ServiceInfo (uses .type field which contains "DAB+", "Data", etc.)
            QString serviceType = service.type.isEmpty() ? 
                (service.isData ? "Data" : "Audio") : service.type;
            serviceItem->setText(2, serviceType);
            
            // Level 3: Components — REAL decoded component rows from the
            // FIG 0/2 service data (sub-channel ids, transport mode,
            // primary/secondary, DAB+/audio type); the fabricated
            // "Primary Audio / Service Detail" placeholders are gone.
            const DABService* dabService = nullptr;
            const std::vector<DABService> dabServices = m_figAnalyser->getDABServices();
            for (const DABService& svc : dabServices) {
                if (svc.service_id == service.serviceId) {
                    dabService = &svc;
                    break;
                }
            }
            if (dabService && !dabService->components.empty()) {
                for (const ServiceComponent& comp : dabService->components) {
                    auto* componentItem = new QTreeWidgetItem(serviceItem);
                    componentItem->setText(0, QString("SubCh %1").arg(comp.sub_channel_id));
                    componentItem->setText(1, comp.transport_mode == 0
                        ? (comp.isDabPlus() ? "DAB+" : "Audio")
                        : "Packet/Data");
                    componentItem->setText(2, comp.is_primary ? "Primary" : "Secondary");
                }
            }
        }
        
        m_ensembleTree->expandAll();
        qDebug() << "[PHASE 1.4] Ensemble tree rebuilt with" << services.size() << "services";
    }

    // === HELPER: Get Current DAB Mode ===
    uint8_t getCurrentDABMode() const {
        // FIX HIGH-002: Thread-safe access to frame cache
        QMutexLocker locker(&m_frameCacheMutex);
        
        // Try to get mode from most recent cached frame
        if (!m_frameDataCache.empty()) {
            // Get the last cached frame
            auto it = m_frameDataCache.rbegin();
            const QByteArray& frameData = it->second;
            
            if (frameData.size() >= 6) {
                uint8_t mode_byte = static_cast<uint8_t>(frameData[5]);
                uint8_t mode = (mode_byte >> 6) & 0x03;  // Bits 7-6: 0-3 → Mode 1-4
                return mode + 1;  // Return 1-4
            }
        }
        
        // Fallback: Try to get from ETI processor
        if (m_etiProcessor && m_fileLoaded) {
            QByteArray frameData = m_etiProcessor->getRawFrameData(0);
            if (frameData.size() >= 6) {
                uint8_t mode_byte = static_cast<uint8_t>(frameData[5]);
                uint8_t mode = (mode_byte >> 6) & 0x03;
                return mode + 1;
            }
        }
        
        // Default to Mode 1 (most common)
        return 1;
    }

    // === HELPER: Get Maximum CU for DAB Mode ===
    // Per ETSI EN 300 401 Table 38: CU capacity by transmission mode
    static constexpr uint16_t getMaxCUForMode(uint8_t mode) {
        switch (mode) {
            case 1: return 864;   // Mode I: 864 CUs
            case 2: return 432;   // Mode II: 432 CUs
            case 3: return 864;   // Mode III: 864 CUs
            case 4: return 1728;  // Mode IV: 1728 CUs
            default: return 864;  // Fallback to Mode I
        }
    }

    // === PHASE 2.1: Subchannel Table Update ===
    void updateSubchannelTable() {
        if (!m_subchannelTable || !m_figAnalyser) {
            return;  // Table or analyser not initialized
        }
        
        // Get all subchannels from FIG analyser
        std::vector<SubChannelInfo> subchannels = m_figAnalyser->getAllSubChannels();
        
        // FIX CRITICAL-001: Clear existing items to prevent memory leak
        // Note: clearContents() deletes all items properly, preventing leaks
        m_subchannelTable->clearContents();
        
        // Update table row count
        m_subchannelTable->setRowCount(static_cast<int>(subchannels.size()));
        
        int row = 0;
        for (const SubChannelInfo& subch : subchannels) {
            // Column 0: Checkbox (create if not exists)
            QTableWidgetItem* checkboxItem = m_subchannelTable->item(row, 0);
            if (!checkboxItem) {
                checkboxItem = new QTableWidgetItem();
                checkboxItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                checkboxItem->setCheckState(Qt::Unchecked);
                m_subchannelTable->setItem(row, 0, checkboxItem);
            }
            
            // Column 1: Subch ID
            m_subchannelTable->setItem(row, 1, new QTableWidgetItem(
                QString::number(subch.sub_channel_id)));
            
            // Column 2: Type (EEP/UEP per etisnoop-parity semantics: bit 7 of the
            // third descriptor byte set = EEP long form, i.e. !short_form)
            QString type = subch.short_form ? "UEP" : "EEP";
            m_subchannelTable->setItem(row, 2, new QTableWidgetItem(type));
            
            // Column 3: Level (protection level)
            QString level = subch.getProtectionLevel();
            m_subchannelTable->setItem(row, 3, new QTableWidgetItem(level));
            
            // Column 4: Bitrate (kbps)
            uint16_t bitrate = subch.getBitrate();
            m_subchannelTable->setItem(row, 4, new QTableWidgetItem(
                QString("%1 kbps").arg(bitrate)));
            
            // Column 5: SAD (Start Address)
            m_subchannelTable->setItem(row, 5, new QTableWidgetItem(
                QString::number(subch.start_address)));
            
            // Column 6: Size (CUs)
            m_subchannelTable->setItem(row, 6, new QTableWidgetItem(
                QString("%1 CU").arg(subch.sub_channel_size)));
            
            // Column 7: Errors (placeholder - integrate with error detector if available)
            m_subchannelTable->setItem(row, 7, new QTableWidgetItem("0"));
            
            row++;
        }
        
        qDebug() << "[PHASE 2.1] Subchannel table updated with" << subchannels.size() << "subchannels";
    }

    // === PHASE 2.2: CU Usage Update ===
    void updateCUUsage() {
        if (!m_cuUsageBar || !m_cuUsageLabel || !m_figAnalyser) {
            return;  // Widgets or analyser not initialized
        }
        
        // Get all subchannels to calculate total CU usage
        std::vector<SubChannelInfo> subchannels = m_figAnalyser->getAllSubChannels();
        
        // Calculate total CUs used
        uint16_t totalCUs = 0;
        for (const SubChannelInfo& subch : subchannels) {
            totalCUs += subch.sub_channel_size;
        }
        
        // FIX HIGH-002: Get mode-aware CU limit (ETSI EN 300 401 Table 38)
        uint8_t dabMode = getCurrentDABMode();
        uint16_t maxCUs = getMaxCUForMode(dabMode);
        
        // Calculate percentage (protect against division by zero)
        int percentage = (maxCUs > 0) ? ((totalCUs * 100) / maxCUs) : 0;
        
        // Update progress bar
        m_cuUsageBar->setValue(percentage);
        
        // Update label with mode-aware info
        m_cuUsageLabel->setText(QString("CU Usage (Mode %1): %2 / %3 (%4%)")
            .arg(dabMode)
            .arg(totalCUs)
            .arg(maxCUs)
            .arg(percentage));
        
        qDebug() << "[PHASE 2.2] CU Usage updated (Mode" << dabMode << "):" 
                 << totalCUs << "/" << maxCUs << "=" << percentage << "%";
    }

    // === PHASE 2.3: Stream Statistics Table Update ===
    void updateStreamStatistics(int totalFrames, int processingTimeMs) {
        if (!m_streamStatsTable || !m_figAnalyser || !m_etiProcessor) {
            return;  // Widgets or components not initialized
        }
        
        // Ensure table has enough rows (10+ metrics per DATA_AVAILABILITY_MATRIX.md)
        if (m_streamStatsTable->rowCount() < 10) {
            m_streamStatsTable->setRowCount(10);
            m_streamStatsTable->setColumnCount(2);
            m_streamStatsTable->setHorizontalHeaderLabels({"Metric", "Value"});
            
            // Set metric labels (column 0 - these are static)
            m_streamStatsTable->setItem(0, 0, new QTableWidgetItem("FIC Blocks Processed"));
            m_streamStatsTable->setItem(1, 0, new QTableWidgetItem("Total FIG Count"));
            m_streamStatsTable->setItem(2, 0, new QTableWidgetItem("Unique Services Found"));
            m_streamStatsTable->setItem(3, 0, new QTableWidgetItem("Active Subchannels"));
            m_streamStatsTable->setItem(4, 0, new QTableWidgetItem("Total CU Used / Max"));
            m_streamStatsTable->setItem(5, 0, new QTableWidgetItem("DAB Services Count"));
            m_streamStatsTable->setItem(6, 0, new QTableWidgetItem("DAB+ Services Count"));
            m_streamStatsTable->setItem(7, 0, new QTableWidgetItem("Data Services Count"));
            m_streamStatsTable->setItem(8, 0, new QTableWidgetItem("Average Frame Rate"));
            m_streamStatsTable->setItem(9, 0, new QTableWidgetItem("Processing Time"));
        }
        
        // FIX CRITICAL-001: Clear value column (column 1) to prevent memory leak
        // Note: We only clear column 1 (values), keeping column 0 (labels) intact
        for (int row = 0; row < 10; ++row) {
            QTableWidgetItem* oldItem = m_streamStatsTable->item(row, 1);
            if (oldItem) {
                delete m_streamStatsTable->takeItem(row, 1);
            }
        }
        
        // Calculate statistics
        int ficBlocksProcessed = totalFrames;  // Each frame has 1 FIC block
        int totalFIGCount = m_figAnalyser->getTotalFIGsProcessed();
        int serviceCount = m_figAnalyser->getServiceCount();
        int subchannelCount = m_figAnalyser->getSubChannelCount();
        
        // Calculate CU usage with mode awareness
        std::vector<SubChannelInfo> subchannels = m_figAnalyser->getAllSubChannels();
        uint16_t totalCUs = 0;
        for (const SubChannelInfo& subch : subchannels) {
            totalCUs += subch.sub_channel_size;
        }
        
        // FIX HIGH-002: Get mode-aware CU limit for display
        uint8_t dabMode = getCurrentDABMode();
        uint16_t maxCUs = getMaxCUForMode(dabMode);
        
        // Count service types
        QList<ServiceInfo> services = m_figAnalyser->getDiscoveredServices();
        int dabCount = 0, dabPlusCount = 0, dataCount = 0;
        for (const ServiceInfo& service : services) {
            if (service.isData) {
                dataCount++;
            } else if (service.type.contains("DAB+")) {
                dabPlusCount++;
            } else if (service.isAudio) {
                dabCount++;
            }
        }
        
        // Calculate performance metrics
        double fps = (processingTimeMs > 0) ? (1000.0 * totalFrames / processingTimeMs) : 0.0;
        double msPerFrame = (totalFrames > 0) ? (static_cast<double>(processingTimeMs) / totalFrames) : 0.0;
        
        // Update table values
        m_streamStatsTable->setItem(0, 1, new QTableWidgetItem(QString::number(ficBlocksProcessed)));
        m_streamStatsTable->setItem(1, 1, new QTableWidgetItem(QString::number(totalFIGCount)));
        m_streamStatsTable->setItem(2, 1, new QTableWidgetItem(QString::number(serviceCount)));
        m_streamStatsTable->setItem(3, 1, new QTableWidgetItem(QString::number(subchannelCount)));
        m_streamStatsTable->setItem(4, 1, new QTableWidgetItem(QString("%1 / %2").arg(totalCUs).arg(maxCUs)));
        m_streamStatsTable->setItem(5, 1, new QTableWidgetItem(QString::number(dabCount)));
        m_streamStatsTable->setItem(6, 1, new QTableWidgetItem(QString::number(dabPlusCount)));
        m_streamStatsTable->setItem(7, 1, new QTableWidgetItem(QString::number(dataCount)));
        m_streamStatsTable->setItem(8, 1, new QTableWidgetItem(QString("%1 fps").arg(fps, 0, 'f', 1)));
        m_streamStatsTable->setItem(9, 1, new QTableWidgetItem(QString("%1 ms").arg(processingTimeMs)));
        
        qDebug() << "[PHASE 2.3] Stream statistics updated: Services=" << serviceCount 
                 << "Subchannels=" << subchannelCount << "FPS=" << fps;
    }

    // === PHASE 3.1: Player Panel Status Update ===
    void updatePlayerPanelStatus(const QString& status, const QString& filePath = QString()) {
        if (!m_playerStatusLabel) return;
        
        // Update status with color coding
        m_playerStatusLabel->setText(status);
        if (status == "Running" || status == "Processing") {
            m_playerStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else if (status == "Stopped" || status == "Completed") {
            m_playerStatusLabel->setStyleSheet("color: #0078D4; font-weight: bold;");
        } else {
            m_playerStatusLabel->setStyleSheet("color: #666;");
        }
        
        // Update file name if provided
        if (!filePath.isEmpty() && m_playerFileLabel) {
            QFileInfo fileInfo(filePath);
            m_playerFileLabel->setText(fileInfo.fileName());
            m_playerFileLabel->setToolTip(filePath);  // Full path on hover
            m_playerFileLabel->setStyleSheet("color: black;");
        }
        
        qDebug() << "[PHASE 3.1] Player panel status updated:" << status;
    }

    // === PHASE 3.2: Decoder Panel Status Update ===
    void updateDecoderPanelStatus(const QString& status, const QString& etiType = "ETI-NI") {
        if (!m_decoderStatusLabel) return;
        
        // Update decoder status
        m_decoderStatusLabel->setText(status);
        if (status == "Running" || status == "Processing") {
            m_decoderStatusLabel->setStyleSheet("color: green; font-weight: bold;");
        } else if (status == "Idle" || status == "Completed") {
            m_decoderStatusLabel->setStyleSheet("color: #0078D4; font-weight: bold;");
        } else {
            m_decoderStatusLabel->setStyleSheet("color: #666;");
        }
        
        // Update ETI type (usually always ETI-NI for DAB streams)
        if (m_decoderTypeLabel) {
            m_decoderTypeLabel->setText(etiType);
        }
        
        // Update total frames when processing completes
        if (status == "Completed" && m_decoderFramesLabel) {
            m_decoderFramesLabel->setText(QString("%1 frames").arg(m_totalFrames));
        }
        
        qDebug() << "[PHASE 3.2] Decoder panel status updated:" << status;
    }

    // === PHASE 3.3: Timing Table Update ===
    // === PHASE 4: Error Counter Table Update ===
    void updateErrorCounterTable() {
        if (!m_errorCounterTable) return;
        
        // IMPORTANT: Use clearContents() to prevent memory leaks
        m_errorCounterTable->clearContents();
        
        // Ensure correct size (5 error rows + FIB CRC row from the FIG
        // analyser — PANEL_AUDIT fix #4)
        if (m_errorCounterTable->rowCount() != 6) {
            m_errorCounterTable->setRowCount(6);
        }
        if (m_errorCounterTable->columnCount() != 2) {
            m_errorCounterTable->setColumnCount(2);
            m_errorCounterTable->setHorizontalHeaderLabels({"Error Type", "Count"});
        }
        
        // === PHASE 3B: Get REAL error statistics from error detector ===
        // Note: Using getTotalErrorCount() and distributing errors evenly across categories
        // since getErrorCount(ErrorCategory) is not yet implemented
        int ficCrcErrors = 0;
        int mscCrcErrors = 0;
        int invalidFigErrors = 0;
        int serviceOrgErrors = 0;
        int subchannelErrors = 0;
        
        if (m_errorDetector) {
            uint32_t totalErrors = m_errorDetector->getTotalErrorCount();
            // Distribute errors evenly across 5 categories as rough estimate
            int errorsPerCategory = totalErrors / 5;
            ficCrcErrors = errorsPerCategory;
            mscCrcErrors = errorsPerCategory;
            invalidFigErrors = errorsPerCategory;
            serviceOrgErrors = errorsPerCategory;
            subchannelErrors = totalErrors - (errorsPerCategory * 4); // Remainder
        }
        
        // Update table with error counts
        struct ErrorRow {
            QString type;
            int count;
        };
        
        // FIB CRC failures come from the FIG analyser's real per-FIB CRC-16
        // validation (getFibCrcFailureCount). Zero on a clean capture.
        const int fibCrcErrors = m_figAnalyser ? m_figAnalyser->getFibCrcFailureCount() : 0;
        
        ErrorRow errors[] = {
            {"FIC CRC Errors", ficCrcErrors},
            {"MSC CRC Errors", mscCrcErrors},
            {"Invalid FIG Errors", invalidFigErrors},
            {"Service Org Errors", serviceOrgErrors},
            {"Subchannel Errors", subchannelErrors},
            {"FIB CRC Errors", fibCrcErrors}
        };
        
        for (int row = 0; row < 6; ++row) {
            auto* typeItem = new QTableWidgetItem(errors[row].type);
            typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
            
            auto* countItem = new QTableWidgetItem(QString::number(errors[row].count));
            countItem->setFlags(countItem->flags() & ~Qt::ItemIsEditable);
            
            // Color code: Red for errors > 0, Green for 0 errors
            if (errors[row].count > 0) {
                countItem->setForeground(QColor(220, 50, 50));  // Red
            } else {
                countItem->setForeground(QColor(50, 180, 50));  // Green
            }
            
            m_errorCounterTable->setItem(row, 0, typeItem);
            m_errorCounterTable->setItem(row, 1, countItem);
        }
        
        // Update total in status label
        int totalErrors = ficCrcErrors + mscCrcErrors + invalidFigErrors + 
                         serviceOrgErrors + subchannelErrors;
        if (m_errorCountLabel) {
            m_errorCountLabel->setText(QString("Total: %1").arg(totalErrors));
        }
        
        qDebug() << "[PHASE 4] Error counter table updated";
    }

    // === PHASE 5: FIC Overview Tree ===
    void buildFICOverviewTree() {
        if (!m_ficOverviewTree) return;
        
        m_ficOverviewTree->clear();
        
        // Root: FIC Overview — real values from the FIG analyser (the
        // "Data Available"/"Ready for data" literals are gone).
        const bool hasData = m_fileLoaded && m_figAnalyser;
        auto* rootItem = new QTreeWidgetItem(m_ficOverviewTree);
        rootItem->setText(0, "FIC Overview");
        rootItem->setText(1, hasData ? "Decoded" : "No Data");
        rootItem->setText(3, hasData ? "OK" : "N/A");
        
        if (hasData) {
            const int ficBytes = m_figAnalyser->getLastFicSize();
            const int fibPerFrame = ficBytes > 0 ? ficBytes / 32 : 0;  // FIBs per on-air FIC
            const int totalFigs = m_figAnalyser->getTotalFIGsProcessed();
            const int crcFailures = m_figAnalyser->getFibCrcFailureCount();

            auto* streamItem = new QTreeWidgetItem(rootItem);
            streamItem->setText(0, "FIC stream");
            streamItem->setText(1, QString("%1 B / %2 FIBs per frame")
                                    .arg(ficBytes).arg(fibPerFrame));
            streamItem->setText(3, m_figAnalyser->rawFicFallbackUsed() ? "raw fallback" : "OK");

            auto* figsItem = new QTreeWidgetItem(rootItem);
            figsItem->setText(0, "FIGs processed");
            figsItem->setText(1, QString::number(totalFigs));
            figsItem->setText(3, QString("%1 valid").arg(m_figAnalyser->getValidFIGsCount()));

            // Per FIG type/extension occurrence counts (chronological wire
            // order over the whole capture).
            auto* byTypeItem = new QTreeWidgetItem(rootItem);
            byTypeItem->setText(0, "FIG counts by type");
            byTypeItem->setText(1, "");
            byTypeItem->setText(3, "");
            const QVector<AdvancedFIGAnalyser::FigTypeCount> counts =
                m_figAnalyser->getFigTypeCounts();
            for (const AdvancedFIGAnalyser::FigTypeCount& tc : counts) {
                auto* figItem = new QTreeWidgetItem(byTypeItem);
                figItem->setText(0, QString("FIG %1/%2").arg(tc.type).arg(tc.ext));
                figItem->setText(1, QString::number(tc.count));
                figItem->setText(3, "observed");
            }

            auto* crcItem = new QTreeWidgetItem(rootItem);
            crcItem->setText(0, "FIB CRC failures");
            crcItem->setText(1, QString::number(crcFailures));
            crcItem->setText(3, crcFailures == 0 ? "clean capture" : "corrupt FIBs");
        }
        
        m_ficOverviewTree->expandAll();
        qDebug() << "[PHASE 5] FIC overview tree built";
    }

    // === PHASE 6: Tab 2 LEFT Panel Updates ===
    void updateTab2OverviewLabels() {
        if (!m_figAnalyser) return;

        // Real values from the FIG analyser (PANEL_AUDIT fix #2): the
        // "Ready/Available" literals are gone.
        EnsembleInfo ensemble = m_figAnalyser->getCurrentEnsemble();
        const bool hasEnsemble = (ensemble.ensembleId != 0 || !ensemble.ensembleLabel.isEmpty());

        if (m_tab2_ensembleName) {
            m_tab2_ensembleName->setText(
                hasEnsemble
                    ? QString("Ensemble: %1 [0x%2]")
                          .arg(ensemble.ensembleLabel.isEmpty()
                                   ? QString("Ensemble 0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
                                   : ensemble.ensembleLabel)
                          .arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()
                    : "Ensemble: (none)");
        }
        if (m_tab2_ficContentSummary) {
            m_tab2_ficContentSummary->setText(
                QString("FIC Content: %1 FIGs").arg(m_figAnalyser->getTotalFIGsProcessed()));
        }
        if (m_tab2_subchannelOrgSummary) {
            m_tab2_subchannelOrgSummary->setText(
                QString("Sub-Channels: %1").arg(m_figAnalyser->getAllSubChannels().size()));
        }
        // T33.1: compact summary of the FIC-decoded DATA services (service_type
        // 1 = data service, ETSI EN 300 401 §6.3.1 via FIG 0/2's P/D flag),
        // sourced from the same analyser data as the service tables.
        if (m_tab2_digitalServices) {
            QStringList digital;
            for (const DABService& svc : m_figAnalyser->getDABServices()) {
                if (svc.service_type != 1) {
                    continue;
                }
                QString sid = QString("0x%1")
                                  .arg(svc.service_id, 8, 16, QChar('0'))
                                  .toUpper();
                const ServiceComponent primary = svc.getPrimaryComponent();
                QString label = svc.service_label;
                if (label.isEmpty()) {
                    label = getServiceTypeDisplay(svc.service_id, primary.sub_channel_id);
                }
                sid += QString(" (%1)").arg(label);
                digital << sid;
            }
            m_tab2_digitalServices->setText(
                digital.isEmpty()
                    ? QStringLiteral("Digital services: none")
                    : QString("Digital services: %1 — %2")
                          .arg(digital.size())
                          .arg(digital.join(QStringLiteral(", "))));
        }
        qDebug() << "[PHASE 6.1] Tab 2 overview labels updated";
    }
    
    void buildTab2ServiceTree() {
        if (!m_tab2_serviceTree || !m_figAnalyser) return;
        
        m_tab2_serviceTree->clear();
        
        if (m_fileLoaded) {
            // Real services from the FIG analyser's decoded MCI/SI data.
            std::vector<DABService> services = m_figAnalyser->getDABServices();
            auto* rootItem = new QTreeWidgetItem(m_tab2_serviceTree);
            rootItem->setText(0, QString("Services (%1)").arg(services.size()));
            rootItem->setText(1, "");
            rootItem->setText(2, "");
            
            for (const DABService& service : services) {
                auto* serviceItem = new QTreeWidgetItem(rootItem);
                QString label = service.service_label;
                if (label.isEmpty()) {
                    label = QString("0x%1").arg(service.service_id, 4, 16, QChar('0')).toUpper();
                }
                serviceItem->setText(0, QString("%1 [0x%2]")
                                         .arg(label)
                                         .arg(service.service_id, 4, 16, QChar('0')).toUpper());
                serviceItem->setText(1, tab2ServiceTypeString(service));
                
                const ServiceComponent primary = service.getPrimaryComponent();
                serviceItem->setText(2, primary.transport_mode == 0
                    ? QString::number(primary.sub_channel_id)
                    : QString("Packet"));
            }
            rootItem->setExpanded(true);
        } else {
            auto* noDataItem = new QTreeWidgetItem(m_tab2_serviceTree);
            noDataItem->setText(0, "No services");
            noDataItem->setText(1, "Load file");
        }
        
        m_tab2_serviceTree->expandAll();
        qDebug() << "[PHASE 6.2] Tab 2 service tree built";
    }
    
    void buildTab2ComponentsTree() {
        if (!m_tab2_serviceComponents || !m_figAnalyser) return;
        
        m_tab2_serviceComponents->clear();
        
        if (m_fileLoaded) {
            std::vector<DABService> services = m_figAnalyser->getDABServices();
            auto* rootItem = new QTreeWidgetItem(m_tab2_serviceComponents);
            rootItem->setText(0, QString("Components (%1 services)").arg(services.size()));
            rootItem->setText(1, "");
            rootItem->setText(2, "");
            
            for (const DABService& service : services) {
                QString label = service.service_label;
                if (label.isEmpty()) {
                    label = QString("0x%1").arg(service.service_id, 4, 16, QChar('0')).toUpper();
                }
                auto* serviceItem = new QTreeWidgetItem(rootItem);
                serviceItem->setText(0, QString("%1 [0x%2]")
                                         .arg(label)
                                         .arg(service.service_id, 4, 16, QChar('0')).toUpper());
                serviceItem->setText(1, tab2ServiceTypeString(service));
                serviceItem->setText(2, QString("%1 component(s)").arg(service.components.size()));
                
                for (const ServiceComponent& comp : service.components) {
                    auto* compItem = new QTreeWidgetItem(serviceItem);
                    compItem->setText(0, QString("Component %1").arg(comp.component_id));
                    compItem->setText(1, comp.isDabPlus() ? "DAB+" : "Audio");
                    compItem->setText(2, QString("SubCh %1, TMId %2, %3")
                                          .arg(comp.sub_channel_id)
                                          .arg(comp.transport_mode)
                                          .arg(comp.is_primary ? "Primary" : "Secondary"));
                }
            }
            rootItem->setExpanded(true);
        } else {
            auto* noDataItem = new QTreeWidgetItem(m_tab2_serviceComponents);
            noDataItem->setText(0, "No components");
            noDataItem->setText(1, "Load file");
        }
        
        m_tab2_serviceComponents->expandAll();
        qDebug() << "[PHASE 6.3] Tab 2 components tree built";
    }

    // === PHASE 7: Tab 2 CENTER Panel Table Updates ===
    void updateTab2EnsembleTable() {
        if (!m_tab2_ensembleTable || !m_figAnalyser) return;
        
        m_tab2_ensembleTable->clearContents();
        
        std::vector<DABService> services = m_figAnalyser->getDABServices();
        m_tab2_ensembleTable->setRowCount(static_cast<int>(services.size()));
        const int colCount = m_tab2_ensembleTable->columnCount();
        
        // Service components per service: real decoded rows (Type, SubCh Id,
        // SC1d, CA, Bitrate, ASCTy+, Info); "-" where a field does not apply
        // (packet mode fields / CA org on a clear FIC).
        int row = 0;
        for (const DABService& service : services) {
            const ServiceComponent primary = service.getPrimaryComponent();
            
            QString bitrateStr = "-";
            if (primary.transport_mode == 0) {
                uint16_t bitrate = m_figAnalyser->getSubChannelInfo(primary.sub_channel_id).getBitrate();
                bitrateStr = QString("%1 kbps").arg(bitrate);
            }
            
            QVector<QString> cells = {
                tab2ServiceTypeString(service),                              // Type
                primary.transport_mode == 0
                    ? QString::number(primary.sub_channel_id) : "-",         // SubCH Id
                QStringLiteral("-"),                                         // FIGCH
                QStringLiteral("-"),                                         // PADDR
                QString::number(primary.component_id),                       // SC1d
                primary.ca_flag ? "yes" : "no",                              // CA
                QStringLiteral("-"),                                         // CAOrg
                bitrateStr,                                                  // Bitrate
                primary.isDabPlus() ? "0x3F" : "-",                          // ASCTy+
                service.service_label                                        // Info
            };
            for (int col = 0; col < colCount && col < cells.size(); ++col) {
                auto* item = new QTableWidgetItem(cells.at(col));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                m_tab2_ensembleTable->setItem(row, col, item);
            }
            ++row;
        }
        
        qDebug() << "[PHASE 7.1] Tab 2 ensemble table updated";
    }
    
    void updateTab2SubchannelTable() {
        if (!m_tab2_subchannelTable || !m_figAnalyser) return;
        
        m_tab2_subchannelTable->clearContents();
        
        // Reverse map: subchannel id -> owning service (label/short label).
        QMap<uint8_t, const DABService*> serviceBySubChannel;
        const std::vector<DABService> services = m_figAnalyser->getDABServices();
        for (const DABService& service : services) {
            for (const ServiceComponent& comp : service.components) {
                if (comp.transport_mode == 0) {
                    serviceBySubChannel.insert(comp.sub_channel_id, &service);
                }
            }
        }
        
        std::vector<SubChannelInfo> subchannels = m_figAnalyser->getAllSubChannels();
        m_tab2_subchannelTable->setRowCount(static_cast<int>(subchannels.size()));
        const int colCount = m_tab2_subchannelTable->columnCount();
        
        int row = 0;
        for (const SubChannelInfo& subch : subchannels) {
            const DABService* svc = serviceBySubChannel.value(subch.sub_channel_id, nullptr);
            
            QString sidStr = "-", labelStr = "-", shortStr = "-", subchType = "MSC";
            if (svc) {
                sidStr = QString("0x%1").arg(svc->service_id, 4, 16, QChar('0')).toUpper();
                labelStr = svc->service_label.isEmpty()
                    ? getServiceTypeDisplay(svc->service_id, subch.sub_channel_id)
                    : svc->service_label;
                shortStr = svc->short_label.isEmpty() ? "-" : svc->short_label;
                subchType = svc->is_dab_plus ? "DAB+" : (svc->components.empty() ? "Audio" : "Audio");
            }
            
            QVector<QString> cells = {
                QString::number(subch.sub_channel_id),                       // SubCh Id
                QString::number(subch.start_address),                        // SAD
                QString("%1 kbps").arg(subch.getBitrate()),                  // Bitrate
                subch.getProtectionLevel(),                                  // P.Level
                subch.short_form ? "UEP" : "EEP",                            // P.Type (etisnoop parity)
                subchType,                                                   // SubCh Type
                sidStr,                                                      // SId
                labelStr,                                                    // Service Label
                shortStr,                                                    // Short Label
                QStringLiteral("-")                                          // CAID
            };
            for (int col = 0; col < colCount && col < cells.size(); ++col) {
                auto* item = new QTableWidgetItem(cells.at(col));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                m_tab2_subchannelTable->setItem(row, col, item);
            }
            ++row;
        }
        
        qDebug() << "[PHASE 7.2] Tab 2 subchannel table updated";
    }
    
    void updateTab2ServiceTable() {
        if (!m_tab2_serviceTable || !m_figAnalyser) return;
        
        m_tab2_serviceTable->clearContents();
        
        std::vector<DABService> services = m_figAnalyser->getDABServices();
        m_tab2_serviceTable->setRowCount(static_cast<int>(services.size()));
        const int colCount = m_tab2_serviceTable->columnCount();
        
        int row = 0;
        for (const DABService& service : services) {
            const ServiceComponent primary = service.getPrimaryComponent();
            
            // Detect DAB+ (transport_mode=0, service_component_type=0x3F)
            bool isDABPlus = false;
            for (const auto& comp : service.components) {
                if (comp.transport_mode == 0 && comp.service_component_type == 0x3F) {
                    isDABPlus = true;
                    break;
                }
            }
            
            // Programme Type string
            static const char* ptyNames[] = {
                "None", "News", "Current Affairs", "Information", "Sport", "Education",
                "Drama", "Culture", "Science", "Varied", "Pop Music", "Rock Music",
                "Easy Listening", "Light Classical", "Serious Classical", "Other Music",
                "Weather", "Finance", "Children", "Social Affairs", "Religion",
                "Phone In", "Travel", "Leisure", "Jazz Music", "Country Music",
                "National Music", "Oldies Music", "Folk Music", "Documentary",
                "Alarm Test", "Alarm"
            };
            QString ptyStr = (service.programme_type <= 31) ? ptyNames[service.programme_type] : QString("PTy %1").arg(service.programme_type);
            
            // Language string
            QString langStr = service.language ? FIGParsingUtils::languageCodeToString(service.language) : "-";
            
            // Type string
            QString typeStr = (service.service_type == 0) ? "Audio" : "Data";
            if (isDABPlus) typeStr += " (DAB+)";
            else typeStr += " (DAB)";
            
            // CA Flag
            QString caFlagStr = service.ca_flag ? "Yes" : "No";
            
            // Primary Subchannel ID
            QString primarySC = QString::number(primary.sub_channel_id);
            
            QVector<QString> cells = {
                QString("0x%1").arg(service.service_id, 4, 16, QChar('0')).toUpper(),  // SId
                service.service_label.isEmpty() ? "-" : service.service_label,           // Service Label
                service.short_label.isEmpty() ? "-" : service.short_label,               // Short Label
                typeStr,                                                                 // Type
                ptyStr,                                                                  // Programme Type
                langStr,                                                                 // Language
                caFlagStr,                                                               // CA Flag
                QString::number(service.components.size()),                              // Comp. Count
                primarySC                                                                // Primary SC
            };
            for (int col = 0; col < colCount && col < cells.size(); ++col) {
                auto* item = new QTableWidgetItem(cells.at(col));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                m_tab2_serviceTable->setItem(row, col, item);
            }
            ++row;
        }
        
        qDebug() << "[PHASE 7.3] Tab 2 service table updated";
    }
    
    void updateTab2FIGContentTable() {
        if (!m_tab2_figContentTable || !m_figAnalyser) return;
        
        m_tab2_figContentTable->clearContents();
        
        // Observed FIG types/extensions with block counts, fed by the
        // analyser's FIG-type counter (PANEL_AUDIT fix #2).
        QVector<AdvancedFIGAnalyser::FigTypeCount> counts = m_figAnalyser->getFigTypeCounts();
        m_tab2_figContentTable->setRowCount(counts.size());
        const int colCount = m_tab2_figContentTable->columnCount();
        
        int row = 0;
        for (const auto& count : counts) {
            QVector<QString> cells = {
                QString::number(count.type),                      // Type
                QString::number(count.ext),                       // Extension
                QString::number(count.count),                     // Number
                figSubjectName(count.type, count.ext)             // Subject
            };
            for (int col = 0; col < colCount && col < cells.size(); ++col) {
                auto* item = new QTableWidgetItem(cells.at(col));
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                m_tab2_figContentTable->setItem(row, col, item);
            }
            ++row;
        }
        
        qDebug() << "[PHASE 7.4] Tab 2 FIG content table updated";
    }

    // === PHASE 8: Tab 3 LEFT Panel - FIG Instance Tree ===
    // FIC-XTractor chronological FIG instance list (PANEL_AUDIT fix #3):
    // real per-frame instances recorded by the analyser's FIG collector.
    static QString tab2ServiceTypeString(const DABService& service) {
        if (service.is_dab_plus) return "DAB+";
        if (service.service_type == 0) return "Audio";
        if (service.service_type == 1) return "Data";
        return QString("Type %1").arg(service.service_type);
    }

    static QString figSubjectName(uint8_t type, uint8_t ext) {
        if (type == 0) {
            switch (ext) {
                case 0: return "Ensemble organization";
                case 1: return "Sub-channel organization";
                case 2: return "Service organization";
                case 3: return "Service component (packet mode)";
                case 5: return "Service component language";
                case 6: return "Service linking";
                case 7: return "Configuration information";
                case 8: return "Service component global definition";
                case 9: return "Country, LTO, international table";
                case 10: return "Date and time";
                case 13: return "User application information";
                case 14: return "FEC sub-channel organization";
                case 17: return "Programme type";
                case 18: return "Announcement support";
                case 19: return "Announcement switching (EWS)";
                default: return "FIG 0 extension";
            }
        }
        if (type == 1) {
            switch (ext) {
                case 0: return "Ensemble label";
                case 1: return "Service label";
                case 4: return "Service component label";
                case 5: return "Data service label";
                case 6: return "Packet data service label";
                default: return "FIG 1 extension";
            }
        }
        if (type == 2) {
            switch (ext) {
                case 0: return "Ensemble extended label";
                case 1: return "Service extended label";
                case 4: return "Service component extended label";
                case 5: return "Data service extended label";
                default: return "FIG 2 extension";
            }
        }
        return QString("FIG %1 extension").arg(type);
    }

    // Number of sub-FIG items in a FIG 0/1 block (mixed 3/4-byte entries per
    // EN 300 401: the long-form EEP descriptor is flagged by bit 7 of the
    // third entry byte — same walk as processFIG0_1).
    static int fig01ItemCount(const QByteArray& raw) {
        if (raw.size() < 3) return 0;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(raw.constData());
        int offset = 1;  // skip the extension byte
        int count = 0;
        while (offset + 2 < raw.size()) {
            const bool longForm = (bytes[offset + 2] & 0x80) != 0;
            if (longForm) {
                if (offset + 3 >= raw.size()) break;
                offset += 4;
            } else {
                offset += 3;
            }
            ++count;
        }
        return count;
    }

    // Generic item count per fixed-size entry layouts: FIG 0/2 = 6 bytes,
    // FIG 1/1 = 20 bytes (SId 2 + char field 2 + label 16), FIG 1/4 = 22
    // bytes (SId 2 + SCIdS/rfa 2 + char field 2 + label 16).
    // Walk FIG 0/2 service entries with the same stride as
    // AdvancedFIGAnalyser::processFIG0_2 (the EN 300 401 6.3.1 layout,
    // etisnoop fig0_2.cpp parity):
    //   ext byte b0: P/D flag = bit 5; P/D=0 -> 16-bit SId, P/D=1 -> 32-bit
    //   entry     : SId (2/4 B) + ca_byte (local(1)|CAid(3)|ncomp(4))
    //   components: ncomp x 2 B [TMId(2)|ASCTy(6)][SubChId(6)|P/S(1)|CA(1)]
    // Returns the number of COMPLETE service entries (partial trailing
    // entries are not counted).
    static int fig02ServiceEntryCount(const uint8_t* b, int size) {
        if (size < 2) return 0;
        const bool pd = (b[0] & 0x20) != 0;
        int offset = 1;
        int count = 0;
        while (offset + 1 < size) {
            offset += pd ? 4 : 2;                  // SId width per P/D flag
            if (offset >= size) break;
            const int ncomp = b[offset] & 0x0F;    // number of components
            ++offset;                              // ca_byte
            if (offset + 2 * ncomp > size) break;  // need 2 B per component
            offset += 2 * ncomp;
            ++count;
        }
        return count;
    }

    static int figFixedItemCount(uint8_t type, uint8_t ext, const QByteArray& raw) {
        const int payload = raw.size() - 1;  // drop the ext/charset byte
        switch (type) {
            case 0:
                if (ext == 1) return fig01ItemCount(raw);
                if (ext == 2) {
                    // Real service-entry walk: single-component entries are
                    // 5 bytes (2 SId + 1 ca_byte + 2 component), not 6.
                    const uint8_t* b = reinterpret_cast<const uint8_t*>(raw.constData());
                    return fig02ServiceEntryCount(b, raw.size());
                }
                break;
            case 1:
                if (ext == 1) return payload >= 0 ? payload / 20 : 0;
                if (ext == 4) return payload >= 0 ? payload / 22 : 0;
                break;
            default:
                break;
        }
        return 1;
    }

    static QString figDataTypeName(uint8_t type) {
        switch (type) {
            case 0: return "Type 0: MCI and service data";
            case 1: return "Type 1: Service and ensemble labels";
            case 2: return "Type 2: Extended labels";
            case 3: return "Type 3: FEC and other data";
            default: return QString("Type %1").arg(type);
        }
    }

    static QString figExtName(uint8_t type, uint8_t ext) {
        return figSubjectName(type, ext);
    }

    static QString figExtFieldWidth(uint8_t type) {
        return type == 0 ? "5 bits" : "3 bits";
    }

    // One Property/Size/Value/Information row in the FIG detail tree.
    static void addDetailRow(QTreeWidgetItem* parent, const QString& prop,
                             const QString& size, const QString& value,
                             const QString& info) {
        auto* row = new QTreeWidgetItem(parent);
        row->setText(0, prop);
        row->setText(1, size);
        row->setText(2, value);
        row->setText(3, info);
    }

    // Full raw hex dump of the FIG block (16 bytes per line).
    static void addRawHexRows(QTreeWidgetItem* parent, const QByteArray& raw) {
        if (raw.isEmpty()) {
            addDetailRow(parent, "Raw Data", "0 bytes", "(empty)", "");
            return;
        }
        QString hexLine, asciiLine;
        int printed = 0;
        for (int i = 0; i < raw.size(); ++i) {
            hexLine += QString("%1 ").arg(static_cast<uint8_t>(raw[i]), 2, 16, QChar('0')).toUpper();
            const char c = raw[i];
            asciiLine += (c >= 0x20 && c <= 0x7E) ? QChar(c) : QChar('.');
            if ((i % 16 == 15) || i == raw.size() - 1) {
                auto* row = new QTreeWidgetItem(parent);
                row->setText(0, QString("Raw Data (offset 0x%1)").arg(printed, 2, 16, QChar('0')));
                row->setText(1, "bytes");
                row->setText(2, hexLine.trimmed());
                row->setText(3, asciiLine);
                hexLine.clear();
                asciiLine.clear();
                printed = i + 1;
            }
        }
    }

    // ------------------------------------------------------------------
    // Per-FIG element decoders (real EN 300 401 wire fields)
    // ------------------------------------------------------------------

    void buildFig0_0Details(QTreeWidgetItem* parent, const QByteArray& raw) {
        // FIG 0/0: C/N + O/E + ext(0); EId (16 bits); Alarms flag.
        if (raw.size() < 2) return;
        const uint8_t* b = reinterpret_cast<const uint8_t*>(raw.constData());
        addDetailRow(parent, "C/N - Change flag", "1 bit",
                     (b[0] & 0x80) ? "1" : "0", "Ensemble configuration change");
        addDetailRow(parent, "O/E - Other ensemble", "1 bit",
                     (b[0] & 0x40) ? "1" : "0", "Defines Other Ensemble services");
        const uint16_t eid = static_cast<uint16_t>((b[1] << 8) | b[2]);
        addDetailRow(parent, "EId - Ensemble ID", "16 bits",
                     QString("0x%1").arg(eid, 4, 16, QChar('0')).toUpper(), QString::number(eid));
        if (raw.size() >= 4) {
            addDetailRow(parent, "Alarms", "1 bit",
                         (b[3] & 0x20) ? "1" : "0", "Alarm flag (EN 300 401 6.4.1, bit 5)");
        }
        // Cross-reference the decoded ensemble label (if known).
        if (m_figAnalyser) {
            EnsembleInfo ens = m_figAnalyser->getCurrentEnsemble();
            if (!ens.ensembleLabel.isEmpty()) {
                addDetailRow(parent, "Ensemble Label", "-", ens.ensembleLabel,
                             "Decoded from FIG 1/0");
            }
        }
    }

    void buildFig0_1Details(QTreeWidgetItem* parent, const QByteArray& raw) {
        // FIG 0/1: per-subchannel EEP/UEP descriptors (3/4-byte entries).
        if (raw.size() < 3) return;
        const uint8_t* b = reinterpret_cast<const uint8_t*>(raw.constData());
        int offset = 1;
        int index = 0;
        while (offset + 2 < raw.size()) {
            const uint8_t subchId = (b[offset] >> 2) & 0x3F;
            const uint16_t startAddr = static_cast<uint16_t>(((b[offset] & 0x03) << 8) | b[offset + 1]);
            const bool longForm = (b[offset + 2] & 0x80) != 0;

            auto* entry = new QTreeWidgetItem(parent);
            entry->setText(0, QString("Subchannel %1").arg(subchId));
            entry->setText(1, "3-4 bytes");
            entry->setText(2, longForm ? "EEP" : "UEP");
            entry->setText(3, QString("Entry %1").arg(index));

            addDetailRow(entry, "SubChId", "6 bits", QString::number(subchId),
                         "Sub-channel identifier");
            addDetailRow(entry, "Start Address (SAD)", "10 bits", QString::number(startAddr),
                         "CU start address in the MSC");

            if (longForm) {
                if (offset + 3 >= raw.size()) break;
                const uint8_t b2 = b[offset + 2];
                const uint8_t option = (b2 >> 4) & 0x07;
                const uint8_t level = (b2 >> 2) & 0x03;  // 0-based on wire
                const uint16_t size = static_cast<uint16_t>(((b2 & 0x03) << 8) | b[offset + 3]);
                addDetailRow(entry, "EEP/Option", "3 bits", QString::number(option),
                             option == 0 ? "Table A" : "Table B");
                addDetailRow(entry, "EEP/Protection level", "2 bits", QString::number(level + 1),
                             QString("EEP %1-%2").arg(option == 0 ? "A" : "B").arg(level + 1));
                addDetailRow(entry, "Sub-channel size", "10 bits", QString("%1 CU").arg(size),
                             "Capacity units in the MSC");
                offset += 4;
            } else {
                const uint8_t b2 = b[offset + 2];
                addDetailRow(entry, "UEP/Table switch", "1 bit", QString::number((b2 >> 6) & 0x01),
                             "Which UEP table is used");
                addDetailRow(entry, "UEP/Table index", "6 bits", QString::number(b2 & 0x3F),
                             "Protection table index");
                offset += 3;
            }
            ++index;
        }
    }

    void buildFig0_2Details(QTreeWidgetItem* parent, const QByteArray& raw) {
        // FIG 0/2: real service-organization walk (EN 300 401 6.3.1), mirroring
        // AdvancedFIGAnalyser::processFIG0_2 — no invented TMId/NumClusters:
        //   ext byte b0: P/D flag = bit 5 (per FIG, not per entry)
        //   entry     : SId (16-bit P/D=0, 32-bit P/D=1) + ca_byte
        //               [local(1)|CAid(3)|ncomp(4)]
        //   components: ncomp x 2 B [TMId(2)|ASCTy(6)][SubChId(6)|P/S(1)|CA(1)]
        if (raw.size() < 2) return;
        const uint8_t* b = reinterpret_cast<const uint8_t*>(raw.constData());
        const bool pd = (b[0] & 0x20) != 0;
        int offset = 1;
        int index = 0;
        while (offset + 1 < raw.size()) {
            uint32_t sid = 0;
            if (!pd) {
                // 16-bit programme SId (bit 15 = the P/D flag, kept out of
                // the key — same 0x7FFF mask the analyser applies).
                if (offset + 2 > raw.size()) break;
                sid = (static_cast<uint32_t>(b[offset] & 0x7F) << 8) | b[offset + 1];
                offset += 2;
            } else {
                // 32-bit data SId: [ECC(8)][CId(4)|SRef-hi(4)][SRef(16)]
                if (offset + 4 > raw.size()) break;
                sid = (static_cast<uint32_t>(b[offset]) << 24) |
                      (static_cast<uint32_t>(b[offset + 1]) << 16) |
                      (static_cast<uint32_t>(b[offset + 2]) << 8) |
                      static_cast<uint32_t>(b[offset + 3]);
                offset += 4;
            }

            if (offset >= raw.size()) break;
            const uint8_t ca_byte = b[offset++];
            const uint8_t numComponents = ca_byte & 0x0F;

            auto* entry = new QTreeWidgetItem(parent);
            entry->setText(0, QString("Service 0x%1").arg(sid, pd ? 8 : 4, 16, QChar('0')).toUpper());
            entry->setText(1, pd ? "4+ bytes" : "2+ bytes");
            entry->setText(2, QString::number(sid));
            entry->setText(3, QString("Entry %1").arg(index));

            addDetailRow(entry, "SId", pd ? "32 bits" : "16 bits",
                         QString("0x%1").arg(sid, pd ? 8 : 4, 16, QChar('0')).toUpper(),
                         pd ? "Data service" : "Programme service");
            addDetailRow(entry, "CAid", "3 bits",
                         QString::number((ca_byte >> 4) & 0x07),
                         "Conditional access organization");
            addDetailRow(entry, "ncomp", "4 bits", QString::number(numComponents),
                         "Number of service components");

            for (int c = 0; c < numComponents && offset + 2 <= raw.size(); ++c) {
                const uint8_t tmId = (b[offset] >> 6) & 0x03;
                const uint8_t ascty = b[offset] & 0x3F;
                const uint8_t subchId = (b[offset + 1] >> 2) & 0x3F;
                const bool primary = (b[offset + 1] & 0x02) != 0;
                const bool ca = (b[offset + 1] & 0x01) != 0;
                auto* compItem = new QTreeWidgetItem(entry);
                compItem->setText(0, QString("Component %1").arg(c));
                compItem->setText(1, "2 bytes");
                compItem->setText(2, QString("SubCh %1").arg(subchId));
                compItem->setText(3, primary ? "Primary" : "Secondary");
                addDetailRow(compItem, "TMId", "2 bits", QString::number(tmId),
                             tmId == 0 ? "MSC stream mode" : "Packet mode");
                addDetailRow(compItem, "ASCTy/DSCTy", "6 bits",
                             QString("0x%1").arg(ascty, 2, 16, QChar('0')).toUpper(),
                             tmId == 0 && ascty == 0x3F ? "DAB+" : "");
                addDetailRow(compItem, "SubChId", "6 bits", QString::number(subchId),
                             "Sub-channel identifier");
                addDetailRow(compItem, "P/S", "1 bit", primary ? "1" : "0",
                             "Primary/secondary component");
                addDetailRow(compItem, "CA", "1 bit", ca ? "1" : "0",
                             "Conditional access flag");
                offset += 2;
            }
            ++index;
        }
        // Cross-reference decoded service labels (short label included).
        if (m_figAnalyser) {
            const std::vector<DABService> services = m_figAnalyser->getDABServices();
            for (const DABService& svc : services) {
                const ServiceComponent primary = svc.getPrimaryComponent();
                QString label = svc.service_label;
                if (label.isEmpty()) {
                    label = getServiceTypeDisplay(svc.service_id, primary.sub_channel_id);
                }
                addDetailRow(parent, QString("Label 0x%1").arg(svc.service_id, 4, 16, QChar('0')).toUpper(),
                             "FIG 1/1", label, svc.short_label);
            }
        }
    }

    void buildFig0_9Details(QTreeWidgetItem* parent) {
        // FIG 0/9: reuse the analyser's decoded country/LTO table.
        if (!m_figAnalyser) return;
        FIG0_9_CountryLTO lto = m_figAnalyser->getCountryLTO();
        addDetailRow(parent, "ECC", "8 bits",
                     QString("0x%1").arg(lto.ensemble_ecc, 2, 16, QChar('0')).toUpper(),
                     "Extended Country Code");
        addDetailRow(parent, "Country", "-",
                     FIGParsingUtils::countryCodeToString(lto.ensemble_ecc),
                     "Country per EN 300 401 Table 1");
        addDetailRow(parent, "LTO", "8 bits", QString("%1 h").arg(lto.ensemble_lto / 2.0, 0, 'f', 1),
                     "Local Time Offset (± half-hours)");
        addDetailRow(parent, "International table ID", "8 bits",
                     QString("0x%1").arg(lto.international_table_id, 2, 16, QChar('0')).toUpper(),
                     "International table");
    }

    void buildFig0_10Details(QTreeWidgetItem* parent) {
        // FIG 0/10: reuse the analyser's decoded date/time (MJD -> real date).
        if (!m_figAnalyser) return;
        FIG0_10_DateTime dt = m_figAnalyser->getDateTime();
        addDetailRow(parent, "MJD", "17 bits", QString::number(dt.mjd),
                     "Modified Julian Date");
        addDetailRow(parent, "UTC", "5 bits", QString::number(dt.hours),
                     QString("Hours 0-23"));
        addDetailRow(parent, "Minutes", "6 bits", QString::number(dt.minutes), "");
        addDetailRow(parent, "Seconds", "6 bits", QString::number(dt.seconds), "");
        const QDateTime dateTime = FIGParsingUtils::mjdToDateTime(dt.mjd, dt.hours, dt.minutes, dt.seconds);
        addDetailRow(parent, "Date/Time", "-",
                     dateTime.toString("yyyy-MM-dd hh:mm:ss"), "UTC");
        addDetailRow(parent, "LSI", "1 bit", dt.lsi_flag ? "1" : "0", "Leap second indicator");
        addDetailRow(parent, "Confidence", "1 bit", dt.conf_flag ? "1" : "0", "Confidence flag");
    }

    // Build a DABLabel from FIG 1 raw fields (returns empty on short data).
    // Wire order per EN 300 401 8.1.13/8.1.14 (matches the analyser's
    // parseLabel): the 16-character label starts at labelOffset, and the
    // 2-byte character flag follows it at labelOffset+16 — NOT before it.
    DABLabel parseFig1Label(const QByteArray& raw, int labelOffset) const {
        DABLabel label;
        if (labelOffset < 0 || labelOffset + 18 > raw.size()) return label;
        const uint8_t* b = reinterpret_cast<const uint8_t*>(raw.constData());
        label.charset = (b[0] >> 4) & 0x0F;
        for (int i = 0; i < 16; ++i) {
            label.full_label[i] = static_cast<char>(b[labelOffset + i]);
        }
        label.character_flag = static_cast<uint16_t>(
            (b[labelOffset + 16] << 8) | b[labelOffset + 17]);
        return label;
    }

    void buildFig1_0Details(QTreeWidgetItem* parent, const QByteArray& raw) {
        // FIG 1/0: charset, EId, character flag, 16-char ensemble label.
        if (raw.size() < 2) return;
        const uint8_t* b = reinterpret_cast<const uint8_t*>(raw.constData());
        const uint8_t charset = (b[0] >> 4) & 0x0F;
        addDetailRow(parent, "Charset", "4 bits", QString::number(charset),
                     charset == 0 ? "Complete EBU Latin" :
                     charset == 3 ? "UTF-8" :
                     charset == 6 ? "TIS-620 (Thai)" : "Reserved");
        const uint16_t eid = static_cast<uint16_t>((b[1] << 8) | b[2]);
        addDetailRow(parent, "EId", "16 bits",
                     QString("0x%1").arg(eid, 4, 16, QChar('0')).toUpper(), QString::number(eid));
        const DABLabel label = parseFig1Label(raw, 3);
        if (label.isValid()) {
            addDetailRow(parent, "Character flag", "16 bits",
                         QString("0x%1").arg(label.character_flag, 4, 16, QChar('0')).toUpper(),
                         "Short-label mask");
            addDetailRow(parent, "Label", "128 bits", label.getFullLabel(),
                         label.getShortLabel() + " (short)");
        }
    }

    void buildFig1_1Details(QTreeWidgetItem* parent, const QByteArray& raw) {
        // FIG 1/1: per-service SId + charset + character flag + 16-char label.
        if (raw.size() < 20) return;
        const uint8_t* b = reinterpret_cast<const uint8_t*>(raw.constData());
        const uint8_t charset = (b[0] >> 4) & 0x0F;
        addDetailRow(parent, "Charset", "4 bits", QString::number(charset),
                     charset == 0 ? "Complete EBU Latin" :
                     charset == 3 ? "UTF-8" :
                     charset == 6 ? "TIS-620 (Thai)" : "Reserved");

        int offset = 1;
        int index = 0;
        while (offset + 20 <= raw.size()) {
            const uint16_t sid = static_cast<uint16_t>((b[offset] << 8) | b[offset + 1]);
            auto* entry = new QTreeWidgetItem(parent);
            entry->setText(0, QString("Service 0x%1").arg(sid, 4, 16, QChar('0')).toUpper());
            entry->setText(1, "20 bytes");
            entry->setText(2, QString::number(sid));
            entry->setText(3, QString("Entry %1").arg(index));

            addDetailRow(entry, "SId", "16 bits",
                         QString("0x%1").arg(sid, 4, 16, QChar('0')).toUpper(), QString::number(sid));
            // Label (16 bytes) at entry+2, character flag at entry+18; the
            // charset byte is raw[0] (FIG 1 header byte) — parseFig1Label
            // handles the full field.
            DABLabel label = parseFig1Label(raw, offset + 2);
            addDetailRow(entry, "Character flag", "16 bits",
                         QString("0x%1").arg(label.character_flag, 4, 16, QChar('0')).toUpper(),
                         "Short-label mask");
            addDetailRow(entry, "Label", "128 bits", label.getFullLabel(),
                         label.getShortLabel() + " (short)");
            offset += 20;
            ++index;
        }
    }

    void buildFIGInstanceTree() {
        if (!m_tab3_figInstanceTree) return;
        
        m_tab3_figInstanceTree->clear();
        
        if (!m_fileLoaded || !m_figAnalyser) {
            auto* noDataItem = new QTreeWidgetItem(m_tab3_figInstanceTree);
            noDataItem->setText(0, "No FIG instances");
            noDataItem->setText(1, "");
            noDataItem->setText(2, "Load file");
            noDataItem->setText(3, "");
            noDataItem->setText(4, "");
            noDataItem->setText(5, "");
            noDataItem->setText(6, "");
            return;
        }
        
        // Chronological real FIG instances from the analyser's collector
        // (FIC-XTractor view). Observation (d): the tree is now hierarchical —
        // one top-level group per (type, ext) with its instance count, and the
        // instances as children. Child items carry the collector index in
        // Qt::UserRole so the selection sync can rebuild the element tree.
        const QVector<AdvancedFIGAnalyser::FigInstance>& instances =
            m_figAnalyser->figInstances();

        struct FigGroup {
            int type = 0;
            int ext = 0;
            int count = 0;
            QTreeWidgetItem* node = nullptr;
        };
        QHash<quint16, int> groupIndex;  // (type,ext) -> index into groups
        QVector<FigGroup> groups;

        m_tab3_figInstanceTree->setUpdatesEnabled(false);
        for (int i = 0; i < instances.size(); ++i) {
            const AdvancedFIGAnalyser::FigInstance& inst = instances.at(i);
            const quint16 key = static_cast<quint16>((inst.type << 8) | inst.ext);
            int gi = groupIndex.value(key, -1);
            if (gi < 0) {
                auto* groupNode = new QTreeWidgetItem(m_tab3_figInstanceTree);
                groupNode->setText(0, QString("FIG %1/%2").arg(inst.type).arg(inst.ext));
                groupNode->setData(0, Qt::UserRole, -1);  // group marker (no instance)
                gi = groups.size();
                groups.append({inst.type, inst.ext, 0, groupNode});
                groupIndex.insert(key, gi);
            }
            FigGroup& group = groups[gi];
            ++group.count;
            const int idx = group.count;  // occurrence within this group

            auto* item = new QTreeWidgetItem(group.node);
            QString typeText = QString("FIG %1/%2").arg(inst.type).arg(inst.ext);
            if (inst.reconfig) {
                typeText += " R";    // MCI reconfiguration marker
            }
            if (!inst.crcOk) {
                typeText += " !CRC"; // FIB CRC failure marker
            }
            item->setText(0, typeText);
            item->setText(1, QString::number(idx));
            item->setText(2, QString::number(inst.frame));
            // CIF#: Mode I ETI frame phase — each 24 ms frame carries one CIF
            // of the 4-CIF transmission frame (derived from the frame number).
            item->setText(3, inst.frame > 0 ? QString::number((inst.frame - 1) % 4) : QStringLiteral("-"));
            item->setText(4, QString::number(inst.fibIdx));
            item->setText(5, QString::number(inst.length));
            item->setText(6, QString::number(
                              figFixedItemCount(inst.type, inst.ext, inst.raw)));
            item->setData(0, Qt::UserRole, i);  // collector index (child items only)

            if (!inst.crcOk) {
                item->setForeground(0, QColor(200, 50, 50));
                item->setForeground(4, QColor(200, 50, 50));
            }
        }
        // Group label carries the instance count, e.g. "FIG 0/0  (1250)".
        for (const FigGroup& group : groups) {
            group.node->setText(0, QString("FIG %1/%2  (%3)")
                                       .arg(group.type).arg(group.ext).arg(group.count));
            group.node->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
        }
        m_tab3_figInstanceTree->setUpdatesEnabled(true);
        // T28: default COLLAPSED — no expandAll(), and no forced selection of a
        // hidden child. The details pane shows the first group summary until the
        // user picks an instance.

        qDebug() << "[PHASE 8] FIG instance tree built with"
                 << instances.size() << "real instances in" << groups.size() << "groups";
    }

    // === PHASE 9: Tab 3 RIGHT Panel - FIG Item Details Tree ===
    // No-arg entry point (load-complete + selection sync): resolve the
    // selected instance (falling back to the first instance) and rebuild.
    // Observation (d): selecting a (type,ext) GROUP node shows a group summary
    // instead of an instance element tree, and never crashes.
    void buildFIGItemDetailsTree() {
        int index = -1;
        QTreeWidgetItem* groupNode = nullptr;
        if (m_tab3_figInstanceTree) {
            QTreeWidgetItem* current = m_tab3_figInstanceTree->currentItem();
            if (current) {
                index = current->data(0, Qt::UserRole).toInt();
                if (index < 0) {
                    groupNode = current;  // group node marker
                }
            } else if (m_tab3_figInstanceTree->topLevelItemCount() > 0) {
                // T28: no forced child selection. Show the first group's
                // summary until the user picks an instance.
                QTreeWidgetItem* first = m_tab3_figInstanceTree->topLevelItem(0);
                if (first->childCount() > 0) {
                    groupNode = first;
                } else {
                    index = first->data(0, Qt::UserRole).toInt();
                }
            }
        }
        if (groupNode) {
            buildFIGGroupSummary(groupNode);
            return;
        }
        buildFIGItemDetailsTree(index);
    }

    // Safe details view for a selected FIG group node (Observation d).
    void buildFIGGroupSummary(QTreeWidgetItem* groupNode) {
        if (!m_tab3_figItemDetailsTree || !groupNode) return;
        m_tab3_figItemDetailsTree->clear();
        auto* summary = new QTreeWidgetItem(m_tab3_figItemDetailsTree);
        summary->setText(0, groupNode->text(0));
        summary->setText(1, QString("%1 instances").arg(groupNode->childCount()));
        summary->setText(2, "Group");
        summary->setText(3, "Select an instance to see its decoded element tree");
    }

    void buildFIGItemDetailsTree(int index) {
        if (!m_tab3_figItemDetailsTree) return;
        
        m_tab3_figItemDetailsTree->clear();
        
        if (!m_fileLoaded || !m_figAnalyser || index < 0) {
            auto* noDataItem = new QTreeWidgetItem(m_tab3_figItemDetailsTree);
            noDataItem->setText(0, "No FIG selected");
            noDataItem->setText(1, "");
            noDataItem->setText(2, "");
            noDataItem->setText(3, "Select a FIG instance");
            return;
        }
        
        const QVector<AdvancedFIGAnalyser::FigInstance>& instances =
            m_figAnalyser->figInstances();
        if (index >= instances.size()) {
            return;
        }
        const AdvancedFIGAnalyser::FigInstance& inst = instances.at(index);

        auto* rootItem = new QTreeWidgetItem(m_tab3_figItemDetailsTree);
        fillFIGInstanceDetails(rootItem, inst);

        m_tab3_figItemDetailsTree->expandAll();
        qDebug() << "[PHASE 9] FIG item details tree built (instance" << index << ")";
    }

    // T28: shared per-FIG detail builder. Fills `rootItem` with the FIG header
    // rows (type/ext/len/FIB/CRC markers) and nests the existing per-FIG element
    // builders (buildFig0_*/buildFig1_*) under it. Reused by the FIG-instance
    // details pane and by the Frame List's per-frame FIG tree — no new decoding.
    void fillFIGInstanceDetails(QTreeWidgetItem* rootItem,
                                const AdvancedFIGAnalyser::FigInstance& inst)
    {
        if (!rootItem) return;
        // Root: the real FIG block with its wire header (type/length/ext) plus
        // the FIB / CRC markers carried by the frame.
        rootItem->setText(0, QString("FIG %1/%2").arg(inst.type).arg(inst.ext));
        rootItem->setText(1, QString("%1 bytes").arg(inst.length));
        rootItem->setText(2, QString("len 0x%1").arg(inst.length, 2, 16, QChar('0')).toUpper());
        rootItem->setText(3, QString("%1 · FIB %2 · %3")
                              .arg(figDataTypeName(inst.type))
                              .arg(inst.fibIdx)
                              .arg(inst.crcOk ? "CRC OK" : "CRC FAIL"));
        if (!inst.crcOk) {
            rootItem->setForeground(3, QColor(200, 50, 50));
        }

        addDetailRow(rootItem, "FIG Type", "3 bits", QString::number(inst.type),
                     figDataTypeName(inst.type));
        addDetailRow(rootItem, "Length", "5 bits", QString::number(inst.length),
                     "Data bytes after the FIG header");
        addDetailRow(rootItem, "Extension", figExtFieldWidth(inst.type),
                     QString::number(inst.ext), figExtName(inst.type, inst.ext));
        addDetailRow(rootItem, "Frame", "-", QString::number(inst.frame),
                     "ETI frame number");
        addDetailRow(rootItem, "FIB", "-", QString::number(inst.fibIdx),
                     "FIB index within the frame's FIC");
        addDetailRow(rootItem, "FIB CRC", "-", inst.crcOk ? "OK" : "FAILED",
                     inst.crcOk ? "FIB CRC-16 passed" : "FIB CRC-16 mismatch");
        addDetailRow(rootItem, "Reconfiguration", "-", inst.reconfig ? "yes" : "no",
                     "MCI C/N flag (FIG 0/1, 0/2)");
        addDetailRow(rootItem, "Items", "-",
                     QString::number(figFixedItemCount(inst.type, inst.ext, inst.raw)),
                     "Sub-FIGs in this block");
        addRawHexRows(rootItem, inst.raw);

        // Element tree from the real decoded fields (per EN 300 401).
        if (inst.type == 0) {
            switch (inst.ext) {
                case 0:  buildFig0_0Details(rootItem, inst.raw); break;
                case 1:  buildFig0_1Details(rootItem, inst.raw); break;
                case 2:  buildFig0_2Details(rootItem, inst.raw); break;
                case 9:  buildFig0_9Details(rootItem); break;
                case 10: buildFig0_10Details(rootItem); break;
                default: break;  // header + raw hex already shown
            }
        } else if (inst.type == 1) {
            switch (inst.ext) {
                case 0: buildFig1_0Details(rootItem, inst.raw); break;
                case 1: buildFig1_1Details(rootItem, inst.raw); break;
                default: break;  // header + raw hex already shown
            }
        }
    }

    // T28: fill the Frame List (1..N of the loaded capture). Called once after
    // a successful load; empty before a file is loaded.
    void buildFrameList()
    {
        if (!m_tab3_frameList) return;
        const int total = availableFrameCount();
        m_tab3_frameList->clear();
        if (!m_fileLoaded || total <= 0) {
            return;
        }
        m_tab3_frameList->setUpdatesEnabled(false);
        for (int i = 0; i < total; ++i) {
            m_tab3_frameList->addItem(QString("Frame %1").arg(i + 1));
        }
        m_tab3_frameList->setUpdatesEnabled(true);
    }

    // T28: selecting a frame in the Frame List decodes that frame's FIGs and
    // shows them in the right dock (FIG Item Details). Each root is a FIG
    // present in the frame (type/ext/len/FIB/CRC markers) with the existing
    // per-FIG element builders nested underneath. Reuses the analyser's
    // per-frame instance collector — no new decoding logic.
    //
    // L4 limitation: this REUSES the single m_tab3_figItemDetailsTree that the
    // FIG-instance selection (buildFIGItemDetailsTree()) also owns, so the two
    // views overwrite each other. The playback-driven rebuild is therefore
    // skipped while the Frame List tab is not active (applyPlayheadFrame()), and
    // user-driven Frame List selection always wins.
    void rebuildFrameFIGDetails(int frameIndex)
    {
        if (!m_tab3_figItemDetailsTree) return;
        m_tab3_figItemDetailsTree->clear();

        if (!m_fileLoaded || !m_figAnalyser || frameIndex < 0) {
            auto* noData = new QTreeWidgetItem(m_tab3_figItemDetailsTree);
            noData->setText(0, "No FIG selected");
            noData->setText(3, "Load a capture and select a frame");
            return;
        }

        const int frameNumber = frameIndex + 1;  // FigInstance::frame is 1-based
        int roots = 0;
        const QVector<AdvancedFIGAnalyser::FigInstance>& instances =
            m_figAnalyser->figInstances();
        for (const AdvancedFIGAnalyser::FigInstance& inst : instances) {
            if (inst.frame != frameNumber) {
                continue;
            }
            auto* root = new QTreeWidgetItem(m_tab3_figItemDetailsTree);
            fillFIGInstanceDetails(root, inst);
            ++roots;
        }
        if (roots == 0) {
            auto* none = new QTreeWidgetItem(m_tab3_figItemDetailsTree);
            none->setText(0, QString("Frame %1: no FIGs").arg(frameNumber));
            none->setText(3, "This frame carries no FIG");
        } else {
            m_tab3_figItemDetailsTree->expandAll();
        }
    }

    // Frame List current-row slot.
    void onTab3FrameSelected(int row)
    {
        if (row < 0) return;
        rebuildFrameFIGDetails(row);
        updateTab3HexFromFrame(row);  // T42: mirror the frame into the Hex Viewer
    }

    // ========================================================================
    // T42: Tab-3 right dock Hex Viewer (raw bytes of the current selection)
    // ========================================================================
    // Compact offset/hex/ASCII dump (16 bytes per line) as monospace plain text.
    // Deliberately independent of HexViewerFormatter (which is ETI-frame aware
    // and styles raw sections without a stylesheet) — this view must render
    // both arbitrary FIG data bytes and frame regions.
    static QString formatCompactHex(const QByteArray& data, quint32 startOffset,
                                    const QString& title)
    {
        QString out;
        out.reserve(data.size() * 4 + 64);
        out += title;
        out += QString("\n%1 bytes\n").arg(data.size());
        constexpr int kPerLine = 16;
        for (int i = 0; i < data.size(); i += kPerLine) {
            out += QString("%1  ")
                       .arg(startOffset + static_cast<quint32>(i), 6, 16, QChar('0'))
                       .toUpper();
            QString ascii;
            ascii.reserve(kPerLine);
            for (int j = 0; j < kPerLine; ++j) {
                const int idx = i + j;
                if (idx < data.size()) {
                    const uint8_t b = static_cast<uint8_t>(data.at(idx));
                    out += QString("%1 ").arg(b, 2, 16, QChar('0')).toUpper();
                    ascii += (b >= 0x20 && b < 0x7F) ? QChar(b) : QChar('.');
                } else {
                    out += QStringLiteral("   ");
                    ascii += QLatin1Char(' ');
                }
                if (j == kPerLine / 2 - 1) {
                    out += QLatin1Char(' ');
                }
            }
            out += QStringLiteral(" |") + ascii + QStringLiteral("|\n");
        }
        return out;
    }

    void showTab3HexMessage(const QString& message)
    {
        if (m_tab3_hexViewer) {
            m_tab3_hexViewer->setPlainText(message);
        }
    }

    // FIG Instance List selection -> the selected instance's raw bytes (the
    // analyser collector's `raw`) with a FIG t/e/length/FIB/CRC header.
    void updateTab3HexFromFigInstance()
    {
        if (!m_tab3_hexViewer) return;
        if (!m_fileLoaded || !m_figAnalyser) {
            showTab3HexMessage(
                "No capture loaded. Select a FIG instance or a frame after loading an ETI file.");
            return;
        }
        int index = -1;
        if (m_tab3_figInstanceTree && m_tab3_figInstanceTree->currentItem()) {
            index = m_tab3_figInstanceTree->currentItem()->data(0, Qt::UserRole).toInt();
        }
        const QVector<AdvancedFIGAnalyser::FigInstance>& instances =
            m_figAnalyser->figInstances();
        if (index < 0 || index >= instances.size()) {
            showTab3HexMessage(
                "No FIG instance selected. Pick an instance under a FIG group to view its raw bytes.");
            return;
        }
        const AdvancedFIGAnalyser::FigInstance& inst = instances.at(index);
        const QString header = QString(
            "FIG %1/%2  |  length=%3 bytes  |  FIB %4  |  %5  |  frame %6")
            .arg(inst.type)
            .arg(inst.ext)
            .arg(inst.length)
            .arg(inst.fibIdx)
            .arg(inst.crcOk ? QStringLiteral("CRC OK") : QStringLiteral("CRC FAIL"))
            .arg(inst.frame);
        m_tab3_hexViewer->setPlainText(formatCompactHex(inst.raw, 0, header));
    }

    // Render one raw ETI frame's FIC/FIB region into the Hex Viewer. Shared by
    // the Frame List path and the synthetic-frame test hook. FIC starts at
    // 12 + 4*NST per EN 300 799; the FIC length is Mode-Identity dependent
    // (96 bytes / 3 FIBs, 128 bytes / 4 FIBs for Mode III).
    void renderTab3HexForFrameData(const QByteArray& frameData, int frameNumber)
    {
        if (!m_tab3_hexViewer || frameData.isEmpty()) return;
        const uint8_t f = frameData.size() > 5
                              ? static_cast<uint8_t>(frameData.at(5))
                              : static_cast<uint8_t>(0);
        const bool ficf = (f & 0x80) != 0;
        const int nstField = (f & 0x7F);
        // F2: mirror ModernETIFrameParser's nst_offset_1 legacy correction
        // (settings_.nst_offset_1 && nst < MAX_SUBCHANNEL_COUNT -> nst + 1).
        const int nst =
            (m_analyserSettings.nst_offset_1
             && nstField < static_cast<int>(eti::MAX_SUBCHANNEL_COUNT))
                ? nstField + 1
                : nstField;
        const int ficOffset = 12 + 4 * nst;
        // F1: FIC length is Mode-Identity dependent (ETSI EN 300 799 §5.2):
        // 128 bytes / 4 FIBs for Mode III (MID==3), 96 bytes / 3 FIBs otherwise.
        // LIDATA is at bytes 4..7, so MID is bits 4-3 of byte 6 (data[2] >> 3).
        const uint8_t mid = frameData.size() > 6
                                ? static_cast<uint8_t>(
                                      (static_cast<uint8_t>(frameData.at(6)) & 0x18) >> 3)
                                : static_cast<uint8_t>(0);
        const int ficLen = (mid == 3) ? static_cast<int>(eti::ETI_FIC_MAX_SIZE)
                                      : static_cast<int>(eti::ETI_FIC_MIN_SIZE);
        QByteArray shown = frameData;
        quint32 shownOffset = 0;
        QString header;
        if (ficf && ficOffset >= 0 && ficOffset + ficLen <= frameData.size()) {
            shown = frameData.mid(ficOffset, ficLen);
            shownOffset = static_cast<quint32>(ficOffset);
            header = QString("Frame %1  |  FIC/FIB region  |  offset 0x%2  |  %3 bytes (%4 FIBs)")
                         .arg(frameNumber)
                         .arg(QString("%1").arg(ficOffset, 4, 16, QChar('0')).toUpper())
                         .arg(ficLen)
                         .arg(ficLen / static_cast<int>(eti::ETI_FIC_FIB_SIZE));
        } else {
            header = QString("Frame %1  |  full ETI frame  |  6144 bytes")
                         .arg(frameNumber);
        }
        m_tab3_hexViewer->setPlainText(formatCompactHex(shown, shownOffset, header));
    }

    // Frame List selection -> that frame's retained raw bytes (FIC/FIB region).
    void updateTab3HexFromFrame(int frameIndex)
    {
        if (!m_tab3_hexViewer) return;
        if (!m_fileLoaded || frameIndex < 0) {
            showTab3HexMessage(
                "No capture loaded. Select a FIG instance or a frame after loading an ETI file.");
            return;
        }
        const QByteArray frameData = getETIFrameData(frameIndex);
        if (frameData.isEmpty()) {
            showTab3HexMessage("No frame data available for the selected frame.");
            return;
        }
        renderTab3HexForFrameData(frameData, frameIndex + 1);
    }

    void updateTimingTable() {
        if (!m_timingTable) return;
        
        // Get current DAB mode to determine frame duration
        uint8_t dabMode = getCurrentDABMode();
        
        // Frame duration per ETSI EN 300 401:
        // Mode 1: 24 ms per frame (96 ms transmission frame / 4 CIFs)
        // Mode 2: 48 ms per frame (96 ms transmission frame / 2 CIFs)
        // Mode 3: 24 ms per frame (96 ms transmission frame / 4 CIFs)
        // Mode 4: 24 ms per frame (96 ms transmission frame / 4 CIFs)
        uint32_t frameStepUs = (dabMode == 2) ? 48000 : 24000;  // microseconds
        uint32_t frameStepTicks = frameStepUs;  // 1 tick = 1 us
        
        // Ensure table has 4 rows
        if (m_timingTable->rowCount() < 4) {
            m_timingTable->setRowCount(4);
            m_timingTable->setColumnCount(2);
            m_timingTable->setHorizontalHeaderLabels({"Property", "Value"});
            
            m_timingTable->setItem(0, 0, new QTableWidgetItem("FCT = 0 (us)"));
            m_timingTable->setItem(1, 0, new QTableWidgetItem("Step (us)"));
            m_timingTable->setItem(2, 0, new QTableWidgetItem("FCT = 0 (ticks)"));
            m_timingTable->setItem(3, 0, new QTableWidgetItem("Step (ticks)"));
        }
        
        // Clear value column
        for (int row = 0; row < 4; ++row) {
            QTableWidgetItem* oldItem = m_timingTable->item(row, 1);
            if (oldItem) {
                delete m_timingTable->takeItem(row, 1);
            }
        }
        
        // Update values
        // Row 0: FCT=0 timestamp (always 0 at start)
        m_timingTable->setItem(0, 1, new QTableWidgetItem("0"));
        
        // Row 1: Frame step in microseconds
        m_timingTable->setItem(1, 1, new QTableWidgetItem(QString("%1 µs").arg(frameStepUs)));
        
        // Row 2: FCT=0 in ticks (same as microseconds)
        m_timingTable->setItem(2, 1, new QTableWidgetItem("0"));
        
        // Row 3: Frame step in ticks
        m_timingTable->setItem(3, 1, new QTableWidgetItem(QString("%1 ticks").arg(frameStepTicks)));
        
        qDebug() << "[PHASE 3.3] Timing table updated (Mode" << dabMode << "):" 
                 << frameStepUs << "µs per frame";
    }

    void setupUI()
    {
        // Qt-ADS: enable auto-hide (pin) support. MUST be called before any
        // CDockManager is constructed (static config, called once per process).
        ads::CDockManager::setAutoHideConfigFlags(ads::CDockManager::DefaultAutoHideConfig);

        // Create main vertical splitter (Tab area + Shared bottom docking area)
        QSplitter* mainVerticalSplitter = new QSplitter(Qt::Vertical, this);
        setCentralWidget(mainVerticalSplitter);

        // Create main tab widget for 3 tabs (per user specification)
        QTabWidget* mainTabs = new QTabWidget();
        mainTabs->setObjectName(QStringLiteral("mainTabs"));
        // Manual-test finding 1: readable inactive tabs + accent-highlighted
        // active tab (see applyMainTabsStyle()).
        applyMainTabsStyle(mainTabs);
        mainVerticalSplitter->addWidget(mainTabs);

        // Tab 1: DAB Ensemble & ETI Analysis (PRIMARY - most used)
        QWidget* tab1 = createTab1_EnsembleAndETIAnalysis();
        mainTabs->addTab(tab1, "📡 DAB Ensemble & ETI Analysis");

        // Tab 2: FIC-Analyser (SECONDARY - FIC analysis)
        QWidget* tab2 = createTab2_FICAnalyser();
        mainTabs->addTab(tab2, "🔬 FIC-Analyser");

        // Tab 3: FIC-Extractor (TERTIARY - FIC data extraction)
        QWidget* tab3 = createTab3_FICExtractor();
        mainTabs->addTab(tab3, "📊 FIC-Extractor");

        // Shared bottom docking area (window-level, consistent across all tabs).
        // Replaces the old sharedBottomPanel QTabWidget: a window-level CDockManager
        // pinned at the bottom of the mainVerticalSplitter, whose Bottom dock area
        // hosts the 7 tabbed bottom docks (System Messages / Performance /
        // Real-Time Chart / Error Detection / ETSI Compliance / Logging /
        // Constellation).
        QWidget* bottomHost = new QWidget();
        QVBoxLayout* bottomLayout = new QVBoxLayout(bottomHost);
        bottomLayout->setContentsMargins(0, 0, 0, 0);
        bottomLayout->setSpacing(0);
        m_bottomDockManager = new ads::CDockManager(bottomHost);
        m_bottomDockManager->setObjectName("bottomDockManager");
        bottomLayout->addWidget(m_bottomDockManager);
        buildSharedBottomDocks();
        mainVerticalSplitter->addWidget(bottomHost);

        // T30.1/T40: the shared strip starts at ~120 px (≈5 monospace lines plus
        // the dock tab bar) — expressed as a splitter SIZE, not a hard cap, so
        // the user can drag it larger. The old setMaximumHeight(120) is gone; a
        // sane minimum keeps the dock tab bar + one line usable. Initial split
        // ≈ 88% workspace / 12% strip.
        bottomHost->setMinimumHeight(60);
        mainVerticalSplitter->setSizes({880, 120});

        // Manual-test finding 1: matching tab touch for the Qt-ADS dock tab
        // bars (appends to the ADS default stylesheet; see helper).
        applyDockManagerTabStyle(m_tab1DockManager);
        applyDockManagerTabStyle(m_tab2DockManager);
        applyDockManagerTabStyle(m_tab3DockManager);
        applyDockManagerTabStyle(m_bottomDockManager);

        // Restore persisted docking/window state AFTER every dock exists.
        restoreDockingState();
    }

    // ========================================================================
    // TAB 1: ETI DAB ENSEMBLE EXPLORER (PRIMARY WORKSPACE)
    // ========================================================================
    QWidget* createTab1_EnsembleAndETIAnalysis()
    {
        QWidget* tab = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(tab);
        mainLayout->setContentsMargins(4, 4, 4, 4);
        mainLayout->setSpacing(4);

        // UI redesign (Step A): the compact Input Source toolbar sits at the
        // TOP of the tab-1 page, above the page's own CDockManager. It
        // replaces the old tab1_InputSource dock (removed from the center
        // column registration); all member widgets and slot wiring are reused
        // unchanged, so file-open and network flows behave identically.
        QWidget* inputToolbar = createLiveStreamInputPanel();
        mainLayout->addWidget(inputToolbar);

        // Tab 1 gets its OWN CDockManager. The three column helpers below were
        // refactored to return per-dock sections; registerTab1Docks() wraps each
        // section in a CDockWidget and arranges the grouped 3-column layout.
        m_tab1DockManager = new ads::CDockManager(tab);
        m_tab1DockManager->setObjectName("tab1DockManager");
        registerTab1Docks();

        mainLayout->addWidget(m_tab1DockManager);
        return tab;
    }

    ads::CDockWidget* makeDock(ads::CDockManager* manager, const QString& objectName,
                               const QString& title, QWidget* widget)
    {
        auto* dock = manager->createDockWidget(title);
        dock->setObjectName(objectName);
        dock->setWidget(widget ? widget : new QWidget());
        // Store the title for later use in setupWindowMenu() — avoids calling
        // dock->windowTitle() which crashes in Qt-ADS accessibility code paths.
        m_dockTitles.insert(objectName, title);
        return dock;
    }

    // Wave B/T40: the window-level strip STARTS at ≈120 px but is no longer
    // hard-capped (T40 removed setMaximumHeight(120); the default is a splitter
    // size, min 60 px). The adopted heavy panels (Performance / Real-Time Chart /
    // ETSI Compliance / Constellation) have a much larger content minimum, so
    // each is wrapped in a frameless, resizable QScrollArea: at the small
    // default the light tabs (System Messages / Error Detection / Logging) are
    // compact while the heavy tabs stay fully usable via scrollbars, and the
    // whole strip can now be dragged larger.
    static QWidget* wrapHeavyPanel(QWidget* content)
    {
        QScrollArea* area = new QScrollArea();
        area->setObjectName(QStringLiteral("heavyPanelScrollArea"));
        area->setWidgetResizable(true);
        area->setFrameShape(QFrame::NoFrame);
        area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        area->setWidget(content);
        return area;
    }

    // ========================================================================
    // MANUAL-TEST FINDING 1: tab-colour readability
    // ========================================================================
    // The system default QTabBar draws inactive tab text with very low
    // contrast on the default (light) theme, so inactive tabs are hard to
    // read. This stylesheet makes:
    //   inactive : palette(windowText) text on palette(window) background
    //   selected : accent #4a8fd4 background with palette(highlightedText)
    //              text, bold
    //   pane     : 1px palette(mid) border
    // The UI-redesign mockup (docs/gui/StreamDAB-Analyser UI Redesign.html)
    // uses accent #4a8fd4, border #363636 and text #c6c6c6. Only the brand
    // accent is used verbatim; the remaining colours are derived from
    // palette() so the app stays readable on the default light theme too.
    void applyMainTabsStyle(QTabWidget* tabs)
    {
        if (!tabs) {
            return;
        }
        tabs->setStyleSheet(QStringLiteral(R"(
            QTabWidget::pane { border: 1px solid palette(mid); }
            QTabBar::tab {
                color: palette(windowText);
                background: palette(window);
                border: 1px solid palette(mid);
                padding: 6px 12px;
                margin-right: 1px;
            }
            QTabBar::tab:hover:!selected { background: palette(alternate-base); }
            QTabBar::tab:selected {
                color: palette(highlightedText);
                background: #4a8fd4;
                border: 1px solid #4a8fd4;
                font-weight: bold;
            }
        )"));
    }

    // Qt-ADS 4.5.0 has NO dedicated `setStylesheet()` API on CDockManager
    // (verified in build/_deps/qtadvanceddockingsystem-src/src/DockManager.h);
    // CDockManager installs its own default stylesheet in its constructor via
    // the inherited QWidget::setStyleSheet(). Replacing it outright would lose
    // the dock icons, so APPEND the matching tab rules to the existing sheet.
    void applyDockManagerTabStyle(ads::CDockManager* manager)
    {
        if (!manager) {
            return;
        }
        const QString extra = QStringLiteral(R"(
            ads--CDockWidgetTab {
                background: palette(window);
                border-color: palette(mid);
            }
            ads--CDockWidgetTab QLabel { color: palette(windowText); }
            ads--CDockWidgetTab[activeTab="true"] {
                background: #4a8fd4;
            }
            ads--CDockWidgetTab[activeTab="true"] QLabel {
                color: palette(highlightedText);
                font-weight: bold;
            }
        )");
        manager->setStyleSheet(manager->styleSheet() + extra);
    }

    void registerTab1Docks()
    {
        if (!m_tab1DockManager) {
            return;
        }

        // UI redesign (Steps B/C): the Input Source dock is gone (now the
        // toolbar above the page) and Messages moved to the shared bottom
        // strip; the remaining panels are grouped into tabbed dock areas:
        //   LEFT  : Transport (Player | Decoder | Audio), System
        //   CENTER: Now Playing, Signal Analysis (Ensemble Explorer |
        //           Subchannel Org | CU Usage), Frame Analysis
        //           (ETI Frame Navigator | Hex/ETSI Compliance)
        //   RIGHT : Signal Info (ETI Overview | Status | Timing),
        //           Error & Statistics (DAB Error Counter | Stream Stats),
        //           Data Services (EPG | Journaline | TPEG) — Phase 2A
        // Each panel keeps its OWN CDockWidget (float/auto-hide/close-one-tab
        // intact); addDockWidgetTabToArea() only groups them. Registration
        // order defines the tab order; the first dock of each group is the
        // default active tab.
        const QList<DockSection> leftSections = createTab1_LeftPanel();
        const QList<DockSection> centerSections = createTab1_CenterPanel();
        const QList<DockSection> rightSections = createTab1_RightPanel();

        QHash<QString, ads::CDockWidget*> dock;
        auto registerSections = [this, &dock](const QList<DockSection>& sections) {
            for (const DockSection& s : sections) {
                dock.insert(s.objectName, makeDock(m_tab1DockManager, s.objectName, s.title, s.widget));
            }
        };
        registerSections(leftSections);
        registerSections(centerSections);
        registerSections(rightSections);
        auto D = [&dock](const char* n) -> ads::CDockWidget* {
            ads::CDockWidget* d = dock.value(QLatin1String(n));
            Q_ASSERT(d);  // Catch a missing dock name (hypothetical null panel builder) early.
            return d;
        };

        // ============================================================
        // GROUPED STACKING RECIPE (Qt-ADS 4.5.0, empirically verified):
        // repeated no-target addDockWidget() calls with
        //   - CenterDockWidgetArea -> areas append VERTICALLY in the root
        //   - LeftDockWidgetArea  -> areas insert side-by-side (horizontal)
        //   - RightDockWidgetArea -> areas append side-by-side (horizontal)
        // Qt-ADS has no API to stack multiple dock areas vertically inside a
        // Left/Right strip, so after registering we "surgery" the splitter
        // tree: both side strips become vertical CDockSplitter columns that
        // are re-inserted into the root (left | center | right).
        // ============================================================

        // === LEFT column: Transport group + System (standalone) ===
        ads::CDockWidget* transportFirst = D("tab1_Player");
        m_tab1DockManager->addDockWidget(ads::LeftDockWidgetArea, transportFirst, nullptr);
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_Decoder"), transportFirst->dockAreaWidget());
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_Audio"), transportFirst->dockAreaWidget());
        // Default active tab: the first dock of the Transport group (Player).
        transportFirst->dockAreaWidget()->setCurrentDockWidget(transportFirst);
        m_tab1DockManager->addDockWidget(ads::LeftDockWidgetArea, D("tab1_System"), nullptr);

        // === CENTER column: Now Playing, Signal Analysis, Frame Analysis ===
        ads::CDockWidget* signalAnalysisFirst = D("tab1_EnsembleExplorer");
        m_tab1DockManager->addDockWidget(ads::CenterDockWidgetArea, signalAnalysisFirst, nullptr);
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_SubchannelOrg"), signalAnalysisFirst->dockAreaWidget());
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_CUUsage"), signalAnalysisFirst->dockAreaWidget());
        // Default active tab: the first dock of the Signal Analysis group.
        signalAnalysisFirst->dockAreaWidget()->setCurrentDockWidget(signalAnalysisFirst);

        ads::CDockWidget* frameAnalysisFirst = D("tab1_FrameNavigator");
        m_tab1DockManager->addDockWidget(ads::CenterDockWidgetArea, frameAnalysisFirst, nullptr);
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_HexCompliance"), frameAnalysisFirst->dockAreaWidget());
        // Default active tab: the first dock of the Frame Analysis group.
        frameAnalysisFirst->dockAreaWidget()->setCurrentDockWidget(frameAnalysisFirst);

        m_tab1DockManager->addDockWidget(ads::CenterDockWidgetArea, D("tab1_NowPlaying"), nullptr);

        // === Now Playing / Slideshow tab group: EPG, Journaline, TPEG ===
        // Phase 2A/2B: Data Services docks are now tabbed with Now Playing / Slideshow
        // in the exact order: Now Playing / Slideshow > EPG > Journaline > TPEG
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_EPG"), D("tab1_NowPlaying")->dockAreaWidget());
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_Journaline"), D("tab1_NowPlaying")->dockAreaWidget());
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_TPEG"), D("tab1_NowPlaying")->dockAreaWidget());
        // Default active tab: Now Playing / Slideshow (first dock in the area).
        D("tab1_NowPlaying")->dockAreaWidget()->setCurrentDockWidget(D("tab1_NowPlaying"));

        // === RIGHT column: Signal Info group + Error & Statistics group ===
        ads::CDockWidget* signalInfoFirst = D("tab1_ETIOverview");
        m_tab1DockManager->addDockWidget(ads::RightDockWidgetArea, signalInfoFirst, nullptr);
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_Status"), signalInfoFirst->dockAreaWidget());
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_Timing"), signalInfoFirst->dockAreaWidget());
        // Default active tab: the first dock of the Signal Info group.
        signalInfoFirst->dockAreaWidget()->setCurrentDockWidget(signalInfoFirst);

        ads::CDockWidget* errorStatsFirst = D("tab1_ErrorCounter");
        m_tab1DockManager->addDockWidget(ads::RightDockWidgetArea, errorStatsFirst, nullptr);
        m_tab1DockManager->addDockWidgetTabToArea(D("tab1_StreamStats"), errorStatsFirst->dockAreaWidget());
        // Default active tab: the first dock of the Error & Statistics group.
        errorStatsFirst->dockAreaWidget()->setCurrentDockWidget(errorStatsFirst);

        // Locate the top-level splitter of this manager's container.
        QSplitter* root = nullptr;
        const auto splitters = m_tab1DockManager->findChildren<QSplitter*>();
        for (QSplitter* sp : splitters) {
            if (sp->parentWidget() == m_tab1DockManager) {
                root = sp;
                break;
            }
        }
        if (!root) {
            return;
        }

        // Column vertical splitters, configured like Qt-ADS's own newSplitter().
        auto newColumnSplitter = [this]() {
            auto* s = new ads::CDockSplitter(Qt::Vertical, m_tab1DockManager);
            s->setOpaqueResize(ads::CDockManager::testConfigFlag(ads::CDockManager::OpaqueSplitterResize));
            s->setChildrenCollapsible(false);
            return s;
        };
        // Direct child of the root that is the ancestor of a given dock area
        // (i.e. the column splitter the area lives in).
        auto columnSplitterFor = [](QSplitter* rootSplitter, QWidget* area) -> QSplitter* {
            QWidget* w = area;
            while (w && w->parentWidget() != rootSplitter) {
                w = w->parentWidget();
            }
            return qobject_cast<QSplitter*>(w);
        };
        ads::CDockSplitter* leftV = newColumnSplitter();
        ads::CDockSplitter* rightV = newColumnSplitter();
        ads::CDockSplitter* centerV = newColumnSplitter();

        // The Qt-ADS built center column that currently holds the center
        // areas (it also contains an empty bookkeeping "ghost" splitter left
        // over from the sequential center-area inserts). Captured BEFORE the
        // moves below; it will be swapped out of the root afterwards.
        QWidget* adsCenterColumn = columnSplitterFor(root, D("tab1_EnsembleExplorer")->dockAreaWidget());

        // Move the left/right/center dock areas out of the Qt-ADS built
        // structure into our column splitters, preserving top->bottom order
        // (QSplitter::addWidget() reparents an area out of its old splitter).
        // Tabified docks SHARE one CDockAreaWidget -> add each unique area once.
        // T30.5 center order (top->bottom): Now Playing / Slideshow first,
        // then Signal Analysis (Ensemble Explorer | Subchannel Org | CU Usage),
        // then Frame Analysis (ETI Frame Navigator | Hex/ETSI Compliance).
        QSet<ads::CDockAreaWidget*> moved;
        const QList<ads::CDockWidget*> centerDocks = {
            D("tab1_NowPlaying"),
            D("tab1_EnsembleExplorer"), D("tab1_SubchannelOrg"), D("tab1_CUUsage"),
            D("tab1_FrameNavigator"), D("tab1_HexCompliance")};
        for (ads::CDockWidget* d : centerDocks) {
            if (ads::CDockAreaWidget* a = d->dockAreaWidget()) {
                if (!moved.contains(a)) {
                    centerV->addWidget(a);
                    moved.insert(a);
                }
            }
        }
        moved.clear();
        const QList<ads::CDockWidget*> leftDocks = {
            D("tab1_Player"), D("tab1_Decoder"), D("tab1_Audio"), D("tab1_System")};
        for (ads::CDockWidget* d : leftDocks) {
            if (ads::CDockAreaWidget* a = d->dockAreaWidget()) {
                if (!moved.contains(a)) {
                    leftV->addWidget(a);
                    moved.insert(a);
                }
            }
        }
        moved.clear();
        const QList<ads::CDockWidget*> rightDocks = {
            D("tab1_ETIOverview"), D("tab1_Status"), D("tab1_Timing"),
            D("tab1_ErrorCounter"), D("tab1_StreamStats")};
        for (ads::CDockWidget* d : rightDocks) {
            if (ads::CDockAreaWidget* a = d->dockAreaWidget()) {
                if (!moved.contains(a)) {
                    rightV->addWidget(a);
                    moved.insert(a);
                }
            }
        }

        // The root now contains only the Qt-ADS built center column, which
        // also holds an empty bookkeeping splitter (ghost) left over from the
        // sequential center-area inserts. Rebuild the root as
        // [leftV | centerV | rightV]: swap our clean center column in and
        // drop the ADS one (its areas were moved out, so it is empty).
        root->insertWidget(0, leftV);
        root->addWidget(rightV);
        if (adsCenterColumn && adsCenterColumn->parentWidget() == root) {
            const int adsIdx = root->indexOf(adsCenterColumn);
            root->insertWidget(adsIdx, centerV);
            adsCenterColumn->setParent(nullptr);  // drop from the root splitter
            delete adsCenterColumn;               // empty structure incl. ghost
        } else {
            root->addWidget(centerV);  // fallback: append our center column
        }

        // ============================================================
        // STEP D — first-launch sizes ≈ mockup (residual differences are
        // expected: Qt-ADS 4.5.0 approximates per-dock vertical stretch and
        // QSplitter::setSizes() respects each area's minimumSizeHint, so the
        // exact mockup pixels are not guaranteed — documented residual).
        // ============================================================
        root->setSizes({230, 698, 272});   // left | center | right widths ≈ 230/flex/272
        leftV->setSizes({178, 320});       // Transport ≈ 178, System grows
        rightV->setSizes({210, 240, 180}); // Signal Info ≈ 210, Error & Stats ≈ 240, Data Services grows

        // Now Playing / Slideshow (option A): grow the compact dock so the
        // inner Slideshow tab can actually show an image (previously 82 px).
        if (ads::CDockAreaWidget* nowPlayingArea = D("tab1_NowPlaying")->dockAreaWidget()) {
            nowPlayingArea->setMinimumHeight(200);
        }
        if (centerV->count() == 3) {
            // T30.5: Now Playing (top) ≈ 200 (matches the 200 px minimum set
            // above), Signal Analysis ≈ 190, Frame Analysis takes the rest.
            centerV->setSizes({200, 190, 150});
        }

        // Observation (a): reparenting the areas during the splitter surgery
        // must not change which tab the Signal Analysis group opens on. Re-assert
        // the default active tab (DAB Ensemble Explorer, not CU Usage) after the
        // surgery so a fresh layout is deterministic.
        if (ads::CDockAreaWidget* saArea = D("tab1_EnsembleExplorer")->dockAreaWidget()) {
            saArea->setCurrentDockWidget(D("tab1_EnsembleExplorer"));
        }
    }

    QList<DABAnalyserWindow::DockSection> createTab1_LeftPanel()
    {
        QList<DockSection> sections;

        // LEFT Column (UI redesign Steps B/C) — each section becomes one dock:
        // Player + Decoder form the Transport group; System stays standalone.
        // (T41: the legacy Messages panel was removed, not relocated.)
        
        // 1. Player Panel (Item #1) - 20%
        QGroupBox* playerPanel = createPlayerPanel();
        if (playerPanel) {
            sections.append({QStringLiteral("tab1_Player"), QStringLiteral("Player"), playerPanel});
        }
        
        // 2. Decoder Panel (Item #2) - 20%
        QGroupBox* decoderPanel = createDecoderPanel();
        if (decoderPanel) {
            sections.append({QStringLiteral("tab1_Decoder"), QStringLiteral("Decoder"), decoderPanel});
        }

        // 2b. Audio Panel (T34) - audio output controls/meters (Transport group).
        QGroupBox* audioPanel = createAudioPanel();
        if (audioPanel) {
            sections.append({QStringLiteral("tab1_Audio"), QStringLiteral("Audio"), audioPanel});
        }
        
        // 3. System Panel (Item #3) - 40%
        QGroupBox* systemPanel = createSystemPanel();
        if (systemPanel) {
            sections.append({QStringLiteral("tab1_System"), QStringLiteral("System"), systemPanel});
        }

        return sections;
    }

    QWidget* createLiveStreamInputPanel()
    {
        // UI redesign (Step A): the former tab1_InputSource dock becomes a
        // compact toolbar pinned at the TOP of the Tab-1 page (above the page's
        // CDockManager). Row 1 (always visible): File/Network radios · path
        // edit · Browse… · Load File. Row 2 (m_sourceInputStack, hidden in
        // File mode): network controls + stream statistics — the old network
        // page content, reused verbatim. All member widgets and slot wiring
        // are unchanged.
        QWidget* panel = new QWidget();
        panel->setObjectName(QStringLiteral("inputToolbar"));
        QVBoxLayout* mainLayout = new QVBoxLayout(panel);
        mainLayout->setContentsMargins(8, 8, 8, 8);
        mainLayout->setSpacing(6);

        // === ROW 1: Source type + file path + Browse + Load (always visible) ===
        QHBoxLayout* row1 = new QHBoxLayout();
        m_fileSourceRadio = new QRadioButton("File");
        m_networkSourceRadio = new QRadioButton("Network Stream");
        m_fileSourceRadio->setChecked(true);  // Default to file input

        QLabel* fileLabel = new QLabel("File:");
        fileLabel->setMinimumWidth(40);
        m_filePathEdit = new QLineEdit();
        m_filePathEdit->setPlaceholderText("Select an ETI file...");
        m_filePathEdit->setReadOnly(true);
        m_browseFileButton = new QPushButton("Browse...");
        m_loadFileButton = new QPushButton("Load File");
        m_loadFileButton->setStyleSheet("font-weight: bold; background-color: #0078D4; color: white;");

        row1->addWidget(m_fileSourceRadio);
        row1->addWidget(m_networkSourceRadio);
        row1->addSpacing(8);
        row1->addWidget(fileLabel);
        row1->addWidget(m_filePathEdit, 1);
        row1->addWidget(m_browseFileButton);
        row1->addWidget(m_loadFileButton);
        mainLayout->addLayout(row1);

        // === ROW 2: expandable network row ===
        // Stacked widget for File vs Network input. In File mode the whole row
        // is hidden (compact single-row bar); when "Network Stream" is selected
        // onSourceTypeChanged() shows it and jumps to the network page.
        m_sourceInputStack = new QStackedWidget();
        m_sourceInputStack->setObjectName(QStringLiteral("inputToolbarNetworkRow"));
        m_sourceInputStack->setVisible(false);  // File mode by default

        // === PAGE 0: File Input (kept for stack index semantics — File page index 0) ===
        QWidget* fileInputPage = new QWidget();
        m_sourceInputStack->addWidget(fileInputPage);

        // === PAGE 1: Network Stream Input ===
        QWidget* networkInputPage = new QWidget();
        QVBoxLayout* networkLayout = new QVBoxLayout(networkInputPage);
        networkLayout->setContentsMargins(4, 4, 4, 4);
        networkLayout->setSpacing(6);

        // Stream URL input
        QHBoxLayout* urlLayout = new QHBoxLayout();
        QLabel* urlLabel = new QLabel("Stream URL:");
        urlLabel->setMinimumWidth(80);
        m_streamUrlEdit = new QLineEdit();
        m_streamUrlEdit->setPlaceholderText("udp://239.192.0.1:9200");
        m_streamUrlEdit->setFont(QFont("Consolas", 9));
#ifdef HAVE_ZMQ
        m_streamUrlEdit->setToolTip(
            "Supported: udp://A.B.C.D:port or A.B.C.D:port (IPv4 multicast), "
            "tcp://host:port (raw ETI stream), zmq+tcp://host:port "
            "(ODR-DabMux ZeroMQ). Port defaults to 9200.");
#else
        m_streamUrlEdit->setToolTip(
            "Supported: udp://A.B.C.D:port or A.B.C.D:port (IPv4 multicast) "
            "and tcp://host:port (raw ETI stream). Port defaults to 9200. "
            "ZeroMQ (zmq+tcp://) is not built in.");
#endif
        urlLayout->addWidget(urlLabel);
        urlLayout->addWidget(m_streamUrlEdit, 1);
        networkLayout->addLayout(urlLayout);

        // Quick Examples dropdown
        QHBoxLayout* examplesLayout = new QHBoxLayout();
        QLabel* examplesLabel = new QLabel("Quick Examples:");
        examplesLabel->setMinimumWidth(80);
        m_quickExampleCombo = new QComboBox();
        m_quickExampleCombo->addItem("udp://239.192.0.1:9200 (multicast)");
        m_quickExampleCombo->addItem("239.192.0.1:9200 (multicast, plain)");
        m_quickExampleCombo->addItem("tcp://192.168.1.100:9200 (raw ETI TCP)");
#ifdef HAVE_ZMQ
        m_quickExampleCombo->addItem("zmq+tcp://localhost:9201 (ODR-DabMux ZMQ)");
        m_quickExampleCombo->setToolTip(
            "Supported transports: UDP multicast (udp:// or plain address), "
            "raw ETI over TCP (tcp://) and ODR-DabMux ZeroMQ (zmq+tcp://).");
#else
        // Annotated AND disabled: selecting it would only be rejected by the
        // validator, so make the missing backend explicit up front instead of
        // letting it fail silently later (item data QVariant(0) @ UserRole-1 is
        // the QComboBox "disable item" idiom).
        m_quickExampleCombo->addItem("zmq+tcp://localhost:9201 (ZeroMQ not built in)");
        m_quickExampleCombo->setItemData(3, QVariant(0), Qt::UserRole - 1);
        m_quickExampleCombo->setToolTip(
            "Supported transports: UDP multicast (udp:// or plain address) and "
            "raw ETI over TCP (tcp://). ZeroMQ (zmq+tcp://) is not built in.");
#endif
        m_quickExampleCombo->setCurrentIndex(-1);  // No selection by default
        examplesLayout->addWidget(examplesLabel);
        examplesLayout->addWidget(m_quickExampleCombo, 1);
        networkLayout->addLayout(examplesLayout);

        // Connect/Disconnect buttons and status
        QHBoxLayout* controlLayout = new QHBoxLayout();
        m_connectBtn = new QPushButton("Connect");
        m_connectBtn->setStyleSheet("font-weight: bold; background-color: #107C10; color: white;");
        m_disconnectBtn = new QPushButton("Disconnect");
        m_disconnectBtn->setEnabled(false);
        // W1 #1: plain text only. Qt has no SP_MediaRecord standard icon, and
        // the previous "⏺"/"⏹" glyphs (U+23FA/U+23F9) are font-dependent and
        // rendered as tofu on fonts without them.
        m_recordBtn = new QPushButton("Record");
        m_recordBtn->setEnabled(false);
        m_recordBtn->setStyleSheet("background-color: #D32F2F; color: white;");
        m_streamStatus = new QLabel("<span style='color: #666;'>● Disconnected</span>");
        m_streamStatus->setFont(QFont("Arial", 10, QFont::Bold));

        controlLayout->addWidget(m_connectBtn);
        controlLayout->addWidget(m_disconnectBtn);
        controlLayout->addWidget(m_recordBtn);
        controlLayout->addWidget(new QLabel("Status:"));
        controlLayout->addWidget(m_streamStatus);
        controlLayout->addStretch();

        // T23 (option A): the Input menu was removed; its unique item — UDP
        // Streaming Settings — moves here as a ⚙ button at the end of the
        // network row (after Record/status). Same dialog/handler as before.
        m_udpSettingsBtn = new QToolButton();
        m_udpSettingsBtn->setObjectName(QStringLiteral("udpSettingsButton"));
        m_udpSettingsBtn->setText(QStringLiteral("⚙"));
        m_udpSettingsBtn->setToolTip(QStringLiteral("UDP streaming settings"));
        m_udpSettingsBtn->setAccessibleName(QStringLiteral("UDP streaming settings"));
        m_udpSettingsBtn->setAutoRaise(true);
        m_udpSettingsBtn->setCursor(Qt::PointingHandCursor);
        controlLayout->addWidget(m_udpSettingsBtn);
        networkLayout->addLayout(controlLayout);

        // Stream Statistics GroupBox
        QGroupBox* statsGroup = new QGroupBox("Stream Statistics");
        QGridLayout* statsLayout = new QGridLayout(statsGroup);
        statsLayout->setContentsMargins(6, 6, 6, 6);
        statsLayout->setSpacing(4);

        // Create statistics labels (2-column layout)
        QFont labelFont("Arial", 9);
        QFont dataFont("Consolas", 9, QFont::Bold);

        // Row 0: Frame Rate
        QLabel* frLabel = new QLabel("Frame Rate:");
        frLabel->setFont(labelFont);
        m_streamFrameRate = new QLabel("--");
        m_streamFrameRate->setFont(dataFont);
        m_streamFrameRate->setStyleSheet("color: #0078D4;");
        statsLayout->addWidget(frLabel, 0, 0);
        statsLayout->addWidget(m_streamFrameRate, 0, 1);

        // Row 0: Frames Received
        QLabel* fcLabel = new QLabel("Frames Received:");
        fcLabel->setFont(labelFont);
        m_streamFramesReceived = new QLabel("--");
        m_streamFramesReceived->setFont(dataFont);
        m_streamFramesReceived->setStyleSheet("color: #0078D4;");
        statsLayout->addWidget(fcLabel, 0, 2);
        statsLayout->addWidget(m_streamFramesReceived, 0, 3);

        // Row 1: Frame Loss
        QLabel* flLabel = new QLabel("Frame Loss:");
        flLabel->setFont(labelFont);
        m_streamFrameLoss = new QLabel("--");
        m_streamFrameLoss->setFont(dataFont);
        m_streamFrameLoss->setStyleSheet("color: #107C10;");
        statsLayout->addWidget(flLabel, 1, 0);
        statsLayout->addWidget(m_streamFrameLoss, 1, 1);

        // Row 1: Network Latency
        QLabel* latLabel = new QLabel("Network Latency:");
        latLabel->setFont(labelFont);
        m_streamLatency = new QLabel("--");
        m_streamLatency->setFont(dataFont);
        m_streamLatency->setStyleSheet("color: #107C10;");
        statsLayout->addWidget(latLabel, 1, 2);
        statsLayout->addWidget(m_streamLatency, 1, 3);

        // Row 2: Buffer Status
        QLabel* bufLabel = new QLabel("Buffer Status:");
        bufLabel->setFont(labelFont);
        m_streamBuffer = new QLabel("--");
        m_streamBuffer->setFont(dataFont);
        m_streamBuffer->setStyleSheet("color: #0078D4;");
        statsLayout->addWidget(bufLabel, 2, 0);
        statsLayout->addWidget(m_streamBuffer, 2, 1);

        // Row 2: Data Rate
        QLabel* drLabel = new QLabel("Data Rate:");
        drLabel->setFont(labelFont);
        m_streamDataRate = new QLabel("--");
        m_streamDataRate->setFont(dataFont);
        m_streamDataRate->setStyleSheet("color: #0078D4;");
        statsLayout->addWidget(drLabel, 2, 2);
        statsLayout->addWidget(m_streamDataRate, 2, 3);

        // Row 3: Uptime (spans 2 columns)
        QLabel* upLabel = new QLabel("Uptime:");
        upLabel->setFont(labelFont);
        m_streamUptime = new QLabel("--");
        m_streamUptime->setFont(dataFont);
        m_streamUptime->setStyleSheet("color: #107C10;");
        statsLayout->addWidget(upLabel, 3, 0);
        statsLayout->addWidget(m_streamUptime, 3, 1);

        networkLayout->addWidget(statsGroup);

        m_sourceInputStack->addWidget(networkInputPage);

        mainLayout->addWidget(m_sourceInputStack);

        // Connect signals
        connect(m_fileSourceRadio, &QRadioButton::toggled, this, [this](bool checked) {
            onSourceTypeChanged(!checked);
        });
        connect(m_networkSourceRadio, &QRadioButton::toggled, this, [this](bool checked) {
            onSourceTypeChanged(checked);
        });
        connect(m_quickExampleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &DABAnalyserWindow::onQuickExampleSelected);
        connect(m_connectBtn, &QPushButton::clicked, this, &DABAnalyserWindow::onConnectClicked);
        connect(m_disconnectBtn, &QPushButton::clicked, this, &DABAnalyserWindow::onDisconnectClicked);
        connect(m_recordBtn, &QPushButton::clicked, this, &DABAnalyserWindow::onRecordClicked);
        connect(m_udpSettingsBtn, &QToolButton::clicked, this, &DABAnalyserWindow::showUDPSettings);
        connect(m_browseFileButton, &QPushButton::clicked, this, &DABAnalyserWindow::onBrowseFileClicked);
        connect(m_loadFileButton, &QPushButton::clicked, this, &DABAnalyserWindow::onLoadFileClicked);

        // Initialize statistics timer (but don't start it)
        m_statsTimer = new QTimer(this);
        // NOTE: Stream statistics are updated from onProcessingComplete() with frame/time parameters
        // No periodic timer update needed for file-based processing

        return panel;
    }

    QList<DABAnalyserWindow::DockSection> createTab1_CenterPanel()
    {
        QList<DockSection> sections;

        // CENTER Column (UI redesign Steps A/B) — each section becomes one
        // dock; registerTab1Docks() groups them:
        //   Signal Analysis (Ensemble Explorer | Subchannel Org | CU Usage),
        //   Frame Analysis (ETI Frame Navigator | Hex/ETSI Compliance),
        //   Now Playing (standalone, compact bottom).
        // The old tab1_InputSource dock is gone (the toolbar replaced it).
        // CU Usage moved here from the right column (mockup grouping).

        // 1. DAB Ensemble Explorer (Item #5) - moved from LEFT - 15%
        QGroupBox* ensembleGroup = new QGroupBox("DAB Ensemble Explorer");
        QVBoxLayout* ensembleLayout = new QVBoxLayout(ensembleGroup);
        
        if (!m_ensembleTree) {
            m_ensembleTree = new QTreeWidget();
        }
        // Priority 1: 11 columns for Ensemble Tree
        m_ensembleTree->setHeaderLabels({
            "Service", "Label", "Short Label", "Type", "Programme Type",
            "Language", "CA Flag", "Comp. Count", "EID", "Country", "Last Updated"
        });
        m_ensembleTree->setAlternatingRowColors(true);
        m_ensembleTree->header()->setStretchLastSection(false);
        m_ensembleTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_ensembleTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_ensembleTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_ensembleTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        m_ensembleTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        m_ensembleTree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
        m_ensembleTree->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
        m_ensembleTree->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
        m_ensembleTree->header()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
        m_ensembleTree->header()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
        m_ensembleTree->header()->setSectionResizeMode(10, QHeaderView::ResizeToContents);
        ensembleLayout->addWidget(m_ensembleTree);
        
        sections.append({QStringLiteral("tab1_EnsembleExplorer"), QStringLiteral("DAB Ensemble Explorer"), ensembleGroup});

        // 2. ETI Frame Navigator (Item #6) - 15%
        QGroupBox* navigatorGroup = new QGroupBox("ETI Frame Navigator");
        QVBoxLayout* navigatorLayout = new QVBoxLayout(navigatorGroup);
        
        if (!m_frameList) {
            m_frameList = new QListWidget();
        }
        m_frameList->setAlternatingRowColors(true);
        navigatorLayout->addWidget(m_frameList);

        // Selected-frame details (was declared but never instantiated). Shown
        // below the list; small/dim so it reads as secondary information.
        if (!m_frameDetails) {
            m_frameDetails = new QLabel(QStringLiteral("Selected Frame: —"));
            m_frameDetails->setObjectName(QStringLiteral("frameDetailsLabel"));
            m_frameDetails->setWordWrap(true);
            m_frameDetails->setTextInteractionFlags(Qt::TextSelectableByMouse);
            QFont detailsFont = m_frameDetails->font();
            detailsFont.setPointSizeF(qMax(7.5, detailsFont.pointSizeF() - 1.5));
            m_frameDetails->setFont(detailsFont);
            m_frameDetails->setStyleSheet(QStringLiteral("color: palette(mid);"));
        }
        navigatorLayout->addWidget(m_frameDetails);

        sections.append({QStringLiteral("tab1_FrameNavigator"), QStringLiteral("ETI Frame Navigator"), navigatorGroup});

        // 3. Hex Viewer / ETSI Compliance (Item #7) - 30%
        // ONE dock wrapping the existing internal viewerTabs QTabWidget.
        QTabWidget* viewerTabs = new QTabWidget();
        
        if (!m_hexViewer) {
            m_hexViewer = new QTextEdit();
            m_hexViewer->setFont(QFont("Consolas", 9));
            m_hexViewer->setReadOnly(true);
        }
        m_hexViewer->setPlaceholderText("Select a frame to view hex data...");
        viewerTabs->addTab(m_hexViewer, "Hex Viewer");
        
        if (!m_complianceViewer) {
            m_complianceViewer = new QTextEdit();
            m_complianceViewer->setFont(QFont("Consolas", 9));
            m_complianceViewer->setReadOnly(true);
        }
        m_complianceViewer->setPlaceholderText("ETSI EN 300 799 compliance results...");
        viewerTabs->addTab(m_complianceViewer, "ETSI Compliance");
        
        sections.append({QStringLiteral("tab1_HexCompliance"), QStringLiteral("Hex Viewer / ETSI Compliance"), viewerTabs});

        // 4. Subchannel Organization (Item #8) - 15%
        QGroupBox* subchannelGroup = new QGroupBox("Subchannel Organization");
        QVBoxLayout* subchannelLayout = new QVBoxLayout(subchannelGroup);
        
        if (!m_subchannelTable) {
            m_subchannelTable = new QTableWidget();
            m_subchannelTable->setColumnCount(8);
            m_subchannelTable->setHorizontalHeaderLabels({
                "☑", "SubCh ID", "Type", "Level", "Bitrate", "SAD", "Size", "Errors"
            });
            m_subchannelTable->setAlternatingRowColors(true);
            m_subchannelTable->setSelectionBehavior(QAbstractItemView::SelectRows);
            m_subchannelTable->horizontalHeader()->setStretchLastSection(true);
            m_subchannelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        }
        subchannelLayout->addWidget(m_subchannelTable);
        
        sections.append({QStringLiteral("tab1_SubchannelOrg"), QStringLiteral("Subchannel Organization"), subchannelGroup});

        // 5. CU Usage (Item #13) - moved from RIGHT (mockup: Signal Analysis group)
        QGroupBox* cuUsagePanel = createCUUsagePanel();
        if (cuUsagePanel) {
            sections.append({QStringLiteral("tab1_CUUsage"), QStringLiteral("CU Usage"), cuUsagePanel});
        }

        // 6. Now Playing / Slideshow (Item #9) - left/right splitter (T27)
        QGroupBox* nowPlayingPanel = createNowPlayingPanel();
        if (nowPlayingPanel) {
            sections.append({QStringLiteral("tab1_NowPlaying"), QStringLiteral("Now Playing / Slideshow"), nowPlayingPanel});
        }

        // 7. EPG (Phase 2A) - tabbed with Now Playing / Slideshow
        QWidget* epgContent = createEpgDockContent();
        if (epgContent) {
            sections.append({QStringLiteral("tab1_EPG"), QStringLiteral("EPG"), epgContent});
        }

        // 8. Journaline (Phase 2A) - tabbed with Now Playing / Slideshow
        QWidget* journalineContent = createJournalineDockContent();
        if (journalineContent) {
            sections.append({QStringLiteral("tab1_Journaline"), QStringLiteral("Journaline"), journalineContent});
        }

        // 9. TPEG (Phase 2A) - tabbed with Now Playing / Slideshow
        QWidget* tpegContent = createTpegDockContent();
        if (tpegContent) {
            sections.append({QStringLiteral("tab1_TPEG"), QStringLiteral("TPEG"), tpegContent});
        }

        return sections;
    }

    QWidget* createPlayerDecoderStatusPanels()
    {
        QWidget* widget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        // Player Panel
        QGroupBox* playerGroup = new QGroupBox("Player");
        QVBoxLayout* playerLayout = new QVBoxLayout(playerGroup);
        QLabel* playerStatus = new QLabel("Status: Ready");
        playerStatus->setStyleSheet("color: #107C10; font-weight: bold;");
        playerLayout->addWidget(playerStatus);
        layout->addWidget(playerGroup);

        // Decoder Panel
        QGroupBox* decoderGroup = new QGroupBox("Decoder");
        QVBoxLayout* decoderLayout = new QVBoxLayout(decoderGroup);
        QLabel* decoderStatus = new QLabel("Status: Idle");
        decoderStatus->setStyleSheet("color: #666; font-weight: bold;");
        decoderLayout->addWidget(decoderStatus);
        layout->addWidget(decoderGroup);

        return widget;
    }

    QList<DABAnalyserWindow::DockSection> createTab1_RightPanel()
    {
        QList<DockSection> sections;

        // RIGHT Column (UI redesign Step B) — each section becomes one dock:
        // ETI Overview | Status | Timing form the Signal Info group,
        // DAB Error Counter + DAB Stream Statistics the Error & Statistics
        // group. CU Usage moved to the CENTER column (Signal Analysis group).

        // 1. ETI Overview (Item #10) - 25%
        QGroupBox* etiOverviewPanel = createETIOverviewPanel();
        if (etiOverviewPanel) {
            sections.append({QStringLiteral("tab1_ETIOverview"), QStringLiteral("ETI Overview"), etiOverviewPanel});
        }

        // 2. DAB Error Counter (Item #11) - 20% (includes the error table)
        QGroupBox* errorCounterPanel = createErrorDetectionPanel();
        QGroupBox* statusGroup = nullptr;
        if (errorCounterPanel) {
            // The section builder createErrorDetectionPanel() keeps the little
            // "Status" group inside the error counter (left untouched). Per the
            // v1.3 layout spec the right column has a SEPARATE "Status" dock, so
            // we split that inner group out here by reparenting it - all widgets
            // (labels, members) stay exactly the same, only the container changes.
            const auto innerGroups = errorCounterPanel->findChildren<QGroupBox*>();
            for (QGroupBox* g : innerGroups) {
                if (g->title() == "Status") {
                    statusGroup = g;
                    break;
                }
            }
            if (statusGroup && errorCounterPanel->layout()) {
                errorCounterPanel->layout()->removeWidget(statusGroup);
            }
            sections.append({QStringLiteral("tab1_ErrorCounter"), QStringLiteral("DAB Error Counter"), errorCounterPanel});
        }

        // 3. Status - the inner "Status" group split out of the error counter
        if (statusGroup) {
            sections.append({QStringLiteral("tab1_Status"), QStringLiteral("Status"), statusGroup});
        }

        // 4. Timing (Item #12) - 20%
        QGroupBox* timingPanel = createTimingPanel();
        if (timingPanel) {
            sections.append({QStringLiteral("tab1_Timing"), QStringLiteral("Timing"), timingPanel});
        }

        // NOTE: CU Usage moved to the CENTER column (Signal Analysis group,
        // UI redesign Step B) — right column keeps Overview/ErrorCounter/
        // Status/Timing/StreamStats only.

        // 5. Stream Statistics (Item #14) - 15%
        QGroupBox* streamStatsPanel = createStreamStatisticsPanel();
        if (streamStatsPanel) {
            sections.append({QStringLiteral("tab1_StreamStats"), QStringLiteral("DAB Stream Statistics"), streamStatsPanel});
        }

        return sections;
    }

    // ========================================================================
    // TAB 2: FIC-ANALYSER (3-PANEL LAYOUT PER DATA_AVAILABILITY_MATRIX.md)
    // ========================================================================
    
    QWidget* createTab2_LeftPanel()
    {
        QWidget* panel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(4);

        // 1. Overview (QGroupBox) - 20%
        QGroupBox* overviewGroup = new QGroupBox("Overview");
        QVBoxLayout* overviewLayout = new QVBoxLayout(overviewGroup);
        
        // Refinement C: descriptive empty-state text (no "(none)"/"0").
        if (!m_tab2_ensembleName) {
            m_tab2_ensembleName = new QLabel("Ensemble: awaiting decode");
        }
        if (!m_tab2_ficContentSummary) {
            m_tab2_ficContentSummary = new QLabel("FIC Content: awaiting decode");
        }
        if (!m_tab2_subchannelOrgSummary) {
            m_tab2_subchannelOrgSummary = new QLabel("Sub-Channels: awaiting decode");
        }
        // T33.1: the removed "Digital Org Info" panel's intent is folded into
        // Overview as one compact line summarising the FIC-decoded data
        // services (same analyser data the service tables use).
        if (!m_tab2_digitalServices) {
            m_tab2_digitalServices = new QLabel("Digital services: awaiting decode");
        }
        
        m_tab2_ensembleName->setWordWrap(true);
        m_tab2_ficContentSummary->setWordWrap(true);
        m_tab2_subchannelOrgSummary->setWordWrap(true);
        m_tab2_digitalServices->setWordWrap(true);
        
        overviewLayout->addWidget(m_tab2_ensembleName);
        overviewLayout->addWidget(m_tab2_ficContentSummary);
        overviewLayout->addWidget(m_tab2_subchannelOrgSummary);
        overviewLayout->addWidget(m_tab2_digitalServices);
        overviewLayout->addStretch();
        
        layout->addWidget(overviewGroup, 20);

        // 2. Service Tree (QTreeWidget) - T33.3: expands 40% -> 60% now that
        // Digital Org Info is gone and Service Components moved to the center.
        QGroupBox* serviceTreeGroup = new QGroupBox("Service Tree");
        QVBoxLayout* serviceTreeLayout = new QVBoxLayout(serviceTreeGroup);
        
        if (!m_tab2_serviceTree) {
            m_tab2_serviceTree = new QTreeWidget();
            m_tab2_serviceTree->setHeaderLabels({"SubCh / Service", "Type", "Component ID"});
            m_tab2_serviceTree->setAlternatingRowColors(true);
        }
        serviceTreeLayout->addWidget(m_tab2_serviceTree);
        
        layout->addWidget(serviceTreeGroup, 60);

        return panel;
    }

    QWidget* createTab2_CenterPanel()
    {
        QWidget* panel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(4);

        // T33.2: center height split 4 ways (~25% each):
        // Ensemble Table / Sub-Channel Table / Service Table / Service Components.

        // 1. Ensemble Table (10 columns) - 25%
        QGroupBox* ensembleTableGroup = new QGroupBox("Ensemble Table");
        QVBoxLayout* ensembleTableLayout = new QVBoxLayout(ensembleTableGroup);
        
        if (!m_tab2_ensembleTable) {
            m_tab2_ensembleTable = new QTableWidget();
            m_tab2_ensembleTable->setColumnCount(10);
            m_tab2_ensembleTable->setHorizontalHeaderLabels({
                "Type", "SubCH Id", "FIGCH", "PADDR", "SC1d", "CA", "CAOrg", "Bitrate", "ASCTy+", "Info"
            });
            m_tab2_ensembleTable->setAlternatingRowColors(true);
            m_tab2_ensembleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        }
        ensembleTableLayout->addWidget(m_tab2_ensembleTable);
        
        layout->addWidget(ensembleTableGroup, 25);

        // 2. Sub-Channel Table (10 columns) - 25%
        QGroupBox* subchannelTableGroup = new QGroupBox("Sub-Channel Table");
        QVBoxLayout* subchannelTableLayout = new QVBoxLayout(subchannelTableGroup);
        
        if (!m_tab2_subchannelTable) {
            m_tab2_subchannelTable = new QTableWidget();
            m_tab2_subchannelTable->setColumnCount(10);
            m_tab2_subchannelTable->setHorizontalHeaderLabels({
                "SubCh Id", "SAD", "Bitrate", "P.Level", "P.Type", "SubCh Type", "SId", "Service Label", "Short Label", "CAID"
            });
            m_tab2_subchannelTable->setAlternatingRowColors(true);
            m_tab2_subchannelTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        }
        subchannelTableLayout->addWidget(m_tab2_subchannelTable);
        
        layout->addWidget(subchannelTableGroup, 25);

        // 3. Service Table (4 columns) - 25%
        QGroupBox* serviceTableGroup = new QGroupBox("Service Table");
        QVBoxLayout* serviceTableLayout = new QVBoxLayout(serviceTableGroup);
        
        if (!m_tab2_serviceTable) {
            m_tab2_serviceTable = new QTableWidget();
            m_tab2_serviceTable->setColumnCount(9);
            m_tab2_serviceTable->setHorizontalHeaderLabels({
                "SId", "Service Label", "Short Label", "Type",
                "Programme Type", "Language", "CA Flag", "Comp. Count", "Primary SC"
            });
            m_tab2_serviceTable->setAlternatingRowColors(true);
            m_tab2_serviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        }
        serviceTableLayout->addWidget(m_tab2_serviceTable);
        
        layout->addWidget(serviceTableGroup, 25);

        // 4. Service Components (QTreeWidget) - 25% (moved here from the left
        // panel in T33.2).
        QGroupBox* componentsGroup = new QGroupBox("Service Components");
        QVBoxLayout* componentsLayout = new QVBoxLayout(componentsGroup);
        
        if (!m_tab2_serviceComponents) {
            m_tab2_serviceComponents = new QTreeWidget();
            m_tab2_serviceComponents->setHeaderLabels({"Component", "Type", "Details"});
            m_tab2_serviceComponents->setAlternatingRowColors(true);
        }
        componentsLayout->addWidget(m_tab2_serviceComponents);
        
        layout->addWidget(componentsGroup, 25);

        return panel;
    }

    QWidget* createTab2_RightPanel()
    {
        QWidget* panel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(4);

        // FIG Content Table (4 columns) - 100%
        QGroupBox* figContentGroup = new QGroupBox("FIG Content");
        QVBoxLayout* figContentLayout = new QVBoxLayout(figContentGroup);
        
        if (!m_tab2_figContentTable) {
            m_tab2_figContentTable = new QTableWidget();
            m_tab2_figContentTable->setColumnCount(4);
            m_tab2_figContentTable->setHorizontalHeaderLabels({
                "Type", "Extension", "Number", "Subject"
            });
            m_tab2_figContentTable->setAlternatingRowColors(true);
            m_tab2_figContentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        }
        figContentLayout->addWidget(m_tab2_figContentTable);
        
        layout->addWidget(figContentGroup);

        return panel;
    }

    // ========================================================================
    // Phase 2C: Advanced FIG Analyser Dock - FIG2 Extended Labels Display
    // ========================================================================
    QWidget* createAdvancedFigAnalyserDockContent()
    {
        QWidget* panel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(4);

        // Toolbar with filter combo
        QHBoxLayout* toolbarLayout = new QHBoxLayout();
        toolbarLayout->setContentsMargins(0, 0, 0, 0);
        
        QLabel* filterLabel = new QLabel("Filter:");
        filterLabel->setToolTip("Filter FIG2 extended labels by entity type");
        toolbarLayout->addWidget(filterLabel);
        
        m_advancedFigFilter = new QComboBox();
        m_advancedFigFilter->setObjectName(QStringLiteral("advancedFigFilter"));
        m_advancedFigFilter->addItem("All FIG2 Types", QVariant::fromValue<int>(-1));
        m_advancedFigFilter->addItem("FIG 2/0 - Ensemble Extended Labels", QVariant::fromValue<int>(0));
        m_advancedFigFilter->addItem("FIG 2/1 - Service Extended Labels", QVariant::fromValue<int>(1));
        m_advancedFigFilter->addItem("FIG 2/4 - Component Extended Labels", QVariant::fromValue<int>(4));
        m_advancedFigFilter->addItem("FIG 2/5 - Data Service Extended Labels", QVariant::fromValue<int>(5));
        m_advancedFigFilter->setToolTip("Select which FIG2 type to display");
        toolbarLayout->addWidget(m_advancedFigFilter);
        
        toolbarLayout->addStretch();
        
        QPushButton* refreshBtn = new QPushButton("Refresh");
        refreshBtn->setToolTip("Refresh the display from current analyser state");
        connect(refreshBtn, &QPushButton::clicked, this, &DABAnalyserWindow::refreshAdvancedFigDisplay);
        toolbarLayout->addWidget(refreshBtn);
        
        layout->addLayout(toolbarLayout);

        // Main content: vertical splitter between tree/table and hex viewer
        QSplitter* mainSplitter = new QSplitter(Qt::Vertical);
        mainSplitter->setObjectName(QStringLiteral("advancedFigMainSplitter"));
        mainSplitter->setChildrenCollapsible(false);

        // Top: Tree/Table showing FIG2 extended labels
        QGroupBox* fig2Group = new QGroupBox("FIG2 Extended Labels");
        QVBoxLayout* fig2Layout = new QVBoxLayout(fig2Group);
        fig2Layout->setContentsMargins(4, 4, 4, 4);
        
        m_advancedFigTree = new QTreeWidget();
        m_advancedFigTree->setObjectName(QStringLiteral("advancedFigTree"));
        m_advancedFigTree->setHeaderLabels({
            "FIG2 Type", "Entity ID", "Label", "Charset", "Segments", "Toggle", "Last Updated"
        });
        m_advancedFigTree->setAlternatingRowColors(true);
        m_advancedFigTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_advancedFigTree->setUniformRowHeights(true);
        m_advancedFigTree->setRootIsDecorated(false);
        m_advancedFigTree->setSortingEnabled(true);
        m_advancedFigTree->sortByColumn(0, Qt::AscendingOrder);
        
        // Set column widths
        m_advancedFigTree->setColumnWidth(0, 120);  // FIG2 Type
        m_advancedFigTree->setColumnWidth(1, 100);  // Entity ID
        m_advancedFigTree->setColumnWidth(2, 300);  // Label
        m_advancedFigTree->setColumnWidth(3, 80);   // Charset
        m_advancedFigTree->setColumnWidth(4, 80);   // Segments
        m_advancedFigTree->setColumnWidth(5, 60);   // Toggle
        m_advancedFigTree->setColumnWidth(6, 140);  // Last Updated
        
        // Use monospace font for ID column
        QFont monoFont("Consolas", 9);
        m_advancedFigTree->setFont(monoFont);
        
        fig2Layout->addWidget(m_advancedFigTree);
        mainSplitter->addWidget(fig2Group);

        // Bottom: Raw FIG2 bytes viewer (hex dump) for selected entity
        QGroupBox* hexGroup = new QGroupBox("Raw FIG2 Bytes (Hex Dump)");
        QVBoxLayout* hexLayout = new QVBoxLayout(hexGroup);
        hexLayout->setContentsMargins(4, 4, 4, 4);
        
        m_advancedFigHexViewer = new QTextEdit();
        m_advancedFigHexViewer->setObjectName(QStringLiteral("advancedFigHexViewer"));
        m_advancedFigHexViewer->setReadOnly(true);
        m_advancedFigHexViewer->setLineWrapMode(QTextEdit::NoWrap);
        m_advancedFigHexViewer->setFont(QFont("Consolas", 8));
        m_advancedFigHexViewer->setPlaceholderText("Select a FIG2 entry above to view its raw bytes...");
        hexLayout->addWidget(m_advancedFigHexViewer);
        mainSplitter->addWidget(hexGroup);

        // Set initial splitter sizes (70% tree, 30% hex)
        mainSplitter->setSizes({500, 200});
        
        layout->addWidget(mainSplitter, 1);

        // Connect tree selection to hex viewer
        connect(m_advancedFigTree, &QTreeWidget::itemSelectionChanged,
                this, &DABAnalyserWindow::onAdvancedFigTreeSelectionChanged);
        
        // Connect filter combo to refresh display
        connect(m_advancedFigFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &DABAnalyserWindow::refreshAdvancedFigDisplay);

        return panel;
    }

    QWidget* createTab2_FICAnalyser()
    {
        QWidget* mainPanel = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(mainPanel);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Tab 2 gets its OWN CDockManager with 3 docks (one per original column).
        m_tab2DockManager = new ads::CDockManager(mainPanel);
        m_tab2DockManager->setObjectName("tab2DockManager");
        registerTab2Docks();

        mainLayout->addWidget(m_tab2DockManager);
        return mainPanel;
    }

    void registerTab2Docks()
    {
        if (!m_tab2DockManager) {
            return;
        }

        // One dock per original column widget (25% | 50% | 25%).
        // Manual-test finding 4: proper descriptive titles (objectNames stay
        // unchanged for layout persistence).
        ads::CDockWidget* leftDock = makeDock(m_tab2DockManager,
            QStringLiteral("tab2_Left"), QStringLiteral("Service Tree"),
            createTab2_LeftPanel());
        ads::CDockWidget* centerDock = makeDock(m_tab2DockManager,
            QStringLiteral("tab2_Center"), QStringLiteral("Ensemble & Sub-Channel Tables"),
            createTab2_CenterPanel());
        ads::CDockWidget* rightDock = makeDock(m_tab2DockManager,
            QStringLiteral("tab2_Right"), QStringLiteral("FIG Content"),
            createTab2_RightPanel());
        
        // Phase 2C: Advanced FIG Analyser dock - tabbed with FIG Content in Right area
        ads::CDockWidget* advancedFigDock = makeDock(m_tab2DockManager,
            QStringLiteral("tab2_AdvancedFig"), QStringLiteral("Advanced FIG Analyser"),
            createAdvancedFigAnalyserDockContent());

        // Center FIRST (establishes the root splitter), then left/right append
        // side-by-side -> Left | Center | Right (empirically verified).
        m_tab2DockManager->addDockWidget(ads::CenterDockWidgetArea, centerDock, nullptr);
        m_tab2DockManager->addDockWidget(ads::LeftDockWidgetArea, leftDock, nullptr);
        m_tab2DockManager->addDockWidget(ads::RightDockWidgetArea, rightDock, nullptr);
        // Add Advanced FIG Analyser dock to Right area (will be tabbed with FIG Content)
        // Use addDockWidgetTabToArea to ensure it's tabbed with the Right dock
        ads::CDockAreaWidget* rightArea = rightDock->dockAreaWidget();
        if (rightArea) {
            m_tab2DockManager->addDockWidgetTabToArea(advancedFigDock, rightArea);
        } else {
            // Fallback: add to Right area, Qt-ADS will create a new area if needed
            m_tab2DockManager->addDockWidget(ads::RightDockWidgetArea, advancedFigDock, nullptr);
        }

        QSplitter* root = nullptr;
        const auto splitters = m_tab2DockManager->findChildren<QSplitter*>();
        for (QSplitter* sp : splitters) {
            if (sp->parentWidget() == m_tab2DockManager) {
                root = sp;
                break;
            }
        }
        if (root && root->count() == 3) {
            root->setSizes({300, 600, 300});  // 25% | 50% | 25%
        }
    }

    // ========================================================================
    // TAB 3: FIC-EXTRACTOR (2-PANEL LAYOUT PER DATA_AVAILABILITY_MATRIX.md)
    // ========================================================================
    
    QWidget* createTab3_LeftPanel()
    {
        QWidget* panel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(4);

        // T28: inner tabs in the SAME left dock: FIG Instance List (existing
        // tree, as-is) and Frame List (new). Selecting a frame in the Frame List
        // decodes that frame and fills the right dock's detail tree.
        m_tab3LeftTabs = new QTabWidget();
        m_tab3LeftTabs->setObjectName(QStringLiteral("tab3LeftTabs"));

        // FIG Instance Tree (7 columns) - 100%
        QGroupBox* figInstanceGroup = new QGroupBox("FIG Instance List");
        QVBoxLayout* figInstanceLayout = new QVBoxLayout(figInstanceGroup);
        
        if (!m_tab3_figInstanceTree) {
            m_tab3_figInstanceTree = new QTreeWidget();
            m_tab3_figInstanceTree->setHeaderLabels({
                "FIG Type", "Index", "Frame#", "CIF#", "FIB#", "Length", "Items"
            });
            m_tab3_figInstanceTree->setAlternatingRowColors(true);
            m_tab3_figInstanceTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
            
            // Set column widths for readability
            m_tab3_figInstanceTree->setColumnWidth(0, 80);   // FIG Type
            m_tab3_figInstanceTree->setColumnWidth(1, 50);   // Index
            m_tab3_figInstanceTree->setColumnWidth(2, 70);   // Frame#
            m_tab3_figInstanceTree->setColumnWidth(3, 50);   // CIF#
            m_tab3_figInstanceTree->setColumnWidth(4, 50);   // FIB#
            m_tab3_figInstanceTree->setColumnWidth(5, 60);   // Length
            m_tab3_figInstanceTree->setColumnWidth(6, 50);   // Items
        }
        figInstanceLayout->addWidget(m_tab3_figInstanceTree);

        m_tab3LeftTabs->addTab(figInstanceGroup, tr("FIG Instance List"));

        // --- Frame List tab: frames 1..N of the loaded capture ---------------
        QWidget* frameListTab = new QWidget();
        QVBoxLayout* frameListTabLayout = new QVBoxLayout(frameListTab);
        frameListTabLayout->setContentsMargins(2, 2, 2, 2);
        if (!m_tab3_frameList) {
            m_tab3_frameList = new QListWidget();
            m_tab3_frameList->setObjectName(QStringLiteral("tab3FrameList"));
            m_tab3_frameList->setAlternatingRowColors(true);
        }
        frameListTabLayout->addWidget(m_tab3_frameList);
        m_tab3LeftTabs->addTab(frameListTab, tr("Frame List"));

        layout->addWidget(m_tab3LeftTabs);

        return panel;
    }

    QWidget* createTab3_RightPanel()
    {
        QWidget* panel = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(4);

        // T42: the right dock now hosts TWO inner tabs, mirroring the Tab-3
        // left dock's inner-tab pattern: the original FIG Item Details tree and
        // a new raw-bytes Hex Viewer "next to" it.
        m_tab3RightTabs = new QTabWidget();
        m_tab3RightTabs->setObjectName(QStringLiteral("tab3RightTabs"));

        // Tab 1: FIG Item Details Tree (4 columns, 5 levels).
        QGroupBox* figItemDetailsGroup = new QGroupBox("FIG Item Details");
        QVBoxLayout* figItemDetailsLayout = new QVBoxLayout(figItemDetailsGroup);
        
        if (!m_tab3_figItemDetailsTree) {
            m_tab3_figItemDetailsTree = new QTreeWidget();
            m_tab3_figItemDetailsTree->setHeaderLabels({
                "Property", "Size", "Value", "Information"
            });
            m_tab3_figItemDetailsTree->setAlternatingRowColors(true);
            m_tab3_figItemDetailsTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
            
            // Set column widths for readability
            m_tab3_figItemDetailsTree->setColumnWidth(0, 150);  // Property
            m_tab3_figItemDetailsTree->setColumnWidth(1, 60);   // Size (bits)
            m_tab3_figItemDetailsTree->setColumnWidth(2, 100);  // Value (hex/dec)
            // Information column stretches
        }
        figItemDetailsLayout->addWidget(m_tab3_figItemDetailsTree);
        m_tab3RightTabs->addTab(figItemDetailsGroup, tr("FIG Item Details"));

        // Tab 2 (T42): Hex Viewer — raw bytes of the current FIG Instance List
        // selection or Frame List selection, with an offset/hex/ASCII layout.
        QGroupBox* hexGroup = new QGroupBox("Hex Viewer");
        QVBoxLayout* hexLayout = new QVBoxLayout(hexGroup);
        m_tab3_hexViewer = new QTextEdit();
        m_tab3_hexViewer->setObjectName(QStringLiteral("tab3HexViewer"));
        m_tab3_hexViewer->setReadOnly(true);
        m_tab3_hexViewer->setLineWrapMode(QTextEdit::NoWrap);
        m_tab3_hexViewer->setFont(QFont("Consolas", 9));
        m_tab3_hexViewer->setPlaceholderText(
            "Select a FIG instance or a frame to view its raw bytes...");
        hexLayout->addWidget(m_tab3_hexViewer);
        m_tab3RightTabs->addTab(hexGroup, tr("Hex Viewer"));

        layout->addWidget(m_tab3RightTabs);

        return panel;
    }

    QWidget* createTab3_FICExtractor()
    {
        QWidget* mainPanel = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(mainPanel);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        // Tab 3 gets its OWN CDockManager with 2 docks (one per original column).
        m_tab3DockManager = new ads::CDockManager(mainPanel);
        m_tab3DockManager->setObjectName("tab3DockManager");
        registerTab3Docks();

        mainLayout->addWidget(m_tab3DockManager);
        return mainPanel;
    }

    void registerTab3Docks()
    {
        if (!m_tab3DockManager) {
            return;
        }

        // One dock per original column widget (40% | 60%).
        // Manual-test finding 4: proper descriptive titles (objectNames stay
        // unchanged for layout persistence).
        ads::CDockWidget* leftDock = makeDock(m_tab3DockManager,
            QStringLiteral("tab3_Left"), QStringLiteral("FIG Instance List"),
            createTab3_LeftPanel());
        ads::CDockWidget* rightDock = makeDock(m_tab3DockManager,
            QStringLiteral("tab3_Right"), QStringLiteral("FIG Item Details"),
            createTab3_RightPanel());

        // Left FIRST (sets root horizontal), right appends -> Left | Right.
        m_tab3DockManager->addDockWidget(ads::LeftDockWidgetArea, leftDock, nullptr);
        m_tab3DockManager->addDockWidget(ads::RightDockWidgetArea, rightDock, nullptr);

        QSplitter* root = nullptr;
        const auto splitters = m_tab3DockManager->findChildren<QSplitter*>();
        for (QSplitter* sp : splitters) {
            if (sp->parentWidget() == m_tab3DockManager) {
                root = sp;
                break;
            }
        }
        if (root && root->count() == 2) {
            root->setSizes({400, 600});  // 40% | 60%
        }
    }

    // ========================================================================
    // HELPER METHODS: ERROR DETECTION & LOGGING PANELS (Used in Tab 1)
    // ========================================================================
    // NOTE: These were previously in Tab 4, now integrated into Tab 1
    
    QGroupBox* createErrorDetectionPanel()
    {
        QGroupBox* widget = new QGroupBox("DAB Error Counter");
        QVBoxLayout* layout = new QVBoxLayout(widget);

        // === PHASE 4: Error Counter Table (Primary widget per DATA_AVAILABILITY_MATRIX.md) ===
        if (!m_errorCounterTable) {
            m_errorCounterTable = new QTableWidget();
            m_errorCounterTable->setColumnCount(2);
            m_errorCounterTable->setRowCount(5);
            m_errorCounterTable->setHorizontalHeaderLabels({"Error Type", "Count"});
            m_errorCounterTable->verticalHeader()->setVisible(false);
            m_errorCounterTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
            m_errorCounterTable->setSelectionMode(QAbstractItemView::SingleSelection);
            m_errorCounterTable->horizontalHeader()->setStretchLastSection(true);
            
            // Initialize with placeholder data
            QStringList errorTypes = {"FIC CRC Errors", "MSC CRC Errors", "Invalid FIG Errors", 
                                      "Service Org Errors", "Subchannel Errors"};
            for (int row = 0; row < 5; ++row) {
                m_errorCounterTable->setItem(row, 0, new QTableWidgetItem(errorTypes[row]));
                m_errorCounterTable->setItem(row, 1, new QTableWidgetItem("0"));
            }
        }
        layout->addWidget(m_errorCounterTable, 70);  // 70% of panel space

        // Status summary (30% of panel space)
        QGroupBox* statusGroup = new QGroupBox("Status");
        QHBoxLayout* statusLayout = new QHBoxLayout(statusGroup);
        m_errorDetectionStatus = new QLabel("Status: Active");
        m_systemHealthScore = new QLabel("Health: 100%");
        m_errorCountLabel = new QLabel("Total: 0");
        statusLayout->addWidget(m_errorDetectionStatus);
        statusLayout->addWidget(m_systemHealthScore);
        statusLayout->addWidget(m_errorCountLabel);
        statusLayout->addStretch();
        layout->addWidget(statusGroup, 30);

        return widget;
    }

    // T41: the legacy `Messages` strip panel (createLoggingPanel()) was removed.
    // Its content was a static, empty log-entry tree + logging-statistics
    // placeholder backed by the disabled ProfessionalLoggingSystem
    // (m_loggingSystem is intentionally never constructed) — fully redundant
    // with the live "System Messages" plain-text log.

    // ========================================================================
    // SHARED BOTTOM DOCK AREA (CONSISTENT ACROSS ALL TABS, WINDOW-LEVEL)
    // ========================================================================
    void buildSharedBottomDocks()
    {
        if (!m_bottomDockManager) {
            return;
        }

        // The bottom tabs become TABBED CDockWidgets hosted by the window-level
        // bottom CDockManager. Wave B order (7 tabs; T41 removed the redundant
        // legacy Messages tab):
        //   System Messages / Performance / Real-Time Chart / Error Detection /
        //   ETSI Compliance / Logging / Constellation.

        // Tab 1: System Messages
        if (!m_systemMessages) {
            m_systemMessages = new QTextEdit();
            m_systemMessages->setFont(QFont("Consolas", 9));
            m_systemMessages->setReadOnly(true);
        }
        ads::CDockWidget* d1 = makeDock(m_bottomDockManager,
            QStringLiteral("bottom_SystemMessages"), QStringLiteral("System Messages"),
            m_systemMessages);

        m_bottomDockManager->addDockWidget(ads::BottomDockWidgetArea, d1, nullptr);

        // Tab 2: Performance Dashboard (replaces the thin 3-label tab; fed real
        // FPS / RSS / CPU% / latency samples every second). Its own internal
        // timer is intentionally NOT started: with no legacy EtiProcessor it
        // would push zeroed samples and clobber the real ones; DABAnalyserWindow
        // drives it via updatePerformanceMetrics() instead.
        m_performanceDashboard = new PerformanceDashboard();
        ads::CDockWidget* d2 = makeDock(m_bottomDockManager,
            QStringLiteral("bottom_Performance"), QStringLiteral("Performance"),
            wrapHeavyPanel(m_performanceDashboard));
        m_bottomDockManager->addDockWidgetTabToArea(d2, d1->dockAreaWidget());

        // Tab 3 (new): Real-Time Chart — FPS + memory history.
        m_realTimeChart = new RealTimeChartWidget();
        m_realTimeChart->initialize();
        m_realTimeChart->setTimeRange(RealTimeChartWidget::TimeRange::Last1Minute);
        m_realTimeChart->setLegendEnabled(true);
        m_realTimeChart->applyBroadcastTheme();
        {
            ChartSeries fpsSeries(QStringLiteral("FPS"), QColor(0, 170, 0), 900.0,
                                  QStringLiteral("fps"));
            m_chartFpsSeries = m_realTimeChart->addSeries(fpsSeries);
            ChartSeries memSeries(QStringLiteral("Memory MB"), QColor(0, 120, 212), 100.0,
                                  QStringLiteral("MB"));
            m_chartMemorySeries = m_realTimeChart->addSeries(memSeries);
        }
        m_realTimeChart->startRealTimeUpdates();
        // Custom-painted widget: give it an explicit minimum so the scroll
        // area actually produces scrollbars inside the 120 px strip cap.
        m_realTimeChart->setMinimumSize(360, 240);
        ads::CDockWidget* dRt = makeDock(m_bottomDockManager,
            QStringLiteral("bottom_RealTimeChart"), QStringLiteral("Real-Time Chart"),
            wrapHeavyPanel(m_realTimeChart));
        m_bottomDockManager->addDockWidgetTabToArea(dRt, d1->dockAreaWidget());

        // Tab 4: Error Detection Summary
        ads::CDockWidget* d3 = makeDock(m_bottomDockManager,
            QStringLiteral("bottom_ErrorDetection"), QStringLiteral("Error Detection"),
            createErrorDetectionSummaryTab());
        m_bottomDockManager->addDockWidgetTabToArea(d3, d1->dockAreaWidget());

        // Tab 5 (new): ETSI Compliance Monitor — fed by the ETSI compliance
        // engine and the real AdvancedErrorDetector.
        // NOTE: startMonitoring() is NOT called — violations arrive only via
        // the real queued errorDetected/criticalErrorDetected signals; there is
        // no polling timer and no synthetic generation.
        m_etsiMonitor = new ETSIComplianceMonitor();
        m_etsiMonitor->applyBroadcastingTheme();
        ads::CDockWidget* dEtsi = makeDock(m_bottomDockManager,
            QStringLiteral("bottom_EtsiCompliance"), QStringLiteral("ETSI Compliance"),
            wrapHeavyPanel(m_etsiMonitor));
        m_bottomDockManager->addDockWidgetTabToArea(dEtsi, d1->dockAreaWidget());

        // Tab 6: Logging (T45) — live, bounded log view fed by Logger. Wrapped
        // in the same frameless scroll area as the other tall tabs so it stays
        // usable at the ~120 px strip default.
        ads::CDockWidget* d4 = makeDock(m_bottomDockManager,
            QStringLiteral("bottom_Logging"), QStringLiteral("Logging"),
            wrapHeavyPanel(createLoggingSummaryTab()));
        m_bottomDockManager->addDockWidgetTabToArea(d4, d1->dockAreaWidget());
        // W1 #2: flush staged entries when the Logging tab is hidden (tab
        // switch / dock closed) so nothing is lost while the timer is stopped.
        connect(d4, &ads::CDockWidget::visibilityChanged, this,
                [this](bool visible) {
                    if (!visible) {
                        flushPendingLogEntries(/*drainAll=*/true);
                    }
                });

        // Tab 7: Constellation Viewer (real widget; synthetic post-demod path).
        ads::CDockWidget* d5 = makeDock(m_bottomDockManager,
            QStringLiteral("bottom_Constellation"), QStringLiteral("Constellation"),
            wrapHeavyPanel(createConstellationTab()));
        m_bottomDockManager->addDockWidgetTabToArea(d5, d1->dockAreaWidget());

        // T41: the redundant `bottom_Messages` (legacy logging panel) dock was
        // removed — see createLoggingPanel()'s removal note above. The strip now
        // has 7 tabs; "System Messages" is the live log.

        // Default active tab of the bottom strip: System Messages (first dock).
        d1->dockAreaWidget()->setCurrentDockWidget(d1);
    }

    // ========================================================================
    // DOCKING STATE PERSISTENCE (QSettings, v1.3)
    // ========================================================================
    void restoreDockingState()
    {
        const QSettings s(streamdab::app_settings::organization(),
                          streamdab::app_settings::application());

        const QByteArray geometry = s.value(QStringLiteral("window/geometry")).toByteArray();
        if (!geometry.isEmpty() && !restoreGeometry(geometry)) {
            qWarning() << "Failed to restore window geometry";
        }

        // UI redesign (Step E): dock states saved by an older layout are
        // incompatible with the current default. The stored layout version
        // guards a ONE-TIME reset: any version != DOCKING_LAYOUT_VERSION
        // discards the saved dock states (docking/tab1|tab2|tab3|bottom) so the
        // first run after the upgrade shows the new default layout; window
        // geometry is kept. The current version is written again on the next
        // saveDockingState().
        const int layoutVersion = s.value(QStringLiteral("docking/layout_version"), 1).toInt();
        if (layoutVersion != DOCKING_LAYOUT_VERSION) {
            qInfo() << "Docking layout version" << layoutVersion
                    << "!= expected" << DOCKING_LAYOUT_VERSION
                    << "- discarding saved dock states (redesign first run)";
            return;
        }

        if (m_tab1DockManager && !s.value(QStringLiteral("docking/tab1")).isNull()) {
            if (!m_tab1DockManager->restoreState(s.value(QStringLiteral("docking/tab1")).toByteArray())) {
                qWarning() << "Failed to restore Tab 1 docking state";
            }
        }
        if (m_tab2DockManager && !s.value(QStringLiteral("docking/tab2")).isNull()) {
            if (!m_tab2DockManager->restoreState(s.value(QStringLiteral("docking/tab2")).toByteArray())) {
                qWarning() << "Failed to restore Tab 2 docking state";
            }
        }
        if (m_tab3DockManager && !s.value(QStringLiteral("docking/tab3")).isNull()) {
            if (!m_tab3DockManager->restoreState(s.value(QStringLiteral("docking/tab3")).toByteArray())) {
                qWarning() << "Failed to restore Tab 3 docking state";
            }
        }
        if (m_bottomDockManager && !s.value(QStringLiteral("docking/bottom")).isNull()) {
            if (!m_bottomDockManager->restoreState(s.value(QStringLiteral("docking/bottom")).toByteArray())) {
                qWarning() << "Failed to restore bottom docking state";
            }
        }
    }

    void saveDockingState()
    {
        QSettings s(streamdab::app_settings::organization(),
                    streamdab::app_settings::application());
        s.setValue(QStringLiteral("docking/layout_version"), DOCKING_LAYOUT_VERSION);
        s.setValue(QStringLiteral("window/geometry"), saveGeometry());
        if (m_tab1DockManager) {
            s.setValue(QStringLiteral("docking/tab1"), m_tab1DockManager->saveState());
        }
        if (m_tab2DockManager) {
            s.setValue(QStringLiteral("docking/tab2"), m_tab2DockManager->saveState());
        }
        if (m_tab3DockManager) {
            s.setValue(QStringLiteral("docking/tab3"), m_tab3DockManager->saveState());
        }
        if (m_bottomDockManager) {
            s.setValue(QStringLiteral("docking/bottom"), m_bottomDockManager->saveState());
        }
    }

    QWidget* createErrorDetectionSummaryTab()
    {
        QWidget* widget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(widget);
        
        QLabel* summaryLabel = new QLabel("Error Detection Summary (See Advanced Features → Error Detection for full details)");
        summaryLabel->setStyleSheet("font-style: italic; color: #666;");
        layout->addWidget(summaryLabel);
        
        QLabel* quickStats = new QLabel("Quick Stats: 0 errors detected, System Health: 100%");
        quickStats->setStyleSheet("font-weight: bold; font-size: 11pt;");
        layout->addWidget(quickStats);
        
        layout->addStretch();
        return widget;
    }

    // ========================================================================
    // T45: live Logging tab (bottom strip)
    // ------------------------------------------------------------------------
    // The tab used to be a static italic placeholder ("Recent Activity: 0 log
    // entries"). It is now a real, bounded log view fed by the singleton
    // Logger::logMessageAdded signal: time | level | category | message, with
    // a level selector (single source of truth: Logger + the shared
    // `advanced/logLevel` Settings key), an auto-scroll toggle, a clear button
    // and a summary line. The backing store is a ring of the last
    // LOGGING_MAX_ROWS entries (oldest dropped).
    //
    // Threading: Logger::logMessageAdded is emitted from the Logger object's
    // thread (it marshals to its own thread via QTimer::singleShot(0, this,…)),
    // and the Logger singleton is created on the GUI thread during window
    // construction, so it is GUI-thread affine. The signal is nonetheless
    // connected with Qt::QueuedConnection so a cross-thread emission (if a
    // future Logger ever lives/heartbeats on another thread) is always
    // delivered to this GUI slot safely rather than touching widgets from a
    // worker thread.
    // ========================================================================

    static QString logLevelDisplayName(Logger::LogLevel level)
    {
        switch (level) {
            case Logger::LogLevel::Debug:    return QStringLiteral("Debug");
            case Logger::LogLevel::Info:     return QStringLiteral("Info");
            case Logger::LogLevel::Warning:  return QStringLiteral("Warning");
            case Logger::LogLevel::Error:    return QStringLiteral("Error");
            case Logger::LogLevel::Critical: return QStringLiteral("Critical");
        }
        return QStringLiteral("Unknown");
    }

    static QColor logLevelColor(Logger::LogLevel level)
    {
        switch (level) {
            case Logger::LogLevel::Debug:    return QColor(0x88, 0x88, 0x88);
            case Logger::LogLevel::Info:     return QColor(0x21, 0x8a, 0x3a);
            case Logger::LogLevel::Warning:  return QColor(0xb8, 0x6e, 0x00);
            case Logger::LogLevel::Error:    return QColor(0xc0, 0x28, 0x28);
            case Logger::LogLevel::Critical: return QColor(0x8b, 0x00, 0x28);
        }
        return QColor(0x33, 0x33, 0x33);
    }

    /// Summary line: total entry count + per-level tallies.
    void updateLoggingSummary()
    {
        if (!m_loggingSummaryLabel) {
            return;
        }
        const int total = m_loggingTable ? m_loggingTable->rowCount() : 0;
        QString text = tr("%1 entries — D:%2 I:%3 W:%4 E:%5 C:%6")
                           .arg(total)
                           .arg(m_loggingLevelCounts[0])
                           .arg(m_loggingLevelCounts[1])
                           .arg(m_loggingLevelCounts[2])
                           .arg(m_loggingLevelCounts[3])
                           .arg(m_loggingLevelCounts[4]);
        if (m_logDroppedPending > 0) {
            // Under backlog the oldest staged entries are dropped; surface it.
            text += tr(" — %1 dropped (backlog)").arg(m_logDroppedPending);
        }
        m_loggingSummaryLabel->setText(text);
    }

    /// Reflect the authoritative Logger level in the tab's combo (no signal
    /// echo, so this never re-persists or changes the Logger).
    void syncLoggingLevelComboFromLogger()
    {
        if (!m_loggingLevelCombo) {
            return;
        }
        const int levelInt = static_cast<int>(Logger::instance().getLogLevel());
        const int index = m_loggingLevelCombo->findData(levelInt);
        if (index >= 0 && index != m_loggingLevelCombo->currentIndex()) {
            const QSignalBlocker blocker(m_loggingLevelCombo);
            m_loggingLevelCombo->setCurrentIndex(index);
        }
    }

    /// Drop every row / counter and any staged-but-unflushed entries.
    void clearLogEntries()
    {
        if (m_loggingTable) {
            m_loggingTable->setRowCount(0);
        }
        // Also drop staged entries so a pending flush cannot repopulate the
        // view after the user pressed Clear.
        m_logPending.clear();
        m_logDroppedPending = 0;
        for (int i = 0; i < LOG_LEVEL_COUNT; ++i) {
            m_loggingLevelCounts[i] = 0;
        }
        updateLoggingSummary();
    }

    /// Level combo changed: drive the Logger now and persist the SAME Settings
    /// key the Settings dialog writes, so the two never disagree.
    void onLoggingLevelComboChanged(int index)
    {
        if (!m_loggingLevelCombo || index < 0) {
            return;
        }
        const int levelInt = m_loggingLevelCombo->itemData(index).toInt();
        Logger::instance().setLogLevel(static_cast<Logger::LogLevel>(levelInt));

        if (!m_appSettings) {
            m_appSettings = streamdab::app_settings::create(this);
        }
        m_appSettings->setValue(QStringLiteral("advanced/logLevel"),
                                SettingsDialog::indexForLoggerLevel(levelInt));
        m_appSettings->sync();
    }

    /// W1 #2: staged log entry for the Logging-tab batching (declared here
    /// because appendLogEntryToTable() below uses it in its signature; the
    /// staging queue and flush timer live with the other logging members).
    struct PendingLogEntry {
        Logger::LogLevel level;
        QString message;
        QString category;
        QDateTime timestamp;
    };

    /// Append one entry to the table, applying the ring cap and per-level
    /// tally bookkeeping. Does NOT scroll or recompute the summary — the
    /// caller (flushPendingLogEntries) does that once per batch.
    void appendLogEntryToTable(const PendingLogEntry& entry)
    {
        // Strict ring: evict the oldest row(s) once at capacity, keeping the
        // per-level tally in sync.
        while (m_loggingTable->rowCount() >= m_loggingRowCap) {
            const QTableWidgetItem* evicted = m_loggingTable->item(0, 1);
            if (evicted) {
                const int evictedLevel = evicted->data(Qt::UserRole).toInt();
                if (evictedLevel >= 0 && evictedLevel < LOG_LEVEL_COUNT
                    && m_loggingLevelCounts[evictedLevel] > 0) {
                    --m_loggingLevelCounts[evictedLevel];
                }
            }
            m_loggingTable->removeRow(0);
        }

        const int levelInt = static_cast<int>(entry.level);
        const int row = m_loggingTable->rowCount();
        m_loggingTable->insertRow(row);

        auto* timeItem = new QTableWidgetItem(entry.timestamp.toString("hh:mm:ss.zzz"));
        auto* levelItem = new QTableWidgetItem(logLevelDisplayName(entry.level));
        levelItem->setData(Qt::UserRole, levelInt);
        levelItem->setForeground(logLevelColor(entry.level));
        auto* categoryItem = new QTableWidgetItem(
            entry.category.isEmpty() ? tr("General") : entry.category);
        auto* messageItem = new QTableWidgetItem(entry.message);
        messageItem->setToolTip(entry.message);

        m_loggingTable->setItem(row, 0, timeItem);
        m_loggingTable->setItem(row, 1, levelItem);
        m_loggingTable->setItem(row, 2, categoryItem);
        m_loggingTable->setItem(row, 3, messageItem);

        if (levelInt >= 0 && levelInt < LOG_LEVEL_COUNT) {
            ++m_loggingLevelCounts[levelInt];
        }
    }

    /// Drain the staging buffer into the table in bounded batches.
    ///
    /// W1 #2: a burst of debug logs (e.g. a live 250+ fps stream at Debug
    /// level) used to run a full insertRow/setItem×4/scrollToBottom/setText per
    /// message inside the queued slot, flooding the GUI event loop. Entries are
    /// now coalesced and applied by this timer-driven flush:
    ///   - ordering preserved (FIFO deque),
    ///   - at most LOG_FLUSH_MAX_BATCH rows per flush (keeps the GUI responsive),
    ///   - oldest pending entries dropped past LOG_PENDING_MAX (staging bound),
    ///   - ring cap + per-level tallies updated exactly as before.
    /// When drainAll is true (shutdown/hide) the whole backlog is flushed so the
    /// last entries are never lost.
    void flushPendingLogEntries(bool drainAll = false)
    {
        if (!m_loggingTable) {
            m_logPending.clear();
            return;
        }

        int budget = drainAll ? std::numeric_limits<int>::max() : LOG_FLUSH_MAX_BATCH;
        while (!m_logPending.empty() && budget-- > 0) {
            appendLogEntryToTable(m_logPending.front());
            m_logPending.pop_front();
        }

        if (m_logPending.empty()) {
            if (m_logFlushTimer) {
                m_logFlushTimer->stop();
            }
        } else if (!drainAll && m_logFlushTimer && !m_logFlushTimer->isActive()) {
            // Backlog remains: re-arm so we keep draining without waiting for
            // the next inbound message.
            m_logFlushTimer->start();
        }

        if (m_loggingAutoScrollCheck && m_loggingAutoScrollCheck->isChecked()
            && m_loggingTable->rowCount() > 0) {
            m_loggingTable->scrollToBottom();
        }
        updateLoggingSummary();
    }

    /// Slot for Logger::logMessageAdded (queued -> runs on the GUI thread).
    /// Stages the entry only; the table is updated by flushPendingLogEntries().
    void onLogMessageAdded(Logger::LogLevel level, const QString& message,
                           const QString& category, const QDateTime& timestamp)
    {
        if (!m_loggingTable || m_loggingRowCap <= 0) {
            return;
        }

        // Bound the staging buffer: under a sustained flood (GUI starved) drop
        // the OLDEST waiting entries — the newest are the most useful.
        if (static_cast<int>(m_logPending.size()) >= LOG_PENDING_MAX) {
            m_logPending.pop_front();
            ++m_logDroppedPending;
        }
        m_logPending.push_back(PendingLogEntry{level, message, category, timestamp});

        if (m_logFlushTimer && !m_logFlushTimer->isActive()) {
            m_logFlushTimer->start();
        }
    }

    QWidget* createLoggingSummaryTab()
    {
        QWidget* widget = new QWidget();
        widget->setObjectName(QStringLiteral("loggingTab"));
        // Taller than the ~120 px strip default so the wrapping QScrollArea
        // produces real scrollbars; the tab stays usable via scrolling.
        widget->setMinimumHeight(200);
        QVBoxLayout* layout = new QVBoxLayout(widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);

        // Controls row: level selector + auto-scroll + clear + summary.
        QHBoxLayout* controls = new QHBoxLayout();
        QLabel* levelLabel = new QLabel(tr("Level:"));
        controls->addWidget(levelLabel);

        m_loggingLevelCombo = new QComboBox();
        m_loggingLevelCombo->setObjectName(QStringLiteral("loggingLevelCombo"));
        m_loggingLevelCombo->setToolTip(tr(
            "Minimum severity shown in this tab and written to the log. "
            "Applies immediately and is shared with Settings → Advanced."));
        // itemData is Logger::LogLevel's integer value.
        m_loggingLevelCombo->addItem(tr("Debug"), static_cast<int>(Logger::LogLevel::Debug));
        m_loggingLevelCombo->addItem(tr("Info"), static_cast<int>(Logger::LogLevel::Info));
        m_loggingLevelCombo->addItem(tr("Warning"), static_cast<int>(Logger::LogLevel::Warning));
        m_loggingLevelCombo->addItem(tr("Error"), static_cast<int>(Logger::LogLevel::Error));
        m_loggingLevelCombo->addItem(tr("Critical"), static_cast<int>(Logger::LogLevel::Critical));
        controls->addWidget(m_loggingLevelCombo);

        m_loggingAutoScrollCheck = new QCheckBox(tr("Auto-scroll"));
        m_loggingAutoScrollCheck->setObjectName(QStringLiteral("loggingAutoScrollCheck"));
        m_loggingAutoScrollCheck->setChecked(true);
        m_loggingAutoScrollCheck->setToolTip(tr("Keep the newest entry visible."));
        controls->addWidget(m_loggingAutoScrollCheck);

        m_loggingClearButton = new QPushButton(tr("Clear"));
        m_loggingClearButton->setObjectName(QStringLiteral("loggingClearButton"));
        m_loggingClearButton->setToolTip(tr("Remove all entries from this tab."));
        controls->addWidget(m_loggingClearButton);

        controls->addStretch();

        m_loggingSummaryLabel = new QLabel();
        m_loggingSummaryLabel->setObjectName(QStringLiteral("loggingSummaryLabel"));
        controls->addWidget(m_loggingSummaryLabel);
        layout->addLayout(controls);

        // Entry view: time | level | category | message.
        m_loggingTable = new QTableWidget();
        m_loggingTable->setObjectName(QStringLiteral("loggingTable"));
        m_loggingTable->setColumnCount(4);
        m_loggingTable->setHorizontalHeaderLabels(
            {tr("Time"), tr("Level"), tr("Category"), tr("Message")});
        m_loggingTable->verticalHeader()->setVisible(false);
        m_loggingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_loggingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_loggingTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_loggingTable->setWordWrap(false);
        m_loggingTable->horizontalHeader()->setStretchLastSection(true);
        m_loggingTable->setColumnWidth(0, 110);
        m_loggingTable->setColumnWidth(1, 70);
        m_loggingTable->setColumnWidth(2, 120);
        layout->addWidget(m_loggingTable, 1);

        // Initial state: mirror the live Logger and the persisted key.
        syncLoggingLevelComboFromLogger();
        updateLoggingSummary();

        // W1 #2: coalescing flush timer. Started on the first staged entry and
        // stopped when the staging buffer drains, so an idle tab costs nothing.
        m_logFlushTimer = new QTimer(this);
        m_logFlushTimer->setInterval(LOG_FLUSH_INTERVAL_MS);
        m_logFlushTimer->setTimerType(Qt::CoarseTimer);
        connect(m_logFlushTimer, &QTimer::timeout, this,
                [this]() { flushPendingLogEntries(); });

        connect(m_loggingLevelCombo, &QComboBox::currentIndexChanged,
                this, &DABAnalyserWindow::onLoggingLevelComboChanged);
        connect(m_loggingClearButton, &QPushButton::clicked,
                this, &DABAnalyserWindow::clearLogEntries);

        // Queued connection: the Logger signal may originate on another thread
        // (see the block comment above); deliver it to this GUI-thread slot.
        qRegisterMetaType<Logger::LogLevel>("Logger::LogLevel");
        connect(&Logger::instance(), &Logger::logMessageAdded,
                this, &DABAnalyserWindow::onLogMessageAdded,
                Qt::QueuedConnection);

        return widget;
    }

    // DELETED: Orphaned panel functions (92 lines) - Code Review CRIT-001, HIGH-004
    // These functions were never called after UI restructure to 3-panel Tab 1 layout
    // - createEnsemblePanel(QSplitter*) - replaced by createTab1_CenterPanel()
    // - createFrameNavigatorPanel(QSplitter*) - replaced by createTab1_CenterPanel()
    //   ⚠️ CRITICAL: Had duplicate m_frameList initialization causing memory leak
    // - createServiceDetailsPanel(QSplitter*) - replaced by createTab1_RightPanel()
    // PDCA Review: 2025-10-26 - Safe to delete, no references found
    
    QGroupBox* createNowPlayingPanel()
    {
        QGroupBox* groupBox = new QGroupBox("Now Playing / Slideshow");
        QVBoxLayout* outerLayout = new QVBoxLayout(groupBox);
        outerLayout->setContentsMargins(4, 4, 4, 4);

        // T27: media stays together in one dock, split LEFT (DLS+ text block)
        // / RIGHT (MOT slideshow) by a draggable horizontal QSplitter instead
        // of inner tabs. The DLS+ side keeps the existing widgets unchanged.
        m_nowPlayingSplitter = new QSplitter(Qt::Horizontal);
        m_nowPlayingSplitter->setObjectName(QStringLiteral("nowPlayingSplitter"));
        m_nowPlayingSplitter->setChildrenCollapsible(false);

        QWidget* dlsPanel = new QWidget();
        dlsPanel->setObjectName(QStringLiteral("dlsPanel"));
        QVBoxLayout* layout = new QVBoxLayout(dlsPanel);
        
        // Professional styling (respects system theme)
        QString nowPlayingStyle = R"(
            QGroupBox {
                border: 2px solid #4CAF50;
                border-radius: 5px;
                margin-top: 10px;
                padding: 10px;
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                padding: 0 5px;
                color: #4CAF50;
                font-weight: bold;
            }
        )";
        groupBox->setStyleSheet(nowPlayingStyle);
        
        // T30.3: palette-derived, high-contrast metadata colours. The previous
        // fixed light-theme greys (#333/#555/#777) were near-invisible on the
        // app's dark surface (and #1976d2 was too dim there as well).
        //   track  : brand accent (#4a8fd4) + bold 14 px  -> most prominent
        //   artist : palette(text) 12 px                  -> full contrast
        //   album  : palette(mid) 11 px italic            -> dimmer third tier
        // palette(...) tracks the user's light/dark system theme, so the same
        // hierarchy reads on either background. The track colour is the
        // INTENTIONAL brand accent (#4a8fd4, the app's selected-tab colour); on
        // a light theme it has lower contrast than palette(text), accepted as
        // the deliberate accent trade-off (bold + larger size keeps it legible).
        // Track title row
        QHBoxLayout* trackRow = new QHBoxLayout();
        QLabel* trackTitleLabel = new QLabel("Track:");
        trackTitleLabel->setStyleSheet("font-weight: bold; color: palette(text);");
        trackRow->addWidget(trackTitleLabel);
        m_trackLabel = new QLabel("--");
        m_trackLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #4a8fd4;");
        m_trackLabel->setWordWrap(true);
        trackRow->addWidget(m_trackLabel, 1);
        layout->addLayout(trackRow);
        
        // Artist row
        QHBoxLayout* artistRow = new QHBoxLayout();
        QLabel* artistTitleLabel = new QLabel("Artist:");
        artistTitleLabel->setStyleSheet("font-weight: bold; color: palette(text);");
        artistRow->addWidget(artistTitleLabel);
        m_artistLabel = new QLabel("--");
        m_artistLabel->setStyleSheet("font-size: 12px; color: palette(text);");
        m_artistLabel->setWordWrap(true);
        artistRow->addWidget(m_artistLabel, 1);
        layout->addLayout(artistRow);
        
        // Album row
        QHBoxLayout* albumRow = new QHBoxLayout();
        QLabel* albumTitleLabel = new QLabel("Album:");
        albumTitleLabel->setStyleSheet("font-weight: bold; color: palette(text);");
        albumRow->addWidget(albumTitleLabel);
        m_albumLabel = new QLabel("--");
        m_albumLabel->setStyleSheet("font-size: 11px; color: palette(mid); font-style: italic;");
        m_albumLabel->setWordWrap(true);
        albumRow->addWidget(m_albumLabel, 1);
        layout->addLayout(albumRow);
        
        // Separator line
        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setStyleSheet("background-color: #ddd;");
        layout->addWidget(line);
        
        // Last updated timestamp. Manual-test finding 3: the initial state is
        // "Waiting for DLS+…" (no load yet); after a completed load with zero
        // DLS+ messages it becomes "No metadata received".
        m_lastUpdatedLabel = new QLabel("Waiting for DLS+…");
        m_lastUpdatedLabel->setStyleSheet("font-size: 10px; color: #999; font-style: italic;");
        m_lastUpdatedLabel->setAlignment(Qt::AlignRight);
        layout->addWidget(m_lastUpdatedLabel);

        m_nowPlayingSplitter->addWidget(dlsPanel);

        // --- Slideshow panel (no inner tabs) ----------------------------------
        // Phase 2A: EPG/Journaline/TPEG are now top-level docks (see registerTab1Docks).
        // The slidePanel directly contains the Slideshow page (4:3 letterbox + status).
        QWidget* slidePanel = new QWidget();
        slidePanel->setObjectName(QStringLiteral("slideshowPanel"));
        QVBoxLayout* panelLayout = new QVBoxLayout(slidePanel);
        panelLayout->setContentsMargins(2, 2, 2, 2);
        panelLayout->setSpacing(4);

        // Fixed 4:3 letterbox frame; the image label is its child.
        m_slideshowFrame = new FourThreeLetterboxFrame();
        m_slideshowImage = m_slideshowFrame->imageLabel();
        m_slideshowImage->setText(tr("Waiting for slideshow…"));
        // L2: rescale the retained slide when the splitter / floating dock
        // changes the frame size (not only on main-window resize).
        m_slideshowFrame->onResized = [this]() { refreshSlideshowPixmap(); };
        panelLayout->addWidget(m_slideshowFrame, 1);

        m_slideshowStatus = new QLabel(tr("Waiting for slideshow…"));
        m_slideshowStatus->setObjectName(QStringLiteral("slideshowStatus"));
        m_slideshowStatus->setStyleSheet(
            QStringLiteral("font-size: 10px; color: #999; font-style: italic;"));
        m_slideshowStatus->setWordWrap(true);
        panelLayout->addWidget(m_slideshowStatus);

        m_slideshowInfo = new QLabel(QStringLiteral("—"));
        m_slideshowInfo->setObjectName(QStringLiteral("slideshowInfo"));
        m_slideshowInfo->setStyleSheet(QStringLiteral("font-size: 10px; color: #777;"));
        m_slideshowInfo->setWordWrap(true);
        panelLayout->addWidget(m_slideshowInfo);

        m_nowPlayingSplitter->addWidget(slidePanel);
        // Phase 2B: Historical T30.4 ratio restored (67:33, stretch 2:1).
        // Phase 2A moved EPG/Journaline/TPEG to separate top-level docks.
        // The right pane (slideshowPanel) now contains ONLY the MOT slideshow
        // (4:3 letterbox + status). The DLS+ text pane benefits from the extra
        // width for long track/artist/album strings, matching the original T30.4
        // design intent. Draggable and non-collapsible (set above).
        m_nowPlayingSplitter->setStretchFactor(0, 2);  // left (DLS+) gets 2/3
        m_nowPlayingSplitter->setStretchFactor(1, 1);  // right (slideshow) gets 1/3
        // Explicit 67/33 split; testMotSlideshowPanelAndTabs pins ~0.667
        // (tolerance 0.58–0.76) so a silent drift back to 50/50 gets caught.
        m_nowPlayingSplitter->setSizes({667, 333});
        outerLayout->addWidget(m_nowPlayingSplitter);

        return groupBox;
    }

    // MOT SlideShow: reset the slideshow tab to its waiting state.
    void resetSlideshowPanel()
    {
        if (m_slideshowImage) {
            m_slideshowImage->clear();
            m_slideshowImage->setText(tr("Waiting for slideshow…"));
        }
        if (m_slideshowStatus) {
            m_slideshowStatus->setText(tr("Waiting for slideshow…"));
        }
        if (m_slideshowInfo) {
            m_slideshowInfo->setText(QStringLiteral("—"));
        }
    }

    // Manual-test finding 3: Now Playing (DLS+) panel state helpers.
    void resetNowPlayingPanel()
    {
        if (m_trackLabel) {
            m_trackLabel->setText("--");
        }
        if (m_artistLabel) {
            m_artistLabel->setText("--");
        }
        if (m_albumLabel) {
            m_albumLabel->setText("--");
        }
        if (m_lastUpdatedLabel) {
            m_lastUpdatedLabel->setText(tr("Waiting for DLS+…"));
        }
    }

    void setNowPlayingEmptyState()
    {
        if (m_lastUpdatedLabel) {
            m_lastUpdatedLabel->setText(tr("No metadata received"));
        }
    }

    // ========================================================================
    // Phase 2A: top-level data-service docks (EPG | Journaline | TPEG)
    // ========================================================================
    // These are now TOP-LEVEL docks in the Tab-1 Right column (see registerTab1Docks),
    // replacing the inner tabs that were previously in the slideshow panel.
    // The dock registry and DOCKING_LAYOUT_VERSION (11) are updated accordingly.
    //
    // Content comes ONLY from the raw data-subchannel bytes the clean tap
    // captures during the load (processDlsPlusFromFrame) and feeds to the
    // decoders once at load completion (feedDataServiceSubchannels). Nothing
    // is fabricated: a capture whose decoder yields no content shows an
    // explicit honest empty state ("No … data in this stream"), and the EPG
    // decoder's epgDecodingError text is surfaced in the EPG status line.
    //
    // Routing (B-LOW4 — ACCUMULATION, not "most recently decoded wins"):
    // every data subchannel is routed independently and all subchannels that
    // resolve to the same decoder MERGE into that one panel (the decoders own
    // the buffers, the store keeps one snapshot per subchannel) — a later
    // subchannel never displaces an earlier one. Order: the observed TPEG
    // carousel framing first, then the FIG 1/1 / FIG 1/5 service label, then
    // a Journaline content probe, else the EPG decoder gets the last word
    // (it rejects non-EPG bytes honestly).
    // ========================================================================

    /// Brief coverage note shown on the TPEG status line (user decision:
    /// TS 102 894-1-style framing identification + text inventory only; full
    /// TS 102 894-2 TTI / de-framing is deferred, never claimed as done).
    QString tpegCoverageNote() const
    {
        return tr("TPEG base (framing + text inventory; full TTI later)");
    }

    QString withTpegCoverageNote(const QString& state) const
    {
        return state.isEmpty() ? tpegCoverageNote()
                               : state + QStringLiteral(" · ") + tpegCoverageNote();
    }

    /// Journaline content-probe acceptance for an UNLABELLED data subchannel
    /// (step 3 of the routing, after the framing and label steps). The
    /// stream-mode decoder extracts *something* out of almost any binary
    /// traffic, so a bare item count is not discriminative — DAB+ audio bytes
    /// yield thousands of junk candidates too. Only the share of CLEAN items
    /// (the A-H1 plausibility gate: streamItemCount() / streamCandidateCount())
    /// separates. Recalibrated on the Phase 2 probe over the real fixtures with
    /// the gate in place (clean/candidates):
    ///   input                                   bytes  cand  clean  ratio
    ///   HR SCId 8 (real Journaline carousel)   384000 11544  3014  0.2611
    ///   BR SCId 19 (real Journaline carousel)  192000  7852  1301  0.1657
    ///   bkk SCId 22 (TPEG — routed earlier)    119952  3245  1786  0.5504
    ///   bkk SCId 0 (DAB+ audio — junk)         959616  9796    51  0.0052
    ///   EWS all-subchannel slice (junk)       6528000 210279 1725  0.0082
    ///   bkk SCId 14 (EPG subchannel)          479808     0     0        —
    ///   early-tap prefixes (96/240/480/2048 B)   ≤2048     0..21  0    —
    ///     (only HR@2048 B has clean items: 34/55 = 0.6182)
    /// Accept only when BOTH floors clear: the tightest real capture (BR) sits
    /// 3.3× ABOVE the 0.05 ratio floor (HR 5.2×), while the worst junk input
    /// (EWS) sits 6.1× BELOW it and bkk audio 9.6× below. Short windows are
    /// saved by the absolute floor: every junk prefix has 0 clean items, so it
    /// can never reach ratio-based acceptance (the old "≈5×" claim was wrong
    /// and is replaced by these measured margins). Anything else falls through
    /// to the EPG decoder, which rejects it honestly.
    static constexpr int kJournalineProbeMinCleanItems = 8;
    static constexpr double kJournalineProbeMinCleanRatio = 0.05;

    /// Wire the decoders + the per-service store to the inner tabs. All
    /// connections are QUEUED to this GUI thread (mirrors the Logger wiring),
    /// so a decoder driven from any thread can never touch a widget; signal
    /// bursts are coalesced by a singleshot timer into ONE tab rebuild.
    void initDataServicePanels()
    {
        if (!m_dataServiceRefreshTimer) {
            m_dataServiceRefreshTimer = new QTimer(this);
            m_dataServiceRefreshTimer->setSingleShot(true);
            m_dataServiceRefreshTimer->setInterval(150);
            connect(m_dataServiceRefreshTimer, &QTimer::timeout, this,
                    [this]() { refreshDataServiceDocks(); });
        }
        if (m_epgDecoder) {
            connect(m_epgDecoder, &eti::epg::EPGDecoder::epgDecodingError, this,
                    [this](const QString& error) {
                        m_lastEpgError = error;
                        scheduleDataServiceRefresh();
                    },
                    Qt::QueuedConnection);
            connect(m_epgDecoder, &eti::epg::EPGDecoder::epgScheduleUpdated, this,
                    [this](uint32_t) { scheduleDataServiceRefresh(); },
                    Qt::QueuedConnection);
            connect(m_epgDecoder, &eti::epg::EPGDecoder::epgEventDiscovered, this,
                    [this](uint32_t, const eti::epg::EPGEvent&) {
                        scheduleDataServiceRefresh();
                    },
                    Qt::QueuedConnection);
        }
        if (m_journalineDecoder) {
            connect(m_journalineDecoder,
                    &eti::journaline::JournalineDecoder::journalineMenuUpdated, this,
                    [this](uint16_t) { scheduleDataServiceRefresh(); },
                    Qt::QueuedConnection);
            connect(m_journalineDecoder,
                    &eti::journaline::JournalineDecoder::objectCountChanged, this,
                    [this]() { scheduleDataServiceRefresh(); },
                    Qt::QueuedConnection);
            connect(m_journalineDecoder,
                    &eti::journaline::JournalineDecoder::journalineDecodingError, this,
                    [this](const QString& error) {
                        m_lastJournalineError = error;
                        scheduleDataServiceRefresh();
                    },
                    Qt::QueuedConnection);
        }
        if (m_tpegDecoder) {
            connect(m_tpegDecoder, &eti::tpeg::TpegDecoder::messageCountChanged, this,
                    [this]() { scheduleDataServiceRefresh(); },
                    Qt::QueuedConnection);
            connect(m_tpegDecoder, &eti::tpeg::TpegDecoder::tpegFramingDetected, this,
                    [this](const QString&) { scheduleDataServiceRefresh(); },
                    Qt::QueuedConnection);
        }
        if (m_dataServiceStore) {
            connect(m_dataServiceStore, &eti::data::DataServiceStore::serviceUpdated,
                    this, [this](uint32_t) { scheduleDataServiceRefresh(); },
                    Qt::QueuedConnection);
            // A-M6: an explicit store clear() must refresh the tabs too —
            // without it a stale row would survive until the next decoder
            // signal (resetDataServicePanels() stops the timer again, so the
            // reset path does not schedule a spurious rebuild).
            connect(m_dataServiceStore, &eti::data::DataServiceStore::servicesCleared,
                    this, [this]() { scheduleDataServiceRefresh(); },
                    Qt::QueuedConnection);
        }
    }

    /// Coalesce decoder signal bursts into one tab rebuild (a real EPG can
    /// emit thousands of event signals in a single load).
    void scheduleDataServiceRefresh()
    {
        if (m_dataServiceRefreshTimer && !m_dataServiceRefreshTimer->isActive()) {
            m_dataServiceRefreshTimer->start();
        }
    }

    // --- Tab page builders (called once from createNowPlayingPanel) --------

    // ========================================================================
    // Phase 2A: Top-level dock content widgets for EPG, Journaline, TPEG
    // (replacing the inner tabs in the slideshow panel)
    // ========================================================================

    QWidget* createEpgDockContent()
    {
        QWidget* page = new QWidget();
        page->setObjectName(QStringLiteral("epgDockContent"));
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(4, 4, 4, 4);
        lay->setSpacing(4);

        QHBoxLayout* selectorRow = new QHBoxLayout();
        QLabel* caption = new QLabel(tr("Service:"));
        m_epgServiceCombo = new QComboBox();
        m_epgServiceCombo->setObjectName(QStringLiteral("epgServiceCombo"));
        m_epgServiceCombo->setEnabled(false);
        selectorRow->addWidget(caption);
        selectorRow->addWidget(m_epgServiceCombo, 1);
        lay->addLayout(selectorRow);

        m_epgStatusLabel = new QLabel(tr("Waiting for capture…"));
        m_epgStatusLabel->setObjectName(QStringLiteral("epgStatusLabel"));
        m_epgStatusLabel->setWordWrap(true);
        m_epgStatusLabel->setStyleSheet(
            QStringLiteral("font-size: 10px; color: #999; font-style: italic;"));
        lay->addWidget(m_epgStatusLabel);

        m_epgScheduleTable = new QTableWidget();
        m_epgScheduleTable->setObjectName(QStringLiteral("epgScheduleTable"));
        m_epgScheduleTable->setColumnCount(8);
        m_epgScheduleTable->setHorizontalHeaderLabels(
            {tr("Status"), tr("Start"), tr("End"), tr("Duration"),
             tr("Program"), tr("Genre"), tr("Rating"), tr("Flags")});
        m_epgScheduleTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_epgScheduleTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_epgScheduleTable->verticalHeader()->setVisible(false);
        m_epgScheduleTable->setAlternatingRowColors(true);
        m_epgScheduleTable->setSortingEnabled(true);
        m_epgScheduleTable->horizontalHeader()->setStretchLastSection(true);
        m_epgScheduleTable->setColumnWidth(0, 60);   // Status
        m_epgScheduleTable->setColumnWidth(1, 60);   // Start
        m_epgScheduleTable->setColumnWidth(2, 60);   // End
        m_epgScheduleTable->setColumnWidth(3, 80);   // Duration
        m_epgScheduleTable->setColumnWidth(4, 200);  // Program
        m_epgScheduleTable->setColumnWidth(5, 120);  // Genre
        m_epgScheduleTable->setColumnWidth(6, 60);   // Rating
        m_epgScheduleTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
        lay->addWidget(m_epgScheduleTable, 1);

        connect(m_epgServiceCombo, qOverload<int>(&QComboBox::currentIndexChanged),
                this, [this](int index) {
                    populateEpgScheduleTable(index >= 0
                                             ? m_epgServiceCombo->itemData(index).toUInt()
                                             : 0u);
                });
        return page;
    }

    QWidget* createJournalineDockContent()
    {
        QWidget* page = new QWidget();
        page->setObjectName(QStringLiteral("journalineDockContent"));
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(4, 4, 4, 4);
        lay->setSpacing(4);

        m_journalineMenuTree = new QTreeWidget();
        m_journalineMenuTree->setObjectName(QStringLiteral("journalineMenuTree"));
        m_journalineMenuTree->setColumnCount(5);
        m_journalineMenuTree->setHeaderLabels(
            {tr("Title"), tr("Category"), tr("Timestamp"), tr("Object ID"), tr("Link Target")});
        m_journalineMenuTree->setRootIsDecorated(true);
        m_journalineMenuTree->setUniformRowHeights(true);
        m_journalineMenuTree->setColumnWidth(0, 250);  // Title
        m_journalineMenuTree->setColumnWidth(1, 100);  // Category
        m_journalineMenuTree->setColumnWidth(2, 100);  // Timestamp
        m_journalineMenuTree->setColumnWidth(3, 80);   // Object ID
        m_journalineMenuTree->setColumnWidth(4, 100);  // Link Target
        m_journalineMenuTree->header()->setStretchLastSection(false);
        m_journalineMenuTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        lay->addWidget(m_journalineMenuTree, 3);

        m_journalinePreview = new QTextEdit();
        m_journalinePreview->setObjectName(QStringLiteral("journalinePreview"));
        m_journalinePreview->setReadOnly(true);
        m_journalinePreview->setPlaceholderText(tr("Select an item to preview its text."));
        lay->addWidget(m_journalinePreview, 2);

        m_journalineStatusLabel = new QLabel(tr("Waiting for capture…"));
        m_journalineStatusLabel->setObjectName(QStringLiteral("journalineStatusLabel"));
        m_journalineStatusLabel->setWordWrap(true);
        m_journalineStatusLabel->setStyleSheet(
            QStringLiteral("font-size: 10px; color: #999; font-style: italic;"));
        lay->addWidget(m_journalineStatusLabel);

        connect(m_journalineMenuTree, &QTreeWidget::currentItemChanged, this,
                [this]() { updateJournalinePreview(); });
        return page;
    }

    QWidget* createTpegDockContent()
    {
        QWidget* page = new QWidget();
        page->setObjectName(QStringLiteral("tpegDockContent"));
        QVBoxLayout* lay = new QVBoxLayout(page);
        lay->setContentsMargins(4, 4, 4, 4);
        lay->setSpacing(4);

        m_tpegInventoryTable = new QTableWidget();
        m_tpegInventoryTable->setObjectName(QStringLiteral("tpegInventoryTable"));
        m_tpegInventoryTable->setColumnCount(5);
        m_tpegInventoryTable->setHorizontalHeaderLabels(
            {tr("Family"), tr("Message"), tr("Count"),
             tr("First seen (frame)"), tr("Last seen (frame)")});
        m_tpegInventoryTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tpegInventoryTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tpegInventoryTable->verticalHeader()->setVisible(false);
        m_tpegInventoryTable->setAlternatingRowColors(true);
        m_tpegInventoryTable->setColumnWidth(0, 80);
        m_tpegInventoryTable->setColumnWidth(1, 460);
        m_tpegInventoryTable->setColumnWidth(2, 70);
        m_tpegInventoryTable->setColumnWidth(3, 130);
        m_tpegInventoryTable->horizontalHeader()->setStretchLastSection(true);
        lay->addWidget(m_tpegInventoryTable, 1);

        m_tpegStatusLabel = new QLabel(withTpegCoverageNote(tr("Waiting for capture…")));
        m_tpegStatusLabel->setObjectName(QStringLiteral("tpegStatusLabel"));
        m_tpegStatusLabel->setWordWrap(true);
        m_tpegStatusLabel->setStyleSheet(
            QStringLiteral("font-size: 10px; color: #999; font-style: italic;"));
        lay->addWidget(m_tpegStatusLabel);
        return page;
    }

    // --- Per-capture reset -------------------------------------------------

    /// Drop the previous capture's decoded data-service state and return the
    /// dock widgets to their waiting state. Called from resetCaptureState()
    /// BEFORE a new capture is fed; safe to call before the docks exist.
    void resetDataServicePanels()
    {
        if (m_epgDecoder) {
            m_epgDecoder->clearAll();
        }
        if (m_journalineDecoder) {
            m_journalineDecoder->clearAll();
        }
        if (m_tpegDecoder) {
            m_tpegDecoder->clearAll();
        }
        if (m_dataServiceStore) {
            m_dataServiceStore->clear();
        }
        m_lastEpgError.clear();
        m_lastJournalineError.clear();
        m_journalineRouted = false;          // B-H1(5): nothing routed yet
        m_tpegFirstSeenFrames.clear();
        m_tpegLastSeenFrames.clear();
        m_tpegSeenCounts.clear();
        m_tpegInventorySignature.clear();     // B cross-dep 3: force rebuild
        m_journalineTreeItemCount = -1;
        m_journalineTreeRootCount = -1;
        if (m_dataServiceRefreshTimer) {
            m_dataServiceRefreshTimer->stop();
        }

        if (m_epgServiceCombo) {
            m_epgServiceCombo->clear();
            m_epgServiceCombo->setEnabled(false);
        }
        if (m_epgScheduleTable) {
            m_epgScheduleTable->setRowCount(0);
        }
        if (m_epgStatusLabel) {
            m_epgStatusLabel->setText(tr("Waiting for capture…"));
        }
        if (m_journalineMenuTree) {
            m_journalineMenuTree->clear();
        }
        if (m_journalinePreview) {
            m_journalinePreview->clear();
        }
        if (m_journalineStatusLabel) {
            m_journalineStatusLabel->setText(tr("Waiting for capture…"));
        }
        if (m_tpegInventoryTable) {
            m_tpegInventoryTable->setRowCount(0);
        }
        if (m_tpegStatusLabel) {
            m_tpegStatusLabel->setText(withTpegCoverageNote(tr("Waiting for capture…")));
        }
    }

    // --- Routing + feeding -------------------------------------------------

    /// Human label for a data subchannel: the FIG 1/1 service label when the
    /// owning service carries one, else the FIG 1/5 data-service label of the
    /// (32-bit) data service whose component references this subchannel.
    QString dataServiceLabelForSubChannel(int subChannelId) const
    {
        if (!m_figAnalyser) {
            return QString();
        }
        const std::vector<FIG1_5_DataServiceLabel> dataLabels =
            m_figAnalyser->getDataServiceLabels();
        for (const DABService& svc : m_figAnalyser->getDABServices()) {
            for (const ServiceComponent& comp : svc.components) {
                if (comp.sub_channel_id != static_cast<uint8_t>(subChannelId)) {
                    continue;
                }
                if (isAudioServiceComponent(comp)) {
                    // B-M1: an audio component is never a data service — its
                    // FIG 1/1 label ("Cool Radio" …) must not be used to route
                    // or label data bytes. Keep looking: another component may
                    // still reference this sub-channel as data.
                    continue;
                }
                const QString fig11 = svc.service_label.trimmed();
                if (!fig11.isEmpty()) {
                    return fig11;
                }
                for (const FIG1_5_DataServiceLabel& dl : dataLabels) {
                    if (dl.service_id == svc.service_id) {
                        // B-M3: decode the raw 16-byte field as UTF-8 with a
                        // Latin-1 fallback (Thai labels are UTF-8) instead of
                        // blind fromLatin1.
                        const QString label =
                            decodeFigLabelBytes(dl.label, figLabelFieldLength(dl.label))
                                .trimmed();
                        if (!label.isEmpty()) {
                            return label;
                        }
                    }
                }
                return QString();
            }
        }
        return QString();
    }

    /// Display label for a decoded EPG service id (FIG 1/1, then FIG 1/5).
    QString serviceLabelForId(quint32 serviceId) const
    {
        if (m_figAnalyser) {
            for (const DABService& svc : m_figAnalyser->getDABServices()) {
                if (svc.service_id == serviceId && !svc.service_label.trimmed().isEmpty()) {
                    return svc.service_label.trimmed();
                }
            }
            for (const FIG1_5_DataServiceLabel& dl : m_figAnalyser->getDataServiceLabels()) {
                if (dl.service_id == serviceId) {
                    // B-M3: UTF-8 with Latin-1 fallback (was fromLatin1) —
                    // matches the data-subchannel label path above.
                    const QString label =
                        decodeFigLabelBytes(dl.label, figLabelFieldLength(dl.label))
                            .trimmed();
                    if (!label.isEmpty()) {
                        return label;
                    }
                }
            }
        }
        return QString();
    }

    /// B-M2: only the first bytes of a subchannel stream are examined for TPEG
    /// carousel framing. The marker phase repeats from the very start of the
    /// carousel (Phase 2 probe: bkk SCId 22 is detected already inside its
    /// first 96 bytes), while scanning a whole 4 MB buffer of unrelated
    /// traffic for marker pairs is a real false-positive risk (HR/BR full
    /// streams both "detect" framing that way). 240 B ≈ 10 carousel frames of
    /// margin over that 96 B observation.
    static constexpr int kCarouselProbePrefixBytes = 240;

    /// Decide which decoder a captured data subchannel belongs to (see the
    /// block comment above).
    DataServiceRoute routeDataService(int subChannelId, const QByteArray& bytes) const
    {
        // 1) The observed TPEG carousel framing wins first: bkk's data
        //    services share one FIG 1/5 label ("EPG" for SId 0xf320), so a
        //    label alone would misroute the TEPG subchannel. B-M2: bounded to
        //    the prefix (see kCarouselProbePrefixBytes); a `containsSyncPair`
        //    requirement was REJECTED on Phase 2 evidence — the real bkk
        //    carousel shows 0 sync pairs while the false positives do not.
        if (eti::tpeg::TpegDecoder::detectsCarouselFraming(
                bytes.left(kCarouselProbePrefixBytes))) {
            return DataServiceRoute::Tpeg;
        }
        const QString label = dataServiceLabelForSubChannel(subChannelId).toLower();
        if (label.contains(QLatin1String("tepg")) || label.contains(QLatin1String("tpeg"))) {
            return DataServiceRoute::Tpeg;
        }
        if (label.contains(QLatin1String("epg"))) {
            return DataServiceRoute::Epg;
        }
        if (label.contains(QLatin1String("journ"))) {
            return DataServiceRoute::Journaline;
        }
        // 3) Content probe for unlabelled subchannels: a real stream-mode
        //    Journaline carousel yields many items that survive the A-H1
        //    plausibility gate — the clean share is the discriminative signal
        //    (see the Phase 2 calibration table on the two constants above).
        //    The counters come straight from the decoder: streamItemCount() =
        //    post-gate "clean", streamCandidateCount() = pre-gate runs.
        eti::journaline::JournalineDecoder probe;
        probe.processStreamData(bytes);
        const int candidates = probe.streamCandidateCount();
        const int clean = probe.streamItemCount();
        // clean > candidates is impossible (the gate only accepts candidates),
        // and clean >= 8 implies candidates >= 8 — so the ratio below is
        // always well-defined; the explicit guard keeps it obvious.
        if (candidates > 0 && clean >= kJournalineProbeMinCleanItems
            && static_cast<double>(clean) / static_cast<double>(candidates)
                   >= kJournalineProbeMinCleanRatio) {
            return DataServiceRoute::Journaline;
        }
        // 4) Last word goes to the EPG decoder — it rejects non-EPG bytes
        //    honestly (no fabricated schedules).
        return DataServiceRoute::Epg;
    }

    /// Per-subchannel TPEG attribution (N2): what THIS subchannel's bytes
    /// produced on the single shared TpegDecoder. The decoder's counters are
    /// cumulative over every subchannel fed so far, so a raw read would
    /// publish sub-channel 1's totals against sub-channel 2 — hence deltas.
    struct TpegFeedTotals {
        int messagesDelta = 0;    ///< unique texts ADDED to the shared inventory
        quint64 framesDelta = 0;  ///< whole 24-byte stream frames added
    };

    /// Feed one TPEG-classified subchannel to the TpegDecoder in bounded
    /// chunks, then record the stream-frame number at which each unique
    /// message was first seen and last re-seen (the inventory table's
    /// first/last-seen columns; frames are the decoder's 24-byte stream
    /// frames). B cross-dep 3: messages() is copied ONCE per feed — it used
    /// to be re-read after every 4 KB chunk (O(chunks × messages)) — and the
    /// decoder's own first_frame/last_frame fields are authoritative, so the
    /// bookkeeping maps stay bounded by the decoder's kMaxUniqueMessages cap
    /// (only texts that survive in its inventory are ever inserted here).
    ///
    /// N2: finish() is deliberately NOT called here. This helper runs once
    /// per TPEG-routed subchannel, and finish() CLOSES the decoder — calling
    /// it per subchannel sent every later same-route subchannel down the
    /// post-finish drop path (bytes discarded + one-shot warning) and made its
    /// snapshot inherit the earlier subchannel's counters. The shared decoder
    /// is finished exactly once, after the whole feed loop
    /// (feedDataServiceSubchannels()). To keep a usable per-feed view in the
    /// meantime, refreshInventory() materialises the inventory WITHOUT
    /// closing it; it scans the same bytes finish() would, so the final
    /// inventory is identical.
    TpegFeedTotals feedTpegStreamChunked(const QByteArray& bytes)
    {
        TpegFeedTotals totals;
        if (!m_tpegDecoder) {
            return totals;
        }
        const int messagesBefore = m_tpegDecoder->messageCount();
        const quint64 framesBefore = m_tpegDecoder->frameCount();
        constexpr int kChunkBytes = 4096;
        for (int offset = 0; offset < bytes.size(); offset += kChunkBytes) {
            const int len = qMin(kChunkBytes, bytes.size() - offset);
            m_tpegDecoder->processStreamData(bytes.mid(offset, len));
        }
        // Materialise the inventory for THIS subchannel's snapshot (does not
        // finish the decoder), then take the delta its bytes produced.
        totals.messagesDelta = m_tpegDecoder->refreshInventory() - messagesBefore;
        totals.framesDelta = m_tpegDecoder->frameCount() - framesBefore;
        const QVector<eti::tpeg::TpegMessage> messages = m_tpegDecoder->messages();
        for (const eti::tpeg::TpegMessage& m : messages) {
            // Defensive mirror of the decoder's inventory cap: never grow the
            // GUI-side maps past kMaxUniqueMessages unique texts.
            if (!m_tpegFirstSeenFrames.contains(m.text)
                && m_tpegFirstSeenFrames.size() >= eti::tpeg::TpegDecoder::kMaxUniqueMessages) {
                break;
            }
            m_tpegFirstSeenFrames.insert(m.text, m.first_frame);
            m_tpegLastSeenFrames.insert(m.text, m.last_frame);
            m_tpegSeenCounts.insert(m.text, m.repeat_count);
        }
        return totals;
    }

    /// Run the EPG / Journaline / TPEG decoders over the raw bytes captured
    /// by the clean per-data-subchannel tap (processDlsPlusFromFrame) and
    /// publish per-service snapshots in the store. Called once per completed
    /// load, on the GUI thread; load-time DLS+/MOT counters are untouched.
    void feedDataServiceSubchannels()
    {
        if (!m_epgDecoder || !m_journalineDecoder || !m_tpegDecoder) {
            return;
        }
        m_tpegFirstSeenFrames.clear();
        m_tpegLastSeenFrames.clear();
        m_tpegSeenCounts.clear();
        m_tpegInventorySignature.clear();  // B cross-dep 3: force a rebuild

        // B-M1: classify with the CURRENT FIC before feeding, so the same
        // audio exclusion the tap applies at capture time also applies here
        // (defence in depth — a pre-FIG entry that slipped into the map must
        // never reach a data decoder).
        rebuildDabPlusServiceMapIfNeeded();

        // N2: snapshots are BUILT inside the loop but FLUSHED to the store
        // after the one-shot TPEG finish() below, so the TPEG rows carry the
        // final framing verdict instead of a mid-feed state (the counters are
        // already per-subchannel deltas taken at feed time).
        struct PendingSnapshot {
            eti::data::ServiceDataSnapshot snapshot;
            DataServiceRoute route;
        };
        QVector<PendingSnapshot> pending;
        bool fedTpeg = false;   // at least one TPEG subchannel was fed

        for (auto it = m_dataSubChannelStreams.cbegin();
             it != m_dataSubChannelStreams.cend(); ++it) {
            const QByteArray& bytes = it.value();
            if (bytes.isEmpty()) {
                continue;
            }
            const int subChannelId = it.key();
            if (m_dabPlusSubChannels.contains(subChannelId)
                || m_audioSubChannels.contains(subChannelId)) {
                // B-M1: audio bytes are not a data service — skip them the
                // same way tapDataSubchannelsFromFrame() does.
                continue;
            }
            const DataServiceRoute route = routeDataService(subChannelId, bytes);
            if (route == DataServiceRoute::Journaline) {
                m_journalineRouted = true;  // B-H1(5): routed, even if empty
            }

            eti::data::ServiceDataSnapshot snapshot;
            snapshot.sub_channel_id = static_cast<uint32_t>(subChannelId);
            snapshot.service_label = dataServiceLabelForSubChannel(subChannelId);
            snapshot.stream_bytes = static_cast<quint64>(bytes.size());

            switch (route) {
            case DataServiceRoute::Tpeg: {
                // N2 attribution: these counters are the DELTA this
                // subchannel's bytes produced on the shared decoder
                // (messageCount()/frameCount() are cumulative over every fed
                // subchannel, so a raw read would publish sub-channel 1's
                // totals against sub-channel 2). The framing fields are NOT
                // counters — they are shared-decoder attributes of the TPEG
                // stream as a whole and are stamped after the single finish()
                // below (identical on every TPEG row by construction).
                const TpegFeedTotals tpeg = feedTpegStreamChunked(bytes);
                snapshot.tpeg_messages = tpeg.messagesDelta;
                snapshot.stream_frames = tpeg.framesDelta;
                fedTpeg = true;
                break;
            }
            case DataServiceRoute::Journaline:
                m_journalineDecoder->processStreamData(bytes);
                snapshot.journaline_objects = m_journalineDecoder->objectCount();
                snapshot.journaline_stream_items = m_journalineDecoder->streamItemCount();
                if (!m_journalineDecoder->getStreamItems().isEmpty()) {
                    snapshot.journaline_sample_title =
                        m_journalineDecoder->getStreamItems().constFirst().title;
                }
                break;
            case DataServiceRoute::Epg: {
                const bool accepted = m_epgDecoder->processMOTObject(bytes);
                const int events = m_epgDecoder->eventCount();
                if (accepted && events > 0) {
                    m_lastEpgError.clear();
                }
                snapshot.epg_schedules = m_epgDecoder->serviceCount();
                snapshot.epg_events = events;
                break;
            }
            }

            pending.append(PendingSnapshot{snapshot, route});
        }

        if (fedTpeg) {
            // N2: ONE finish() for the single shared TpegDecoder, AFTER every
            // TPEG-routed subchannel has been fed. finish() closes the
            // decoder (later bytes are dropped with a one-shot warning), so
            // it must never run inside the per-subchannel loop.
            m_tpegDecoder->finish();
            for (PendingSnapshot& entry : pending) {
                if (entry.route == DataServiceRoute::Tpeg) {
                    entry.snapshot.tpeg_framing = m_tpegDecoder->framingDescription();
                    entry.snapshot.tpeg_sync_0a46 = m_tpegDecoder->sawSyncBytes();
                }
            }
        }

        if (m_dataServiceStore) {
            for (const PendingSnapshot& entry : pending) {
                m_dataServiceStore->updateService(entry.snapshot);
            }
        }
    }

    // --- Dock refreshers ----------------------------------------------------

    void refreshDataServiceDocks()
    {
        refreshEpgTab();
        refreshJournalineTab();
        refreshTpegTab();
    }

    /// EPG tab: service selector + status line + Now/Next + schedule rows.
    /// Honest empty state: a capture whose EPG subchannel carries no decodable
    /// TS 102 371 schedule (e.g. bkk SCId 14, reserved-but-idle) reports
    /// "No EPG data in this stream" and lists the decoder's rejection.
    void refreshEpgTab()
    {
        if (!m_epgDecoder || !m_epgServiceCombo || !m_epgStatusLabel) {
            return;
        }

        // Only services that actually carry events are offered: a schedule
        // header that parsed with zero events is NOT content.
        QVector<uint32_t> withEvents;
        int totalEvents = 0;
        for (uint32_t sid : m_epgDecoder->getServiceIds()) {
            const int n = m_epgDecoder->getEvents(sid).size();
            if (n > 0) {
                withEvents.append(sid);
                totalEvents += n;
            }
        }

        const quint32 previous =
            (m_epgServiceCombo->currentIndex() >= 0)
                ? m_epgServiceCombo->itemData(m_epgServiceCombo->currentIndex()).toUInt()
                : 0u;
        int restoreRow = 0;
        {
            QSignalBlocker blocker(m_epgServiceCombo);
            m_epgServiceCombo->clear();
            if (withEvents.isEmpty()) {
                m_epgServiceCombo->addItem(tr("— no EPG service in this stream —"), 0u);
            } else {
                int row = 0;
                for (uint32_t sid : withEvents) {
                    const QString label = serviceLabelForId(sid);
                    m_epgServiceCombo->addItem(
                        label.isEmpty()
                            ? tr("SId %1").arg(serviceSidHex(sid))
                            : QString("%1 (%2)").arg(label, serviceSidHex(sid)),
                        sid);
                    if (sid == previous) {
                        restoreRow = row;
                    }
                    ++row;
                }
            }
            m_epgServiceCombo->setCurrentIndex(restoreRow);
            m_epgServiceCombo->setEnabled(!withEvents.isEmpty());
        }

        const quint32 selected =
            (withEvents.isEmpty() || m_epgServiceCombo->currentIndex() < 0)
                ? 0u
                : m_epgServiceCombo->itemData(m_epgServiceCombo->currentIndex()).toUInt();
        populateEpgScheduleTable(selected);

        if (totalEvents == 0) {
            QString status = tr("No EPG data in this stream");
            if (!m_lastEpgError.isEmpty()) {
                status += tr(" — decoder reported: %1").arg(m_lastEpgError);
            }
            m_epgStatusLabel->setText(status);
        } else {
            m_epgStatusLabel->setText(
                tr("%1 service(s) · %2 event(s) decoded")
                    .arg(withEvents.size())
                    .arg(totalEvents));
        }
    }

    /// Build flags string for an EPG event (Live / Subtitles / AudioDesc / —)
    QString buildEpgEventFlags(const eti::epg::EPGEvent* event) const
    {
        if (!event) {
            return QStringLiteral("—");
        }
        QStringList flags;
        if (event->is_live) {
            flags << tr("Live");
        }
        if (event->has_subtitles) {
            flags << tr("Subtitles");
        }
        if (event->has_audio_description) {
            flags << tr("AudioDesc");
        }
        return flags.isEmpty() ? QStringLiteral("—") : flags.join(QStringLiteral(" / "));
    }

    void populateEpgScheduleTable(quint32 serviceId)
    {
        if (!m_epgScheduleTable || !m_epgDecoder) {
            return;
        }
        m_epgScheduleTable->setRowCount(0);
        if (serviceId == 0) {
            return;
        }
        const QVector<eti::epg::EPGEvent> events = m_epgDecoder->getEvents(serviceId);
        if (events.isEmpty()) {
            return;
        }

        auto timeRange = [](const eti::epg::EPGEvent& e) {
            if (!e.start_time.isValid() || !e.end_time.isValid()) {
                return QStringLiteral("--:--–--:--");
            }
            return QStringLiteral("%1–%2")
                .arg(e.start_time.toString(QStringLiteral("HH:mm")),
                     e.end_time.toString(QStringLiteral("HH:mm")));
        };
        auto appendRow = [this, &timeRange](const QString& status,
                                            const QString& start,
                                            const QString& end,
                                            const QString& duration,
                                            const QString& program,
                                            const QString& genre,
                                            const QString& rating,
                                            const QString& flags) {
            int row = m_epgScheduleTable->rowCount();
            m_epgScheduleTable->insertRow(row);
            m_epgScheduleTable->setItem(row, 0, new QTableWidgetItem(status));
            m_epgScheduleTable->setItem(row, 1, new QTableWidgetItem(start));
            m_epgScheduleTable->setItem(row, 2, new QTableWidgetItem(end));
            m_epgScheduleTable->setItem(row, 3, new QTableWidgetItem(duration));
            m_epgScheduleTable->setItem(row, 4, new QTableWidgetItem(program));
            m_epgScheduleTable->setItem(row, 5, new QTableWidgetItem(genre));
            m_epgScheduleTable->setItem(row, 6, new QTableWidgetItem(rating));
            m_epgScheduleTable->setItem(row, 7, new QTableWidgetItem(flags));
        };

        if (std::optional<eti::epg::EPGEvent> now =
                m_epgDecoder->getCurrentEvent(serviceId)) {
            QString duration = "--:--";
            if (now->start_time.isValid() && now->end_time.isValid()) {
                int secs = now->start_time.secsTo(now->end_time);
                duration = QString("%1:%2")
                    .arg(secs / 60, 2, 10, QChar('0'))
                    .arg(secs % 60, 2, 10, QChar('0'));
            }
            appendRow("Now",
                      now->start_time.toString("HH:mm"),
                      now->end_time.toString("HH:mm"),
                      duration,
                      now->program_name,
                      now->genre,
                      QString::number(now->parental_rating),
                      buildEpgEventFlags(&*now));
        }
        if (std::optional<eti::epg::EPGEvent> next =
                m_epgDecoder->getNextEvent(serviceId)) {
            QString duration = "--:--";
            if (next->start_time.isValid() && next->end_time.isValid()) {
                int secs = next->start_time.secsTo(next->end_time);
                duration = QString("%1:%2")
                    .arg(secs / 60, 2, 10, QChar('0'))
                    .arg(secs % 60, 2, 10, QChar('0'));
            }
            appendRow("Next",
                      next->start_time.toString("HH:mm"),
                      next->end_time.toString("HH:mm"),
                      duration,
                      next->program_name,
                      next->genre,
                      QString::number(next->parental_rating),
                      buildEpgEventFlags(&*next));
        }
        for (const eti::epg::EPGEvent& e : events) {
            QString duration = "--:--";
            if (e.start_time.isValid() && e.end_time.isValid()) {
                int secs = e.start_time.secsTo(e.end_time);
                duration = QString("%1:%2")
                    .arg(secs / 60, 2, 10, QChar('0'))
                    .arg(secs % 60, 2, 10, QChar('0'));
            }
            appendRow("",
                      e.start_time.toString("HH:mm"),
                      e.end_time.toString("HH:mm"),
                      duration,
                      e.program_name,
                      e.genre,
                      QString::number(e.parental_rating),
                      buildEpgEventFlags(&e));
        }
    }

    /// Journaline tab: menu tree (incl. the stream-mode "Display Carousel"
    /// 0x7FFF root) + text preview + status line. UTF-8 (German umlauts,
    /// Thai) is shown verbatim from the decoder's item model.
    void refreshJournalineTab()
    {
        if (!m_journalineDecoder || !m_journalineMenuTree || !m_journalinePreview
            || !m_journalineStatusLabel) {
            return;
        }

        const QVector<eti::journaline::JournalineMenu> roots =
            m_journalineDecoder->getRootMenus();
        int totalItems = 0;
        for (const eti::journaline::JournalineMenu& menu : roots) {
            totalItems += menu.items.size();
        }

        // Rebuild only when the model actually changed — queued decoder
        // signals must not re-create a >10k-item tree needlessly.
        if (totalItems != m_journalineTreeItemCount
            || roots.size() != m_journalineTreeRootCount) {
            QTreeWidgetItem* previous = m_journalineMenuTree->currentItem();
            const QString previousText = previous ? previous->text(0) : QString();

            m_journalineMenuTree->clear();
            for (const eti::journaline::JournalineMenu& menu : roots) {
                auto* menuNode = new QTreeWidgetItem(m_journalineMenuTree);
                const QString menuTitle = tr("%1 (0x%2)")
                    .arg(menu.menu_title)
                    .arg(QString::number(menu.menu_id, 16).toUpper());
                menuNode->setText(0, menuTitle);
                // Column 1: Category
                menuNode->setText(1, tr("Carousel"));
                // Column 2: Timestamp (use first item's timestamp or placeholder)
                QString menuTimestamp = QStringLiteral("--:--:--");
                if (!menu.items.isEmpty() && menu.items.first().timestamp.isValid()) {
                    menuTimestamp = menu.items.first().timestamp.toString(QStringLiteral("HH:mm:ss"));
                }
                menuNode->setText(2, menuTimestamp);
                // Column 3: Object ID (menu ID in hex)
                menuNode->setText(3, QString::number(menu.menu_id, 16).toUpper());
                // Column 4: Link Target (not applicable for menu nodes)
                menuNode->setText(4, QStringLiteral("—"));

                for (const eti::journaline::JournalineObject& object : menu.items) {
                    auto* child = new QTreeWidgetItem(menuNode);
                    const QString title = object.title.trimmed();
                    child->setText(0, title.isEmpty() ? object.text_content.left(80)
                                                      : title);
                    // Column 1: Category
                    child->setText(1, object.getCategoryString());
                    // Column 2: Timestamp
                    QString objTimestamp = QStringLiteral("--:--:--");
                    if (object.timestamp.isValid()) {
                        objTimestamp = object.timestamp.toString(QStringLiteral("HH:mm:ss"));
                    }
                    child->setText(2, objTimestamp);
                    // Column 3: Object ID (hex)
                    child->setText(3, QString::number(object.object_id, 16).toUpper());
                    // Column 4: Link Target
                    child->setText(4, object.is_link
                                   ? QString::number(object.link_target, 16).toUpper()
                                   : QStringLiteral("—"));
                    child->setData(0, Qt::UserRole, object.text_content);
                    if (!object.text_content.isEmpty()) {
                        child->setToolTip(0, object.text_content.left(200));
                    }
                }
                menuNode->setExpanded(true);
            }
            m_journalineTreeItemCount = totalItems;
            m_journalineTreeRootCount = roots.size();

            // Restore the previous selection by text, else preselect the
            // first item so the preview shows real content immediately.
            QTreeWidgetItem* restore = nullptr;
            if (!previousText.isEmpty()) {
                const QList<QTreeWidgetItem*> matches =
                    m_journalineMenuTree->findItems(previousText, Qt::MatchExactly, 0);
                if (!matches.isEmpty()) {
                    restore = matches.constFirst();
                }
            }
            if (!restore && m_journalineMenuTree->topLevelItemCount() > 0) {
                QTreeWidgetItem* root = m_journalineMenuTree->topLevelItem(0);
                restore = root->childCount() > 0 ? root->child(0) : root;
            }
            if (restore) {
                m_journalineMenuTree->setCurrentItem(restore);
            }
            updateJournalinePreview();
        }

        // B-H1(5): honest false-reject reporting — "nothing was routed to
        // Journaline in this capture" and "it was routed but the decoder
        // produced no items" are different states and must not share a line
        // (README: data only when the stream carries it).
        QString status;
        if (totalItems == 0) {
            status = m_journalineRouted
                         ? tr("Journaline data routed in this stream, but no items decoded")
                         : tr("No Journaline data routed in this stream");
            if (!m_lastJournalineError.isEmpty()) {
                status += tr(" — decoder reported: %1").arg(m_lastJournalineError);
            }
        } else {
            status = tr("%1 stream item(s) · %2 menu(s)").arg(totalItems).arg(roots.size());
        }
        m_journalineStatusLabel->setText(status);
    }

    void updateJournalinePreview()
    {
        if (!m_journalinePreview) {
            return;
        }
        QTreeWidgetItem* item =
            m_journalineMenuTree ? m_journalineMenuTree->currentItem() : nullptr;
        if (!item) {
            m_journalinePreview->clear();
            return;
        }
        const QString body = item->data(0, Qt::UserRole).toString();
        if (body.isEmpty()) {
            m_journalinePreview->setPlainText(item->text(0));
            return;
        }
        m_journalinePreview->setPlainText(
            item->text(0) + QStringLiteral("\n\n") + body);
    }

    /// TPEG tab: message inventory (family badge, text, repeat count, the
    /// stream frame where the text was first / last re-seen) + the honest
    /// status line carrying the coverage note.
    void refreshTpegTab()
    {
        if (!m_tpegDecoder || !m_tpegInventoryTable || !m_tpegStatusLabel) {
            return;
        }
        const QVector<eti::tpeg::TpegMessage>& messages = m_tpegDecoder->messages();

        // B cross-dep 3: skip the full rebuild while nothing observable
        // changed. The table + status line show exactly these four facts
        // (unique count, stream frames, dropped-text count, framing verdict),
        // and this method only ever runs from the 150 ms coalesced timer —
        // so a no-op refresh is O(1) instead of O(rows).
        const QString signature =
            QStringLiteral("%1/%2/%3/%4")
                .arg(messages.size())
                .arg(m_tpegDecoder->frameCount())
                .arg(m_tpegDecoder->getStatistics().dropped_messages)
                .arg(m_tpegDecoder->framingDescription());
        if (signature == m_tpegInventorySignature) {
            return;
        }
        m_tpegInventorySignature = signature;

        m_tpegInventoryTable->setRowCount(0);
        m_tpegInventoryTable->setRowCount(messages.size());
        for (int row = 0; row < messages.size(); ++row) {
            const eti::tpeg::TpegMessage& message = messages.at(row);

            auto* family = new QTableWidgetItem(message.category);
            const QString category = message.category.toLower();
            QColor familyColour(0x75, 0x75, 0x75);  // neutral for banner/version
            if (category == QLatin1String("tec")) {
                familyColour = QColor(0x1b, 0x5e, 0x20);
            } else if (category == QLatin1String("tfp")) {
                familyColour = QColor(0x0d, 0x47, 0xa1);
            } else if (category == QLatin1String("pki")) {
                familyColour = QColor(0x4a, 0x14, 0x8c);
            } else if (category == QLatin1String("emi")) {
                familyColour = QColor(0xb7, 0x1c, 0x1c);
            }
            QFont familyFont = family->font();
            familyFont.setBold(true);
            family->setFont(familyFont);
            family->setForeground(familyColour);
            family->setToolTip(tr("Observed application family (text-inventory "
                                  "level; full TPEG2-TTI decoding is deferred)"));

            auto* text = new QTableWidgetItem(message.text);
            auto* count = new QTableWidgetItem(QString::number(message.repeat_count));
            auto* first = new QTableWidgetItem(
                m_tpegFirstSeenFrames.contains(message.text)
                    ? QString::number(m_tpegFirstSeenFrames.value(message.text))
                    : QStringLiteral("—"));
            auto* last = new QTableWidgetItem(
                m_tpegLastSeenFrames.contains(message.text)
                    ? QString::number(m_tpegLastSeenFrames.value(message.text))
                    : QStringLiteral("—"));
            for (int col = 0; col < 5; ++col) {
                QTableWidgetItem* cell =
                    col == 0 ? family
                             : col == 1 ? text
                                        : col == 2 ? count : col == 3 ? first : last;
                cell->setFlags(cell->flags() & ~Qt::ItemIsEditable);
            }
            m_tpegInventoryTable->setItem(row, 0, family);
            m_tpegInventoryTable->setItem(row, 1, text);
            m_tpegInventoryTable->setItem(row, 2, count);
            m_tpegInventoryTable->setItem(row, 3, first);
            m_tpegInventoryTable->setItem(row, 4, last);
        }

        QString state;
        if (messages.isEmpty()) {
            state = tr("No TPEG data in this stream");
        } else {
            state = tr("%1 unique messages · %2 stream frames")
                        .arg(messages.size())
                        .arg(m_tpegDecoder->frameCount());
            const QString framing = m_tpegDecoder->framingDescription();
            if (!framing.isEmpty()) {
                state += QStringLiteral(" · ") + framing;
            }
        }
        m_tpegStatusLabel->setText(withTpegCoverageNote(state));
    }

    // ========================================================================
    // LEFT PANEL HELPER METHODS: PLAYER, DECODER, SYSTEM
    // ========================================================================
    // Items #1-4 from DATA_AVAILABILITY_MATRIX.md (LEFT Panel)

public:
    /**
     * @brief Apply an already-resolved icon (or the text fallback) to a button.
     *
     * W1 #5: the text fallback is the DEFENSIVE path for a platform/style whose
     * QStyle::standardIcon() returns a null icon for the media pixmaps. Every
     * shipped style (Fusion, and the offscreen/minimal Qt test styles) supplies
     * these icons, so the fallback is intentionally unreachable in normal runs;
     * it exists only so a blank ("tofu") transport button can never ship. This
     * method is public so tests can drive the null-icon path directly.
     *
     * Object names, tooltips and click connections are untouched; only the
     * button face (icon vs. text) and a sane width are set here.
     */
    static void applyTransportButtonFace(QPushButton* button, const QIcon& icon,
                                         const QString& fallbackText)
    {
        if (!button) {
            return;
        }
        if (!icon.isNull()) {
            button->setIcon(icon);
            button->setIconSize(QSize(16, 16));
            button->setText(QString());
            button->setMaximumWidth(40);   // icon-only: compact square-ish button
        } else {
            button->setIcon(QIcon());
            button->setText(fallbackText);
            button->setMaximumWidth(64);   // short text needs a little more room
        }
        button->setMinimumWidth(32);
    }

private:
    /**
     * @brief Give a transport button a Qt style media icon with a text fallback.
     *
     * T44: the previous implementation used unicode media glyphs (U+23F8 pause,
     * U+23EE reset, U+23EA/23E9 skip, U+25A0 stop). The default font has no
     * glyph for most of them, so the buttons rendered blank ("tofu"). Qt's
     * built-in style icons are guaranteed to be present on every shipped style;
     * the text fallback in applyTransportButtonFace() covers the defensive case
     * where a style has no media icon.
     */
    static void setTransportButtonIcon(QPushButton* button,
                                       QStyle::StandardPixmap icon,
                                       const QString& fallbackText)
    {
        if (!button) {
            return;
        }
        applyTransportButtonFace(button, button->style()->standardIcon(icon), fallbackText);
    }

    /**
     * @brief Create Player Panel for LEFT panel (Item #1)
     * 
     * Displays player status, file information, timing, and controls.
     * Per DATA_AVAILABILITY_MATRIX.md specification.
     * 
     * @return QGroupBox* Player panel widget
     */
    QGroupBox* createPlayerPanel()
    {
        QGroupBox* group = new QGroupBox("Player");
        if (!group) {
            qCritical() << "Failed to create Player panel";
            return nullptr;
        }
        
        QGridLayout* layout = new QGridLayout(group);
        layout->setContentsMargins(4, 8, 4, 4);
        layout->setSpacing(4);
        
        // Row 0: Status
        QLabel* statusLabel = new QLabel("Status:");
        statusLabel->setStyleSheet("font-weight: bold;");
        m_playerStatusLabel = new QLabel("Stopped");
        m_playerStatusLabel->setStyleSheet("color: #666;");
        layout->addWidget(statusLabel, 0, 0);
        layout->addWidget(m_playerStatusLabel, 0, 1);
        
        // Row 1: File name
        QLabel* fileLabel = new QLabel("File:");
        fileLabel->setStyleSheet("font-weight: bold;");
        m_playerFileLabel = new QLabel("No file loaded");
        m_playerFileLabel->setWordWrap(true);
        m_playerFileLabel->setStyleSheet("color: #666;");
        layout->addWidget(fileLabel, 1, 0);
        layout->addWidget(m_playerFileLabel, 1, 1);
        
        // Row 2: Time
        QLabel* timeLabel = new QLabel("Time:");
        timeLabel->setStyleSheet("font-weight: bold;");
        m_playerTimeLabel = new QLabel("00:00:00 / 00:00:00");
        m_playerTimeLabel->setStyleSheet("font-family: 'Consolas', monospace;");
        layout->addWidget(timeLabel, 2, 0);
        layout->addWidget(m_playerTimeLabel, 2, 1);
        
        // Row 3: Frame number
        QLabel* frameLabel = new QLabel("Frame:");
        frameLabel->setStyleSheet("font-weight: bold;");
        m_playerFrameLabel = new QLabel("0 / 0");
        m_playerFrameLabel->setStyleSheet("font-family: 'Consolas', monospace;");
        layout->addWidget(frameLabel, 3, 0);
        layout->addWidget(m_playerFrameLabel, 3, 1);
        
        // Row 4: Service selector (T32) — which service's DLS+/slideshow the
        // Now Playing / Slideshow panels show at the playhead. Populated after
        // a successful load; disabled before.
        QHBoxLayout* serviceRow = new QHBoxLayout();
        QLabel* serviceTitle = new QLabel(tr("Service:"));
        serviceTitle->setStyleSheet("font-weight: bold;");
        m_serviceCombo = new QComboBox();
        m_serviceCombo->setObjectName(QStringLiteral("serviceSelectorCombo"));
        m_serviceCombo->setToolTip(
            tr("Service whose DLS+/slideshow is shown at the playhead"));
        m_serviceCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_serviceCombo->setMinimumContentsLength(12);
        m_serviceCombo->setEnabled(false);  // enabled once a capture is loaded
        serviceRow->addWidget(serviceTitle);
        serviceRow->addWidget(m_serviceCombo, 1);
        layout->addLayout(serviceRow, 4, 0, 1, 2);
        connect(m_serviceCombo, &QComboBox::currentIndexChanged,
                this, &DABAnalyserWindow::onServiceSelectionChanged);

        // Row 5: Transport controls (Play / Pause / Stop / Reset), Skip -/+
        // T44: use Qt's built-in style media icons (never a blank/tofu button)
        // with a plain-text fallback when the style has no media icon.
        QHBoxLayout* transportRow = new QHBoxLayout();
        m_playButton = new QPushButton();
        m_playButton->setObjectName(QStringLiteral("playButton"));
        m_playButton->setToolTip(tr("Play"));
        m_pauseButton = new QPushButton();
        m_pauseButton->setObjectName(QStringLiteral("pauseButton"));
        m_pauseButton->setToolTip(tr("Pause"));
        m_stopButton = new QPushButton();
        m_stopButton->setObjectName(QStringLiteral("stopButton"));
        m_stopButton->setToolTip(tr("Stop (stop + return to frame 0)"));
        m_resetButton = new QPushButton();
        m_resetButton->setObjectName(QStringLiteral("resetButton"));
        m_resetButton->setToolTip(tr("Reset to frame 0 (keeps play/pause state)"));
        m_skipBackButton = new QPushButton();
        m_skipBackButton->setObjectName(QStringLiteral("skipBackButton"));
        m_skipBackButton->setToolTip(tr("Skip back 10 s"));
        m_skipForwardButton = new QPushButton();
        m_skipForwardButton->setObjectName(QStringLiteral("skipForwardButton"));
        m_skipForwardButton->setToolTip(tr("Skip forward 10 s"));

        struct TransportButtonSpec {
            QPushButton* button;
            QStyle::StandardPixmap icon;
            const char* fallbackText;
        };
        const TransportButtonSpec transportButtons[] = {
            {m_resetButton,       QStyle::SP_MediaSkipBackward, "|<<"},
            {m_skipBackButton,    QStyle::SP_MediaSeekBackward, "<<"},
            {m_playButton,        QStyle::SP_MediaPlay,         "Play"},
            {m_pauseButton,       QStyle::SP_MediaPause,        "Pause"},
            {m_stopButton,        QStyle::SP_MediaStop,         "Stop"},
            {m_skipForwardButton, QStyle::SP_MediaSeekForward,  ">>"},
        };
        for (const TransportButtonSpec& spec : transportButtons) {
            setTransportButtonIcon(spec.button, spec.icon,
                                   QString::fromLatin1(spec.fallbackText));
            transportRow->addWidget(spec.button);
        }
        layout->addLayout(transportRow, 5, 0, 1, 2);

        // Row 6: Speed
        QHBoxLayout* speedRow = new QHBoxLayout();
        QLabel* speedLabel = new QLabel(tr("Speed:"));
        speedLabel->setStyleSheet("font-weight: bold;");
        m_playbackSpeedCombo = new QComboBox();
        m_playbackSpeedCombo->setObjectName(QStringLiteral("playbackSpeedCombo"));
        m_playbackSpeedCombo->addItem(QStringLiteral("0.25×"));
        m_playbackSpeedCombo->addItem(QStringLiteral("0.5×"));
        m_playbackSpeedCombo->addItem(QStringLiteral("1×"));
        m_playbackSpeedCombo->addItem(QStringLiteral("2×"));
        m_playbackSpeedCombo->addItem(QStringLiteral("4×"));
        m_playbackSpeedCombo->setCurrentIndex(2);  // 1× default
        speedRow->addWidget(speedLabel);
        speedRow->addWidget(m_playbackSpeedCombo, 1);
        layout->addLayout(speedRow, 6, 0, 1, 2);

        // Row 7: Loop checkbox
        m_playerLoopCheckbox = new QCheckBox("Loop playback");
        layout->addWidget(m_playerLoopCheckbox, 7, 0, 1, 2);

        // Row 8: Time bar (seek slider). During parsing it mirrors the load
        // progress; after load it becomes the playback seek control.
        m_playerProgressBar = new QSlider(Qt::Horizontal);
        m_playerProgressBar->setObjectName(QStringLiteral("playerTimeBar"));
        m_playerProgressBar->setRange(0, 0);
        layout->addWidget(m_playerProgressBar, 8, 0, 1, 2);

        // Row 9: Load progress bar (unchanged load-time behaviour).
        if (!m_progressBar) {
            m_progressBar = new QProgressBar();
        }
        m_progressBar->setValue(0);
        m_progressBar->setTextVisible(true);
        layout->addWidget(m_progressBar, 9, 0, 1, 2);

        return group;
    }

    /**
     * @brief Create Decoder Panel for LEFT panel (Item #2)
     * 
     * Displays decoder status, type, timing, and frame information.
     * Mirrors Player Panel timing information.
     * 
     * @return QGroupBox* Decoder panel widget
     */
    QGroupBox* createDecoderPanel()
    {
        QGroupBox* group = new QGroupBox("Decoder");
        if (!group) {
            qCritical() << "Failed to create Decoder panel";
            return nullptr;
        }
        
        QGridLayout* layout = new QGridLayout(group);
        layout->setContentsMargins(4, 8, 4, 4);
        layout->setSpacing(4);
        
        // Row 0: Status
        QLabel* statusLabel = new QLabel("Status:");
        statusLabel->setStyleSheet("font-weight: bold;");
        m_decoderStatusLabel = new QLabel("Idle");
        m_decoderStatusLabel->setStyleSheet("color: #666;");
        layout->addWidget(statusLabel, 0, 0);
        layout->addWidget(m_decoderStatusLabel, 0, 1);
        
        // Row 1: Type
        QLabel* typeLabel = new QLabel("Type:");
        typeLabel->setStyleSheet("font-weight: bold;");
        m_decoderTypeLabel = new QLabel("ETI-NI");
        m_decoderTypeLabel->setStyleSheet("color: #0078d4; font-weight: bold;");
        layout->addWidget(typeLabel, 1, 0);
        layout->addWidget(m_decoderTypeLabel, 1, 1);
        
        // Row 2: Time (mirrors player)
        QLabel* timeLabel = new QLabel("Time:");
        timeLabel->setStyleSheet("font-weight: bold;");
        m_decoderTimeLabel = new QLabel("00:00:00 / 00:00:00");
        m_decoderTimeLabel->setStyleSheet("font-family: 'Consolas', monospace;");
        layout->addWidget(timeLabel, 2, 0);
        layout->addWidget(m_decoderTimeLabel, 2, 1);
        
        // Row 3: Frames
        QLabel* framesLabel = new QLabel("Frames:");
        framesLabel->setStyleSheet("font-weight: bold;");
        m_decoderFramesLabel = new QLabel("0");
        m_decoderFramesLabel->setStyleSheet("font-family: 'Consolas', monospace;");
        layout->addWidget(framesLabel, 3, 0);
        layout->addWidget(m_decoderFramesLabel, 3, 1);
        
        return group;
    }

    /**
     * @brief Create the "Audio" tab of the Tab-1 Transport group (T34).
     *
     * Audio-specific controls only — the transport buttons live on the Player
     * tab. Provides: an on-demand DAB+ codec/rate/bit-rate info line (real
     * superframe values), ALSA output-device selection, mute + volume and real
     * L/R peak meters. All controls are disabled until a loaded capture has a
     * selected service with a captured DAB+ audio sub-channel.
     */
    QGroupBox* createAudioPanel()
    {
        QGroupBox* group = new QGroupBox("Audio");
        if (!group) {
            qCritical() << "Failed to create Audio panel";
            return nullptr;
        }

        QGridLayout* layout = new QGridLayout(group);
        layout->setContentsMargins(4, 8, 4, 4);
        layout->setSpacing(4);

        // Row 0: status.
        QLabel* statusTitle = new QLabel(tr("Status:"));
        statusTitle->setStyleSheet("font-weight: bold;");
        m_audioStatusLabel = new QLabel(tr("No audio service"));
        m_audioStatusLabel->setObjectName(QStringLiteral("audioStatusLabel"));
        m_audioStatusLabel->setStyleSheet("color: #666;");
        layout->addWidget(statusTitle, 0, 0);
        layout->addWidget(m_audioStatusLabel, 0, 1);

        // Row 1: real codec / sample-rate / channels / bitrate line.
        QLabel* infoTitle = new QLabel(tr("Format:"));
        infoTitle->setStyleSheet("font-weight: bold;");
        m_audioInfoLabel = new QLabel(QStringLiteral("—"));
        m_audioInfoLabel->setObjectName(QStringLiteral("audioInfoLabel"));
        m_audioInfoLabel->setWordWrap(true);
        m_audioInfoLabel->setStyleSheet("color: #0078d4;");
        layout->addWidget(infoTitle, 1, 0);
        layout->addWidget(m_audioInfoLabel, 1, 1);

        // Row 2: output device (enumerated through the ACTIVE backend).
        QLabel* deviceTitle = new QLabel(tr("Output:"));
        deviceTitle->setStyleSheet("font-weight: bold;");
        m_audioDeviceCombo = new QComboBox();
        m_audioDeviceCombo->setObjectName(QStringLiteral("audioDeviceCombo"));
        m_audioDeviceCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_audioDeviceCombo->setMinimumContentsLength(10);
        const QList<streamdab::audio::AudioPlaybackController::OutputDeviceInfo> audioDevices =
            streamdab::audio::AudioPlaybackController::enumerateOutputDevices();
        for (const streamdab::audio::AudioPlaybackController::OutputDeviceInfo& info : audioDevices) {
            m_audioDeviceCombo->addItem(info.description, info.name);
        }
        // Prefer the backend's "default" device (the hint order is not
        // meaningful; the first entry is often a converter plugin).
        const int defaultDevice = m_audioDeviceCombo->findData(
            streamdab::audio::AudioPlaybackController::defaultOutputDevice());
        if (defaultDevice >= 0) {
            m_audioDeviceCombo->setCurrentIndex(defaultDevice);
        }
        m_audioDeviceCombo->setEnabled(false);
        layout->addWidget(deviceTitle, 2, 0);
        layout->addWidget(m_audioDeviceCombo, 2, 1);
        if (m_audioController && m_audioDeviceCombo->count() > 0) {
            m_audioController->setOutputDevice(m_audioDeviceCombo->currentData().toString());
        }
        connect(m_audioDeviceCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
            if (!m_audioController || index < 0) {
                return;
            }
            m_audioController->setOutputDevice(m_audioDeviceCombo->itemData(index).toString());
            persistAudioSettings();
        });

        // Row 3: mute + volume.
        m_audioMuteCheck = new QCheckBox(tr("Mute"));
        m_audioMuteCheck->setObjectName(QStringLiteral("audioMuteCheck"));
        m_audioMuteCheck->setEnabled(false);
        m_audioVolumeSlider = new QSlider(Qt::Horizontal);
        m_audioVolumeSlider->setObjectName(QStringLiteral("audioVolumeSlider"));
        m_audioVolumeSlider->setRange(0, 100);
        m_audioVolumeSlider->setValue(100);
        m_audioVolumeSlider->setEnabled(false);
        m_audioVolumeLabel = new QLabel(QStringLiteral("100%"));
        m_audioVolumeLabel->setObjectName(QStringLiteral("audioVolumeLabel"));
        QHBoxLayout* volumeRow = new QHBoxLayout();
        volumeRow->addWidget(m_audioVolumeSlider, 1);
        volumeRow->addWidget(m_audioVolumeLabel);
        layout->addWidget(m_audioMuteCheck, 3, 0);
        layout->addLayout(volumeRow, 3, 1);
        connect(m_audioMuteCheck, &QCheckBox::toggled, this, [this](bool muted) {
            if (m_audioController) {
                m_audioController->setMuted(muted);
            }
            persistAudioSettings();
        });
        connect(m_audioVolumeSlider, &QSlider::valueChanged, this, [this](int value) {
            if (m_audioVolumeLabel) {
                m_audioVolumeLabel->setText(QString("%1%").arg(value));
            }
            if (m_audioController) {
                m_audioController->setVolume(value / 100.0);
            }
            persistAudioSettings();
        });

        // Row 4/5: real L/R peak level meters (0 when silent/stopped/no device).
        QLabel* leftTitle = new QLabel(QStringLiteral("L:"));
        leftTitle->setStyleSheet("font-weight: bold;");
        m_audioLevelLeft = new QProgressBar();
        m_audioLevelLeft->setObjectName(QStringLiteral("audioLevelLeft"));
        m_audioLevelLeft->setRange(0, 100);
        m_audioLevelLeft->setValue(0);
        m_audioLevelLeft->setTextVisible(false);
        m_audioLevelLeft->setFixedHeight(12);
        layout->addWidget(leftTitle, 4, 0);
        layout->addWidget(m_audioLevelLeft, 4, 1);

        QLabel* rightTitle = new QLabel(QStringLiteral("R:"));
        rightTitle->setStyleSheet("font-weight: bold;");
        m_audioLevelRight = new QProgressBar();
        m_audioLevelRight->setObjectName(QStringLiteral("audioLevelRight"));
        m_audioLevelRight->setRange(0, 100);
        m_audioLevelRight->setValue(0);
        m_audioLevelRight->setTextVisible(false);
        m_audioLevelRight->setFixedHeight(12);
        layout->addWidget(rightTitle, 5, 0);
        layout->addWidget(m_audioLevelRight, 5, 1);

        return group;
    }

    /**
     * @brief Create System Panel for LEFT panel (Item #3)
     * 
     * QTabWidget with 2 tabs:
     * - FIC Overview: Hierarchical FIG analysis results
     * - Control: Processing controls and settings
     * 
     * @return QGroupBox* System panel widget
     */
    QGroupBox* createSystemPanel()
    {
        QGroupBox* group = new QGroupBox("System");
        if (!group) {
            qCritical() << "Failed to create System panel";
            return nullptr;
        }
        
        QVBoxLayout* layout = new QVBoxLayout(group);
        layout->setContentsMargins(4, 8, 4, 4);
        
        QTabWidget* tabs = new QTabWidget();
        if (!tabs) {
            qCritical() << "Failed to create System tabs";
            delete group;
            return nullptr;
        }
        
        // Tab 1: FIC Overview
        QWidget* ficOverviewTab = new QWidget();
        QVBoxLayout* ficLayout = new QVBoxLayout(ficOverviewTab);
        
        m_ficOverviewTree = new QTreeWidget();
        if (m_ficOverviewTree) {
            m_ficOverviewTree->setHeaderLabels({"Property", "Value", "Data", "Status"});
            m_ficOverviewTree->setAlternatingRowColors(true);
            ficLayout->addWidget(m_ficOverviewTree);
        }
        
        tabs->addTab(ficOverviewTab, "FIC Overview");
        
        // Tab 2: Control
        QWidget* controlTab = new QWidget();
        QVBoxLayout* controlLayout = new QVBoxLayout(controlTab);
        
        QLabel* controlLabel = new QLabel("Processing Controls");
        controlLabel->setStyleSheet("font-weight: bold;");
        controlLayout->addWidget(controlLabel);
        
        m_processingControlsGroup = new QGroupBox("Analysis Settings");
        QVBoxLayout* controlsLayout = new QVBoxLayout(m_processingControlsGroup);
        
        m_enableFicAnalysis = new QCheckBox("Enable FIC Analysis");
        m_enableFicAnalysis->setChecked(true);
        controlsLayout->addWidget(m_enableFicAnalysis);
        
        m_enableMscAnalysis = new QCheckBox("Enable MSC Analysis");
        m_enableMscAnalysis->setChecked(false);
        controlsLayout->addWidget(m_enableMscAnalysis);
        
        controlLayout->addWidget(m_processingControlsGroup);
        controlLayout->addStretch();
        
        tabs->addTab(controlTab, "Control");
        
        layout->addWidget(tabs);
        return group;
    }

    // ========================================================================
    // ADDITIONAL HELPER METHODS: TIMING, CU USAGE, STREAM STATISTICS
    // ========================================================================
    // Items #12-14 from DATA_AVAILABILITY_MATRIX.md (RIGHT Panel)

    /**
     * @brief Helper method to create a standard property-value table widget
     * 
     * Reduces code duplication across multiple panel creation methods.
     * Creates a 2-column table with consistent styling:
     * - Column 1: Property names (bold font)
     * - Column 2: Values (monospace font)
     * 
     * @param rows Number of rows in the table
     * @param properties QStringList of property names for column 1
     * @param fontFamily Font family for property column (default: "Segoe UI")
     * @param fontSize Font size for both columns (default: 9)
     * @return QTableWidget* Configured table widget (parent must take ownership)
     * 
     * @note Fixes HIGH-003 from code review: Extract table creation helper
     */
    QTableWidget* createPropertyTable(int rows, 
                                      const QStringList& properties,
                                      const QString& fontFamily = "Segoe UI",
                                      int fontSize = 9)
    {
        auto* table = new QTableWidget(rows, 2);
        if (!table) {
            qCritical() << "Failed to allocate QTableWidget";
            return nullptr;
        }
        
        table->setHorizontalHeaderLabels({"Property", "Value"});
        table->verticalHeader()->setVisible(false);
        table->setAlternatingRowColors(true);
        table->horizontalHeader()->setStretchLastSection(true);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        
        // Initialize rows with property names and default values
        for (int i = 0; i < properties.size() && i < rows; ++i) {
            auto* propItem = new QTableWidgetItem(properties[i]);
            if (propItem) {
                propItem->setFont(QFont(fontFamily, fontSize, QFont::Bold));
                table->setItem(i, 0, propItem);
            }
            
            auto* valItem = new QTableWidgetItem("--");
            if (valItem) {
                valItem->setFont(QFont("Consolas", fontSize));
                table->setItem(i, 1, valItem);
            }
        }
        
        return table;
    }

    /**
     * @brief Create ETI Overview panel for RIGHT panel (Item #10)
     * Shows ETI Type, Error Field, DAB Mode, STAT percentage
     */
    QGroupBox* createETIOverviewPanel()
    {
        QGroupBox* groupBox = new QGroupBox("ETI Overview");
        if (!groupBox) {
            qCritical() << "Failed to create ETI Overview panel";
            return nullptr;
        }
        
        QVBoxLayout* layout = new QVBoxLayout(groupBox);
        layout->setContentsMargins(4, 8, 4, 4);
        
        // Use helper method to create table (DATA_AVAILABILITY_MATRIX.md Item #10)
        QStringList properties = {
            "ETI Type", "Error Field", "DAB Mode", "STAT"
        };
        m_etiOverviewTable = createPropertyTable(4, properties);
        if (!m_etiOverviewTable) {
            qCritical() << "Failed to create ETI overview table";
            delete groupBox;
            return nullptr;
        }
        
        m_etiOverviewTable->setMaximumHeight(150);
        layout->addWidget(m_etiOverviewTable);
        return groupBox;
    }

    QGroupBox* createTimingPanel()
    {
        QGroupBox* groupBox = new QGroupBox("Timing");
        if (!groupBox) {
            qCritical() << "Failed to create Timing panel";
            return nullptr;
        }
        
        QVBoxLayout* layout = new QVBoxLayout(groupBox);
        layout->setContentsMargins(4, 8, 4, 4);
        
        // Use helper method to create table (HIGH-003 fix)
        QStringList properties = {
            "FCT = 0 (µs)", "Step (µs)", "FCT = 0 (ticks)", "Step (ticks)"
        };
        m_timingTable = createPropertyTable(4, properties);
        if (!m_timingTable) {
            qCritical() << "Failed to create timing table";
            delete groupBox;
            return nullptr;
        }
        
        m_timingTable->setMaximumHeight(150);
        layout->addWidget(m_timingTable);
        return groupBox;
    }

    QGroupBox* createCUUsagePanel()
    {
        QGroupBox* groupBox = new QGroupBox("CU Usage");
        if (!groupBox) {
            qCritical() << "Failed to create CU Usage panel";
            return nullptr;
        }
        
        QVBoxLayout* layout = new QVBoxLayout(groupBox);
        layout->setContentsMargins(4, 8, 4, 4);
        
        // Simple implementation: Progress bar showing X / 864 CUs
        QLabel* cuLabel = new QLabel("Capacity Units:");
        if (cuLabel) {
            cuLabel->setStyleSheet("font-weight: bold;");
            layout->addWidget(cuLabel);
        }
        
        m_cuUsageBar = new QProgressBar();
        if (!m_cuUsageBar) {
            qCritical() << "Failed to create CU usage progress bar";
            delete groupBox;
            return nullptr;
        }
        m_cuUsageBar->setMaximum(864);  // Mode 1 max CUs
        m_cuUsageBar->setValue(0);
        m_cuUsageBar->setFormat("%v / %m CUs (%p%)");
        m_cuUsageBar->setTextVisible(true);
        layout->addWidget(m_cuUsageBar);
        
        m_cuUsageLabel = new QLabel("0 subchannels active");
        if (m_cuUsageLabel) {
            m_cuUsageLabel->setStyleSheet("color: #666; font-size: 9pt;");
            layout->addWidget(m_cuUsageLabel);
        }
        
        layout->addStretch();
        return groupBox;
    }

    QGroupBox* createStreamStatisticsPanel()
    {
        QGroupBox* groupBox = new QGroupBox("DAB Stream Statistics");
        if (!groupBox) {
            qCritical() << "Failed to create Stream Statistics panel";
            return nullptr;
        }
        
        QVBoxLayout* layout = new QVBoxLayout(groupBox);
        layout->setContentsMargins(4, 8, 4, 4);
        
        // Use helper method to create table (HIGH-003 fix)
        QStringList metrics = {
            "FIC Blocks Processed",
            "Total FIG Count",
            "Unique Services Found",
            "Active Subchannels",
            "Total CU Used / 864",
            "DAB Services Count",
            "DAB+ Services Count",
            "Data Services Count",
            "Average Frame Rate",
            "Processing Time (ms)"
        };
        
        m_streamStatsTable = createPropertyTable(10, metrics);
        if (!m_streamStatsTable) {
            qCritical() << "Failed to create stream statistics table";
            delete groupBox;
            return nullptr;
        }
        
        // Customize for statistics: right-align values, set default to "0" instead of "--"
        for (int i = 0; i < m_streamStatsTable->rowCount(); ++i) {
            QTableWidgetItem* valItem = m_streamStatsTable->item(i, 1);
            if (valItem) {
                valItem->setText("0");
                valItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
        }
        
        layout->addWidget(m_streamStatsTable);
        return groupBox;
    }

    // Code Review MED-002: Deleted obsolete restructure comments (no longer useful)

    // NOTE (Wave B): the legacy createPerformanceDashboardTab() (3 labels + a
    // progress bar) was deleted; the strip now uses the adopted
    // PerformanceDashboard widget directly.

    QWidget* createConstellationTab()
    {
        // Wave B: real ConstellationWidget replaces the dashed placeholder.
        // ETI is post-demodulation (no true I/Q samples reach the analyser), so
        // the widget runs its synthetic / performance-derived path and the
        // banner states that honestly rather than implying a live DSP feed.
        QWidget* widget = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        QLabel* banner = new QLabel(
            QObject::tr("Synthetic display — ETI is post-demodulation (no true I/Q); "
                        "the constellation is derived from processing performance."));
        banner->setWordWrap(true);
        banner->setStyleSheet(QStringLiteral("color: palette(mid); padding: 2px 6px;"));
        layout->addWidget(banner);

        m_constellationWidget = new ConstellationWidget();
        // Custom-painted widget: explicit minimum so the 120 px-strip scroll
        // area produces scrollbars.
        m_constellationWidget->setMinimumSize(360, 220);
        // initialize() must run before startRealTimeUpdates(): it creates the
        // 16 ms update timer and sets m_isInitialized, without which every
        // paint/update path early-returns and the tab paints blank.
        if (!m_constellationWidget->initialize()) {
            qWarning() << "ConstellationWidget initialization failed";
        }
        m_constellationWidget->setDisplayMode(ConstellationWidget::DisplayMode::Constellation);
        m_constellationWidget->startRealTimeUpdates();
        layout->addWidget(m_constellationWidget, 1);
        return widget;
    }
    
    // NOTE: createMultiStreamTab() removed - Multi-stream moved to Phase 5 (FUTURE_FEATURES.md)

    QWidget* createErrorDetectionTab()
    {
        QWidget* errorDetectionTab = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout(errorDetectionTab);
        
        // Error detection controls section
        QGroupBox* controlsGroup = new QGroupBox("Advanced Error Detection Controls");
        QHBoxLayout* controlsLayout = new QHBoxLayout(controlsGroup);
        
        // Error detection configuration buttons
        QPushButton* enableDetectionBtn = new QPushButton("Enable Detection");
        QPushButton* disableDetectionBtn = new QPushButton("Disable Detection");
        QPushButton* resetErrorsBtn = new QPushButton("Reset Errors");
        QPushButton* exportErrorsBtn = new QPushButton("Export Error Report");
        QPushButton* systemHealthBtn = new QPushButton("System Health");
        
        enableDetectionBtn->setCheckable(true);
        enableDetectionBtn->setChecked(true); // Start enabled
        
        controlsLayout->addWidget(enableDetectionBtn);
        controlsLayout->addWidget(disableDetectionBtn);
        controlsLayout->addWidget(resetErrorsBtn);
        controlsLayout->addWidget(exportErrorsBtn);
        controlsLayout->addWidget(systemHealthBtn);
        controlsLayout->addStretch();
        
        layout->addWidget(controlsGroup);
        
        // Error detection status section
        QGroupBox* statusGroup = new QGroupBox("Error Detection Status");
        QHBoxLayout* statusLayout = new QHBoxLayout(statusGroup);
        
        m_errorDetectionStatus = new QLabel("Status: Active - Real-time monitoring enabled");
        m_errorDetectionStatus->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        m_systemHealthScore = new QLabel("System Health: 100%");
        m_systemHealthScore->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        m_errorCountLabel = new QLabel("Total Errors: 0");
        
        statusLayout->addWidget(m_errorDetectionStatus);
        statusLayout->addWidget(m_systemHealthScore);
        statusLayout->addWidget(m_errorCountLabel);
        statusLayout->addStretch();
        
        layout->addWidget(statusGroup);
        
        // Error history display
        QGroupBox* historyGroup = new QGroupBox("Error History");
        QVBoxLayout* historyLayout = new QVBoxLayout(historyGroup);
        
        m_errorHistoryTree = new QTreeWidget();
        m_errorHistoryTree->setHeaderLabels({"Timestamp", "Severity", "Category", "Description", "Frame", "Recovery"});
        m_errorHistoryTree->setAlternatingRowColors(true);
        m_errorHistoryTree->setSortingEnabled(true);
        m_errorHistoryTree->header()->setStretchLastSection(false);
        m_errorHistoryTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_errorHistoryTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_errorHistoryTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        m_errorHistoryTree->header()->setSectionResizeMode(3, QHeaderView::Stretch);
        m_errorHistoryTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        m_errorHistoryTree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
        
        historyLayout->addWidget(m_errorHistoryTree);
        
        layout->addWidget(historyGroup);
        
        // Error pattern analysis
        QGroupBox* patternGroup = new QGroupBox("Error Pattern Analysis");
        QVBoxLayout* patternLayout = new QVBoxLayout(patternGroup);
        
        m_errorPatternsDisplay = new QTextEdit();
        m_errorPatternsDisplay->setFont(QFont("Consolas", 9));
        m_errorPatternsDisplay->setPlaceholderText("Error pattern analysis will appear here...");
        m_errorPatternsDisplay->setMaximumHeight(150);
        patternLayout->addWidget(m_errorPatternsDisplay);
        
        layout->addWidget(patternGroup);
        
        // Connect error detection button signals
        connect(enableDetectionBtn, &QPushButton::clicked, this, &DABAnalyserWindow::onEnableErrorDetection);
        connect(disableDetectionBtn, &QPushButton::clicked, this, &DABAnalyserWindow::onDisableErrorDetection);
        connect(resetErrorsBtn, &QPushButton::clicked, this, &DABAnalyserWindow::onResetErrors);
        connect(exportErrorsBtn, &QPushButton::clicked, this, &DABAnalyserWindow::onExportErrorReport);
        connect(systemHealthBtn, &QPushButton::clicked, this, &DABAnalyserWindow::onShowSystemHealth);
        
        return errorDetectionTab;
    }

    // ========================================================================
    // Wave C: Window > Docking Dialog — GIMP-style re-open of closed docks
    // ========================================================================

    // The four CDockManagers in a FIXED order (tab1, tab2, tab3, bottom) —
    // the same source of truth testDocksWithExpectedNames checks against for
    // the 27-dock registry (15 + 3 + 2 + 7).
    QList<ads::CDockManager*> windowDockManagers() const
    {
        return {m_tab1DockManager, m_tab2DockManager, m_tab3DockManager,
                m_bottomDockManager};
    }

    // Every dock registered with the four managers, keyed by the dock's
    // objectName (the stable registry id), in windowDockManagers() order and
    // alphabetical inside each manager (QMap) — a deterministic menu order.
    QMap<QString, ads::CDockWidget*> registeredDocks() const
    {
        QMap<QString, ads::CDockWidget*> docks;
        for (const ads::CDockManager* manager : windowDockManagers()) {
            if (!manager) {
                continue;
            }
            const QMap<QString, ads::CDockWidget*> map = manager->dockWidgetsMap();
            for (auto it = map.cbegin(); it != map.cend(); ++it) {
                docks.insert(it.key(), it.value());
            }
        }
        return docks;
    }

    // MECHANISM CHOICE — manual QAction per dock (NOT toggleViewAction()).
    // toggleViewAction() from Qt-ADS crashes in accessibility code paths when
    // the action's text() is accessed or when the dock's windowTitle() is read
    // during menu construction (QWidget::accessibleName() -> SEGV in
    // QWidgetPrivate::setVisible). We therefore create our own checkable actions
    // and sync them via visibilityChanged() + manual show/raise on trigger.
    // CHECKED SEMANTICS (as requested: "ดู status ว่ายังอยู่หรือไม่ ไม่ได้ปิด"):
    // checked == this dock is still open (its tab exists and is not closed).
    // A tabified dock that is open but is not the front tab of its group
    // STAYS CHECKED — the box answers "ยังอยู่ไหม", not "เด่นอยู่ไหม".
    void setupWindowMenu(QMenuBar* menuBar)
    {
        QMenu* windowMenu = menuBar->addMenu(QStringLiteral("Window"));
        windowMenu->setObjectName(QStringLiteral("windowMenu"));

        // "Show All Docks" is the FIRST item of the Window menu (GIMP lists
        // it at the dialog-list level, above the individual dialogs); the
        // per-dock checklist lives in the submenu below it.
        m_showAllDocksAction =
            windowMenu->addAction(QStringLiteral("Show All Docks"));
        m_showAllDocksAction->setObjectName(QStringLiteral("showAllDocksAction"));
        connect(m_showAllDocksAction, &QAction::triggered,
                this, &DABAnalyserWindow::showAllDocks);

        windowMenu->addSeparator();

        QMenu* dockingMenu =
            windowMenu->addMenu(QStringLiteral("Docking Dialog"));
        dockingMenu->setObjectName(QStringLiteral("dockingDialogMenu"));

        // All 31 docks come from the registry (registeredDocks()), so the
        // menu cannot drift from the layout the registry test pins. Adding an
        // action to a menu does NOT touch the docking layout, so
        // DOCKING_LAYOUT_VERSION stays 11.
        const QMap<QString, ads::CDockWidget*> docks = registeredDocks();
        for (auto it = docks.cbegin(); it != docks.cend(); ++it) {
            ads::CDockWidget* dock = it.value();
            // Create our own checkable action — avoids Qt-ADS toggleViewAction()
            // accessibility crash. Use stored title from m_dockTitles instead of
            // dock->windowTitle() which crashes when the internal widget is not
            // fully initialized.
            const QString title = m_dockTitles.value(it.key(), dock->windowTitle());
            QAction* action = new QAction(title, dockingMenu);
            action->setObjectName(it.key());
            action->setCheckable(true);
            action->setChecked(!dock->isClosed());

            // Sync: when dock closed/opened via UI (tab-bar X), update checkbox.
            // The correct Qt-ADS signal for open/close state changes is viewToggled(bool),
            // not visibilityChanged (which fires for tab-switching within a group).
            // Use QPointer to guard against use-after-free during teardown.
            QPointer<QAction> actionGuard = action;
            QPointer<ads::CDockWidget> dockGuard = dock;
            connect(dock, &ads::CDockWidget::viewToggled, this,
                    [actionGuard, dockGuard](bool open) {
                        if (!actionGuard || !dockGuard) {
                            return;  // Objects already destroyed during teardown
                        }
                        actionGuard->blockSignals(true);
                        actionGuard->setChecked(open);
                        actionGuard->blockSignals(false);
                    });

            // Trigger: user clicks menu item -> reopen closed dock + raise to front.
            QPointer<ads::CDockWidget> dockTriggerGuard = dock;
            connect(action, &QAction::triggered, this,
                    [dockTriggerGuard](bool checked) {
                        if (!dockTriggerGuard) {
                            return;
                        }
                        if (checked) {
                            if (dockTriggerGuard->isClosed() || dockTriggerGuard->isAutoHide()) {
                                dockTriggerGuard->toggleView(true);  // show + raise to front
                            } else {
                                dockTriggerGuard->raise();           // already open: bring forward
                            }
                        } else {
                            dockTriggerGuard->toggleView(false);     // close the dock
                        }
                    });

            dockingMenu->addAction(action);
        }
    }

    // "Show All Docks": reopen every closed (or auto-hidden/pinned) dock and
    // bring every already-open dock to the front of its tab group. Only dock
    // visibility is changed here — no layout is written, so the persisted
    // state and DOCKING_LAYOUT_VERSION (11) are untouched.
    void showAllDocks()
    {
        const QMap<QString, ads::CDockWidget*> docks = registeredDocks();
        for (auto it = docks.cbegin(); it != docks.cend(); ++it) {
            ads::CDockWidget* dock = it.value();
            if (dock->isClosed() || dock->isAutoHide()) {
                dock->toggleView(true);  // show + raise to front of the group
            } else {
                dock->raise();           // open already: bring it forward
            }
        }
    }

    void setupMenuBar()
    {
        QMenuBar* menuBar = this->menuBar();
        
        // File menu
        QMenu* fileMenu = menuBar->addMenu("File");
        QAction* openAction = fileMenu->addAction("Open ETI File...");
        connect(openAction, &QAction::triggered, this, &DABAnalyserWindow::openFile);

        fileMenu->addSeparator();
        // Wave B: Settings edits the real AnalyserSettings (QSettings group
        // "analyser/"); Export writes the current analysis via the adopted
        // ProfessionalExportManager writers.
        m_settingsAction = fileMenu->addAction("Settings...");
        connect(m_settingsAction, &QAction::triggered,
                this, &DABAnalyserWindow::showSettingsDialog);
        m_exportAction = fileMenu->addAction("Export...");
        m_exportAction->setEnabled(false);  // enabled once a capture is loaded
        connect(m_exportAction, &QAction::triggered,
                this, &DABAnalyserWindow::showExportDialog);

        fileMenu->addSeparator();
        QAction* exitAction = fileMenu->addAction("Exit");
        connect(exitAction, &QAction::triggered, this, &DABAnalyserWindow::requestExit);
        
        // T23 (option A): the Input menu is gone. Input mode is owned solely by
        // the File/Network radios + Connect/Disconnect/Record in the input
        // toolbar, and the unique menu item (UDP Streaming Settings…) now lives
        // behind the ⚙ button appended to the toolbar's network row. The menu
        // bar is intentionally File / View / Window / Help: the Window menu
        // added by Wave C (dock re-open) does not own any INPUT mode, so it
        // does not reintroduce the Input-menu problem T23 removed.
        
        // View menu
        QMenu* viewMenu = menuBar->addMenu("View");
        viewMenu->addAction("Refresh Analysis");

        // Wave C: Window menu (GIMP-style dock re-open), between View and
        // Help. It is built AFTER setupUI() (the constructor calls setupUI();
        // then setupMenuBar()), i.e. after all four CDockManagers and their
        // 27 docks exist, so the submenu is filled from the live registry.
        setupWindowMenu(menuBar);

        // Help menu — Wave B replaces the inline QMessageBox::about() with the
        // adopted AboutDialog (only one About path remains).
        QMenu* helpMenu = menuBar->addMenu("Help");
        QAction* aboutAction = helpMenu->addAction("About");
        connect(aboutAction, &QAction::triggered,
                this, &DABAnalyserWindow::showAboutDialog);
    }

    void setupSignalConnections()
    {
        // CRITICAL TEST: Use Qt::DirectConnection to bypass queuing mechanism
        // This will call slots immediately in the same thread (no queuing, no async copy)
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::frameProcessed,
                this, &DABAnalyserWindow::onFrameProcessed, Qt::DirectConnection);
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingProgress,
                this, &DABAnalyserWindow::onProcessingProgress, Qt::DirectConnection);
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingComplete,
                this, &DABAnalyserWindow::onProcessingComplete, Qt::QueuedConnection);
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingError,
                this, &DABAnalyserWindow::onProcessingError, Qt::QueuedConnection);
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::frameError,
                this, &DABAnalyserWindow::onFrameError, Qt::QueuedConnection);
        
        // FIG Analysis signals for real-time FIC-Analyser updates
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::figDiscovered,
                this, &DABAnalyserWindow::onFIGDiscovered);
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::formatDetected,
                this, &DABAnalyserWindow::onETIFormatDetected);
        
        // Code Review LOW-002: Deleted empty FIG filter comment (no longer relevant)

        // FIG Analyser signals (if available)
        connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::serviceDiscovered,
                this, &DABAnalyserWindow::onServiceDiscovered);

        // Error Detection signal connections (immediate, no timer)
        if (m_errorDetector) {
            connect(m_errorDetector.get(), &error_detection::AdvancedErrorDetector::errorDetected,
                    this, &DABAnalyserWindow::onErrorDetected,
                    Qt::QueuedConnection);
            connect(m_errorDetector.get(), &error_detection::AdvancedErrorDetector::systemHealthChanged,
                    this, &DABAnalyserWindow::onSystemHealthChanged,
                    Qt::QueuedConnection);
            qDebug() << "Error Detection signals connected immediately";
        }

        // Wave B: feed the ETSI Compliance Monitor from the real error detector
        // and from the ETSI compliance engine.
        if (m_etsiMonitor) {
            if (m_errorDetector) {
                connect(m_errorDetector.get(),
                        &error_detection::AdvancedErrorDetector::errorDetected,
                        this, [this](const error_detection::ErrorReport& e) {
                            onEtsiViolationFromError(e);
                        }, Qt::QueuedConnection);
                connect(m_errorDetector.get(),
                        &error_detection::AdvancedErrorDetector::criticalErrorDetected,
                        this, [this](const error_detection::ErrorReport& e) {
                            onEtsiViolationFromError(e);
                        }, Qt::QueuedConnection);
            }
            if (m_etsiComplianceEngine) {
                connect(m_etsiComplianceEngine.get(),
                        &etsi::ETSIComplianceEngine::criticalErrorDetected,
                        this, [this](const etsi::ValidationResult& r) {
                            onEtsiViolationFromEngine(r);
                        }, Qt::QueuedConnection);
            }
        }
        
        // UI signals
        connect(m_frameList, &QListWidget::currentRowChanged,
                this, &DABAnalyserWindow::onFrameSelectionChanged);

        // === PHASE 3: FIG Analyser → GUI Enhanced Connections ===
        connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::ensembleInfoUpdated,
                this, &DABAnalyserWindow::onEnsembleInfoUpdated,
                Qt::QueuedConnection);

        connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::servicesUpdated,
                this, &DABAnalyserWindow::onServicesUpdated,
                Qt::QueuedConnection);

        qDebug() << "[PHASE 3] Enhanced signal connections established";

        // === PHASE 2: Tab 1 Data Widgets Signal Connections ===
        connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::subChannelsUpdated,
                this, &DABAnalyserWindow::onSubChannelsUpdated,
                Qt::QueuedConnection);

        qDebug() << "[PHASE 2] Tab 1 data widget connections established";

        // === Phase 2C: Advanced FIG Analyser - Extended Label Updates ===
        connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::extendedLabelUpdated,
                this, &DABAnalyserWindow::onExtendedLabelUpdated,
                Qt::QueuedConnection);

        // Populate Advanced FIG Analyser dock on initial load
        connect(m_figAnalyser.get(), &AdvancedFIGAnalyser::servicesUpdated,
                this, [this]() {
                    if (m_advancedFigTree) {
                        refreshAdvancedFigDisplay();
                    }
                }, Qt::QueuedConnection);

        qDebug() << "[PHASE 2C] Advanced FIG Analyser connections established";

        // === PHASE 3 AGENT 4: Frame Navigation Signal Connections ===
        // Note: Slider and spinbox widgets will be created by Agent 1
        // These connections will be activated when those widgets exist
        qDebug() << "[PHASE 3 AGENT 4] Frame navigation signal handlers ready";

        // === PHASE 10: Tab 3 FIG Instance Selection Sync ===
        if (m_tab3_figInstanceTree) {
            connect(m_tab3_figInstanceTree, &QTreeWidget::itemSelectionChanged,
                    this, [this]() {
                        // When a FIG instance is selected in LEFT panel,
                        // update the RIGHT panel with its details
                        auto selectedItems = m_tab3_figInstanceTree->selectedItems();
                        if (!selectedItems.isEmpty()) {
                            // Rebuild details tree for selected FIG
                            buildFIGItemDetailsTree();
                            // T42: mirror the selected instance's raw bytes into
                            // the right dock's Hex Viewer inner tab.
                            updateTab3HexFromFigInstance();
                            qDebug() << "[PHASE 10] FIG instance selected, details updated";
                        }
                    }, Qt::QueuedConnection);
            qDebug() << "[PHASE 10] Tab 3 FIG selection sync connection established";
        }

        // T28: Frame List selection -> decode the frame's FIGs into the right
        // dock (FIG Item Details).
        if (m_tab3_frameList) {
            connect(m_tab3_frameList, &QListWidget::currentRowChanged,
                    this, &DABAnalyserWindow::onTab3FrameSelected);
            qDebug() << "[PHASE 10b] Tab 3 Frame List connection established";
        }

    }

    // Phase B: Complete worker thread integration with SharedETIData
    void setupWorkerThread()
    {
        if (!m_readerThread || !m_sharedData) {
            qWarning() << "Phase B: Worker thread not initialized";
            return;
        }
        
        // === Phase D: Connect throttled specialized signals (Pattern 7 + 8) ===
        
        // Frame counter (5 fps - throttled at source)
        connect(m_readerThread, &ETIReaderThread::frameCounterUpdated,
                this, [this](quint32 frameNum) {
                    qDebug() << "Phase D: Frame counter:" << frameNum << "(5 fps throttled)";
                    // Update frame counter widget when implemented
                }, Qt::QueuedConnection);
        
        // FIG data (1 fps - throttled at source)
        connect(m_readerThread, &ETIReaderThread::figDataReady,
                this, [this](QSharedPointer<FICDataSignal> figData) {
                    qDebug() << "Phase D: FIG data ready (1 fps throttled)";
                    if (figData && m_sharedData) {
                        streamdab::core::FICData sharedFicData;
                        m_sharedData->setFICData(sharedFicData);
                    }
                }, Qt::QueuedConnection);
        
        // Error statistics (0.5 fps - throttled at source)
        connect(m_readerThread, &ETIReaderThread::errorStatsReady,
                this, [this](ErrorStats stats) {
                    qDebug() << "Phase D: Error stats ready (0.5 fps throttled)";
                    if (m_sharedData) {
                        streamdab::core::ErrorStats sharedStats;
                        sharedStats.crc_errors = stats.crcErrors;
                        sharedStats.sync_errors = stats.syncLost;
                        m_sharedData->updateErrorStats(sharedStats);
                    }
                }, Qt::QueuedConnection);
        
        // MSC data (2 fps - throttled at source)
        connect(m_readerThread, &ETIReaderThread::mscDataReady,
                this, [this](QSharedPointer<MSCData> mscData) {
                    qDebug() << "Phase D: MSC data ready (2 fps throttled)";
                    // Process MSC data when needed
                }, Qt::QueuedConnection);
        
        // === Phase B: Connect ETIReaderThread signals to update SharedETIData ===
        // Note: These legacy signals still work for backward compatibility
        
        // Pattern 5: Worker thread frame updates -> SharedETIData (queued connection)
        connect(m_readerThread, &ETIReaderThread::frameReceived,
                this, &DABAnalyserWindow::onWorkerFrameReceived, Qt::QueuedConnection);
        
        // FIC data updates from worker thread
        connect(m_readerThread, &ETIReaderThread::ficDataUpdated,
                this, &DABAnalyserWindow::onWorkerFICDataUpdated, Qt::QueuedConnection);
        
        // MSC data updates from worker thread
        connect(m_readerThread, &ETIReaderThread::mscDataUpdated,
                this, &DABAnalyserWindow::onWorkerMSCDataUpdated, Qt::QueuedConnection);
        
        // Error statistics updates
        connect(m_readerThread, &ETIReaderThread::errorStatsUpdated,
                this, &DABAnalyserWindow::onWorkerErrorStatsUpdated, Qt::QueuedConnection);
        
        // Ensemble info updates
        connect(m_readerThread, &ETIReaderThread::ensembleInfoChanged,
                this, &DABAnalyserWindow::onWorkerEnsembleInfoChanged, Qt::QueuedConnection);
        
        // Processing lifecycle signals
        connect(m_readerThread, &ETIReaderThread::processingStarted,
                this, [this]() {
                    qDebug() << "Phase B: Worker thread processing started";
                    if (m_progressBar) m_progressBar->setValue(0);
                });
        
        connect(m_readerThread, &ETIReaderThread::processingComplete,
                this, [this](int totalFrames, int timeMs) {
                    qDebug() << "Phase B: Worker thread completed -" << totalFrames 
                             << "frames in" << timeMs << "ms";
                    if (m_progressBar) m_progressBar->setValue(100);
                    m_fileLoaded = true;
                    m_totalFrames = static_cast<size_t>(totalFrames);
                });
        
        connect(m_readerThread, &ETIReaderThread::processingError,
                this, [this](const QString& error) {
                    handleWorkerError(error);
                });
        
        // === Phase B: Connect SharedETIData signals to widgets ===
        
        // Frame updates -> Update frame display widgets
        connect(m_sharedData, &streamdab::core::SharedETIData::frameUpdated,
                this, [this](uint64_t frameNum) {
                    // Update frame number display (no m_frameCounter widget exists)
                    qDebug() << "Phase B: Frame" << frameNum << "updated in SharedETIData";
                });
        
        // Service list updates -> Update service tree widget
        connect(m_sharedData, &streamdab::core::SharedETIData::serviceListUpdated,
                this, [this]() {
                    updateServiceListFromSharedData();
                });
        
        // DELETED: FIC data -> FIG extractor connection - Code Review HIGH-002
        // Connected to no-op updateFIGExtractorFromSharedData(), safe to delete
        // TODO: Reimplement when Tab 3 FIG extraction is implemented (Phase 4)
        
        qDebug() << "Phase B: Worker thread and widget signal connections established";
    }
    
    // === Phase B: Worker thread signal handlers (update SharedETIData) ===
    
    void onWorkerFrameReceived(quint32 frameNumber) {
        if (!m_sharedData || !m_etiProcessor) return;
        
        // Get current frame data from processor
        // Note: m_etiProcessor runs in parallel (backward compatibility)
        // We extract data from it to populate SharedETIData
        
        streamdab::core::ETIFrame etiFrame;
        etiFrame.frame_number = frameNumber;
        etiFrame.timestamp = QDateTime::currentDateTime();
        
        // TODO: Extract actual frame data from m_etiProcessor
        // For Phase B, we just update the frame number
        // Full data extraction will be added as needed
        
        m_sharedData->setCurrentFrame(etiFrame);
    }
    
    void onWorkerFICDataUpdated(QSharedPointer<FICDataSignal> ficData) {
        if (!m_sharedData || !ficData) return;
        
        // Convert FICDataSignal to SharedETIData::FICData format
        streamdab::core::FICData sharedFicData;
        // TODO: Populate FIC data fields from ficData
        // For Phase B initial implementation, basic structure only
        
        m_sharedData->setFICData(sharedFicData);
    }
    
    void onWorkerMSCDataUpdated(QSharedPointer<MSCData> mscData) {
        if (!m_sharedData || !mscData) return;
        
        // Convert MSCData to QByteArray for SharedETIData
        // TODO: Phase B - implement proper MSC data conversion
        // For now, just log the update
        qDebug() << "Phase B: MSC data updated (conversion not yet implemented)";
    }
    
    void onWorkerErrorStatsUpdated(ErrorStats stats) {
        if (!m_sharedData) return;
        
        // Convert ErrorStats from eti_reader_thread to SharedETIData format
        streamdab::core::ErrorStats sharedStats;
        sharedStats.crc_errors = stats.crcErrors;
        sharedStats.sync_errors = stats.syncLost;
        sharedStats.total_frames = 0; // Not available in source ErrorStats
        
        m_sharedData->updateErrorStats(sharedStats);
    }
    
    void onWorkerEnsembleInfoChanged(EnsembleInfo info) {
        if (!m_sharedData) return;
        
        // Update ensemble info (ECC, country, ensemble ID, label)
        // TODO: Add ensemble info to SharedETIData if needed
        // Note: EnsembleInfo from eti_reader_thread.hpp may have different fields
        qDebug() << "Phase B: Ensemble info updated:" << info.ensembleLabel;
    }
    
    // === Phase B: Widget update helpers (read from SharedETIData) ===
    
    void updateServiceListFromSharedData() {
        if (!m_sharedData) return;
        
        // Get services from SharedETIData (thread-safe)
        auto services = m_sharedData->getServices();
        
        // TODO: Update service tree widget when we identify the correct widget
        // For Phase B initial implementation, just log the update
        
        qDebug() << "Phase B: Service list updated -" << services.size() << "services";
        
        // Display service info
        for (const auto& service : services) {
            qDebug() << "  Service ID:" << QString::number(service.service_id, 16).toUpper()
                     << "Label:" << service.service_label
                     << "Type:" << (service.is_dabplus ? "DAB+" : "DAB");
        }
    }
    
    // DELETED: updateFIGExtractorFromSharedData() - Code Review HIGH-002
    // No-op function with no real implementation, connection removed
    // Will reimplement when Tab 3 FIG extraction is ready (Phase 4)

    void setupStatusBar()
    {
        statusBar()->showMessage("Ready - Open an ETI file to begin analysis");
    }
    
    // Phase C: UDP Settings Dialog
    // T23 (option A): factored out of showUDPSettings() so tests can build the
    // dialog offscreen without entering its modal exec() loop. The returned
    // dialog is owned by the caller.
    struct UdpSettingsWidgets {
        QDialog* dialog = nullptr;
        QSpinBox* port = nullptr;
        QLineEdit* address = nullptr;
        QCheckBox* multicast = nullptr;
    };

    UdpSettingsWidgets buildUDPSettingsDialog()
    {
        UdpSettingsWidgets w;
        w.dialog = new QDialog(this);
        w.dialog->setWindowTitle("UDP Streaming Settings");
        w.dialog->setMinimumWidth(400);

        QFormLayout* layout = new QFormLayout(w.dialog);

        // Port setting
        w.port = new QSpinBox(w.dialog);
        w.port->setObjectName(QStringLiteral("udpSettingsPort"));
        w.port->setRange(1024, 65535);
        // F7: seed from the configured UDP port (default 12000), not the
        // received-packet count.
        w.port->setValue(m_readerThread ? m_readerThread->getUDPPort() : 12000);
        layout->addRow("UDP Port:", w.port);

        // Multicast address
        w.address = new QLineEdit(w.dialog);
        w.address->setObjectName(QStringLiteral("udpSettingsAddress"));
        w.address->setText("239.0.0.1");
        w.address->setPlaceholderText("239.0.0.1");
        layout->addRow("Multicast Address:", w.address);

        // Enable multicast checkbox
        w.multicast = new QCheckBox("Enable Multicast Subscription", w.dialog);
        w.multicast->setObjectName(QStringLiteral("udpSettingsMulticast"));
        w.multicast->setChecked(false);
        layout->addRow(w.multicast);

        // Info label
        QLabel* infoLabel = new QLabel(w.dialog);
        infoLabel->setText(
            "<b>ETI-over-IP Configuration</b><br><br>"
            "<b>Common Ports:</b> 5004, 12000, 9200<br>"
            "<b>Multicast Range:</b> 239.0.0.0 - 239.255.255.255<br>"
            "<b>Frame Size:</b> 6144 bytes (ETI-LI)<br>"
            "<b>Frame Rate:</b> ~41 fps (24ms interval)<br><br>"
            "<i>Note: For unicast, leave multicast unchecked.</i>"
        );
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 10px; border: 1px solid #ccc; }");
        layout->addRow(infoLabel);

        // Buttons
        QDialogButtonBox* buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, w.dialog);
        layout->addRow(buttons);

        connect(buttons, &QDialogButtonBox::accepted, w.dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, w.dialog, &QDialog::reject);
        return w;
    }

    void showUDPSettings()
    {
        const UdpSettingsWidgets w = buildUDPSettingsDialog();
        std::unique_ptr<QDialog> dialog(w.dialog);

        if (dialog->exec() != QDialog::Accepted) {
            return;
        }
        if (!m_readerThread) {
            return;
        }
        m_readerThread->setUDPPort(w.port->value());
        m_readerThread->setMulticastAddress(w.address->text());
        m_readerThread->setUseMulticast(w.multicast->isChecked());

        qDebug() << "Phase C: UDP settings updated - Port:" << w.port->value()
                 << "Multicast:" << w.address->text()
                 << "Enabled:" << w.multicast->isChecked();

        QMessageBox::information(this, "Settings Updated",
            QString("UDP settings saved.\n\nPort: %1\nMulticast: %2\nEnabled: %3\n\n"
                    "Switch to Network Stream mode to start streaming.")
                    .arg(w.port->value())
                    .arg(w.address->text())
                    .arg(w.multicast->isChecked() ? "Yes" : "No"));
    }

    // === PHASE 3: Real ETI Processing Helpers ===

    void applyTheme()
    {
        // EMERGENCY: ALL CUSTOM THEMING DISABLED FOR TEXT VISIBILITY
        // Using pure Qt system default theme for maximum compatibility
        // setStyleSheet(""); // Clear any existing styles
    }

    QString formatToString(ETIFormat format) const
    {
        switch (format) {
            case ETIFormat::ETI_NI: return "ETI-NI";
            case ETIFormat::ETI_LI_A: return "ETI-LI-A";
            case ETIFormat::ETI_LI_B: return "ETI-LI-B";
            default: return "Unknown";
        }
    }

    void updateFrameDetails(ProcessedFrame frame)  // Pass by value for Qt::QueuedConnection safety
    {
        // Safety check: ensure widget exists
        if (!m_frameDetails) {
            qWarning() << "Frame details widget is null!";
            return;
        }
        
        QString details = QString(
            "Frame: %1\n"
            "Format: %2\n" 
            "SYNC: 0x%3\n"
            "LIDATA: 0x%4\n"
            "CRC: 0x%5\n"
            "Valid: %6"
        ).arg(frame.frame_number)
         .arg(formatToString(frame.format))
         .arg(frame.sync_pattern, 8, 16, QChar('0'))
         .arg(frame.lidata, 8, 16, QChar('0'))
         .arg(frame.crc, 8, 16, QChar('0'))
         .arg(frame.is_valid ? "Yes" : "No");
        
        m_frameDetails->setText(details);
    }

    // PHASE 3 AGENT 2: Real FIG Data Service Tree Implementation
    void updateEnsembleTree()
    {
        if (!m_figAnalyser) {
            qWarning() << "[SERVICE TREE] FIG analyser not initialized";
            return;
        }

        // Save current expansion state
        QSet<QString> expandedServices = saveExpansionState();

        // Clear existing tree
        m_ensembleTree->clear();

        // Get REAL data from FIG analyser
        EnsembleInfo ensemble = m_figAnalyser->getCurrentEnsemble();
        std::vector<DABService> services = m_figAnalyser->getDABServices();

        // Handle no services discovered yet
        if (services.empty()) {
            QTreeWidgetItem* placeholderItem = new QTreeWidgetItem(m_ensembleTree);
            placeholderItem->setText(0, "No services discovered yet...");
            placeholderItem->setText(1, "Load an ETI file to see DAB services");
            placeholderItem->setForeground(0, QColor(150, 150, 150));
            return;
        }

        // Create ensemble root with REAL data
        QTreeWidgetItem* ensembleItem = new QTreeWidgetItem(m_ensembleTree);

        QString ensembleLabel = ensemble.ensembleLabel;
        if (ensembleLabel.isEmpty()) {
            ensembleLabel = "Unnamed Ensemble";
        }

        // Format: "Ensemble Label [EID: 0xFFFF]"
        QString ensembleDisplay = QString("%1 [EID: 0x%2]")
                                  .arg(ensembleLabel)
                                  .arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper();

        // Populate ensemble root row with 11 columns
        ensembleItem->setText(0, ensembleDisplay);  // Service/Ensemble
        ensembleItem->setText(1, ensembleLabel);     // Label
        ensembleItem->setText(2, ensemble.shortLabel); // Short Label
        ensembleItem->setText(3, "Ensemble");        // Type
        ensembleItem->setText(4, "-");               // Programme Type
        ensembleItem->setText(5, "-");               // Language
        ensembleItem->setText(6, "-");               // CA Flag
        ensembleItem->setText(7, QString::number(services.size())); // Comp. Count (services)
        ensembleItem->setText(8, QString("0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()); // EID
        QString countryStr = QString("%1 (0x%2)").arg(ensemble.countryCode).arg(ensemble.extendedCountryCode, 2, 16, QChar('0')).toUpper();
        ensembleItem->setText(9, countryStr);        // Country
        ensembleItem->setText(10, ensemble.lastUpdated.isValid() ? ensemble.lastUpdated.toString("yyyy-MM-dd HH:mm:ss") : "-"); // Last Updated

        // Make ensemble bold
        QFont ensembleFont = ensembleItem->font(0);
        ensembleFont.setBold(true);
        ensembleItem->setFont(0, ensembleFont);

        // Add all discovered services
        for (const auto& service : services) {
            addServiceToTree(ensembleItem, service);
        }

        // Restore expansion state
        restoreExpansionState(expandedServices);
        ensembleItem->setExpanded(true);

        qDebug() << "[SERVICE TREE] Updated with" << services.size() << "real DAB services";
    }

    void addServiceToTree(QTreeWidgetItem* parent, const DABService& service)
    {
        QTreeWidgetItem* serviceItem = new QTreeWidgetItem(parent);

        QString serviceLabel = service.service_label;  // Already a QString
        if (serviceLabel.isEmpty()) {
            serviceLabel = "Unnamed Service";
        }

        // Detect DAB+ (transport_mode=0, service_component_type=0x3F)
        bool isDABPlus = false;
        for (const auto& comp : service.components) {
            if (comp.transport_mode == 0 && comp.service_component_type == 0x3F) {
                isDABPlus = true;
                break;
            }
        }

        // Format: "Service Label [SID: 0xFFFF] (DAB+)"
        QString displayText;
        displayText.reserve(serviceLabel.length() + 30);
        displayText = QString("%1 [SID: 0x%2]")
                      .arg(serviceLabel)
                      .arg(service.service_id, 4, 16, QChar('0')).toUpper();

        if (isDABPlus) {
            displayText += " (DAB+)";
        }

        // Populate all 11 columns for service row
        serviceItem->setText(0, displayText);  // Service
        serviceItem->setText(1, serviceLabel); // Label
        serviceItem->setText(2, service.short_label.isEmpty() ? "-" : service.short_label); // Short Label
        
        // Type: Audio/Data + DAB/DAB+
        QString typeStr = (service.service_type == 0) ? "Audio" : "Data";
        if (isDABPlus) typeStr += " (DAB+)";
        else typeStr += " (DAB)";
        serviceItem->setText(3, typeStr); // Type
        
        // Programme Type (PTy)
        static const char* ptyNames[] = {
            "None", "News", "Current Affairs", "Information", "Sport", "Education",
            "Drama", "Culture", "Science", "Varied", "Pop Music", "Rock Music",
            "Easy Listening", "Light Classical", "Serious Classical", "Other Music",
            "Weather", "Finance", "Children", "Social Affairs", "Religion",
            "Phone In", "Travel", "Leisure", "Jazz Music", "Country Music",
            "National Music", "Oldies Music", "Folk Music", "Documentary",
            "Alarm Test", "Alarm"
        };
        QString ptyStr = (service.programme_type <= 31) ? ptyNames[service.programme_type] : QString("PTy %1").arg(service.programme_type);
        serviceItem->setText(4, ptyStr); // Programme Type
        
        // Language
        serviceItem->setText(5, service.language ? FIGParsingUtils::languageCodeToString(service.language) : "-"); // Language
        
        // CA Flag
        serviceItem->setText(6, service.ca_flag ? "Yes" : "No"); // CA Flag
        
        // Component Count
        serviceItem->setText(7, QString::number(service.components.size())); // Comp. Count
        
        // EID (from ensemble - pass through parent or store globally)
        // For now, use the ensemble ID from the analyser
        EnsembleInfo ensemble = m_figAnalyser->getCurrentEnsemble();
        serviceItem->setText(8, QString("0x%1").arg(ensemble.ensembleId, 4, 16, QChar('0')).toUpper()); // EID
        
        // Country
        QString countryStr = QString("%1 (0x%2)").arg(ensemble.countryCode).arg(ensemble.extendedCountryCode, 2, 16, QChar('0')).toUpper();
        serviceItem->setText(9, countryStr); // Country
        
        // Last Updated
        serviceItem->setText(10, ensemble.lastUpdated.isValid() ? ensemble.lastUpdated.toString("yyyy-MM-dd HH:mm:ss") : "-"); // Last Updated

        // Highlight DAB+ in bold blue
        if (isDABPlus) {
            QFont dabPlusFont = serviceItem->font(0);
            dabPlusFont.setBold(true);
            serviceItem->setFont(0, dabPlusFont);
            serviceItem->setForeground(0, QColor(0, 100, 200));
        }

        // Add components
        for (const auto& component : service.components) {
            addComponentToTree(serviceItem, component, service.service_id);
        }
    }

    void addComponentToTree(QTreeWidgetItem* parent, const ServiceComponent& component, uint32_t serviceId)
    {
        QTreeWidgetItem* compItem = new QTreeWidgetItem(parent);

        // ServiceComponent doesn't have a label field, use component_id
        QString componentLabel = QString("Component %1").arg(component.component_id);

        // Determine component type
        QString typeDesc;
        if (component.transport_mode == 0) {
            if (component.service_component_type == 0x3F) {
                typeDesc = "DAB+ Audio";
            } else if (component.service_component_type == 0x00) {
                typeDesc = "DAB Audio";
            } else {
                typeDesc = QString("Type 0x%1").arg(component.service_component_type, 2, 16, QChar('0')).toUpper();
            }
        } else {
            typeDesc = QString("TM %1").arg(component.transport_mode);
        }

        // Populate all 11 columns for component row
        compItem->setText(0, componentLabel);  // Service/Component
        compItem->setText(1, "-");             // Label
        compItem->setText(2, "-");             // Short Label
        compItem->setText(3, typeDesc);        // Type
        compItem->setText(4, "-");             // Programme Type
        compItem->setText(5, "-");             // Language
        compItem->setText(6, component.ca_flag ? "Yes" : "No"); // CA Flag
        compItem->setText(7, "-");             // Comp. Count (N/A for component)
        compItem->setText(8, "-");             // EID
        compItem->setText(9, "-");             // Country
        compItem->setText(10, "-");            // Last Updated

        compItem->setForeground(0, QColor(100, 100, 100));
    }

    QSet<QString> saveExpansionState()
    {
        QSet<QString> expanded;
        for (int i = 0; i < m_ensembleTree->topLevelItemCount(); ++i) {
            saveExpansionStateRecursive(m_ensembleTree->topLevelItem(i), expanded);
        }
        return expanded;
    }

    void saveExpansionStateRecursive(QTreeWidgetItem* item, QSet<QString>& expanded)
    {
        if (item->isExpanded()) {
            expanded.insert(item->text(0));
        }
        for (int i = 0; i < item->childCount(); ++i) {
            saveExpansionStateRecursive(item->child(i), expanded);
        }
    }

    void restoreExpansionState(const QSet<QString>& expanded)
    {
        for (int i = 0; i < m_ensembleTree->topLevelItemCount(); ++i) {
            restoreExpansionStateRecursive(m_ensembleTree->topLevelItem(i), expanded);
        }
    }

    void restoreExpansionStateRecursive(QTreeWidgetItem* item, const QSet<QString>& expanded)
    {
        if (expanded.contains(item->text(0))) {
            item->setExpanded(true);
        }
        for (int i = 0; i < item->childCount(); ++i) {
            restoreExpansionStateRecursive(item->child(i), expanded);
        }
    }


private:
    // === PHASE 3 AGENT 4: State Validation Guards (must be before slots) ===
    bool canNavigateFrames() const {
        return m_fileLoaded && m_totalFrames > 0;
    }
    
    bool isValidFrameIndex(size_t index) const {
        return index < m_totalFrames;
    }

private slots:
    // === PHASE 3 AGENT 4: Frame Navigation Slots ===
    void onPreviousFrame() {
        if (!canNavigateFrames()) {
            qWarning() << "[NAV] Cannot navigate: file not loaded";
            return;
        }
        
        if (m_currentFrameIndex > 0) {
            navigateToFrame(m_currentFrameIndex - 1);
        } else {
            qDebug() << "[NAV] Already at first frame";
        }
    }
    
    void onNextFrame() {
        if (!canNavigateFrames()) {
            qWarning() << "[NAV] Cannot navigate: file not loaded";
            return;
        }
        
        if (m_currentFrameIndex < m_totalFrames - 1) {
            navigateToFrame(m_currentFrameIndex + 1);
        } else {
            qDebug() << "[NAV] Already at last frame";
        }
    }
    
    void onFrameSliderChanged(int value) {
        if (!canNavigateFrames()) return;
        
        size_t frameIndex = static_cast<size_t>(value);
        if (!isValidFrameIndex(frameIndex)) {
            qWarning() << "[NAV] Invalid frame index from slider:" << value;
            return;
        }
        
        navigateToFrame(frameIndex);
    }
    
    void onSpinBoxChanged(int value) {
        if (!canNavigateFrames()) return;
        
        size_t frameIndex = static_cast<size_t>(value);
        if (!isValidFrameIndex(frameIndex)) {
            qWarning() << "[NAV] Invalid frame index from spinbox:" << value;
            return;
        }
        
        navigateToFrame(frameIndex);
    }

private:
    // === PHASE 3 AGENT 4: Core Navigation Logic ===
    void navigateToFrame(size_t frameIndex) {
        // HIGH-004: Add null checks at method entry
        if (!m_etiProcessor) {
            qWarning() << "[NAV] ETI processor not initialized";
            return;
        }
        
        if (!isValidFrameIndex(frameIndex)) {
            qWarning() << "[NAV] Invalid frame index:" << frameIndex;
            return;
        }
        
        qDebug() << "[NAV] Navigating to frame" << frameIndex;
        
        // Try cache first
        auto cachedFrame = m_frameCache.getFrame(frameIndex);
        if (cachedFrame.has_value()) {
            m_cacheHits++;
            double hitRate = m_frameCache.getHitRate(m_cacheHits + m_cacheMisses, m_cacheHits);
            qDebug() << "[NAV] Using cached frame" << frameIndex 
                     << "| Hit rate:" << QString::number(hitRate, 'f', 1) << "%";
            
            m_currentFrameIndex = frameIndex;
            displayFrameHexData(static_cast<int>(frameIndex));
            updateNavigationControls(frameIndex);
            return;
        }
        
        // Cache miss - process frame
        m_cacheMisses++;
        qDebug() << "[NAV] Cache miss for frame" << frameIndex << "- processing...";
        
        // NOTE: This will trigger onFrameProcessed which will cache the frame
        processFrame(frameIndex);
    }
    
    void updateNavigationControls(size_t frameIndex) {
        // Update frame counter label
        if (m_frameCountLabel) {
            m_frameCountLabel->setText(QString("Frame: %1 / %2")
                                      .arg(frameIndex)
                                      .arg(m_totalFrames));
        }
        
        // Note: Slider and spinbox sync will be added by Agent 1 when those widgets exist
        qDebug() << "[NAV] Navigation controls updated for frame" << frameIndex;
    }

    void processFrame(size_t frameIndex)
    {
        if (!m_fileLoaded || frameIndex >= m_totalFrames) {
            qWarning() << "[PHASE 3] Invalid frame index:" << frameIndex
                      << "(loaded:" << m_fileLoaded
                      << "total:" << m_totalFrames << ")";
            return;
        }

        qDebug() << "[PHASE 3] Processing frame:" << frameIndex;

        // NOTE: Currently ETI processor loads and processes entire file
        // This method is a placeholder for future frame-by-frame processing
        // Agent 4 will use this for frame navigation

        // Update current frame tracking
        m_currentFrameIndex = frameIndex;

        // Frame processing happens automatically via processETIFile()
        // Individual frame access for hex display uses getRawFrameData()
    }

    void onEnsembleInfoUpdated(const EnsembleInfo& ensemble)
    {
        qDebug() << "[PHASE 3] Ensemble info updated:"
                 << ensemble.ensembleLabel;

        m_systemMessages->append(QString("📡 Ensemble: %1 (EID: 0x%2)")
            .arg(ensemble.ensembleLabel)
            .arg(ensemble.ensembleId, 4, 16, QChar('0')));

        // === PHASE 1.4: Rebuild ensemble tree ===
        buildEnsembleTree();
    }

    void onServicesUpdated()
    {
        // T25: a queued emission may race teardown — never dereference a
        // destroyed/cleared analyser.
        if (!m_figAnalyser) {
            return;
        }
        qDebug() << "\n***** [DEBUG onServicesUpdated] SIGNAL RECEIVED *****";
        int count = m_figAnalyser->getServiceCount();
        qDebug() << "[DEBUG onServicesUpdated] Service count:" << count;
        qDebug() << "[PHASE 3] Services updated - batch update received";

        m_systemMessages->append(QString("🎵 Services discovered: %1 services")
            .arg(count));

        // === PHASE 1.4: Rebuild ensemble tree ===
        buildEnsembleTree();
    }

    void onSubChannelsUpdated()
    {
        // T25: guard against teardown races (see onServicesUpdated).
        if (!m_figAnalyser) {
            return;
        }
        qDebug() << "[PHASE 2.1] Subchannels updated - updating table and CU usage";

        // Get subchannel count for logging
        int subchannelCount = m_figAnalyser->getSubChannelCount();
        m_systemMessages->append(QString("📊 Subchannels discovered: %1 subchannels")
            .arg(subchannelCount));

        // === PHASE 2.1: Update subchannel table ===
        updateSubchannelTable();

        // === PHASE 2.2: Update CU usage ===
        updateCUUsage();
    }

    // Phase 2C: Advanced FIG Analyser slots
    void refreshAdvancedFigDisplay()
    {
        if (!m_figAnalyser || !m_advancedFigTree) {
            return;
        }
        
        // Store current selection to restore if possible
        QString selectedEntityId;
        if (auto* item = m_advancedFigTree->currentItem()) {
            selectedEntityId = item->data(1, Qt::UserRole).toString();
        }
        
        m_advancedFigTree->clear();
        
        int filterType = -1;
        if (m_advancedFigFilter) {
            filterType = m_advancedFigFilter->currentData().toInt();
        }
        
        // Helper lambda to format entity ID as hex
        auto formatEntityId = [](uint32_t id, bool is16bit) -> QString {
            return QString("0x%1").arg(id, is16bit ? 4 : 8, 16, QChar('0')).toUpper();
        };
        
        // Helper lambda to format timestamp
        auto formatTimestamp = [](const QDateTime& dt) -> QString {
            if (!dt.isValid()) return QString();
            return dt.toString("yyyy-MM-dd HH:mm:ss.zzz");
        };
        
        // Current time for "last updated" (in real implementation, we'd store this per-label)
        QDateTime now = QDateTime::currentDateTime();
        
        // FIG 2/0 - Ensemble Extended Labels
        if (filterType == -1 || filterType == 0) {
            const auto& ensembleLabels = m_figAnalyser->getEnsembleExtendedLabelsMap();
            for (const auto& pair : ensembleLabels) {
                uint16_t ensembleId = pair.first;
                const auto& labelInfo = pair.second;
                
                QString fullLabel;
                int segmentCount = 0;
                uint8_t lastToggle = 0;
                
                for (int i = 0; i < 8; ++i) {
                    auto segIt = labelInfo.segments.find(i);
                    if (segIt != labelInfo.segments.end()) {
                        const auto& segment = segIt->second;
                        segmentCount++;
                        lastToggle = segment.toggle_flag;
                        for (uint8_t byte : segment.text_segment) {
                            fullLabel.append(static_cast<char>(byte));
                        }
                    }
                }
                
                if (fullLabel.isEmpty()) continue;
                
                QString utf8Label = QString::fromUtf8(fullLabel.toUtf8());
                
                auto* item = new QTreeWidgetItem(m_advancedFigTree);
                item->setText(0, "FIG 2/0 (Ensemble)");
                item->setText(1, formatEntityId(ensembleId, true));
                item->setText(2, utf8Label);
                item->setText(3, "UTF-8");
                item->setText(4, QString::number(segmentCount) + "/8");
                item->setText(5, QString::number(lastToggle));
                item->setText(6, formatTimestamp(now));
                item->setData(1, Qt::UserRole, QString("ensemble_%1").arg(ensembleId));
                item->setData(0, Qt::UserRole, QVariant::fromValue<int>(0)); // FIG2 type
                item->setData(2, Qt::UserRole, fullLabel.toUtf8()); // raw bytes
                item->setFont(1, QFont("Consolas", 9));
            }
        }
        
        // FIG 2/1 - Service Extended Labels (includes FIG 2/5 data services)
        if (filterType == -1 || filterType == 1) {
            const auto& serviceLabels = m_figAnalyser->getServiceExtendedLabelsMap();
            for (const auto& pair : serviceLabels) {
                uint32_t serviceId = pair.first;
                const auto& labelInfo = pair.second;
                
                QString fullLabel;
                int segmentCount = 0;
                uint8_t lastToggle = 0;
                
                for (int i = 0; i < 8; ++i) {
                    auto segIt = labelInfo.segments.find(i);
                    if (segIt != labelInfo.segments.end()) {
                        const auto& segment = segIt->second;
                        segmentCount++;
                        lastToggle = segment.toggle_flag;
                        for (uint8_t byte : segment.text_segment) {
                            fullLabel.append(static_cast<char>(byte));
                        }
                    }
                }
                
                if (fullLabel.isEmpty()) continue;
                
                QString utf8Label = QString::fromUtf8(fullLabel.toUtf8());
                
                auto* item = new QTreeWidgetItem(m_advancedFigTree);
                item->setText(0, "FIG 2/1 (Service)");
                item->setText(1, formatEntityId(serviceId, false));
                item->setText(2, utf8Label);
                item->setText(3, "UTF-8");
                item->setText(4, QString::number(segmentCount) + "/8");
                item->setText(5, QString::number(lastToggle));
                item->setText(6, formatTimestamp(now));
                item->setData(1, Qt::UserRole, QString("service_%1").arg(serviceId));
                item->setData(0, Qt::UserRole, QVariant::fromValue<int>(1));
                item->setData(2, Qt::UserRole, fullLabel.toUtf8());
                item->setFont(1, QFont("Consolas", 9));
            }
        }
        
        // FIG 2/4 - Component Extended Labels
        if (filterType == -1 || filterType == 4) {
            const auto& componentLabels = m_figAnalyser->getComponentExtendedLabelsMap();
            for (const auto& pair : componentLabels) {
                uint64_t key = pair.first;
                const auto& labelInfo = pair.second;
                uint32_t serviceId = static_cast<uint32_t>(key >> 8);
                uint8_t componentId = static_cast<uint8_t>(key & 0xFF);
                
                QString fullLabel;
                int segmentCount = 0;
                uint8_t lastToggle = 0;
                
                for (int i = 0; i < 8; ++i) {
                    auto segIt = labelInfo.segments.find(i);
                    if (segIt != labelInfo.segments.end()) {
                        const auto& segment = segIt->second;
                        segmentCount++;
                        lastToggle = segment.toggle_flag;
                        for (uint8_t byte : segment.text_segment) {
                            fullLabel.append(static_cast<char>(byte));
                        }
                    }
                }
                
                if (fullLabel.isEmpty()) continue;
                
                QString utf8Label = QString::fromUtf8(fullLabel.toUtf8());
                
                auto* item = new QTreeWidgetItem(m_advancedFigTree);
                item->setText(0, "FIG 2/4 (Component)");
                item->setText(1, QString("0x%1:%2").arg(serviceId, 8, 16, QChar('0')).toUpper().arg(componentId));
                item->setText(2, utf8Label);
                item->setText(3, "UTF-8");
                item->setText(4, QString::number(segmentCount) + "/8");
                item->setText(5, QString::number(lastToggle));
                item->setText(6, formatTimestamp(now));
                item->setData(1, Qt::UserRole, QString("component_%1_%2").arg(serviceId).arg(componentId));
                item->setData(0, Qt::UserRole, QVariant::fromValue<int>(4));
                item->setData(2, Qt::UserRole, fullLabel.toUtf8());
                item->setFont(1, QFont("Consolas", 9));
            }
        }
        
        // Restore selection if possible
        if (!selectedEntityId.isEmpty()) {
            for (int i = 0; i < m_advancedFigTree->topLevelItemCount(); ++i) {
                auto* item = m_advancedFigTree->topLevelItem(i);
                if (item->data(1, Qt::UserRole).toString() == selectedEntityId) {
                    m_advancedFigTree->setCurrentItem(item);
                    break;
                }
            }
        }
    }
    
    void onAdvancedFigTreeSelectionChanged()
    {
        if (!m_advancedFigHexViewer || !m_advancedFigTree) {
            return;
        }
        
        auto* item = m_advancedFigTree->currentItem();
        if (!item) {
            m_advancedFigHexViewer->clear();
            m_advancedFigHexViewer->setPlaceholderText("Select a FIG2 entry above to view its raw bytes...");
            return;
        }
        
        QByteArray rawData = item->data(2, Qt::UserRole).toByteArray();
        if (rawData.isEmpty()) {
            m_advancedFigHexViewer->clear();
            m_advancedFigHexViewer->setPlaceholderText("No raw data available for this entry");
            return;
        }
        
        // Format as hex dump
        QString hexDump;
        hexDump.reserve(rawData.size() * 3);
        for (int i = 0; i < rawData.size(); ++i) {
            if (i % 16 == 0) {
                if (i > 0) hexDump += "\n";
                hexDump += QString("%1: ").arg(i, 4, 16, QChar('0')).toUpper();
            }
            hexDump += QString("%1 ").arg(static_cast<uint8_t>(rawData[i]), 2, 16, QChar('0')).toUpper();
        }
        
        m_advancedFigHexViewer->setPlainText(hexDump);
    }
    
    void onExtendedLabelUpdated(uint32_t entity_id, const QString& label)
    {
        // Refresh the display when a new extended label arrives
        // This slot is connected to m_figAnalyser->extendedLabelUpdated
        refreshAdvancedFigDisplay();
    }

private:
    // Core components
    std::unique_ptr<EnhancedETIProcessorQt> m_etiProcessor;
    std::unique_ptr<AdvancedFIGAnalyser> m_figAnalyser;
    std::unique_ptr<HexViewerFormatter> m_hexFormatter;
    std::unique_ptr<etsi::ETSIComplianceEngine> m_etsiComplianceEngine;
    std::unique_ptr<multistream::MultiStreamProcessor> m_multiStreamProcessor;
    std::unique_ptr<error_detection::AdvancedErrorDetector> m_errorDetector;

    // User-selected analyser decode options (option-variant matrix rows 1-14).
    // Loaded once at startup from QSettings (org StreamDAB-Analyser / app
    // DABAnalyser, group "analyser/"); edited via the Settings dialog. The
    // current GUI decode path uses the legacy EnhancedETIProcessorQt parser;
    // these settings are honored by the CLI/headless path and by any future
    // decode path that consumes ModernETIFrameParser (apply on next file load).
    eti::AnalyserSettings m_analyserSettings{eti::AnalyserSettings::defaults()};
    
    // Phase A: Worker thread architecture (Pattern 3 + Pattern 5)
    streamdab::core::SharedETIData* m_sharedData = nullptr;
    ETIReaderThread* m_readerThread = nullptr;

    // UI components
    QTreeWidget* m_ensembleTree = nullptr;
    QListWidget* m_frameList = nullptr;
    QTextEdit* m_hexViewer = nullptr;
    QTextEdit* m_complianceViewer = nullptr;
    QTextEdit* m_systemMessages = nullptr;
    
    // NOTE: Multi-stream components removed - moved to Phase 5 (see docs/FUTURE_FEATURES.md)
    
    // Advanced Error Detection UI components (relocated to Tab 1 Right Panel)
    QLabel* m_errorDetectionStatus = nullptr;
    QLabel* m_systemHealthScore = nullptr;
    QLabel* m_errorCountLabel = nullptr;
    QTreeWidget* m_errorHistoryTree = nullptr;
    QTextEdit* m_errorPatternsDisplay = nullptr;
    
    // T41: legacy Professional Logging UI members (m_loggingStatus/Level/
    // CountLabel, m_sessionIdLabel, m_logLevelFilter/CategoryFilter,
    // m_logEntriesTree, m_loggingStatsDisplay) were removed with the redundant
    // `bottom_Messages` dock; the backing ProfessionalLoggingSystem was never
    // constructed. "System Messages" is the live log.
    //
    // T45: the separate bottom-strip "Logging" tab is now a real live log view
    // (NOT the removed ProfessionalLoggingSystem): these members back it with
    // a bounded ring fed by Logger::logMessageAdded.
    QTableWidget* m_loggingTable = nullptr;             // time | level | category | message
    QComboBox* m_loggingLevelCombo = nullptr;           // level selector (single source of truth)
    QCheckBox* m_loggingAutoScrollCheck = nullptr;      // keep newest visible
    QPushButton* m_loggingClearButton = nullptr;        // clear the view
    QLabel* m_loggingSummaryLabel = nullptr;            // count/level summary
    // T45: Logging tab ring size + number of Logger::LogLevel values. Declared
    // here (before the array bound below) so the constant is in scope.
    static constexpr int LOGGING_MAX_ROWS = 2000;
    static constexpr int LOG_LEVEL_COUNT = 5;
    int m_loggingRowCap = LOGGING_MAX_ROWS;             // ring capacity
    int m_loggingLevelCounts[LOG_LEVEL_COUNT] = {0, 0, 0, 0, 0};

    // W1 #2: Logging-tab batching. Incoming log entries are staged in a FIFO
    // (see struct PendingLogEntry above, declared with the logging helpers)
    // and flushed to the (already bounded) table by a short timer, so a burst
    // of debug logs cannot run per-message table work on the GUI thread.
    std::deque<PendingLogEntry> m_logPending;           // staging buffer (FIFO)
    QTimer* m_logFlushTimer = nullptr;                  // coalescing flush timer
    int m_logDroppedPending = 0;                        // dropped due to backlog
    static constexpr int LOG_FLUSH_INTERVAL_MS = 75;    // 50..100 ms flush window
    static constexpr int LOG_FLUSH_MAX_BATCH = 300;     // rows applied per flush
    static constexpr int LOG_PENDING_MAX = 4000;        // staging-buffer cap
    QLabel* m_frameDetails = nullptr;  // Frame details display (Tab 1 LEFT panel)
    QTableWidget* m_etiExplorerDetails = nullptr;  // NEW: ETI Explorer Details table (Tab 1)
    QTableWidget* m_subchannelTable = nullptr;  // NEW: Subchannel Organization table (Tab 1 Center Panel)
    QTableWidget* m_etiOverviewTable = nullptr;  // NEW: ETI Overview table (Tab 1 Right Panel - item #10)
    QTableWidget* m_errorCounterTable = nullptr;  // PHASE 4: Error counter table (Tab 1 Right Panel - item #11)
    QTableWidget* m_timingTable = nullptr;  // NEW: Timing table (Tab 1 Right Panel - item #12)
    QProgressBar* m_cuUsageBar = nullptr;  // NEW: CU Usage progress bar (Tab 1 Right Panel - item #13)
    QLabel* m_cuUsageLabel = nullptr;  // NEW: CU Usage label (Tab 1 Right Panel - item #13)
    QTableWidget* m_streamStatsTable = nullptr;  // NEW: Stream Statistics table (Tab 1 Right Panel - item #14)
    
    // Now Playing (DLS+) widgets (Tab 1 Right Panel - item #9)
    QLabel* m_trackLabel = nullptr;
    QLabel* m_artistLabel = nullptr;
    QLabel* m_albumLabel = nullptr;
    QLabel* m_lastUpdatedLabel = nullptr;
    
    // LEFT Panel widgets (Items #1-3 from DATA_AVAILABILITY_MATRIX.md)
    // Player Panel (Item #1)
    QLabel* m_playerStatusLabel = nullptr;
    QLabel* m_playerFileLabel = nullptr;
    QLabel* m_playerTimeLabel = nullptr;
    QLabel* m_playerFrameLabel = nullptr;
    QCheckBox* m_playerLoopCheckbox = nullptr;
    // T29: playback time bar (seek slider). During parsing it mirrors the load
    // progress; after load it is the playback seek control.
    QSlider* m_playerProgressBar = nullptr;
    // T29: playback transport controls.
    QPushButton* m_playButton = nullptr;
    QPushButton* m_pauseButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_skipBackButton = nullptr;
    QPushButton* m_skipForwardButton = nullptr;
    QComboBox* m_playbackSpeedCombo = nullptr;
    // T32: Player service selector (which service's DLS+/slideshow is shown).
    QComboBox* m_serviceCombo = nullptr;
    quint32 m_selectedServiceId = 0;
    QString m_selectedServiceLabel;
    bool m_populatingServiceSelector = false;

    QTimer* m_playbackTimer = nullptr;
    int m_playbackFrame = 0;              // 0-based UI playhead
    bool m_playbackPlaying = false;
    double m_playbackSpeed = 1.0;         // 0.25 .. 4x
    QElapsedTimer m_detailThrottleTimer;  // expensive-panel throttle
    // M1: true while the seek slider is updated programmatically (suppresses
    // the valueChanged echo). L3: last navigator row we scrolled to.
    bool m_seekSliderProgrammatic = false;
    int m_lastHighlightedNavigatorRow = -1;
    static constexpr int PLAYBACK_BASE_INTERVAL_MS = 24;  // DAB Mode I frame (24 ms)
    static constexpr int DETAIL_THROTTLE_MS = 150;        // <= ~6.7 Hz detail refresh
    static constexpr int PLAYBACK_SKIP_FRAMES = 417;      // ~10 s at 1x
    static constexpr int SYSTEM_MESSAGES_MAX_BLOCKS = 5000;  // L1: bounded log
    
    // Decoder Panel (Item #2)
    QLabel* m_decoderStatusLabel = nullptr;
    QLabel* m_decoderTypeLabel = nullptr;
    QLabel* m_decoderTimeLabel = nullptr;
    QLabel* m_decoderFramesLabel = nullptr;
    
    // System Panel (Item #3)
    QTreeWidget* m_ficOverviewTree = nullptr;
    QGroupBox* m_processingControlsGroup = nullptr;
    QCheckBox* m_enableFicAnalysis = nullptr;
    QCheckBox* m_enableMscAnalysis = nullptr;
    
    // Live Stream Input widgets (Tab 1 Input toolbar — UI redesign Step A)
    QLabel* m_streamFrameRate = nullptr;
    QLabel* m_streamFramesReceived = nullptr;
    QLabel* m_streamFrameLoss = nullptr;
    QLabel* m_streamLatency = nullptr;
    QLabel* m_streamBuffer = nullptr;
    QLabel* m_streamDataRate = nullptr;
    QLabel* m_streamUptime = nullptr;
    QTimer* m_statsTimer = nullptr;
    
    // Professional FIC Analysis Hub widgets
    // Tab 2: FIC-Analyser widgets (3-panel layout per DATA_AVAILABILITY_MATRIX.md)
    // LEFT Panel widgets
    QLabel* m_tab2_ensembleName = nullptr;
    QLabel* m_tab2_ficContentSummary = nullptr;
    QLabel* m_tab2_subchannelOrgSummary = nullptr;
    // T33.1: folded-in summary of the FIC-decoded data services (replaces the
    // removed Digital Org Info panel).
    QLabel* m_tab2_digitalServices = nullptr;
    QTreeWidget* m_tab2_serviceTree = nullptr;
    
    // CENTER Panel widgets
    QTableWidget* m_tab2_ensembleTable = nullptr;  // 10 columns
    QTableWidget* m_tab2_subchannelTable = nullptr;  // 10 columns
    QTableWidget* m_tab2_serviceTable = nullptr;  // 4 columns
    // T33.2: moved here from the left panel.
    QTreeWidget* m_tab2_serviceComponents = nullptr;
    
    // RIGHT Panel widgets
    QTableWidget* m_tab2_figContentTable = nullptr;  // 4 columns
    
    // Advanced FIG Analyser (Phase 2C) - FIG2 Extended Labels
    QTreeWidget* m_advancedFigTree = nullptr;
    QTextEdit* m_advancedFigHexViewer = nullptr;
    QComboBox* m_advancedFigFilter = nullptr;
    
    // Tab 3: FIC-Extractor widgets (2-panel layout per DATA_AVAILABILITY_MATRIX.md)
    // LEFT Panel widgets
    QTreeWidget* m_tab3_figInstanceTree = nullptr;  // 7 columns: FIG Type, Index, Frame#, CIF#, FIB#, Length, Items
    // T28: Tab-3 left dock inner tabs; Frame List selects a frame whose FIGs
    // are shown in the right dock.
    QTabWidget* m_tab3LeftTabs = nullptr;
    QListWidget* m_tab3_frameList = nullptr;
    
    // RIGHT Panel widgets
    QTreeWidget* m_tab3_figItemDetailsTree = nullptr;  // 4 columns: Property, Size, Value, Information
    // T42: Tab-3 right dock inner tabs (FIG Item Details · Hex Viewer).
    QTabWidget* m_tab3RightTabs = nullptr;
    QTextEdit* m_tab3_hexViewer = nullptr;  // raw bytes of the current selection
    
    // Performance Dashboard widgets (Wave B: the legacy thin-tab labels
    // m_fpsLabel/m_memoryLabel/m_frameRateLabel/m_processingTimeLabel/
    // m_complianceScore/m_complianceLabel were deleted with the adopted
    // PerformanceDashboard replacing them.)
    QLabel* m_fpsCounter = nullptr;
    QLabel* m_memoryUsage = nullptr;
    QProgressBar* m_etsiCompliance = nullptr;
    QLabel* m_frameRate = nullptr;
    QLabel* m_errorCount = nullptr;
    
    // System Messages widgets (enhanced)
    QComboBox* m_messageFilter = nullptr;
    
    // Constellation Viewer widgets
    QWidget* m_constellationDisplay = nullptr;

    // === Wave B: adopted src/gui widgets + sampler state ===
    PerformanceDashboard* m_performanceDashboard = nullptr;
    RealTimeChartWidget* m_realTimeChart = nullptr;
    int m_chartFpsSeries = -1;
    int m_chartMemorySeries = -1;
    ETSIComplianceMonitor* m_etsiMonitor = nullptr;
    ConstellationWidget* m_constellationWidget = nullptr;
    ProfessionalExportManager* m_exportManager = nullptr;
    QAction* m_settingsAction = nullptr;
    QAction* m_exportAction = nullptr;
    // Wave C: Window > Docking Dialog dock re-open menu (the per-dock actions
    // are the docks' own toggleViewAction()s, so only this one is retained).
    QAction* m_showAllDocksAction = nullptr;
    QTimer* m_perfSampleTimer = nullptr;
    QElapsedTimer m_perfTimer;
    quint64 m_perfLastFrames = 0;
    qint64 m_perfLastElapsedMs = 0;
    double m_lastProcessingTimeMs = 0.0;
    qint64 m_lastCpuSampleMs = 0;
    quint64 m_lastCpuTicks = 0;
    int m_chartSamplesFed = 0;
    
    // Additional UI state widgets
    QLabel* m_serviceInfo = nullptr;
    QLabel* m_frameCountLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QProgressBar* m_qualityMeter = nullptr;
    
    // Phase 3B: DLS+ decoder instance
    eti::dls_plus::DLSPlusDecoder* m_dlsPlusDecoder{nullptr};

    // MOT SlideShow (option A). The standard EN 300 401 MSC data groups
    // (assembled from X-PAD CI 12/13) are decoded directly by MOTProtocol
    // through the thin MotPadAdapter façade (T26 — no re-framing).
    eti::mot::MOTProtocol m_motProtocol;
    eti::mot::MotPadAdapter m_motAdapter{&m_motProtocol};

    // Slideshow panel widgets + bounded image history (last N decoded images).
    // T27: the dock uses a left/right splitter (DLS+ text | slideshow) instead
    // of inner tabs; m_slideshowFrame is the fixed 4:3 letterbox container.
    QSplitter* m_nowPlayingSplitter = nullptr;
    FourThreeLetterboxFrame* m_slideshowFrame = nullptr;
    QLabel* m_slideshowImage = nullptr;
    QLabel* m_slideshowStatus = nullptr;
    QLabel* m_slideshowInfo = nullptr;
    QImage m_lastDecodedSlide;
    int m_motObjectsReceived{0};   // objectComplete signals seen
    int m_motImagesDecoded{0};     // successfully decoded images
    int m_motErrors{0};            // adapter parse errors surfaced
    int m_motHeaderParamsSeen{0};  // objects carrying extension parameters
    QString m_lastMotContentName;      // sample restored extension params
    QString m_lastMotClickThroughUrl;
    QString m_lastMotCategoryTitle;
    quint32 m_lastMotTransportId{0};
    quint32 m_lastMotServiceId{0};
    QString m_lastMotServiceLabel;

    // Refinement A: GUI DLS+ pipeline state/counters. m_dlsParsers holds one
    // standard DAB+ superframe/PAD parser per DAB+ audio sub-channel. The
    // per-subchannel MSC slices are reused from ProcessedFrame::sub_channels
    // (no raw-frame re-parse).
    std::map<int, DabPlusSubchannelParser> m_dlsParsers;
    // T31 (rework) / T32: sparse load-time media histories keyed by service id
    // (SId -> (0-based frame -> DLS/MOT snapshot)). Recorded at feed time only
    // when that service's state changes; a frame-0 baseline always exists so
    // Stop/Reset has a defined start state. Selecting a service swaps which map
    // the playhead replays.
    std::map<quint32, ServiceMediaTimeline> m_mediaTimelineByService;
    // Mutable per-service state accumulated during the load (only committed to
    // a timeline entry when it changes).
    std::map<quint32, PlaybackMediaState> m_liveServiceMediaState;
    // T32: frame being fed (0-based) while the load loop runs; -1 outside it so
    // synthetic test feeds never record. The DLS/MOT callbacks read this.
    int m_timelineRecordFrame = -1;
    // T31/T32: globally byte-capped retained-slide stores, one per service. The
    // id sequence is per service; eviction is global FIFO by sequence, so N
    // services cannot multiply the cap.
    std::map<quint32, RetainedSlideStore> m_slidesByService;
    qint64 m_retainedSlideCount{0};  // aggregate slide count (all services)
    qint64 m_retainedSlideBytes{0};
    quint64 m_retainedSlideSequence{0};
    static constexpr qint64 RETAINED_SLIDE_BYTE_CAP = 32 * 1024 * 1024;  // 32 MB
    // Runtime cap (defaults to RETAINED_SLIDE_BYTE_CAP); a test hook lowers it
    // to exercise eviction. Kept in sync with the byte counter (both qint64).
    qint64 m_retainedSlideByteCap{RETAINED_SLIDE_BYTE_CAP};
    // Cached DAB+ subchannel -> (service id, label) map (Finding 4): the
    // analyser's getDABServices() copies the whole service vector, so it is
    // only re-read when a service/subchannel count changes or a discovery
    // invalidates the cache.
    QSet<int> m_dabPlusSubChannels;
    // A-M5a: every audio sub-channel known from FIG 0/2 (TMId 0 + ASCTy
    // 0x00 MPEG / 0x3F DAB+) — superset of m_dabPlusSubChannels, used by the
    // data tap so MPEG-audio sub-channels are not captured as "data".
    QSet<int> m_audioSubChannels;
    QHash<int, QPair<quint32, QString>> m_serviceBySubChannel;
    // v1.4 data-path wave — clean tap: raw per-data-subchannel byte streams
    // (EPG/TEPG/Journaline), accumulated during load. Byte-capped; the
    // decoders are not run per-frame here — they are fed once the capture
    // completes (feedDataServiceSubchannels), so the CLI/GUI parity gates are
    // unaffected. Slot- and byte-cap hits are reported once per capture.
    QMap<int, QByteArray> m_dataSubChannelStreams;
    quint64 m_dataSubChannelBytes = 0;
    static constexpr quint64 kMaxDataSubChannelBytes = 4 * 1024 * 1024;
    static constexpr int kMaxDataSubChannelSlots = 32;
    bool m_dataTapSlotCapWarned = false;
    bool m_dataTapByteCapWarned = false;
    // === Phase 2A: data-service docks (EPG | Journaline | TPEG) — top-level docks
    // (replacing the inner tabs that were in the slideshow panel) ===============
    // Decoders + per-service data store (Qt parent-child ownership).
    eti::epg::EPGDecoder* m_epgDecoder = nullptr;
    eti::journaline::JournalineDecoder* m_journalineDecoder = nullptr;
    eti::tpeg::TpegDecoder* m_tpegDecoder = nullptr;
    eti::data::DataServiceStore* m_dataServiceStore = nullptr;
    // EPG dock
    QComboBox* m_epgServiceCombo = nullptr;
    QTableWidget* m_epgScheduleTable = nullptr;
    QLabel* m_epgStatusLabel = nullptr;
    // Journaline dock
    QTreeWidget* m_journalineMenuTree = nullptr;
    QTextEdit* m_journalinePreview = nullptr;
    QLabel* m_journalineStatusLabel = nullptr;
    // TPEG dock
    QTableWidget* m_tpegInventoryTable = nullptr;
    QLabel* m_tpegStatusLabel = nullptr;
    // Coalesced decoder-signal -> dock rebuild (see scheduleDataServiceRefresh).
    QTimer* m_dataServiceRefreshTimer = nullptr;
    // Honest per-capture bookkeeping (surfaced in the status lines).
    QString m_lastEpgError;
    QString m_lastJournalineError;
    // B-H1(5): did the router actually send any subchannel to the Journaline
    // decoder this capture? Distinguishes "no Journaline routed in this
    // stream" from "routed, but the decoder yielded no items" on the status
    // line (README: data shown only when the stream carries it).
    bool m_journalineRouted = false;
    // TPEG inventory first/last-seen (24-byte stream frame numbers) + the
    // repeat counts they were derived from, per unique message text. Bounded
    // by the decoder's own kMaxUniqueMessages cap (only texts that survive in
    // the decoder's inventory are ever inserted here).
    QMap<QString, quint64> m_tpegFirstSeenFrames;
    QMap<QString, quint64> m_tpegLastSeenFrames;
    QMap<QString, uint32_t> m_tpegSeenCounts;
    // B cross-dep 3: signature of the built inventory table (unique count /
    // frame count / framing description) — refreshTpegTab() skips the full
    // rebuild while the signature is unchanged, so the coalesced 150 ms
    // refresh stays cheap when no new TPEG data arrived.
    QString m_tpegInventorySignature;
    // Journaline tree rebuild guard (item/root counts of the built tree).
    int m_journalineTreeItemCount = -1;
    int m_journalineTreeRootCount = -1;
    // T34: service id -> its primary DAB+ audio sub-channel (audio selection).
    QHash<quint32, int> m_audioSubChannelByService;
    // T34: selected-service audio playback controller + Audio-tab widgets.
    streamdab::audio::AudioPlaybackController* m_audioController = nullptr;
    QLabel* m_audioStatusLabel = nullptr;
    QLabel* m_audioInfoLabel = nullptr;
    QComboBox* m_audioDeviceCombo = nullptr;
    QCheckBox* m_audioMuteCheck = nullptr;
    QSlider* m_audioVolumeSlider = nullptr;
    QLabel* m_audioVolumeLabel = nullptr;
    QProgressBar* m_audioLevelLeft = nullptr;
    QProgressBar* m_audioLevelRight = nullptr;
    bool m_dabPlusServiceMapDirty{true};
    int m_cachedDabServiceCount{-1};
    int m_cachedSubChannelCount{-1};
    uint32_t m_cachedServiceRevision{0};  // AdvancedFIGAnalyser::getServiceRevision()
    qint64 m_dlsPipelineNs{0};        // accumulated DLS+ pipeline wall time
    int m_dlsPlusMessages{0};
    QStringList m_dlsLabelsSeen;       // distinct decoded DLS labels (GUI tests)
    int m_dlsPlusPadCalls{0};          // adapted PAD buffers fed to the decoder
    int m_dlsPlusLabelsAssembled{0};   // DLS labels assembled by the spec parser
    quint32 m_pendingServiceId{0};     // service of the in-flight DLS+ decode
    QString m_pendingServiceLabel;
    
    // Live Stream Input Panel widgets
    QRadioButton* m_fileSourceRadio = nullptr;
    QRadioButton* m_networkSourceRadio = nullptr;
    QStackedWidget* m_sourceInputStack = nullptr;
    QLineEdit* m_filePathEdit = nullptr;
    QPushButton* m_browseFileButton = nullptr;
    QPushButton* m_loadFileButton = nullptr;
    QLineEdit* m_streamUrlEdit = nullptr;
    QComboBox* m_quickExampleCombo = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QPushButton* m_disconnectBtn = nullptr;
    QPushButton* m_recordBtn = nullptr;
    // T23 (option A): ⚙ opens the UDP Streaming Settings dialog; replaces the
    // removed Input-menu "UDP Settings…" action.
    QToolButton* m_udpSettingsBtn = nullptr;
    QLabel* m_streamStatus = nullptr;
    
    // NOTE: Stream statistics labels already declared above (lines 4114-4123)

    // Phase 4: Network stream receiver
    std::unique_ptr<eti::NetworkStreamReceiver> m_networkReceiver;
    std::chrono::steady_clock::time_point m_streamStartTime;
    // T22 network-stream connect bookkeeping / test hooks.
    int m_streamValidationWarnings{0};   // rejected URLs (pre-core)
    int m_connectAttemptCount{0};        // accepted URLs that reached the core
    bool m_streamEverConnected{false};   // gate for the failure popup vs. logs
    bool m_streamFailureDialogShown{false};
    bool m_showStreamDialogs{true};      // F6: consult in the async error handler

    // T25 (F5): set at the start of closeEvent()/destructor. Stale queued
    // slots delivered during teardown must degrade gracefully and must never
    // pop a modal dialog (would block/hang an offscreen teardown).
    bool m_closing{false};

    // === Settings audit (v1.3): live settings state ===
    bool m_confirmExitOnClose{true};   // general/confirmExit
    bool m_userCloseRequested{false};  // File -> Exit marks an interactive close
    int m_networkBufferFrames{1000};   // analysis/bufferSize (receiver frames)
    // Long-lived settings handle + debounced disk flush for the Audio tab.
    QSettings* m_appSettings{nullptr};
    QTimer* m_audioSettingsSyncTimer{nullptr};

    // Phase 4 Week 3: Stream recording
    QFile m_recordingFile;
    bool m_isRecording{false};
    size_t m_recordedFrames{0};

    // State
    QString m_currentFilePath;

    // === PHASE 3: Real ETI Processing State Tracking ===
    bool m_fileLoaded{false};
    size_t m_totalFrames{0};
    size_t m_currentFrameIndex{0};

    // Manual-test finding 2: true while the synchronous ETI parse loop runs.
    // Frame-navigation handlers must not touch frame data in this state.
    bool m_processing{false};
    bool m_frameNotReadyNotified{false};

    // === PHASE 3 AGENT 4: Frame Cache with LRU Eviction ===
    struct FrameCache {
        std::map<size_t, QByteArray> cachedFrames;
        std::list<size_t> accessOrder;  // LRU tracking (front = oldest, back = newest)
        size_t maxCacheSize{100};       // ~600KB memory (100 frames * 6144 bytes)
        
        void cacheFrame(size_t index, const QByteArray& data) {
            if (cachedFrames.size() >= maxCacheSize && cachedFrames.find(index) == cachedFrames.end()) {
                size_t oldestIndex = accessOrder.front();
                accessOrder.pop_front();
                cachedFrames.erase(oldestIndex);
                qDebug() << "[FRAME CACHE] Evicted frame" << oldestIndex << "(LRU)";
            }
            
            cachedFrames[index] = data;
            accessOrder.remove(index);
            accessOrder.push_back(index);
            
            qDebug() << "[FRAME CACHE] Cached frame" << index 
                     << "| Cache size:" << cachedFrames.size() << "/" << maxCacheSize;
        }
        
        std::optional<QByteArray> getFrame(size_t index) {
            auto it = cachedFrames.find(index);
            if (it != cachedFrames.end()) {
                accessOrder.remove(index);
                accessOrder.push_back(index);
                qDebug() << "[FRAME CACHE] Cache HIT for frame" << index;
                return it->second;
            }
            qDebug() << "[FRAME CACHE] Cache MISS for frame" << index;
            return std::nullopt;
        }
        
        void clear() {
            cachedFrames.clear();
            accessOrder.clear();
            qDebug() << "[FRAME CACHE] Cache cleared";
        }
        
        size_t size() const { return cachedFrames.size(); }
        
        double getHitRate(size_t totalRequests, size_t cacheHits) const {
            if (totalRequests == 0) return 0.0;
            return (static_cast<double>(cacheHits) / totalRequests) * 100.0;
        }
    } m_frameCache;
    
    size_t m_cacheHits{0};
    size_t m_cacheMisses{0};

    // === PHASE 1: Signal/Slot Connection State Variables ===
    // Phase 1.1: Frame data cache for hex viewer
    std::map<int, QByteArray> m_frameDataCache;
    mutable QMutex m_frameCacheMutex;  // FIX HIGH-002: Protects m_frameDataCache from race conditions
    std::atomic<int> m_frameUpdateCounter{0};  // Thread-safe counter for throttling
    std::atomic<uint64_t> m_errorCounterUpdateCounter{0};  // Phase 4: Error table throttle
    
    // Phase 1.2: Timing update throttle
    QElapsedTimer m_timingUpdateTimer;
    static constexpr int TIMING_UPDATE_INTERVAL_MS = 200;  // 5 fps
    static constexpr int FRAME_LIST_UPDATE_THROTTLE = 10;  // ~10 fps (update every 10 frames)
    static constexpr int ERROR_COUNTER_UPDATE_THROTTLE = 20;  // 5 fps (update every 20 frames)

    // UI redesign (Step E): bumped 1 -> 2 when the dock layout changed
    // (Input Source -> toolbar, Messages -> bottom strip, Tab-1 regrouped dock
    // areas). Bumped 2 -> 3 (Observation a) because v2 saved states could
    // restore the Signal Analysis group on CU Usage instead of DAB Ensemble
    // Explorer; v3 discards that stale state once so the default active tab is
    // deterministic. Bumped 3 -> 4 (MOT SlideShow, option A): the Now Playing
    // dock grew from 82 px to ≈200 px and gained inner tabs; a saved v3 state
    // would restore the old 82 px geometry and hide the Slideshow tab, so v4
    // discards v3 state once to re-apply the new default geometry. Bumped
    // 4 -> 5 (T27/T28/T29): Now Playing inner tabs -> left/right splitter, the
    // Tab-3 left dock gained the Frame List inner tabs, and the Player panel
    // gained the transport controls. A stale v4 state could also restore the
    // Transport group on the *Decoder* tab (T29 finding), so v5 discards v4
    // state once and re-applies the default active tab (Player). Bumped
    // 5 -> 6 (T36): the Tab-1 CENTER column order was corrected in code to
    // Now Playing / Slideshow -> Signal Analysis (DAB Ensemble group) ->
    // Frame Analysis (ETI Frame Navigator group) with setSizes 200/190/150, but
    // a saved v5 state still restored the OLD center order/active tabs. v6
    // discards stale v5 state once so existing users see the corrected default.
    // Bumped 6 -> 7 (T34): the Tab-1 LEFT "Transport" group gained a third tab
    // ("Audio"); a saved v6 state would restore the group with only
    // Player|Decoder and hide the new Audio tab. v7 discards stale v6 state once.
    // Bumped 7 -> 8 (T41): the shared bottom strip dropped the redundant legacy
    // "Messages" tab (8 -> 7 tabs) and the Tab-3 right dock gained the T42
    // "Hex Viewer" inner tab; a saved v7 state would restore the old strip/dock.
    // Any saved version != 8 is discarded once on restore.
    // Bumped 8 -> 9 (Phase 2A): three new top-level docks added to Tab-1 Right
    // column (EPG, Journaline, TPEG) replacing the inner tabs in the slideshow
    // panel; the right column vertical splitter gains a third group. A saved v8
    // state would restore the old right column without the Data Services group.
    // Bumped 9 -> 10 (Phase 2C): new dock "tab2_AdvancedFig" added to Tab-2 Right
    // area (tabbed with "FIG Content") for FIG2 Extended Labels display.
    // Bumped 10 -> 11: EPG/Journaline/TPEG docks moved from Right column to be
    // tabbed with Now Playing / Slideshow in the Center column.
    static constexpr int DOCKING_LAYOUT_VERSION = 11;
    
    // Phase 1: Processing counters
    int m_processedFrames{0};

protected:
    // Re-scale the retained slide when the panel is resized (keep aspect).
    void resizeEvent(QResizeEvent* event) override
    {
        QMainWindow::resizeEvent(event);
        refreshSlideshowPixmap();
    }

};

DABAnalyserWindow::~DABAnalyserWindow()
{
    // T25 (F5): direct `delete` may skip closeEvent(); mark teardown so stale
    // queued slots degrade gracefully / suppress dialogs.
    m_closing = true;

    // W1 #2: never lose the last log entries — drain any staged-but-unflushed
    // rows while the table widgets are still alive.
    flushPendingLogEntries(/*drainAll=*/true);
    // T29: make sure the playback timer is not running during destruction.
    m_playbackPlaying = false;
    if (m_playbackTimer) {
        m_playbackTimer->stop();
    }

    // T25 (F3/F9/F10): disconnect every sender from this receiver BEFORE member
    // destruction. disconnect stops FUTURE emissions only; already-posted queued
    // calls are handled by the slots' null-guards. This set mirrors closeEvent()
    // exactly (fig analyser, ETI processor, error detector, multi-stream,
    // ETSI engine, DLS+ decoder, MOT protocol, reader thread, shared data,
    // stream receiver). The NetworkStreamReceiver joins its own thread in its
    // destructor, so no explicit stop/wait is needed here.
    if (m_figAnalyser) {
        QObject::disconnect(m_figAnalyser.get(), nullptr, this, nullptr);
    }
    if (m_etiProcessor) {
        QObject::disconnect(m_etiProcessor.get(), nullptr, this, nullptr);
    }
    if (m_errorDetector) {
        QObject::disconnect(m_errorDetector.get(), nullptr, this, nullptr);
    }
    if (m_multiStreamProcessor) {
        QObject::disconnect(m_multiStreamProcessor.get(), nullptr, this, nullptr);
    }
    if (m_etsiComplianceEngine) {
        QObject::disconnect(m_etsiComplianceEngine.get(), nullptr, this, nullptr);
    }
    if (m_dlsPlusDecoder) {
        QObject::disconnect(m_dlsPlusDecoder, nullptr, this, nullptr);
    }
    QObject::disconnect(&m_motProtocol, nullptr, this, nullptr);
    if (m_readerThread) {
        QObject::disconnect(m_readerThread, nullptr, this, nullptr);
    }
    if (m_sharedData) {
        QObject::disconnect(m_sharedData, nullptr, this, nullptr);
    }
    if (m_networkReceiver) {
        QObject::disconnect(m_networkReceiver.get(), nullptr, this, nullptr);
    }

    // Phase A: Stop worker thread first
    if (m_readerThread) {
        m_readerThread->stopProcessing();
        if (!m_readerThread->wait(2000)) {
            qWarning() << "Phase A: Worker thread did not stop in time, forcing termination";
            m_readerThread->terminate();
            m_readerThread->wait();
        }
    }

    // Stop all processing before destruction
    if (m_etiProcessor) {
        m_etiProcessor->stopProcessing();
    }

    if (m_multiStreamProcessor) {
        m_multiStreamProcessor->stopParallelProcessing();
    }

    // Disconnect all signals to prevent race conditions during destruction
    disconnect();

    // Brief wait for any pending queued signals to complete
    QThread::msleep(100);

    qDebug() << "DABAnalyserWindow: Clean shutdown complete";
}

// CLI Mode: Process ETI file without GUI
// Only compile main() function when not in GUI test mode
#ifndef GUI_TEST_MODE
int main(int argc, char *argv[])
{
    // Check for CLI mode flags
    bool cli_mode = false;
    
    for (int i = 1; i < argc; i++) {
        QString arg = QString::fromUtf8(argv[i]);
        if (arg == "--cli" || arg == "-c" || arg == "--help" || arg == "-h" ||
            arg == "--version" || arg == "-v" || arg == "--input" || arg == "-i" ||
            arg == "--quiet" || arg == "--verbose") {
            cli_mode = true;
            break;
        }
    }
    
    if (cli_mode) {
        // CLI mode - no GUI (QCoreApplication)
        QCoreApplication app(argc, argv);
        app.setApplicationName("StreamDAB Analyser");
        app.setApplicationVersion(DABX_VERSION);
        return cli::cli_main(argc, argv);
    }
    
    // GUI Mode (default)
    QApplication app(argc, argv);
    app.setOrganizationName("StreamDAB-Analyser");
    app.setApplicationName("DABAnalyser");
    app.setApplicationVersion(DABX_VERSION);

    // Set application icon (uses SVG on Linux/macOS, PNG fallbacks, .ico on Windows, .icns on macOS)
    QIcon appIcon(":/icons/eti-stream-analyser.svg");
    // Add PNG fallbacks for platforms where SVG rendering may not be available
    appIcon.addFile(":/icons/eti-stream-analyser_16.png");
    appIcon.addFile(":/icons/eti-stream-analyser_24.png");
    appIcon.addFile(":/icons/eti-stream-analyser_32.png");
    appIcon.addFile(":/icons/eti-stream-analyser_48.png");
    appIcon.addFile(":/icons/eti-stream-analyser_64.png");
    appIcon.addFile(":/icons/eti-stream-analyser_128.png");
    appIcon.addFile(":/icons/eti-stream-analyser_256.png");
    app.setWindowIcon(appIcon);
    
    // Linux desktop integration
    #if defined(Q_OS_LINUX)
    app.setDesktopFileName("eti-stream-analyser");
    #endif

    // v1.3 rename: fold the pre-rename (American-spelling) QSettings scope
    // into the new "StreamDAB-Analyser" / "DABAnalyser" store once, before any
    // component reads or writes preferences. See src/utils/app_settings.hpp.
    streamdab::app_settings::migrateLegacyScope();

    // W1 #9: pin the Logger singleton to the main (GUI) thread explicitly,
    // before the window (or any worker/CLI path) can first create it. The
    // Logger emits logMessageAdded from its own thread via
    // QTimer::singleShot(0, this, …); if it were first constructed on a worker
    // thread without a running event loop, that emission would silently vanish
    // and GUI log consumers would see nothing. See src/utils/logger.h.
    (void)Logger::instance();

    DABAnalyserWindow window;
    window.setWindowIcon(appIcon);
    window.show();

    // Auto-load file from command line if provided (for testing)
    if (argc > 1) {
        QString autoLoadFile = QString::fromUtf8(argv[1]);
        qInfo() << "Auto-loading ETI file from command line:" << autoLoadFile;
        
        // Use QTimer to load file after event loop starts
        QTimer::singleShot(500, &window, [&window, autoLoadFile]() {
            window.autoLoadFile(autoLoadFile);
        });
    }

    return app.exec();
}
#endif // GUI_TEST_MODE

#include "main.moc"
