#!/usr/bin/env python3
"""Create a test ETI file for StreamDAB validation"""

def create_test_eti_file():
    # ETI frame size is 6144 bytes
    ETI_FRAME_SIZE = 6144
    NUM_FRAMES = 100  # Create 100 test frames

    # ETI sync pattern: 0x49, 0x93, 0x1E, 0x03
    sync_pattern = bytes([0x49, 0x93, 0x1E, 0x03])

    with open("test_stream.eti", "wb") as f:
        for frame_num in range(NUM_FRAMES):
            # Create ETI frame
            frame = bytearray(ETI_FRAME_SIZE)

            # Add sync pattern at beginning
            frame[0:4] = sync_pattern

            # Add frame number (for testing)
            frame[4] = frame_num & 0xFF
            frame[5] = (frame_num >> 8) & 0xFF

            # Fill rest with test pattern
            for i in range(6, ETI_FRAME_SIZE):
                frame[i] = (i + frame_num) & 0xFF

            f.write(frame)

    print(f"Created test_stream.eti with {NUM_FRAMES} frames ({NUM_FRAMES * ETI_FRAME_SIZE} bytes)")

if __name__ == "__main__":
    create_test_eti_file()
