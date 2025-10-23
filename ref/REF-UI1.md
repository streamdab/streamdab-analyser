# REF-UI1: การวิเคราะห์โครงสร้าง GUI ของ DAB-XPlorer และ ETI-XPlorer

## รูปที่ 1: DAB-XPlorer (Theme สีอ่อน)

### โครงสร้างหลัก
- **Main Window** แบ่งเป็น 4 พื้นที่หลัก

### 1. Player Panel (บนซ้าย)
- **QGroupBox** หัวข้อ "Player Panel"
- แสดงสถานะการเล่น:
  - Label: Status (running/stopped)
  - Label: DAB_MOS_<mode>_<session>.bin
  - Label: Start/Current/End Time
  - Label: Audio data, Frame number
  - Button: Loops (toggle)
  - Progress bar แสดงเวลา (00:00:00 - 00:16:47 - 00:08:29)

### 2. Decoder Panel (บนขวา)
- **QGroupBox** หัวข้อ "Decoder Panel"
- แสดงสถานะ decoder:
  - Label: Status (running)
  - Label: Type
  - Label: Start/Current/End Time (เหมือน Player Panel)
  - Label: Frames (จำนวน frame)

### 3. System Panel (ล่างซ้าย)
- **QGroupBox** หัวข้อ "System Panel"
- **QTabWidget** มี 3 tabs:
  - **Transfer tab**: แสดงค่าพารามิเตอร์
  - **Control tab**
  - Tab อื่นๆ
- **QTableWidget** แสดงข้อมูล:
  - Columns: Property | Value | Data
  - Rows เช่น: G.703, TOCC, TCIE, TCL, PCL, AIS, PRES
- มี **QTreeWidget** แสดง Properties แบบ hierarchical:
  - FIC Level: Property, Value, Data, Status Code, IMPEG Audio Counter
- Buttons: Clear (ล่างขวา)

### 4. Messages Panel (ล่างขวา)
- **QGroupBox** หัวข้อ "Messages"
- แสดง Log File path
- **QTableView** แสดงข้อความ log:
  - Columns: Time | CIF | Source | Class | Message
  - มี checkbox column ด้านซ้าย
  - แสดง timestamp (15:35:38)
  - ประเภท: Analyzer, Applica...
  - Level: HCI, SBCH, MCI
  - Messages หลากหลาย (Sub-Channel errors, MPEG Audio Layer)
- Buttons: Filter, Clear (บนขวา)

### 5. Control Bar (กลาง)
- **QToolBar** มี media control buttons:
  - Browse button
  - Previous/Rewind
  - Play/Pause
  - Stop
  - Forward/Next
  - Media position slider

---

## รูปที่ 2: ETI-XPlorer (Theme สีอ่อน)

### โครงสร้างหลัก
- **Main Window** แบ่งเป็น 4 พื้นที่หลัก

### 1. Overview Panel (บนซ้าย)
- **QGroupBox** หัวข้อ "Overview"
- **QTableWidget** แสดงข้อมูลทั่วไป:
  - Columns: Property | Value
  - Rows:
    - ETI Type: ETSI(NI, G.704)
    - Error Field: Level 0
    - DAB Mode: 1
    - STAT: 0x8 64 / 99%

### 2. Error Counter Panel (กลางซ้าย)
- **QGroupBox** หัวข้อ "Error Counter"
- **QTableWidget** แบบ 2 คอลัมน์:
  - Columns: Error Type | Errors
  - Rows:
    - Bad Information
    - G.704 (G.703)
    - ETINA (G.703)
    - TATS (G.703)
  - มี warning icons (⚠️) บางแถว
  - แสดงจำนวน errors

### 3. Timing Panel (บนกลาง)
- **QGroupBox** หัวข้อ "Timing"
- **QTableWidget**:
  - Columns: Property | Value
  - Rows:
    - FCT = 0: 900000.0 us
    - Step: 24000.0 us
    - FCT = 0: 900000
    - Step: 24000.0 us

### 4. CU Usage Panel (บนขวา)
- **QGroupBox** หัวข้อ "CU Usage"
- **Stacked Bar Chart** แสดงการใช้งาน Capacity Units
  - แสดงเป็นแท่งสีต่างๆ (น้ำเงิน, ส้ม, เทา)
  - แบ่งเป็นส่วนๆ ตาม SubChannel

