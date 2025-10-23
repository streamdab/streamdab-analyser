# REF-UI3: การวิเคราะห์โครงสร้าง GUI ของ FIC-XTractor

## ภาพรวม FIC-XTractor

FIC-XTractor เป็น protocol analyzer สำหรับวิเคราะห์ Fast Information Channel (FIC) ของ DAB ที่ทำงานในระดับ bit stream โดยสามารถ:
- อ่าน FIC จากไฟล์หรือ time-shift buffer
- Decode และแสดงผลตามลำดับเวลา (frame by frame) หรือตาม FIG type/extension
- แสดง FIG แบบ tree-like view พร้อม decode elements
- แปลง binary patterns เป็นรูปแบบที่อ่านง่าย (text, date, frequency, coordinates)
- แสดง FIB ในรูป hexadecimal พร้อม highlight FIG ที่เลือก
- แสดง icons เพื่อ flag errors (FIG errors, CRC errors, multiplex reconfiguration)

---

## โครงสร้าง GUI หลัก

### Main Window Layout
- **QMainWindow** แบ่งเป็น 4 พื้นที่หลัก:
  1. Top Toolbar
  2. Left Panel (Frame List / FIG List / FIG Database tabs)
  3. Right Panel (FIB Hex / FIG Item / FIG Statistic tabs)
  4. Bottom Status Bar

---

## 1. Top Section - Menu และ Toolbar

### 1.1 Menu Bar
- **QMenuBar** ประกอบด้วย:
  - **File** menu
  - **Tools** menu
  - **Help** menu

### 1.2 Main Toolbar
- **QToolBar** มี widgets ต่อไปนี้:
  - **QLabel**: "Stream" (label ด้านซ้าย)
  - **LED Indicator** (สีเขียว): แสดงสถานะ stream
  - **QToolButton**: ปุ่มรูปลูกศร (reload/refresh)
  - **QLabel**: "File" 
  - **QToolButton**: ปุ่มรูปโฟลเดอร์ (open file)
  - **QToolButton**: ปุ่มรูปซอง/กล่อง (save)
  - **Media Control Buttons**:
    - Pause button (||)
    - Previous/Rewind button (<<)
    - Next frame button (>)
    - Fast forward button (>>)
    - Skip to end button (|>)
  - **QToolButton**: ปุ่มรูปกรอบ/หน้าต่าง (window layout)

---

## 2. Left Panel - FIG Analysis Views

### 2.1 Tab Widget
- **QTabWidget** มี 3 tabs:
  - **Frame List** (active ในรูป)
  - **FIG List**
  - **FIG Database**

---

### 2.2 Frame List Tab

#### Header Section
- **QLabel** หรือ **QComboBox**:
  - "Type / Ext:" (dropdown selector)
  - แสดงค่า: "Type 0 - MCI and SI"

#### Tree Widget - FIG Hierarchy
- **QTreeWidget** แสดงโครงสร้างแบบ hierarchical (3-4 levels):

##### Level 1: FIG Type Groups
- **Nodes** (collapsible):
  - ⊟ Type 0 - MCI and SI (มี sub-items)
    - พร้อม summary: 1259 F
  - ⊞ FIG 0/0 - Ensemble information (collapsed)
    - พร้อม count: 2409
  - ⊞ FIG 0/1 - Sub-Channel information (collapsed)
    - พร้อม count: 5403
  - ⊟ FIG 0/2 - Service organisation (expanded)
    - พร้อม count: 12609

##### Level 2: FIG Instances
- **Table-like format** แสดงภายใต้ expanded FIG:
  - **Columns**: 
    - FIG type (เช่น FIG 0/2)
    - Index number (0-15+)
    - Frame# (10:76-10:91)
    - CIF# (0-2)
    - FIB# (23-30)
    - Length (24-30)
    - Items (2-4)
  
