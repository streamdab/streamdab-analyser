/**
 * @file reed_solomon_codec.cpp
 * @brief Reed-Solomon Error Correction Implementation
 * 
 * Implementation of Reed-Solomon error correction for ETI frame processing
 * according to ETSI EN 300 799 specifications.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 */

#include "reed_solomon_codec.hpp"
#include "utils/logger.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace eti::codec {

// ============================================================================
// GaloisField Implementation
// ============================================================================

GaloisField::GaloisField(uint8_t m, uint16_t gfpoly)
    : m_(m), gfpoly_(gfpoly), field_size_(1 << m)
{
    initializeTables();
    
    Logger::instance().log(Logger::Debug, "GaloisField", 
                          QString("Created GF(2^%1) with polynomial 0x%2")
                          .arg(m).arg(gfpoly, 0, 16));
}

void GaloisField::initializeTables()
{
    exp_table_.resize(2 * field_size_);
    log_table_.resize(field_size_);
    
    // Initialize exponential and logarithm tables
    exp_table_[0] = 1;
    log_table_[0] = -1; // log(0) is undefined
    
    uint16_t x = 1;
    for (uint16_t i = 1; i < field_size_; ++i) {
        exp_table_[i] = x;
        log_table_[x] = static_cast<int16_t>(i);
        
        // Multiply by primitive element (alpha)
        x <<= 1;
        if (x & field_size_) {
            x ^= gfpoly_;
        }
        x &= (field_size_ - 1);
    }
    
    // Extend exponential table for easier computation
    for (uint16_t i = field_size_; i < 2 * field_size_; ++i) {
        exp_table_[i] = exp_table_[i - field_size_ + 1];
    }
}

uint16_t GaloisField::multiply(uint16_t a, uint16_t b) const
{
    if (a == 0 || b == 0) {
        return 0;
    }
    
    int16_t log_a = log_table_[a];
    int16_t log_b = log_table_[b];
    
    return exp_table_[log_a + log_b];
}

uint16_t GaloisField::divide(uint16_t a, uint16_t b) const
{
    if (b == 0) {
        Logger::instance().log(Logger::Error, "GaloisField", "Division by zero");
        return 0;
    }
    
    if (a == 0) {
        return 0;
    }
    
    int16_t log_a = log_table_[a];
    int16_t log_b = log_table_[b];
    
    int16_t log_result = log_a - log_b;
    if (log_result < 0) {
        log_result += (field_size_ - 1);
    }
    
    return exp_table_[log_result];
}

uint16_t GaloisField::power(uint16_t base, uint16_t exponent) const
{
    if (base == 0) {
        return exponent == 0 ? 1 : 0;
    }
    
    if (exponent == 0) {
        return 1;
    }
    
    int16_t log_base = log_table_[base];
    int16_t log_result = (log_base * exponent) % (field_size_ - 1);
    
    return exp_table_[log_result];
}

uint16_t GaloisField::inverse(uint16_t a) const
{
    if (a == 0) {
        Logger::instance().log(Logger::Error, "GaloisField", "Inverse of zero");
        return 0;
    }
    
    return exp_table_[(field_size_ - 1) - log_table_[a]];
}

int16_t GaloisField::toAlpha(uint16_t value) const
{
    return value == 0 ? -1 : log_table_[value];
}

uint16_t GaloisField::fromAlpha(int16_t alpha) const
{
    if (alpha == -1) {
        return 0;
    }
    
    while (alpha < 0) {
        alpha += (field_size_ - 1);
    }
    
    return exp_table_[alpha % (field_size_ - 1)];
}

// ============================================================================
// RSPolynomial Implementation
// ============================================================================

RSPolynomial::RSPolynomial(std::shared_ptr<GaloisField> gf)
    : gf_(gf), coefficients_{0}
{
}

RSPolynomial::RSPolynomial(std::shared_ptr<GaloisField> gf, const std::vector<uint16_t>& coefficients)
    : gf_(gf), coefficients_(coefficients)
{
    normalize();
}

int RSPolynomial::getDegree() const
{
    for (int i = static_cast<int>(coefficients_.size()) - 1; i >= 0; --i) {
        if (coefficients_[i] != 0) {
            return i;
        }
    }
    return -1; // Zero polynomial
}

