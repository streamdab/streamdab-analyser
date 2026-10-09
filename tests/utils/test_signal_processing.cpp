#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTest>
#include <QtMath>
#include <algorithm>
#include <numeric>
#include "../../src/utils/signal_processing.h"

class SignalProcessingTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        SignalProcessing::clearError();
    }

    void TearDown() override 
    {
        SignalProcessing::clearError();
    }

    // Helper methods
    SignalProcessing::RealVector createSineWave(double frequency, double amplitude, double duration, double sampleRate)
    {
        return SignalProcessing::generateSineWave(frequency, amplitude, duration, sampleRate);
    }

    SignalProcessing::RealVector createTestSignal(int size, double value = 1.0)
    {
        SignalProcessing::RealVector signal;
        signal.reserve(size);
        for (int i = 0; i < size; ++i) {
            signal.append(value);
        }
        return signal;
    }

    SignalProcessing::ComplexVector createComplexSignal(int size)
    {
        SignalProcessing::ComplexVector signal;
        signal.reserve(size);
        for (int i = 0; i < size; ++i) {
            signal.append(SignalProcessing::Complex(static_cast<double>(i), static_cast<double>(i) * 0.5));
        }
        return signal;
    }

    QByteArray createInt16AudioData(const SignalProcessing::RealVector& samples)
    {
        QByteArray data;
        data.resize(samples.size() * 2);
        qint16* intData = reinterpret_cast<qint16*>(data.data());
        
        for (int i = 0; i < samples.size(); ++i) {
            double clamped = qBound(-1.0, samples[i], 1.0);
            intData[i] = static_cast<qint16>(clamped * 32767.0);
        }
        
        return data;
    }

    bool isNearlyEqual(double a, double b, double tolerance = 1e-6)
    {
        return std::abs(a - b) < tolerance;
    }

    bool vectorsNearlyEqual(const SignalProcessing::RealVector& a, const SignalProcessing::RealVector& b, double tolerance = 1e-6)
    {
        if (a.size() != b.size()) return false;
        
        for (int i = 0; i < a.size(); ++i) {
            if (!isNearlyEqual(a[i], b[i], tolerance)) {
                return false;
            }
        }
        return true;
    }
};

// FFT Operations Tests
TEST_F(SignalProcessingTest, FftBasicFunctionality)
{
    // Create a simple 8-point signal
    SignalProcessing::RealVector input = {1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0};
    
    SignalProcessing::ComplexVector result = SignalProcessing::fft(input);
    
    EXPECT_EQ(result.size(), 8);
    EXPECT_FALSE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, FftPowerOfTwoRequirement)
{
    SignalProcessing::RealVector input = {1.0, 2.0, 3.0}; // Size 3, not power of 2
    
    SignalProcessing::ComplexVector result = SignalProcessing::fft(input);
    
    EXPECT_TRUE(result.isEmpty());
    EXPECT_TRUE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, FftComplexInput)
{
    SignalProcessing::ComplexVector input = createComplexSignal(4);
    
    SignalProcessing::ComplexVector result = SignalProcessing::fft(input);
    
    EXPECT_EQ(result.size(), 4);
    EXPECT_FALSE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, IfftInverseFunctionality)
{
    SignalProcessing::RealVector original = {1.0, 2.0, 3.0, 4.0};
    
    SignalProcessing::ComplexVector fftResult = SignalProcessing::fft(original);
    SignalProcessing::ComplexVector ifftResult = SignalProcessing::ifft(fftResult);
    
    EXPECT_EQ(ifftResult.size(), 4);
    
    // Check that IFFT(FFT(x)) ≈ x
    for (int i = 0; i < original.size(); ++i) {
        EXPECT_NEAR(ifftResult[i].real(), original[i], 1e-10);
        EXPECT_NEAR(ifftResult[i].imag(), 0.0, 1e-10);
    }
}