##### Example Rows (FIG 0/2):
```
FIG 0/2    0   10:76   2   24   4
FIG 0/2    1   10:77   0   23   3
FIG 0/2    2   10:78   1   30   4
FIG 0/2    3   10:79   0   30   4   ← Selected (highlighted in grey)
FIG 0/2    3   10:79   1   30   2
FIG 0/2    4   10:80   1   26   2
FIG 0/2    4   10:80   2   26   2
FIG 0/2    6   10:82   0   24   4
FIG 0/2    6   10:82   1   23   3
FIG 0/2    8   10:84   1   30   4
FIG 0/2    8   10:84   2   30   4
FIG 0/2    9   10:85   0   30   2
FIG 0/2    9   10:85   1   26   2
FIG 0/2   10   10:86   0   26   2
FIG 0/2   11   10:87   1   24   4
FIG 0/2   12   10:88   1   23   3
FIG 0/2   13   10:89   1   30   4
FIG 0/2   14   10:90   0   30   4
FIG 0/2   14   10:90   1   30   2
FIG 0/2   15   10:91   0   26   2
... (scrollable)
```

#### Features:
- ✓ **Collapsible tree nodes** (⊞/⊟ symbols)
- ✓ **Selection highlighting** (grey background)
- ✓ **Scrollable** content
- ✓ **Summary counts** for each FIG type
- ✓ **Icon indicators** สำหรับ errors (ระบุไว้ใน description)

---

## 3. Right Panel - FIG Details

### 3.1 Tab Widget
- **QTabWidget** มี 3 tabs:
  - **FIB Hex** (top tab)
  - **FIG Item** (middle tab - active ในรูป)
  - **FIG Statistic** (bottom tab)

---

### 3.2 FIB Hex Tab

#### Hex Viewer
- **QTextEdit** หรือ **QPlainTextEdit** แสดง hexadecimal data:
  - **Format**: Address | Hex bytes (16 bytes/row) | ASCII representation
  - **Example**:
    ```
    0000   1D 02 1G C4 02 3F 52 FF   .....?R.
    0008   FB 1A 45 02 JF 12 FF E0   ..E.....
    0010   D7 FB D9 8F 1E F4 53 46   .......SF
    0018   DD DC 8F FA 1E FA E3 6B   .......k
    ```
  - **Grey background highlighting**: แสดง FIG ที่เลือกใน Frame List

#### Features:
- ✓ แสดง address offset (0000, 0008, 0010, 0018, ...)
- ✓ แสดง hex values (16 bytes per row)
- ✓ แสดง ASCII representation (ด้านขวา)
- ✓ Highlight selected FIG ด้วย grey background
- ✓ Scrollable สำหรับ FIB ที่ยาว

---

### 3.3 FIG Item Tab

#### FIG Details Tree
- **QTreeWidget** แสดง FIG elements แบบ hierarchical (4-5 levels):

##### Level 1: FIG Header
- **Node**: 🔧 **FIG Header**
  - ✓ **FIG Type**: 3 bits, Value: 0
  - ✓ **Length of FIG data field**: 5 bits, Value: 29

##### Level 2: FIG Type Details
- **Node**: 🌺 **FIG Type 0 - MCI and SI**
  - ✓ **C/N flag**: 1 bit, Value: false
  - ✓ **OE flag**: 1 bit, Value: false
  - ✓ **P/D flag**: 1 bit, Value: false
  - ✓ **Extension**: 5 bits, Value: 2

##### Level 3: FIG Content
- **Node**: 🌸 **FIG 0/2 - Service organisation**
  - **Sub-node**: 🌼 **FIG 0/2** (expandable)

##### Level 4: Service Details
- **Under FIG 0/2 node**:
  - ✓ **SId**: 16 bits, Value: 0x10C4
  - ✓ **Local flag**: 1 bit, Value: false
  - ✓ **CAId**: 3 bits, Value: 0x0, Info: "No Access Control"
  - ✓ **Number of SC**: 4 bits, Value: 2
  - 🔻 **Xxx** (expandable node)

##### Level 5: Service Component Description
- **Node**: 🌷 **Service Component description**
  - ✓ **TMId**: 2 bits, Value: 0x0, Info: "MSC - Stream mode"
  - ✓ **ASCTy**: 6 bits, Value: 63, Info: "MPEG-1 HE-AAC-v"
  - ✓ **SubChId**: 6 bits, Value: 20

