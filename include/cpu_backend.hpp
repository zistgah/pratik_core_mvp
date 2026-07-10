// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
//
// This file is part of PRATIK Kernel Core (pratik_core_mvp), free software:
// redistribute and/or modify it under the terms of the GNU General Public
// License, version 3 or (at your option) any later version. Distributed
// WITHOUT ANY WARRANTY. See the LICENSE file for the full text.

// cpu_backend.hpp — pure C++20 PRATIK kernel (cache-friendly, single memory space).
// © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
#pragma once
#include "kernel_common.hpp"
#include <vector>
#include <unordered_map>
#include <deque>

namespace pratik {

class CpuKernel final : public IKernel {
public:
    explicit CpuKernel(KernelConfig cfg, int d_init = 4);

    const char* name() const override { return "CPU"; }

    Step  ingest(const Event& e) override;
    void  tick() override;
    bool  maybe_spawn() override;

    int    D_active()  const override { return d_active_; }
    trit_t state(int d) const override { return state_[d]; }
    bool   provisional(int d) const override { return prov_[d]; }
    void   set_resources(double battery, double memory) override { battery_ = battery; memory_ = memory; }
    void   reset_baseline(const std::vector<trit_t>& s0) override;
    std::vector<trit_t> snapshot_states() const override {
        return std::vector<trit_t>(state_.begin(), state_.begin() + d_active_);
    }
    std::unique_ptr<IKernel> clone() const override {
        return std::make_unique<CpuKernel>(*this);
    }

    // convenience accessors for the harness
    double battery() const { return battery_; }
    double memory()  const { return memory_; }
    int    reinforcements(int d) const { return reinforce_[d]; }
    double persistence(int d) const { return Pi_[d]; }
    std::uint32_t last_spawn_token() const { return last_spawn_token_; }

private:
    double theta_c() const;                 // Axiom IV resource-modulated threshold
    void   spawn(std::uint32_t token);      // the autogenous spawning operator *

    KernelConfig cfg_;
    int d_active_ = 4;

    std::vector<trit_t> state_;             // S : active-dim ternary coordinates (pre-allocated D_max)
    std::vector<bool>   prov_;              // provisional (spawned, not yet crystallized)
    std::vector<double> Pi_;                // per-dim structural persistence
    std::vector<int>    reinforce_;         // reinforcement counts (drive resolution)

    std::unordered_map<std::uint32_t,int> grammar_;   // G : token -> dimension
    std::unordered_map<std::uint32_t,int> frustration_; // sign-frustration cycles per token

    std::deque<Event> ledger_;              // I : append-only ring buffer (capped)
    std::uint32_t     pending_spawn_token_ = 0;
    bool              spawn_requested_ = false;
    std::uint32_t     last_spawn_token_ = 0;

    double battery_ = 1.0;                   // E_battery
    double memory_  = 1.0;                    // M_memory
};

} // namespace pratik