TEST_F(SignalProcessingTest, RealFft)
{
    SignalProcessing::RealVector input = {1.0, 0.0, -1.0, 0.0};
    
    SignalProcessing::RealVector result = SignalProcessing::realFft(input);
    
    EXPECT_EQ(result.size(), 4);
    EXPECT_FALSE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, MagnitudeAndPhase)
{
    SignalProcessing::ComplexVector complex = {
        SignalProcessing::Complex(3.0, 4.0),
        SignalProcessing::Complex(0.0, 1.0),
        SignalProcessing::Complex(1.0, 0.0)
    };
    
    SignalProcessing::RealVector magnitudes = SignalProcessing::magnitude(complex);
    SignalProcessing::RealVector phases = SignalProcessing::phase(complex);
    
    EXPECT_EQ(magnitudes.size(), 3);
    EXPECT_EQ(phases.size(), 3);
    
    EXPECT_NEAR(magnitudes[0], 5.0, 1e-10); // sqrt(3^2 + 4^2) = 5
    EXPECT_NEAR(magnitudes[1], 1.0, 1e-10); // sqrt(0^2 + 1^2) = 1
    EXPECT_NEAR(magnitudes[2], 1.0, 1e-10); // sqrt(1^2 + 0^2) = 1
}

TEST_F(SignalProcessingTest, PowerSpectrum)
{
    SignalProcessing::ComplexVector complex = {
        SignalProcessing::Complex(3.0, 4.0),
        SignalProcessing::Complex(1.0, 1.0)
    };
    
    SignalProcessing::RealVector power = SignalProcessing::powerSpectrum(complex);
    
    EXPECT_EQ(power.size(), 2);
    EXPECT_NEAR(power[0], 25.0, 1e-10); // 3^2 + 4^2 = 25
    EXPECT_NEAR(power[1], 2.0, 1e-10);  // 1^2 + 1^2 = 2
}

// Window Functions Tests
TEST_F(SignalProcessingTest, GenerateHanningWindow)
{
    SignalProcessing::RealVector window = SignalProcessing::generateWindow(SignalProcessing::WindowType::Hanning, 8);
    
    EXPECT_EQ(window.size(), 8);
    EXPECT_NEAR(window[0], 0.0, 1e-10); // Hanning window starts at 0
    EXPECT_NEAR(window[window.size()-1], 0.0, 1e-10); // Hanning window ends at 0
    
    // Check symmetry
    for (int i = 0; i < window.size() / 2; ++i) {
        EXPECT_NEAR(window[i], window[window.size() - 1 - i], 1e-10);
    }
}

TEST_F(SignalProcessingTest, GenerateHammingWindow)
{
    SignalProcessing::RealVector window = SignalProcessing::generateWindow(SignalProcessing::WindowType::Hamming, 10);
    
    EXPECT_EQ(window.size(), 10);
    EXPECT_GT(window[0], 0.0); // Hamming window doesn't start at 0
    
    // All values should be positive
    for (double value : window) {
        EXPECT_GE(value, 0.0);
    }
}

TEST_F(SignalProcessingTest, GenerateNoWindow)
{
    SignalProcessing::RealVector window = SignalProcessing::generateWindow(SignalProcessing::WindowType::None, 5);
    
    EXPECT_EQ(window.size(), 5);
    
    // All values should be 1.0 for no window
    for (double value : window) {
        EXPECT_NEAR(value, 1.0, 1e-10);
    }
}

TEST_F(SignalProcessingTest, ApplyWindowFunction)
{
    SignalProcessing::RealVector signal = createTestSignal(8, 1.0);
    SignalProcessing::RealVector original = signal;
    
    SignalProcessing::applyWindow(signal, SignalProcessing::WindowType::Hanning);
    
    // Signal should be modified
    EXPECT_FALSE(vectorsNearlyEqual(signal, original));
    
    // First and last samples should be near zero for Hanning
    EXPECT_NEAR(signal[0], 0.0, 1e-10);
    EXPECT_NEAR(signal[signal.size()-1], 0.0, 1e-10);
}

TEST_F(SignalProcessingTest, WindowScalingFactor)
{
    double scalingFactor = SignalProcessing::windowScalingFactor(SignalProcessing::WindowType::Hanning, 8);
    
    EXPECT_GT(scalingFactor, 1.0); // Scaling factor should compensate for windowing loss
}

// Audio Level Analysis Tests
TEST_F(SignalProcessingTest, CalculatePeakLevel)
{
    SignalProcessing::RealVector samples = {0.5, -0.8, 0.3, -0.1};
    
    double peakLevel = SignalProcessing::calculatePeakLevel(samples);
    
    // Peak should be 0.8, converted to dB
    double expectedDb = 20.0 * std::log10(0.8);
    EXPECT_NEAR(peakLevel, expectedDb, 1e-6);
}