### 5. Subch Organization Panel (ล่าง)
- **QGroupBox** หัวข้อ "Subch Organization"
- **QTableWidget** แสดงรายละเอียด SubChannel:
  - Columns: Subch ID | Type | Level | Bitrate | SAD | Size | Errors
  - มี checkbox column ด้านซ้าย
  - Rows เช่น:
    - SubCh 0: LEP, Level 1, 192kbps, 120 OU, 80 OU, 0 errors
    - SubCh 1D: LEP, 3, 192kbps, 140 OU, 140 OU
    - SubCh 1E: EEP, 2-A, 104kbps, 104 OU, 13 OU
    - ... และอื่นๆ หลาย SubChannel
  - มี warning icons (⚠️) บางแถว

### 6. Additional Error Counters (ล่างซ้าย)
- แสดงข้อมูลเพิ่มเติม:
  - FIC Counter: 1
  - FP Phase: 0
  - FP Length: 0
  - TIST NA: 1 (⚠️)
  - NST: 1
  - RFUDF: 1 (⚠️)
  - Reed Solomon: 1 (⚠️)
  - Header: 0
  - Parity Stream: 0
  - RF/C Organization: 0 (⚠️)
  - CU Size: 0
  - Content: 1

---

## รูปที่ 3: DAB-XPlorer Bangkok ETI Analysis (Theme สีดำ - ปัจจุบัน)

### โครงสร้างหลัก
- **Main Window** แบ่งเป็น 3 พื้นที่หลัก (Dark Theme)

