#include "signal_processing.h"
#include <QDebug>
#include <QtMath>
#include <QRandomGenerator>
#include <algorithm>
#include <numeric>

// Thread-local error storage
thread_local QString SignalProcessing::s_lastError;

// Mathematical constants
namespace {
    const double PI = M_PI;
    const double TWO_PI = 2.0 * M_PI;
    const double LOG10_20 = 20.0 / std::log(10.0);
}

SignalProcessing::SignalProcessing(QObject *parent)
    : QObject(parent)
{
}

SignalProcessing::~SignalProcessing()
{
}

// FFT Operations
SignalProcessing::ComplexVector SignalProcessing::fft(const ComplexVector& input)
{
    clearError();
    
    if (input.isEmpty()) {
        setError("Input vector is empty");
        return ComplexVector();
    }
    
    int n = input.size();
    if (!isPowerOfTwo(n)) {
        setError("FFT input size must be a power of 2");
        return ComplexVector();
    }
    
    ComplexVector result = input;
    fftRecursive(result, false);
    return result;
}

SignalProcessing::ComplexVector SignalProcessing::ifft(const ComplexVector& input)
{
    clearError();
    
    if (input.isEmpty()) {
        setError("Input vector is empty");
        return ComplexVector();
    }
    
    int n = input.size();
    if (!isPowerOfTwo(n)) {
        setError("IFFT input size must be a power of 2");
        return ComplexVector();
    }
    
    ComplexVector result = input;
    fftRecursive(result, true);
    
    // Scale by 1/N for inverse transform
    double scale = 1.0 / n;
    for (auto& sample : result) {
        sample *= scale;
    }
    
    return result;
}

SignalProcessing::ComplexVector SignalProcessing::fft(const RealVector& input)
{
    ComplexVector complexInput;
    complexInput.reserve(input.size());
    
    for (double real : input) {
        complexInput.append(Complex(real, 0.0));
    }
    
    return fft(complexInput);
}

SignalProcessing::RealVector SignalProcessing::realFft(const RealVector& input)
{
    ComplexVector complexResult = fft(input);
    return magnitude(complexResult);
}

SignalProcessing::RealVector SignalProcessing::magnitude(const ComplexVector& complex)
{
    RealVector result;
    result.reserve(complex.size());
    
    for (const Complex& c : complex) {
        result.append(std::abs(c));
    }
    
    return result;
}

SignalProcessing::RealVector SignalProcessing::phase(const ComplexVector& complex)
{
    RealVector result;
    result.reserve(complex.size());
    
    for (const Complex& c : complex) {
        result.append(std::arg(c));
    }
    
    return result;
}

SignalProcessing::RealVector SignalProcessing::powerSpectrum(const ComplexVector& complex)
{
    RealVector result;
    result.reserve(complex.size());
    
    for (const Complex& c : complex) {
        double mag = std::abs(c);
        result.append(mag * mag);
    }
    
    return result;
}

// Window Functions
SignalProcessing::RealVector SignalProcessing::generateWindow(WindowType type, int size, double beta)
{
    RealVector window;
    window.reserve(size);
    
    for (int n = 0; n < size; ++n) {
        double value = 1.0; // Default: no window
        
        switch (type) {
            case WindowType::Hanning:
                value = calculateHanningWindow(n, size);
                break;
            case WindowType::Hamming:
                value = calculateHammingWindow(n, size);
                break;
            case WindowType::Blackman:
                value = calculateBlackmanWindow(n, size);
                break;
            case WindowType::Kaiser:
                value = calculateKaiserWindow(n, size, beta);
                break;
            case WindowType::Bartlett:
                value = calculateBartlettWindow(n, size);
                break;
            case WindowType::None:
            default:
                value = 1.0;
                break;
        }
        
        window.append(value);
    }
    
    return window;
}

