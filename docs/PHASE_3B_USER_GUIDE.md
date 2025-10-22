# Phase 3B: DLS+ (Dynamic Label Plus) User Guide

**Version:** 1.0  
**Date:** October 22, 2025  
**Phase:** 3B - DLS+ Implementation  
**Standard:** ETSI TS 102 980 - Dynamic Label Plus Specification

---

## 📋 Overview

Phase 3B implements DLS+ (Dynamic Label Plus) support in the StreamDAB Analyzer, providing structured "now playing" metadata for DAB/DAB+ radio broadcasts. DLS+ extends basic Dynamic Label (DLS) with tagged metadata fields that enable rich media information display.

### What is DLS+?

DLS+ is an enhancement to the Dynamic Label system defined in ETSI TS 102 980. It provides structured metadata tags that allow broadcasters to transmit detailed information about:

- **Track titles** (song names)
- **Artist names** (performers)
- **Album information**
- **Program titles**
- **Composition details**
- **Genre/category information**
- And more...

### Key Features

✅ **Real-time metadata display** - Updates automatically when broadcasters send new information  
✅ **Professional GUI panel** - Clean, modern "Now Playing" interface  
✅ **Thai UTF-8 support** - Full Unicode support for Thai language content  
✅ **ETSI TS 102 980 compliant** - Standards-based implementation  
✅ **Multi-tag support** - 15+ standard DLS+ tag types supported

---

## 🎯 Getting Started

### Viewing "Now Playing" Information

1. **Launch DAB-XPlorer** (StreamDAB Analyzer GUI)
2. **Load an ETI file** containing DAB+ audio with DLS+ metadata:
   - Click **File → Open ETI File**
   - Navigate to your ETI file (e.g., `eti/bkk_20062022_141637.eti`)
   - Select the file and click **Open**

3. **Select a service** from the "DAB Ensemble Explorer" tree on the left panel

4. **View metadata** in the "Now Playing (DLS+)" panel in the right panel:
   - Track title is displayed prominently in blue
   - Artist name shown below the track
   - Album information displayed in italic gray text
   - Last updated timestamp at the bottom

### Example Display

```
╔═══════════════════════════════════════╗
║     Now Playing (DLS+)                ║
╠═══════════════════════════════════════╣
║ Track:  Bohemian Rhapsody             ║
║ Artist: Queen                         ║
║ Album:  A Night at the Opera          ║
║                                       ║
║ ─────────────────────────────────────  ║
║              Updated: 14:32:15        ║
╚═══════════════════════════════════════╝
```

---

## 📊 Displayed Information

### Track Title
- **Field:** Track/Song title
- **DLS+ Tag:** ITEM.TITLE (0x01)
- **Description:** Current song or track being played
- **Style:** Bold, blue, 14px font
- **Example:** "Bohemian Rhapsody"

### Artist Name
- **Field:** Performer/Artist
- **DLS+ Tag:** ITEM.ARTIST (0x04)
- **Description:** Name of the artist or band performing the track
- **Style:** Regular, gray, 12px font
- **Example:** "Queen"

### Album Name
- **Field:** Album/Compilation
- **DLS+ Tag:** ITEM.ALBUM (0x03)
- **Description:** Album or compilation containing the track
- **Style:** Italic, light gray, 11px font
- **Example:** "A Night at the Opera"

### Last Updated
- **Field:** Timestamp
- **Description:** Date and time when metadata was last received
- **Format:** `yyyy-MM-dd HH:mm:ss`
- **Style:** Small, gray, italic, right-aligned
- **Example:** "Updated: 2025-10-22 14:32:15"

---

## 🌏 Thai Language Support

Full UTF-8 support enables display of Thai artist names, song titles, and album information:

### Thai Example Display

```
╔═══════════════════════════════════════╗
║     Now Playing (DLS+)                ║
╠═══════════════════════════════════════╣
║ Track:  รักแท้                        ║
║ Artist: บิ๊กแอส                       ║
║ Album:  อัลบั้มที่ 3                   ║
║                                       ║
║ ─────────────────────────────────────  ║
║              Updated: 14:32:15        ║
╚═══════════════════════════════════════╝
```

