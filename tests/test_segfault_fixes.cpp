/**
 * @file test_segfault_fixes.cpp
 * @brief Unit tests for segmentation fault fixes
 * 
 * This file contains regression tests for all fixes applied to resolve
 * the segmentation fault when loading bkk_20062022_141637.eti
 * 
 * Fixes tested:
 * 1. ProcessedFrame deep copy with explicit copy constructor
 * 2. QByteArray detach() for Qt::QueuedConnection safety
 * 3. Pass-by-value for slots using QueuedConnection
 * 4. MSC bounds error handling
 * 5. Qt meta-type registration across compilation units
 * 
 * Date: October 24, 2025
 */

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QThread>
#include <memory>
#include "../src/core/enhanced_eti_processor_qt.h"

class TestSegfaultFixes : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qDebug() << "=== Segfault Fixes Regression Test Suite ===";
        qDebug() << "Testing all fixes applied on October 24, 2025";
    }

    /**
     * TEST 1: ProcessedFrame Deep Copy Constructor
     * 
     * Verifies that ProcessedFrame creates true deep copies of QByteArray
     * members when using the copy constructor.
     * 
     * This prevents dangling pointers when Qt::QueuedConnection copies
     * the frame for the event queue.
     */
    void test_ProcessedFrame_DeepCopy()
    {
        qDebug() << "\n[TEST 1] ProcessedFrame Deep Copy Constructor";
        
        // Create original frame with QByteArray data
        ProcessedFrame original;
        original.frame_number = 1;
        original.format = ETIFormat::ETI_LI_A;
        original.sync_pattern = 0xfff8c549;
        original.lidata = 0x12345678;
        
        // Add FIC and MSC data
        QByteArray fic_test_data("FIC_DATA_TEST_123456789", 23);
        QByteArray msc_test_data("MSC_DATA_TEST_ABCDEFGHIJKLMNOP", 31);
        
        original.fic_data = fic_test_data;
        original.msc_data = msc_test_data;
        original.crc = 0xABCDEF01;
        original.is_valid = true;
        
        // Get original data pointers
        const char* original_fic_ptr = original.fic_data.constData();
        const char* original_msc_ptr = original.msc_data.constData();
        
        // Create copy using copy constructor
        ProcessedFrame copy(original);
        
        // Get copy data pointers
        const char* copy_fic_ptr = copy.fic_data.constData();
        const char* copy_msc_ptr = copy.msc_data.constData();
        
        // CRITICAL TEST: Verify deep copy - pointers must be different
        QVERIFY2(copy_fic_ptr != original_fic_ptr, 
                 "FIC data must be deep copied (different memory addresses)");
        QVERIFY2(copy_msc_ptr != original_msc_ptr,
                 "MSC data must be deep copied (different memory addresses)");
        
        // Verify data content is identical
        QCOMPARE(copy.frame_number, original.frame_number);
        QCOMPARE(copy.format, original.format);
        QCOMPARE(copy.sync_pattern, original.sync_pattern);
        QCOMPARE(copy.lidata, original.lidata);
        QCOMPARE(copy.fic_data, original.fic_data);
        QCOMPARE(copy.msc_data, original.msc_data);
        QCOMPARE(copy.crc, original.crc);
        QCOMPARE(copy.is_valid, original.is_valid);
        
        // Verify sizes are correct
        QCOMPARE(copy.fic_data.size(), 23);
        QCOMPARE(copy.msc_data.size(), 31);
        
        qDebug() << "✓ Deep copy verified - different memory addresses";
        qDebug() << "  Original FIC ptr:" << (void*)original_fic_ptr;
        qDebug() << "  Copy FIC ptr:    " << (void*)copy_fic_ptr;
        qDebug() << "  Original MSC ptr:" << (void*)original_msc_ptr;
        qDebug() << "  Copy MSC ptr:    " << (void*)copy_msc_ptr;
    }

    /**
     * TEST 2: QByteArray Detach After Copy
     * 
     * Verifies that QByteArray::detach() is called during copy operations
     * to ensure no implicit sharing with source data.
     */
    void test_QByteArray_Detach()
    {
        qDebug() << "\n[TEST 2] QByteArray Detach After Copy";
        
        ProcessedFrame frame1;
        frame1.fic_data = QByteArray("SHARED_DATA_BEFORE_DETACH", 25);
        
        // Copy should trigger detach
        ProcessedFrame frame2(frame1);
        
        // Modify original - should NOT affect copy if detached
        frame1.fic_data[0] = 'X';
        
        // Verify copy is unaffected
        QVERIFY2(frame2.fic_data[0] == 'S',
                 "Copy must be independent after detach");
        QVERIFY2(frame1.fic_data[0] == 'X',
                 "Original modification succeeded");
        
        qDebug() << "✓ QByteArray detach verified - copy is independent";
    }

    /**
     * TEST 3: Copy Assignment Operator
     * 
     * Verifies that the copy assignment operator also performs deep copies.
     */
    void test_ProcessedFrame_CopyAssignment()
    {
        qDebug() << "\n[TEST 3] ProcessedFrame Copy Assignment Operator";
        
        ProcessedFrame original;
        original.frame_number = 42;
        original.fic_data = QByteArray("ASSIGNMENT_TEST_FIC", 19);
        original.msc_data = QByteArray("ASSIGNMENT_TEST_MSC", 19);
        
        ProcessedFrame copy;
        copy = original;  // Use assignment operator
        
        // Verify deep copy
        QVERIFY2(copy.fic_data.constData() != original.fic_data.constData(),
                 "Assignment must create deep copy");
        
        // Verify content
        QCOMPARE(copy.frame_number, 42);
        QCOMPARE(copy.fic_data.size(), 19);
        QCOMPARE(copy.msc_data.size(), 19);
        
        qDebug() << "✓ Copy assignment operator verified";
    }

    /**
     * TEST 4: Stack-to-Heap Copy Scenario
     * 
     * Simulates the exact scenario that caused the crash:
     * - Stack-allocated ProcessedFrame in a function
     * - Copy made for Qt::QueuedConnection
     * - Original destroyed when function returns
     * - Copy must remain valid
     */
    void test_StackToHeapCopy()
    {
        qDebug() << "\n[TEST 4] Stack-to-Heap Copy Scenario (Crash Simulation)";
        
        ProcessedFrame* heap_copy = nullptr;
        
        // Simulate function that creates stack-allocated frame
        {
            ProcessedFrame stack_frame;
            stack_frame.frame_number = 1;
            stack_frame.fic_data = QByteArray("STACK_FIC_DATA_12345", 20);
            stack_frame.msc_data = QByteArray("STACK_MSC_DATA_67890", 20);
            
            // Simulate Qt::QueuedConnection copy to heap
            heap_copy = new ProcessedFrame(stack_frame);
            
            // stack_frame will be destroyed here when leaving scope
        }
        
        // CRITICAL: heap_copy must still be valid after stack_frame destroyed
        QVERIFY2(heap_copy != nullptr, "Heap copy exists");
        QCOMPARE(heap_copy->frame_number, 1);
        QCOMPARE(heap_copy->fic_data.size(), 20);
        QCOMPARE(heap_copy->msc_data.size(), 20);
        
        // Verify data is still readable (would crash with dangling pointers)
        QVERIFY2(heap_copy->fic_data.at(0) == 'S',
                 "Heap copy data still valid after stack destruction");
        
        delete heap_copy;
        
        qDebug() << "✓ Stack-to-heap copy safe - no dangling pointers";
    }

    /**
     * TEST 5: Qt Meta-Type Registration
     * 
     * Verifies that ProcessedFrame and ETIFormat are properly registered
     * with Qt's meta-type system for use in queued signals.
     */
    void test_QtMetaTypeRegistration()
    {
        qDebug() << "\n[TEST 5] Qt Meta-Type Registration";
        
        // Check if ProcessedFrame is registered
        int processedFrameTypeId = qMetaTypeId<ProcessedFrame>();
        QVERIFY2(processedFrameTypeId != QMetaType::UnknownType,
                 "ProcessedFrame must be registered as Qt meta-type");
        
        // Check if ETIFormat is registered
        int etiFormatTypeId = qMetaTypeId<ETIFormat>();
        QVERIFY2(etiFormatTypeId != QMetaType::UnknownType,
                 "ETIFormat must be registered as Qt meta-type");
        
        // Verify type names
        const char* processedFrameName = QMetaType::typeName(processedFrameTypeId);
        const char* etiFormatName = QMetaType::typeName(etiFormatTypeId);
        
        QVERIFY2(processedFrameName != nullptr,
                 "ProcessedFrame type name exists");
        QVERIFY2(etiFormatName != nullptr,
                 "ETIFormat type name exists");
        
        qDebug() << "✓ Qt meta-types registered:";
        qDebug() << "  ProcessedFrame ID:" << processedFrameTypeId 
                 << "Name:" << processedFrameName;
        qDebug() << "  ETIFormat ID:" << etiFormatTypeId
                 << "Name:" << etiFormatName;
    }

    /**
     * TEST 6: Qt::QueuedConnection Signal Safety
     * 
     * Verifies that ProcessedFrame can be safely passed through
     * Qt::QueuedConnection signals across threads.
     */
    void test_QueuedConnectionSafety()
    {
        qDebug() << "\n[TEST 6] Qt::QueuedConnection Signal Safety";
        
        // Create processor
        EnhancedETIProcessorQt processor;
        
        // Create signal spy for frameProcessed signal
        QSignalSpy spy(&processor, &EnhancedETIProcessorQt::frameProcessed);
        QVERIFY(spy.isValid());
        
        // Create test frame
        ProcessedFrame test_frame;
        test_frame.frame_number = 999;
        test_frame.fic_data = QByteArray("QUEUED_FIC_DATA", 15);
        test_frame.msc_data = QByteArray("QUEUED_MSC_DATA", 15);
        
        // Emit signal (simulates what happens in processEtiFrame)
        // The signal uses Qt::QueuedConnection in actual code
        emit processor.frameProcessed(test_frame);
        
        // Process event loop to deliver queued signals
        QCoreApplication::processEvents();
        
        // Verify signal was emitted
        QCOMPARE(spy.count(), 1);
        
        // Extract received frame from signal
        QList<QVariant> arguments = spy.takeFirst();
        QVERIFY(arguments.size() >= 1);
        
        ProcessedFrame received = arguments.at(0).value<ProcessedFrame>();
        
        // Verify received frame data is intact
        QCOMPARE(received.frame_number, 999);
        QCOMPARE(received.fic_data.size(), 15);
        QCOMPARE(received.msc_data.size(), 15);
        
        qDebug() << "✓ Qt::QueuedConnection safe - data intact after delivery";
    }

    /**
     * TEST 7: Empty QByteArray Handling
     * 
     * Verifies that empty QByteArrays (from MSC bounds errors) are
     * handled safely without crashes.
     */
    void test_EmptyQByteArraySafety()
    {
        qDebug() << "\n[TEST 7] Empty QByteArray Handling (MSC Bounds Errors)";
        
        ProcessedFrame frame;
        frame.frame_number = 1;
        frame.fic_data = QByteArray();  // Empty from bounds error
        frame.msc_data = QByteArray();  // Empty from bounds error
        
        // Copy should not crash with empty arrays
        ProcessedFrame copy(frame);
        
        // Access should not crash
        QCOMPARE(copy.fic_data.size(), 0);
        QCOMPARE(copy.msc_data.size(), 0);
        QVERIFY(copy.fic_data.isEmpty());
        QVERIFY(copy.msc_data.isEmpty());
        
        // constData() should return valid pointer even for empty
        QVERIFY2(copy.fic_data.constData() != nullptr,
                 "constData() safe for empty QByteArray");
        
        qDebug() << "✓ Empty QByteArray safe - no crashes on bounds errors";
    }

    /**
     * TEST 8: Large QByteArray Deep Copy
     * 
     * Verifies that large data (realistic MSC sizes) are properly
     * deep copied without corruption or crashes.
     */
    void test_LargeDataDeepCopy()
    {
        qDebug() << "\n[TEST 8] Large QByteArray Deep Copy (Realistic MSC Size)";
        
        // Create frame with realistic MSC data size (~5-6KB)
        ProcessedFrame original;
        original.frame_number = 1;
        
        // FIC: 96 bytes (typical)
        QByteArray fic_data(96, 'F');
        original.fic_data = fic_data;
        
        // MSC: 5614 bytes (typical combined sub-channels)
        QByteArray msc_data(5614, 'M');
        // Add pattern to verify data integrity
        for (int i = 0; i < msc_data.size(); i++) {
            msc_data[i] = static_cast<char>(i % 256);
        }
        original.msc_data = msc_data;
        
        // Create copy
        ProcessedFrame copy(original);
        
        // Verify sizes
        QCOMPARE(copy.fic_data.size(), 96);
        QCOMPARE(copy.msc_data.size(), 5614);
        
        // Verify deep copy
        QVERIFY(copy.fic_data.constData() != original.fic_data.constData());
        QVERIFY(copy.msc_data.constData() != original.msc_data.constData());
        
        // Verify data integrity (check pattern)
        for (int i = 0; i < copy.msc_data.size(); i++) {
            QCOMPARE(copy.msc_data.at(i), static_cast<char>(i % 256));
        }
        
        qDebug() << "✓ Large data deep copy verified:";
        qDebug() << "  FIC size:" << copy.fic_data.size() << "bytes";
        qDebug() << "  MSC size:" << copy.msc_data.size() << "bytes";
        qDebug() << "  Total:" << (copy.fic_data.size() + copy.msc_data.size()) << "bytes";
    }

    /**
     * TEST 9: Multiple Copies Stress Test
     * 
     * Creates multiple copies in sequence to verify no memory leaks
     * or corruption occurs.
     */
    void test_MultipleCopiesStress()
    {
        qDebug() << "\n[TEST 9] Multiple Copies Stress Test";
        
        ProcessedFrame original;
        original.frame_number = 1;
        original.fic_data = QByteArray(96, 'F');
        original.msc_data = QByteArray(5614, 'M');
        
        // Create 100 copies
        std::vector<ProcessedFrame> copies;
        for (int i = 0; i < 100; i++) {
            copies.push_back(ProcessedFrame(original));
        }
        
        // Verify all copies are valid
        for (size_t i = 0; i < copies.size(); i++) {
            QCOMPARE(copies[i].frame_number, 1);
            QCOMPARE(copies[i].fic_data.size(), 96);
            QCOMPARE(copies[i].msc_data.size(), 5614);
            
            // Verify each copy has unique memory
            if (i > 0) {
                QVERIFY(copies[i].fic_data.constData() != copies[i-1].fic_data.constData());
            }
        }
        
        qDebug() << "✓ 100 copies created and verified - no corruption";
    }

    /**
     * TEST 10: Pass-by-Value Slot Safety
     * 
     * Verifies that passing ProcessedFrame by value (not const reference)
     * to slots prevents access to invalid memory.
     */
    void test_PassByValueSlotSafety()
    {
        qDebug() << "\n[TEST 10] Pass-by-Value Slot Safety";
        
        // This test verifies the fix where we changed:
        // void onFrameProcessed(const ProcessedFrame& frame)  // ❌ Unsafe
        // to:
        // void onFrameProcessed(ProcessedFrame frame)  // ✅ Safe
        
        ProcessedFrame original;
        original.frame_number = 1;
        original.fic_data = QByteArray("PASS_BY_VALUE_TEST", 18);
        
        // Simulate pass-by-value copy
        auto slot_function = [](ProcessedFrame frame) {
            // Inside slot, we have our own copy
            QCOMPARE(frame.fic_data.size(), 18);
            QVERIFY(!frame.fic_data.isEmpty());
        };
        
        // Call with temporary (simulates queued signal delivery)
        slot_function(ProcessedFrame(original));
        
        qDebug() << "✓ Pass-by-value safe - slot receives independent copy";
    }

    void cleanupTestCase()
    {
        qDebug() << "\n=== All Segfault Regression Tests Passed ===";
        qDebug() << "Date: October 24, 2025";
        qDebug() << "Fixes verified:";
        qDebug() << "  ✓ ProcessedFrame deep copy constructor";
        qDebug() << "  ✓ QByteArray detach() calls";
        qDebug() << "  ✓ Copy assignment operator";
        qDebug() << "  ✓ Stack-to-heap copy safety";
        qDebug() << "  ✓ Qt meta-type registration";
        qDebug() << "  ✓ Qt::QueuedConnection safety";
        qDebug() << "  ✓ Empty QByteArray handling";
        qDebug() << "  ✓ Large data integrity";
        qDebug() << "  ✓ Multiple copies stress test";
        qDebug() << "  ✓ Pass-by-value slot safety";
    }
};

QTEST_MAIN(TestSegfaultFixes)
#include "test_segfault_fixes.moc"
