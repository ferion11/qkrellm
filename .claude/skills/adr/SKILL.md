---
name: adr
description: Create a new Architecture Decision Record in docs/adr (numbered from the template) and index it. Use whenever a change introduces or alters an architectural decision — new dependency or Qt module, ABI policy, file formats, budget limits, platform behaviour — or when an existing ADR must be superseded.
---

# Write an ADR

1. Next number: `ls docs/adr | grep -E '^[0-9]{4}-' | sort | tail -1` → +1, four digits.
2. `cp docs/adr/0000-template.md docs/adr/NNNN-short-imperative-title.md`
3. Fill **Context** (forces, measurements, links), **Decision** (checkable against a diff),
   **Consequences** (what to watch, how enforced). Date = today. Status `Proposed` unless the
   maintainer already agreed (then `Accepted`).
4. Add a row to the table in `docs/adr/README.md`.
5. Superseding: set the old ADR's status to `Superseded by NNNN` (the only allowed edit to an
   accepted ADR) and reference it from the new one.
6. If the decision creates a new invariant, add it to the Invariants list in `AGENTS.md`.
7. Mention the ADR in the commit message body.