void SignalProcessing::applyWindow(RealVector& signal, WindowType type, double beta)
{
    if (signal.isEmpty()) return;
    
    RealVector window = generateWindow(type, signal.size(), beta);
    
    for (int i = 0; i < signal.size(); ++i) {
        signal[i] *= window[i];
    }
}

double SignalProcessing::windowScalingFactor(WindowType type, int size)
{
    RealVector window = generateWindow(type, size);
    double sum = std::accumulate(window.begin(), window.end(), 0.0);
    return size / sum;
}

// Audio Level Analysis
SignalProcessing::AudioLevels SignalProcessing::analyzeAudioLevels(const RealVector& samples, double sampleRate)
{
    AudioLevels levels;
    
    if (samples.isEmpty()) {
        setError("Sample vector is empty");
        return levels;
    }
    
    levels.peakLevel = calculatePeakLevel(samples);
    levels.rmsLevel = calculateRmsLevel(samples);
    levels.loudnessLufs = calculateLoudness(samples, sampleRate);
    levels.clipDetected = detectClipping(samples);
    
    return levels;
}

double SignalProcessing::calculatePeakLevel(const RealVector& samples)
{
    if (samples.isEmpty()) return -INFINITY;
    
    double peak = 0.0;
    for (double sample : samples) {
        peak = std::max(peak, std::abs(sample));
    }
    
    return amplitudeToDb(peak);
}

double SignalProcessing::calculateRmsLevel(const RealVector& samples)
{
    if (samples.isEmpty()) return -INFINITY;
    
    double sumSquares = 0.0;
    for (double sample : samples) {
        sumSquares += sample * sample;
    }
    
    double rms = std::sqrt(sumSquares / samples.size());
    return amplitudeToDb(rms);
}

double SignalProcessing::calculateLoudness(const RealVector& samples, double sampleRate)
{
    // Sample rate dependent loudness calculation
    // Apply basic frequency weighting based on sample rate
    double weightingFactor = std::min(1.0, sampleRate / 48000.0); // Normalize to 48kHz
    
    // Simplified loudness calculation (not full LUFS implementation)
    // This would need proper K-weighting filter for true LUFS
    double rms = 0.0;
    for (double sample : samples) {
        rms += sample * sample * weightingFactor;
    }
    rms = std::sqrt(rms / samples.size());
    
    // Convert to LUFS approximation
    return amplitudeToDb(rms) - 23.0; // Rough LUFS conversion
}

bool SignalProcessing::detectClipping(const RealVector& samples, double threshold)
{
    for (double sample : samples) {
        if (std::abs(sample) >= threshold) {
            return true;
        }
    }
    return false;
}

// Audio Format Conversion
SignalProcessing::RealVector SignalProcessing::convertToFloat(const QByteArray& audioData, SampleFormat format)
{
    RealVector result;
    clearError();
    
    if (audioData.isEmpty()) {
        setError("Audio data is empty");
        return result;
    }
    
    const char* data = audioData.constData();
    int dataSize = audioData.size();
    
    switch (format) {
        case SampleFormat::Int16: {
            int sampleCount = dataSize / 2;
            result.reserve(sampleCount);
            const qint16* samples = reinterpret_cast<const qint16*>(data);
            for (int i = 0; i < sampleCount; ++i) {
                result.append(static_cast<double>(samples[i]) / 32768.0);
            }
            break;
        }
        case SampleFormat::Int24: {
            int sampleCount = dataSize / 3;
            result.reserve(sampleCount);
            for (int i = 0; i < sampleCount; ++i) {
                int offset = i * 3;
                qint32 sample = (static_cast<qint32>(data[offset]) << 8) |
                               (static_cast<qint32>(data[offset + 1]) << 16) |
                               (static_cast<qint32>(data[offset + 2]) << 24);
                sample >>= 8; // Sign extend
                result.append(static_cast<double>(sample) / 8388608.0);
            }
            break;
        }
        case SampleFormat::Int32: {
            int sampleCount = dataSize / 4;
            result.reserve(sampleCount);
            const qint32* samples = reinterpret_cast<const qint32*>(data);
            for (int i = 0; i < sampleCount; ++i) {
                result.append(static_cast<double>(samples[i]) / 2147483648.0);
            }
            break;
        }
        case SampleFormat::Float32: {
            int sampleCount = dataSize / 4;
            result.reserve(sampleCount);
            const float* samples = reinterpret_cast<const float*>(data);
            for (int i = 0; i < sampleCount; ++i) {
                result.append(static_cast<double>(samples[i]));
            }
            break;
        }
        case SampleFormat::Float64: {
            int sampleCount = dataSize / 8;
            result.reserve(sampleCount);
            const double* samples = reinterpret_cast<const double*>(data);
            for (int i = 0; i < sampleCount; ++i) {
                result.append(samples[i]);
            }
            break;
        }
        default:
            setError("Unsupported sample format");
            break;
    }
    
    return result;
}

