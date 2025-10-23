# REF-UI2: การวิเคราะห์โครงสร้าง GUI ของ FIC-XPlorer

## ภาพรวม FIC-XPlorer

FIC-XPlorer เป็น tool สำหรับวิเคราะห์ Fast Information Channel (FIC) ของ DAB โดยมี GUI ที่แสดงข้อมูล FIC แบบละเอียด มี 2 layouts หลัก:
- **รูปที่ 1**: แสดงข้อมูลแบบ Ensemble และ Service Component
- **รูปที่ 2**: แสดงข้อมูลแบบ Service Organization และ FIG Details

---

## รูปที่ 1: FIC-XPlorer - Ensemble View

### โครงสร้างหลัก
- **Main Window** แบ่งเป็น 3 พื้นที่หลัก (แนวนอน)

### 1. Left Panel - Overview & Service Tree

#### 1.1 Overview Panel (บนซ้าย)
- **QGroupBox** หัวข้อ "Overview"
- **QLabel** แสดงข้อมูล:
  - Sydney Mux1 3C
  - FIC Content:
  - Sub-Channel Org.

#### 1.2 Service Tree (กลางซ้าย)
- **QTreeWidget** แบบ hierarchical (3 levels):
  - **Level 1**: SubCh 1-9
    - ไม่มี columns (แค่ label)
    - เป็น parent nodes
  - **Level 2**: Service types (Audio, Data, Packet)
    - แสดงเลข ID (1, 2, 3, ...)
  - **Level 3**: Sub-items (ไม่มีในรูปนี้)

#### 1.3 List of Digital Org Info (ล่างซ้าย)
- **QLabel** แสดง timestamps:
  - 0x202E: Digital Org. '
  - 0x2B1E: None 969
  - 0x2D31: ABC dig
  - 0x2D32: '885 Digital Org.'
  - 0x2D3E: '885 Digital Org.'
  - 0x2D3E: 2dE
  - 0x2D3e: DAB Plus 1
  - 0x2D3e: DAB Plus 2
  - และอื่นๆ

#### 1.4 Service Components List (ล่างซ้ายสุด)
- **QTreeWidget** แบบ simple list:
  - Audio
  - SubCh 1
  - SubCh 2
  - SubCh 3
  - SubCh 4
  - SubCh 5
  - SubCh 6
  - SubCh 8
  - FIDC
  - Packet
    - SC1d 0x1b, PA#1 0x1
    - SC2d 0x2c, PA#d 0x1

---

### 2. Center Top Panel - Ensemble Information

#### 2.1 Ensemble Tab
- **QTabWidget** มี 2 tabs:
  - **Ensemble** (active)
  - **Service Component**

#### 2.2 Ensemble Table
- **QTableWidget** แสดงข้อมูล Ensemble:
  - **Columns**: Type | SubCH Id | FIGCH | PADDR | SC1d | CA | CAOrg | Bitrate | ASCTy+ | Info
  - **Rows**:
    - Audio (หลายแถว) พร้อมข้อมูล bitrate (160 kbps, 96 kbps, 56 kbps)
    - Data
    - Packet
  - แสดงค่า:
    - SubCH Id: 1-9
    - Bitrate: 160 kbps, 96 kbps, 56 kbps, 48 kbps
    - CA: No
    - CAOrg: - (dash)
    - ASCTy+: 0x00, 0x2F, 0x3F, 0x2E
    - Info: 0, 1
  - มีข้อมูล hexadecimal ที่ท้ายตาราง: 0x001, 0x001, No

#### 2.3 List of Services (ล่างกลาง)
- **QTableWidget** แสดงรายการ Services:
  - **Columns**: SId | Service | P/S | SC1dS | SC Label | SC Short Label
  - **Row example**:
    - SId: 0x2022
    - Service: '885 Digital Org.'
    - P/S: 0
    - SC1dS: (blank)
    - SC Label: (blank)
    - SC Short Label: (blank)

---

### 3. Center Bottom Panel - Sub-Channel Details

#### 3.1 Sub-Channel Tab
- **QTabWidget** มี tabs:
  - **Sub-Channel** (active)
  - **Service** (tab ที่สอง)