TEST_F(SignalProcessingTest, CalculateRmsLevel)
{
    SignalProcessing::RealVector samples = {1.0, -1.0, 1.0, -1.0};
    
    double rmsLevel = SignalProcessing::calculateRmsLevel(samples);
    
    // RMS of alternating ±1.0 should be 1.0, which is 0 dB
    EXPECT_NEAR(rmsLevel, 0.0, 1e-6);
}

TEST_F(SignalProcessingTest, DetectClipping)
{
    SignalProcessing::RealVector noClipping = {0.5, -0.8, 0.3};
    SignalProcessing::RealVector withClipping = {0.5, 1.0, 0.3}; // 1.0 is clipping
    
    EXPECT_FALSE(SignalProcessing::detectClipping(noClipping));
    EXPECT_TRUE(SignalProcessing::detectClipping(withClipping));
}

TEST_F(SignalProcessingTest, AnalyzeAudioLevels)
{
    SignalProcessing::RealVector samples = createSineWave(1000.0, 0.5, 0.1, 48000.0);
    
    SignalProcessing::AudioLevels levels = SignalProcessing::analyzeAudioLevels(samples, 48000.0);
    
    EXPECT_GT(levels.peakLevel, -10.0); // Should be reasonable peak level
    EXPECT_GT(levels.rmsLevel, -10.0);  // Should be reasonable RMS level
    EXPECT_FALSE(levels.clipDetected);  // 0.5 amplitude shouldn't clip
}

// Audio Format Conversion Tests
TEST_F(SignalProcessingTest, ConvertInt16ToFloat)
{
    SignalProcessing::RealVector originalSamples = {0.5, -0.5, 0.0, 1.0};
    QByteArray audioData = createInt16AudioData(originalSamples);
    
    SignalProcessing::RealVector convertedSamples = SignalProcessing::convertToFloat(audioData, SignalProcessing::SampleFormat::Int16);
    
    EXPECT_EQ(convertedSamples.size(), originalSamples.size());
    
    // Check conversion accuracy (within quantization error)
    for (int i = 0; i < originalSamples.size(); ++i) {
        EXPECT_NEAR(convertedSamples[i], originalSamples[i], 1.0/32768.0);
    }
}

TEST_F(SignalProcessingTest, ConvertFloatToInt16)
{
    SignalProcessing::RealVector samples = {0.5, -0.5, 0.0, 1.0};
    
    QByteArray audioData = SignalProcessing::convertFromFloat(samples, SignalProcessing::SampleFormat::Int16);
    
    EXPECT_EQ(audioData.size(), samples.size() * 2); // 2 bytes per sample
}

TEST_F(SignalProcessingTest, ConvertEmptyData)
{
    QByteArray emptyData;
    
    SignalProcessing::RealVector result = SignalProcessing::convertToFloat(emptyData, SignalProcessing::SampleFormat::Int16);
    
    EXPECT_TRUE(result.isEmpty());
    EXPECT_TRUE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, StereoToMono)
{
    SignalProcessing::RealVector stereo = {1.0, 0.0, 0.5, -0.5}; // L, R, L, R
    
    SignalProcessing::RealVector mono = SignalProcessing::stereoToMono(stereo);
    
    EXPECT_EQ(mono.size(), 2);
    EXPECT_NEAR(mono[0], 0.5, 1e-10); // (1.0 + 0.0) / 2
    EXPECT_NEAR(mono[1], 0.0, 1e-10); // (0.5 + -0.5) / 2
}

TEST_F(SignalProcessingTest, MonoToStereo)
{
    SignalProcessing::RealVector mono = {0.5, -0.3};
    
    SignalProcessing::RealVector stereo = SignalProcessing::monoToStereo(mono);
    
    EXPECT_EQ(stereo.size(), 4);
    EXPECT_NEAR(stereo[0], 0.5, 1e-10);  // L
    EXPECT_NEAR(stereo[1], 0.5, 1e-10);  // R
    EXPECT_NEAR(stereo[2], -0.3, 1e-10); // L
    EXPECT_NEAR(stereo[3], -0.3, 1e-10); // R
}

