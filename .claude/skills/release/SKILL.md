---
name: release
description: Cut a qkrellm release — SemVer bump, CHANGELOG, metainfo release entry, tag, and update of the Gentoo ebuild in the danrepo overlay. Use only when the maintainer asks to prepare or publish a release.
---

# Release qkrellm

Never push, tag or publish without explicit maintainer approval for that step.

1. Preconditions: on `main`, CI green, `cmake --workflow --preset dev` and `asan` pass,
   `benchmarks/measure.sh --check` passes; roadmap milestone items ticked.
2. Version (SemVer; app version is independent of `QK_ABI_VERSION_*`):
   - `project(qkrellm VERSION X.Y.Z)` in `CMakeLists.txt`
   - new `<release version="X.Y.Z" date="YYYY-MM-DD"/>` in
     `packaging/io.github.ferion11.qkrellm.metainfo.xml`
   - `CHANGELOG.md`: move Unreleased → `## [X.Y.Z] - YYYY-MM-DD`, update compare links.
3. Validate metadata if tools exist: `appstreamcli validate packaging/*.metainfo.xml`,
   `desktop-file-validate packaging/*.desktop`.
4. Commit `chore(release): X.Y.Z`, then (after approval) `git tag -s vX.Y.Z` and push.
5. Gentoo: copy `packaging/gentoo/qkrellm-9999.ebuild` to
   `/home/admlocal/git/danrepo/app-admin/qkrellm/qkrellm-X.Y.Z.ebuild` (non-live: replace git-r3
   with `SRC_URI` of the tag tarball, `KEYWORDS="~amd64"`), run
   `pkgdev manifest` (or `ebuild … manifest`) there, and commit in that repo separately.
6. GitHub release notes from the CHANGELOG section (`gh release create vX.Y.Z --notes-file …`).
