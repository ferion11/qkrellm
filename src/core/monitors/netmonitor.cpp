// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/monitors/netmonitor.h"

#include <sys/stat.h>

namespace qk::core {

namespace {

bool hasDevice(const char* sysClassNet, std::string_view name)
{
    const std::string path = std::string(sysClassNet) + "/" + std::string(name) + "/device";
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

} // namespace

NetMonitor::NetMonitor(MetricRegistry& registry, const char* path, const char* sysClassNet)
    : m_registry(&registry)
    , m_file(path, std::size_t{4} * 1024)
    , m_current(std::make_unique<sensors::NetDev>())
    , m_previousTime(Clock::now())
{
    if (!readInto(*m_current)) {
        return;
    }
    m_valid = true;
    const sensors::NetDev& dev = *m_current;

    std::vector<std::size_t> chosen;
    for (std::size_t i = 0; i < dev.count; ++i) {
        const auto name = dev.interfaces.at(i).name();
        if (name != "lo" && hasDevice(sysClassNet, name)) {
            chosen.push_back(i);
        }
    }
    if (chosen.empty()) {
        for (std::size_t i = 0; i < dev.count; ++i) {
            if (dev.interfaces.at(i).name() != "lo") {
                chosen.push_back(i);
            }
        }
    }

    for (const std::size_t i : chosen) {
        const sensors::NetInterface& itf = dev.interfaces.at(i);
        const std::string name(itf.name());
        const auto add = [&](const char* suffix, const std::string& label) {
            return registry.add({
                .id = "net." + name + "." + suffix,
                .label = label,
                .unit = MetricUnit::BytesPerSecond,
            });
        };
        m_interfaces.push_back({
            .name = name,
            .rx = add("rx", name + " rx"),
            .tx = add("tx", name + " tx"),
            .total = add("total", name),
        });
        m_previous.push_back({.rx = itf.rxBytes, .tx = itf.txBytes, .present = true});
    }
}

NetMonitor::~NetMonitor() = default;

bool NetMonitor::readInto(sensors::NetDev& out)
{
    const auto text = m_file.read();
    if (!text) {
        return false;
    }
    auto parsed = sensors::parseNetDev(*text);
    if (!parsed) {
        return false;
    }
    out = *parsed;
    return true;
}

void NetMonitor::sample()
{
    sampleAt(Clock::now());
}

void NetMonitor::sampleAt(Clock::time_point now)
{
    if (!m_valid || !readInto(*m_current)) {
        return; // charts simply get no new point
    }
    const double seconds = std::chrono::duration<double>(now - m_previousTime).count();
    m_previousTime = now;

    for (std::size_t k = 0; k < m_interfaces.size(); ++k) {
        const Interface& itf = m_interfaces[k];
        Counters& previous = m_previous[k];
        const auto index = m_current->find(itf.name);
        double rx = 0.0;
        double tx = 0.0;
        if (index) {
            const sensors::NetInterface& current = m_current->interfaces.at(*index);
            if (previous.present) {
                rx = sensors::counterRate(previous.rx, current.rxBytes, seconds);
                tx = sensors::counterRate(previous.tx, current.txBytes, seconds);
            }
            previous = {.rx = current.rxBytes, .tx = current.txBytes, .present = true};
        } else {
            previous.present = false; // gone: next appearance starts a new baseline
        }
        m_registry->push(itf.rx, static_cast<float>(rx));
        m_registry->push(itf.tx, static_cast<float>(tx));
        m_registry->push(itf.total, static_cast<float>(rx + tx));
    }
}

} // namespace qk::core
