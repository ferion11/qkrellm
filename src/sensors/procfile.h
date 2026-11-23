// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>
#include <optional>
#include <string_view>

namespace qk::sensors {

/// Re-readable kernel text file (/proc, /sys). Opens once (O_RDONLY | O_CLOEXEC) and
/// re-reads with pread(fd, …, 0) into an owned buffer, so steady-state reads do not
/// allocate. The buffer grows (doubling) only when a file no longer fits, e.g. once at
/// startup for a large /proc/stat.
class ProcFile
{
public:
    explicit ProcFile(const char* path, std::size_t initialCapacity = std::size_t{16} * 1024);
    ~ProcFile();
    ProcFile(const ProcFile&) = delete;
    ProcFile& operator=(const ProcFile&) = delete;
    ProcFile(ProcFile&& other) noexcept;
    ProcFile& operator=(ProcFile&& other) noexcept;

    [[nodiscard]] bool isOpen() const { return m_fd >= 0; }

    /// Reads the whole file. The view stays valid until the next read() or destruction.
    /// std::nullopt if the file is not open or a read error occurs.
    [[nodiscard]] std::optional<std::string_view> read();

    /// Current buffer capacity (exposed for tests).
    [[nodiscard]] std::size_t capacity() const { return m_capacity; }

private:
    int m_fd = -1;
    std::unique_ptr<char[]> m_buffer;
    std::size_t m_capacity = 0;
};

} // namespace qk::sensors
