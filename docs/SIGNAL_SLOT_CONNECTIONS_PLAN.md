# Signal/Slot Connections Plan - GUI_PATTERNS.md Compliance

> **Created:** 2025-10-26  
> **Purpose:** Map all widget connections to proper signal/slot patterns per GUI_PATTERNS.md  
> **Status:** ⚠️ INCOMPLETE - Many new widgets lack proper connections

---

## Current Architecture (Existing)

### Pattern 3: Worker Thread + Event Loop ✅
- **ETIReaderThread**: Dedicated thread for ETI frame processing (exists but needs more signals)
- **EnhancedETIProcessorQt**: Main ETI processor with signals (exists)
- **AdvancedFIGAnalyzer**: FIG analysis processor with signals (exists)

### Pattern 5: Shared Data Model ✅
- **SharedETIData**: Thread-safe data container (exists)

### Pattern 7: Multiple Specialized Signals ⚠️ PARTIAL
- Some signals exist (frameProcessed, figDiscovered, etc.)
- Need more specialized signals for new widgets

### Pattern 8: Update Throttling ⚠️ PARTIAL
- Some throttling in ETIReaderThread
- Need throttling for high-frequency widgets

---

## TAB 1: DAB Ensemble & ETI Analysis - Connection Requirements

### LEFT Panel Widgets

#### 1. Player Panel (Item #1)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_playerStatusLabel | File loading state | N/A (set directly) | On file open/close | ⚠️ TODO |
| m_playerFileLabel | File path | N/A (set directly) | On file open | ⚠️ TODO |
| m_playerTimeLabel | Frame timing | EnhancedETIProcessorQt::frameProcessed | ~5 fps (throttled) | ⚠️ TODO |
| m_playerFrameLabel | Frame count | EnhancedETIProcessorQt::processingProgress | ~5 fps (throttled) | ⚠️ TODO |
| m_playerLoopCheckbox | User input | QCheckBox::toggled | User interaction | ⚠️ TODO |
| m_progressBar | Processing progress | EnhancedETIProcessorQt::processingProgress | ~10 fps | ✅ EXISTS (m_progressBar) |

**Actions Needed:**
- Connect frame timing calculation (FCT * 24ms) to m_playerTimeLabel
- Connect frame counters to m_playerFrameLabel
- Implement loop playback functionality with m_playerLoopCheckbox

#### 2. Decoder Panel (Item #2)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_decoderStatusLabel | Processing state | EnhancedETIProcessorQt::processingComplete/Error | On state change | ⚠️ TODO |
| m_decoderTypeLabel | ETI format | EnhancedETIProcessorQt::formatDetected | Once per file | ⚠️ TODO |
| m_decoderTimeLabel | Frame timing | EnhancedETIProcessorQt::frameProcessed | ~5 fps (throttled) | ⚠️ TODO |
| m_decoderFramesLabel | Total frames | EnhancedETIProcessorQt::processingComplete | On completion | ⚠️ TODO |

**Actions Needed:**
- Connect ETI format detection to m_decoderTypeLabel
- Mirror timing from Player panel to Decoder panel
- Update status on processing state changes

#### 3. System Panel (Item #3)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_ficOverviewTree | FIG analysis | AdvancedFIGAnalyzer::ensembleInfoUpdated | ~1 fps (throttled) | ⚠️ TODO |
| m_enableFicAnalysis | User input | QCheckBox::toggled | User interaction | ⚠️ TODO |
| m_enableMscAnalysis | User input | QCheckBox::toggled | User interaction | ⚠️ TODO |

**Actions Needed:**
- Populate FIC Overview tree from FIG analysis results
- Implement FIC/MSC analysis toggle functionality

#### 4. Messages Panel (Item #4)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_logEntriesTree | ProfessionalLoggingSystem | logEntryAdded | On log event | ⚠️ DISABLED (causes freeze) |

**Actions Needed:**
- Fix logging system freeze issue
- Re-enable professional logging signals

---

### CENTER Panel Widgets

#### 5. DAB Ensemble Explorer (Item #5)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_ensembleTree | AdvancedFIGAnalyzer | servicesUpdated | ~1 fps (throttled) | ⚠️ TODO (old signal exists, needs new widget) |

**Actions Needed:**
- Connect servicesUpdated signal to populate m_ensembleTree
- Build hierarchical service tree from FIG analysis results

#### 6. ETI Frame Navigator (Item #6)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_frameList | EnhancedETIProcessorQt | frameProcessed | ~10 fps (throttled) | ✅ PARTIAL (old connection exists) |

**Actions Needed:**
- Update connection to use new m_frameList widget
- Ensure proper frame list population