TEST_F(SignalProcessingTest, ResampleSignal)
{
    SignalProcessing::RealVector input = {1.0, 2.0, 3.0, 4.0};
    
    // Upsample 2x
    SignalProcessing::RealVector upsampled = SignalProcessing::resample(input, 1000.0, 2000.0);
    EXPECT_EQ(upsampled.size(), 8);
    
    // Downsample 2x
    SignalProcessing::RealVector downsampled = SignalProcessing::resample(input, 2000.0, 1000.0);
    EXPECT_EQ(downsampled.size(), 2);
}

// Filtering Tests
TEST_F(SignalProcessingTest, LowPassFilter)
{
    SignalProcessing::RealVector input = createSineWave(1000.0, 1.0, 0.01, 48000.0);
    
    SignalProcessing::RealVector filtered = SignalProcessing::lowPassFilter(input, 500.0, 48000.0);
    
    EXPECT_EQ(filtered.size(), input.size());
    EXPECT_FALSE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, HighPassFilter)
{
    SignalProcessing::RealVector input = createSineWave(100.0, 1.0, 0.01, 48000.0);
    
    SignalProcessing::RealVector filtered = SignalProcessing::highPassFilter(input, 200.0, 48000.0);
    
    EXPECT_EQ(filtered.size(), input.size());
    EXPECT_FALSE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, BandPassFilter)
{
    SignalProcessing::RealVector input = createSineWave(1000.0, 1.0, 0.01, 48000.0);
    
    SignalProcessing::RealVector filtered = SignalProcessing::bandPassFilter(input, 500.0, 1500.0, 48000.0);
    
    EXPECT_EQ(filtered.size(), input.size());
    EXPECT_FALSE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, FilterInvalidParameters)
{
    SignalProcessing::RealVector input = createTestSignal(100);
    
    // Invalid frequency (too high)
    SignalProcessing::RealVector result = SignalProcessing::lowPassFilter(input, 50000.0, 48000.0);
    
    EXPECT_EQ(result.size(), input.size()); // Should return unfiltered input
    EXPECT_TRUE(SignalProcessing::hasError());
}

// Spectrum Analysis Tests
TEST_F(SignalProcessingTest, AnalyzeSpectrum)
{
    // Create a 1kHz sine wave
    SignalProcessing::RealVector signal = createSineWave(1000.0, 1.0, 0.1, 8192.0);
    
    SignalProcessing::SpectrumAnalysis analysis = SignalProcessing::analyzeSpectrum(signal, 8192.0);
    
    EXPECT_FALSE(analysis.frequencies.isEmpty());
    EXPECT_FALSE(analysis.magnitudes.isEmpty());
    EXPECT_EQ(analysis.frequencies.size(), analysis.magnitudes.size());
    
    // Fundamental frequency should be around 1000 Hz
    EXPECT_NEAR(analysis.fundamentalFreq, 1000.0, 50.0);
}

TEST_F(SignalProcessingTest, FindFundamentalFrequency)
{
    // Create spectrum with peak at 1000 Hz
    SignalProcessing::RealVector spectrum(1000, -60.0); // -60 dB background
    SignalProcessing::RealVector frequencies;
    
    for (int i = 0; i < 1000; ++i) {
        frequencies.append(i * 10.0); // 0 to 9990 Hz in 10 Hz steps
    }
    
    // Add peak at 1000 Hz (index 100)
    spectrum[100] = 0.0; // 0 dB peak
    
    double fundamental = SignalProcessing::findFundamentalFrequency(spectrum, frequencies);
    EXPECT_NEAR(fundamental, 1000.0, 0.1);
}

TEST_F(SignalProcessingTest, CalculateTotalHarmonicDistortion)
{
    SignalProcessing::RealVector spectrum(1000, -60.0);
    SignalProcessing::RealVector frequencies;
    
    for (int i = 0; i < 1000; ++i) {
        frequencies.append(i * 10.0);
    }
    
    // Fundamental at 1000 Hz
    spectrum[100] = 0.0;   // 0 dB
    // 2nd harmonic at 2000 Hz
    spectrum[200] = -20.0; // -20 dB
    
    double thd = SignalProcessing::calculateTotalHarmonicDistortion(spectrum, frequencies, 1000.0);
    EXPECT_GT(thd, 0.0);
    EXPECT_LT(thd, 100.0); // Should be reasonable THD percentage
}

