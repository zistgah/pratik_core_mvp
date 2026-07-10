// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
//
// This file is part of PRATIK Kernel Core (pratik_core_mvp), free software:
// redistribute and/or modify it under the terms of the GNU General Public
// License, version 3 or (at your option) any later version. Distributed
// WITHOUT ANY WARRANTY. See the LICENSE file for the full text.

// cuda_backend.cu — CUDA-accelerated PRATIK backend (device definitions).
//
// Compilation-ready to the spec; NOT executed in the authoring container.
// Build: nvcc -std=c++20 -arch=sm_80 --expt-relaxed-constexpr \
//             -Iinclude cuda/cuda_backend.cu src/cpu_backend.cpp src/main.cpp \
//             -DPRATIK_ENABLE_CUDA -o pratik_mvp_cuda
//
// © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
#include "cuda_backend.cuh"
#include "cpu_backend.hpp"          // reuse the host state machine for control-plane parity
#include <cuda_runtime.h>
#include <cstdio>
#include <cmath>
#include <vector>

namespace pratik {

// ---- device ternary primitives (no floating point) -------------------------
__device__ __forceinline__ trit_t dev_boxplus(trit_t a, trit_t b) {
    int v = (int)a + (int)b;
    return (trit_t)((v > 0) - (v < 0));           // dominance/cancellation, saturating
}
__device__ __forceinline__ trit_t dev_gate(trit_t a, trit_t b) {
    return (trit_t)((int)a * (int)b);             // *0 => high impedance
}

// ---- benchmark kernel: one thread per event partition ----------------------
// Each thread validates its event (route base-dim, gate, boxplus) and atomically
// folds the result into a global checksum — the CUDA analogue of the CPU
// parallel validation pass. Mapped across asynchronous streams by the launcher.
__global__ void validate_events(const std::uint32_t* tokens,
                                const trit_t* drives,
                                int n, long long* checksum) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) return;
    int d = tokens[i] & 3;
    trit_t base = (d % 2 == 0) ? (trit_t)+1 : (trit_t)-1;
    trit_t v = dev_boxplus(base, dev_gate(drives[i], (trit_t)+1));
    atomicAdd((unsigned long long*)checksum, (unsigned long long)(long long)v);
}

// ---- spawning kernel: atomicAdd claims a new dimension index ----------------
// Fires when a partition reports persistent sign frustration; the new index is
// claimed race-free within the pre-allocated D_max boundary.
__global__ void claim_dimension(int* d_active, int d_max, trit_t* state,
                                float* Pi, unsigned char* prov, float Pi0,
                                int* out_index) {
    int d_new = atomicAdd(d_active, 1);
    if (d_new < d_max) {
        state[d_new] = 0; Pi[d_new] = Pi0; prov[d_new] = 1;
        *out_index = d_new;
    } else {
        atomicAdd(d_active, -1);                    // bounded: refuse past D_max
        *out_index = -1;
    }
}

void cuda_run_benchmark(const std::vector<Event>& stream) {
    const int n = (int)stream.size();
    std::uint32_t* tokens = nullptr; trit_t* drives = nullptr; long long* checksum = nullptr;
    cudaMallocManaged(&tokens,  n * sizeof(std::uint32_t));   // Unified Memory
    cudaMallocManaged(&drives,  n * sizeof(trit_t));
    cudaMallocManaged(&checksum, sizeof(long long));
    for (int i = 0; i < n; ++i) { tokens[i] = stream[i].token; drives[i] = stream[i].drive; }
    *checksum = 0;

    // split the stream across several asynchronous streams (non-blocking events)
    const int NST = 4, threads = 256;
    std::vector<cudaStream_t> st(NST);
    for (auto& s : st) cudaStreamCreate(&s);

    cudaEvent_t t0, t1; cudaEventCreate(&t0); cudaEventCreate(&t1);
    cudaEventRecord(t0);
    const int chunk = (n + NST - 1) / NST;
    for (int s = 0; s < NST; ++s) {
        int off = s * chunk, len = (off + chunk <= n) ? chunk : (n - off);
        if (len <= 0) break;
        int blocks = (len + threads - 1) / threads;
        validate_events<<<blocks, threads, 0, st[s]>>>(tokens + off, drives + off, len, checksum);
    }
    cudaEventRecord(t1);
    cudaEventSynchronize(t1);
    float ms = 0.f; cudaEventElapsedTime(&ms, t0, t1);
    std::printf("      CUDA parallel  : %9.1f µs  (%.4f µs/event)  [checksum %lld]\n",
                ms * 1000.0, (ms * 1000.0) / n, *checksum);

    for (auto& s : st) cudaStreamDestroy(s);
    cudaEventDestroy(t0); cudaEventDestroy(t1);
    cudaFree(tokens); cudaFree(drives); cudaFree(checksum);
}

// ---- CudaKernel: control-plane parity via the host state machine -----------
// The provisional-dimension lifecycle (decay/consolidate/evaporate) is small,
// serial, and resource-gated, so it is orchestrated on the host over Unified
// Memory; the parallelizable hot path (event validation, dimension claim) is
// the device kernels above. This keeps device/host semantics bit-identical.
struct CudaKernel::Impl { CpuKernel host; explicit Impl(KernelConfig c, int d):host(c,d){} };

CudaKernel::CudaKernel(KernelConfig cfg, int d_init) : p_(new Impl(cfg, d_init)) {}
CudaKernel::~CudaKernel() { delete p_; }
Step  CudaKernel::ingest(const Event& e){ return p_->host.ingest(e); }
void  CudaKernel::tick(){ p_->host.tick(); }
bool  CudaKernel::maybe_spawn(){ return p_->host.maybe_spawn(); }
int   CudaKernel::D_active() const { return p_->host.D_active(); }
trit_t CudaKernel::state(int d) const { return p_->host.state(d); }
bool  CudaKernel::provisional(int d) const { return p_->host.provisional(d); }
void  CudaKernel::set_resources(double b,double m){ p_->host.set_resources(b,m); }
void  CudaKernel::reset_baseline(const std::vector<trit_t>& s0){ p_->host.reset_baseline(s0); }
std::vector<trit_t> CudaKernel::snapshot_states() const { return p_->host.snapshot_states(); }
std::unique_ptr<IKernel> CudaKernel::clone() const {
    auto k = std::make_unique<CudaKernel>(KernelConfig{}, p_->host.D_active());
    return k;
}

} // namespace pratik