### 1. DAB Ensemble Explorer (ซ้าย)
- **QTreeWidget** แบบ hierarchical:
  - **Level 1**: Bangkok D... (Ensemble)
    - Columns: Service | Label | Type
    - แสดง: NBT Thailand | Ensemble
  - **Level 2**: NBT Ra... (Service)
    - แสดง: National Broadc... | Audio DAB+ (
  - **Level 3**: Fra... (Component)
    - แสดง: Active | Service Detail
  - มี expand/collapse arrows (▼ ▶)
  - แสดงหลาย services: NBT News Cha..., Evening...

### 2. ETI Frame Navigator (กลาง)
- **Progress Bar**: แสดง 100%
- **Label**: Frames: 5001 / 5001
- **QListWidget** แสดงรายการ frames:
  - Format: Frame X: ETI-LI-A (0xff8c549) หรือ ETI-LI-B (0xdf073ab6)
  - แสดง Frame 1-13 (scrollable)
  - มีการสลับระหว่าง A และ B frames

### 3. Frame Details (ล่าง)
- **QGroupBox** หัวข้อ "Frame Details"
- แสดงข้อมูล:
  - **Label**: Selected Frame: 4992
  - **Label**: ETI Frame Size: 6144 bytes
  - **Label**: Frame Offset: 30664704

### 4. Hex Viewer / ETSI Compliance (กลาง-ล่าง)
- **QTabWidget** มี 2 tabs:
  - **Hex Viewer tab** (active):
    - **QTextEdit** แสดงข้อมูล hex:
      - Format: Address | Hex values (16 bytes/row) | ASCII
      - แสดงที่อยู่: 0000:, 0010:, 0020:, ...
      - ข้อมูล hex มีสีต่างๆ (แดง, เขียว, เหลือง, ส้ม)
      - แสดง heading: "ETI Frame #4992 - Hex Viewer (6144 bytes)"
  - **ETSI Compliance tab**

### 5. Current Service Panel (ขวา)
- **QGroupBox** หัวข้อ "Current Service"
- แสดง:
  - **Label**: No service selected
  
### 6. Signal Quality Panel (ขวา-กลาง)
- **QGroupBox** หัวข้อ "Signal Quality"
- **Label**: Quality: Good
- **Progress Bar**: แสดง 85%

### 7. Now Playing (DLCP) Panel (ขวา-ล่าง)
- **QGroupBox** หัวข้อ "Now Playing (DLCP)"
- **QTableWidget** หรือ Labels:
  - Rows: Track | Artist | Album
  - แสดง: -- (no data)
  - **Label**: No metadata received

### 8. Status Bar (ล่างสุด)
- แสดงข้อความ: "Bangkok ETI Processing Complete - All 5,001 frames processed!"

---

## สรุปการรวม GUI เป็นหนึ่งเดียว

### Widgets หลักที่ต้องใช้ (Qt6):

1. **QMainWindow** - หน้าต่างหลัก
2. **QSplitter** - แบ่งพื้นที่แนวนอน/แนวตั้ง
3. **QTreeWidget** - แสดง Ensemble/Service hierarchy (3 levels)
4. **QTableWidget** - แสดงข้อมูลตาราง (Properties, Messages, SubChannels)
5. **QListWidget** - แสดง ETI Frames
6. **QGroupBox** - จัดกลุ่ม widgets
7. **QTabWidget** - สลับระหว่าง Hex Viewer / ETSI Compliance / System tabs
8. **QTextEdit** - แสดง Hex data
9. **QProgressBar** - แสดง playback position, signal quality
10. **QPushButton** - Control buttons (Play, Pause, Stop, etc.)
11. **QToolBar** - Media controls
12. **QLabel** - แสดงข้อมูลสถานะ
13. **QChartView** (QtCharts) - CU Usage stacked bar chart
14. **QStatusBar** - แสดงสถานะด้านล่าง

### Layout Structure แนะนำ:
```
QMainWindow
├── QToolBar (Media Controls)
├── QSplitter (Horizontal)
│   ├── Left Panel (QSplitter Vertical)
│   │   ├── DAB Ensemble Explorer (QTreeWidget)
│   │   ├── System Panel (QTabWidget)
│   │   │   ├── Transfer Tab (QTableWidget)
│   │   │   ├── Control Tab
│   │   │   └── Overview Tab (ETI properties)
│   │   └── Error Counter (QTableWidget)
│   ├── Center Panel (QSplitter Vertical)
│   │   ├── Player/Decoder Status (QGroupBox x2)
│   │   ├── ETI Frame Navigator (QListWidget)
│   │   ├── QTabWidget
│   │   │   ├── Hex Viewer (QTextEdit)
│   │   │   ├── ETSI Compliance
│   │   │   └── Subch Organization (QTableWidget)
│   │   └── Messages/Log (QTableWidget)
│   └── Right Panel (QSplitter Vertical)
│       ├── Current Service (QGroupBox)
│       ├── Signal Quality (QProgressBar)
│       ├── CU Usage Chart (QChartView)
│       └── Now Playing/Metadata (QGroupBox)
└── QStatusBar
```

---

## สรุป Columns ของแต่ละ Table

### Messages Table (Log)
- Checkbox | Time | CIF | Source | Class | Message

### System Panel - Transfer Tab
- Property | Value | Data

### System Panel - Tree (FIC Properties)
- Property | Value | Data | Status Code | IMPEG Audio Counter

### Overview Table (ETI)
- Property | Value

### Error Counter Table
- Error Type | Errors

### Timing Table
- Property | Value

### Subch Organization Table
- Checkbox | Subch ID | Type | Level | Bitrate | SAD | Size | Errors

### DAB Ensemble Tree (3 Levels)
- **Level 1 (Ensemble)**: Service | Label | Type
- **Level 2 (Service)**: Label | Type
- **Level 3 (Component)**: Status | Type

---

## หมายเหตุสำหรับการพัฒนา

1. ใช้ **QSplitter** เพื่อให้ผู้ใช้ปรับขนาดแต่ละ panel ได้
2. ใช้ **Dark Theme** สำหรับ version ปัจจุบัน
3. ใช้ **Icons** และ **Color Coding** สำหรับ status และ errors
4. รองรับ **Scrollable** สำหรับ Lists และ Tables ที่มีข้อมูลมาก
5. ใช้ **QTabWidget** สำหรับจัดกลุ่ม features ที่เกี่ยวข้อง
6. แสดง **Warnings/Errors** ด้วย icons (⚠️) และสีเหลือง/แดง
7. **Hex Viewer** ต้องรองรับ syntax highlighting หลายสี
8. **Progress Bars** แสดงทั้งแบบ percentage และ time-based
9. **Media Controls** ใช้ standard icons (Play, Pause, Stop, etc.)
10. **Status Bar** แสดงข้อมูลสถานะและผลลัพธ์การประมวลผล
