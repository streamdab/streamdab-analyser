# StreamDAB Stream Analyzer - License Compliance Report

Generated: อ. 23 ก.ย. 2568 17:13:38 +07

## Core Dependencies

| Component | Version | License | Compatibility | Notes |
|-----------|---------|---------|---------------|-------|
| Qt6 | 6.4.2 | LGPL-3.0/GPL-3.0 | ✅ Compatible | Core GUI framework |
| FFTW3 | 3.3.10 | GPL-2.0+ | ✅ Compatible | Signal processing |
| ALSA | 1.2.11 | LGPL-2.1 | ✅ Compatible | Linux audio |

## Audio Decoders (Alternative Options)

| Component | Version | License | Compatibility | Notes |
|-----------|---------|---------|---------------|-------|
| FDK-AAC | 2.0.2 | Modified BSD | ⚠️ Restrictions | Patent considerations |
| FAAD2 | Unknown | GPL-2.0 | ✅ Compatible | Recommended for OSS |

## Optional Dependencies

| Component | Version | License | Compatibility | Notes |
|-----------|---------|---------|---------------|-------|
| YAML-CPP | Unknown | MIT | ✅ Compatible | Configuration files |

## Recommendations

1. **For Open Source Distribution**: Use GPL-3.0 license with FAAD2 decoder
2. **For Commercial Distribution**: Evaluate FDK-AAC patent licensing requirements
3. **Attribution Requirements**: Include third-party license attributions in distribution
4. **Documentation**: Maintain this compliance report with version updates

## Legal Disclaimer

This report is for informational purposes only. Consult with qualified legal counsel for
commercial distribution decisions, especially regarding patent licensing requirements.
