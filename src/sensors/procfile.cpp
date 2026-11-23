// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/procfile.h"

#include <cerrno>
#include <utility>

#include <fcntl.h>
#include <unistd.h>

namespace qk::sensors {

namespace {
// Hard ceiling so a misbehaving file cannot make us allocate without bound.
constexpr std::size_t kMaxCapacity = std::size_t{16} * 1024 * 1024;
} // namespace

ProcFile::ProcFile(const char* path, std::size_t initialCapacity)
    : m_fd(::open(path, O_RDONLY | O_CLOEXEC))
    , m_buffer(std::make_unique<char[]>(initialCapacity))
    , m_capacity(initialCapacity)
{}

ProcFile::~ProcFile()
{
    if (m_fd >= 0) {
        ::close(m_fd);
    }
}

ProcFile::ProcFile(ProcFile&& other) noexcept
    : m_fd(std::exchange(other.m_fd, -1))
    , m_buffer(std::move(other.m_buffer))
    , m_capacity(std::exchange(other.m_capacity, 0))
{}

ProcFile& ProcFile::operator=(ProcFile&& other) noexcept
{
    if (this != &other) {
        if (m_fd >= 0) {
            ::close(m_fd);
        }
        m_fd = std::exchange(other.m_fd, -1);
        m_buffer = std::move(other.m_buffer);
        m_capacity = std::exchange(other.m_capacity, 0);
    }
    return *this;
}

std::optional<std::string_view> ProcFile::read()
{
    if (m_fd < 0 || m_capacity == 0) {
        return std::nullopt;
    }
    for (;;) {
        std::size_t size = 0;
        // procfs may return a file in several chunks: keep reading until EOF.
        for (;;) {
            const ssize_t n =
                ::pread(m_fd, &m_buffer[size], m_capacity - size, static_cast<off_t>(size));
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return std::nullopt;
            }
            if (n == 0) {
                return std::string_view(m_buffer.get(), size);
            }
            size += static_cast<std::size_t>(n);
            if (size == m_capacity) {
                break; // buffer full: grow and re-read from offset 0 for a consistent copy
            }
        }
        if (m_capacity * 2 > kMaxCapacity) {
            return std::nullopt;
        }
        m_capacity *= 2;
        m_buffer = std::make_unique<char[]>(m_capacity);
    }
}

} // namespace qk::sensors
