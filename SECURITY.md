# Security policy

## Reporting

Please report vulnerabilities privately through GitHub's
[security advisories](https://github.com/ferion11/qkrellm/security/advisories/new) for this
repository, not in public issues. Expect an acknowledgement within a week.

## Scope and threat model

- qkrellm runs with the user's privileges and needs no elevated rights; it only reads
  world-readable files under `/proc` and `/sys`. It must never require setuid or capabilities.
- **Plugins are native code** loaded into the process and are fully trusted: install only
  plugins you trust. qkrellm loads plugins only from the user's data directory and the
  installation prefix, never from the current directory or world-writable paths.
- Parsers of kernel data are fuzzed; malformed input must never crash the application.