#### 3.2 Sub-Channel Table
- **QTableWidget** แสดงรายละเอียด Sub-Channel:
  - **Columns**: SubCh Id | SAD | Bitrate | P.Level | P.Type | SubCh Type | SId | Service Label | Short Label | CAID
  - **Rows** (หลายแถว):
    - SubCh 1: SAD 0, 192 kbps, 3, UEP, Audio, 0x2E1, '885 Digital Org.', '885 Dig', No access control
    - SubCh 2: SAD 48, 160 kbps, 3, UEP, Audio, 0x2D21, ABC dig, ABC dig, No access control
    - SubCh 3: SAD 528, 160 kbps, 1, UEP, Audio, 0x2022, '885 Digital One', '885 Dig', No access control
    - SubCh 4: SAD 680, 96 kbps, 3, UEP, Audio, 0x2D24, DLE, DLE, No access control
    - SubCh 5: SAD 788, 96 kbps, 3, UEP, Audio, 0x2D3e, DAB Plus 1, DAB-Ple1, No access control
    - SubCh 6: SAD 896, 56 kbps, 3-A, EEP, Packet, 0x2D3e, DAB Plus 2, DAB-Ple1, No access control
    - SubCh 8: SAD 948, 48 kbps, 3-A, EEP, Audio, 0xD200209, EPG, EPG, No access control
    - SubCh 9: (ไม่แสดงครบ)

#### 3.3 List of Services (ล่างซ้าย - ซ้ำจากด้านบน)
- **QTableWidget**:
  - **Columns**: SId | Service Label
  - **Rows**:
    - 0x2022: '885 Digital Org.'

---

### 4. Right Panel - FIC Content & Service Components

#### 4.1 FIC Content Table (บนขวา)
- **QGroupBox** หัวข้อ "FIC Content"
- **QTableWidget**:
  - **Columns**: Type | Extension | Number | Subject
  - **Rows** (scrollable):
    - 0: 5, 191, Ensemble information
    - 0: 2, 1, Service organisation
    - 0: 1, 191, Sub-Channel information
    - 0: 2, 0, Service organisation
    - 0: 2, 15, Service organisation (audio)
    - 0: 0, 31, SC global definition
    - 0: 8, -, Country, LTO and International
    - 0: 9, 2, Country LTO and International
    - 0: 13, -, User Application Data
    - 0: 14, 15, FEC sub-channel organisation
    - 0: 17, 15, Programme Type
    - 0: 16, -, Programme Number
    - 1: 0, 0, Ensemble label
    - 1: 1, 0, Service label
    - 1: 4, 0, Service label
    - 1: 5, 1, Data service label
    - 5: -, 2, Traffic Message Channel

#### 4.2 List of Service Components (ล่างขวา)
- **QTableWidget**:
  - **Columns**: Type | Id | PA&Dr | P/S | SC1dS | SC Label
  - **Rows**:
    - Audio: 4, -, P, -, 0, -

---

## รูปที่ 2: FIC-XPlorer - Service Organization View

### โครงสร้างหลัก
- **Main Window** แบ่งเป็น 3 พื้นที่หลัก (เหมือนรูปที่ 1)

### 1. Left Panel - FIC Content Tree

#### 1.1 Overview Panel (บนซ้าย)
- **QGroupBox** หัวข้อ "Overview"
- **QLabel** แสดงข้อมูล:
  - '10N Norders: '
  - FIC Content:
  - Sub-Channel Org.

