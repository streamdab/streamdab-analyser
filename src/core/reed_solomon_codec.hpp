/**
 * @file reed_solomon_codec.hpp
 * @brief Reed-Solomon Error Correction for ETI Frame Processing
 * 
 * Implementation of Reed-Solomon error correction according to ETSI EN 300 799
 * for ETI frame validation and error recovery. Provides RS(204,188) codec
 * for MPEG transport stream protection and RS(255,239) for FIC protection.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#pragma once

#include <QObject>
#include <cstdint>
#include <array>
#include <vector>
#include <span>
#include <memory>
#include <optional>

namespace eti::codec {

/**
 * @brief Reed-Solomon coding parameters
 */
struct RSCodecParams {
    uint16_t n;          // Total codeword length
    uint16_t k;          // Information symbols
    uint16_t t;          // Error correction capability (symbols)
    uint8_t m;           // Symbol size in bits
    uint16_t gfpoly;     // Generator polynomial for GF(2^m)
    uint8_t fcr;         // First consecutive root
    uint8_t prim;        // Primitive element
    
    /**
     * @brief Standard RS(204,188) parameters for MPEG-TS
     */
    static constexpr RSCodecParams mpeg_ts() {
        return {204, 188, 8, 8, 0x11d, 0, 1};
    }
    
    /**
     * @brief Standard RS(255,239) parameters for FIC
     */
    static constexpr RSCodecParams fic_protection() {
        return {255, 239, 8, 8, 0x11d, 0, 1};
    }
    
    /**
     * @brief Validate codec parameters
     */
    [[nodiscard]] bool isValid() const {
        return n > k && k > 0 && t == (n - k) / 2 && m >= 3 && m <= 16;
    }
    
    /**
     * @brief Get redundancy length
     */
    [[nodiscard]] uint16_t getRedundancyLength() const {
        return n - k;
    }
};

/**
 * @brief Error correction result
 */
struct CorrectionResult {
    enum class Status {
        SUCCESS,             // No errors detected
        CORRECTED,          // Errors detected and corrected
        UNCORRECTABLE,      // Too many errors to correct
        INVALID_CODEWORD    // Invalid codeword format
    };
    
    Status status{Status::SUCCESS};
    uint16_t errors_detected{0};       // Number of error symbols detected
    uint16_t errors_corrected{0};      // Number of error symbols corrected
    std::vector<uint16_t> error_positions; // Positions of corrected errors
    std::vector<uint8_t> corrected_data;   // Corrected data (if applicable)
    
    /**
     * @brief Check if correction was successful
     */
    [[nodiscard]] bool wasSuccessful() const {
        return status == Status::SUCCESS || status == Status::CORRECTED;
    }
    
    /**
     * @brief Check if any errors were found
     */
    [[nodiscard]] bool hasErrors() const {
        return errors_detected > 0;
    }
    
    /**
     * @brief Get error correction capability usage percentage
     */
    [[nodiscard]] double getUsagePercentage(uint16_t max_correctable) const {
        return max_correctable > 0 ? 
               static_cast<double>(errors_detected) / max_correctable * 100.0 : 0.0;
    }
};

/**
 * @brief Galois Field arithmetic implementation
 */
class GaloisField {
public:
    explicit GaloisField(uint8_t m, uint16_t gfpoly);
    ~GaloisField() = default;
    
    // Disable copy/move for performance
    GaloisField(const GaloisField&) = delete;
    GaloisField& operator=(const GaloisField&) = delete;
    GaloisField(GaloisField&&) = delete;
    GaloisField& operator=(GaloisField&&) = delete;
    
    /**
     * @brief Get field size (2^m)
     */
    [[nodiscard]] uint16_t getFieldSize() const { return field_size_; }
    
    /**
     * @brief Galois field addition (XOR)
     */
    [[nodiscard]] uint16_t add(uint16_t a, uint16_t b) const {
        return a ^ b;
    }
    
    /**
     * @brief Galois field subtraction (same as addition)
     */
    [[nodiscard]] uint16_t subtract(uint16_t a, uint16_t b) const {
        return add(a, b);
    }
    
    /**
     * @brief Galois field multiplication
     */
    [[nodiscard]] uint16_t multiply(uint16_t a, uint16_t b) const;
    
    /**
     * @brief Galois field division
     */
    [[nodiscard]] uint16_t divide(uint16_t a, uint16_t b) const;
    
    /**
     * @brief Galois field power
     */
    [[nodiscard]] uint16_t power(uint16_t base, uint16_t exponent) const;
    
    /**
     * @brief Get multiplicative inverse
     */
    [[nodiscard]] uint16_t inverse(uint16_t a) const;
    
    /**
     * @brief Convert to alpha representation
     */
    [[nodiscard]] int16_t toAlpha(uint16_t value) const;
    
