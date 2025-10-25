# UI Restructure Plan - Match DATA_AVAILABILITY_MATRIX.md Exactly

> **Date:** 2025-10-26  
> **Goal:** Reorganize UI to match DATA_AVAILABILITY_MATRIX.md specification exactly  
> **User Feedback:** "It still the same previous UI. Are you sure it can change as DATA_AVAILABILITY_MATRIX.md design?"  
> **Status:** 🔄 Planning → Implementation

---

## Problem Statement

The current UI uses a "pragmatic" layout (documented in UI_LAYOUT_ANALYSIS.md Option B) where we kept the working panels in their current locations and added missing panels. However, the user expects the EXACT layout from DATA_AVAILABILITY_MATRIX.md specification.

**User's Correct Observation:** The UI hasn't changed to match the specification - it still shows the old layout.

---

## Current vs Specification Mismatch

### LEFT PANEL (25% width)

**Specification Says:**
1. Player Panel (QGroupBox) - Status, file name, time, frame number, loop button, progress bar
2. Decoder Panel (QGroupBox) - Status, type, time, frames
3. System Panel (QTabWidget) - Tab 1: FIC Overview, Tab 2: Control
4. Messages Panel (QGroupBox) - Log file, time, CIF, source, class, message

**Current Implementation Has:**
1. ❌ DAB Ensemble Explorer (QTreeWidget) - WRONG! Should be in CENTER
2. ❌ ETI Explorer Details (QTableWidget) - WRONG! Should be in RIGHT as "ETI Overview"
3. ✅ Messages Panel (QGroupBox) - CORRECT

**Verdict:** 75% WRONG - Need complete LEFT panel rebuild

---

### CENTER PANEL (50% width)

**Specification Says:**
5. DAB Ensemble Explorer (QTreeWidget) - Service tree hierarchy
6. ETI Frame Navigator (QListWidget) - Frame list
7. Hex Viewer / ETSI Compliance (QTabWidget) - Hex + Compliance tabs
8. Subchannel Organization (QTableWidget) - 8-column table
9. Now Playing (DLS+) (QGroupBox) - Track, artist, album

**Current Implementation Has:**
1. ✅ Live Stream Input (NEW) - Not in spec but useful
2. ✅ Player/Decoder Status - Simplified version
3. ✅ ETI Frame Navigator - CORRECT
4. ✅ Subchannel Organization - CORRECT (just added!)
5. ✅ Hex Viewer / ETSI Compliance - CORRECT
6. ❌ DAB Ensemble Explorer - MISSING! It's in LEFT panel!
7. ❌ Now Playing (DLS+) - MISSING! It's in RIGHT panel!