// Signal Generation Tests
TEST_F(SignalProcessingTest, GenerateSineWave)
{
    double frequency = 1000.0;
    double amplitude = 0.5;
    double duration = 0.001; // 1ms
    double sampleRate = 48000.0;
    
    SignalProcessing::RealVector sineWave = SignalProcessing::generateSineWave(frequency, amplitude, duration, sampleRate);
    
    int expectedSamples = static_cast<int>(duration * sampleRate);
    EXPECT_EQ(sineWave.size(), expectedSamples);
    
    // Check amplitude bounds
    for (double sample : sineWave) {
        EXPECT_LE(std::abs(sample), amplitude + 1e-10);
    }
}

TEST_F(SignalProcessingTest, GenerateWhiteNoise)
{
    double amplitude = 0.5;
    double duration = 0.01;
    double sampleRate = 48000.0;
    
    SignalProcessing::RealVector noise = SignalProcessing::generateWhiteNoise(amplitude, duration, sampleRate);
    
    int expectedSamples = static_cast<int>(duration * sampleRate);
    EXPECT_EQ(noise.size(), expectedSamples);
    
    // Check that noise is not constant (very high probability)
    bool hasVariation = false;
    for (int i = 1; i < noise.size(); ++i) {
        if (std::abs(noise[i] - noise[i-1]) > 1e-10) {
            hasVariation = true;
            break;
        }
    }
    EXPECT_TRUE(hasVariation);
}

TEST_F(SignalProcessingTest, GeneratePinkNoise)
{
    SignalProcessing::RealVector pinkNoise = SignalProcessing::generatePinkNoise(0.5, 0.01, 48000.0);
    
    EXPECT_FALSE(pinkNoise.isEmpty());
    EXPECT_EQ(pinkNoise.size(), 480); // 0.01 * 48000
}

TEST_F(SignalProcessingTest, GenerateChirp)
{
    SignalProcessing::RealVector chirp = SignalProcessing::generateChirp(100.0, 1000.0, 0.01, 48000.0);
    
    EXPECT_FALSE(chirp.isEmpty());
    EXPECT_EQ(chirp.size(), 480);
}

// Signal Analysis Tests
TEST_F(SignalProcessingTest, CalculateSnr)
{
    SignalProcessing::RealVector signal = createSineWave(1000.0, 1.0, 0.01, 48000.0);
    SignalProcessing::RealVector noise = SignalProcessing::generateWhiteNoise(0.1, 0.01, 48000.0);
    
    double snr = SignalProcessing::calculateSnr(signal, noise);
    
    // SNR should be positive for signal larger than noise
    EXPECT_GT(snr, 0.0);
}

TEST_F(SignalProcessingTest, CalculateCrossCorrelation)
{
    SignalProcessing::RealVector signal1 = createSineWave(1000.0, 1.0, 0.01, 48000.0);
    SignalProcessing::RealVector signal2 = signal1; // Identical signals
    
    double correlation = SignalProcessing::calculateCrossCorrelation(signal1, signal2);
    
    // Identical signals should have correlation close to 1.0
    EXPECT_NEAR(correlation, 1.0, 0.1);
}

TEST_F(SignalProcessingTest, CalculateAutoCorrelation)
{
    SignalProcessing::RealVector signal = createSineWave(1000.0, 1.0, 0.01, 48000.0);
    
    SignalProcessing::RealVector autocorr = SignalProcessing::calculateAutoCorrelation(signal, 100);
    
    EXPECT_EQ(autocorr.size(), 101); // maxLag + 1
    EXPECT_NEAR(autocorr[0], 1.0, 0.01); // Zero lag should be 1.0 (normalized)
}

// Utility Functions Tests
TEST_F(SignalProcessingTest, LinearToDbConversion)
{
    EXPECT_NEAR(SignalProcessing::linearToDb(1.0), 0.0, 1e-10);
    EXPECT_NEAR(SignalProcessing::linearToDb(0.1), -20.0, 1e-6);
    EXPECT_NEAR(SignalProcessing::linearToDb(10.0), 20.0, 1e-6);
    EXPECT_EQ(SignalProcessing::linearToDb(0.0), -INFINITY);
}

