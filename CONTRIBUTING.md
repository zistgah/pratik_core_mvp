# Contributing — PRATIK Kernel Core

© 1993–2026 Abhishek Choudhary. All rights reserved. · AyeAI · GPL-3.0-or-later

By contributing you agree your contribution is licensed under GPL-3.0-or-later.

## Ground rules (house conventions)
- **Verify by execution.** Build and run before proposing a change; paste the harness output.
- **No floating point in the `⊞`/`⊙` core.** Integer / LUT only; the `static_assert`s must hold.
- **The tests are the proofs.** New behaviour ⇒ a harness assertion mapping to an axiom/theorem.
- **Bounded substrate.** `D_active ≤ D_max`; never reallocate the global matrix in the loop.
- **fork → PR → review → merge.** PRs are loud (link the run output). Incremental patches only.
- **Preserve every file's SPDX + copyright header.**
- **No irreversible step** (DOI mint, release) without explicit maintainer confirmation.

## Build
```bash
./build.sh          # CPU (g++ -std=c++20)
./build.sh cuda     # CPU + CUDA (nvcc -arch=sm_80)
```