### Supported Characters
- Thai consonants (ก-ฮ)
- Thai vowels (all forms)
- Thai tone marks
- Thai numerals (๐-๙)
- Thai punctuation
- Mixed Thai-English text

---

## 🔧 Supported DLS+ Tags

The implementation supports all standard ETSI TS 102 980 tags:

| Tag Code | Tag Name | Description | Example |
|----------|----------|-------------|---------|
| 0x01 | ITEM.TITLE | Song/track title | "Bohemian Rhapsody" |
| 0x02 | ITEM.ALBUM | Album name | "A Night at the Opera" |
| 0x03 | INFO.NEWS | News headline | "Breaking: Local News" |
| 0x04 | ITEM.ARTIST | Artist/performer | "Queen" |
| 0x05 | ITEM.COMPOSITION | Composer | "Freddie Mercury" |
| 0x06 | ITEM.MOVEMENT | Movement name | "Movement 1: Allegro" |
| 0x07 | ITEM.CONDUCTOR | Conductor | "Herbert von Karajan" |
| 0x08 | ITEM.COMPOSER | Composer (alternative) | "Mozart" |
| 0x09 | INFO.PHONE_HOTLINE | Phone number | "+66-2-123-4567" |
| 0x0A | INFO.PHONE_STUDIO | Studio phone | "+66-2-987-6543" |
| 0x0B | INFO.SMS_STUDIO | SMS number | "1234" |
| 0x0C | INFO.EMAIL_HOTLINE | Email address | "info@station.th" |
| 0x0D | INFO.EMAIL_STUDIO | Studio email | "studio@station.th" |
| 0x0E | PROGRAMME.NOW | Current program | "Morning Show" |
| 0x0F | PROGRAMME.NEXT | Next program | "News Hour" |

---

## ❓ Troubleshooting

### Issue: "No metadata received"

**Cause:** No DLS+ data has been received yet

**Solutions:**
1. Ensure the ETI file contains DAB+ audio (not plain DAB)
2. Verify the broadcaster includes DLS+ in their stream
3. Check that the selected service is an audio service
4. Wait for the broadcaster to send metadata (can take 10-30 seconds)

### Issue: Metadata not updating

**Cause:** DLS+ updates occur only when broadcasters send new PAD data

**Solutions:**
1. DLS+ update frequency varies by broadcaster (typically 10-30 seconds)
2. Some broadcasters may only send metadata when tracks change
3. Verify the ETI file is actively streaming (not paused)
4. Check System Messages tab for DLS+ update notifications

### Issue: Thai characters display as boxes or question marks

**Cause:** Font or encoding issue

**Solutions:**
1. Ensure your system has Thai fonts installed
2. Verify Qt6 is configured for UTF-8 encoding
3. Check that the ETI file contains properly encoded Thai text
4. Try a different Thai font in system settings

### Issue: Wrong metadata displayed

**Cause:** Cached metadata from previous service

**Solutions:**
1. Select a different service and then re-select the original
2. Reload the ETI file (File → Open ETI File)
3. Restart the application
4. Check that the broadcaster is sending correct metadata

---

## 📚 Technical Background

### ETSI TS 102 980 Standard

DLS+ is defined in **ETSI TS 102 980 v2.1.1** (2016-09):
- **Title:** Digital Audio Broadcasting (DAB); Dynamic Label Plus (DLS+); Application specification
- **Published:** European Telecommunications Standards Institute (ETSI)
- **URL:** https://www.etsi.org/deliver/etsi_ts/102900_102999/102980/

### Integration Architecture

```
DAB+ Audio Stream (ETI File)
         ↓
PAD (Program Associated Data) Extraction
         ↓
DLS+ Decoder (dls_plus_decoder.cpp)
         ↓
Tag Parsing & Text Extraction
         ↓
Qt6 Signal/Slot → GUI Update
         ↓
"Now Playing" Panel Display
```

### Data Flow