TEST_F(SignalProcessingTest, DbToLinearConversion)
{
    EXPECT_NEAR(SignalProcessing::dbToLinear(0.0), 1.0, 1e-10);
    EXPECT_NEAR(SignalProcessing::dbToLinear(-20.0), 0.1, 1e-6);
    EXPECT_NEAR(SignalProcessing::dbToLinear(20.0), 10.0, 1e-6);
}

TEST_F(SignalProcessingTest, AmplitudeDbConversions)
{
    double amplitude = 0.5;
    double db = SignalProcessing::amplitudeToDb(amplitude);
    double backToAmplitude = SignalProcessing::dbToAmplitude(db);
    
    EXPECT_NEAR(backToAmplitude, amplitude, 1e-10);
}

TEST_F(SignalProcessingTest, NormalizeSignal)
{
    SignalProcessing::RealVector signal = {0.5, -1.0, 0.8, -0.3};
    
    SignalProcessing::RealVector normalized = SignalProcessing::normalize(signal, 0.5);
    
    // Find peak of normalized signal
    double peak = 0.0;
    for (double sample : normalized) {
        peak = std::max(peak, std::abs(sample));
    }
    
    EXPECT_NEAR(peak, 0.5, 1e-10);
}

TEST_F(SignalProcessingTest, RemoveDcOffset)
{
    SignalProcessing::RealVector signal = {1.0, 2.0, 3.0, 4.0}; // DC offset = 2.5
    
    SignalProcessing::RealVector noDc = SignalProcessing::removeDcOffset(signal);
    
    // Calculate mean of result
    double mean = std::accumulate(noDc.begin(), noDc.end(), 0.0) / noDc.size();
    EXPECT_NEAR(mean, 0.0, 1e-10);
}

TEST_F(SignalProcessingTest, IsPowerOfTwo)
{
    EXPECT_TRUE(SignalProcessing::isPowerOfTwo(1));
    EXPECT_TRUE(SignalProcessing::isPowerOfTwo(2));
    EXPECT_TRUE(SignalProcessing::isPowerOfTwo(4));
    EXPECT_TRUE(SignalProcessing::isPowerOfTwo(8));
    EXPECT_TRUE(SignalProcessing::isPowerOfTwo(1024));
    
    EXPECT_FALSE(SignalProcessing::isPowerOfTwo(0));
    EXPECT_FALSE(SignalProcessing::isPowerOfTwo(3));
    EXPECT_FALSE(SignalProcessing::isPowerOfTwo(5));
    EXPECT_FALSE(SignalProcessing::isPowerOfTwo(1000));
}

TEST_F(SignalProcessingTest, NextPowerOfTwo)
{
    EXPECT_EQ(SignalProcessing::nextPowerOfTwo(1), 1);
    EXPECT_EQ(SignalProcessing::nextPowerOfTwo(2), 2);
    EXPECT_EQ(SignalProcessing::nextPowerOfTwo(3), 4);
    EXPECT_EQ(SignalProcessing::nextPowerOfTwo(5), 8);
    EXPECT_EQ(SignalProcessing::nextPowerOfTwo(1000), 1024);
}

// DAB/ETI Specific Functions Tests
TEST_F(SignalProcessingTest, DecodeMpeg1Layer2Placeholder)
{
    QByteArray mpegData(100, 0x00);
    
    SignalProcessing::RealVector result = SignalProcessing::decodeMpeg1Layer2(mpegData);
    
    EXPECT_TRUE(result.isEmpty());
    EXPECT_TRUE(SignalProcessing::hasError());
    EXPECT_TRUE(SignalProcessing::getLastErrorString().contains("not implemented"));
}

TEST_F(SignalProcessingTest, DecodeAacPlusPlaceholder)
{
    QByteArray aacData(100, 0x00);
    
    SignalProcessing::RealVector result = SignalProcessing::decodeAacPlus(aacData);
    
    EXPECT_TRUE(result.isEmpty());
    EXPECT_TRUE(SignalProcessing::hasError());
    EXPECT_TRUE(SignalProcessing::getLastErrorString().contains("not implemented"));
}

