// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
//
// This file is part of PRATIK Kernel Core (pratik_core_mvp), free software:
// redistribute and/or modify it under the terms of the GNU General Public
// License, version 3 or (at your option) any later version. Distributed
// WITHOUT ANY WARRANTY. See the LICENSE file for the full text.

// cpu_backend.cpp — PRATIK kernel state machine.
// © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
#include "cpu_backend.hpp"
#include <cmath>
#include <algorithm>

namespace pratik {

CpuKernel::CpuKernel(KernelConfig cfg, int d_init)
    : cfg_(cfg), d_active_(d_init),
      state_(cfg.D_max, 0), prov_(cfg.D_max, false),
      Pi_(cfg.D_max, 0.0), reinforce_(cfg.D_max, 0) {
    // seed the initial permanent substrate with a stable non-zero baseline,
    // and bind base tokens 0..d_init-1 into the grammar G (token d -> dim d).
    for (int d = 0; d < d_active_; ++d) {
        state_[d]   = (d % 2 == 0) ? +1 : -1;
        grammar_[static_cast<std::uint32_t>(d)] = d;
    }
}

double CpuKernel::theta_c() const {
    // θ_c = θ0 / (E_battery · M_memory) ; resources drop => threshold rises => conservative
    const double denom = std::max(1e-9, battery_ * memory_);
    return cfg_.theta0 / denom;
}

void CpuKernel::reset_baseline(const std::vector<trit_t>& s0) {
    for (int d = 0; d < static_cast<int>(s0.size()) && d < d_active_; ++d)
        state_[d] = s0[d];
}

// Route an event through the grammar, apply ⊞ into the target coordinate,
// and detect sign frustration (unmapped token, or a drive that cancels to 0).
Step CpuKernel::ingest(const Event& e) {
    if (ledger_.size() >= static_cast<std::size_t>(cfg_.ledger_cap)) ledger_.pop_front();
    ledger_.push_back(e);

    auto it = grammar_.find(e.token);
    if (it == grammar_.end()) {
        // unmapped projection -> accrue sign frustration (Axiom II precondition)
        int f = ++frustration_[e.token];
        if (f >= cfg_.frust_threshold) { spawn_requested_ = true; pending_spawn_token_ = e.token; }
        return Step{-1, 0};                         // no coordinate touched yet
    }

    const int d = it->second;
    const trit_t before = state_[d];
    // gate then accumulate: ⊙ routes the drive through an enable line (+1 = open),
    // ⊞ accumulates it into the coordinate (dominance/cancellation).
    const trit_t gated = gate(e.drive, /*enable=*/+1);
    const trit_t after = boxplus(before, gated);
    state_[d] = after;

    if (after == 0 && e.drive != 0) {
        // persistent cancellation on a mapped dim is also sign frustration
        int f = ++frustration_[e.token];
        if (f >= cfg_.frust_threshold) { spawn_requested_ = true; pending_spawn_token_ = e.token; }
    } else if (after != 0) {
        frustration_[e.token] = 0;                  // resolved -> reinforcement
        ++reinforce_[d];
    }
    return Step{d, after};
}

bool CpuKernel::maybe_spawn() {
    if (!spawn_requested_) return false;
    spawn_requested_ = false;
    spawn(pending_spawn_token_);
    return true;
}

// The autogenous spawning operator * : claim a provisional index inside the
// pre-allocated D_max boundary, without reallocating the global substrate.
void CpuKernel::spawn(std::uint32_t token) {
    if (d_active_ >= cfg_.D_max) return;            // bounded: Lyapunov guarantee
    const int d_new = d_active_++;                  // (CUDA path uses atomicAdd here)
    prov_[d_new]      = true;
    state_[d_new]     = 0;
    Pi_[d_new]        = cfg_.Pi0;
    reinforce_[d_new] = 0;
    grammar_[token]   = d_new;                      // future events on this token now resolve here
    frustration_[token] = 0;
    last_spawn_token_ = token;
}

// Axiom III (decay) + Axiom IV (crystallize / evaporate), one physical tick.
void CpuKernel::tick() {
    const double th = theta_c();
    for (int d = d_active_ - 1; d >= 0; --d) {
        if (!prov_[d]) continue;
        Pi_[d] *= std::exp(-cfg_.gamma * cfg_.dt);          // Π_{t+1} = Π_t e^{-γΔt}
        if (Pi_[d] >= th && reinforce_[d] >= 1) {
            prov_[d] = false;                               // crystallize into G (permanent)
        } else if (Pi_[d] < cfg_.Pi_floor) {
            // evaporate: collapse the top-most provisional dim cleanly (keeps indices contiguous)
            if (d == d_active_ - 1) {
                prov_[d] = false; state_[d] = 0; Pi_[d] = 0.0; reinforce_[d] = 0;
                --d_active_;
            }
        }
    }
}

} // namespace pratik