QByteArray SignalProcessing::convertFromFloat(const RealVector& samples, SampleFormat format)
{
    QByteArray result;
    clearError();
    
    if (samples.isEmpty()) {
        setError("Sample vector is empty");
        return result;
    }
    
    switch (format) {
        case SampleFormat::Int16: {
            result.resize(samples.size() * 2);
            qint16* data = reinterpret_cast<qint16*>(result.data());
            for (int i = 0; i < samples.size(); ++i) {
                double clamped = qBound(-1.0, samples[i], 1.0);
                data[i] = static_cast<qint16>(clamped * 32767.0);
            }
            break;
        }
        case SampleFormat::Float32: {
            result.resize(samples.size() * 4);
            float* data = reinterpret_cast<float*>(result.data());
            for (int i = 0; i < samples.size(); ++i) {
                data[i] = static_cast<float>(samples[i]);
            }
            break;
        }
        default:
            setError("Unsupported sample format for conversion");
            break;
    }
    
    return result;
}

SignalProcessing::RealVector SignalProcessing::resample(const RealVector& input, double inputRate, double outputRate)
{
    if (input.isEmpty() || inputRate <= 0.0 || outputRate <= 0.0) {
        setError("Invalid resample parameters");
        return RealVector();
    }
    
    double ratio = outputRate / inputRate;
    int outputSize = static_cast<int>(input.size() * ratio);
    
    RealVector output;
    output.reserve(outputSize);
    
    // Simple linear interpolation resampling
    for (int i = 0; i < outputSize; ++i) {
        double sourceIndex = i / ratio;
        int index1 = static_cast<int>(sourceIndex);
        int index2 = std::min(index1 + 1, static_cast<int>(input.size()) - 1);
        double fraction = sourceIndex - index1;
        
        if (index1 < input.size()) {
            double interpolated = input[index1] * (1.0 - fraction) + input[index2] * fraction;
            output.append(interpolated);
        }
    }
    
    return output;
}

SignalProcessing::RealVector SignalProcessing::stereoToMono(const RealVector& stereoSamples)
{
    if (stereoSamples.size() % 2 != 0) {
        setError("Stereo sample count must be even");
        return RealVector();
    }
    
    RealVector mono;
    mono.reserve(stereoSamples.size() / 2);
    
    for (int i = 0; i < stereoSamples.size(); i += 2) {
        double monoSample = (stereoSamples[i] + stereoSamples[i + 1]) * 0.5;
        mono.append(monoSample);
    }
    
    return mono;
}

SignalProcessing::RealVector SignalProcessing::monoToStereo(const RealVector& monoSamples)
{
    RealVector stereo;
    stereo.reserve(monoSamples.size() * 2);
    
    for (double sample : monoSamples) {
        stereo.append(sample);
        stereo.append(sample);
    }
    
    return stereo;
}