    /**
     * @brief Convert from alpha representation
     */
    [[nodiscard]] uint16_t fromAlpha(int16_t alpha) const;

private:
    void initializeTables();
    
    uint8_t m_;                    // Field parameter
    uint16_t gfpoly_;             // Generator polynomial
    uint16_t field_size_;         // 2^m
    std::vector<uint16_t> exp_table_;  // Exponential table
    std::vector<int16_t> log_table_;   // Logarithm table
};

/**
 * @brief Reed-Solomon polynomial operations
 */
class RSPolynomial {
public:
    explicit RSPolynomial(std::shared_ptr<GaloisField> gf);
    RSPolynomial(std::shared_ptr<GaloisField> gf, const std::vector<uint16_t>& coefficients);
    
    /**
     * @brief Get polynomial degree
     */
    [[nodiscard]] int getDegree() const;
    
    /**
     * @brief Get coefficient at position
     */
    [[nodiscard]] uint16_t getCoefficient(size_t position) const;
    
    /**
     * @brief Set coefficient at position
     */
    void setCoefficient(size_t position, uint16_t value);
    
    /**
     * @brief Evaluate polynomial at given point
     */
    [[nodiscard]] uint16_t evaluate(uint16_t x) const;
    
    /**
     * @brief Add two polynomials
     */
    [[nodiscard]] RSPolynomial add(const RSPolynomial& other) const;
    
    /**
     * @brief Multiply two polynomials
     */
    [[nodiscard]] RSPolynomial multiply(const RSPolynomial& other) const;
    
    /**
     * @brief Divide polynomial (return quotient and remainder)
     */
    [[nodiscard]] std::pair<RSPolynomial, RSPolynomial> divide(const RSPolynomial& divisor) const;
    
    /**
     * @brief Scale polynomial by constant
     */
    [[nodiscard]] RSPolynomial scale(uint16_t factor) const;
    
    /**
     * @brief Get polynomial coefficients
     */
    [[nodiscard]] const std::vector<uint16_t>& getCoefficients() const {
        return coefficients_;
    }

private:
    std::shared_ptr<GaloisField> gf_;
    std::vector<uint16_t> coefficients_;
    
    void normalize();
};

/**
 * @brief Reed-Solomon Encoder/Decoder
 */
class ReedSolomonCodec : public QObject {
    Q_OBJECT
    
public:
    explicit ReedSolomonCodec(const RSCodecParams& params, QObject* parent = nullptr);
    ~ReedSolomonCodec() override;
    
    /**
     * @brief Initialize codec with parameters
     */
    bool initialize();
    
    /**
     * @brief Check if codec is initialized
     */
    [[nodiscard]] bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Get codec parameters
     */
    [[nodiscard]] const RSCodecParams& getParameters() const { return params_; }
    
    /**
     * @brief Encode data with Reed-Solomon protection
     * @param data Input data (k symbols)
     * @return Encoded codeword (n symbols) or empty on error
     */
    [[nodiscard]] std::vector<uint8_t> encode(std::span<const uint8_t> data);
    
    /**
     * @brief Decode Reed-Solomon protected data
     * @param codeword Input codeword (n symbols)
     * @return Correction result with decoded data
     */
    [[nodiscard]] CorrectionResult decode(std::span<const uint8_t> codeword);
    
    /**
     * @brief Check Reed-Solomon codeword for errors (decode-only)
     * @param codeword Input codeword to check
     * @return Correction result (without data correction)
     */
    [[nodiscard]] CorrectionResult check(std::span<const uint8_t> codeword);
    
    /**
     * @brief Calculate syndrome for error detection
     * @param codeword Input codeword
     * @return Syndrome polynomial (empty if no errors)
     */
    [[nodiscard]] std::vector<uint16_t> calculateSyndrome(std::span<const uint8_t> codeword);
    
    /**
     * @brief Get error correction statistics
     */
    struct Statistics {
        uint64_t total_codewords{0};
        uint64_t error_free_codewords{0};
        uint64_t corrected_codewords{0};
        uint64_t uncorrectable_codewords{0};
        uint64_t total_errors_corrected{0};
        double correction_efficiency{100.0};  // Percentage of correctable errors
        
        [[nodiscard]] double getErrorRate() const {
            return total_codewords > 0 ? 
                   static_cast<double>(total_codewords - error_free_codewords) / total_codewords * 100.0 : 0.0;
        }
        
        [[nodiscard]] double getCorrectionRate() const {
            return total_codewords > 0 ? 
                   static_cast<double>(corrected_codewords) / total_codewords * 100.0 : 0.0;
        }
    };
    
    [[nodiscard]] Statistics getStatistics() const { return statistics_; }
    