#### 1.2 FIC Content Tree (กลางซ้าย)
- **QTreeWidget** แบบ hierarchical (3-4 levels):
  - **Level 1**: หมวดหมู่หลัก
    - SubCh 1-12
    - List of Services
    - Service Ovg.
    - FIDC
    - Packet
  - **Level 2**: รายการย่อย
    - ภายใต้ SubCh: ไม่มี sub-items
    - ภายใต้ List of Services: รายการ Service IDs
    - ภายใต้ Service Ovg.: 
      - (0x1746) 'TrustRadio
      - (0xd210) 'DEUTSCHLANDFUN...'
      - (0xd230) 'DKULTUR
      - (0xd350) 'DR Wiss.'
      - (0xd360) 'NDR INFO '
      - (0xd387) 'NDR Traffic '
      - (0xd391) 'SWR 1 RP '
      - (0xd401020) ' Mobilitinfo '
      - (0xd41003) 'DATA Data Service'
      - (0xd411003) 'DATA DRM MFIS 1 '
      - (0xd41003) 'DATA DRM MFIS 11'
      - (0xd41003) 'Bibl-Prog. Guide'
    - ภายใต้ FIDC: FIDC-Ovg
    - ภายใต้ Packet: หลาย SC1d entries
  - **Level 3**: SubCh details
    - ภายใต้ Audio: SubCh 1-10
  - **Level 4**: (ไม่มีในรูปนี้)

---

### 2. Center Top Panel - Sub-Channel Table

#### 2.1 Sub-Channel Tab
- **QTabWidget** แสดง tab "Sub-Channel"

#### 2.2 Sub-Channel Table
- **QTableWidget**:
  - **Columns**: SubCh Id | SAD | Bitrate | P.Level | P.Type | SubCh Type
  - **Rows**:
    - 1: 0, 112 kbps, 3, UEP, Audio
    - 2: 448, 128 kbps, 3, UEP, Audio
    - 3: 544, 128 kbps, 3, UEP, Audio
    - 4: 640, 128 kbps, 2-A, EEP, Audio
    - 5: 722, 152 kbps, 3, UEP, Audio
    - 6: 820, 152 kbps, 2-A, EEP, Packet
    - 7: 904, 104 kbps, 3-A, EEP, Audio
    - 8: 0, 128 kbps, 3, UEP, Audio
    - 9: 700, 16 kbps, 3-A, EEP, Packet
    - 10: 448, 192 kbps, 3, UEP, Audio
    - 12: 300, 80 kbps, 3-A, EEP, Packet

#### 2.3 List of Services (ล่างกลาง)
- **QTableWidget**:
  - **Columns**: SId | Service Label
  - **Rows**:
    - 0xd387: 'NDR Traffic '

---

### 3. Center Bottom Panel - Service Table

#### 3.1 Service Tab
- **QTabWidget** แสดง tab "Service"

#### 3.2 Service Table
- **QTableWidget**:
  - **Columns**: SId | Service Label | Short Label | CAID
  - **Rows** (หลายแถว):
    - 0x1746: 'TrustRadio, 'TrustRadio, No access control
    - 0xd210: 'DEUTSCHLANDFUN...', 'DLF', No access control
    - 0xd230: 'DKULTUR, 'DKULTUR, No access control
    - 0xd350: 'DR Wiss.', 'DR Wiss.', No access control
    - 0xd360: 'NDR INFO ', 'NDR INFO', No access control
    - 0xd387: 'NDR Traffic ', 'Traffic', No access control
    - 0xd391: 'SWR 1 RP ', 'SWR 1 RP', No access control
    - 0xd401: 'NLR 1 NDS ', 'NLR1 NDS', No access control
    - (...และอื่นๆ)
    - 0xd41003: 'DATA DRM MFIS 11', 'DATA 11', No access control
    - 0xd51003: 'DATA DRM MFIS 1 ', 'DATA 1', No access control

#### 3.3 List of Service Components (ล่างกลาง)
- **QTableWidget**:
  - **Columns**: Type | Id | PA&Dr | P/S | SC1dS | SC Label
  - **Rows**:
    - Audio: 4, -, P, 1, 1, NDR Traffic

---

### 4. Right Panel - Service Component Details

#### 4.1 Service Component Panel (บนขวา)
- **QGroupBox** หัวข้อ "Service Component"
- **QTableWidget**:
  - **Columns**: Type | SubCH Id | FIGCH | PADDR | SC1d | CA | CAOrg | Bitrate | ASCTy+ | InCA-...
  - **Rows**:
    - Audio: 4, -, -, -, 64 kbps, No, -, 64 kbps, 0x00
    - Audio: 5, -, -, -, 192 kbps, No, -, 192 kbps, 0x00
    - Audio: 6, -, -, -, 192 kbps, No, -, 192 kbps, 0x00
    - Audio: 10, -, -, -, 192 kbps, No, -, 192 kbps, 0x00
    - Packet: 6, -, 0x001, 0x02E, No, -, -, 0x00, 0x2C
  - ข้อมูลบางคอลัมน์มี scroll bar

