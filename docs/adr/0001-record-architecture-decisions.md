# 0001 — Record architecture decisions as ADRs

- **Status:** Accepted
- **Date:** 2026-11-22

## Context

qkrellm will be developed largely across many independent sessions — human and AI agents —
that do not share memory. Decisions that live only in chat history get re-litigated or
silently broken.

## Decision

Every architecturally significant decision is written as a short ADR in `docs/adr/`,
numbered sequentially, indexed in `docs/adr/README.md`. Accepted ADRs are immutable; a
change of mind is a new ADR that supersedes the old one. `AGENTS.md` lists the invariants
that ADRs establish so agents see them without reading every ADR.

## Consequences

- Agents must check `docs/adr/` before proposing structural changes, and create an ADR when
  they introduce one (Definition of Done).
- A small overhead per decision; in exchange the "why" survives.
