#pragma once

#include <QTime>
#include <cstdint>

/**
 * @brief ETI Frame Data for Alert System processing
 */
struct AlertEtiFrameData {
    struct SignalQuality {
        double snr = 0.0;      // Signal-to-Noise Ratio in dB
        double ber = 0.0;      // Bit Error Rate (0.0 to 1.0)
        double evm = 0.0;      // Error Vector Magnitude
        double rssi = 0.0;     // Received Signal Strength Indicator
    } signalQuality;
    
    QTime timestamp;
    uint32_t frameNumber = 0;
    bool isValid = false;
    
    AlertEtiFrameData() : timestamp(QTime::currentTime()) {}
};

/**
 * @brief Audio Level Data for Alert System monitoring
 */
struct AudioLevelData {
    double peakLevel = -120.0;    // Peak level in dBFS
    double rmsLevel = -120.0;     // RMS level in dBFS
    double lufs = -120.0;         // LUFS measurement
    QTime timestamp;
    uint16_t serviceId = 0;
    uint8_t componentId = 0;
    bool isValid = false;
    
    AudioLevelData() : timestamp(QTime::currentTime()) {}
};