**Verdict:** 50% CORRECT - Missing 2 major panels (they're in wrong locations)

---

### RIGHT PANEL (25% width)

**Specification Says:**
10. ETI Overview (QTableWidget) - ETI Type, Error Field, DAB Mode, STAT
11. DAB Error Counter (QTableWidget) - CRC errors, invalid FIGs
12. Timing (QTableWidget) - FCT timing
13. CU Usage (Chart) - CU allocation bar
14. DAB Stream Statistics (QTableWidget) - 10+ metrics

**Current Implementation Has:**
1. ❌ Current Service (QGroupBox) - Not in spec
2. ❌ Signal Quality (QProgressBar) - Not in spec
3. ❌ Now Playing (DLS+) (QGroupBox) - WRONG! Should be in CENTER!
4. ✅ DAB Error Counter - CORRECT
5. ✅ Timing - CORRECT
6. ✅ CU Usage - CORRECT
7. ✅ Stream Statistics - CORRECT
8. ❌ ETI Overview - MISSING! (ETI Explorer Details in LEFT is this, but wrong location)

**Verdict:** 40% CORRECT - Has extra panels + missing ETI Overview in right location

---

## What Needs to Change

### Phase 1: LEFT Panel Complete Rebuild

**Remove:**
- DAB Ensemble Explorer (move to CENTER)
- ETI Explorer Details (convert to ETI Overview, move to RIGHT)

**Add:**
1. **Player Panel (NEW)**
   ```cpp
   QGroupBox* createPlayerPanel()
   {
       QGroupBox* group = new QGroupBox("Player");
       // Status: Running/Stopped
       // File name: Display m_currentFilePath
       // Start/Current/End Time: Calculate from FCT
       // Frame number: m_currentFrameIndex
       // Loops checkbox
       // Progress bar: m_progressBar
   }
   ```

2. **Decoder Panel (NEW)**
   ```cpp
   QGroupBox* createDecoderPanel()
   {
       QGroupBox* group = new QGroupBox("Decoder");
       // Status: Processing state
       // Type: "ETI-NI"
       // Start/Current/End Time: Mirror player
       // Frames: m_totalFrames
   }
   ```

3. **System Panel (NEW)**
   ```cpp
   QGroupBox* createSystemPanel()
   {
       QGroupBox* group = new QGroupBox("System");
       QTabWidget* tabs = new QTabWidget();
       
       // Tab 1: FIC Overview (QTreeWidget)
       // - Display FIG analysis results hierarchically
       // - Property | Value | Data | Status Code
       
       // Tab 2: Control
       // - Processing controls
       // - Analysis settings
       // - Display options
   }
   ```

4. **Keep Messages Panel** ✅

**Estimated Work:** 3-4 hours

---

### Phase 2: CENTER Panel Reorganization

**Add (move from LEFT):**
- DAB Ensemble Explorer (already exists as m_ensembleTree)

**Add (move from RIGHT):**
- Now Playing (DLS+) panel (already exists via createNowPlayingPanel())

**Keep:**
- Live Stream Input (extra, but useful)
- Player/Decoder Status (simplified, can keep or replace with LEFT panels)
- ETI Frame Navigator ✅
- Subchannel Organization ✅
- Hex Viewer / ETSI Compliance ✅

**Changes Needed:**
```cpp
QWidget* createTab1_CenterPanel()
{
    // 1. DAB Ensemble Explorer (from LEFT) - 15%
    layout->addWidget(m_ensembleTree, 15);
    
    // 2. ETI Frame Navigator - 15%
    layout->addWidget(navigatorGroup, 15);
    
    // 3. Subchannel Organization - 15%
    layout->addWidget(subchannelGroup, 15);
    
    // 4. Hex Viewer / ETSI Compliance - 30%
    layout->addWidget(viewerTabs, 30);
    
    // 5. Now Playing (from RIGHT) - 25%
    QGroupBox* nowPlayingPanel = createNowPlayingPanel();
    layout->addWidget(nowPlayingPanel, 25);
}
```

**Estimated Work:** 1-2 hours

---

### Phase 3: RIGHT Panel Reorganization

**Add (from LEFT):**
- ETI Overview (currently "ETI Explorer Details")

**Remove:**
- Current Service (not in spec)
- Signal Quality (not in spec)
- Now Playing (move to CENTER)

**Keep:**
- DAB Error Counter ✅
- Timing ✅
- CU Usage ✅
- Stream Statistics ✅

**Changes Needed:**
```cpp
QWidget* createTab1_RightPanel()
{
    // 1. ETI Overview (from LEFT "ETI Explorer Details")
    QGroupBox* etiOverviewGroup = new QGroupBox("ETI Overview");
    layout->addWidget(etiOverviewGroup);
    
    // 2. DAB Error Counter ✅
    layout->addWidget(errorCounterPanel);
    
    // 3. Timing ✅
    layout->addWidget(timingPanel);
    
    // 4. CU Usage ✅
    layout->addWidget(cuUsagePanel);
    
    // 5. Stream Statistics ✅
    layout->addWidget(streamStatsPanel);
}
```

**Estimated Work:** 1 hour

---

## Implementation Strategy

### Option A: Complete Rebuild (RECOMMENDED for specification match)

**Approach:** Rewrite createTab1_LeftPanel, createTab1_CenterPanel, createTab1_RightPanel from scratch

**Pros:**
- ✅ 100% matches DATA_AVAILABILITY_MATRIX.md
- ✅ Clean implementation
- ✅ No compromises

**Cons:**
- ❌ 5-7 hours work
- ❌ May break existing functionality temporarily
- ❌ Need to test everything

**Timeline:** 1 working day

---

### Option B: Incremental Move (FASTER but may have temporary issues)

**Approach:** Move panels one by one, test after each move

**Steps:**
1. Create Player + Decoder + System panels in LEFT (2 hours)
2. Move Ensemble Tree from LEFT to CENTER (30 min)
3. Move ETI Explorer Details from LEFT to RIGHT (rename to ETI Overview) (30 min)
4. Move Now Playing from RIGHT to CENTER (30 min)
5. Remove extra panels from RIGHT (Current Service, Signal Quality) (15 min)
6. Test and adjust layouts (1 hour)

**Timeline:** 4-5 hours

---

## Recommendation

**Use Option B: Incremental Move**

**Rationale:**
1. User wants the specification layout - we must deliver
2. Incremental approach allows testing at each step
3. Can commit after each successful move
4. Less risky than complete rebuild

---

## Detailed Implementation Plan

### Step 1: Create Player Panel (30 min)

**File:** `src/main.cpp`  
**Location:** Add helper method after `createNowPlayingPanel()`

```cpp
QGroupBox* createPlayerPanel()
{
    QGroupBox* group = new QGroupBox("Player");
    if (!group) {
        qCritical() << "Failed to create Player panel";
        return nullptr;
    }
    
    QGridLayout* layout = new QGridLayout(group);
    layout->setContentsMargins(4, 8, 4, 4);
    
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
    m_playerFileLabel = new QLabel("No file");
    m_playerFileLabel->setWordWrap(true);
    layout->addWidget(fileLabel, 1, 0);
    layout->addWidget(m_playerFileLabel, 1, 1);
    
    // Row 2: Time
    QLabel* timeLabel = new QLabel("Time:");
    timeLabel->setStyleSheet("font-weight: bold;");
    m_playerTimeLabel = new QLabel("00:00 / 00:00");
    layout->addWidget(timeLabel, 2, 0);
    layout->addWidget(m_playerTimeLabel, 2, 1);
    
    // Row 3: Frame
    QLabel* frameLabel = new QLabel("Frame:");
    frameLabel->setStyleSheet("font-weight: bold;");
    m_playerFrameLabel = new QLabel("0 / 0");
    layout->addWidget(frameLabel, 3, 0);
    layout->addWidget(m_playerFrameLabel, 3, 1);
    
    // Row 4: Loop checkbox
    m_playerLoopCheckbox = new QCheckBox("Loop");
    layout->addWidget(m_playerLoopCheckbox, 4, 0, 1, 2);
    
    // Row 5: Progress bar
    m_playerProgressBar = new QProgressBar();
    m_playerProgressBar->setValue(0);
    layout->addWidget(m_playerProgressBar, 5, 0, 1, 2);
    
    return group;
}
```

**Member Variables to Add:**
```cpp
// Player Panel widgets
QLabel* m_playerStatusLabel = nullptr;
QLabel* m_playerFileLabel = nullptr;
QLabel* m_playerTimeLabel = nullptr;
QLabel* m_playerFrameLabel = nullptr;
QCheckBox* m_playerLoopCheckbox = nullptr;
QProgressBar* m_playerProgressBar = nullptr;
```

---

### Step 2: Create Decoder Panel (20 min)

Similar structure to Player Panel with Decoder-specific fields.

---

### Step 3: Create System Panel (40 min)

QTabWidget with 2 tabs:
- Tab 1: FIC Overview (QTreeWidget)
- Tab 2: Control (settings widgets)

---

### Step 4: Rebuild LEFT Panel (20 min)

Replace createTab1_LeftPanel() completely:
```cpp
QWidget* createTab1_LeftPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    // 1. Player Panel (20%)
    QGroupBox* playerPanel = createPlayerPanel();
    layout->addWidget(playerPanel, 20);
    
    // 2. Decoder Panel (20%)
    QGroupBox* decoderPanel = createDecoderPanel();
    layout->addWidget(decoderPanel, 20);
    
    // 3. System Panel (40%)
    QGroupBox* systemPanel = createSystemPanel();
    layout->addWidget(systemPanel, 40);
    
    // 4. Messages Panel (20%)
    QGroupBox* messagesPanel = createLoggingPanel();
    layout->addWidget(messagesPanel, 20);
    
    return panel;
}
```

---

### Step 5-8: Move Panels Between LEFT/CENTER/RIGHT

Move widgets using existing variables (m_ensembleTree, m_etiExplorerDetails, etc.)

---

## Timeline

**Total Estimated Time:** 5-6 hours

- Step 1: Create Player Panel - 30 min
- Step 2: Create Decoder Panel - 20 min
- Step 3: Create System Panel - 40 min
- Step 4: Rebuild LEFT panel - 20 min
- Step 5: Move Ensemble Tree to CENTER - 15 min
- Step 6: Move ETI Explorer to RIGHT - 15 min
- Step 7: Move Now Playing to CENTER - 15 min
- Step 8: Remove extra RIGHT panels - 10 min
- Testing & Layout Adjustment - 1 hour
- Documentation - 30 min

**Start Time:** Now  
**Expected Completion:** 5-6 hours from now

---

## Success Criteria

✅ LEFT Panel has: Player, Decoder, System (FIC Overview + Control), Messages  
✅ CENTER Panel has: Ensemble Explorer, Frame Navigator, Subchannel Org, Hex Viewer, Now Playing  
✅ RIGHT Panel has: ETI Overview, Error Counter, Timing, CU Usage, Stream Statistics  
✅ All panels visible and functional  
✅ Zero compilation errors  
✅ UI matches DATA_AVAILABILITY_MATRIX.md exactly

---

## Risk Assessment

**Low Risk:**
- All widgets already exist (just moving them)
- Panel creation methods already work
- Layout system (QVBoxLayout, QHBoxLayout) is straightforward

**Medium Risk:**
- May need to adjust signal/slot connections
- Layout percentages may need tweaking
- Some panels may overlap initially

**Mitigation:**
- Test after each step
- Commit after each successful move
- Keep backup of working code

---

**Status:** Ready to implement. Shall I proceed with Step 1?