#### Features:
- ✓ **Colored icons** แยกตาม level/category (🔧🌺🌸🌼🌷)
- ✓ **Checkmark icons** (✓) บน leaf nodes
- ✓ **Expandable/collapsible** nodes (🔻 symbol)
- ✓ **Columns**: Property | Size | Value | Information
- ✓ **Bit-level details**: แสดงขนาดเป็น bits (1 bit, 2 bits, 3 bits, etc.)
- ✓ **Value display**: แสดงทั้ง hex (0x10C4) และ decimal (63)
- ✓ **Decoded information**: แสดง human-readable info (เช่น "No Access Control")
- ✓ **Scrollable** ทั้ง vertical และ horizontal

---

### 3.4 FIG Statistic Tab
- ไม่แสดงในรูป แต่น่าจะเป็น **QTableWidget** หรือ **QChartView** แสดงสถิติ FIG

---

## 4. Bottom Section - Status Bar

### Status Bar
- **QStatusBar** แสดงข้อมูล:
  - **Left section**: 
    - "Live Stream" (status label)
  - **Center section**:
    - "DAB-XPlorer::0031.8016" (application/version info)
  - **Right section**:
    - "EFCO stream" (stream type)
    - "Frames 0 - 9958 / 10000" (progress indicator)

#### Features:
- แสดงข้อมูลแบบ real-time
- แสดง frame counter และ total frames
- แสดง stream status

---

## สรุป Widgets และโครงสร้าง (Qt6)

### Widgets หลักที่ใช้:

1. **QMainWindow** - หน้าต่างหลัก
2. **QMenuBar** - Menu bar (File, Tools, Help)
3. **QToolBar** - Toolbar ด้านบน พร้อม media controls
4. **QTabWidget** (x2) - สำหรับ Left และ Right panels
5. **QTreeWidget** (x2) - แสดง FIG hierarchy และ FIG Item details
6. **QTextEdit** หรือ **QPlainTextEdit** - แสดง hex dump
7. **QSplitter** - แบ่งพื้นที่ Left และ Right panels
8. **QToolButton** - ปุ่มต่างๆ ใน toolbar
9. **QLabel** - Labels ใน toolbar และ tree
10. **QComboBox** - Dropdown selector (Type / Ext)
11. **QLed** หรือ custom widget - LED indicator
12. **QStatusBar** - Status bar ด้านล่าง
13. **QScrollBar** - Scrollbars สำหรับ scrollable areas
14. **QIcon** - Icons สำหรับ tree nodes และ buttons

---

## สรุป Columns ของแต่ละ Component

### 1. Frame List Tree (FIG Instance Table)
- FIG Type | Index | Frame# | CIF# | FIB# | Length | Items

### 2. FIG Item Tree (FIG Element Details)
- Property | Size | Value | Information

**หมายเหตุ**: 
- Column "Size" แสดงเป็น "X bits" (เช่น 1 bit, 3 bits, 5 bits)
- Column "Value" แสดงทั้ง decimal และ hex (0x...)
- Column "Information" แสดง decoded text/description

---

## โครงสร้าง Tree Widgets

### Tree 1: Frame List (Left Panel)

#### Level 1: FIG Type Groups
- Type 0 - MCI and SI
- FIG 0/0 - Ensemble information
- FIG 0/1 - Sub-Channel information
- FIG 0/2 - Service organisation
- ... (และ FIG types อื่นๆ)

#### Level 2: FIG Instances (Table format)
- แสดงรายการ FIG instances ตาม frame/CIF/FIB
- Format: FIG X/Y | Index | Frame# | CIF# | FIB# | Length | Items

#### Icons/Indicators:
- ⊞ Collapsed node
- ⊟ Expanded node
- ⚠️ Error indicator (สำหรับ FIG errors)
- 🔴 CRC error indicator
- 🔄 Multiplex reconfiguration indicator

---

### Tree 2: FIG Item Details (Right Panel)

#### Level 1: FIG Header (🔧)
- FIG Type
- Length of FIG data field

#### Level 2: FIG Type Details (🌺)
- C/N flag
- OE flag
- P/D flag
- Extension

#### Level 3: FIG Content (🌸)
- FIG X/Y - [Description]

#### Level 4: Element Groups (🌼)
- FIG elements (SId, Local flag, CAId, etc.)
- Sub-groups (Xxx)

