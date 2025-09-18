#pragma once

#include <vector>
#include <cstdint>

namespace TestFixtures {
    namespace EtiStreams {
        /**
         * @brief Sample ETI frame data for testing
         * 
         * This represents a minimal valid ETI-NI frame structure
         * according to ETSI EN 300 799 standards for testing purposes.
         */
        const std::vector<uint8_t> SAMPLE_ETI_FRAME = {
            // ETI-NI Frame Header (ETSI EN 300 799 Section 5.3.1)
            0x49, 0x89, 0xAB, 0x96,  // ERR/FSYNC - Error level & Frame sync pattern
            
            // LIDATA field
            0x00, 0x00, 0x00, 0x00,  // FC (Frame Characterization) - 4 bytes
            0x00, 0x00, 0x00, 0x00,  // NST, FL, reserved fields
            
            // STC (Stream Time Code) - 4 bytes  
            0x00, 0x00, 0x00, 0x00,
            
            // EOH (End of Header) - 4 bytes
            0x00, 0x00, 0x00, 0x00,
            
            // MST (Main Service Transport) - minimal content
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            
            // EOF (End of Frame) - 4 bytes
            0xFF, 0xFF, 0xFF, 0xFF
        };
        
        /**
         * @brief Sample FIC (Fast Information Channel) data
         * 
         * Contains test FIG (Fast Information Group) data for parsing tests
         */
        const std::vector<uint8_t> SAMPLE_FIC_DATA = {
            // FIG Type 0/0 - Ensemble information
            0x00, 0x08,  // FIG header (Type 0, Length 8)
            0x12, 0x34,  // EId (Ensemble Identifier)
            0x00, 0x01,  // Change flag, Alarm flag, CIF count
            
            // FIG Type 0/1 - Service information
            0x01, 0x0C,  // FIG header (Type 1, Length 12)
            0x56, 0x78,  // SId (Service Identifier)
            0x10,        // Service flags
            0x20, 0x00,  // Subchannel info
        };
        
        /**
         * @brief Sample MSC (Main Service Channel) data
         * 
         * Contains test audio stream data for processing tests
         */
        const std::vector<uint8_t> SAMPLE_MSC_DATA = {
            // DAB+ AAC frame header
            0xFF, 0xF1, 0x50, 0x80, 0x01, 0x1F, 0xFC,
            
            // Sample AAC payload (simplified)
            0x21, 0x10, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        
        /**
         * @brief Create a valid ETI frame for testing
         * @return Vector containing complete ETI frame
         */
        std::vector<uint8_t> createValidEtiFrame() {
            return SAMPLE_ETI_FRAME;
        }
        
        /**
         * @brief Create an invalid ETI frame for error testing
         * @return Vector containing invalid ETI frame
         */
        std::vector<uint8_t> createInvalidEtiFrame() {
            std::vector<uint8_t> invalid_frame = SAMPLE_ETI_FRAME;
            // Corrupt the sync pattern to make it invalid
            invalid_frame[0] = 0x00;
            invalid_frame[1] = 0x00;
            return invalid_frame;
        }
        
        /**
         * @brief Create test FIG data of specific type
         * @param figType FIG type to create
         * @return Vector containing FIG data
         */
        std::vector<uint8_t> createTestFigData(uint8_t figType) {
            std::vector<uint8_t> fig_data;
            
            // FIG header
            fig_data.push_back(figType);
            fig_data.push_back(0x08); // Length
            
            // Add type-specific test data
            switch (figType) {
                case 0: // Ensemble info
                    fig_data.insert(fig_data.end(), {0x12, 0x34, 0x00, 0x01});
                    break;
                case 1: // Service info
                    fig_data.insert(fig_data.end(), {0x56, 0x78, 0x10, 0x20});
                    break;
                default:
                    fig_data.insert(fig_data.end(), {0x00, 0x00, 0x00, 0x00});
                    break;
            }
            
            return fig_data;
        }
    }
}