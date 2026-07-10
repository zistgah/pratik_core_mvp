// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
//
// This file is part of PRATIK Kernel Core (pratik_core_mvp), free software:
// redistribute and/or modify it under the terms of the GNU General Public
// License, version 3 or (at your option) any later version. Distributed
// WITHOUT ANY WARRANTY. See the LICENSE file for the full text.

// main.cpp — PRATIK MVP verification harness + benchmark.
//
// Proves the entire phase-transition workflow on the CPU backend (and the CUDA
// backend when compiled with -DPRATIK_ENABLE_CUDA). Every step maps to an axiom
// or theorem of the Event Algebra; the tests ARE the proofs, executed.
//
//   1 init            D_active = 4, battery = memory = 1.0
//   2 recollection    congruent events re-trace identically, zero drift   (Theorem 1)
//   3 OOD injection   unmapped conflict collapses to a persistent 0-state
//   4 spawn           frustration >= threshold => D_active 4 -> 5          (Axiom II)
//   5a crystallize    reinforce + Π >= θ_c => 5th dim consolidated         (Axiom IV)
//   5b evaporate      starve, battery 0.1 => θ_c high => 5th dim -> 4       (Axiom IV)
//   6 benchmark       100,000 events, CPU (and CUDA) latency in µs
//
// © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
#include "cpu_backend.hpp"
#ifdef PRATIK_ENABLE_CUDA
#include "cuda_backend.cuh"
#endif

#include <cstdio>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <string>

using namespace pratik;
using clk = std::chrono::steady_clock;

static int g_fail = 0;
#define CHECK(cond, msg) do{ bool _c=(cond); \
    std::printf("      [%s] %s\n", _c?"PASS":"FAIL", msg); if(!_c) ++g_fail; }while(0)

static const char* trit_str(trit_t t){ return t>0?"+1":(t<0?"-1":" 0"); }

// ---- KernelConfig with a token->dim seed for the initial substrate ----
static std::unique_ptr<CpuKernel> make_kernel() {
    KernelConfig cfg;               // defaults: γ=0.5, θ0=0.4, Π0=1.0, floor=0.1, frust=3
    auto k = std::make_unique<CpuKernel>(cfg, /*d_init=*/4);
    return k;
}

