<!-- context: https://zistgah.org/pratik_core_mvp/ -->
# CONTEXT — cold-start map for PRATIK Kernel Core

_A fresh AI session or a new contributor rebuilds full working context from this file plus
the sources it points to. © 1993–2026 Abhishek Choudhary. All rights reserved. · AyeAI · GPL-3.0-or-later._

## Mission (one line)
A resource-bounded, balanced-ternary, **event-driven** kernel that learns by reshaping its
own substrate (autogenous dimensional spawning + resource-modulated crystallization) — the
executable core of PEDLER / PRATIK, in which statistical ML is a degenerate boundary case.

## Rebuild context from three sources, in order
1. **[`CONTRACT.md`](CONTRACT.md)** — what PEDLER/PRATIK *is* and how it must be handled.
   Read it first; do not assume this is a neural network.
2. **The chapter** — https://zistgah.org/chapter/ — the mathematics: Event Algebra `E`,
   the five axioms, Theorem 1 (recollection), the Degenerate Statistical Reduction, the
   Lyapunov stability proof, ternary silicon.
3. **This repository** — the executable proof. Start at `src/main.cpp` (the harness),
   then `src/cpu_backend.cpp` (the state machine), then `include/trit.hpp` (the primitives).
   `index.html` is the same state machine in JS with a 3D live view.

## The invariants (never violate)
- Alphabet is balanced ternary `T = {-1,0,+1}`; **no floating point** in the `⊞`/`⊙` core
  (`static_assert`-checked at compile time).
- Core tuple `P = (I, G, U, S, F, *)`; the substrate is dynamic: `D_active(t)` grows via `*`
  and shrinks via evaporation, always `≤ D_max` (bounded — the Lyapunov guarantee).
- Persistence `Π_{t+1}=Π_t e^{-γΔt}`; threshold `θ_c = θ0/(E·M)` is a *physical* constraint,
  monotone in resources — starvation ⇒ provably more conservative.
- Recollection is exact re-trace from `s0` with `Δw = 0` (Theorem 1), not statistical convergence.
- The tests are the proofs — every behaviour maps to an axiom/theorem in the harness.

## File map
| Path | Role |
|---|---|
| `include/trit.hpp` | `trit_t`, `⊞`, `⊙` (+ compile-time proofs) |
| `include/kernel_common.hpp` | `Event`, `KernelConfig`, `IKernel` interface |
| `include/cpu_backend.hpp` / `src/cpu_backend.cpp` | the CPU state machine (source of truth) |
| `include/cuda_backend.cuh` / `cuda/cuda_backend.cu` | CUDA backend (compilation-ready, **not run** here) |
| `src/main.cpp` | verification harness + 100k-event benchmark |
| `index.html` | dependency-free JS port + 3D visualizer (Pages landing) |
| `doi/misty.json` | DOI deposit metadata |
| `zistgah_seed_pratik.sh` | seed repo + mint DOI + OpenTimestamps |

## Working rules
- fork → PR → review → merge; PRs loud (link the harness output); incremental patches only.
- Verify by execution; honest figures; flag, don't fake. Preserve SPDX + copyright headers.
- No irreversible step (DOI mint, release) without a typed confirmation; tokens from env only.

## Axiom ↔ test cross-reference
`Theorem 1` → harness STEP 2 · `Axiom II (*)` → STEP 3–4 · `Axiom III/IV (Π,θ_c)` → STEP 5a/5b.