#### 7. Hex Viewer / ETSI Compliance (Item #7)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_hexViewer | Frame selection | QListWidget::currentRowChanged | On user selection | ✅ EXISTS |
| m_complianceViewer | Frame selection | QListWidget::currentRowChanged | On user selection | ⚠️ TODO |

**Actions Needed:**
- Ensure hex viewer updates on frame selection
- Add ETSI compliance viewer updates

#### 8. Subchannel Organization (Item #8)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_subchannelTable | AdvancedFIGAnalyzer | FIG 0/1 parsing | ~1 fps (throttled) | ⚠️ TODO |

**Actions Needed:**
- Connect FIG 0/1 subchannel data to m_subchannelTable
- Populate 8 columns: ☑, SubCh ID, Type, Level, Bitrate, SAD, Size, Errors

#### 9. Now Playing (DLS+) (Item #9)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_trackLabel | DLSPlusDecoder | dlsPlusMetadataReceived | On DLS+ update | ⚠️ TODO (decoder exists, signal missing) |
| m_artistLabel | DLSPlusDecoder | dlsPlusMetadataReceived | On DLS+ update | ⚠️ TODO |
| m_albumLabel | DLSPlusDecoder | dlsPlusMetadataReceived | On DLS+ update | ⚠️ TODO |
| m_lastUpdatedLabel | DLSPlusDecoder | dlsPlusMetadataReceived | On DLS+ update | ⚠️ TODO |

**Actions Needed:**
- Add dlsPlusMetadataReceived signal to DLSPlusDecoder
- Connect signal to update Now Playing labels

---

### RIGHT Panel Widgets

#### 10. ETI Overview (Item #10)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_etiOverviewTable | EnhancedETIProcessorQt | frameProcessed | On frame update | ⚠️ TODO (NEW widget) |

**Actions Needed:**
- Extract ETI Type, Error Field, DAB Mode, STAT from frame header
- Populate 4-row table on frame update

#### 11. DAB Error Counter (Item #11)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_errorHistoryTree | AdvancedErrorDetector | errorDetected | On error | ✅ EXISTS |

**Actions Needed:**
- Verify error detection connections work with new widget

#### 12. Timing (Item #12)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_timingTable | EnhancedETIProcessorQt | frameProcessed | On frame update | ⚠️ TODO (NEW widget) |

**Actions Needed:**
- Calculate FCT=0 (µs), Step (µs), FCT=0 (ticks), Step (ticks)
- Populate 4-row table from frame header

#### 13. CU Usage (Item #13)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_cuUsageBar | AdvancedFIGAnalyzer | FIG 0/1 parsing | ~1 fps (throttled) | ⚠️ TODO (NEW widget) |
| m_cuUsageLabel | AdvancedFIGAnalyzer | FIG 0/1 parsing | ~1 fps (throttled) | ⚠️ TODO (NEW widget) |

**Actions Needed:**
- Sum subchannel sizes from FIG 0/1
- Update progress bar (X / 864 CUs for Mode 1)
- Update label with subchannel count

#### 14. Stream Statistics (Item #14)
| Widget | Signal Source | Signal Name | Update Rate | Implementation |
|--------|---------------|-------------|-------------|----------------|
| m_streamStatsTable | EnhancedETIProcessorQt | frameProcessed | ~1 fps (throttled) | ⚠️ TODO (NEW widget) |

**Actions Needed:**
- Calculate bitrate, data rate, frame rate, etc.
- Populate statistics table

---

## TAB 2: FIC-Analyzer - Connection Requirements

### LEFT Panel Widgets
| Widget | Signal Source | Signal Name | Implementation |
|--------|---------------|-------------|----------------|
| m_tab2_ensembleName | AdvancedFIGAnalyzer | ensembleInfoUpdated | ⚠️ TODO (NEW) |
| m_tab2_ficContentSummary | AdvancedFIGAnalyzer | FIG count tracking | ⚠️ TODO (NEW) |
| m_tab2_subchannelOrgSummary | AdvancedFIGAnalyzer | FIG 0/1 count | ⚠️ TODO (NEW) |
| m_tab2_serviceTree | AdvancedFIGAnalyzer | servicesUpdated | ⚠️ TODO (NEW) |
| m_tab2_digitalOrgInfo | AdvancedFIGAnalyzer | servicesUpdated | ⚠️ TODO (NEW) |
| m_tab2_serviceComponents | AdvancedFIGAnalyzer | servicesUpdated | ⚠️ TODO (NEW) |

### CENTER Panel Widgets
| Widget | Signal Source | Signal Name | Implementation |
|--------|---------------|-------------|----------------|
| m_tab2_ensembleTable | AdvancedFIGAnalyzer | FIG 0/2 parsing | ⚠️ TODO (NEW) |
| m_tab2_subchannelTable | AdvancedFIGAnalyzer | FIG 0/1 parsing | ⚠️ TODO (NEW) |
| m_tab2_serviceTable | AdvancedFIGAnalyzer | FIG 0/2 + 1/1 | ⚠️ TODO (NEW) |