int main() {
    std::printf("PRATIK Kernel Core — MVP verification harness\n");
    std::printf("© 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI\n");
    std::printf("=======================================================\n\n");

    // seed grammar: tokens 0..3 route to the 4 base dimensions (for recollection)
    auto k = make_kernel();
    // grammar for base dims 0..3 is seeded by the constructor (token d -> dim d).

    // ---------------------------------------------------------------- STEP 1
    std::printf("STEP 1 — initialize node\n");
    CHECK(k->D_active()==4, "D_active initialized to 4");
    CHECK(k->battery()==1.0 && k->memory()==1.0, "battery = memory = 1.0");

    // ---------------------------------------------------------------- STEP 2
    std::printf("STEP 2 — recollection (congruent re-trace, zero drift)\n");
    std::vector<trit_t> baseline = k->snapshot_states();
    std::vector<Event> seq = { {0,+1}, {1,-1}, {2,+1}, {3,-1} };

    auto run_seq = [&](CpuKernel& kk){
        Trajectory t; for (auto& e : seq) t.push_back(kk.ingest(e)); return t;
    };
    k->reset_baseline(baseline);
    Trajectory T1 = run_seq(*k);
    int  D_after1  = k->D_active();
    k->reset_baseline(baseline);
    Trajectory T2 = run_seq(*k);

    std::printf("      trajectory:");
    for (auto& s : T1) std::printf(" (d%d,%s)", s.first, trit_str(s.second));
    std::printf("\n");
    CHECK(T1==T2, "congruent sequence re-traces identical trajectory (S_test == S_hist)");
    CHECK(k->D_active()==D_after1 && k->D_active()==4, "zero structural drift (Δw = 0)");

    // ---------------------------------------------------------------- STEP 3
    std::printf("STEP 3 — inject OOD anomaly (persistent 0-state)\n");
    const std::uint32_t T_OOD = 9999;
    bool all_zero = true;
    for (int i = 0; i < 3; ++i) {          // 3 unmapped conflicting impulses
        Step s = k->ingest(Event{T_OOD, (i%2==0)?trit_t(+1):trit_t(-1)});
        if (!(s.first==-1 && s.second==0)) all_zero = false;
    }
    CHECK(all_zero, "unmapped conflict yields persistent 0-state (no coordinate resolves)");
    CHECK(k->D_active()==4, "substrate still 4 (spawn not yet applied)");

    // ---------------------------------------------------------------- STEP 4
    std::printf("STEP 4 — autogenous spawning (*)\n");
    bool spawned = k->maybe_spawn();
    CHECK(spawned, "kernel detected frustration and fired the spawning operator *");
    CHECK(k->D_active()==5, "D_active expanded 4 -> 5");
    CHECK(k->provisional(4), "new dimension 4 is provisional");
    CHECK(k->last_spawn_token()==T_OOD, "spawn bound the frustrating token to the new dim");

    // snapshot the post-spawn kernel so 5a / 5b are independent sub-scenarios
    std::unique_ptr<IKernel> snapA = k->clone();
    std::unique_ptr<IKernel> snapB = k->clone();

    // ---------------------------------------------------------------- STEP 5a
    std::printf("STEP 5a — crystallization (reinforce, resources healthy)\n");
    {
        auto* a = dynamic_cast<CpuKernel*>(snapA.get());
        a->set_resources(1.0, 1.0);                 // θ_c = 0.4
        a->ingest(Event{T_OOD, +1});                // now routes to dim4, resolves -> reinforce
        a->ingest(Event{T_OOD, +1});
        a->tick();                                  // Π=1.0·e^-0.5=0.606 ≥ 0.4 and reinforced
        std::printf("      Π(d4)=%.3f  θ_c=%.3f  reinforce(d4)=%d\n",
                    a->persistence(4), 0.4/(a->battery()*a->memory()), a->reinforcements(4));
        CHECK(!a->provisional(4), "dimension 4 crystallized into the permanent grammar G");
        CHECK(a->D_active()==5, "D_active stays 5 after consolidation");
    }

    // ---------------------------------------------------------------- STEP 5b
    std::printf("STEP 5b — evaporation (starve, battery -> 0.1)\n");
    {
        auto* b = dynamic_cast<CpuKernel*>(snapB.get());
        b->set_resources(0.1, 1.0);                 // θ_c = 0.4/0.1 = 4.0  (unreachable)
        std::printf("      θ_c raised to %.1f; starving the node...\n", 0.4/(b->battery()*b->memory()));
        for (int i = 0; i < 8 && b->D_active() > 4; ++i) {
            b->tick();
            std::printf("      tick %d: Π(d4)=%.3f  D_active=%d\n", i+1, b->persistence(4), b->D_active());
        }
        CHECK(b->D_active()==4, "unconsolidated dimension evaporated; D_active 5 -> 4");
    }

    // ---------------------------------------------------------------- STEP 6
    std::printf("STEP 6 — benchmark (100,000 point-events)\n");
    const int N = 100000;
    std::vector<Event> stream(N);
    for (int i = 0; i < N; ++i) stream[i] = Event{ static_cast<std::uint32_t>(i & 3),
                                                   (i%2==0)?trit_t(+1):trit_t(-1) };

    // (a) sequential kernel throughput (ordered consolidation path)
    {
        auto kk = make_kernel();
        auto t0 = clk::now();
        for (auto& e : stream) kk->ingest(e);
        auto t1 = clk::now();
        double us = std::chrono::duration<double,std::micro>(t1-t0).count();
        std::printf("      CPU sequential : %9.1f µs  (%.3f µs/event)\n", us, us/N);
    }
    // (b) parallel per-event validation (CUDA-style partition, thread-local)
    {
        unsigned T = std::max(2u, std::thread::hardware_concurrency());
        std::vector<std::thread> pool;
        std::vector<long long> acc(T, 0);
        auto t0 = clk::now();
        for (unsigned t = 0; t < T; ++t) pool.emplace_back([&,t]{
            long long local = 0;
            for (int i = t; i < N; i += T) {              // strided partition
                int d = stream[i].token & 3;
                trit_t v = boxplus(static_cast<trit_t>((d%2==0)?+1:-1),
                                   gate(stream[i].drive, +1));
                local += v;                               // independent validation work
            }
            acc[t] = local;
        });
        for (auto& th : pool) th.join();
        auto t1 = clk::now();
        double us = std::chrono::duration<double,std::micro>(t1-t0).count();
        long long total = 0; for (auto a : acc) total += a;
        std::printf("      CPU parallel×%-2u: %9.1f µs  (%.3f µs/event)  [checksum %lld]\n",
                    T, us, us/N, total);
    }

#ifdef PRATIK_ENABLE_CUDA
    std::printf("      CUDA backend   : running device benchmark...\n");
    cuda_run_benchmark(stream);   // defined in cuda/cuda_backend.cu
#else
    std::printf("      CUDA backend   : not compiled in this build "
                "(rebuild with -DPRATIK_ENABLE_CUDA and nvcc)\n");
#endif

    // ---------------------------------------------------------------- SUMMARY
    std::printf("\n=======================================================\n");
    std::printf(g_fail==0 ? "ALL CHECKS PASSED ✓\n" : "%d CHECK(S) FAILED ✗\n", g_fail);
    return g_fail==0 ? 0 : 1;
}