#### Level 5: Detailed Elements (🌷)
- Service Component description
- Field-level details (TMId, ASCTy, SubChId, etc.)

#### Icon Legend:
- 🔧 Header/Configuration
- 🌺 FIG Type Level
- 🌸 FIG Content Level
- 🌼 Element Group Level
- 🌷 Detailed Element Level
- ✓ Leaf node (final value)
- 🔻 Expandable node

---

## Layout Structure แนะนำ:

```
QMainWindow (FIC-XTractor)
├── QMenuBar
│   ├── File
│   ├── Tools
│   └── Help
├── QToolBar (Top)
│   ├── Stream Section
│   │   ├── QLabel: "Stream"
│   │   ├── LED Indicator (custom widget)
│   │   └── QToolButton (reload)
│   ├── File Section
│   │   ├── QLabel: "File"
│   │   ├── QToolButton (open)
│   │   └── QToolButton (save)
│   ├── Media Controls
│   │   ├── QToolButton (pause)
│   │   ├── QToolButton (prev)
│   │   ├── QToolButton (next)
│   │   ├── QToolButton (forward)
│   │   └── QToolButton (skip)
│   └── QToolButton (window layout)
├── QSplitter (Horizontal - Main Content)
│   ├── Left Panel (QTabWidget)
│   │   ├── Frame List Tab
│   │   │   ├── QComboBox (Type/Ext selector)
│   │   │   └── QTreeWidget (FIG Hierarchy)
│   │   │       ├── Level 1: FIG Type Groups
│   │   │       ├── Level 2: FIG Instances (table format)
│   │   │       │   └── Columns: Type | Index | Frame# | CIF# | FIB# | Length | Items
│   │   │       └── Features: collapsible, icons, selection
│   │   ├── FIG List Tab
│   │   │   └── (similar structure, different organization)
│   │   └── FIG Database Tab
│   │       └── (reference database view)
│   └── Right Panel (QTabWidget)
│       ├── FIB Hex Tab
│       │   └── QTextEdit/QPlainTextEdit (Hex Viewer)
│       │       ├── Format: Address | Hex | ASCII
│       │       └── Features: syntax highlighting, selection marking
│       ├── FIG Item Tab
│       │   └── QTreeWidget (FIG Element Details)
│       │       ├── Level 1: FIG Header (🔧)
│       │       ├── Level 2: FIG Type Details (🌺)
│       │       ├── Level 3: FIG Content (🌸)
│       │       ├── Level 4: Element Groups (🌼)
│       │       ├── Level 5: Detailed Elements (🌷)
│       │       └── Columns: Property | Size | Value | Information
│       └── FIG Statistic Tab
│           └── QTableWidget/QChartView (Statistics)
└── QStatusBar (Bottom)
    ├── Left: Live Stream status
    ├── Center: Application info
    └── Right: Frame counter
```

---

## หมายเหตุสำหรับการพัฒนา

### General Features:
1. **Synchronization**: เมื่อเลือก FIG ใน Frame List, FIG Item และ FIB Hex ต้อง update แบบ synchronized
2. **Highlighting**: FIG ที่เลือกใน Frame List ต้องถูก highlight ใน FIB Hex (grey background)
3. **Error Indicators**: ใช้ icons เพื่อแสดง FIG errors, CRC errors, และ multiplex reconfig
4. **Bit-level Parsing**: แสดงข้อมูลในระดับ bit พร้อม decode เป็น human-readable
5. **Multiple Views**: รองรับ 3 views (Frame List, FIG List, FIG Database)
6. **Real-time Update**: รองรับการอัพเดทข้อมูลแบบ real-time จาก live stream

### Tree Widget Customization:
1. **Custom Icons**: ใช้ custom icons สำหรับแต่ละ level (🔧🌺🌸🌼🌷✓)
2. **Column Headers**: แสดง headers ชัดเจน (Property | Size | Value | Information)
3. **Expandable States**: จำสถานะ expand/collapse ของแต่ละ node
4. **Selection Sync**: เลือก item ใน tree แล้ว sync กับ hex viewer

