// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
//
// This file is part of PRATIK Kernel Core (pratik_core_mvp), free software:
// redistribute and/or modify it under the terms of the GNU General Public
// License, version 3 or (at your option) any later version. Distributed
// WITHOUT ANY WARRANTY. See the LICENSE file for the full text.

// cuda_backend.cuh — CUDA-accelerated PRATIK backend (device declarations).
//
// NOTE: written compilation-ready to the spec and to mirror cpu_backend
// semantics. It requires an NVIDIA toolchain (nvcc, sm_80+) and has NOT been
// executed in the authoring container (no GPU). Build via CMake with
// -DPRATIK_ENABLE_CUDA=ON, or: nvcc -std=c++20 -arch=sm_80 --expt-relaxed-constexpr ...
//
// © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
#pragma once
#include "kernel_common.hpp"
#include <vector>

namespace pratik {

// Parallel per-event validation benchmark over asynchronous device streams.
// Each GPU thread processes one point-event partition (boxplus/gate) and
// atomically accumulates a checksum; prints device latency in microseconds.
void cuda_run_benchmark(const std::vector<Event>& stream);

// Device-resident kernel mirroring the CPU state machine. The state-space
// matrix and ledger live in CUDA Unified Memory (cudaMallocManaged); the
// spawning operator claims new dimension indices with atomicAdd on a device
// counter, inside the pre-allocated D_max boundary.
class CudaKernel final : public IKernel {
public:
    explicit CudaKernel(KernelConfig cfg, int d_init = 4);
    ~CudaKernel() override;

    const char* name() const override { return "CUDA"; }

    Step  ingest(const Event& e) override;   // enqueues onto the active stream
    void  tick() override;                    // host-orchestrated decay/consolidate over UM
    bool  maybe_spawn() override;

    int    D_active()  const override;
    trit_t state(int d) const override;
    bool   provisional(int d) const override;
    void   set_resources(double battery, double memory) override;
    void   reset_baseline(const std::vector<trit_t>& s0) override;
    std::vector<trit_t> snapshot_states() const override;
    std::unique_ptr<IKernel> clone() const override;

private:
    struct Impl;                              // pimpl hides CUDA types from host TUs
    Impl* p_;
};

} // namespace pratik
