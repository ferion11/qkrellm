// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/ringbuffer.h"

#include <QObject>
#include <QTest>

using qk::core::RingBuffer;

namespace {

class TestRingBuffer final : public QObject
{
    Q_OBJECT

private slots:
    void fillsInOrder()
    {
        RingBuffer<int> rb(3);
        QVERIFY(rb.empty());
        QCOMPARE(rb.capacity(), std::size_t{3});
        rb.push(1);
        rb.push(2);
        QCOMPARE(rb.size(), std::size_t{2});
        QCOMPARE(rb[0], 1);
        QCOMPARE(rb[1], 2);
        QCOMPARE(rb.latest(), 2);
    }

    void overwritesOldestWhenFull()
    {
        RingBuffer<int> rb(3);
        for (int i = 1; i <= 7; ++i) {
            rb.push(i);
        }
        QCOMPARE(rb.size(), std::size_t{3});
        QCOMPARE(rb[0], 5);
        QCOMPARE(rb[1], 6);
        QCOMPARE(rb[2], 7);
        QCOMPARE(rb.latest(), 7);
    }

    void clearResets()
    {
        RingBuffer<int> rb(2);
        rb.push(1);
        rb.push(2);
        rb.push(3);
        rb.clear();
        QVERIFY(rb.empty());
        rb.push(9);
        QCOMPARE(rb[0], 9);
        QCOMPARE(rb.latest(), 9);
    }

    void zeroCapacityIgnoresPushes()
    {
        RingBuffer<int> rb(0);
        rb.push(1);
        QVERIFY(rb.empty());
    }

    void capacityOne()
    {
        RingBuffer<float> rb(1);
        rb.push(0.25F);
        rb.push(0.5F);
        QCOMPARE(rb.size(), std::size_t{1});
        QCOMPARE(rb.latest(), 0.5F);
    }
};

} // namespace

QTEST_APPLESS_MAIN(TestRingBuffer)
#include "tst_ringbuffer.moc"