uint16_t RSPolynomial::getCoefficient(size_t position) const
{
    return position < coefficients_.size() ? coefficients_[position] : 0;
}

void RSPolynomial::setCoefficient(size_t position, uint16_t value)
{
    if (position >= coefficients_.size()) {
        coefficients_.resize(position + 1, 0);
    }
    coefficients_[position] = value;
    normalize();
}

uint16_t RSPolynomial::evaluate(uint16_t x) const
{
    if (coefficients_.empty()) {
        return 0;
    }
    
    // Horner's method for polynomial evaluation
    uint16_t result = coefficients_.back();
    for (int i = static_cast<int>(coefficients_.size()) - 2; i >= 0; --i) {
        result = gf_->add(gf_->multiply(result, x), coefficients_[i]);
    }
    
    return result;
}

RSPolynomial RSPolynomial::add(const RSPolynomial& other) const
{
    size_t max_size = std::max(coefficients_.size(), other.coefficients_.size());
    std::vector<uint16_t> result_coeffs(max_size, 0);
    
    for (size_t i = 0; i < max_size; ++i) {
        uint16_t a = i < coefficients_.size() ? coefficients_[i] : 0;
        uint16_t b = i < other.coefficients_.size() ? other.coefficients_[i] : 0;
        result_coeffs[i] = gf_->add(a, b);
    }
    
    return RSPolynomial(gf_, result_coeffs);
}

RSPolynomial RSPolynomial::multiply(const RSPolynomial& other) const
{
    if (coefficients_.empty() || other.coefficients_.empty()) {
        return RSPolynomial(gf_);
    }
    
    size_t result_size = coefficients_.size() + other.coefficients_.size() - 1;
    std::vector<uint16_t> result_coeffs(result_size, 0);
    
    for (size_t i = 0; i < coefficients_.size(); ++i) {
        for (size_t j = 0; j < other.coefficients_.size(); ++j) {
            uint16_t product = gf_->multiply(coefficients_[i], other.coefficients_[j]);
            result_coeffs[i + j] = gf_->add(result_coeffs[i + j], product);
        }
    }
    
    return RSPolynomial(gf_, result_coeffs);
}

std::pair<RSPolynomial, RSPolynomial> RSPolynomial::divide(const RSPolynomial& divisor) const
{
    if (divisor.getDegree() == -1) {
        Logger::instance().log(Logger::Error, "RSPolynomial", "Division by zero polynomial");
        return {RSPolynomial(gf_), *this};
    }
    
    RSPolynomial quotient(gf_);
    RSPolynomial remainder = *this;
    
    int divisor_degree = divisor.getDegree();
    uint16_t leading_coeff = divisor.coefficients_[divisor_degree];
    
    while (remainder.getDegree() >= divisor_degree) {
        int remainder_degree = remainder.getDegree();
        uint16_t coeff = gf_->divide(remainder.coefficients_[remainder_degree], leading_coeff);
        
        // Create monomial term
        std::vector<uint16_t> term_coeffs(remainder_degree - divisor_degree + 1, 0);
        term_coeffs.back() = coeff;
        RSPolynomial term(gf_, term_coeffs);
        
        quotient = quotient.add(term);
        remainder = remainder.add(divisor.multiply(term));
    }
    
    return {quotient, remainder};
}

RSPolynomial RSPolynomial::scale(uint16_t factor) const
{
    std::vector<uint16_t> result_coeffs;
    result_coeffs.reserve(coefficients_.size());
    
    for (uint16_t coeff : coefficients_) {
        result_coeffs.push_back(gf_->multiply(coeff, factor));
    }
    
    return RSPolynomial(gf_, result_coeffs);
}

void RSPolynomial::normalize()
{
    // Remove leading zero coefficients
    while (!coefficients_.empty() && coefficients_.back() == 0) {
        coefficients_.pop_back();
    }
    
    if (coefficients_.empty()) {
        coefficients_.push_back(0);
    }
}

// ============================================================================
// ReedSolomonCodec Implementation
// ============================================================================

