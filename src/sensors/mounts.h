// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace qk::sensors {

/// One line of /proc/self/mounts: "device mountpoint fstype options dump pass".
/// Kernel: fs/proc_namespace.c (show_vfsmnt); spaces, tabs, newlines and backslashes in
/// the first two fields are escaped as octal "\040", "\011", "\012", "\134".
/// Fixtures: tests/fixtures/proc/<tag>/proc/mounts (real captures).
struct MountEntry
{
    std::string device;
    std::string mountPoint;
    std::string fsType;
};

/// Parses /proc/self/mounts. Lines with fewer than three fields are skipped. Allocates:
/// for startup discovery only.
[[nodiscard]] std::vector<MountEntry> parseMounts(std::string_view text);

/// Mounts worth showing: backed by a block device ("/dev/..."), not squashfs (snap and
/// image mounts), and each device only once (bind mounts and btrfs subvolumes repeat it;
/// the first mount wins). Order is preserved.
[[nodiscard]] std::vector<MountEntry> selectDiskMounts(const std::vector<MountEntry>& mounts);

/// Space usage of a filesystem, from statvfs(3) fields.
struct FsUsage
{
    std::uint64_t total = 0; ///< bytes
    std::uint64_t used = 0;  ///< bytes
    double fraction = 0.0;   ///< like df's Use%: used / (used + available to users)
};

/// Pure: computes FsUsage from statvfs fields (f_blocks, f_bfree, f_bavail, f_frsize),
/// saturating on inconsistent or overflowing values.
[[nodiscard]] FsUsage fsUsage(std::uint64_t blocks, std::uint64_t freeBlocks,
                              std::uint64_t availableBlocks, std::uint64_t blockSize);

} // namespace qk::sensors
