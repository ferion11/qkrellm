// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <memory>

namespace qk::core {

/// Fixed-capacity history. Storage is allocated once in the constructor; push() never
/// allocates and overwrites the oldest value when full. Index 0 is the oldest sample.
template<typename T>
class RingBuffer
{
public:
    explicit RingBuffer(std::size_t capacity)
        : m_data(std::make_unique<T[]>(capacity))
        , m_capacity(capacity)
    {}

    void push(const T& value) noexcept
    {
        if (m_capacity == 0) {
            return;
        }
        m_data[m_head] = value;
        m_head = (m_head + 1) % m_capacity;
        if (m_size < m_capacity) {
            ++m_size;
        }
    }

    [[nodiscard]] std::size_t size() const noexcept { return m_size; }
    [[nodiscard]] std::size_t capacity() const noexcept { return m_capacity; }
    [[nodiscard]] bool empty() const noexcept { return m_size == 0; }

    /// Precondition: index < size(). 0 = oldest, size() - 1 = newest.
    [[nodiscard]] const T& operator[](std::size_t index) const noexcept
    {
        return m_data[(m_head + m_capacity - m_size + index) % m_capacity];
    }

    /// Precondition: !empty().
    [[nodiscard]] const T& latest() const noexcept { return (*this)[m_size - 1]; }

    void clear() noexcept
    {
        m_size = 0;
        m_head = 0;
    }

private:
    std::unique_ptr<T[]> m_data;
    std::size_t m_capacity = 0;
    std::size_t m_head = 0; // next write position
    std::size_t m_size = 0;
};

} // namespace qk::core