    /**
     * @brief Reset statistics counters
     */
    void resetStatistics();

signals:
    /**
     * @brief Emitted when errors are detected and corrected
     * @param result Correction result details
     */
    void errorscorrected(const CorrectionResult& result);
    
    /**
     * @brief Emitted when uncorrectable errors are detected
     * @param error_count Number of errors detected
     * @param max_correctable Maximum correctable errors
     */
    void uncorrectableErrors(uint16_t error_count, uint16_t max_correctable);

private:
    // Core RS algorithms
    RSPolynomial generateGeneratorPolynomial();
    std::vector<uint16_t> findErrorLocator(const std::vector<uint16_t>& syndrome);
    std::vector<uint16_t> findErrorPositions(const std::vector<uint16_t>& error_locator);
    std::vector<uint16_t> findErrorValues(const std::vector<uint16_t>& syndrome,
                                         const std::vector<uint16_t>& error_positions);
    
    // Helper methods
    void updateStatistics(const CorrectionResult& result);
    bool isValidCodeword(std::span<const uint8_t> codeword) const;
    
    // Member variables
    RSCodecParams params_;
    bool initialized_{false};
    
    std::shared_ptr<GaloisField> galois_field_;
    RSPolynomial generator_poly_;
    
    Statistics statistics_;
};

/**
 * @brief ETI-specific Reed-Solomon error correction
 */
class ETIReedSolomonProcessor : public QObject {
    Q_OBJECT
    
public:
    explicit ETIReedSolomonProcessor(QObject* parent = nullptr);
    ~ETIReedSolomonProcessor() override;
    
    /**
     * @brief Initialize with standard ETI RS parameters
     */
    bool initialize();
    
    /**
     * @brief Process ETI frame for error correction
     * @param frame_data Raw ETI frame data
     * @return Correction result with potentially corrected frame
     */
    [[nodiscard]] CorrectionResult processETIFrame(std::span<const uint8_t, 6144> frame_data);
    
    /**
     * @brief Check FIC field for errors
     * @param fic_data FIC field data (32 bytes)
     * @return Correction result for FIC
     */
    [[nodiscard]] CorrectionResult checkFICField(std::span<const uint8_t, 32> fic_data);
    
    /**
     * @brief Check MSC sub-channel for errors
     * @param msc_data MSC sub-channel data
     * @param protection_level Protection level (1-5)
     * @return Correction result for MSC
     */
    [[nodiscard]] CorrectionResult checkMSCData(std::span<const uint8_t> msc_data,
                                               uint8_t protection_level);
    
    /**
     * @brief Get comprehensive error statistics
     */
    struct ETIErrorStats {
        CorrectionResult::Status overall_status{CorrectionResult::Status::SUCCESS};
        uint16_t fic_errors{0};
        uint16_t msc_errors{0};
        uint16_t total_errors_corrected{0};
        double signal_integrity{100.0};  // Overall signal integrity percentage
        
        [[nodiscard]] bool hasSignificantErrors() const {
            return signal_integrity < 95.0;
        }
    };
    
    [[nodiscard]] ETIErrorStats getETIErrorStats() const { return eti_stats_; }

signals:
    /**
     * @brief Emitted when ETI frame errors are corrected
     * @param frame_number Frame sequence number
     * @param stats Error correction statistics
     */
    void etiFrameCorrected(uint32_t frame_number, const ETIErrorStats& stats);
    
    /**
     * @brief Emitted when frame has uncorrectable errors
     * @param frame_number Frame sequence number
     * @param error_description Description of errors
     */
    void etiFrameUncorrectable(uint32_t frame_number, const QString& error_description);

private:
    std::unique_ptr<ReedSolomonCodec> fic_codec_;
    std::unique_ptr<ReedSolomonCodec> msc_codec_;
    
    ETIErrorStats eti_stats_;
    uint32_t frame_counter_{0};
    
    void updateETIStats(const std::vector<CorrectionResult>& results);
    CorrectionResult processProtectedData(std::span<const uint8_t> data, 
                                        ReedSolomonCodec* codec);
};

/**
 * @brief Factory functions for standard RS codecs
 */
namespace factory {
    /**
     * @brief Create MPEG-TS RS(204,188) codec
     */
    [[nodiscard]] std::unique_ptr<ReedSolomonCodec> createMPEGTSCodec(QObject* parent = nullptr);
    
    /**
     * @brief Create FIC RS(255,239) codec
     */
    [[nodiscard]] std::unique_ptr<ReedSolomonCodec> createFICCodec(QObject* parent = nullptr);
    
    /**
     * @brief Create ETI processor with all required codecs
     */
    [[nodiscard]] std::unique_ptr<ETIReedSolomonProcessor> createETIProcessor(QObject* parent = nullptr);
}

} // namespace eti::codec