TEST_F(SignalProcessingTest, AnalyzeDabAudioLevels)
{
    SignalProcessing::RealVector samples = createSineWave(1000.0, 0.5, 0.01, 48000.0);
    QByteArray audioData = createInt16AudioData(samples);
    
    SignalProcessing::AudioLevels levels = SignalProcessing::analyzeDabAudioLevels(
        audioData, SignalProcessing::SampleFormat::Int16, 48000.0);
    
    EXPECT_GT(levels.peakLevel, -10.0);
    EXPECT_GT(levels.rmsLevel, -10.0);
    EXPECT_FALSE(levels.clipDetected);
}

TEST_F(SignalProcessingTest, AnalyzeDabSpectrum)
{
    SignalProcessing::RealVector samples = createSineWave(1000.0, 0.5, 0.1, 8192.0);
    QByteArray audioData = createInt16AudioData(samples);
    
    SignalProcessing::SpectrumAnalysis analysis = SignalProcessing::analyzeDabSpectrum(
        audioData, SignalProcessing::SampleFormat::Int16, 8192.0);
    
    EXPECT_FALSE(analysis.frequencies.isEmpty());
    EXPECT_FALSE(analysis.magnitudes.isEmpty());
    EXPECT_NEAR(analysis.fundamentalFreq, 1000.0, 50.0);
}

// Error Handling Tests
TEST_F(SignalProcessingTest, ErrorHandling)
{
    SignalProcessing::clearError();
    EXPECT_FALSE(SignalProcessing::hasError());
    EXPECT_TRUE(SignalProcessing::getLastErrorString().isEmpty());
    
    // Trigger an error
    SignalProcessing::fft(SignalProcessing::RealVector{1.0, 2.0, 3.0}); // Not power of 2
    
    EXPECT_TRUE(SignalProcessing::hasError());
    EXPECT_FALSE(SignalProcessing::getLastErrorString().isEmpty());
    
    SignalProcessing::clearError();
    EXPECT_FALSE(SignalProcessing::hasError());
    EXPECT_TRUE(SignalProcessing::getLastErrorString().isEmpty());
}

// Edge Cases and Robustness Tests
TEST_F(SignalProcessingTest, EmptyVectorHandling)
{
    SignalProcessing::RealVector empty;
    
    // These should handle empty vectors gracefully
    SignalProcessing::ComplexVector fftResult = SignalProcessing::fft(empty);
    EXPECT_TRUE(fftResult.isEmpty());
    EXPECT_TRUE(SignalProcessing::hasError());
    
    SignalProcessing::clearError();
    
    double peakLevel = SignalProcessing::calculatePeakLevel(empty);
    EXPECT_EQ(peakLevel, -INFINITY);
    
    SignalProcessing::RealVector normalized = SignalProcessing::normalize(empty);
    EXPECT_TRUE(normalized.isEmpty());
}

TEST_F(SignalProcessingTest, SingleSampleHandling)
{
    SignalProcessing::RealVector single = {0.5};
    
    double peak = SignalProcessing::calculatePeakLevel(single);
    double rms = SignalProcessing::calculateRmsLevel(single);
    
    EXPECT_NEAR(peak, SignalProcessing::amplitudeToDb(0.5), 1e-10);
    EXPECT_NEAR(rms, SignalProcessing::amplitudeToDb(0.5), 1e-10);
}

TEST_F(SignalProcessingTest, LargeSignalHandling)
{
    // Test with a large signal (1 second at 48kHz)
    SignalProcessing::RealVector largeSignal = createSineWave(440.0, 0.5, 1.0, 48000.0);
    
    EXPECT_EQ(largeSignal.size(), 48000);
    
    SignalProcessing::AudioLevels levels = SignalProcessing::analyzeAudioLevels(largeSignal, 48000.0);
    EXPECT_GT(levels.peakLevel, -20.0);
    EXPECT_GT(levels.rmsLevel, -20.0);
    
    SignalProcessing::RealVector normalized = SignalProcessing::normalize(largeSignal);
    EXPECT_EQ(normalized.size(), largeSignal.size());
}

TEST_F(SignalProcessingTest, ExtremeValueHandling)
{
    // Test with extreme values
    SignalProcessing::RealVector extremes = {-1000.0, 1000.0, 0.0};
    
    SignalProcessing::RealVector normalized = SignalProcessing::normalize(extremes, 1.0);
    
    // Should be normalized to ±1.0
    double peak = 0.0;
    for (double sample : normalized) {
        peak = std::max(peak, std::abs(sample));
    }
    EXPECT_NEAR(peak, 1.0, 1e-10);
}