#### 4.2 List of Services (กลางขวา)
- **QTableWidget**:
  - **Columns**: SId | Service | P/S | SC1dS | SC Label | SC Short Label
  - **Rows**:
    - 0xd387: 'NDR Traffic ', P, 1, NDR Traffic, Traffic

---

### 5. Bottom Right Panel - Ensemble & FIG Content

#### 5.1 Ensemble Panel (ล่างขวา)
- **QGroupBox** หัวข้อ "Ensemble"
- **QTableWidget** (หรือ Labels):
  - **Rows**: Property | Value
  - Ensemble Label: '10N Norders: '
  - Short Label: '10N Nds'
  - ECC: -
  - Country: -
  - Recording: no change
  - Number of Services: 12
  - Number of Sub-Channel: 12

#### 5.2 FIG Content Panel (ล่างขวาสุด)
- **QGroupBox** หัวข้อ "FIG Content"
- **QTableWidget**:
  - **Columns**: Type | Extension | Number | Subject
  - **Rows** (scrollable, หลายแถว):
    - 0: 0, 40, Ensemble information
    - 0: 1, 132, Sub-Channel information
    - 0: 2, 19, Service organisation
    - 0: 2, 17, Service organisation (audio)
    - 0: 6, 19, Service linking information
    - 0: 6, 26, SC global definition
    - 0: 8, -, Country, LTO and international table
    - 0: 10, -, Date and Time
    - 0: 13, 49, User Application Data
    - 0: 14, 48, FEC sub-channel organisation
    - 0: 16, -, Announcement support
    - 0: 24, 2, Transmitter identification information
    - 0: 22, -, Service label
    - 1: 0, 4, Ensemble label
    - 1: 1, 26, Service label
    - 1: 4, 12, Service label
    - 1: 5, 20, Data service label
    - 5: 1, 1, Traffic Message Channel

---

## สรุป Widgets และโครงสร้าง (Qt6)

### Widgets หลักที่ใช้:

1. **QMainWindow** - หน้าต่างหลัก
2. **QSplitter** - แบ่งพื้นที่ 3 ส่วน (Left | Center | Right)
3. **QTreeWidget** - แสดง hierarchical data (3-4 levels)
4. **QTableWidget** - แสดงข้อมูลตาราง
5. **QTabWidget** - สลับระหว่าง Ensemble / Service Component / Sub-Channel tabs
6. **QGroupBox** - จัดกลุ่ม widgets
7. **QLabel** - แสดงข้อความ
8. **QScrollArea** - สำหรับ scroll ข้อมูลที่ยาว

---

## สรุป Columns ของแต่ละ Table

### 1. Ensemble Table
- Type | SubCH Id | FIGCH | PADDR | SC1d | CA | CAOrg | Bitrate | ASCTy+ | Info

### 2. Sub-Channel Table
- SubCh Id | SAD | Bitrate | P.Level | P.Type | SubCh Type | (บางครั้งมี) SId | Service Label | Short Label | CAID

### 3. Service Table
- SId | Service Label | Short Label | CAID

### 4. List of Services Table (แบบสั้น)
- SId | Service | P/S | SC1dS | SC Label | SC Short Label

### 5. Service Component Table
- Type | SubCH Id | FIGCH | PADDR | SC1d | CA | CAOrg | Bitrate | ASCTy+ | InCA-...

### 6. FIG Content Table
- Type | Extension | Number | Subject

### 7. List of Service Components Table
- Type | Id | PA&Dr | P/S | SC1dS | SC Label

### 8. Ensemble Properties (Label pairs)
- Property | Value

---

## โครงสร้าง Tree Widgets

### Tree 1: Service Tree (รูปที่ 1)
- **Level 1**: SubCh 1-9
- **Level 2**: Audio/Data/Packet (ตัวเลข ID)
- **Level 3**: (อาจมี sub-items เพิ่มเติม)