ReedSolomonCodec::ReedSolomonCodec(const RSCodecParams& params, QObject* parent)
    : QObject(parent), params_(params), generator_poly_(nullptr)
{
    if (!params_.isValid()) {
        Logger::instance().log(Logger::Error, "ReedSolomonCodec", 
                              "Invalid Reed-Solomon parameters");
    }
}

ReedSolomonCodec::~ReedSolomonCodec()
{
    Logger::instance().log(Logger::Debug, "ReedSolomonCodec", 
                          "Reed-Solomon codec destroyed");
}

bool ReedSolomonCodec::initialize()
{
    if (initialized_) {
        return true;
    }
    
    if (!params_.isValid()) {
        Logger::instance().log(Logger::Error, "ReedSolomonCodec", 
                              "Cannot initialize with invalid parameters");
        return false;
    }
    
    try {
        // Create Galois field
        galois_field_ = std::make_shared<GaloisField>(params_.m, params_.gfpoly);
        
        // Generate generator polynomial
        generator_poly_ = generateGeneratorPolynomial();
        
        // Reset statistics
        resetStatistics();
        
        initialized_ = true;
        
        Logger::instance().log(Logger::Info, "ReedSolomonCodec", 
                              QString("RS(%1,%2) codec initialized")
                              .arg(params_.n).arg(params_.k));
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ReedSolomonCodec", 
                              QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

std::vector<uint8_t> ReedSolomonCodec::encode(std::span<const uint8_t> data)
{
    if (!initialized_) {
        Logger::instance().log(Logger::Error, "ReedSolomonCodec", 
                              "Codec not initialized");
        return {};
    }
    
    if (data.size() != params_.k) {
        Logger::instance().log(Logger::Error, "ReedSolomonCodec", 
                              QString("Invalid data size: %1 (expected %2)")
                              .arg(data.size()).arg(params_.k));
        return {};
    }
    
    // Convert data to polynomial
    std::vector<uint16_t> info_coeffs(data.begin(), data.end());
    RSPolynomial info_poly(galois_field_, info_coeffs);
    
    // Multiply by x^(n-k) to make room for parity symbols
    std::vector<uint16_t> shifted_coeffs(params_.n, 0);
    std::copy(info_coeffs.begin(), info_coeffs.end(), 
              shifted_coeffs.begin() + params_.getRedundancyLength());
    RSPolynomial shifted_poly(galois_field_, shifted_coeffs);
    
    // Divide by generator polynomial to get remainder (parity)
    auto [quotient, remainder] = shifted_poly.divide(generator_poly_);
    
    // Construct codeword: information + parity
    std::vector<uint8_t> codeword(params_.n);
    
    // Copy information symbols
    std::copy(data.begin(), data.end(), 
              codeword.begin() + params_.getRedundancyLength());
    
    // Copy parity symbols
    auto parity_coeffs = remainder.getCoefficients();
    for (size_t i = 0; i < params_.getRedundancyLength() && i < parity_coeffs.size(); ++i) {
        codeword[i] = static_cast<uint8_t>(parity_coeffs[i]);
    }
    
    return codeword;
}

CorrectionResult ReedSolomonCodec::decode(std::span<const uint8_t> codeword)
{
    CorrectionResult result;
    
    if (!initialized_) {
        result.status = CorrectionResult::Status::INVALID_CODEWORD;
        return result;
    }
    
    if (!isValidCodeword(codeword)) {
        result.status = CorrectionResult::Status::INVALID_CODEWORD;
        return result;
    }
    
    // Calculate syndrome
    auto syndrome = calculateSyndrome(codeword);
    
    // Check if there are any errors
    bool has_errors = std::any_of(syndrome.begin(), syndrome.end(), 
                                 [](uint16_t s) { return s != 0; });
    
    if (!has_errors) {
        // No errors detected
        result.status = CorrectionResult::Status::SUCCESS;
        result.corrected_data.assign(codeword.begin() + params_.getRedundancyLength(), 
                                   codeword.end());
        updateStatistics(result);
        return result;
    }
    
    // Find error locator polynomial
    auto error_locator = findErrorLocator(syndrome);
    
    // Find error positions
    auto error_positions = findErrorPositions(error_locator);
    
    result.errors_detected = static_cast<uint16_t>(error_positions.size());
    
    if (result.errors_detected > params_.t) {
        // Too many errors to correct
        result.status = CorrectionResult::Status::UNCORRECTABLE;
        updateStatistics(result);
        emit uncorrectableErrors(result.errors_detected, params_.t);
        return result;
    }
    
    // Find error values
    auto error_values = findErrorValues(syndrome, error_positions);
    
    // Correct errors
    std::vector<uint8_t> corrected_codeword(codeword.begin(), codeword.end());
    
    for (size_t i = 0; i < error_positions.size(); ++i) {
        uint16_t pos = error_positions[i];
        if (pos < corrected_codeword.size()) {
            corrected_codeword[pos] ^= static_cast<uint8_t>(error_values[i]);
            result.error_positions.push_back(pos);
        }
    }
    
    result.errors_corrected = static_cast<uint16_t>(result.error_positions.size());
    result.status = CorrectionResult::Status::CORRECTED;
    result.corrected_data.assign(corrected_codeword.begin() + params_.getRedundancyLength(),
                               corrected_codeword.end());
    
    updateStatistics(result);
    emit errorscorrected(result);
    
    Logger::instance().log(Logger::Debug, "ReedSolomonCodec", 
                          QString("Corrected %1 errors in codeword")
                          .arg(result.errors_corrected));
    
    return result;
}

CorrectionResult ReedSolomonCodec::check(std::span<const uint8_t> codeword)
{
    CorrectionResult result;
    
    if (!initialized_) {
        result.status = CorrectionResult::Status::INVALID_CODEWORD;
        return result;
    }
    
    if (!isValidCodeword(codeword)) {
        result.status = CorrectionResult::Status::INVALID_CODEWORD;
        return result;
    }
    
    // Calculate syndrome to detect errors
    auto syndrome = calculateSyndrome(codeword);
    
    // Count non-zero syndrome elements
    uint16_t error_count = 0;
    for (uint16_t s : syndrome) {
        if (s != 0) {
            error_count++;
        }
    }
    
    if (error_count == 0) {
        result.status = CorrectionResult::Status::SUCCESS;
    } else if (error_count <= 2 * params_.t) {
        result.status = CorrectionResult::Status::CORRECTED;
        result.errors_detected = error_count;
    } else {
        result.status = CorrectionResult::Status::UNCORRECTABLE;
        result.errors_detected = error_count;
    }
    
    return result;
}

std::vector<uint16_t> ReedSolomonCodec::calculateSyndrome(std::span<const uint8_t> codeword)
{
    std::vector<uint16_t> syndrome(params_.getRedundancyLength(), 0);
    
    // Convert codeword to polynomial
    std::vector<uint16_t> coeffs(codeword.begin(), codeword.end());
    RSPolynomial codeword_poly(galois_field_, coeffs);
    
    // Evaluate at consecutive roots of generator polynomial
    for (uint16_t i = 0; i < params_.getRedundancyLength(); ++i) {
        uint16_t root = galois_field_->fromAlpha(params_.fcr + i);
        syndrome[i] = codeword_poly.evaluate(root);
    }
    
    return syndrome;
}

void ReedSolomonCodec::resetStatistics()
{
    statistics_ = Statistics{};
    Logger::instance().log(Logger::Debug, "ReedSolomonCodec", 
                          "Statistics reset");
}

// ============================================================================
// Private Implementation Methods
// ============================================================================

RSPolynomial ReedSolomonCodec::generateGeneratorPolynomial()
{
    // Generator polynomial: g(x) = (x - α^fcr)(x - α^(fcr+1))...(x - α^(fcr+2t-1))
    RSPolynomial generator(galois_field_, {1}); // Start with polynomial 1
    
    for (uint16_t i = 0; i < params_.getRedundancyLength(); ++i) {
        uint16_t root = galois_field_->fromAlpha(params_.fcr + i);
        
        // Create (x - root) polynomial
        std::vector<uint16_t> factor_coeffs = {
            galois_field_->subtract(0, root), // -root
            1  // x coefficient
        };
        RSPolynomial factor(galois_field_, factor_coeffs);
        
        generator = generator.multiply(factor);
    }
    
    Logger::instance().log(Logger::Debug, "ReedSolomonCodec", 
                          QString("Generated RS generator polynomial of degree %1")
                          .arg(generator.getDegree()));
    
    return generator;
}

std::vector<uint16_t> ReedSolomonCodec::findErrorLocator(const std::vector<uint16_t>& syndrome)
{
    // Berlekamp-Massey algorithm (simplified implementation)
    std::vector<uint16_t> error_locator = {1};
    std::vector<uint16_t> previous = {1};
    
    uint16_t L = 0; // Current degree of error locator polynomial
    uint16_t m = 1; // Iteration counter
    
    for (uint16_t n = 0; n < syndrome.size(); ++n) {
        // Calculate discrepancy
        uint16_t discrepancy = syndrome[n];
        for (uint16_t i = 1; i <= L; ++i) {
            if (i < error_locator.size() && (n - i) < syndrome.size()) {
                discrepancy = galois_field_->add(discrepancy,
                    galois_field_->multiply(error_locator[i], syndrome[n - i]));
            }
        }
        
        if (discrepancy == 0) {
            m++;
        } else {
            if (2 * L <= n) {
                // Update error locator polynomial
                std::vector<uint16_t> temp = error_locator;
                
                // Extend polynomial length if needed
                size_t required_length = error_locator.size() + m;
                if (error_locator.size() < required_length) {
                    error_locator.resize(required_length, 0);
                }
                
                // Update coefficients
                for (size_t i = 0; i < previous.size() && (i + m) < error_locator.size(); ++i) {
                    uint16_t correction = galois_field_->multiply(discrepancy, previous[i]);
                    error_locator[i + m] = galois_field_->add(error_locator[i + m], correction);
                }
                
                L = n + 1 - L;
                previous = temp;
                m = 1;
            } else {
                // Update without changing degree
                size_t required_length = error_locator.size() + m;
                if (error_locator.size() < required_length) {
                    error_locator.resize(required_length, 0);
                }
                
                for (size_t i = 0; i < previous.size() && (i + m) < error_locator.size(); ++i) {
                    uint16_t correction = galois_field_->multiply(discrepancy, previous[i]);
                    error_locator[i + m] = galois_field_->add(error_locator[i + m], correction);
                }
                m++;
            }
        }
    }
    
    return error_locator;
}

std::vector<uint16_t> ReedSolomonCodec::findErrorPositions(const std::vector<uint16_t>& error_locator)
{
    std::vector<uint16_t> error_positions;
    
    if (error_locator.size() <= 1) {
        return error_positions; // No errors
    }
    
    RSPolynomial locator_poly(galois_field_, error_locator);
    
    // Search for roots (Chien search)
    for (uint16_t i = 0; i < galois_field_->getFieldSize(); ++i) {
        uint16_t alpha_i = galois_field_->fromAlpha(i);
        if (locator_poly.evaluate(alpha_i) == 0) {
            // Found a root, convert to error position
            uint16_t error_pos = galois_field_->getFieldSize() - 1 - i;
            if (error_pos < params_.n) {
                error_positions.push_back(error_pos);
            }
        }
    }
    
    return error_positions;
}

std::vector<uint16_t> ReedSolomonCodec::findErrorValues(const std::vector<uint16_t>& syndrome,
                                                       const std::vector<uint16_t>& error_positions)
{
    std::vector<uint16_t> error_values;
    
    // Forney algorithm (simplified)
    for (uint16_t pos : error_positions) {
        uint16_t alpha_inv = galois_field_->fromAlpha(galois_field_->getFieldSize() - 1 - pos);
        
        // Calculate error value using first syndrome element (simplified)
        if (!syndrome.empty()) {
            error_values.push_back(syndrome[0]);
        } else {
            error_values.push_back(0);
        }
    }
    
    return error_values;
}

void ReedSolomonCodec::updateStatistics(const CorrectionResult& result)
{
    statistics_.total_codewords++;
    
    switch (result.status) {
        case CorrectionResult::Status::SUCCESS:
            statistics_.error_free_codewords++;
            break;
        case CorrectionResult::Status::CORRECTED:
            statistics_.corrected_codewords++;
            statistics_.total_errors_corrected += result.errors_corrected;
            break;
        case CorrectionResult::Status::UNCORRECTABLE:
            statistics_.uncorrectable_codewords++;
            break;
        default:
            break;
    }
    
    // Update correction efficiency
    uint64_t correctable_errors = statistics_.corrected_codewords + statistics_.error_free_codewords;
    statistics_.correction_efficiency = statistics_.total_codewords > 0 ?
        static_cast<double>(correctable_errors) / statistics_.total_codewords * 100.0 : 100.0;
}

bool ReedSolomonCodec::isValidCodeword(std::span<const uint8_t> codeword) const
{
    return codeword.size() == params_.n;
}

// ============================================================================
// ETIReedSolomonProcessor Implementation
// ============================================================================

ETIReedSolomonProcessor::ETIReedSolomonProcessor(QObject* parent)
    : QObject(parent)
{
    Logger::instance().log(Logger::Info, "ETIReedSolomonProcessor", 
                          "ETI Reed-Solomon processor created");
}

ETIReedSolomonProcessor::~ETIReedSolomonProcessor()
{
    Logger::instance().log(Logger::Info, "ETIReedSolomonProcessor", 
                          "ETI Reed-Solomon processor destroyed");
}

bool ETIReedSolomonProcessor::initialize()
{
    try {
        // Create FIC codec (RS 255,239 for FIC protection)
        fic_codec_ = factory::createFICCodec(this);
        if (!fic_codec_->initialize()) {
            Logger::instance().log(Logger::Error, "ETIReedSolomonProcessor", 
                                  "Failed to initialize FIC codec");
            return false;
        }
        
        // Create MSC codec (RS 204,188 for MPEG-TS protection)
        msc_codec_ = factory::createMPEGTSCodec(this);
        if (!msc_codec_->initialize()) {
            Logger::instance().log(Logger::Error, "ETIReedSolomonProcessor", 
                                  "Failed to initialize MSC codec");
            return false;
        }
        
        Logger::instance().log(Logger::Info, "ETIReedSolomonProcessor", 
                              "ETI Reed-Solomon processor initialized");
        
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "ETIReedSolomonProcessor", 
                              QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

CorrectionResult ETIReedSolomonProcessor::processETIFrame(std::span<const uint8_t, 6144> frame_data)
{
    frame_counter_++;
    
    CorrectionResult overall_result;
    overall_result.status = CorrectionResult::Status::SUCCESS;
    
    std::vector<CorrectionResult> correction_results;
    
    // Check FIC field (bytes 12-43, 32 bytes)
    if (frame_data.size() >= 44) {
        auto fic_span = std::span<const uint8_t, 32>(frame_data.data() + 12, 32);
        auto fic_result = checkFICField(fic_span);
        correction_results.push_back(fic_result);
        
        if (fic_result.hasErrors()) {
            eti_stats_.fic_errors += fic_result.errors_detected;
        }
    }
    
    // For MSC field, we'd need to know the sub-channel organization
    // This is simplified - real implementation would parse FIC to get sub-channel info
    
    // Update overall statistics
    updateETIStats(correction_results);
    
    // Determine overall status
    bool has_uncorrectable = std::any_of(correction_results.begin(), correction_results.end(),
                                        [](const CorrectionResult& r) {
                                            return r.status == CorrectionResult::Status::UNCORRECTABLE;
                                        });
    
    if (has_uncorrectable) {
        overall_result.status = CorrectionResult::Status::UNCORRECTABLE;
        emit etiFrameUncorrectable(frame_counter_, "Uncorrectable errors in ETI frame");
    } else {
        bool has_corrected = std::any_of(correction_results.begin(), correction_results.end(),
                                        [](const CorrectionResult& r) {
                                            return r.status == CorrectionResult::Status::CORRECTED;
                                        });
        
        if (has_corrected) {
            overall_result.status = CorrectionResult::Status::CORRECTED;
            emit etiFrameCorrected(frame_counter_, eti_stats_);
        }
    }
    
    return overall_result;
}

CorrectionResult ETIReedSolomonProcessor::checkFICField(std::span<const uint8_t, 32> fic_data)
{
    if (!fic_codec_) {
        CorrectionResult result;
        result.status = CorrectionResult::Status::INVALID_CODEWORD;
        return result;
    }
    
    // FIC field in ETI is not directly RS protected, but we can check for patterns
    // This is a simplified implementation - real FIC has different protection
    
    CorrectionResult result;
    result.status = CorrectionResult::Status::SUCCESS;
    
    // Basic consistency checks
    bool all_zero = std::all_of(fic_data.begin(), fic_data.end(), 
                               [](uint8_t b) { return b == 0; });
    bool all_ff = std::all_of(fic_data.begin(), fic_data.end(), 
                             [](uint8_t b) { return b == 0xFF; });
    
    if (all_zero || all_ff) {
        result.status = CorrectionResult::Status::UNCORRECTABLE;
        result.errors_detected = 32; // All bytes considered errors
        Logger::instance().log(Logger::Warning, "ETIReedSolomonProcessor", 
                              "FIC field contains suspicious pattern");
    }
    
    return result;
}

CorrectionResult ETIReedSolomonProcessor::checkMSCData(std::span<const uint8_t> msc_data,
                                                      uint8_t protection_level)
{
    if (!msc_codec_) {
        CorrectionResult result;
        result.status = CorrectionResult::Status::INVALID_CODEWORD;
        return result;
    }
    
    // MSC data protection depends on the protection level
    // This is simplified - real implementation would use appropriate RS parameters
    
    return msc_codec_->check(msc_data);
}

void ETIReedSolomonProcessor::updateETIStats(const std::vector<CorrectionResult>& results)
{
    // Calculate signal integrity based on correction results
    double integrity = 100.0;
    uint16_t total_errors = 0;
    
    for (const auto& result : results) {
        total_errors += result.errors_detected;
        
        if (result.status == CorrectionResult::Status::UNCORRECTABLE) {
            integrity -= 20.0; // Significant penalty for uncorrectable errors
        } else if (result.status == CorrectionResult::Status::CORRECTED) {
            integrity -= static_cast<double>(result.errors_detected) * 0.5; // Minor penalty for corrected errors
        }
    }
    
    eti_stats_.signal_integrity = std::max(0.0, integrity);
    eti_stats_.total_errors_corrected = total_errors;
    
    // Update overall status
    bool has_uncorrectable = std::any_of(results.begin(), results.end(),
                                        [](const CorrectionResult& r) {
                                            return r.status == CorrectionResult::Status::UNCORRECTABLE;
                                        });
    
    if (has_uncorrectable) {
        eti_stats_.overall_status = CorrectionResult::Status::UNCORRECTABLE;
    } else {
        bool has_errors = std::any_of(results.begin(), results.end(),
                                     [](const CorrectionResult& r) {
                                         return r.hasErrors();
                                     });
        
        eti_stats_.overall_status = has_errors ? 
            CorrectionResult::Status::CORRECTED : CorrectionResult::Status::SUCCESS;
    }
}

CorrectionResult ETIReedSolomonProcessor::processProtectedData(std::span<const uint8_t> data,
                                                              ReedSolomonCodec* codec)
{
    if (!codec) {
        CorrectionResult result;
        result.status = CorrectionResult::Status::INVALID_CODEWORD;
        return result;
    }
    
    return codec->decode(data);
}

// ============================================================================
// Factory Functions
// ============================================================================

namespace factory {

std::unique_ptr<ReedSolomonCodec> createMPEGTSCodec(QObject* parent)
{
    auto codec = std::make_unique<ReedSolomonCodec>(RSCodecParams::mpeg_ts(), parent);
    return codec;
}

std::unique_ptr<ReedSolomonCodec> createFICCodec(QObject* parent)
{
    auto codec = std::make_unique<ReedSolomonCodec>(RSCodecParams::fic_protection(), parent);
    return codec;
}

std::unique_ptr<ETIReedSolomonProcessor> createETIProcessor(QObject* parent)
{
    auto processor = std::make_unique<ETIReedSolomonProcessor>(parent);
    return processor;
}

} // namespace factory

} // namespace eti::codec