<!--
SPDX-License-Identifier: GPL-3.0-or-later
Copyright (C) 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
-->
# PRATIK Kernel Core — MVP

[![DOI](https://img.shields.io/badge/DOI-10.5281/zenodo.21288232-b8912f)](https://zenodo.org/record/21288232)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-294b8f.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-0b1e46)](CMakeLists.txt)

A resource-bounded, **balanced-ternary, event-driven** kernel — *not* a floating-point
neural network. Reference implementation of the **PRATIK** (Participatory Recursive Adaptive
Trans-Intelligence Kernel) core for the chapter *"Beyond Curve-Fitting"* (PEDLER / the AyeAI
Triad). **Live visualizer:** [zistgah.org/pratik_core_mvp](https://zistgah.org/pratik_core_mvp/).

> **What PEDLER/PRATIK is (do not assume):** a *developmental Event Algebra* in which
> experience reshapes the computational substrate itself — its grammar `G`, topology `S`,
> and dimensionality `D_active`. Statistical ML is a *degenerate boundary case* of it.
> See [`CONTRACT.md`](CONTRACT.md) before building on or commenting about this work.

© 1993–2026 Abhishek Choudhary. All rights reserved. · Affiliation: AyeAI.

## Contents
```
pratik_core_mvp/
├── index.html            interactive 3D state visualizer (dependency-free; Pages landing)
├── include/              trit.hpp · kernel_common.hpp · ikernel.hpp · cpu_backend.hpp · cuda_backend.cuh
├── src/                  cpu_backend.cpp · main.cpp (verification harness + benchmark)
├── cuda/                 cuda_backend.cu (Unified Memory · streams · atomicAdd)
├── CMakeLists.txt  build.sh
├── doi/misty.json        DOI deposit metadata (Misty DOI)
├── LICENSE  COPYING  NOTICE  AUTHORS  CONTRIBUTING.md  SECURITY.md  CHANGELOG.md
├── CONTRACT.md           how to engage with / handle this work (human + AI)
├── CONTEXT.md            cold-start map for a fresh AI/human session
├── CITATION.cff  codemeta.json
└── zistgah_seed_pratik.sh   seed the repo + mint DOI + OpenTimestamps
```

## Build & run
```bash
./build.sh            # CPU-only (g++ -std=c++20, no cmake needed) — builds and runs the harness
./build.sh cuda       # CPU + CUDA (nvcc -arch=sm_80 --expt-relaxed-constexpr)
# or:  cmake -B build -DPRATIK_ENABLE_CUDA=ON && cmake --build build && ./build/pratik_mvp
```

## Primitives (no floating point in the core path)
- `boxplus` (a ⊞ b): dominance/cancellation, saturating — `+1⊞-1=0`, `+1⊞+1=+1`, `0⊞x=x`.
- `gate` (a ⊙ b): multiplicative routing; `×0` ⇒ high-impedance (freezes the path).
- `*` (spawn): `D_active → D_active+1`, claims a provisional index inside `D_max` (CUDA: `atomicAdd`).
- `Π` (persistence): `Π_{t+1}=Π_t·e^{-γΔt}`; crystallize iff `Π ≥ θ_c = θ0/(E·M)` **and** reinforced;
  else evaporate (`Δd=-1`). Resources drop ⇒ `θ_c↑` ⇒ hyper-conservative.

## Verification harness — the tests *are* the proofs
| Step | Proves | Axiom/Thm |
|---|---|---|
| 1 | `D_active=4`, `battery=memory=1.0` | init |
| 2 | congruent events re-trace identically, zero drift | **Theorem 1** |
| 3 | unmapped conflict ⇒ persistent 0-state | Axiom II pre |
| 4 | frustration ≥ threshold ⇒ `D_active 4→5` | **Axiom II** (`*`) |
| 5a | reinforce + `Π≥θ_c` ⇒ 5th dim consolidated | **Axiom IV** |
| 5b | starve, `battery→0.1` ⇒ `θ_c` high ⇒ 5th dim → 4 | **Axiom IV** |
| 6 | 100,000 events — CPU (and CUDA) µs latency | benchmark |

Observed CPU run (g++ 13.3, x86-64): recollection re-traces identically (Δw=0); spawn 4→5;
crystallize holds at 5 (Π=0.607 ≥ θ_c=0.400); evaporate 5→4 under starvation;
~0.03–0.10 µs/event sequential. **ALL CHECKS PASSED ✓.**

## Status of the CUDA backend
Written compilation-ready to spec (Unified Memory, async streams, `atomicAdd` dimension
claim) and **not executed in the authoring environment** (no GPU). Build/run it on hardware
with `./build.sh cuda`.

## License
GNU General Public License v3.0 or later — see [LICENSE](LICENSE). Preserve SPDX and
copyright headers. See [NOTICE](NOTICE) for the author's copyright assertion.
