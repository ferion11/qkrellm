// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/mounts.h"

#include "sensors/textscan.h"

#include <algorithm>
#include <limits>

namespace qk::sensors {

namespace {

bool isOctal(char c)
{
    return c >= '0' && c <= '7';
}

/// Undoes the kernel's octal escaping ("\040" -> ' ').
std::string unescape(std::string_view field)
{
    std::string out;
    out.reserve(field.size());
    for (std::size_t i = 0; i < field.size(); ++i) {
        if (field[i] == '\\' && i + 3 < field.size() && isOctal(field[i + 1]) &&
            isOctal(field[i + 2]) && isOctal(field[i + 3])) {
            const int value =
                ((field[i + 1] - '0') * 64) + ((field[i + 2] - '0') * 8) + (field[i + 3] - '0');
            if (value <= 0xff) { // "\777" is not a byte: keep it literally
                out += static_cast<char>(value);
                i += 3;
                continue;
            }
        }
        out += field[i];
    }
    return out;
}

std::uint64_t mulSaturating(std::uint64_t a, std::uint64_t b)
{
    if (a != 0 && b > std::numeric_limits<std::uint64_t>::max() / a) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return a * b;
}

} // namespace

std::vector<MountEntry> parseMounts(std::string_view text)
{
    std::vector<MountEntry> mounts;
    while (!text.empty()) {
        std::string_view line = nextLine(text);
        const std::string_view device = nextToken(line);
        const std::string_view mountPoint = nextToken(line);
        const std::string_view fsType = nextToken(line);
        if (fsType.empty()) {
            continue;
        }
        mounts.push_back({
            .device = unescape(device),
            .mountPoint = unescape(mountPoint),
            .fsType = std::string(fsType),
        });
    }
    return mounts;
}

std::vector<MountEntry> selectDiskMounts(const std::vector<MountEntry>& mounts)
{
    std::vector<MountEntry> selected;
    for (const MountEntry& m : mounts) {
        if (!m.device.starts_with("/dev/") || m.fsType == "squashfs") {
            continue;
        }
        const bool seen = std::ranges::any_of(
            selected, [&](const MountEntry& s) { return s.device == m.device; });
        if (!seen) {
            selected.push_back(m);
        }
    }
    return selected;
}

FsUsage fsUsage(std::uint64_t blocks, std::uint64_t freeBlocks, std::uint64_t availableBlocks,
                std::uint64_t blockSize)
{
    freeBlocks = std::min(freeBlocks, blocks);
    availableBlocks = std::min(availableBlocks, freeBlocks);
    const std::uint64_t usedBlocks = blocks - freeBlocks;
    FsUsage u;
    u.total = mulSaturating(blocks, blockSize);
    u.used = mulSaturating(usedBlocks, blockSize);
    const std::uint64_t denominator = usedBlocks + availableBlocks; // <= blocks: no overflow
    u.fraction =
        denominator == 0 ? 0.0 : static_cast<double>(usedBlocks) / static_cast<double>(denominator);
    return u;
}

} // namespace qk::sensors
