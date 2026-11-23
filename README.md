# qkrellm

A lightweight system monitor in the spirit of [GKrellM](http://gkrellm.srcbox.net/),
written in **Qt 6**: a compact stack of charts and meters for CPU, memory, network, disks
and sensors, extensible through a **stable C plugin API**. Linux only.

> **Status:** early development — M2 done: clock, CPU, load, memory/swap, disks,
> filesystems, network, temperatures/fans, battery and uptime, sampled once per second
> (≈ 2.8 MiB private memory, ≈ 0.15 % of one core). Configuration and themes arrive in M3.
> See [docs/roadmap.md](docs/roadmap.md).

## Try it

A development AppImage (x86_64) is rebuilt on every push to `main` and published as the
[`continuous-master` pre-release](https://github.com/ferion11/qkrellm/releases/tag/continuous-master):

```sh
chmod +x qkrellm-*-x86_64.AppImage && ./qkrellm-*-x86_64.AppImage
```

It is a snapshot of the current state, not a stable release.

## Goals

- **Frugal:** ≤ 16 MiB private memory, ≈ 0 % idle CPU, fast startup
  ([resource budget](docs/resource-budget.md)); links only Qt Core/Gui/Widgets.
- **Modern plugin API:** versioned C ABI usable from C, C++, Rust…, independent of Qt
  versions ([plugin API](docs/plugin-api.md)).
- **Simple, tested internals:** pure parsers tested against real kernel snapshots and fuzzed.

## Build

```sh
cmake --workflow --preset dev                       # Debug build + tests
cmake --preset release && cmake --build --preset release
/tmp/qkrellm/build/release/src/app/qkrellm
```

Build trees live in `/tmp/qkrellm/` — the source tree stays clean. Requirements and
workflow: [CONTRIBUTING.md](CONTRIBUTING.md) and [AGENTS.md](AGENTS.md).

## Documentation

- [Architecture](docs/architecture.md) · [Decisions (ADRs)](docs/adr/README.md) ·
  [Roadmap](docs/roadmap.md) · [Changelog](CHANGELOG.md) · [Security](SECURITY.md)

## License

GPL-3.0-or-later. See [LICENSE](LICENSE).