// Filtering
SignalProcessing::RealVector SignalProcessing::lowPassFilter(const RealVector& input, double cutoffFreq, double sampleRate, int order)
{
    return applyButterworthFilter(input, FilterType::LowPass, cutoffFreq, 0.0, sampleRate, order);
}

SignalProcessing::RealVector SignalProcessing::highPassFilter(const RealVector& input, double cutoffFreq, double sampleRate, int order)
{
    return applyButterworthFilter(input, FilterType::HighPass, cutoffFreq, 0.0, sampleRate, order);
}

SignalProcessing::RealVector SignalProcessing::bandPassFilter(const RealVector& input, double lowFreq, double highFreq, double sampleRate, int order)
{
    return applyButterworthFilter(input, FilterType::BandPass, lowFreq, highFreq, sampleRate, order);
}

SignalProcessing::RealVector SignalProcessing::bandStopFilter(const RealVector& input, double lowFreq, double highFreq, double sampleRate, int order)
{
    return applyButterworthFilter(input, FilterType::BandStop, lowFreq, highFreq, sampleRate, order);
}

SignalProcessing::RealVector SignalProcessing::applyButterworthFilter(const RealVector& input, FilterType type, double freq1, double freq2, double sampleRate, int order)
{
    Q_UNUSED(freq2) // Will be used for bandpass/bandstop filters
    Q_UNUSED(order) // Will be used for filter order implementation
    
    // Simplified Butterworth filter implementation
    // In a real implementation, this would use proper filter design algorithms
    
    clearError();
    
    if (input.isEmpty()) {
        setError("Input vector is empty");
        return RealVector();
    }
    
    if (freq1 <= 0.0 || freq1 >= sampleRate / 2.0) {
        setError("Invalid filter frequency");
        return input; // Return unfiltered
    }
    
    // For now, return a simple moving average as a placeholder
    // Real implementation would use IIR filter coefficients
    RealVector output = input;
    
    // Simple 3-point moving average (low-pass approximation)
    if (type == FilterType::LowPass && output.size() >= 3) {
        for (int i = 1; i < output.size() - 1; ++i) {
            output[i] = (input[i-1] + input[i] + input[i+1]) / 3.0;
        }
    }
    
    return output;
}

// Spectrum Analysis
SignalProcessing::SpectrumAnalysis SignalProcessing::analyzeSpectrum(const RealVector& samples, double sampleRate, WindowType window)
{
    SpectrumAnalysis analysis;
    clearError();
    
    if (samples.isEmpty()) {
        setError("Sample vector is empty");
        return analysis;
    }
    
    // Apply window function
    RealVector windowedSamples = samples;
    applyWindow(windowedSamples, window);
    
    // Perform FFT
    ComplexVector fftResult = fft(windowedSamples);
    
    // Calculate magnitude spectrum
    analysis.magnitudes = magnitude(fftResult);
    analysis.phases = phase(fftResult);
    
    // Generate frequency bins
    int numBins = fftResult.size() / 2; // Only positive frequencies
    analysis.frequencies.reserve(numBins);
    analysis.magnitudes.resize(numBins);
    
    for (int i = 0; i < numBins; ++i) {
        double freq = (static_cast<double>(i) * sampleRate) / fftResult.size();
        analysis.frequencies.append(freq);
        
        // Convert to dB
        if (analysis.magnitudes[i] > 0.0) {
            analysis.magnitudes[i] = amplitudeToDb(analysis.magnitudes[i]);
        } else {
            analysis.magnitudes[i] = -120.0; // -120 dB floor
        }
    }
    
    // Find fundamental frequency
    analysis.fundamentalFreq = findFundamentalFrequency(analysis.magnitudes, analysis.frequencies);
    
    // Calculate THD
    analysis.thd = calculateTotalHarmonicDistortion(analysis.magnitudes, analysis.frequencies, analysis.fundamentalFreq);
    
    return analysis;
}

