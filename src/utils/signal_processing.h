#pragma once

#include <QObject>
#include <QVector>
#include <QByteArray>
#include <complex>
#include <cmath>

/**
 * @class SignalProcessing
 * @brief Digital Signal Processing utilities for ETI Stream Analyser
 * 
 * Provides DSP functions for audio processing, FFT analysis, filtering,
 * and signal analysis specific to DAB/ETI streams.
 */
class SignalProcessing : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Complex number type for FFT operations
     */
    using Complex = std::complex<double>;
    using ComplexVector = QVector<Complex>;
    using RealVector = QVector<double>;

    /**
     * @brief Window function types for FFT analysis
     */
    enum class WindowType {
        None = 0,
        Hanning = 1,
        Hamming = 2,
        Blackman = 3,
        Kaiser = 4,
        Bartlett = 5
    };

    /**
     * @brief Audio sample formats
     */
    enum class SampleFormat {
        Unknown = 0,
        Int16 = 1,      // 16-bit signed integer
        Int24 = 2,      // 24-bit signed integer
        Int32 = 3,      // 32-bit signed integer
        Float32 = 4,    // 32-bit floating point
        Float64 = 5     // 64-bit floating point
    };

    /**
     * @brief Filter types
     */
    enum class FilterType {
        LowPass = 0,
        HighPass = 1,
        BandPass = 2,
        BandStop = 3
    };

    /**
     * @brief Audio level analysis result
     */
    struct AudioLevels {
        double peakLevel = 0.0;        // Peak level in dB
        double rmsLevel = 0.0;         // RMS level in dB
        double loudnessLufs = 0.0;     // Loudness in LUFS
        bool clipDetected = false;     // Digital clipping detected
        
        AudioLevels() = default;
        AudioLevels(double peak, double rms, double lufs, bool clip)
            : peakLevel(peak), rmsLevel(rms), loudnessLufs(lufs), clipDetected(clip) {}
    };

    /**
     * @brief Spectrum analysis result
     */
    struct SpectrumAnalysis {
        RealVector frequencies;        // Frequency bins (Hz)
        RealVector magnitudes;         // Magnitude spectrum (dB)
        RealVector phases;             // Phase spectrum (radians)
        double fundamentalFreq = 0.0;  // Fundamental frequency (Hz)
        double thd = 0.0;             // Total Harmonic Distortion (%)
        
        SpectrumAnalysis() = default;
    };

    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit SignalProcessing(QObject *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SignalProcessing();

    // FFT Operations
    static ComplexVector fft(const ComplexVector& input);
    static ComplexVector ifft(const ComplexVector& input);
    static ComplexVector fft(const RealVector& input);
    static RealVector realFft(const RealVector& input);
    static RealVector magnitude(const ComplexVector& complex);
    static RealVector phase(const ComplexVector& complex);
    static RealVector powerSpectrum(const ComplexVector& complex);

    // Window Functions
    static RealVector generateWindow(WindowType type, int size, double beta = 8.0);
    static void applyWindow(RealVector& signal, WindowType type, double beta = 8.0);
    static double windowScalingFactor(WindowType type, int size);

    // Audio Level Analysis
    static AudioLevels analyzeAudioLevels(const RealVector& samples, double sampleRate);
    static double calculatePeakLevel(const RealVector& samples);
    static double calculateRmsLevel(const RealVector& samples);
    static double calculateLoudness(const RealVector& samples, double sampleRate);
    static bool detectClipping(const RealVector& samples, double threshold = 0.99);

    // Audio Format Conversion
    static RealVector convertToFloat(const QByteArray& audioData, SampleFormat format);
    static QByteArray convertFromFloat(const RealVector& samples, SampleFormat format);
    static RealVector resample(const RealVector& input, double inputRate, double outputRate);
    static RealVector stereoToMono(const RealVector& stereoSamples);
    static RealVector monoToStereo(const RealVector& monoSamples);

    // Filtering
    static RealVector lowPassFilter(const RealVector& input, double cutoffFreq, double sampleRate, int order = 4);
    static RealVector highPassFilter(const RealVector& input, double cutoffFreq, double sampleRate, int order = 4);
    static RealVector bandPassFilter(const RealVector& input, double lowFreq, double highFreq, double sampleRate, int order = 4);
    static RealVector bandStopFilter(const RealVector& input, double lowFreq, double highFreq, double sampleRate, int order = 4);
    static RealVector applyButterworthFilter(const RealVector& input, FilterType type, double freq1, double freq2, double sampleRate, int order);

    // Spectrum Analysis
    static SpectrumAnalysis analyzeSpectrum(const RealVector& samples, double sampleRate, WindowType window = WindowType::Hanning);
    static RealVector calculateMelSpectrogram(const RealVector& samples, double sampleRate, int numMelFilters = 40);
    static double findFundamentalFrequency(const RealVector& spectrum, const RealVector& frequencies);
    static double calculateTotalHarmonicDistortion(const RealVector& spectrum, const RealVector& frequencies, double fundamentalFreq);

    // Signal Generation
    static RealVector generateSineWave(double frequency, double amplitude, double duration, double sampleRate, double phase = 0.0);
    static RealVector generateWhiteNoise(double amplitude, double duration, double sampleRate);
    static RealVector generatePinkNoise(double amplitude, double duration, double sampleRate);
    static RealVector generateChirp(double startFreq, double endFreq, double duration, double sampleRate);

    // Signal Analysis
    static double calculateSnr(const RealVector& signal, const RealVector& noise);
    static double calculateThd(const RealVector& signal, double sampleRate, double fundamentalFreq);
    static RealVector calculateSpectrogram(const RealVector& signal, int windowSize, int hopSize, WindowType window = WindowType::Hanning);
    static double calculateCrossCorrelation(const RealVector& signal1, const RealVector& signal2);
    static RealVector calculateAutoCorrelation(const RealVector& signal, int maxLag);

    // Utility Functions
    static double linearToDb(double linear);
    static double dbToLinear(double db);
    static double amplitudeToDb(double amplitude);
    static double dbToAmplitude(double db);
    static RealVector normalize(const RealVector& signal, double targetLevel = 1.0);
    static RealVector removeDcOffset(const RealVector& signal);
    static bool isPowerOfTwo(int n);
    static int nextPowerOfTwo(int n);

    // DAB/ETI Specific Functions
    static RealVector decodeMpeg1Layer2(const QByteArray& mpegData, double sampleRate = 48000.0);
    static RealVector decodeAacPlus(const QByteArray& aacData, double sampleRate = 48000.0);
    static AudioLevels analyzeDabAudioLevels(const QByteArray& audioFrame, SampleFormat format, double sampleRate);
    static SpectrumAnalysis analyzeDabSpectrum(const QByteArray& audioFrame, SampleFormat format, double sampleRate);

    // Error Handling
    static QString getLastErrorString();
    static bool hasError();
    static void clearError();

signals:
    /**
     * @brief Emitted when spectrum analysis is complete
     * @param analysis Spectrum analysis result
     */
    void spectrumAnalysisComplete(const SpectrumAnalysis& analysis);

    /**
     * @brief Emitted when audio levels are analyzed
     * @param levels Audio level analysis result
     */
    void audioLevelsAnalyzed(const AudioLevels& levels);

    /**
     * @brief Emitted during long processing operations
     * @param progress Progress percentage (0-100)
     */
    void progressUpdate(int progress);

private:
    // Internal FFT implementation
    static void fftRecursive(ComplexVector& data, bool inverse);
    static void bitReversePermutation(ComplexVector& data);
    static int reverseBits(int num, int bits);

    // Internal filter design
    static RealVector designButterworthCoefficients(FilterType type, double freq1, double freq2, double sampleRate, int order);
    static RealVector applyIirFilter(const RealVector& input, const RealVector& coefficients);

    // Internal utility methods
    static double calculateHanningWindow(int n, int N);
    static double calculateHammingWindow(int n, int N);
    static double calculateBlackmanWindow(int n, int N);
    static double calculateKaiserWindow(int n, int N, double beta);
    static double calculateBartlettWindow(int n, int N);
    static double besselI0(double x);

    // Error tracking
    static thread_local QString s_lastError;
    static void setError(const QString& error);
};