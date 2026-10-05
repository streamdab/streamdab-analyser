/**
 * @file test_segfault_fixes.cpp
 * @brief Unit tests for segmentation fault fixes
 * 
 * This file contains regression tests for all fixes applied to resolve
 * the segmentation fault when loading bkk_20062022_141637.eti
 * 
 * Fixes tested (updated for T24's shared-slice hand-off):
 * 1. ProcessedFrame copy is safe — slice bytes are refcounted (shared_ptr),
 *    not deep-copied, and stay alive after the source frame is destroyed
 * 2. QByteArray detach for Qt::QueuedConnection safety (fic_data)
 * 3. Pass-by-value for slots using QueuedConnection
 * 4. Empty / absent sub-channel slices handled safely
 * 5. Qt meta-type registration across compilation units
 * 
 * Date: October 24, 2025; T24 update: October 5, 2026
 */

#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QThread>
#include <memory>
#include <utility>
#include <initializer_list>
#include "../src/core/enhanced_eti_processor_qt.h"

namespace {
using Slice = ProcessedFrame::SubChannelSlice;

std::shared_ptr<const std::vector<Slice>> makeSlices(
    std::initializer_list<std::pair<uint8_t, QByteArray>> entries)
{
    auto slices = std::make_shared<std::vector<Slice>>();
    slices->reserve(entries.size());
    for (const auto& e : entries) {
        Slice s;
        s.sub_channel_id = e.first;
        s.data = e.second;
        slices->push_back(std::move(s));
    }
    return slices;
}
} // namespace