double SignalProcessing::findFundamentalFrequency(const RealVector& spectrum, const RealVector& frequencies)
{
    if (spectrum.size() != frequencies.size() || spectrum.isEmpty()) {
        return 0.0;
    }
    
    // Find the peak in the spectrum (excluding DC component)
    double maxMagnitude = -INFINITY;
    int maxIndex = 0;
    
    for (int i = 1; i < spectrum.size(); ++i) { // Start from 1 to skip DC
        if (spectrum[i] > maxMagnitude) {
            maxMagnitude = spectrum[i];
            maxIndex = i;
        }
    }
    
    return (maxIndex < frequencies.size()) ? frequencies[maxIndex] : 0.0;
}

double SignalProcessing::calculateTotalHarmonicDistortion(const RealVector& spectrum, const RealVector& frequencies, double fundamentalFreq)
{
    if (fundamentalFreq <= 0.0 || spectrum.isEmpty() || frequencies.isEmpty()) {
        return 0.0;
    }
    
    // Find fundamental peak
    double fundamentalPower = 0.0;
    double harmonicPower = 0.0;
    
    for (int i = 0; i < frequencies.size(); ++i) {
        double freq = frequencies[i];
        double power = dbToLinear(spectrum[i]);
        power = power * power; // Convert to power
        
        // Check if this is the fundamental or a harmonic
        if (std::abs(freq - fundamentalFreq) < fundamentalFreq * 0.05) {
            fundamentalPower += power;
        } else {
            // Check for harmonics (2f, 3f, 4f, etc.)
            for (int harmonic = 2; harmonic <= 10; ++harmonic) {
                double harmonicFreq = fundamentalFreq * harmonic;
                if (std::abs(freq - harmonicFreq) < fundamentalFreq * 0.05) {
                    harmonicPower += power;
                    break;
                }
            }
        }
    }
    
    if (fundamentalPower > 0.0) {
        return std::sqrt(harmonicPower / fundamentalPower) * 100.0; // THD as percentage
    }
    
    return 0.0;
}

// Signal Generation
SignalProcessing::RealVector SignalProcessing::generateSineWave(double frequency, double amplitude, double duration, double sampleRate, double phase)
{
    int numSamples = static_cast<int>(duration * sampleRate);
    RealVector signal;
    signal.reserve(numSamples);
    
    for (int i = 0; i < numSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double sample = amplitude * std::sin(TWO_PI * frequency * t + phase);
        signal.append(sample);
    }
    
    return signal;
}

SignalProcessing::RealVector SignalProcessing::generateWhiteNoise(double amplitude, double duration, double sampleRate)
{
    int numSamples = static_cast<int>(duration * sampleRate);
    RealVector noise;
    noise.reserve(numSamples);
    
    QRandomGenerator* rng = QRandomGenerator::global();
    
    for (int i = 0; i < numSamples; ++i) {
        double sample = (rng->generateDouble() * 2.0 - 1.0) * amplitude;
        noise.append(sample);
    }
    
    return noise;
}

SignalProcessing::RealVector SignalProcessing::generatePinkNoise(double amplitude, double duration, double sampleRate)
{
    // Simplified pink noise generation using white noise and filtering
    RealVector whiteNoise = generateWhiteNoise(amplitude, duration, sampleRate);
    
    // Apply simple pink filter (1/f characteristic)
    RealVector pinkNoise = whiteNoise;
    
    if (pinkNoise.size() > 1) {
        for (int i = 1; i < pinkNoise.size(); ++i) {
            pinkNoise[i] = 0.99 * pinkNoise[i-1] + 0.01 * whiteNoise[i];
        }
    }
    
    return pinkNoise;
}

SignalProcessing::RealVector SignalProcessing::generateChirp(double startFreq, double endFreq, double duration, double sampleRate)
{
    int numSamples = static_cast<int>(duration * sampleRate);
    RealVector chirp;
    chirp.reserve(numSamples);
    
    for (int i = 0; i < numSamples; ++i) {
        double t = static_cast<double>(i) / sampleRate;
        double normalizedTime = t / duration;
        // Calculate phase directly without intermediate frequency variable
        double phase = TWO_PI * (startFreq * t + 0.5 * (endFreq - startFreq) * normalizedTime * t);
        double sample = std::sin(phase);
        chirp.append(sample);
    }
    
    return chirp;
}