// Integration Tests
TEST_F(SignalProcessingTest, CompleteAudioProcessingWorkflow)
{
    // Create test audio signal
    SignalProcessing::RealVector original = createSineWave(1000.0, 0.8, 0.1, 48000.0);
    
    // 1. Analyze original levels
    SignalProcessing::AudioLevels originalLevels = SignalProcessing::analyzeAudioLevels(original, 48000.0);
    EXPECT_GT(originalLevels.peakLevel, -5.0);
    
    // 2. Apply window and analyze spectrum
    SignalProcessing::SpectrumAnalysis spectrum = SignalProcessing::analyzeSpectrum(original, 48000.0);
    EXPECT_NEAR(spectrum.fundamentalFreq, 1000.0, 50.0);
    
    // 3. Convert to bytes and back
    QByteArray audioBytes = SignalProcessing::convertFromFloat(original, SignalProcessing::SampleFormat::Int16);
    SignalProcessing::RealVector recovered = SignalProcessing::convertToFloat(audioBytes, SignalProcessing::SampleFormat::Int16);
    
    // 4. Verify recovery (within quantization error)
    EXPECT_EQ(recovered.size(), original.size());
    for (int i = 0; i < std::min(static_cast<int>(original.size()), 100); ++i) { // Check first 100 samples
        EXPECT_NEAR(recovered[i], original[i], 1.0/32768.0);
    }
    
    // 5. Apply filtering
    SignalProcessing::RealVector filtered = SignalProcessing::lowPassFilter(recovered, 2000.0, 48000.0);
    EXPECT_EQ(filtered.size(), recovered.size());
    
    // 6. Normalize result
    SignalProcessing::RealVector final = SignalProcessing::normalize(filtered, 0.5);
    double finalPeak = SignalProcessing::calculatePeakLevel(final);
    EXPECT_NEAR(finalPeak, SignalProcessing::amplitudeToDb(0.5), 1.0);
}

TEST_F(SignalProcessingTest, FrequencyAnalysisWorkflow)
{
    // Create complex signal with multiple frequencies
    SignalProcessing::RealVector signal1 = createSineWave(440.0, 0.5, 0.1, 8192.0);  // A4
    SignalProcessing::RealVector signal2 = createSineWave(880.0, 0.3, 0.1, 8192.0);  // A5
    SignalProcessing::RealVector signal3 = createSineWave(1320.0, 0.2, 0.1, 8192.0); // E6
    
    // Mix signals
    SignalProcessing::RealVector mixed;
    mixed.reserve(signal1.size());
    for (int i = 0; i < signal1.size(); ++i) {
        mixed.append(signal1[i] + signal2[i] + signal3[i]);
    }
    
    // Analyze spectrum
    SignalProcessing::SpectrumAnalysis analysis = SignalProcessing::analyzeSpectrum(mixed, 8192.0);
    
    EXPECT_FALSE(analysis.frequencies.isEmpty());
    EXPECT_FALSE(analysis.magnitudes.isEmpty());
    
    // Should detect fundamental (strongest component)
    EXPECT_GT(analysis.fundamentalFreq, 400.0);
    EXPECT_LT(analysis.fundamentalFreq, 500.0);
    
    // THD should be significant due to multiple harmonics
    EXPECT_GT(analysis.thd, 0.0);
}

// Performance Tests (basic)
TEST_F(SignalProcessingTest, FftPerformanceBaseline)
{
    // Test FFT performance with reasonable size
    SignalProcessing::RealVector input = createSineWave(1000.0, 1.0, 0.1, 1024.0);
    
    // This mainly ensures no crashes with typical sizes
    SignalProcessing::ComplexVector result = SignalProcessing::fft(input);
    
    EXPECT_EQ(result.size(), 1024);
    EXPECT_FALSE(SignalProcessing::hasError());
}

TEST_F(SignalProcessingTest, WindowFunctionPerformance)
{
    // Test window generation with large sizes
    for (int size = 64; size <= 4096; size *= 2) {
        SignalProcessing::RealVector window = SignalProcessing::generateWindow(
            SignalProcessing::WindowType::Hanning, size);
        
        EXPECT_EQ(window.size(), size);
    }
}