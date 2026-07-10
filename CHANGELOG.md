# Changelog — PRATIK Kernel Core

© 1993–2026 Abhishek Choudhary. All rights reserved. · AyeAI · GPL-3.0-or-later

## [1.0.0] — 2026
Initial public reference MVP of the PRATIK kernel core.

- Balanced-ternary primitives `⊞`/`⊙` with compile-time `static_assert` proofs (no FP in the core).
- Core tuple `(I,G,U,S,F,*)`: ledger, grammar routing, pre-allocated `D_max` substrate, dynamic `D_active`.
- Autogenous spawning operator `*`; resource-modulated persistence `Π`, threshold `θ_c = θ0/(E·M)`.
- Crystallization and evaporation (Axiom IV).
- CPU backend (built + verified). CUDA backend (Unified Memory, streams, atomicAdd) — compilation-ready.
- Verification harness proving Theorem 1 (recollection) and Axioms II/IV; 100k-event benchmark.
- Dependency-free interactive 3D visualizer (`index.html`).