class TestSegfaultFixes : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qDebug() << "=== Segfault Fixes Regression Test Suite ===";
        qDebug() << "Testing all fixes applied on October 24, 2025 (T24 updated)";
    }

    /**
     * TEST 1: ProcessedFrame Copy — Shared Slice Ownership
     *
     * T24: the copy shares the `sub_channels` shared_ptr (single owner of the
     * MSC bytes) rather than deep-copying every slice. The bytes must stay valid
     * and identical for both frames, and `msc_size` must be preserved.
     */
    void test_ProcessedFrame_CopySharesSlices()
    {
        qDebug() << "\n[TEST 1] ProcessedFrame Copy — Shared Slice Ownership";

        ProcessedFrame original;
        original.frame_number = 1;
        original.format = ETIFormat::ETI_LI_A;
        original.sync_pattern = 0xfff8c549;
        original.lidata = 0x12345678;

        original.fic_data = QByteArray("FIC_DATA_TEST_123456789", 23);
        original.sub_channels = makeSlices({{7, QByteArray("MSC_DATA_TEST_ABCDEFGHIJKLMNOP", 31)}});
        original.msc_size = 31;
        original.crc = 0xABCDEF01;
        original.is_valid = true;

        // Copy shares the slice owner (no 31-byte duplication). Assert the
        // T24 semantics explicitly: the shared_ptr identity is preserved and the
        // copy bumps the refcount (1 -> 2), proving an O(1) hand-off.
        ProcessedFrame copy(original);
        QVERIFY2(copy.sub_channels == original.sub_channels,
                 "copy must share the same slice owner (no deep copy)");
        QCOMPARE(copy.sub_channels.get(), original.sub_channels.get());
        QCOMPARE(original.sub_channels.use_count(), 2L);
        QVERIFY(copy.hasSubChannels());
        QCOMPARE(int(copy.subChannels().size()), 1);
        QCOMPARE(copy.subChannels()[0].sub_channel_id, static_cast<uint8_t>(7));
        QCOMPARE(copy.subChannels()[0].data,
                 QByteArray("MSC_DATA_TEST_ABCDEFGHIJKLMNOP", 31));

        // Scalar + FIC fields preserved.
        QCOMPARE(copy.frame_number, original.frame_number);
        QCOMPARE(copy.format, original.format);
        QCOMPARE(copy.sync_pattern, original.sync_pattern);
        QCOMPARE(copy.lidata, original.lidata);
        QCOMPARE(copy.fic_data, original.fic_data);
        QCOMPARE(copy.msc_size, original.msc_size);
        QCOMPARE(copy.crc, original.crc);
        QCOMPARE(copy.is_valid, original.is_valid);

        QCOMPARE(copy.fic_data.size(), 23);
        QCOMPARE(copy.msc_size, static_cast<std::size_t>(31));

        qDebug() << "✓ Shared slice ownership verified — bytes alive, no duplication";
    }

    /**
     * TEST 2: QByteArray Detach After Copy
     * 
     * Verifies that fic_data copies remain independent (Qt COW detach).
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
     * Verifies scalar fields, msc_size and shared slices after assignment.
     */
    void test_ProcessedFrame_CopyAssignment()
    {
        qDebug() << "\n[TEST 3] ProcessedFrame Copy Assignment Operator";
        
        ProcessedFrame original;
        original.frame_number = 42;
        original.fic_data = QByteArray("ASSIGNMENT_TEST_FIC", 19);
        original.sub_channels = makeSlices({{5, QByteArray("ASSIGNMENT_TEST_MSC", 19)}});
        original.msc_size = 19;
        
        ProcessedFrame copy;
        copy = original;  // Use assignment operator

        // FIC is COW, so after assignment the two may still share a buffer.
        // Equality holds before mutation; independence is proven by mutating
        // the ORIGINAL after the copy and checking the copy is unchanged
        // (COW detaches only on write).
        QCOMPARE(copy.fic_data, original.fic_data);
        QCOMPARE(copy.fic_data.size(), 19);
        original.fic_data[0] = 'X';
        QVERIFY2(copy.fic_data[0] == 'A',
                 "copy must be COW-independent from the original after assignment");
        QVERIFY2(original.fic_data[0] == 'X', "original mutation applied");

        // Slice owner is shared and the assignment bumped its refcount; content
        // intact and msc_size copied.
        QVERIFY(copy.sub_channels == original.sub_channels);
        QCOMPARE(copy.sub_channels.get(), original.sub_channels.get());
        QCOMPARE(original.sub_channels.use_count(), 2L);
        QCOMPARE(copy.frame_number, 42);
        QCOMPARE(copy.msc_size, static_cast<std::size_t>(19));
        QCOMPARE(copy.subChannels().size(), static_cast<std::size_t>(1));
        QCOMPARE(copy.subChannels()[0].data.size(), 19);
        
        qDebug() << "✓ Copy assignment operator verified";
    }

    /**
     * TEST 4: Stack-to-Heap Copy Scenario
     * 
     * Simulates the exact scenario that caused the crash:
     * - Stack-allocated ProcessedFrame in a function
     * - Copy made for Qt::QueuedConnection
     * - Original destroyed when function returns
     * - The shared slice bytes must remain valid for the heap copy
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
            stack_frame.sub_channels = makeSlices({{3, QByteArray("STACK_MSC_DATA_67890", 20)}});
            stack_frame.msc_size = 20;
            
            // Simulate Qt::QueuedConnection copy to heap
            heap_copy = new ProcessedFrame(stack_frame);
            
            // stack_frame will be destroyed here when leaving scope
        }
        
        // CRITICAL: heap_copy must still be valid after stack_frame destroyed
        QVERIFY2(heap_copy != nullptr, "Heap copy exists");
        QCOMPARE(heap_copy->frame_number, 1);
        QCOMPARE(heap_copy->fic_data.size(), 20);
        QCOMPARE(heap_copy->msc_size, static_cast<std::size_t>(20));
        QVERIFY(heap_copy->hasSubChannels());
        QCOMPARE(heap_copy->subChannels()[0].data.size(), 20);
        
        // Verify data is still readable (would crash with dangling pointers)
        QVERIFY2(heap_copy->subChannels()[0].data.at(0) == 'S',
                 "Heap copy slice data still valid after stack destruction");
        
        delete heap_copy;
        
        qDebug() << "✓ Stack-to-heap copy safe - refcounted slices survived";
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
        
        // Verify type names (Qt6: use QMetaType::fromType, typeName(int) is deprecated)
        const char* processedFrameName = QMetaType::fromType<ProcessedFrame>().name();
        const char* etiFormatName = QMetaType::fromType<ETIFormat>().name();
        
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
        test_frame.sub_channels = makeSlices({{1, QByteArray("QUEUED_MSC_DATA", 15)}});
        test_frame.msc_size = 15;
        
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
        QCOMPARE(received.msc_size, static_cast<std::size_t>(15));
        QVERIFY(received.hasSubChannels());
        QCOMPARE(received.subChannels()[0].data.size(), 15);
        
        qDebug() << "✓ Qt::QueuedConnection safe - data intact after delivery";
    }

    /**
     * TEST 7: Absent / Empty MSC Slices
     * 
     * Verifies that a frame with no MSC slices (bounds errors / no MSC) is
     * handled safely without crashes.
     */
    void test_EmptySubChannelsSafety()
    {
        qDebug() << "\n[TEST 7] Absent MSC Slices Handling";
        
        ProcessedFrame frame;
        frame.frame_number = 1;
        frame.fic_data = QByteArray();  // Empty from bounds error
        frame.msc_size = 0;             // No MSC bytes
        // sub_channels stays null (no slices)
        
        // Copy should not crash
        ProcessedFrame copy(frame);
        
        // Access should not crash
        QCOMPARE(copy.fic_data.size(), 0);
        QCOMPARE(copy.msc_size, static_cast<std::size_t>(0));
        QVERIFY(!copy.hasSubChannels());
        QVERIFY(copy.subChannels().empty());
        QVERIFY(copy.fic_data.isEmpty());
        
        // constData() should return valid pointer even for empty
        QVERIFY2(copy.fic_data.constData() != nullptr,
                 "constData() safe for empty QByteArray");
        
        qDebug() << "✓ Absent MSC slices safe - no crashes";
    }

    /**
     * TEST 8: Large Slice Data Integrity
     * 
     * Verifies that realistic MSC slice data stays intact across copies and is
     * not duplicated (the shared owner keeps the total retained bytes low).
     */
    void test_LargeDataIntegrity()
    {
        qDebug() << "\n[TEST 8] Large Slice Data Integrity (Realistic MSC Size)";
        
        ProcessedFrame original;
        original.frame_number = 1;
        
        // FIC: 96 bytes (typical)
        original.fic_data = QByteArray(96, 'F');
        
        // MSC: 5614 bytes (typical combined sub-channels), split into slices.
        QByteArray slice_a(2807, 'M');
        QByteArray slice_b(2807, 'N');
        for (int i = 0; i < slice_a.size(); i++) slice_a[i] = static_cast<char>(i % 256);
        for (int i = 0; i < slice_b.size(); i++) slice_b[i] = static_cast<char>((i + 7) % 256);
        original.sub_channels = makeSlices({{0, slice_a}, {1, slice_b}});
        original.msc_size = 5614;
        
        // Create copy
        ProcessedFrame copy(original);
        
        // Verify sizes + shared owner
        QCOMPARE(copy.fic_data.size(), 96);
        QCOMPARE(copy.msc_size, static_cast<std::size_t>(5614));
        QVERIFY(copy.sub_channels == original.sub_channels);
        QCOMPARE(copy.subChannels().size(), static_cast<std::size_t>(2));
        QCOMPARE(copy.subChannels()[0].data.size(), 2807);
        QCOMPARE(copy.subChannels()[1].data.size(), 2807);
        
        // Verify data integrity (check pattern)
        for (int i = 0; i < copy.subChannels()[0].data.size(); i++) {
            QCOMPARE(copy.subChannels()[0].data.at(i), static_cast<char>(i % 256));
        }
        
        qDebug() << "✓ Large slice data verified:";
        qDebug() << "  FIC size:" << copy.fic_data.size() << "bytes";
        qDebug() << "  MSC size:" << copy.msc_size << "bytes";
    }

    /**
     * TEST 9: Multiple Copies Stress Test
     * 
     * Creates multiple copies in sequence to verify no memory leaks or
     * corruption; all copies share the single slice owner.
     */
    void test_MultipleCopiesStress()
    {
        qDebug() << "\n[TEST 9] Multiple Copies Stress Test";
        
        ProcessedFrame original;
        original.frame_number = 1;
        original.fic_data = QByteArray(96, 'F');
        original.sub_channels = makeSlices({{0, QByteArray(5614, 'M')}});
        original.msc_size = 5614;
        
        // Create 100 copies
        std::vector<ProcessedFrame> copies;
        for (int i = 0; i < 100; i++) {
            copies.push_back(ProcessedFrame(original));
        }
        
        // Verify all copies are valid and share the same slice owner.
        for (size_t i = 0; i < copies.size(); i++) {
            QCOMPARE(copies[i].frame_number, 1);
            QCOMPARE(copies[i].fic_data.size(), 96);
            QCOMPARE(copies[i].msc_size, static_cast<std::size_t>(5614));
            QVERIFY(copies[i].sub_channels == original.sub_channels);
            QCOMPARE(copies[i].subChannels()[0].data.size(), 5614);
        }
        
        qDebug() << "✓ 100 copies created and verified - single slice owner, no corruption";
    }

    /**
     * TEST 10: Pass-by-Value Slot Safety
     * 
     * Verifies that passing ProcessedFrame by value (not const reference)
     * to slots keeps the refcounted slices alive inside the slot.
     */
    void test_PassByValueSlotSafety()
    {
        qDebug() << "\n[TEST 10] Pass-by-Value Slot Safety";
        
        ProcessedFrame original;
        original.frame_number = 1;
        original.fic_data = QByteArray("PASS_BY_VALUE_TEST", 18);
        original.sub_channels = makeSlices({{0, QByteArray("SLICE_BYTES", 11)}});
        original.msc_size = 11;
        
        // Simulate pass-by-value copy
        auto slot_function = [](ProcessedFrame frame) {
            // Inside slot, we have our own copy
            QCOMPARE(frame.fic_data.size(), 18);
            QVERIFY(!frame.fic_data.isEmpty());
            QVERIFY(frame.hasSubChannels());
            QCOMPARE(frame.subChannels()[0].data.size(), 11);
        };
        
        // Call with temporary (simulates queued signal delivery)
        slot_function(ProcessedFrame(original));
        
        qDebug() << "✓ Pass-by-value safe - slice bytes alive inside the slot";
    }

    void cleanupTestCase()
    {
        qDebug() << "\n=== All Segfault Regression Tests Passed ===";
        qDebug() << "Fixes verified:";
        qDebug() << "  ✓ ProcessedFrame shared slice ownership (T24)";
        qDebug() << "  ✓ QByteArray detach() (fic_data)";
        qDebug() << "  ✓ Copy assignment operator";
        qDebug() << "  ✓ Stack-to-heap copy safety";
        qDebug() << "  ✓ Qt meta-type registration";
        qDebug() << "  ✓ Qt::QueuedConnection safety";
        qDebug() << "  ✓ Absent MSC slices handling";
        qDebug() << "  ✓ Large data integrity";
        qDebug() << "  ✓ Multiple copies stress test";
        qDebug() << "  ✓ Pass-by-value slot safety";
    }
};

QTEST_MAIN(TestSegfaultFixes)
#include "test_segfault_fixes.moc"