### RIGHT Panel Widgets
| Widget | Signal Source | Signal Name | Implementation |
|--------|---------------|-------------|----------------|
| m_tab2_figContentTable | EnhancedETIProcessorQt | figDiscovered | ⚠️ TODO (NEW - needs FIG counting) |

**Actions Needed:**
- Count FIG occurrences by type/extension
- Populate FIG Content table with Type, Extension, Number, Subject

---

## TAB 3: FIC-Extractor - Connection Requirements

### LEFT Panel Widgets
| Widget | Signal Source | Signal Name | Implementation |
|--------|---------------|-------------|----------------|
| m_tab3_figInstanceTree | EnhancedETIProcessorQt | figDiscovered | ⚠️ TODO (NEW - needs frame/CIF/FIB tracking) |

**Actions Needed:**
- Track FIG instances across frames with Frame#, CIF#, FIB# positions
- Populate 7-column tree: FIG Type, Index, Frame#, CIF#, FIB#, Length, Items

### RIGHT Panel Widgets
| Widget | Signal Source | Signal Name | Implementation |
|--------|---------------|-------------|----------------|
| m_tab3_figItemDetailsTree | m_tab3_figInstanceTree | currentItemChanged | ⚠️ TODO (NEW - needs bit-level parsing) |

**Actions Needed:**
- Parse selected FIG instance into detailed fields
- Populate 4-column tree: Property, Size (bits), Value, Information
- **⚠️ WARNING:** Requires bit-level field size tracking (not yet implemented in parsers)

---

## Implementation Priority

### Phase 1: Critical Tab 1 Connections (HIGH PRIORITY)
1. ✅ Frame list population (m_frameList)
2. ⚠️ Hex viewer updates (m_hexViewer)
3. ⚠️ Player timing (m_playerTimeLabel, m_decoderTimeLabel)
4. ⚠️ ETI Overview table (m_etiOverviewTable)
5. ⚠️ Ensemble tree (m_ensembleTree)

### Phase 2: Tab 1 Data Display (MEDIUM PRIORITY)
6. ⚠️ Subchannel table (m_subchannelTable)
7. ⚠️ CU Usage (m_cuUsageBar)
8. ⚠️ Timing table (m_timingTable)
9. ⚠️ Stream statistics (m_streamStatsTable)
10. ⚠️ Now Playing DLS+ (m_trackLabel, etc.)

### Phase 3: Tab 2 FIC-Analyzer (MEDIUM PRIORITY)
11. ⚠️ All Tab 2 LEFT panel widgets
12. ⚠️ All Tab 2 CENTER panel tables
13. ⚠️ FIG Content table

### Phase 4: Tab 3 FIC-Extractor (LOW PRIORITY - needs parser enhancements)
14. ⚠️ FIG Instance tracking
15. ⚠️ FIG Item Details (requires bit-level parsing)

---

## Required New Signals

### EnhancedETIProcessorQt
- ✅ frameProcessed(ProcessedFrame) - EXISTS
- ✅ figDiscovered(FIGInfo) - EXISTS
- ⚠️ **NEW:** etiHeaderUpdated(ETIHeader) - for ETI Overview
- ⚠️ **NEW:** timingInfoUpdated(TimingInfo) - for Timing table
- ⚠️ **NEW:** streamStatsUpdated(StreamStats) - for Stream Statistics

### AdvancedFIGAnalyzer
- ✅ ensembleInfoUpdated(EnsembleInfo) - EXISTS
- ✅ servicesUpdated() - EXISTS
- ⚠️ **NEW:** subchannelInfoUpdated(QList<SubchannelInfo>) - for Subchannel table, CU Usage
- ⚠️ **NEW:** figCountUpdated(QMap<QString, int>) - for FIG Content table

### DLSPlusDecoder (Phase 3B)
- ⚠️ **NEW:** dlsPlusMetadataReceived(DLSPlusMetadata) - for Now Playing

---

## Next Steps

1. **Implement Phase 1 connections** (critical Tab 1 widgets)
2. **Add missing signals** to processors
3. **Create Qt GUI tests** for each connected widget
4. **Document throttling strategy** for high-frequency updates
5. **Fix logging system freeze** issue

---

**Status Summary:**
- ✅ Architecture patterns correct (Pattern 3 + 5 + 7 + 8)
- ⚠️ **Many new widgets lack signal connections**
- ⚠️ **New signals needed in processors**
- ⚠️ **Tab 2/3 completely disconnected**
- 🎯 **Priority: Connect Phase 1 critical widgets first**