### Tree 2: FIC Content Tree (รูปที่ 2)
- **Level 1**: หมวดหมู่หลัก
  - SubCh 1-12
  - List of Services
  - Service Ovg.
  - FIDC
  - Packet
- **Level 2**: รายการย่อย
  - Service IDs (hex format)
  - Service names with IDs
- **Level 3**: Audio SubChannels
  - SubCh 1-10
- **Level 4**: (สำหรับ expansion ในอนาคต)

### Tree 3: Service Components Tree (รูปที่ 1)
- **Level 1**: Audio, FIDC, Packet
- **Level 2**: SubCh 1-8
- **Level 3**: SC1d และ PA#d entries

---

## Layout Structure แนะนำ:

```
QMainWindow (FIC-XPlorer)
├── QSplitter (Horizontal - 3 sections)
│   ├── Left Panel (QSplitter Vertical)
│   │   ├── Overview (QGroupBox)
│   │   │   └── QLabel (แสดงข้อมูล overview)
│   │   ├── Service Tree (QTreeWidget - 3 levels)
│   │   │   ├── Level 1: SubCh nodes
│   │   │   ├── Level 2: Service types
│   │   │   └── Level 3: Components
│   │   └── Digital Org Info / Service Components
│   │       ├── QLabel list
│   │       └── QTreeWidget
│   ├── Center Panel (QSplitter Vertical)
│   │   ├── Top Section (QTabWidget)
│   │   │   ├── Ensemble Tab
│   │   │   │   ├── Ensemble Table (QTableWidget)
│   │   │   │   └── List of Services (QTableWidget)
│   │   │   └── Service Component Tab
│   │   │       └── Service Component Table (QTableWidget)
│   │   └── Bottom Section (QTabWidget)
│   │       ├── Sub-Channel Tab
│   │       │   ├── Sub-Channel Table (QTableWidget)
│   │       │   └── List of Services (QTableWidget)
│   │       └── Service Tab
│   │           ├── Service Table (QTableWidget)
│   │           └── List of Service Components (QTableWidget)
│   └── Right Panel (QSplitter Vertical)
│       ├── FIC Content Table (QTableWidget)
│       ├── Service Component Details (QTableWidget)
│       ├── List of Services (QTableWidget)
│       ├── Ensemble Properties (QGroupBox)
│       │   └── Property-Value Labels
│       └── FIG Content Table (QTableWidget)
└── QStatusBar (optional)
```

---

## หมายเหตุสำหรับการพัฒนา

1. **Tree Widgets** ต้องรองรับ 3-4 levels ของ hierarchy
2. **Tables** ต้องมี horizontal scrolling สำหรับคอลัมน์ที่กว้าง
3. **Tabs** ใช้สำหรับสลับระหว่าง views ที่แตกต่างกัน
4. **Hex Display** แสดงเป็น format 0x... (เช่น 0xd387, 0x2022)
5. **Service IDs** แสดงทั้งแบบ decimal และ hexadecimal
6. **Scrollable Areas** สำหรับรายการที่ยาว
7. **Resizable Splitters** ให้ผู้ใช้ปรับขนาดแต่ละ panel ได้
8. **Color Coding** อาจใช้สีเพื่อแยกประเภทของ Service (Audio/Data/Packet)
9. **Context Menus** สำหรับ right-click บน tree/table items
10. **Export Functions** สำหรับ export ข้อมูลเป็น CSV หรือ text file

---

## การเชื่อมโยงกับ REF-UI1

FIC-XPlorer สามารถรวมเข้ากับ DAB-XPlorer/ETI-XPlorer โดย:
1. เพิ่มเป็น **QDockWidget** ที่สามารถ dock/undock ได้
2. เพิ่มเป็น **Tab** ใน main QTabWidget
3. แสดงเป็น **Separate Window** ที่เปิดจาก menu
4. รวมข้อมูล FIC ใน **System Panel** ของ main window

แนะนำให้ใช้วิธีที่ 1 (QDockWidget) เพื่อความยืดหยุ่นสูงสุด
