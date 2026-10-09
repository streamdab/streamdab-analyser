# Attributions and third-party notices

StreamDAB Analyser is released under the **GNU General Public License v3.0 or
later** (see [LICENSE](LICENSE)). It builds on, and was informed by, the work of
the open-source DAB community and incorporates third-party libraries. Their
licences are listed below. All trademarks belong to their respective owners.

## Reference implementations and prior art

StreamDAB Analyser is an independent implementation. Where the standards leave
room for interpretation, behaviour was cross-checked against these projects, and
several parsers follow their field naming/approach:

| Project | What it is | Link | Licence |
|---|---|---|---|
| **ETISnoop** | ETI file analyser (the reference used for ETI/FIG parity checks, incl. header CRC and superframe handling) | https://github.com/Opendigitalradio/etisnoop | GPL-2.0-or-later |
| **eti-tools** | ETI conversion utilities (`na2ni`, `edi2eti`, `eti2zmq`, `ni2out`, …) used to prepare and validate test streams | https://github.com/piratfm/eti-tools | GPL-2.0 |
| **DABlin** | DAB/DAB+ decoder (referenced for the AudioSpecificConfig construction and DAB+ superframe field naming) | https://github.com/Opendigitalradio/dablin | GPL-3.0-or-later |
| **dabtools** | DAB reception/ETI recording tools (`dab2eti`) | https://github.com/linuxstb/dabtools | GPL-2.0 |
| **ODR-DabMux / ODR-DabMod** | DAB multiplexer and modulator from the Opendigitalradio toolchain (ZeroMQ ETI sources were tested against them) | https://github.com/Opendigitalradio | GPL-3.0-or-later |

With thanks to their authors and to the wider DAB community — in particular the
OpenDigitalRadio maintainers, who keep the ecosystem documented and testable.

## Standards

The analyser implements or validates against the following ETSI specifications.
The standards themselves are **not** redistributed with this project; official
PDFs are available free of charge from ETSI:

| Standard | Subject | Link |
|---|---|---|
| ETSI EN 300 799 | Ensemble Transport Interface (ETI) | https://www.etsi.org/deliver/etsi_en/300700_300799/300799/ |
| ETSI EN 300 401 | DAB — radio broadcasting systems (FIG, FIC, MSC) | https://www.etsi.org/deliver/etsi_en/300400_300499/300401/ |
| ETSI TS 102 563 | DAB+ audio coding | https://www.etsi.org/deliver/etsi_ts/102500_102599/102563/ |
| ETSI EN 301 234 | Multimedia Object Transfer (MOT) | https://www.etsi.org/deliver/etsi_en/301200_301299/301234/ |
| ETSI TS 102 371 | Digital Audio Broadcasting — EPG | https://www.etsi.org/deliver/etsi_ts/102300_102399/102371/ |
| ETSI TS 102 979 | Journaline | https://www.etsi.org/deliver/etsi_ts/102900_102999/102979/ |
| ETSI TS 102 980 | Dynamic Label Plus (DLS+) | https://www.etsi.org/deliver/etsi_ts/102900_102999/102980/ |
| ETSI TS 102 894 | TPEG (traffic/travel protocol) — framing identification + observed text inventory only; full TTI decoding not claimed | https://www.etsi.org/deliver/etsi_ts/102800_102899/102894/ |

## Third-party libraries

| Library | Used for | Licence |
|---|---|---|
| **Qt 6** (Core, Widgets, Network, Multimedia, Test) | application framework, GUI, networking, audio output | LGPL-3.0 / GPL-3.0 (or commercial) |
| **Qt Advanced Docking System** (Qt-ADS 4.5.0) | docking layout | LGPL-2.1 |
| **libfaad2** | DAB+ HE-AAC decoding | GPL-2.0 |
| **libfftw3** | FFT / spectrum analysis | GPL-2.0-or-later |
| **ZeroMQ** (`libzmq`, `cppzmq`) | `zmq+tcp://` ETI transport | MPL-2.0 |
| **ALSA** (`libasound2`) | Linux audio output | LGPL-2.1 |
| **GoogleTest** | unit/integration test framework | BSD-3-Clause |
| **CMake** | build system | BSD-3-Clause |
| **PkgConfig** | optional dependency detection | GPL-2.0-or-later (build-time only) |

## Test data

The end-to-end test suite is split into PUBLIC (self-contained, runs in CI) and
PRIVATE (capture-dependent) targets. The PRIVATE tests use real broadcast
captures — `eti/bkk_20062022_141637.eti` (a Bangkok ensemble with Thai labels,
DLS+, MOT slideshow, EPG and TPEG), and the Hessischer Rundfunk / Bayerischer
Rundfunk warning-day sets — which are **World DAB licensed test material**. They
are **not** distributed with or from this repository; the PRIVATE suite runs
where the files are available locally. They are test data, not part of the
GPL-licensed source code.

Other third-party tools' screens and manuals used during development are kept
out of the public repository for copyright reasons.

## Your contributions

By submitting a pull request you agree that your contribution is licensed under
the same terms as the project (GPL-3.0-or-later) and that you have the right to
submit it.