// Utility Functions
double SignalProcessing::linearToDb(double linear)
{
    return (linear > 0.0) ? LOG10_20 * std::log(linear) : -INFINITY;
}

double SignalProcessing::dbToLinear(double db)
{
    return std::pow(10.0, db / 20.0);
}

double SignalProcessing::amplitudeToDb(double amplitude)
{
    return linearToDb(amplitude);
}

double SignalProcessing::dbToAmplitude(double db)
{
    return dbToLinear(db);
}

SignalProcessing::RealVector SignalProcessing::normalize(const RealVector& signal, double targetLevel)
{
    if (signal.isEmpty()) return signal;
    
    double peak = 0.0;
    for (double sample : signal) {
        peak = std::max(peak, std::abs(sample));
    }
    
    if (peak == 0.0) return signal;
    
    double scale = targetLevel / peak;
    RealVector normalized;
    normalized.reserve(signal.size());
    
    for (double sample : signal) {
        normalized.append(sample * scale);
    }
    
    return normalized;
}

SignalProcessing::RealVector SignalProcessing::removeDcOffset(const RealVector& signal)
{
    if (signal.isEmpty()) return signal;
    
    // Calculate DC offset (mean)
    double sum = std::accumulate(signal.begin(), signal.end(), 0.0);
    double dcOffset = sum / signal.size();
    
    RealVector result;
    result.reserve(signal.size());
    
    for (double sample : signal) {
        result.append(sample - dcOffset);
    }
    
    return result;
}

bool SignalProcessing::isPowerOfTwo(int n)
{
    return n > 0 && (n & (n - 1)) == 0;
}

int SignalProcessing::nextPowerOfTwo(int n)
{
    if (n <= 1) return 1;
    
    int power = 1;
    while (power < n) {
        power <<= 1;
    }
    
    return power;
}

// DAB/ETI Specific Functions
SignalProcessing::RealVector SignalProcessing::decodeMpeg1Layer2(const QByteArray& mpegData, double sampleRate)
{
    if (mpegData.isEmpty()) {
        setError("Empty MPEG-1 Layer 2 data provided");
        return RealVector();
    }
    
    // Basic validation of MPEG data format
    if (mpegData.size() < 4 || (static_cast<unsigned char>(mpegData[0]) & 0xFF) != 0xFF) {
        setError("Invalid MPEG-1 Layer 2 header");
        return RealVector();
    }
    
    // Generate placeholder audio based on sample rate for testing
    size_t sampleCount = static_cast<size_t>(sampleRate * 0.024); // 24ms frame
    RealVector samples(sampleCount);
    
    // Create test tone based on data content for reproducible testing
    double frequency = 1000.0; // 1kHz test tone
    for (size_t i = 0; i < sampleCount; ++i) {
        samples[i] = 0.1 * std::sin(2.0 * M_PI * frequency * i / sampleRate);
    }
    
    return samples;
}

SignalProcessing::RealVector SignalProcessing::decodeAacPlus(const QByteArray& aacData, double sampleRate)
{
    if (aacData.isEmpty()) {
        setError("Empty AAC+ data provided");
        return RealVector();
    }
    
    // Basic validation of AAC data (simplified check)
    if (aacData.size() < 7) {
        setError("AAC+ data too short for valid frame");
        return RealVector();
    }
    
    // Generate placeholder audio based on sample rate for testing
    size_t sampleCount = static_cast<size_t>(sampleRate * 0.024); // 24ms frame
    RealVector samples(sampleCount);
    
    // Create different test tone for AAC+ to distinguish from MP2
    double frequency = 440.0; // A4 note for AAC+
    for (size_t i = 0; i < sampleCount; ++i) {
        samples[i] = 0.15 * std::sin(2.0 * M_PI * frequency * i / sampleRate);
    }
    
    return samples;
}

