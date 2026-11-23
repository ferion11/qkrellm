// SPDX-License-Identifier: GPL-3.0-or-later
#include "sensors/hwmon.h"

#include "sensors/textscan.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string_view>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

namespace qk::sensors {

namespace {

constexpr std::size_t kSmallFile = 256; // name/label attributes are short

/// First line of a small sysfs text file, trimmed; nullopt if unreadable or empty.
std::optional<std::string> readLine(const std::string& path)
{
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return std::nullopt;
    }
    std::array<char, kSmallFile> buffer{};
    const ssize_t n = ::read(fd, buffer.data(), buffer.size());
    ::close(fd);
    if (n <= 0) {
        return std::nullopt;
    }
    std::string_view text(buffer.data(), static_cast<std::size_t>(n));
    text = nextLine(text);
    const auto first = text.find_first_not_of(" \t");
    const auto last = text.find_last_not_of(" \t\r");
    if (first == std::string_view::npos) {
        return std::nullopt;
    }
    return std::string(text.substr(first, last - first + 1));
}

/// "hwmon12" -> 12; "temp3_input" with prefix "temp" -> 3.
std::optional<int> numberAfter(std::string_view text, std::string_view prefix,
                               std::string_view suffix)
{
    if (!text.starts_with(prefix) || !text.ends_with(suffix) ||
        text.size() <= prefix.size() + suffix.size()) {
        return std::nullopt;
    }
    const auto digits = text.substr(prefix.size(), text.size() - prefix.size() - suffix.size());
    const auto value = parseUnsigned(digits);
    if (!value || *value > static_cast<std::uint64_t>(INT_MAX)) {
        return std::nullopt;
    }
    return static_cast<int>(*value);
}

std::vector<std::string> listDir(const std::string& path)
{
    std::vector<std::string> names;
    const std::unique_ptr<DIR, int (*)(DIR*)> dir(::opendir(path.c_str()), ::closedir);
    if (!dir) {
        return names;
    }
    while (const dirent* entry = ::readdir(dir.get())) {
        const std::string_view name(entry->d_name);
        if (name != "." && name != "..") {
            names.emplace_back(name);
        }
    }
    return names;
}

std::string deviceName(const std::string& hwmonDir)
{
    std::array<char, PATH_MAX> resolved{};
    if (::realpath((hwmonDir + "/device").c_str(), resolved.data()) == nullptr) {
        return {};
    }
    const std::string_view path(resolved.data());
    const auto slash = path.find_last_of('/');
    return std::string(slash == std::string_view::npos ? path : path.substr(slash + 1));
}

} // namespace

std::vector<HwmonSensor> discoverHwmon(const std::string& root)
{
    std::vector<HwmonSensor> sensors;
    for (const std::string& entry : listDir(root)) {
        const auto hwmon = numberAfter(entry, "hwmon", "");
        if (!hwmon) {
            continue;
        }
        const std::string dir = root + "/" + entry;
        const auto chip = readLine(dir + "/name");
        if (!chip) {
            continue;
        }
        const std::string device = deviceName(dir);
        for (const std::string& file : listDir(dir)) {
            for (const auto kind : {HwmonSensor::Kind::Temperature, HwmonSensor::Kind::Fan}) {
                const std::string_view type =
                    kind == HwmonSensor::Kind::Temperature ? "temp" : "fan";
                const auto channel = numberAfter(file, type, "_input");
                if (!channel) {
                    continue;
                }
                const std::string base = std::string(type) + std::to_string(*channel);
                sensors.push_back({
                    .kind = kind,
                    .hwmon = *hwmon,
                    .channel = *channel,
                    .chip = *chip,
                    .device = device,
                    .label = readLine(dir + "/" + base + "_label").value_or(base),
                    .inputPath = dir + "/" + file,
                });
            }
        }
    }
    std::ranges::sort(sensors, [](const HwmonSensor& a, const HwmonSensor& b) {
        if (a.hwmon != b.hwmon) {
            return a.hwmon < b.hwmon;
        }
        if (a.kind != b.kind) {
            return a.kind < b.kind;
        }
        return a.channel < b.channel;
    });
    return sensors;
}

} // namespace qk::sensors