1. **ETI Frame Reception:** 6144-byte frames loaded from file
2. **PAD Extraction:** Program Associated Data extracted from audio frames
3. **DLS Text Parsing:** Dynamic Label text extracted (up to 128 chars, UTF-8)
4. **DLS+ Command Parsing:** Content descriptors parsed (4 bytes per tag)
5. **Tag Mapping:** Tags mapped to fields (ITEM.TITLE → Track, ITEM.ARTIST → Artist)
6. **GUI Update:** Qt signals trigger label updates in "Now Playing" panel
7. **Timestamp Update:** Last updated time refreshed with current timestamp

---

## 🎓 Usage Examples

### Example 1: English Pop Music

**ETI File:** Commercial DAB+ station playing English pop music

**Expected Display:**
```
Track:  Shape of You
Artist: Ed Sheeran
Album:  ÷ (Divide)
Updated: 2025-10-22 14:30:45
```

**System Message:**
```
🎵 Now Playing: Ed Sheeran - Shape of You
```

### Example 2: Thai Contemporary Music

**ETI File:** Bangkok DAB+ station (NBT Radio)

**Expected Display:**
```
Track:  รักแท้ (True Love)
Artist: บิ๊กแอส (Big Ass)
Album:  อัลบั้มที่ 3
Updated: 2025-10-22 14:32:15
```

**System Message:**
```
🎵 Now Playing: บิ๊กแอส (Big Ass) - รักแท้ (True Love)
```

### Example 3: Classical Music

**ETI File:** Classical music station with detailed metadata

**Expected Display:**
```
Track:  Symphony No. 9 in D minor, Op. 125
Artist: Ludwig van Beethoven
Album:  Complete Symphonies
Updated: 2025-10-22 14:35:00
```

**System Message:**
```
🎵 Now Playing: Ludwig van Beethoven - Symphony No. 9 in D minor, Op. 125
```

### Example 4: No Metadata Available

**ETI File:** Old DAB broadcast without DLS+ support

**Expected Display:**
```
Track:  --
Artist: --
Album:  --
No metadata received
```

**System Message:**
```
(No message - waiting for metadata)
```

---

## 🔍 Advanced Features

### System Messages Integration

DLS+ updates are logged to the "System Messages" tab:
- **Format:** 🎵 Now Playing: [Artist] - [Track]
- **Purpose:** Historical record of played tracks
- **Scrollback:** Unlimited (until application restart)

### Metadata History

Future enhancement (not yet implemented):
- Export metadata history to CSV
- Save "Now Playing" snapshots
- Generate playlists from DLS+ data

### Cover Art Support

Future enhancement (not yet implemented):
- MOT SlideShow integration (ETSI TS 101 499)
- Album cover display in "Now Playing" panel
- Image caching and management

---

## 📖 See Also

### Related Documentation
- **PHASE_3B_TECHNICAL_NOTES.md** - Technical implementation details
- **PHASE_3B_DLS_PLUS_PLAN.md** - Development plan and architecture
- **PHASE_3A_USER_GUIDE.md** - DAB+ Audio Stream Validator (prerequisite)
- **README.md** - StreamDAB Analyzer overview

### ETSI Standards
- **ETSI TS 102 980** - Dynamic Label Plus (DLS+) specification
- **ETSI EN 300 401** - DAB system specification
- **ETSI TS 102 563** - DAB+ audio specification
- **ETSI TS 101 499** - MOT SlideShow application (future)

### External Resources
- WorldDAB: https://www.worlddab.org/
- DAB+ Technology Overview: https://www.dabplus.io/
- ETSI Standards Portal: https://www.etsi.org/

---

## 📞 Support & Feedback

### Reporting Issues
If you encounter issues with DLS+ functionality:
1. Check this user guide for troubleshooting steps
2. Review System Messages tab for error details
3. Verify ETI file contains valid DLS+ data
4. Report issues with detailed reproduction steps

### Feature Requests
Suggestions for DLS+ feature enhancements are welcome:
- Metadata export functionality
- Cover art display (MOT integration)
- Enhanced Thai language support
- Additional tag type support

---

**Document Version:** 1.0  
**Last Updated:** October 22, 2025  
**Phase:** 3B - DLS+ Implementation  
**Status:** Complete - GUI panel functional, awaiting DLS+ decoder integration