### Hex Viewer Features:
1. **Syntax Highlighting**: ใช้สีต่างๆ สำหรับ highlight patterns
2. **Selection Marking**: highlight FIG ที่เลือกด้วย grey background
3. **Address Display**: แสดง address offset ด้านซ้าย
4. **ASCII Column**: แสดง ASCII representation ด้านขวา
5. **Scrolling**: รองรับ smooth scrolling

### Performance:
1. **Lazy Loading**: โหลดข้อมูลแบบ lazy สำหรับ tree ที่มี items มาก
2. **Virtual Scrolling**: ใช้ virtual scrolling สำหรับ large datasets
3. **Caching**: cache parsed FIG data เพื่อประสิทธิภาพ

### Data Format:
1. **Bit Precision**: แสดงขนาดข้อมูลเป็น bits (1 bit, 2 bits, 3 bits, etc.)
2. **Multiple Representations**: แสดงค่าทั้งแบบ hex (0x...) และ decimal
3. **Decoded Information**: แปลงค่าเป็น text ที่อ่านง่าย
4. **Date/Time**: parse และแสดง date/time (FIG 0/10)
5. **Frequency**: parse และแสดง frequency (FIG 0/21)
6. **Coordinates**: parse และแสดง geographic coordinates (FIG 0/22)

---

## การเชื่อมโยงกับ REF-UI1 และ REF-UI2

FIC-XTractor สามารถรวมเข้ากับ DAB-XPlorer/ETI-XPlorer และ FIC-XPlorer โดย:

### Option 1: Integrated View
- เพิ่มเป็น **Tab** ใน main QTabWidget
- ใช้ร่วมกันกับ ETI Frame Navigator และ Hex Viewer
- Share FIC data ระหว่าง tools

### Option 2: Dockable Widget
- สร้างเป็น **QDockWidget** ที่ dock/undock ได้
- Float เป็น separate window เมื่อต้องการ
- Dock กลับมาเมื่อต้องการประหยัดพื้นที่

### Option 3: Split View
- ใช้ **QSplitter** แบ่งพื้นที่กับ tools อื่น
- แสดง FIC-XTractor ควบคู่กับ ETI-XPlorer
- Resize ได้ตามต้องการ

### แนะนำ:
ใช้ **Option 2 (QDockWidget)** เพื่อ:
- ความยืดหยุ่นสูงสุด
- ผู้ใช้สามารถจัด layout ได้เอง
- รองรับ multiple monitors
- Save/restore layout state

---

## Integration Points

### Shared Components:
1. **FIC Data Source**: ใช้ FIC data source ร่วมกัน
2. **Frame Navigator**: sync กับ ETI Frame Navigator
3. **Hex Viewer**: ใช้ hex viewer component ร่วมกัน
4. **Status Bar**: แสดงข้อมูล status รวม

### Data Flow:
1. ETI-XPlorer → extracts FIC
2. FIC-XTractor → analyzes FIC
3. FIC-XPlorer → displays FIC content
4. Results → shared across all tools

### UI Consistency:
1. ใช้ **Dark Theme** เหมือน REF-UI1
2. ใช้ **Icon Set** เดียวกัน
3. ใช้ **Font และ Colors** สอดคล้องกัน
4. ใช้ **Layout Patterns** ที่คล้ายกัน

---

## สรุป Core Features

### FIC-XTractor Core Capabilities:
1. ✅ Parse FIC on bit stream level
2. ✅ Display chronologically (frame by frame) OR by FIG type/extension
3. ✅ Tree-like view แสดง FIG elements ตาม DAB standard
4. ✅ Decode binary patterns เป็น human-readable form
5. ✅ Display FIB in hexadecimal with highlighting
6. ✅ Flag errors (FIG errors, CRC errors, multiplex reconfig)
7. ✅ Support multiple input sources (file, time-shift buffer)
8. ✅ Real-time analysis สำหรับ live streams

### Specialized Decoding:
- ✅ Text strings
- ✅ Dates (FIG 0/10)
- ✅ Frequencies (FIG 0/21)
- ✅ Geographic coordinates (FIG 0/22)
- ✅ Service organization (FIG 0/2)
- ✅ Sub-channel information (FIG 0/1)
- ✅ Ensemble information (FIG 0/0)