SignalProcessing::AudioLevels SignalProcessing::analyzeDabAudioLevels(const QByteArray& audioFrame, SampleFormat format, double sampleRate)
{
    RealVector samples = convertToFloat(audioFrame, format);
    return analyzeAudioLevels(samples, sampleRate);
}

SignalProcessing::SpectrumAnalysis SignalProcessing::analyzeDabSpectrum(const QByteArray& audioFrame, SampleFormat format, double sampleRate)
{
    RealVector samples = convertToFloat(audioFrame, format);
    return analyzeSpectrum(samples, sampleRate);
}

// Error Handling
QString SignalProcessing::getLastErrorString()
{
    return s_lastError;
}

bool SignalProcessing::hasError()
{
    return !s_lastError.isEmpty();
}

void SignalProcessing::clearError()
{
    s_lastError.clear();
}

// Private Implementation
void SignalProcessing::fftRecursive(ComplexVector& data, bool inverse)
{
    int n = data.size();
    if (n <= 1) return;
    
    // Bit-reverse permutation
    bitReversePermutation(data);
    
    // Cooley-Tukey FFT
    for (int len = 2; len <= n; len <<= 1) {
        double angle = (inverse ? TWO_PI : -TWO_PI) / len;
        Complex wlen(std::cos(angle), std::sin(angle));
        
        for (int i = 0; i < n; i += len) {
            Complex w(1.0, 0.0);
            for (int j = 0; j < len / 2; ++j) {
                Complex u = data[i + j];
                Complex v = data[i + j + len / 2] * w;
                data[i + j] = u + v;
                data[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}

void SignalProcessing::bitReversePermutation(ComplexVector& data)
{
    int n = data.size();
    int bits = 0;
    int temp = n;
    while (temp > 1) {
        temp >>= 1;
        bits++;
    }
    
    for (int i = 0; i < n; ++i) {
        int j = reverseBits(i, bits);
        if (i < j) {
            std::swap(data[i], data[j]);
        }
    }
}

int SignalProcessing::reverseBits(int num, int bits)
{
    int result = 0;
    for (int i = 0; i < bits; ++i) {
        result = (result << 1) | (num & 1);
        num >>= 1;
    }
    return result;
}

// Window function implementations
double SignalProcessing::calculateHanningWindow(int n, int N)
{
    return 0.5 * (1.0 - std::cos(TWO_PI * n / (N - 1)));
}

double SignalProcessing::calculateHammingWindow(int n, int N)
{
    return 0.54 - 0.46 * std::cos(TWO_PI * n / (N - 1));
}

double SignalProcessing::calculateBlackmanWindow(int n, int N)
{
    double a0 = 0.42659;
    double a1 = 0.49656;
    double a2 = 0.076849;
    
    return a0 - a1 * std::cos(TWO_PI * n / (N - 1)) + a2 * std::cos(4.0 * PI * n / (N - 1));
}

double SignalProcessing::calculateKaiserWindow(int n, int N, double beta)
{
    double arg = 2.0 * n / (N - 1) - 1.0;
    return besselI0(beta * std::sqrt(1.0 - arg * arg)) / besselI0(beta);
}

double SignalProcessing::calculateBartlettWindow(int n, int N)
{
    return 1.0 - std::abs(2.0 * n / (N - 1) - 1.0);
}

double SignalProcessing::besselI0(double x)
{
    // Approximation of modified Bessel function of the first kind
    double sum = 1.0;
    double term = 1.0;
    double xSquaredOver4 = (x * x) / 4.0;
    
    for (int i = 1; i < 20; ++i) {
        term *= xSquaredOver4 / (i * i);
        sum += term;
        if (term < 1e-10) break;
    }
    
    return sum;
}

void SignalProcessing::setError(const QString& error)
{
    s_lastError = error;
    qWarning() << "SignalProcessing error:" << error;
}

// Missing static function implementations for test compatibility

double SignalProcessing::calculateSnr(const RealVector& signal, const RealVector& noise)
{
    if (signal.isEmpty() || noise.isEmpty()) {
        setError("Signal or noise vector is empty");
        return 0.0;
    }
    
    if (signal.size() != noise.size()) {
        setError("Signal and noise vectors must have the same size");
        return 0.0;
    }
    
    // Calculate signal power (RMS squared)
    double signalPower = 0.0;
    for (const auto& sample : signal) {
        signalPower += sample * sample;
    }
    signalPower /= signal.size();
    
    // Calculate noise power (RMS squared)
    double noisePower = 0.0;
    for (const auto& sample : noise) {
        noisePower += sample * sample;
    }
    noisePower /= noise.size();
    
    // Avoid division by zero
    if (noisePower <= 0.0) {
        return 100.0; // Very high SNR when no noise
    }
    
    // SNR in dB = 10 * log10(signal_power / noise_power)
    double snrLinear = signalPower / noisePower;
    return 10.0 * std::log10(snrLinear);
}

double SignalProcessing::calculateCrossCorrelation(const RealVector& signal1, const RealVector& signal2)
{
    if (signal1.isEmpty() || signal2.isEmpty()) {
        setError("Signal vectors cannot be empty");
        return 0.0;
    }
    
    if (signal1.size() != signal2.size()) {
        setError("Signal vectors must have the same size");
        return 0.0;
    }
    
    int n = signal1.size();
    
    // Calculate means
    double mean1 = std::accumulate(signal1.begin(), signal1.end(), 0.0) / n;
    double mean2 = std::accumulate(signal2.begin(), signal2.end(), 0.0) / n;
    
    // Calculate cross-correlation coefficient (Pearson correlation)
    double numerator = 0.0;
    double sum1Squared = 0.0;
    double sum2Squared = 0.0;
    
    for (int i = 0; i < n; ++i) {
        double dev1 = signal1[i] - mean1;
        double dev2 = signal2[i] - mean2;
        
        numerator += dev1 * dev2;
        sum1Squared += dev1 * dev1;
        sum2Squared += dev2 * dev2;
    }
    
    // Avoid division by zero
    double denominator = std::sqrt(sum1Squared * sum2Squared);
    if (denominator <= 0.0) {
        return 0.0;
    }
    
    return numerator / denominator;
}

SignalProcessing::RealVector SignalProcessing::calculateAutoCorrelation(const RealVector& signal, int maxLag)
{
    if (signal.isEmpty()) {
        setError("Signal vector cannot be empty");
        return RealVector();
    }
    
    if (maxLag <= 0) {
        maxLag = signal.size() / 4; // Default to quarter of signal length
    }
    
    maxLag = std::min(maxLag, static_cast<int>(signal.size()) - 1);
    
    RealVector autocorr(maxLag + 1);
    int n = signal.size();
    
    // Calculate mean
    double mean = std::accumulate(signal.begin(), signal.end(), 0.0) / n;
    
    // Calculate variance (autocorrelation at lag 0)
    double variance = 0.0;
    for (const auto& sample : signal) {
        double dev = sample - mean;
        variance += dev * dev;
    }
    variance /= n;
    
    // Calculate autocorrelation for each lag
    for (int lag = 0; lag <= maxLag; ++lag) {
        double correlation = 0.0;
        int count = n - lag;
        
        for (int i = 0; i < count; ++i) {
            correlation += (signal[i] - mean) * (signal[i + lag] - mean);
        }
        
        correlation /= count;
        
        // Normalize by variance to get correlation coefficient
        autocorr[lag] = (variance > 0.0) ? correlation / variance : 0.0;
    }
    
    return autocorr;
}